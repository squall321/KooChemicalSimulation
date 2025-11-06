/**
 * @file SolverOptions.h
 * @brief Solver configuration options
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha1
 * @date 2025-11-06
 *
 * Defines configuration options for PDE solvers.
 */

#ifndef KOO_SOLVER_PDE_SOLVER_OPTIONS_H
#define KOO_SOLVER_PDE_SOLVER_OPTIONS_H

#include "SolverTypes.h"
#include <string>
#include <map>
#include <stdexcept>

namespace koo {
namespace solver {
namespace pde {

/**
 * @brief Solver options and configuration
 */
class SolverOptions {
public:
    SolverOptions()
        : linearSolver_(LinearSolverType::GMRES),
          preconditioner_(PreconditionerType::NONE),
          maxIterations_(1000),
          tolerance_(1e-6),
          relativeTolerance_(1e-6),
          verbose_(false),
          monitorConvergence_(false) {}

    // === Linear Solver Options ===

    /**
     * @brief Set linear solver type
     */
    void setLinearSolver(LinearSolverType type) { linearSolver_ = type; }

    /**
     * @brief Get linear solver type
     */
    LinearSolverType getLinearSolver() const { return linearSolver_; }

    /**
     * @brief Set preconditioner type
     */
    void setPreconditioner(PreconditionerType type) { preconditioner_ = type; }

    /**
     * @brief Get preconditioner type
     */
    PreconditionerType getPreconditioner() const { return preconditioner_; }

    // === Convergence Options ===

    /**
     * @brief Set maximum iterations
     */
    void setMaxIterations(int maxIter) { maxIterations_ = maxIter; }

    /**
     * @brief Get maximum iterations
     */
    int getMaxIterations() const { return maxIterations_; }

    /**
     * @brief Set absolute tolerance
     */
    void setTolerance(double tol) { tolerance_ = tol; }

    /**
     * @brief Get absolute tolerance
     */
    double getTolerance() const { return tolerance_; }

    /**
     * @brief Set relative tolerance
     */
    void setRelativeTolerance(double relTol) { relativeTolerance_ = relTol; }

    /**
     * @brief Get relative tolerance
     */
    double getRelativeTolerance() const { return relativeTolerance_; }

    // === Output Options ===

    /**
     * @brief Set verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Check if verbose output enabled
     */
    bool isVerbose() const { return verbose_; }

    /**
     * @brief Set convergence monitoring
     */
    void setMonitorConvergence(bool monitor) { monitorConvergence_ = monitor; }

    /**
     * @brief Check if convergence monitoring enabled
     */
    bool isMonitoringConvergence() const { return monitorConvergence_; }

    // === Time Integration Options ===

    /**
     * @brief Set time integration scheme
     */
    void setTimeIntegrationScheme(TimeIntegrationScheme scheme) {
        timeScheme_ = scheme;
    }

    /**
     * @brief Get time integration scheme
     */
    TimeIntegrationScheme getTimeIntegrationScheme() const { return timeScheme_; }

    /**
     * @brief Set time step
     */
    void setTimeStep(double dt) { timeStep_ = dt; }

    /**
     * @brief Get time step
     */
    double getTimeStep() const { return timeStep_; }

    /**
     * @brief Set number of time steps
     */
    void setNumTimeSteps(int numSteps) { numTimeSteps_ = numSteps; }

    /**
     * @brief Get number of time steps
     */
    int getNumTimeSteps() const { return numTimeSteps_; }

    /**
     * @brief Set final time
     */
    void setFinalTime(double tf) { finalTime_ = tf; }

    /**
     * @brief Get final time
     */
    double getFinalTime() const { return finalTime_; }

    // === FE Space Options ===

    /**
     * @brief Set finite element space type
     */
    void setFESpaceType(FESpaceType type) { feSpace_ = type; }

    /**
     * @brief Get finite element space type
     */
    FESpaceType getFESpaceType() const { return feSpace_; }

    /**
     * @brief Set polynomial order
     */
    void setPolynomialOrder(int order) { polynomialOrder_ = order; }

    /**
     * @brief Get polynomial order
     */
    int getPolynomialOrder() const { return polynomialOrder_; }

    // === Custom Parameters ===

    /**
     * @brief Set custom parameter
     */
    void setParameter(const std::string& key, double value) {
        customParameters_[key] = value;
    }

