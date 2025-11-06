/**
 * @file Solvers.h
 * @brief GPU iterative linear solvers
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 52: GPU Linear Algebra
 *
 * Provides GPU-accelerated iterative solvers for sparse linear systems.
 * Implements Conjugate Gradient (CG) and BiCGStab algorithms.
 */

#ifndef KOO_GPU_LINALG_SOLVERS_H
#define KOO_GPU_LINALG_SOLVERS_H

#include "Vector.h"
#include "Matrix.h"
#include <iostream>
#include <iomanip>

namespace koo {
namespace gpu {
namespace linalg {

/**
 * @struct SolverStats
 * @brief Statistics from iterative solver
 *
 * Phase 52: Solver Information
 */
struct SolverStats {
    int iterations{0};          ///< Number of iterations performed
    double residual{0.0};       ///< Final residual norm
    double initialResidual{0.0}; ///< Initial residual norm
    bool converged{false};      ///< Convergence flag
    double elapsedTime{0.0};    ///< Elapsed time (ms)

    /**
     * @brief Get relative residual
     */
    double relativeResidual() const {
        return (initialResidual > 0) ? (residual / initialResidual) : 0.0;
    }

    /**
     * @brief Print statistics
     */
    void print(std::ostream& os = std::cout) const {
        os << "Solver Statistics:\n";
        os << "  Iterations: " << iterations << "\n";
        os << "  Initial Residual: " << std::scientific << initialResidual << "\n";
        os << "  Final Residual: " << std::scientific << residual << "\n";
        os << "  Relative Residual: " << std::scientific << relativeResidual() << "\n";
        os << "  Converged: " << (converged ? "Yes" : "No") << "\n";
        os << "  Time: " << std::fixed << std::setprecision(2) << elapsedTime << " ms\n";
    }
};

/**
 * @class ConjugateGradient
 * @brief Conjugate Gradient solver for symmetric positive definite systems
 *
 * Phase 52: GPU Iterative Solvers
 *
 * Solves: A * x = b
 * where A is symmetric positive definite.
 *
 * @tparam T Element type (float or double)
 */
template<typename T>
class ConjugateGradient {
public:
    /**
     * @brief Constructor
     * @param matrix Sparse matrix A
     * @param maxIter Maximum iterations (default: 1000)
     * @param tolerance Convergence tolerance (default: 1e-6)
     */
    explicit ConjugateGradient(const GPUSparseMatrixCSR<T>& matrix,
                              int maxIter = 1000,
                              double tolerance = 1e-6)
        : A_(matrix),
          maxIter_(maxIter),
          tolerance_(tolerance),
          verbose_(false) {}

    /**
     * @brief Set verbosity
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Set maximum iterations
     */
    void setMaxIterations(int maxIter) { maxIter_ = maxIter; }

    /**
     * @brief Set tolerance
     */
    void setTolerance(double tol) { tolerance_ = tol; }

