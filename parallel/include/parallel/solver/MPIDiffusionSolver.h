/**
 * @file MPIDiffusionSolver.h
 * @brief MPI-parallel diffusion equation solver
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 * @date 2025-11-08
 *
 * Priority D1: MPI Parallelization
 *
 * MPI-parallel solver for the diffusion equation:
 *   ∂C/∂t = D ∇²C
 *
 * Features:
 * - Domain decomposition (1D, 2D, 3D)
 * - Ghost cell communication
 * - Explicit and implicit time-stepping
 * - Parallel I/O
 * - Performance monitoring
 */

#ifndef KOO_PARALLEL_SOLVER_MPI_DIFFUSION_SOLVER_H
#define KOO_PARALLEL_SOLVER_MPI_DIFFUSION_SOLVER_H

#include "parallel/mpi/MPIWrapper.h"
#include "parallel/domain/DomainDecomposition.h"
#include "physics/diffusion/FickDiffusion.h"
#include <vector>
#include <memory>
#include <string>
#include <chrono>

namespace koo {
namespace parallel {
namespace solver {

// ============================================================================
// Performance Statistics
// ============================================================================

/**
 * @brief Performance statistics for MPI solver
 */
struct MPISolverStats {
    double totalTime = 0.0;              ///< Total wall time (s)
    double computeTime = 0.0;            ///< Computation time (s)
    double communicationTime = 0.0;      ///< Communication time (s)
    double ioTime = 0.0;                 ///< I/O time (s)
    size_t nTimeSteps = 0;               ///< Number of time steps
    size_t nGhostExchanges = 0;          ///< Number of ghost exchanges

    /**
     * @brief Get communication overhead percentage
     */
    double getCommOverhead() const {
        if (totalTime < 1e-10) return 0.0;
        return 100.0 * communicationTime / totalTime;
    }

    /**
     * @brief Print statistics
     */
    void print(const mpi::MPIComm& comm) const;
};

// ============================================================================
// MPI Diffusion Solver (1D)
// ============================================================================

/**
 * @brief 1D MPI-parallel diffusion solver
 *
 * Solves: ∂C/∂t = D ∂²C/∂x²
 *
 * Domain decomposition: Each process owns a contiguous subdomain
 * Ghost cells: 1 layer on each side for 2nd-order spatial discretization
 */
class MPIDiffusionSolver1D {
public:
    /**
     * @brief Constructor
     * @param globalNx Global number of grid points
     * @param L Domain length (m)
     * @param diffusivity Diffusion coefficient (m²/s)
     * @param comm MPI communicator
     */
    MPIDiffusionSolver1D(int globalNx, double L, double diffusivity,
                         const mpi::MPIComm& comm);

    /**
     * @brief Set initial condition
     * @param initialCondition Function: x -> C(x, t=0)
     */
    void setInitialCondition(std::function<double(double)> initialCondition);

    /**
     * @brief Set boundary conditions (Dirichlet)
     * @param leftValue Value at x=0
     * @param rightValue Value at x=L
     */
    void setDirichletBC(double leftValue, double rightValue);

    /**
     * @brief Set boundary conditions (Neumann)
     * @param leftFlux Flux at x=0 (dC/dx)
     * @param rightFlux Flux at x=L (dC/dx)
     */
    void setNeumannBC(double leftFlux, double rightFlux);

    /**
     * @brief Solve for one time step (explicit Euler)
     * @param dt Time step size (s)
     */
    void stepExplicit(double dt);

    /**
     * @brief Solve for multiple time steps
     * @param dt Time step size (s)
     * @param nSteps Number of steps
     * @param outputInterval Output every N steps (0 = no output)
     */
    void solve(double dt, int nSteps, int outputInterval = 0);

    /**
     * @brief Get local concentration values
     */
    const std::vector<double>& getLocalConcentration() const { return C_; }

    /**
     * @brief Get local x coordinates
     */
    const std::vector<double>& getLocalCoordinates() const { return x_; }

    /**
     * @brief Get global solution (gathered to root)
     * @param globalC Output vector (only filled on root)
     * @param globalX Output coordinates (only filled on root)
     */
    void gatherGlobalSolution(std::vector<double>& globalC,
                             std::vector<double>& globalX);

    /**
     * @brief Get performance statistics
     */
    const MPISolverStats& getStats() const { return stats_; }

    /**
     * @brief Get local domain bounds [start, end)
     */
    std::pair<int, int> getLocalBounds() const {
        return {localStart_, localEnd_};
    }

    /**
     * @brief Get current time
     */
    double getCurrentTime() const { return currentTime_; }

