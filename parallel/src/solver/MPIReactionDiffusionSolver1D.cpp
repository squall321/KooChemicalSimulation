/**
 * @file MPIReactionDiffusionSolver1D.cpp
 * @brief Implementation of 1D MPI-parallel reaction-diffusion solver
 */

#include "parallel/solver/MPIReactionDiffusionSolver.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace koo {
namespace parallel {
namespace solver {

MPIReactionDiffusionSolver1D::MPIReactionDiffusionSolver1D(
    int globalNx, double L,
    double Du, double Dv,
    ReactionFunction reactionU,
    ReactionFunction reactionV,
    const mpi::MPIComm& comm)
    : comm_(comm),
      rank_(comm.getRank()),
      nprocs_(comm.getSize()),
      globalNx_(globalNx),
      L_(L),
      dx_(L / (globalNx - 1)),
      Du_(Du),
      Dv_(Dv),
      reactionU_(reactionU),
      reactionV_(reactionV),
      bcType_(BCType::Neumann),
      bcULeft_(0.0), bcURight_(0.0),
      bcVLeft_(0.0), bcVRight_(0.0),
      currentTime_(0.0) {

    setupDomain();

    int totalSize = localNx_ + 2;
    u_.resize(totalSize, 0.0);
    v_.resize(totalSize, 0.0);
    u_new_.resize(totalSize, 0.0);
    v_new_.resize(totalSize, 0.0);
    x_.resize(totalSize);

    for (int i = 0; i < totalSize; ++i) {
        int globalIdx = localStart_ - 1 + i;
        x_[i] = globalIdx * dx_;
    }
}

void MPIReactionDiffusionSolver1D::setupDomain() {
    int pointsPerProc = globalNx_ / nprocs_;
    int remainder = globalNx_ % nprocs_;

    localStart_ = rank_ * pointsPerProc + std::min(rank_, remainder);
    localEnd_ = localStart_ + pointsPerProc + (rank_ < remainder ? 1 : 0);
    localNx_ = localEnd_ - localStart_;

    leftNeighbor_ = (rank_ > 0) ? rank_ - 1 : -1;
    rightNeighbor_ = (rank_ < nprocs_ - 1) ? rank_ + 1 : -1;
}

void MPIReactionDiffusionSolver1D::setInitialCondition(
    std::function<double(double)> initialU,
    std::function<double(double)> initialV) {

    for (int i = 1; i <= localNx_; ++i) {
        u_[i] = initialU(x_[i]);
        v_[i] = initialV(x_[i]);
    }

    exchangeGhostCells();
}

void MPIReactionDiffusionSolver1D::setDirichletBC(
    double uLeft, double uRight, double vLeft, double vRight) {
    bcType_ = BCType::Dirichlet;
    bcULeft_ = uLeft;
    bcURight_ = uRight;
    bcVLeft_ = vLeft;
    bcVRight_ = vRight;
}

void MPIReactionDiffusionSolver1D::setNeumannBC(
    double uFluxLeft, double uFluxRight,
    double vFluxLeft, double vFluxRight) {
    bcType_ = BCType::Neumann;
    bcULeft_ = uFluxLeft;
    bcURight_ = uFluxRight;
    bcVLeft_ = vFluxLeft;
    bcVRight_ = vFluxRight;
}

void MPIReactionDiffusionSolver1D::exchangeGhostCells() {
    auto start = std::chrono::high_resolution_clock::now();

#ifdef USE_MPI
    const int tag_u = 0;
    const int tag_v = 1;

    if (leftNeighbor_ >= 0) {
        comm_.send(&u_[1], 1, leftNeighbor_, tag_u);
        comm_.send(&v_[1], 1, leftNeighbor_, tag_v);
    }
    if (rightNeighbor_ >= 0) {
        comm_.recv(&u_[localNx_ + 1], 1, rightNeighbor_, tag_u);
        comm_.recv(&v_[localNx_ + 1], 1, rightNeighbor_, tag_v);
    }

    if (rightNeighbor_ >= 0) {
        comm_.send(&u_[localNx_], 1, rightNeighbor_, tag_u);
        comm_.send(&v_[localNx_], 1, rightNeighbor_, tag_v);
    }
    if (leftNeighbor_ >= 0) {
        comm_.recv(&u_[0], 1, leftNeighbor_, tag_u);
        comm_.recv(&v_[0], 1, leftNeighbor_, tag_v);
    }
#endif

    auto end = std::chrono::high_resolution_clock::now();
    stats_.communicationTime += std::chrono::duration<double>(end - start).count();
    stats_.nGhostExchanges++;
}

void MPIReactionDiffusionSolver1D::applyBoundaryConditions() {
    if (bcType_ == BCType::Dirichlet) {
        if (rank_ == 0) {
            u_[0] = bcULeft_;
            v_[0] = bcVLeft_;
        }
        if (rank_ == nprocs_ - 1) {
            u_[localNx_ + 1] = bcURight_;
            v_[localNx_ + 1] = bcVRight_;
        }
    } else {  // Neumann
        if (rank_ == 0) {
            u_[0] = u_[1] - bcULeft_ * dx_;
            v_[0] = v_[1] - bcVLeft_ * dx_;
        }
        if (rank_ == nprocs_ - 1) {
            u_[localNx_ + 1] = u_[localNx_] + bcURight_ * dx_;
            v_[localNx_ + 1] = v_[localNx_] + bcVRight_ * dx_;
        }
    }
}

void MPIReactionDiffusionSolver1D::stepExplicit(double dt) {
    auto stepStart = std::chrono::high_resolution_clock::now();

    exchangeGhostCells();
    applyBoundaryConditions();

    auto compStart = std::chrono::high_resolution_clock::now();

    double dx2_inv = 1.0 / (dx_ * dx_);

    for (int i = 1; i <= localNx_; ++i) {
        // Laplacians
        double laplacian_u = (u_[i+1] - 2.0 * u_[i] + u_[i-1]) * dx2_inv;
        double laplacian_v = (v_[i+1] - 2.0 * v_[i] + v_[i-1]) * dx2_inv;

        // Reactions
        double react_u = reactionU_(u_[i], v_[i]);
        double react_v = reactionV_(u_[i], v_[i]);

        // Update
        double du_dt = Du_ * laplacian_u + react_u;
        double dv_dt = Dv_ * laplacian_v + react_v;

        u_new_[i] = u_[i] + dt * du_dt;
        v_new_[i] = v_[i] + dt * dv_dt;
    }

    std::swap(u_, u_new_);
    std::swap(v_, v_new_);

    auto compEnd = std::chrono::high_resolution_clock::now();
    stats_.computeTime += std::chrono::duration<double>(compEnd - compStart).count();

    currentTime_ += dt;
    stats_.nTimeSteps++;

    auto stepEnd = std::chrono::high_resolution_clock::now();
    stats_.totalTime += std::chrono::duration<double>(stepEnd - stepStart).count();
}

void MPIReactionDiffusionSolver1D::solve(double dt, int nSteps, int outputInterval) {
    if (comm_.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "MPI Reaction-Diffusion Solver (1D)\n";
        std::cout << "========================================\n";
        std::cout << "Global grid:      " << globalNx_ << "\n";
        std::cout << "Processes:        " << nprocs_ << "\n";
        std::cout << "Domain:           [0, " << L_ << "] m\n";
        std::cout << "Grid spacing:     " << dx_ << " m\n";
        std::cout << "Diffusivity:      Du=" << Du_ << ", Dv=" << Dv_ << " m²/s\n";
        std::cout << "Time step:        " << dt << " s\n";
        std::cout << "Total steps:      " << nSteps << "\n";
        std::cout << "CFL number:       " << checkCFL(dt);
        if (checkCFL(dt) >= 0.5) {
            std::cout << " (WARNING: > 0.5!)";
        }
        std::cout << "\n========================================\n";
    }

    for (int step = 0; step < nSteps; ++step) {
        stepExplicit(dt);

        if (outputInterval > 0 && step % outputInterval == 0) {
            if (comm_.isRoot()) {
                std::cout << "Step " << std::setw(6) << step << " / " << nSteps
                          << "  t = " << std::fixed << std::setprecision(6)
                          << currentTime_ << " s\n";
            }
        }
    }

    if (comm_.isRoot()) {
        std::cout << "\nSimulation complete!\n";
    }

    stats_.print(comm_);
}

void MPIReactionDiffusionSolver1D::gatherGlobalSolution(
    std::vector<double>& globalU, std::vector<double>& globalV,
    std::vector<double>& globalX) {

    auto start = std::chrono::high_resolution_clock::now();

    if (comm_.isRoot()) {
        globalU.resize(globalNx_);
        globalV.resize(globalNx_);
        globalX.resize(globalNx_);
    }

    std::vector<int> sizes(nprocs_);
    std::vector<int> offsets(nprocs_);
    int mySize = localNx_;

    comm_.allGather(&mySize, 1, sizes.data(), 1);

    int totalSize = 0;
    for (int i = 0; i < nprocs_; ++i) {
        offsets[i] = totalSize;
        totalSize += sizes[i];
    }

    std::vector<double> localU(localNx_), localV(localNx_), localX(localNx_);
    for (int i = 0; i < localNx_; ++i) {
        localU[i] = u_[i + 1];
        localV[i] = v_[i + 1];
        localX[i] = x_[i + 1];
    }

#ifdef USE_MPI
    MPI_Gatherv(localU.data(), localNx_, MPI_DOUBLE,
                globalU.data(), sizes.data(), offsets.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Gatherv(localV.data(), localNx_, MPI_DOUBLE,
                globalV.data(), sizes.data(), offsets.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Gatherv(localX.data(), localNx_, MPI_DOUBLE,
                globalX.data(), sizes.data(), offsets.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);
#else
    if (comm_.isRoot()) {
        globalU = localU;
        globalV = localV;
        globalX = localX;
    }
#endif

    auto end = std::chrono::high_resolution_clock::now();
    stats_.ioTime += std::chrono::duration<double>(end - start).count();
}

double MPIReactionDiffusionSolver1D::checkCFL(double dt) const {
    double cfl_u = Du_ * dt / (dx_ * dx_);
    double cfl_v = Dv_ * dt / (dx_ * dx_);
    return std::max(cfl_u, cfl_v);
}

} // namespace solver
} // namespace parallel
} // namespace koo