    /**
     * @brief Get custom parameter
     */
    double getParameter(const std::string& key, double defaultValue = 0.0) const {
        auto it = customParameters_.find(key);
        return (it != customParameters_.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Check if parameter exists
     */
    bool hasParameter(const std::string& key) const {
        return customParameters_.find(key) != customParameters_.end();
    }

    // === Validation ===

    /**
     * @brief Validate options
     */
    bool validate(std::string& errorMsg) const {
        if (maxIterations_ <= 0) {
            errorMsg = "Max iterations must be positive";
            return false;
        }

        if (tolerance_ <= 0.0) {
            errorMsg = "Tolerance must be positive";
            return false;
        }

        if (relativeTolerance_ <= 0.0) {
            errorMsg = "Relative tolerance must be positive";
            return false;
        }

        if (timeStep_ <= 0.0 && numTimeSteps_ > 0) {
            errorMsg = "Time step must be positive for time-dependent problems";
            return false;
        }

        if (polynomialOrder_ < 1) {
            errorMsg = "Polynomial order must be >= 1";
            return false;
        }

        return true;
    }

    /**
     * @brief Get string representation
     */
    std::string toString() const {
        std::string result = "Solver Options:\n";
        result += "  Linear Solver: " + pde::toString(linearSolver_) + "\n";
        result += "  Preconditioner: " + pde::toString(preconditioner_) + "\n";
        result += "  Max Iterations: " + std::to_string(maxIterations_) + "\n";
        result += "  Tolerance: " + std::to_string(tolerance_) + "\n";
        result += "  Relative Tolerance: " + std::to_string(relativeTolerance_) + "\n";
        result += "  Verbose: " + std::string(verbose_ ? "yes" : "no") + "\n";
        result += "  FE Space: " + pde::toString(feSpace_) + "\n";
        result += "  Polynomial Order: " + std::to_string(polynomialOrder_) + "\n";

        if (numTimeSteps_ > 0) {
            result += "  Time Integration: " + pde::toString(timeScheme_) + "\n";
            result += "  Time Step: " + std::to_string(timeStep_) + "\n";
            result += "  Num Steps: " + std::to_string(numTimeSteps_) + "\n";
        }

        return result;
    }

private:
    // Linear solver settings
    LinearSolverType linearSolver_;
    PreconditionerType preconditioner_;
    int maxIterations_;
    double tolerance_;
    double relativeTolerance_;

    // Output settings
    bool verbose_;
    bool monitorConvergence_;

    // Time integration settings
    TimeIntegrationScheme timeScheme_{TimeIntegrationScheme::IMPLICIT_EULER};
    double timeStep_{0.01};
    int numTimeSteps_{0};
    double finalTime_{0.0};

    // FE space settings
    FESpaceType feSpace_{FESpaceType::H1};
    int polynomialOrder_{1};

    // Custom parameters
    std::map<std::string, double> customParameters_;
};

/**
 * @brief Common solver option presets
 */
namespace presets {

/**
 * @brief Get default options
 */
inline SolverOptions getDefault() {
    return SolverOptions();
}

/**
 * @brief Get options for fast solve (lower accuracy)
 */
inline SolverOptions getFast() {
    SolverOptions opts;
    opts.setLinearSolver(LinearSolverType::CG);
    opts.setPreconditioner(PreconditionerType::JACOBI);
    opts.setMaxIterations(100);
    opts.setTolerance(1e-4);
    opts.setRelativeTolerance(1e-4);
    return opts;
}

/**
 * @brief Get options for accurate solve
 */
inline SolverOptions getAccurate() {
    SolverOptions opts;
    opts.setLinearSolver(LinearSolverType::GMRES);
    opts.setPreconditioner(PreconditionerType::ILU);
    opts.setMaxIterations(5000);
    opts.setTolerance(1e-10);
    opts.setRelativeTolerance(1e-10);
    return opts;
}

/**
 * @brief Get options for time-dependent problems
 */
inline SolverOptions getTimeDependent(double dt, double finalTime) {
    SolverOptions opts;
    opts.setLinearSolver(LinearSolverType::GMRES);
    opts.setPreconditioner(PreconditionerType::ILU);
    opts.setTimeIntegrationScheme(TimeIntegrationScheme::CRANK_NICOLSON);
    opts.setTimeStep(dt);
    opts.setFinalTime(finalTime);
    opts.setNumTimeSteps(static_cast<int>(finalTime / dt));
    return opts;
}

} // namespace presets

} // namespace pde
} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_PDE_SOLVER_OPTIONS_H
