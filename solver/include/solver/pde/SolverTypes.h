/**
 * @file SolverTypes.h
 * @brief Common types and enumerations for PDE solvers
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha1
 * @date 2025-11-06
 *
 * Defines common types, enumerations, and structures used by all PDE solvers.
 */

#ifndef KOO_SOLVER_PDE_SOLVER_TYPES_H
#define KOO_SOLVER_PDE_SOLVER_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>

namespace koo {
namespace solver {
namespace pde {

/**
 * @brief PDE solver backend types
 */
enum class SolverBackend {
    NGSOLVE,        ///< NGSolve solver
    MFEM,           ///< MFEM solver
    DEALII,         ///< deal.II solver
    FENICS,         ///< FEniCS solver
    CUSTOM          ///< Custom implementation
};

/**
 * @brief PDE types
 */
enum class PDEType {
    ELLIPTIC,       ///< Elliptic PDE (e.g., Poisson, Laplace)
    PARABOLIC,      ///< Parabolic PDE (e.g., heat equation)
    HYPERBOLIC,     ///< Hyperbolic PDE (e.g., wave equation)
    MIXED,          ///< Mixed type
    NONLINEAR       ///< Nonlinear PDE
};

/**
 * @brief Solver types for linear systems
 */
enum class LinearSolverType {
    DIRECT,         ///< Direct solver (LU, Cholesky, etc.)
    CG,             ///< Conjugate Gradient
    GMRES,          ///< GMRES
    BICGSTAB,       ///< BiCGStab
    MINRES,         ///< MINRES
    CUSTOM          ///< Custom iterative solver
};

/**
 * @brief Preconditioner types
 */
enum class PreconditionerType {
    NONE,           ///< No preconditioning
    JACOBI,         ///< Jacobi preconditioner
    SSOR,           ///< Symmetric SOR
    ILU,            ///< Incomplete LU
    ICC,            ///< Incomplete Cholesky
    MULTIGRID,      ///< Multigrid preconditioner
    AMG,            ///< Algebraic multigrid
    CUSTOM          ///< Custom preconditioner
};

/**
 * @brief Time integration schemes
 */
enum class TimeIntegrationScheme {
    EXPLICIT_EULER,     ///< Explicit Euler
    IMPLICIT_EULER,     ///< Implicit Euler
    CRANK_NICOLSON,     ///< Crank-Nicolson
    BDF2,               ///< Backward Differentiation Formula (order 2)
    RK4,                ///< Runge-Kutta 4th order
    ADAPTIVE            ///< Adaptive time stepping
};

/**
 * @brief Solver status
 */
enum class SolverStatus {
    NOT_INITIALIZED,    ///< Solver not initialized
    INITIALIZED,        ///< Solver initialized
    SOLVING,            ///< Currently solving
    CONVERGED,          ///< Solution converged
    DIVERGED,           ///< Solution diverged
    MAX_ITERATIONS,     ///< Maximum iterations reached
    ERROR               ///< Error occurred
};

/**
 * @brief Finite element space types
 */
enum class FESpaceType {
    H1,             ///< H1 (continuous) space
    L2,             ///< L2 (discontinuous) space
    HCURL,          ///< H(curl) space
    HDIV,           ///< H(div) space
    MIXED,          ///< Mixed space
    DG              ///< Discontinuous Galerkin
};

/**
 * @brief Solver convergence information
 */
struct ConvergenceInfo {
    int iterations{0};              ///< Number of iterations
    double residual{0.0};           ///< Final residual
    double relativeError{0.0};      ///< Relative error
    double solutionNorm{0.0};       ///< Solution norm
    double timeElapsed{0.0};        ///< Time elapsed (seconds)
    SolverStatus status{SolverStatus::NOT_INITIALIZED};  ///< Solver status
    std::string message;            ///< Status message

    /**
     * @brief Check if solver converged
     */
    bool converged() const {
        return status == SolverStatus::CONVERGED;
    }

    /**
     * @brief Get string representation
     */
    std::string toString() const {
        std::string result = "ConvergenceInfo:\n";
        result += "  Status: " + statusToString(status) + "\n";
        result += "  Iterations: " + std::to_string(iterations) + "\n";
        result += "  Residual: " + std::to_string(residual) + "\n";
        result += "  Relative Error: " + std::to_string(relativeError) + "\n";
        result += "  Solution Norm: " + std::to_string(solutionNorm) + "\n";
        result += "  Time: " + std::to_string(timeElapsed) + " s\n";
        if (!message.empty()) {
            result += "  Message: " + message + "\n";
        }
        return result;
    }