    /**
     * @brief Solve: A * x = b
     * @param b Right-hand side
     * @param x Solution vector (initial guess)
     * @return Solver statistics
     */
    SolverStats solve(const GPUVector<T>& b, GPUVector<T>& x) {
        SolverStats stats;

        // Start timer
        Event startEvent, stopEvent;
        startEvent.record();

        int n = A_.numRows();

        if (x.size() != static_cast<size_t>(n)) {
            x.resize(n);
            x.zero();  // Zero initial guess
        }

        // Allocate workspace vectors
        GPUVector<T> r(n);  // Residual
        GPUVector<T> p(n);  // Search direction
        GPUVector<T> Ap(n); // A * p

        // r = b - A*x
        A_.spmv(x, r, -1.0, 0.0);  // r = -A*x
        r.axpy(1.0, b);             // r = b + r

        // p = r
        p.copy(r);

        // rho = r^T * r
        T rho = r.dot(r);
        stats.initialResidual = std::sqrt(static_cast<double>(rho));
        stats.residual = stats.initialResidual;

        if (verbose_) {
            std::cout << "CG Solver:\n";
            std::cout << "  Initial residual: " << std::scientific << stats.initialResidual << "\n";
        }

        // CG iteration
        for (int iter = 0; iter < maxIter_; ++iter) {
            // Check convergence
            T residualNorm = std::sqrt(rho);
            stats.residual = static_cast<double>(residualNorm);

            if (residualNorm < tolerance_ * stats.initialResidual || residualNorm < tolerance_) {
                stats.converged = true;
                stats.iterations = iter;
                break;
            }

            // Ap = A * p
            A_.spmv(p, Ap);

            // alpha = rho / (p^T * Ap)
            T pAp = p.dot(Ap);
            if (std::abs(pAp) < 1e-14) {
                if (verbose_) {
                    std::cout << "  WARNING: pAp near zero, stopping\n";
                }
                break;
            }
            T alpha = rho / pAp;

            // x = x + alpha * p
            x.axpy(alpha, p);

            // r = r - alpha * Ap
            r.axpy(-alpha, Ap);

            // rho_new = r^T * r
            T rho_new = r.dot(r);

            // beta = rho_new / rho
            T beta = rho_new / rho;

            // p = r + beta * p
            p.scale(beta);
            p.axpy(1.0, r);

            rho = rho_new;

            stats.iterations = iter + 1;

            if (verbose_ && (iter % 10 == 0 || iter < 5)) {
                std::cout << "  Iter " << std::setw(4) << iter
                         << ": residual = " << std::scientific << residualNorm << "\n";
            }
        }

        stopEvent.record();
        stopEvent.synchronize();
        stats.elapsedTime = Event::elapsedTime(startEvent, stopEvent);

        if (verbose_) {
            std::cout << "  Final iterations: " << stats.iterations << "\n";
            std::cout << "  Final residual: " << std::scientific << stats.residual << "\n";
            std::cout << "  Converged: " << (stats.converged ? "Yes" : "No") << "\n";
        }

        return stats;
    }

private:
    const GPUSparseMatrixCSR<T>& A_;
    int maxIter_;
    double tolerance_;
    bool verbose_;
};

/**
 * @class BiCGStab
 * @brief BiConjugate Gradient Stabilized solver for non-symmetric systems
 *
 * Phase 52: GPU Iterative Solvers
 *
 * Solves: A * x = b
 * where A can be non-symmetric.
 *
 * @tparam T Element type (float or double)
 */
template<typename T>
class BiCGStab {
public:
    /**
     * @brief Constructor
     * @param matrix Sparse matrix A
     * @param maxIter Maximum iterations (default: 1000)
     * @param tolerance Convergence tolerance (default: 1e-6)
     */
    explicit BiCGStab(const GPUSparseMatrixCSR<T>& matrix,
                     int maxIter = 1000,
                     double tolerance = 1e-6)
        : A_(matrix),
          maxIter_(maxIter),
          tolerance_(tolerance),
          verbose_(false) {}

    /**
     * @brief Set verbosity
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Set maximum iterations
     */
    void setMaxIterations(int maxIter) { maxIter_ = maxIter; }

    /**
     * @brief Set tolerance
     */
    void setTolerance(double tol) { tolerance_ = tol; }

