/**
 * @file ISolver.h
 * @brief Abstract interface for PDE solvers
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha1
 * @date 2025-11-06
 *
 * Defines the abstract interface that all PDE solvers must implement.
 */

#ifndef KOO_SOLVER_PDE_ISOLVER_H
#define KOO_SOLVER_PDE_ISOLVER_H

#include "SolverTypes.h"
#include "SolverOptions.h"
#include "LinearSystem.h"
#include "mesh/MeshManager.h"
#include <memory>
#include <string>

namespace koo {
namespace solver {
namespace pde {

/**
 * @brief Abstract interface for PDE solvers
 *
 * This interface defines the common operations that all PDE solvers
 * (NGSolve, MFEM, deal.II, etc.) must implement.
 */
class ISolver {
public:
    virtual ~ISolver() = default;

    /**
     * @brief Get solver backend type
     */
    virtual SolverBackend getBackend() const = 0;

    /**
     * @brief Get solver name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get solver version
     */
    virtual std::string getVersion() const {
        return "1.0.0";
    }

    // === Initialization ===

    /**
     * @brief Initialize solver with mesh
     * @param meshManager Mesh manager containing the computational domain
     * @return True if initialization successful
     */
    virtual bool initialize(std::shared_ptr<mesh::MeshManager> meshManager) = 0;

    /**
     * @brief Check if solver is initialized
     */
    virtual bool isInitialized() const = 0;

    // === Configuration ===

    /**
     * @brief Set solver options
     */
    virtual void setOptions(const SolverOptions& options) = 0;

    /**
     * @brief Get solver options
     */
    virtual const SolverOptions& getOptions() const = 0;

    /**
     * @brief Set PDE type
     */
    virtual void setPDEType(PDEType type) = 0;

    /**
     * @brief Get PDE type
     */
    virtual PDEType getPDEType() const = 0;

    // === Assembly ===

    /**
     * @brief Assemble system matrix and RHS
     * @return True if assembly successful
     */
    virtual bool assemble() = 0;

    /**
     * @brief Apply boundary conditions
     * @return True if successful
     */
    virtual bool applyBoundaryConditions() = 0;

    // === Solution ===

    /**
     * @brief Solve the PDE system
     * @return Convergence information
     */
    virtual ConvergenceInfo solve() = 0;

    /**
     * @brief Solve time-dependent problem
     * @return Convergence information for final time step
     */
    virtual ConvergenceInfo solveTimeDependent() {
        // Default implementation: single time step
        return solve();
    }

    // === Results ===

    /**
     * @brief Get solution vector
     */
    virtual const SolutionVector& getSolution() const = 0;

    /**
     * @brief Get solution at specific point
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Solution value
     */
    virtual double getSolutionAt(double x, double y, double z) const = 0;

    /**
     * @brief Export solution to file
     * @param filename Output filename
     * @param format File format
     * @return True if successful
     */
    virtual bool exportSolution(const std::string& filename,
                                const std::string& format = "vtk") const = 0;

    // === System Access ===

    /**
     * @brief Get linear system
     */
    virtual const LinearSystem& getLinearSystem() const = 0;

    /**
     * @brief Get last convergence info
     */
    virtual const ConvergenceInfo& getLastConvergenceInfo() const = 0;

    // === Status ===

    /**
     * @brief Get solver status
     */
    virtual SolverStatus getStatus() const = 0;

    /**
     * @brief Get last error message
     */
    virtual std::string getLastError() const = 0;

    /**
     * @brief Reset solver
     */
    virtual void reset() = 0;

    // === Utility ===

    /**
     * @brief Print solver information
     */
    virtual void printInfo() const {
        std::cout << "Solver: " << getName() << "\n";
        std::cout << "Backend: " << toString(getBackend()) << "\n";
        std::cout << "Version: " << getVersion() << "\n";
        std::cout << "PDE Type: " << toString(getPDEType()) << "\n";
        std::cout << "Status: " << ConvergenceInfo::statusToString(getStatus()) << "\n";
    }

    /**
     * @brief Get statistics
     */
    virtual std::string getStatistics() const {
        std::string result = "Solver Statistics:\n";
        result += "  Name: " + getName() + "\n";
        result += "  Backend: " + toString(getBackend()) + "\n";
        result += "  Status: " + ConvergenceInfo::statusToString(getStatus()) + "\n";

        const auto& info = getLastConvergenceInfo();
        if (info.converged()) {
            result += "  Last Solve:\n";
            result += "    Iterations: " + std::to_string(info.iterations) + "\n";
            result += "    Residual: " + std::to_string(info.residual) + "\n";
            result += "    Time: " + std::to_string(info.timeElapsed) + " s\n";
        }

        return result;
    }
};

/**
 * @brief Base class for PDE solvers with common functionality
 */
class BaseSolver : public ISolver {
public:
    BaseSolver()
        : pdeType_(PDEType::ELLIPTIC),
          status_(SolverStatus::NOT_INITIALIZED),
          initialized_(false) {}

    virtual ~BaseSolver() = default;

    // Implement common functionality

    bool isInitialized() const override {
        return initialized_;
    }

    void setOptions(const SolverOptions& options) override {
        options_ = options;
    }

    const SolverOptions& getOptions() const override {
        return options_;
    }

    void setPDEType(PDEType type) override {
        pdeType_ = type;
    }

    PDEType getPDEType() const override {
        return pdeType_;
    }

    const SolutionVector& getSolution() const override {
        return solution_;
    }

    const LinearSystem& getLinearSystem() const override {
        return linearSystem_;
    }

    const ConvergenceInfo& getLastConvergenceInfo() const override {
        return lastConvergenceInfo_;
    }

    SolverStatus getStatus() const override {
        return status_;
    }

    std::string getLastError() const override {
        return lastError_;
    }

    void reset() override {
        initialized_ = false;
        status_ = SolverStatus::NOT_INITIALIZED;
        lastError_.clear();
        solution_.resize(0);
    }

protected:
    std::shared_ptr<mesh::MeshManager> meshManager_;
    SolverOptions options_;
    PDEType pdeType_;
    SolverStatus status_;
    bool initialized_;

    LinearSystem linearSystem_;
    SolutionVector solution_;
    ConvergenceInfo lastConvergenceInfo_;
    std::string lastError_;
};

} // namespace pde
} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_PDE_ISOLVER_H
