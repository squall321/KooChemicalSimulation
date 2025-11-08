/**
 * @file MPIDiffusionSolver1D.cpp
 * @brief Implementation of 1D MPI-parallel diffusion solver
 */

#include "parallel/solver/MPIDiffusionSolver.h"
#include "physics/diffusion/DiffusionCoefficient.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace parallel {
namespace solver {

// ============================================================================
// Performance Statistics
// ============================================================================

void MPISolverStats::print(const mpi::MPIComm& comm) const {
    if (comm.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "MPI Solver Performance Statistics\n";
        std::cout << "========================================\n";
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Total time:          " << totalTime << " s\n";
        std::cout << "Compute time:        " << computeTime << " s ("
                  << std::setprecision(1) << 100.0 * computeTime / totalTime << "%)\n";
        std::cout << "Communication time:  " << std::setprecision(6) << communicationTime << " s ("
                  << std::setprecision(1) << getCommOverhead() << "%)\n";
        std::cout << "I/O time:            " << std::setprecision(6) << ioTime << " s ("
                  << std::setprecision(1) << 100.0 * ioTime / totalTime << "%)\n";
        std::cout << "Time steps:          " << nTimeSteps << "\n";
        std::cout << "Ghost exchanges:     " << nGhostExchanges << "\n";
        std::cout << "Avg step time:       " << (nTimeSteps > 0 ? totalTime / nTimeSteps : 0.0) << " s\n";
        std::cout << "========================================\n";
    }
}

// ============================================================================
// MPIDiffusionSolver1D Implementation
// ============================================================================

MPIDiffusionSolver1D::MPIDiffusionSolver1D(int globalNx, double L, double diffusivity,
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
      currentTime_(0.0) {

    if (globalNx < nprocs_) {
        throw std::runtime_error("Global grid size must be >= number of processes");
    }

    // Setup domain decomposition
    setupDomain();

    // Allocate arrays (interior + 2 ghost cells)
    int totalSize = localNx_ + 2;
    C_.resize(totalSize, 0.0);
    C_new_.resize(totalSize, 0.0);
    x_.resize(totalSize);

    // Set x coordinates (including ghosts)
    for (int i = 0; i < totalSize; ++i) {
        int globalIdx = localStart_ - 1 + i;  // -1 for left ghost
        x_[i] = globalIdx * dx_;
    }

    // Create physics objects
    auto diffCoeff = std::make_shared<physics::ConstantDiffusion>(diffusivity);
    fick_ = std::make_shared<physics::FickDiffusion>(diffCoeff);
}

void MPIDiffusionSolver1D::setupDomain() {
    // Simple contiguous decomposition
    int pointsPerProc = globalNx_ / nprocs_;
    int remainder = globalNx_ % nprocs_;

    localStart_ = rank_ * pointsPerProc + std::min(rank_, remainder);
    localEnd_ = localStart_ + pointsPerProc + (rank_ < remainder ? 1 : 0);
    localNx_ = localEnd_ - localStart_;

    // Determine neighbors
    leftNeighbor_ = (rank_ > 0) ? rank_ - 1 : -1;
    rightNeighbor_ = (rank_ < nprocs_ - 1) ? rank_ + 1 : -1;
}

void MPIDiffusionSolver1D::setInitialCondition(std::function<double(double)> initialCondition) {
    // Set interior points
    for (int i = 1; i <= localNx_; ++i) {
        C_[i] = initialCondition(x_[i]);
    }

    // Initialize ghost cells
    exchangeGhostCells();
}

void MPIDiffusionSolver1D::setDirichletBC(double leftValue, double rightValue) {
    bcType_ = BCType::Dirichlet;
    bcLeft_ = leftValue;
    bcRight_ = rightValue;
}

void MPIDiffusionSolver1D::setNeumannBC(double leftFlux, double rightFlux) {
    bcType_ = BCType::Neumann;
    bcLeft_ = leftFlux;
    bcRight_ = rightFlux;
}

void MPIDiffusionSolver1D::exchangeGhostCells() {
    auto start = std::chrono::high_resolution_clock::now();

#ifdef USE_MPI
    const int tag = 0;

    // Send to left, receive from right
    if (leftNeighbor_ >= 0) {
        comm_.send(&C_[1], 1, leftNeighbor_, tag);  // Send leftmost interior
    }
    if (rightNeighbor_ >= 0) {
        comm_.recv(&C_[localNx_ + 1], 1, rightNeighbor_, tag);  // Receive right ghost
    }

    // Send to right, receive from left
    if (rightNeighbor_ >= 0) {
        comm_.send(&C_[localNx_], 1, rightNeighbor_, tag);  // Send rightmost interior
    }
    if (leftNeighbor_ >= 0) {
        comm_.recv(&C_[0], 1, leftNeighbor_, tag);  // Receive left ghost
    }
#endif

    auto end = std::chrono::high_resolution_clock::now();
    stats_.communicationTime += std::chrono::duration<double>(end - start).count();
    stats_.nGhostExchanges++;
}

void MPIDiffusionSolver1D::applyBoundaryConditions() {
    if (bcType_ == BCType::Dirichlet) {
        // Only leftmost and rightmost processes set BCs
        if (rank_ == 0) {
            C_[0] = bcLeft_;
        }
        if (rank_ == nprocs_ - 1) {
            C_[localNx_ + 1] = bcRight_;
        }
    } else {
        // Neumann BC: use one-sided differences
        if (rank_ == 0) {
            // dC/dx = bcLeft_ => C[0] = C[1] - bcLeft_ * dx
            C_[0] = C_[1] - bcLeft_ * dx_;
        }
        if (rank_ == nprocs_ - 1) {
            // dC/dx = bcRight_ => C[end] = C[end-1] + bcRight_ * dx
            C_[localNx_ + 1] = C_[localNx_] + bcRight_ * dx_;
        }
    }
}

void MPIDiffusionSolver1D::computeLaplacian(std::vector<double>& laplacian) {
    laplacian.resize(localNx_ + 2, 0.0);

    // Interior points: standard 3-point stencil
    for (int i = 1; i <= localNx_; ++i) {
        laplacian[i] = (C_[i+1] - 2.0 * C_[i] + C_[i-1]) / (dx_ * dx_);
    }
}

void MPIDiffusionSolver1D::stepExplicit(double dt) {
    auto stepStart = std::chrono::high_resolution_clock::now();

    // Exchange ghost cells
    exchangeGhostCells();
    applyBoundaryConditions();

    // Compute update
    auto compStart = std::chrono::high_resolution_clock::now();

    std::vector<double> laplacian;
    computeLaplacian(laplacian);

    for (int i = 1; i <= localNx_; ++i) {
        double dC_dt = diffusivity_ * laplacian[i];
        C_new_[i] = C_[i] + dt * dC_dt;
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

void MPIDiffusionSolver1D::solve(double dt, int nSteps, int outputInterval) {
    if (comm_.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "MPI Diffusion Solver (1D)\n";
        std::cout << "========================================\n";
        std::cout << "Global grid:      " << globalNx_ << "\n";
        std::cout << "Processes:        " << nprocs_ << "\n";
        std::cout << "Local points:     " << localNx_ << " (rank " << rank_ << ")\n";
        std::cout << "Domain:           [0, " << L_ << "] m\n";
        std::cout << "Grid spacing:     " << dx_ << " m\n";
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

    // Print statistics
    stats_.print(comm_);
}

void MPIDiffusionSolver1D::gatherGlobalSolution(std::vector<double>& globalC,
                                                std::vector<double>& globalX) {
    auto start = std::chrono::high_resolution_clock::now();

    if (comm_.isRoot()) {
        globalC.resize(globalNx_);
        globalX.resize(globalNx_);
    }

    // Gather sizes from all processes
    std::vector<int> sizes(nprocs_);
    std::vector<int> offsets(nprocs_);
    int mySize = localNx_;

    comm_.allGather(&mySize, 1, sizes.data(), 1);

    int totalSize = 0;
    for (int i = 0; i < nprocs_; ++i) {
        offsets[i] = totalSize;
        totalSize += sizes[i];
    }

    // Extract interior points (skip ghosts)
    std::vector<double> localInterior(localNx_);
    std::vector<double> localX(localNx_);
    for (int i = 0; i < localNx_; ++i) {
        localInterior[i] = C_[i + 1];  // Skip left ghost
        localX[i] = x_[i + 1];
    }

    // Gather to root
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

    auto end = std::chrono::high_resolution_clock::now();
    stats_.ioTime += std::chrono::duration<double>(end - start).count();
}

double MPIDiffusionSolver1D::checkCFL(double dt) const {
    return diffusivity_ * dt / (dx_ * dx_);
}

} // namespace solver
} // namespace parallel
} // namespace koo
