/**
 * @file MPIDiffusionSolver1DOptimized.cpp
 * @brief Implementation of optimized MPI diffusion solver
 */

#include "parallel/solver/MPIDiffusionSolverOptimized.h"
#include "physics/diffusion/DiffusionCoefficient.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace parallel {
namespace solver {

MPIDiffusionSolver1DOptimized::MPIDiffusionSolver1DOptimized(
    int globalNx, double L, double diffusivity,
    const mpi::MPIComm& comm)
    : comm_(comm),
      rank_(comm.getRank()),
      nprocs_(comm.getSize()),
      globalNx_(globalNx),
      L_(L),
      dx_(L / (globalNx - 1)),
      diffusivity_(diffusivity),
      bcType_(BCType::Neumann),
      bcLeft_(0.0),
      bcRight_(0.0),
      currentTime_(0.0),
      overlapTime_(0.0),
      totalCommTime_(0.0) {

    if (globalNx < nprocs_) {
        throw std::runtime_error("Global grid size must be >= number of processes");
    }

    setupDomain();

    asyncComm_ = std::make_unique<mpi::MPIAsyncComm>(comm);

    int totalSize = localNx_ + 2;
    C_.resize(totalSize, 0.0);
    C_new_.resize(totalSize, 0.0);
    x_.resize(totalSize);

    for (int i = 0; i < totalSize; ++i) {
        int globalIdx = localStart_ - 1 + i;
        x_[i] = globalIdx * dx_;
    }
}

void MPIDiffusionSolver1DOptimized::setupDomain() {
    int pointsPerProc = globalNx_ / nprocs_;
    int remainder = globalNx_ % nprocs_;

    localStart_ = rank_ * pointsPerProc + std::min(rank_, remainder);
    localEnd_ = localStart_ + pointsPerProc + (rank_ < remainder ? 1 : 0);
    localNx_ = localEnd_ - localStart_;

    leftNeighbor_ = (rank_ > 0) ? rank_ - 1 : -1;
    rightNeighbor_ = (rank_ < nprocs_ - 1) ? rank_ + 1 : -1;
}

void MPIDiffusionSolver1DOptimized::setInitialCondition(
    std::function<double(double)> initialCondition) {

    for (int i = 1; i <= localNx_; ++i) {
        C_[i] = initialCondition(x_[i]);
    }

    // Initial synchronous ghost exchange
    auto requests = startGhostExchange();
    completeGhostExchange(requests);
}

void MPIDiffusionSolver1DOptimized::setDirichletBC(double leftValue, double rightValue) {
    bcType_ = BCType::Dirichlet;
    bcLeft_ = leftValue;
    bcRight_ = rightValue;
}

void MPIDiffusionSolver1DOptimized::setNeumannBC(double leftFlux, double rightFlux) {
    bcType_ = BCType::Neumann;
    bcLeft_ = leftFlux;
    bcRight_ = rightFlux;
}

std::vector<std::unique_ptr<mpi::MPIRequest>>
MPIDiffusionSolver1DOptimized::startGhostExchange() {

    std::vector<std::unique_ptr<mpi::MPIRequest>> requests;

    // Prepare send buffers
    if (localNx_ > 0) {
        leftGhostSend_ = C_[1];           // Leftmost interior point
        rightGhostSend_ = C_[localNx_];   // Rightmost interior point
    }

    // Non-blocking send/recv with left neighbor
    if (leftNeighbor_ >= 0) {
        requests.push_back(asyncComm_->isend(&leftGhostSend_, 1, leftNeighbor_, 0));
        requests.push_back(asyncComm_->irecv(&leftGhostRecv_, 1, leftNeighbor_, 0));
    }

    // Non-blocking send/recv with right neighbor
    if (rightNeighbor_ >= 0) {
        requests.push_back(asyncComm_->isend(&rightGhostSend_, 1, rightNeighbor_, 0));
        requests.push_back(asyncComm_->irecv(&rightGhostRecv_, 1, rightNeighbor_, 0));
    }

    return requests;
}

void MPIDiffusionSolver1DOptimized::completeGhostExchange(
    std::vector<std::unique_ptr<mpi::MPIRequest>>& requests) {

    // Wait for all communications to complete
    mpi::waitAll(requests);

    // Copy received data to ghost cells
    if (leftNeighbor_ >= 0) {
        C_[0] = leftGhostRecv_;
    }
    if (rightNeighbor_ >= 0) {
        C_[localNx_ + 1] = rightGhostRecv_;
    }
}

void MPIDiffusionSolver1DOptimized::computeInterior(double dt) {
    // Compute all interior points except the ones adjacent to ghosts
    double dx2_inv = 1.0 / (dx_ * dx_);

    // Skip first and last interior points (they need ghosts)
    int start = 2;
    int end = localNx_ - 1;

    if (localNx_ <= 2) return;  // Too small for interior points

    for (int i = start; i <= end; ++i) {
        double laplacian = (C_[i+1] - 2.0 * C_[i] + C_[i-1]) * dx2_inv;
        double dC_dt = diffusivity_ * laplacian;
        C_new_[i] = C_[i] + dt * dC_dt;
    }
}

void MPIDiffusionSolver1DOptimized::computeBoundary(double dt) {
    // Compute boundary interior points (need ghost values)
    double dx2_inv = 1.0 / (dx_ * dx_);

    // Left boundary point (i=1, needs ghost at i=0)
    if (localNx_ > 0) {
        double laplacian = (C_[2] - 2.0 * C_[1] + C_[0]) * dx2_inv;
        double dC_dt = diffusivity_ * laplacian;
        C_new_[1] = C_[1] + dt * dC_dt;
    }

    // Right boundary point (i=localNx_, needs ghost at i=localNx_+1)
    if (localNx_ > 1) {
        double laplacian = (C_[localNx_+1] - 2.0 * C_[localNx_] + C_[localNx_-1]) * dx2_inv;
        double dC_dt = diffusivity_ * laplacian;
        C_new_[localNx_] = C_[localNx_] + dt * dC_dt;
    }
}

void MPIDiffusionSolver1DOptimized::applyBoundaryConditions() {
    if (bcType_ == BCType::Dirichlet) {
        if (rank_ == 0) {
            C_[0] = bcLeft_;
        }
        if (rank_ == nprocs_ - 1) {
            C_[localNx_ + 1] = bcRight_;
        }
    } else {
        if (rank_ == 0) {
            C_[0] = C_[1] - bcLeft_ * dx_;
        }
        if (rank_ == nprocs_ - 1) {
            C_[localNx_ + 1] = C_[localNx_] + bcRight_ * dx_;
        }
    }
}

void MPIDiffusionSolver1DOptimized::stepExplicitOptimized(double dt) {
    auto stepStart = std::chrono::high_resolution_clock::now();

    // 1. Start non-blocking ghost exchange
    auto commStart = std::chrono::high_resolution_clock::now();
    auto requests = startGhostExchange();
    auto commInitEnd = std::chrono::high_resolution_clock::now();

    // 2. Compute interior points (while communication happens)
    auto compStart = std::chrono::high_resolution_clock::now();
    computeInterior(dt);
    auto compInteriorEnd = std::chrono::high_resolution_clock::now();

    // 3. Wait for ghost exchange to complete
    auto commWaitStart = std::chrono::high_resolution_clock::now();
    completeGhostExchange(requests);
    applyBoundaryConditions();
    auto commEnd = std::chrono::high_resolution_clock::now();

    // 4. Compute boundary points
    computeBoundary(dt);
    auto compEnd = std::chrono::high_resolution_clock::now();

    // Update solution
    std::swap(C_, C_new_);

    currentTime_ += dt;
    stats_.nTimeSteps++;

    // Performance tracking
    double commTime = std::chrono::duration<double>(commEnd - commStart).count();
    double interiorTime = std::chrono::duration<double>(compInteriorEnd - compStart).count();

    stats_.communicationTime += commTime;
    stats_.computeTime += std::chrono::duration<double>(compEnd - compStart).count();

    // Overlap time is the minimum of comm and interior computation
    overlapTime_ += std::min(commTime, interiorTime);
    totalCommTime_ += commTime;

    auto stepEnd = std::chrono::high_resolution_clock::now();
    stats_.totalTime += std::chrono::duration<double>(stepEnd - stepStart).count();
}

void MPIDiffusionSolver1DOptimized::solve(double dt, int nSteps, int outputInterval) {
    if (comm_.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "Optimized MPI Diffusion Solver (1D)\n";
        std::cout << "========================================\n";
        std::cout << "Global grid:      " << globalNx_ << "\n";
        std::cout << "Processes:        " << nprocs_ << "\n";
        std::cout << "Local points:     " << localNx_ << "\n";
        std::cout << "Domain:           [0, " << L_ << "] m\n";
        std::cout << "Grid spacing:     " << dx_ << " m\n";
        std::cout << "Diffusivity:      " << diffusivity_ << " m²/s\n";
        std::cout << "Time step:        " << dt << " s\n";
        std::cout << "Total steps:      " << nSteps << "\n";
        std::cout << "Optimization:     Communication/computation overlap\n";
        std::cout << "========================================\n";
    }

    for (int step = 0; step < nSteps; ++step) {
        stepExplicitOptimized(dt);

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
        std::cout << "Overlap efficiency: " << std::fixed << std::setprecision(1)
                  << getOverlapEfficiency() << "%\n";
    }

    stats_.print(comm_);
}

void MPIDiffusionSolver1DOptimized::gatherGlobalSolution(
    std::vector<double>& globalC, std::vector<double>& globalX) {

    if (comm_.isRoot()) {
        globalC.resize(globalNx_);
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

    std::vector<double> localInterior(localNx_);
    std::vector<double> localX(localNx_);
    for (int i = 0; i < localNx_; ++i) {
        localInterior[i] = C_[i + 1];
        localX[i] = x_[i + 1];
    }

#ifdef USE_MPI
    MPI_Gatherv(localInterior.data(), localNx_, MPI_DOUBLE,
                globalC.data(), sizes.data(), offsets.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Gatherv(localX.data(), localNx_, MPI_DOUBLE,
                globalX.data(), sizes.data(), offsets.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);
#else
    if (comm_.isRoot()) {
        globalC = localInterior;
        globalX = localX;
    }
#endif
}

double MPIDiffusionSolver1DOptimized::checkCFL(double dt) const {
    return diffusivity_ * dt / (dx_ * dx_);
}

double MPIDiffusionSolver1DOptimized::getOverlapEfficiency() const {
    if (totalCommTime_ < 1e-10) return 0.0;
    return 100.0 * overlapTime_ / totalCommTime_;
}

} // namespace solver
} // namespace parallel
} // namespace koo
