/**
 * @file MPIDiffusionSolver2D.cpp
 * @brief Implementation of 2D MPI-parallel diffusion solver
 */

#include "parallel/solver/MPIDiffusionSolver.h"
#include "physics/diffusion/DiffusionCoefficient.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace parallel {
namespace solver {

MPIDiffusionSolver2D::MPIDiffusionSolver2D(int globalNx, int globalNy,
                                           double Lx, double Ly,
                                           double diffusivity,
                                           const mpi::MPIComm& comm)
    : comm_(comm),
      rank_(comm.getRank()),
      nprocs_(comm.getSize()),
      globalNx_(globalNx),
      globalNy_(globalNy),
      Lx_(Lx),
      Ly_(Ly),
      dx_(Lx / (globalNx - 1)),
      dy_(Ly / (globalNy - 1)),
      diffusivity_(diffusivity),
      bcValue_(0.0),
      currentTime_(0.0) {

    if (globalNx < 2 || globalNy < 2) {
        throw std::runtime_error("Grid size must be >= 2 in each direction");
    }

    setupDomain();

    // Allocate arrays (including ghost cells)
    int nx_total = localNx_ + 2;  // +2 for ghost cells
    int ny_total = localNy_ + 2;

    C_.resize(nx_total, std::vector<double>(ny_total, 0.0));
    C_new_.resize(nx_total, std::vector<double>(ny_total, 0.0));

    // Create physics
    auto diffCoeff = std::make_shared<physics::ConstantDiffusion>(diffusivity);
    fick_ = std::make_shared<physics::FickDiffusion>(diffCoeff);
}

void MPIDiffusionSolver2D::setupDomain() {
    // Create Cartesian decomposition
    decomp_ = std::make_unique<domain::CartesianDecomposition>(
        globalNx_, globalNy_, 1, comm_);

    auto bounds = decomp_->getLocalBounds();
    localStartX_ = bounds[0];
    localEndX_ = bounds[1];
    localStartY_ = bounds[2];
    localEndY_ = bounds[3];

    localNx_ = localEndX_ - localStartX_ + 1;
    localNy_ = localEndY_ - localStartY_ + 1;
}

void MPIDiffusionSolver2D::setInitialCondition(
    std::function<double(double, double)> initialCondition) {

    // Set interior points
    for (int i = 1; i <= localNx_; ++i) {
        for (int j = 1; j <= localNy_; ++j) {
            int globalI = localStartX_ + i - 1;
            int globalJ = localStartY_ + j - 1;
            double x = globalI * dx_;
            double y = globalJ * dy_;
            C_[i][j] = initialCondition(x, y);
        }
    }

    exchangeGhostCells();
}

void MPIDiffusionSolver2D::setDirichletBC(double value) {
    bcValue_ = value;
}

void MPIDiffusionSolver2D::exchangeGhostCells() {
    auto start = std::chrono::high_resolution_clock::now();

#ifdef USE_MPI
    auto neighbors = decomp_->getNeighborRanks();
    const int tag = 0;

    // Simple implementation: exchange with 4 face neighbors
    auto coords = decomp_->getProcessCoords();

    // Send/recv in X direction
    if (!decomp_->isAtBoundary(0, 0)) {  // Has left neighbor
        int leftRank = neighbors[0];
        // Send left column, receive left ghost
        std::vector<double> sendBuf(localNy_);
        std::vector<double> recvBuf(localNy_);

        for (int j = 1; j <= localNy_; ++j) {
            sendBuf[j-1] = C_[1][j];
        }

        comm_.send(sendBuf.data(), localNy_, leftRank, tag);
        comm_.recv(recvBuf.data(), localNy_, leftRank, tag);

        for (int j = 1; j <= localNy_; ++j) {
            C_[0][j] = recvBuf[j-1];
        }
    }

    if (!decomp_->isAtBoundary(0, 1)) {  // Has right neighbor
        int rightRank = neighbors[1];
        std::vector<double> sendBuf(localNy_);
        std::vector<double> recvBuf(localNy_);

        for (int j = 1; j <= localNy_; ++j) {
            sendBuf[j-1] = C_[localNx_][j];
        }

        comm_.send(sendBuf.data(), localNy_, rightRank, tag);
        comm_.recv(recvBuf.data(), localNy_, rightRank, tag);

        for (int j = 1; j <= localNy_; ++j) {
            C_[localNx_ + 1][j] = recvBuf[j-1];
        }
    }

    // Send/recv in Y direction
    if (!decomp_->isAtBoundary(1, 0)) {  // Has bottom neighbor
        int bottomRank = neighbors[2];
        std::vector<double> sendBuf(localNx_);
        std::vector<double> recvBuf(localNx_);

        for (int i = 1; i <= localNx_; ++i) {
            sendBuf[i-1] = C_[i][1];
        }

        comm_.send(sendBuf.data(), localNx_, bottomRank, tag);
        comm_.recv(recvBuf.data(), localNx_, bottomRank, tag);

        for (int i = 1; i <= localNx_; ++i) {
            C_[i][0] = recvBuf[i-1];
        }
    }

    if (!decomp_->isAtBoundary(1, 1)) {  // Has top neighbor
        int topRank = neighbors[3];
        std::vector<double> sendBuf(localNx_);
        std::vector<double> recvBuf(localNx_);

        for (int i = 1; i <= localNx_; ++i) {
            sendBuf[i-1] = C_[i][localNy_];
        }

        comm_.send(sendBuf.data(), localNx_, topRank, tag);
        comm_.recv(recvBuf.data(), localNx_, topRank, tag);

        for (int i = 1; i <= localNx_; ++i) {
            C_[i][localNy_ + 1] = recvBuf[i-1];
        }
    }
#endif

    auto end = std::chrono::high_resolution_clock::now();
    stats_.communicationTime += std::chrono::duration<double>(end - start).count();
    stats_.nGhostExchanges++;
}

void MPIDiffusionSolver2D::applyBoundaryConditions() {
    // Dirichlet BC on all boundaries
    // Only processes at domain boundaries set these

    if (decomp_->isAtBoundary(0, 0)) {  // Left boundary
        for (int j = 0; j < static_cast<int>(C_[0].size()); ++j) {
            C_[0][j] = bcValue_;
        }
    }

    if (decomp_->isAtBoundary(0, 1)) {  // Right boundary
        for (int j = 0; j < static_cast<int>(C_[localNx_ + 1].size()); ++j) {
            C_[localNx_ + 1][j] = bcValue_;
        }
    }

    if (decomp_->isAtBoundary(1, 0)) {  // Bottom boundary
        for (int i = 0; i <= localNx_ + 1; ++i) {
            C_[i][0] = bcValue_;
        }
    }

    if (decomp_->isAtBoundary(1, 1)) {  // Top boundary
        for (int i = 0; i <= localNx_ + 1; ++i) {
            C_[i][localNy_ + 1] = bcValue_;
        }
    }
}

void MPIDiffusionSolver2D::stepExplicit(double dt) {
    auto stepStart = std::chrono::high_resolution_clock::now();

    // Exchange ghost cells
    exchangeGhostCells();
    applyBoundaryConditions();

    // Compute update
    auto compStart = std::chrono::high_resolution_clock::now();

    double dx2_inv = 1.0 / (dx_ * dx_);
    double dy2_inv = 1.0 / (dy_ * dy_);

    for (int i = 1; i <= localNx_; ++i) {
        for (int j = 1; j <= localNy_; ++j) {
            // 5-point stencil Laplacian
            double laplacian_x = (C_[i+1][j] - 2.0 * C_[i][j] + C_[i-1][j]) * dx2_inv;
            double laplacian_y = (C_[i][j+1] - 2.0 * C_[i][j] + C_[i][j-1]) * dy2_inv;
            double laplacian = laplacian_x + laplacian_y;

            double dC_dt = diffusivity_ * laplacian;
            C_new_[i][j] = C_[i][j] + dt * dC_dt;
        }
    }

    // Update solution
    std::swap(C_, C_new_);

    auto compEnd = std::chrono::high_resolution_clock::now();
    stats_.computeTime += std::chrono::duration<double>(compEnd - compStart).count();

    currentTime_ += dt;
    stats_.nTimeSteps++;

    auto stepEnd = std::chrono::high_resolution_clock::now();
    stats_.totalTime += std::chrono::duration<double>(stepEnd - stepStart).count();
}

void MPIDiffusionSolver2D::solve(double dt, int nSteps, int outputInterval) {
    if (comm_.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "MPI Diffusion Solver (2D)\n";
        std::cout << "========================================\n";
        std::cout << "Global grid:      " << globalNx_ << " x " << globalNy_ << "\n";
        std::cout << "Processes:        " << nprocs_ << "\n";
        std::cout << "Local grid:       " << localNx_ << " x " << localNy_ << " (rank " << rank_ << ")\n";
        std::cout << "Domain:           [0, " << Lx_ << "] x [0, " << Ly_ << "] m\n";
        std::cout << "Grid spacing:     dx=" << dx_ << ", dy=" << dy_ << " m\n";
        std::cout << "Diffusivity:      " << diffusivity_ << " m²/s\n";
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
                          << "  t = " << std::fixed << std::setprecision(6) << currentTime_ << " s\n";
            }
        }
    }

    if (comm_.isRoot()) {
        std::cout << "\nSimulation complete!\n";
    }

    stats_.print(comm_);
}

void MPIDiffusionSolver2D::gatherGlobalSolution(std::vector<std::vector<double>>& globalC) {
    // Simplified: each process writes its own data
    // Full implementation would use MPI_Gatherv with proper indexing
    // For now, just copy local data
    if (comm_.isRoot()) {
        globalC.resize(globalNx_, std::vector<double>(globalNy_, 0.0));
    }

    // TODO: Implement proper MPI_Gatherv for 2D data
    // This is a placeholder - full implementation would gather all subdomain data to root
}

double MPIDiffusionSolver2D::checkCFL(double dt) const {
    double cfl_x = diffusivity_ * dt / (dx_ * dx_);
    double cfl_y = diffusivity_ * dt / (dy_ * dy_);
    return cfl_x + cfl_y;
}

} // namespace solver
} // namespace parallel
} // namespace koo