    /**
     * @brief Check CFL condition
     * @param dt Time step size
     * @return CFL number (should be < 0.5 for stability)
     */
    double checkCFL(double dt) const;

private:
    /**
     * @brief Exchange ghost cells with neighbors
     */
    void exchangeGhostCells();

    /**
     * @brief Apply boundary conditions
     */
    void applyBoundaryConditions();

    /**
     * @brief Compute Laplacian at interior points
     */
    void computeLaplacian(std::vector<double>& laplacian);

    /**
     * @brief Setup domain decomposition
     */
    void setupDomain();

    // MPI
    const mpi::MPIComm& comm_;
    int rank_;
    int nprocs_;

    // Global problem
    int globalNx_;
    double L_;
    double dx_;
    double diffusivity_;

    // Local domain
    int localNx_;           ///< Number of local interior points
    int localStart_;        ///< Global index of first local point
    int localEnd_;          ///< Global index of last local point (exclusive)
    int leftNeighbor_;      ///< Rank of left neighbor (-1 if none)
    int rightNeighbor_;     ///< Rank of right neighbor (-1 if none)

    // Solution arrays
    std::vector<double> C_;      ///< Local concentration [localNx + 2 ghosts]
    std::vector<double> C_new_;  ///< Updated concentration
    std::vector<double> x_;      ///< Local x coordinates

    // Boundary conditions
    enum class BCType { Dirichlet, Neumann };
    BCType bcType_;
    double bcLeft_;
    double bcRight_;

    // Time
    double currentTime_;

    // Statistics
    MPISolverStats stats_;

    // Physics
    std::shared_ptr<physics::FickDiffusion> fick_;
};

// ============================================================================
// MPI Diffusion Solver (2D)
// ============================================================================

/**
 * @brief 2D MPI-parallel diffusion solver
 *
 * Solves: ∂C/∂t = D (∂²C/∂x² + ∂²C/∂y²)
 *
 * Uses Cartesian domain decomposition
 */
class MPIDiffusionSolver2D {
public:
    /**
     * @brief Constructor
     * @param globalNx Global grid points in x
     * @param globalNy Global grid points in y
     * @param Lx Domain length in x (m)
     * @param Ly Domain length in y (m)
     * @param diffusivity Diffusion coefficient (m²/s)
     * @param comm MPI communicator
     */
    MPIDiffusionSolver2D(int globalNx, int globalNy,
                         double Lx, double Ly,
                         double diffusivity,
                         const mpi::MPIComm& comm);

    /**
     * @brief Set initial condition
     * @param initialCondition Function: (x, y) -> C(x, y, t=0)
     */
    void setInitialCondition(std::function<double(double, double)> initialCondition);

    /**
     * @brief Set Dirichlet boundary conditions (zero on all boundaries)
     */
    void setDirichletBC(double value = 0.0);

    /**
     * @brief Solve for one time step (explicit Euler)
     */
    void stepExplicit(double dt);

    /**
     * @brief Solve for multiple time steps
     */
    void solve(double dt, int nSteps, int outputInterval = 0);

    /**
     * @brief Get local concentration (includes ghost cells)
     */
    const std::vector<std::vector<double>>& getLocalConcentration() const { return C_; }

    /**
     * @brief Get performance statistics
     */
    const MPISolverStats& getStats() const { return stats_; }

    /**
     * @brief Gather global solution to root
     */
    void gatherGlobalSolution(std::vector<std::vector<double>>& globalC);

    /**
     * @brief Check CFL condition
     */
    double checkCFL(double dt) const;

    /**
     * @brief Get current time
     */
    double getCurrentTime() const { return currentTime_; }

private:
    void exchangeGhostCells();
    void applyBoundaryConditions();
    void setupDomain();

    const mpi::MPIComm& comm_;
    int rank_;
    int nprocs_;

    int globalNx_, globalNy_;
    double Lx_, Ly_;
    double dx_, dy_;
    double diffusivity_;

    // Domain decomposition
    std::unique_ptr<domain::CartesianDecomposition> decomp_;
    int localNx_, localNy_;
    int localStartX_, localStartY_;
    int localEndX_, localEndY_;

    // Solution (2D array: C_[i][j] where i is local x, j is local y)
    std::vector<std::vector<double>> C_;
    std::vector<std::vector<double>> C_new_;

    double bcValue_;
    double currentTime_;
    MPISolverStats stats_;
    std::shared_ptr<physics::FickDiffusion> fick_;
};

} // namespace solver
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_SOLVER_MPI_DIFFUSION_SOLVER_H