    /**
     * @brief Solve: A * x = b
     * @param b Right-hand side
     * @param x Solution vector (initial guess)
     * @return Solver statistics
     */
    SolverStats solve(const GPUVector<T>& b, GPUVector<T>& x) {
        SolverStats stats;

        // Start timer
        Event startEvent, stopEvent;
        startEvent.record();

        int n = A_.numRows();

        if (x.size() != static_cast<size_t>(n)) {
            x.resize(n);
            x.zero();
        }

        // Allocate workspace vectors
        GPUVector<T> r(n);    // Residual
        GPUVector<T> r0(n);   // Initial residual
        GPUVector<T> p(n);    // Search direction
        GPUVector<T> v(n);    // A * p
        GPUVector<T> s(n);    // Intermediate residual
        GPUVector<T> t(n);    // A * s

        // r = b - A*x
        A_.spmv(x, r, -1.0, 0.0);
        r.axpy(1.0, b);

        // r0 = r (arbitrary choice)
        r0.copy(r);

        // p = r
        p.copy(r);

        T rho = 1.0;
        T alpha = 1.0;
        T omega = 1.0;

        stats.initialResidual = r.norm2();
        stats.residual = stats.initialResidual;

        if (verbose_) {
            std::cout << "BiCGStab Solver:\n";
            std::cout << "  Initial residual: " << std::scientific << stats.initialResidual << "\n";
        }

        // BiCGStab iteration
        for (int iter = 0; iter < maxIter_; ++iter) {
            // Check convergence
            T residualNorm = r.norm2();
            stats.residual = static_cast<double>(residualNorm);

            if (residualNorm < tolerance_ * stats.initialResidual || residualNorm < tolerance_) {
                stats.converged = true;
                stats.iterations = iter;
                break;
            }

            // rho_new = (r0, r)
            T rho_new = r0.dot(r);

            if (std::abs(rho_new) < 1e-14) {
                if (verbose_) {
                    std::cout << "  WARNING: rho near zero, stopping\n";
                }
                break;
            }

            // beta = (rho_new/rho) * (alpha/omega)
            T beta = (rho_new / rho) * (alpha / omega);

            // p = r + beta * (p - omega * v)
            p.axpy(-omega, v);   // p = p - omega*v
            p.scale(beta);        // p = beta*p
            p.axpy(1.0, r);       // p = r + p

            // v = A * p
            A_.spmv(p, v);

            // alpha = rho_new / (r0, v)
            T r0v = r0.dot(v);
            if (std::abs(r0v) < 1e-14) {
                if (verbose_) {
                    std::cout << "  WARNING: r0v near zero, stopping\n";
                }
                break;
            }
            alpha = rho_new / r0v;

            // s = r - alpha * v
            s.copy(r);
            s.axpy(-alpha, v);

            // Check early convergence
            T sNorm = s.norm2();
            if (sNorm < tolerance_) {
                x.axpy(alpha, p);
                stats.converged = true;
                stats.iterations = iter + 1;
                break;
            }

            // t = A * s
            A_.spmv(s, t);

            // omega = (t, s) / (t, t)
            T ts = t.dot(s);
            T tt = t.dot(t);
            if (std::abs(tt) < 1e-14) {
                if (verbose_) {
                    std::cout << "  WARNING: tt near zero, stopping\n";
                }
                break;
            }
            omega = ts / tt;

            // x = x + alpha * p + omega * s
            x.axpy(alpha, p);
            x.axpy(omega, s);

            // r = s - omega * t
            r.copy(s);
            r.axpy(-omega, t);

            rho = rho_new;

            stats.iterations = iter + 1;

            if (verbose_ && (iter % 10 == 0 || iter < 5)) {
                std::cout << "  Iter " << std::setw(4) << iter
                         << ": residual = " << std::scientific << residualNorm << "\n";
            }
        }

        stopEvent.record();
        stopEvent.synchronize();
        stats.elapsedTime = Event::elapsedTime(startEvent, stopEvent);

        if (verbose_) {
            std::cout << "  Final iterations: " << stats.iterations << "\n";
            std::cout << "  Final residual: " << std::scientific << stats.residual << "\n";
            std::cout << "  Converged: " << (stats.converged ? "Yes" : "No") << "\n";
        }

        return stats;
    }

private:
    const GPUSparseMatrixCSR<T>& A_;
    int maxIter_;
    double tolerance_;
    bool verbose_;
};

// Type aliases
template<typename T>
using CG = ConjugateGradient<T>;

using CGF = ConjugateGradient<float>;
using CGD = ConjugateGradient<double>;

using BiCGStabF = BiCGStab<float>;
using BiCGStabD = BiCGStab<double>;

} // namespace linalg
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_LINALG_SOLVERS_H