    /**
     * @brief Convert status to string
     */
    static std::string statusToString(SolverStatus status) {
        switch (status) {
            case SolverStatus::NOT_INITIALIZED: return "Not Initialized";
            case SolverStatus::INITIALIZED:     return "Initialized";
            case SolverStatus::SOLVING:         return "Solving";
            case SolverStatus::CONVERGED:       return "Converged";
            case SolverStatus::DIVERGED:        return "Diverged";
            case SolverStatus::MAX_ITERATIONS:  return "Max Iterations";
            case SolverStatus::ERROR:           return "Error";
            default:                            return "Unknown";
        }
    }
};

/**
 * @brief Solution vector wrapper
 */
class SolutionVector {
public:
    SolutionVector() = default;

    explicit SolutionVector(size_t size) : data_(size, 0.0) {}

    SolutionVector(const std::vector<double>& data) : data_(data) {}

    /**
     * @brief Get size
     */
    size_t size() const { return data_.size(); }

    /**
     * @brief Resize vector
     */
    void resize(size_t newSize) { data_.resize(newSize); }

    /**
     * @brief Access element
     */
    double& operator[](size_t i) { return data_[i]; }
    const double& operator[](size_t i) const { return data_[i]; }

    /**
     * @brief Get data pointer
     */
    double* data() { return data_.data(); }
    const double* data() const { return data_.data(); }

    /**
     * @brief Get data vector
     */
    std::vector<double>& getData() { return data_; }
    const std::vector<double>& getData() const { return data_; }

    /**
     * @brief Set all values to zero
     */
    void zero() {
        std::fill(data_.begin(), data_.end(), 0.0);
    }

    /**
     * @brief Set all values to a constant
     */
    void setConstant(double value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    /**
     * @brief Compute L2 norm
     */
    double norm() const {
        double sum = 0.0;
        for (double val : data_) {
            sum += val * val;
        }
        return std::sqrt(sum);
    }

private:
    std::vector<double> data_;
};

/**
 * @brief String conversion utilities
 */
inline std::string toString(SolverBackend backend) {
    switch (backend) {
        case SolverBackend::NGSOLVE: return "NGSolve";
        case SolverBackend::MFEM:    return "MFEM";
        case SolverBackend::DEALII:  return "deal.II";
        case SolverBackend::FENICS:  return "FEniCS";
        case SolverBackend::CUSTOM:  return "Custom";
        default:                     return "Unknown";
    }
}

inline std::string toString(PDEType type) {
    switch (type) {
        case PDEType::ELLIPTIC:   return "Elliptic";
        case PDEType::PARABOLIC:  return "Parabolic";
        case PDEType::HYPERBOLIC: return "Hyperbolic";
        case PDEType::MIXED:      return "Mixed";
        case PDEType::NONLINEAR:  return "Nonlinear";
        default:                  return "Unknown";
    }
}

inline std::string toString(LinearSolverType type) {
    switch (type) {
        case LinearSolverType::DIRECT:   return "Direct";
        case LinearSolverType::CG:       return "CG";
        case LinearSolverType::GMRES:    return "GMRES";
        case LinearSolverType::BICGSTAB: return "BiCGStab";
        case LinearSolverType::MINRES:   return "MINRES";
        case LinearSolverType::CUSTOM:   return "Custom";
        default:                         return "Unknown";
    }
}

inline std::string toString(PreconditionerType type) {
    switch (type) {
        case PreconditionerType::NONE:      return "None";
        case PreconditionerType::JACOBI:    return "Jacobi";
        case PreconditionerType::SSOR:      return "SSOR";
        case PreconditionerType::ILU:       return "ILU";
        case PreconditionerType::ICC:       return "ICC";
        case PreconditionerType::MULTIGRID: return "Multigrid";
        case PreconditionerType::AMG:       return "AMG";
        case PreconditionerType::CUSTOM:    return "Custom";
        default:                            return "Unknown";
    }
}

inline std::string toString(FESpaceType type) {
    switch (type) {
        case FESpaceType::H1:    return "H1";
        case FESpaceType::L2:    return "L2";
        case FESpaceType::HCURL: return "H(curl)";
        case FESpaceType::HDIV:  return "H(div)";
        case FESpaceType::MIXED: return "Mixed";
        case FESpaceType::DG:    return "DG";
        default:                 return "Unknown";
    }
}

inline std::string toString(TimeIntegrationScheme scheme) {
    switch (scheme) {
        case TimeIntegrationScheme::EXPLICIT_EULER: return "Explicit Euler";
        case TimeIntegrationScheme::IMPLICIT_EULER: return "Implicit Euler";
        case TimeIntegrationScheme::CRANK_NICOLSON: return "Crank-Nicolson";
        case TimeIntegrationScheme::BDF2:           return "BDF2";
        case TimeIntegrationScheme::RK4:            return "RK4";
        case TimeIntegrationScheme::ADAPTIVE:       return "Adaptive";
        default:                                    return "Unknown";
    }
}

} // namespace pde
} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_PDE_SOLVER_TYPES_H
