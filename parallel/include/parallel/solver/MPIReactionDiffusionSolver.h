/**
 * @file MPIReactionDiffusionSolver.h
 * @brief MPI-parallel reaction-diffusion equation solver
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 * @date 2025-11-08
 *
 * Priority D1: MPI Parallelization
 *
 * MPI-parallel solver for reaction-diffusion systems:
 *   ∂u/∂t = D_u ∇²u + f(u,v)
 *   ∂v/∂t = D_v ∇²v + g(u,v)
 *
 * Supports multiple reaction-diffusion models:
 * - Gray-Scott model (pattern formation)
 * - Brusselator model (oscillations)
 * - FitzHugh-Nagumo model (excitable media)
 */

#ifndef KOO_PARALLEL_SOLVER_MPI_REACTION_DIFFUSION_SOLVER_H
#define KOO_PARALLEL_SOLVER_MPI_REACTION_DIFFUSION_SOLVER_H

#include "parallel/mpi/MPIWrapper.h"
#include "parallel/domain/DomainDecomposition.h"
#include "parallel/solver/MPIDiffusionSolver.h"
#include "physics/coupling/ReactionDiffusion.h"
#include <vector>
#include <memory>
#include <functional>

namespace koo {
namespace parallel {
namespace solver {

// ============================================================================
// Reaction-Diffusion Models
// ============================================================================

/**
 * @brief Gray-Scott reaction-diffusion model
 *
 * u + 2v → 3v  (reaction rate k)
 * v → P        (decay rate F)
 *
 * ∂u/∂t = D_u ∇²u - uv² + F(1-u)
 * ∂v/∂t = D_v ∇²v + uv² - (F+k)v
 *
 * Classic patterns: spots, stripes, spirals, worms
 */
struct GrayScottParams {
    double Du = 2.0e-5;     ///< Diffusion coefficient for u
    double Dv = 1.0e-5;     ///< Diffusion coefficient for v
    double F = 0.055;       ///< Feed rate
    double k = 0.062;       ///< Kill rate

    // Classic parameter sets for different patterns:
    // Spots:    F=0.055, k=0.062
    // Stripes:  F=0.035, k=0.065
    // Spirals:  F=0.018, k=0.051
    // Worms:    F=0.078, k=0.061
};

/**
 * @brief Brusselator reaction-diffusion model
 *
 * Oscillatory reaction system
 *
 * ∂u/∂t = D_u ∇²u + a - (b+1)u + u²v
 * ∂v/∂t = D_v ∇²v + bu - u²v
 */
struct BrusselatorParams {
    double Du = 1.0e-5;     ///< Diffusion coefficient for u
    double Dv = 2.0e-5;     ///< Diffusion coefficient for v
    double a = 1.0;         ///< Parameter a
    double b = 3.0;         ///< Parameter b
};

// ============================================================================
// MPI Reaction-Diffusion Solver (2D)
// ============================================================================

/**
 * @brief 2D MPI-parallel reaction-diffusion solver
 *
 * Solves coupled PDEs:
 *   ∂u/∂t = D_u (∂²u/∂x² + ∂²u/∂y²) + f(u,v)
 *   ∂v/∂t = D_v (∂²v/∂x² + ∂²v/∂y²) + g(u,v)
 */
class MPIReactionDiffusionSolver2D {
public:
    using ReactionFunction = std::function<double(double, double)>;

    /**
     * @brief Constructor
     * @param globalNx Global grid points in x
     * @param globalNy Global grid points in y
     * @param Lx Domain length in x (m)
     * @param Ly Domain length in y (m)
     * @param Du Diffusion coefficient for species u
     * @param Dv Diffusion coefficient for species v
     * @param reactionU Reaction term for u: f(u,v)
     * @param reactionV Reaction term for v: g(u,v)
     * @param comm MPI communicator
     */
    MPIReactionDiffusionSolver2D(
        int globalNx, int globalNy,
        double Lx, double Ly,
        double Du, double Dv,
        ReactionFunction reactionU,
        ReactionFunction reactionV,
        const mpi::MPIComm& comm);

    /**
     * @brief Create Gray-Scott model solver
     */
    static std::unique_ptr<MPIReactionDiffusionSolver2D> createGrayScott(
        int globalNx, int globalNy,
        double Lx, double Ly,
        const GrayScottParams& params,
        const mpi::MPIComm& comm);

    /**
     * @brief Create Brusselator model solver
     */
    static std::unique_ptr<MPIReactionDiffusionSolver2D> createBrusselator(
        int globalNx, int globalNy,
        double Lx, double Ly,
        const BrusselatorParams& params,
        const mpi::MPIComm& comm);

