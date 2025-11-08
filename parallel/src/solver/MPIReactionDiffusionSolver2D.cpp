/**
 * @file MPIReactionDiffusionSolver2D.cpp
 * @brief Implementation of 2D MPI-parallel reaction-diffusion solver
 */

#include "parallel/solver/MPIReactionDiffusionSolver.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

namespace koo {
namespace parallel {
namespace solver {

MPIReactionDiffusionSolver2D::MPIReactionDiffusionSolver2D(
    int globalNx, int globalNy,
    double Lx, double Ly,
    double Du, double Dv,
    ReactionFunction reactionU,
    ReactionFunction reactionV,
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
      Du_(Du),
      Dv_(Dv),
      reactionU_(reactionU),
      reactionV_(reactionV),
      bcType_(BCType::Dirichlet),
      bcU_(0.0),
      bcV_(0.0),
      currentTime_(0.0),
      isPeriodic_(false) {

    setupDomain();

    // Allocate arrays (including ghost cells)
    int nx_total = localNx_ + 2;
    int ny_total = localNy_ + 2;

    u_.resize(nx_total, std::vector<double>(ny_total, 0.0));
    v_.resize(nx_total, std::vector<double>(ny_total, 0.0));
    u_new_.resize(nx_total, std::vector<double>(ny_total, 0.0));
    v_new_.resize(nx_total, std::vector<double>(ny_total, 0.0));
}

std::unique_ptr<MPIReactionDiffusionSolver2D>
MPIReactionDiffusionSolver2D::createGrayScott(
    int globalNx, int globalNy,
    double Lx, double Ly,
    const GrayScottParams& params,
    const mpi::MPIComm& comm) {

    // Gray-Scott reaction terms
    auto reactionU = [F = params.F, k = params.k](double u, double v) {
        return -u * v * v + F * (1.0 - u);
    };

    auto reactionV = [F = params.F, k = params.k](double u, double v) {
        return u * v * v - (F + k) * v;
    };

    return std::make_unique<MPIReactionDiffusionSolver2D>(
        globalNx, globalNy, Lx, Ly,
        params.Du, params.Dv,
        reactionU, reactionV, comm);
}

std::unique_ptr<MPIReactionDiffusionSolver2D>
MPIReactionDiffusionSolver2D::createBrusselator(
    int globalNx, int globalNy,
    double Lx, double Ly,
    const BrusselatorParams& params,
    const mpi::MPIComm& comm) {

    auto reactionU = [a = params.a, b = params.b](double u, double v) {
        return a - (b + 1.0) * u + u * u * v;
    };

    auto reactionV = [b = params.b](double u, double v) {
        return b * u - u * u * v;
    };

    return std::make_unique<MPIReactionDiffusionSolver2D>(
        globalNx, globalNy, Lx, Ly,
        params.Du, params.Dv,
        reactionU, reactionV, comm);
}

void MPIReactionDiffusionSolver2D::setupDomain() {
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

void MPIReactionDiffusionSolver2D::setInitialCondition(
    std::function<double(double, double)> initialU,
    std::function<double(double, double)> initialV) {

    for (int i = 1; i <= localNx_; ++i) {
        for (int j = 1; j <= localNy_; ++j) {
            int globalI = localStartX_ + i - 1;
            int globalJ = localStartY_ + j - 1;
            double x = globalI * dx_;
            double y = globalJ * dy_;
            u_[i][j] = initialU(x, y);
            v_[i][j] = initialV(x, y);
        }
    }

    exchangeGhostCells();
}

void MPIReactionDiffusionSolver2D::setDirichletBC(double uValue, double vValue) {
    bcType_ = BCType::Dirichlet;
    bcU_ = uValue;
    bcV_ = vValue;
    isPeriodic_ = false;
}

void MPIReactionDiffusionSolver2D::setPeriodicBC() {
    bcType_ = BCType::Periodic;
    isPeriodic_ = true;
}

void MPIReactionDiffusionSolver2D::exchangeGhostCells() {
    auto start = std::chrono::high_resolution_clock::now();

#ifdef USE_MPI
    const int tag_u = 0;
    const int tag_v = 1;

    auto neighbors = decomp_->getNeighborRanks();

    // X direction (left-right)
    if (!decomp_->isAtBoundary(0, 0) || isPeriodic_) {
        int leftRank = isPeriodic_ && decomp_->isAtBoundary(0, 0) ?
            neighbors[1] : neighbors[0];  // Wrap around if periodic

        std::vector<double> sendBufU(localNy_), sendBufV(localNy_);
        std::vector<double> recvBufU(localNy_), recvBufV(localNy_);

        for (int j = 1; j <= localNy_; ++j) {
            sendBufU[j-1] = u_[1][j];
            sendBufV[j-1] = v_[1][j];
        }

        comm_.send(sendBufU.data(), localNy_, leftRank, tag_u);
        comm_.send(sendBufV.data(), localNy_, leftRank, tag_v);
        comm_.recv(recvBufU.data(), localNy_, leftRank, tag_u);
        comm_.recv(recvBufV.data(), localNy_, leftRank, tag_v);

        for (int j = 1; j <= localNy_; ++j) {
            u_[0][j] = recvBufU[j-1];
            v_[0][j] = recvBufV[j-1];
        }
    }

    if (!decomp_->isAtBoundary(0, 1) || isPeriodic_) {
        int rightRank = isPeriodic_ && decomp_->isAtBoundary(0, 1) ?
            neighbors[0] : neighbors[1];

        std::vector<double> sendBufU(localNy_), sendBufV(localNy_);
        std::vector<double> recvBufU(localNy_), recvBufV(localNy_);

        for (int j = 1; j <= localNy_; ++j) {
            sendBufU[j-1] = u_[localNx_][j];
            sendBufV[j-1] = v_[localNx_][j];
        }

        comm_.send(sendBufU.data(), localNy_, rightRank, tag_u);
        comm_.send(sendBufV.data(), localNy_, rightRank, tag_v);
        comm_.recv(recvBufU.data(), localNy_, rightRank, tag_u);
        comm_.recv(recvBufV.data(), localNy_, rightRank, tag_v);

        for (int j = 1; j <= localNy_; ++j) {
            u_[localNx_ + 1][j] = recvBufU[j-1];
            v_[localNx_ + 1][j] = recvBufV[j-1];
        }
    }

    // Y direction (bottom-top)
    if (!decomp_->isAtBoundary(1, 0) || isPeriodic_) {
        int bottomRank = isPeriodic_ && decomp_->isAtBoundary(1, 0) ?
            neighbors[3] : neighbors[2];

        std::vector<double> sendBufU(localNx_), sendBufV(localNx_);
        std::vector<double> recvBufU(localNx_), recvBufV(localNx_);

        for (int i = 1; i <= localNx_; ++i) {
            sendBufU[i-1] = u_[i][1];
            sendBufV[i-1] = v_[i][1];
        }

        comm_.send(sendBufU.data(), localNx_, bottomRank, tag_u);
        comm_.send(sendBufV.data(), localNx_, bottomRank, tag_v);
        comm_.recv(recvBufU.data(), localNx_, bottomRank, tag_u);
        comm_.recv(recvBufV.data(), localNx_, bottomRank, tag_v);

        for (int i = 1; i <= localNx_; ++i) {
            u_[i][0] = recvBufU[i-1];
            v_[i][0] = recvBufV[i-1];
        }
    }

    if (!decomp_->isAtBoundary(1, 1) || isPeriodic_) {
        int topRank = isPeriodic_ && decomp_->isAtBoundary(1, 1) ?
            neighbors[2] : neighbors[3];

        std::vector<double> sendBufU(localNx_), sendBufV(localNx_);
        std::vector<double> recvBufU(localNx_), recvBufV(localNx_);

        for (int i = 1; i <= localNx_; ++i) {
            sendBufU[i-1] = u_[i][localNy_];
            sendBufV[i-1] = v_[i][localNy_];
        }

        comm_.send(sendBufU.data(), localNx_, topRank, tag_u);
        comm_.send(sendBufV.data(), localNx_, topRank, tag_v);
        comm_.recv(recvBufU.data(), localNx_, topRank, tag_u);
        comm_.recv(recvBufV.data(), localNx_, topRank, tag_v);

        for (int i = 1; i <= localNx_; ++i) {
            u_[i][localNy_ + 1] = recvBufU[i-1];
            v_[i][localNy_ + 1] = recvBufV[i-1];
        }
    }
#endif

    auto end = std::chrono::high_resolution_clock::now();
    stats_.communicationTime += std::chrono::duration<double>(end - start).count();
    stats_.nGhostExchanges++;
}

void MPIReactionDiffusionSolver2D::applyBoundaryConditions() {
    if (bcType_ == BCType::Dirichlet && !isPeriodic_) {
        if (decomp_->isAtBoundary(0, 0)) {
            for (int j = 0; j < static_cast<int>(u_[0].size()); ++j) {
                u_[0][j] = bcU_;
                v_[0][j] = bcV_;
            }
        }
        if (decomp_->isAtBoundary(0, 1)) {
            for (int j = 0; j < static_cast<int>(u_[localNx_ + 1].size()); ++j) {
                u_[localNx_ + 1][j] = bcU_;
                v_[localNx_ + 1][j] = bcV_;
            }
        }
        if (decomp_->isAtBoundary(1, 0)) {
            for (int i = 0; i <= localNx_ + 1; ++i) {
                u_[i][0] = bcU_;
                v_[i][0] = bcV_;
            }
        }
        if (decomp_->isAtBoundary(1, 1)) {
            for (int i = 0; i <= localNx_ + 1; ++i) {
                u_[i][localNy_ + 1] = bcU_;
                v_[i][localNy_ + 1] = bcV_;
            }
        }
    }
}

void MPIReactionDiffusionSolver2D::stepExplicit(double dt) {
    auto stepStart = std::chrono::high_resolution_clock::now();

    exchangeGhostCells();
    applyBoundaryConditions();

    auto compStart = std::chrono::high_resolution_clock::now();

    double dx2_inv = 1.0 / (dx_ * dx_);
    double dy2_inv = 1.0 / (dy_ * dy_);

    for (int i = 1; i <= localNx_; ++i) {
        for (int j = 1; j <= localNy_; ++j) {
            // Laplacian for u
            double laplacian_u_x = (u_[i+1][j] - 2.0 * u_[i][j] + u_[i-1][j]) * dx2_inv;
            double laplacian_u_y = (u_[i][j+1] - 2.0 * u_[i][j] + u_[i][j-1]) * dy2_inv;
            double laplacian_u = laplacian_u_x + laplacian_u_y;

            // Laplacian for v
            double laplacian_v_x = (v_[i+1][j] - 2.0 * v_[i][j] + v_[i-1][j]) * dx2_inv;
            double laplacian_v_y = (v_[i][j+1] - 2.0 * v_[i][j] + v_[i][j-1]) * dy2_inv;
            double laplacian_v = laplacian_v_x + laplacian_v_y;

            // Reaction terms
            double react_u = reactionU_(u_[i][j], v_[i][j]);
            double react_v = reactionV_(u_[i][j], v_[i][j]);

            // Update
            double du_dt = Du_ * laplacian_u + react_u;
            double dv_dt = Dv_ * laplacian_v + react_v;

            u_new_[i][j] = u_[i][j] + dt * du_dt;
            v_new_[i][j] = v_[i][j] + dt * dv_dt;
        }
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

void MPIReactionDiffusionSolver2D::solve(double dt, int nSteps, int outputInterval) {
    if (comm_.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "MPI Reaction-Diffusion Solver (2D)\n";
        std::cout << "========================================\n";
        std::cout << "Global grid:      " << globalNx_ << " x " << globalNy_ << "\n";
        std::cout << "Processes:        " << nprocs_ << "\n";
        std::cout << "Local grid:       " << localNx_ << " x " << localNy_ << "\n";
        std::cout << "Domain:           [0, " << Lx_ << "] x [0, " << Ly_ << "] m\n";
        std::cout << "Grid spacing:     dx=" << dx_ << ", dy=" << dy_ << " m\n";
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

double MPIReactionDiffusionSolver2D::checkCFL(double dt) const {
    double cfl_u_x = Du_ * dt / (dx_ * dx_);
    double cfl_u_y = Du_ * dt / (dy_ * dy_);
    double cfl_v_x = Dv_ * dt / (dx_ * dx_);
    double cfl_v_y = Dv_ * dt / (dy_ * dy_);
    return std::max(cfl_u_x + cfl_u_y, cfl_v_x + cfl_v_y);
}

void MPIReactionDiffusionSolver2D::writeLocalVTK(const std::string& baseFilename, int step) {
    std::string filename = baseFilename + "_rank" + std::to_string(rank_) +
                          "_step" + std::to_string(step) + ".vtk";

    std::ofstream file(filename);
    if (!file.is_open()) return;

    // VTK header
    file << "# vtk DataFile Version 3.0\n";
    file << "Reaction-Diffusion Local Domain\n";
    file << "ASCII\n";
    file << "DATASET STRUCTURED_POINTS\n";
    file << "DIMENSIONS " << localNx_ << " " << localNy_ << " 1\n";
    file << "ORIGIN " << localStartX_ * dx_ << " " << localStartY_ * dy_ << " 0\n";
    file << "SPACING " << dx_ << " " << dy_ << " 1\n";
    file << "POINT_DATA " << localNx_ * localNy_ << "\n";

    // Write u
    file << "SCALARS u double 1\n";
    file << "LOOKUP_TABLE default\n";
    for (int j = 1; j <= localNy_; ++j) {
        for (int i = 1; i <= localNx_; ++i) {
            file << u_[i][j] << "\n";
        }
    }

    // Write v
    file << "SCALARS v double 1\n";
    file << "LOOKUP_TABLE default\n";
    for (int j = 1; j <= localNy_; ++j) {
        for (int i = 1; i <= localNx_; ++i) {
            file << v_[i][j] << "\n";
        }
    }

    file.close();
}

} // namespace solver
} // namespace parallel
} // namespace koo
