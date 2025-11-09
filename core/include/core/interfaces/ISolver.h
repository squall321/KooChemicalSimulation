/**
 * @file ISolver.h
 * @brief Interface for PDE solvers
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha2
 * @date 2025-11-06
 *
 * This file defines the ISolver interface, which provides an abstraction
 * for different PDE solver backends (NGSolve, MFEM, etc.).
 */

#ifndef KOO_CORE_INTERFACES_ISOLVER_H
#define KOO_CORE_INTERFACES_ISOLVER_H

#include <memory>
#include <string>
#include <vector>

namespace koo {
namespace core {

// Forward declarations
class IMesh;

/**
 * @brief Enumeration of solver types
 */
enum class SolverType {
    DIRECT,      ///< Direct solver (LU, Cholesky, etc.)
    ITERATIVE,   ///< Iterative solver (CG, GMRES, etc.)
    MULTIGRID,   ///< Multigrid solver
    CUSTOM       ///< Custom solver implementation
};

/**
 * @brief Enumeration of solver status
 */
enum class SolverStatus {
    NOT_INITIALIZED,  ///< Solver has not been initialized
    READY,            ///< Solver is ready to solve
    SOLVING,          ///< Solver is currently solving
    CONVERGED,        ///< Solver converged successfully
    FAILED,           ///< Solver failed to converge
    ERROR             ///< Solver encountered an error
};

/**
 * @brief Interface for PDE solvers
 *
 * ISolver provides a unified interface for different finite element method
 * (FEM) solvers. This abstraction allows the framework to work with multiple
 * solver backends (NGSolve, MFEM, deal.II, etc.) through a common API.
 *
 * Key responsibilities:
 * - Setting up the discretization and finite element space
 * - Assembling system matrices and vectors
 * - Solving linear and nonlinear systems
 * - Providing access to solution vectors
 * - Managing solver parameters and convergence criteria
 *
 * Design Pattern: Strategy Pattern
 *
 * @note Implementations should be thread-safe for read operations
 *
 * Example usage:
 * @code
 * auto solver = createNGSolveSolver();
 * solver->setup(mesh);
 * solver->assemble();
 * solver->solve();
 * auto solution = solver->getSolution();
 * @endcode
 */
class ISolver {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ISolver() = default;

    /**
     * @brief Set up the solver with a mesh
     *
     * Initializes the finite element space, degrees of freedom, and
     * allocates memory for system matrices and vectors.
     *
     * @param mesh Shared pointer to the mesh object
     * @throws std::invalid_argument if mesh is nullptr
     * @throws std::runtime_error if setup fails
     */
    virtual void setup(std::shared_ptr<IMesh> mesh) = 0;

    /**
     * @brief Assemble the system matrix and right-hand side
     *
     * Constructs the global system of equations by assembling contributions
     * from all elements. This should be called before solve().
     *
     * @throws std::runtime_error if assembly fails or solver not set up
     */
    virtual void assemble() = 0;

    /**
     * @brief Solve the assembled system
     *
     * Solves the linear or nonlinear system of equations. The solution
     * is stored internally and can be retrieved via getSolution().
     *
     * @return true if solver converged, false otherwise
     * @throws std::runtime_error if solve fails
     */
    virtual bool solve() = 0;

    /**
     * @brief Get the solution vector
     *
     * Returns the solution computed by the last call to solve().
     *
     * @return Reference to solution vector (solver-specific format)
     * @throws std::runtime_error if no solution available
     */
    virtual const std::vector<double>& getSolution() const = 0;

    /**
     * @brief Get solver type
     *
     * @return The type of this solver
     */
    virtual SolverType getType() const = 0;

    /**
     * @brief Get current solver status
     *
     * @return Current status of the solver
     */
    virtual SolverStatus getStatus() const = 0;

    /**
     * @brief Get name/identifier of the solver
     *
     * @return Solver name (e.g., "NGSolve", "MFEM", "Custom")
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Set convergence tolerance
     *
     * Sets the tolerance for iterative solvers. The solver stops when
     * the relative residual falls below this value.
     *
     * @param tol Convergence tolerance (must be positive)
     * @throws std::invalid_argument if tol <= 0
     */
    virtual void setTolerance(double tol) = 0;

    /**
     * @brief Get current convergence tolerance
     *
     * @return Convergence tolerance
     */
    virtual double getTolerance() const = 0;

    /**
     * @brief Set maximum number of iterations
     *
     * For iterative solvers, sets the maximum number of iterations allowed.
     *
     * @param maxIter Maximum iterations (must be positive)
     * @throws std::invalid_argument if maxIter == 0
     */
    virtual void setMaxIterations(size_t maxIter) = 0;

    /**
     * @brief Get maximum number of iterations
     *
     * @return Maximum iterations
     */
    virtual size_t getMaxIterations() const = 0;

    /**
     * @brief Get number of iterations performed in last solve
     *
     * @return Number of iterations (0 for direct solvers)
     */
    virtual size_t getIterationCount() const = 0;

    /**
     * @brief Get final residual from last solve
     *
     * @return Residual norm
     */
    virtual double getResidual() const = 0;

    /**
     * @brief Reset the solver to initial state
     *
     * Clears assembled matrices and solution vectors. After reset,
     * setup() and assemble() must be called again before solve().
     */
    virtual void reset() = 0;

    /**
     * @brief Get number of degrees of freedom
     *
     * @return Total number of DOFs in the system
     */
    virtual size_t getNumDOFs() const = 0;

    /**
     * @brief Check if solver supports nonlinear problems
     *
     * @return true if nonlinear problems are supported
     */
    virtual bool supportsNonlinear() const {
        return false; // Default: linear only
    }

    /**
     * @brief Check if solver supports time-dependent problems
     *
     * @return true if time integration is supported
     */
    virtual bool supportsTimeDependent() const {
        return false; // Default: steady-state only
    }

protected:
    /**
     * @brief Protected default constructor
     */
    ISolver() = default;

    /**
     * @brief Protected copy constructor (deleted - prevent copying)
     */
    ISolver(const ISolver&) = delete;

    /**
     * @brief Protected copy assignment (deleted - prevent copying)
     */
    ISolver& operator=(const ISolver&) = delete;

    /**
     * @brief Protected move constructor
     */
    ISolver(ISolver&&) = default;

    /**
     * @brief Protected move assignment
     */
    ISolver& operator=(ISolver&&) = default;
};

/**
 * @brief Shared pointer type for ISolver
 */
using ISolverPtr = std::shared_ptr<ISolver>;

/**
 * @brief Unique pointer type for ISolver
 */
using ISolverUniquePtr = std::unique_ptr<ISolver>;

} // namespace core
} // namespace koo

#endif // KOO_CORE_INTERFACES_ISOLVER_H