    /**
     * @brief Set initial condition for both species
     * @param initialU Function: (x, y) -> u(x, y, t=0)
     * @param initialV Function: (x, y) -> v(x, y, t=0)
     */
    void setInitialCondition(
        std::function<double(double, double)> initialU,
        std::function<double(double, double)> initialV);

    /**
     * @brief Set Dirichlet boundary conditions
     */
    void setDirichletBC(double uValue = 0.0, double vValue = 0.0);

    /**
     * @brief Set periodic boundary conditions
     */
    void setPeriodicBC();

    /**
     * @brief Solve for one time step (explicit Euler)
     */
    void stepExplicit(double dt);

    /**
     * @brief Solve for multiple time steps
     */
    void solve(double dt, int nSteps, int outputInterval = 0);

    /**
     * @brief Get local concentration for species u
     */
    const std::vector<std::vector<double>>& getLocalU() const { return u_; }

    /**
     * @brief Get local concentration for species v
     */
    const std::vector<std::vector<double>>& getLocalV() const { return v_; }

    /**
     * @brief Get performance statistics
     */
    const MPISolverStats& getStats() const { return stats_; }

    /**
     * @brief Check CFL condition
     */
    double checkCFL(double dt) const;

    /**
     * @brief Get current time
     */
    double getCurrentTime() const { return currentTime_; }

    /**
     * @brief Write local subdomain to VTK (each process writes its own file)
     */
    void writeLocalVTK(const std::string& baseFilename, int step);

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
    double Du_, Dv_;

    // Reaction functions
    ReactionFunction reactionU_;
    ReactionFunction reactionV_;

    // Domain decomposition
    std::unique_ptr<domain::CartesianDecomposition> decomp_;
    int localNx_, localNy_;
    int localStartX_, localStartY_;
    int localEndX_, localEndY_;

    // Solution (2D arrays with ghost cells)
    std::vector<std::vector<double>> u_;      ///< Species u
    std::vector<std::vector<double>> v_;      ///< Species v
    std::vector<std::vector<double>> u_new_;  ///< Updated u
    std::vector<std::vector<double>> v_new_;  ///< Updated v

    // Boundary conditions
    enum class BCType { Dirichlet, Periodic };
    BCType bcType_;
    double bcU_, bcV_;

    double currentTime_;
    MPISolverStats stats_;

    bool isPeriodic_;
};

// ============================================================================
// MPI Reaction-Diffusion Solver (1D)
// ============================================================================

/**
 * @brief 1D MPI-parallel reaction-diffusion solver
 */
class MPIReactionDiffusionSolver1D {
public:
    using ReactionFunction = std::function<double(double, double)>;

    /**
     * @brief Constructor
     */
    MPIReactionDiffusionSolver1D(
        int globalNx, double L,
        double Du, double Dv,
        ReactionFunction reactionU,
        ReactionFunction reactionV,
        const mpi::MPIComm& comm);

    void setInitialCondition(
        std::function<double(double)> initialU,
        std::function<double(double)> initialV);

    void setDirichletBC(double uLeft, double uRight, double vLeft, double vRight);
    void setNeumannBC(double uFluxLeft, double uFluxRight,
                     double vFluxLeft, double vFluxRight);

    void stepExplicit(double dt);
    void solve(double dt, int nSteps, int outputInterval = 0);

    const std::vector<double>& getLocalU() const { return u_; }
    const std::vector<double>& getLocalV() const { return v_; }

    void gatherGlobalSolution(
        std::vector<double>& globalU, std::vector<double>& globalV,
        std::vector<double>& globalX);

    double checkCFL(double dt) const;
    double getCurrentTime() const { return currentTime_; }
    const MPISolverStats& getStats() const { return stats_; }

private:
    void exchangeGhostCells();
    void applyBoundaryConditions();
    void setupDomain();

    const mpi::MPIComm& comm_;
    int rank_;
    int nprocs_;

    int globalNx_;
    double L_;
    double dx_;
    double Du_, Dv_;

    ReactionFunction reactionU_;
    ReactionFunction reactionV_;

    int localNx_;
    int localStart_, localEnd_;
    int leftNeighbor_, rightNeighbor_;

    std::vector<double> u_, v_;
    std::vector<double> u_new_, v_new_;
    std::vector<double> x_;

    enum class BCType { Dirichlet, Neumann };
    BCType bcType_;
    double bcULeft_, bcURight_, bcVLeft_, bcVRight_;

    double currentTime_;
    MPISolverStats stats_;
};

} // namespace solver
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_SOLVER_MPI_REACTION_DIFFUSION_SOLVER_H
