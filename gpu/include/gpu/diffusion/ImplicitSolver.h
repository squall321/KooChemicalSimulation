/**
 * @file ImplicitSolver.h
 * @brief Implicit time stepping for GPU diffusion solvers
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Provides implicit time integration methods for diffusion equations:
 * - Backward Euler (1st order, unconditionally stable)
 * - Crank-Nicolson (2nd order, unconditionally stable)
 *
 * Uses GPU sparse linear solvers from Phase 52.
 * No CFL restrictions - stable for any time step!
 */

#ifndef KOO_GPU_DIFFUSION_IMPLICIT_SOLVER_H
#define KOO_GPU_DIFFUSION_IMPLICIT_SOLVER_H

#include "DiffusionKernels.h"
#include "../linalg/Vector.h"
#include "../linalg/Matrix.h"
#include "../linalg/Solvers.h"
#include <vector>

namespace koo {
namespace gpu {
namespace diffusion {

/**
 * @class ImplicitDiffusionSolver1D
 * @brief Implicit solver for 1D diffusion equation
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Solves: du/dt = D * d²u/dx²
 *
 * Backward Euler: (I - dt*D*L) u^(n+1) = u^n
 * Crank-Nicolson: (I - dt/2*D*L) u^(n+1) = (I + dt/2*D*L) u^n
 *
 * where L is the discrete Laplacian operator.
 *
 * @tparam T Floating point type (float or double)
 */
template<typename T>
class ImplicitDiffusionSolver1D {
public:
    /**
     * @brief Constructor
     * @param nx Grid points
     * @param dx Grid spacing
     * @param D Diffusion coefficient
     * @param crankNicolson Use Crank-Nicolson (else Backward Euler)
     */
    ImplicitDiffusionSolver1D(int nx, T dx, T D, bool crankNicolson = false)
        : nx_(nx),
          dx_(dx),
          D_(D),
          crankNicolson_(crankNicolson),
          bcLeft_(BoundaryType::NEUMANN),
          bcRight_(BoundaryType::NEUMANN),
          valueLeft_(0),
          valueRight_(0),
          verbose_(false),
          systemMatrix_(nullptr),
          cgSolver_(nullptr),
          matrixBuilt_(false) {
    }

    /**
     * @brief Set boundary conditions
     */
    void setBoundaryConditions(BoundaryType left, BoundaryType right,
                               T valueLeft = 0, T valueRight = 0) {
        bcLeft_ = left;
        bcRight_ = right;
        valueLeft_ = valueLeft;
        valueRight_ = valueRight;
        matrixBuilt_ = false;  // Need to rebuild matrix
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Build system matrix for given time step
     *
     * Matrix: A = I - theta * dt * D * L
     * where theta = 1.0 (Backward Euler) or 0.5 (Crank-Nicolson)
     */
    void buildMatrix(T dt) {
        T theta = crankNicolson_ ? 0.5 : 1.0;
        T alpha = -theta * dt * D_ / (dx_ * dx_);

        // Build tridiagonal matrix in COO format
        std::vector<int> rows;
        std::vector<int> cols;
        std::vector<T> vals;

        for (int i = 0; i < nx_; ++i) {
            if (i == 0) {
                // Left boundary
                if (bcLeft_ == BoundaryType::DIRICHLET) {
                    rows.push_back(0);
                    cols.push_back(0);
                    vals.push_back(1.0);
                } else {
                    // Neumann: use ghost point
                    rows.push_back(0);
                    cols.push_back(0);
                    vals.push_back(1.0 - alpha);

                    rows.push_back(0);
                    cols.push_back(1);
                    vals.push_back(alpha);
                }
            } else if (i == nx_ - 1) {
                // Right boundary
                if (bcRight_ == BoundaryType::DIRICHLET) {
                    rows.push_back(nx_-1);
                    cols.push_back(nx_-1);
                    vals.push_back(1.0);
                } else {
                    // Neumann
                    rows.push_back(nx_-1);
                    cols.push_back(nx_-2);
                    vals.push_back(alpha);

                    rows.push_back(nx_-1);
                    cols.push_back(nx_-1);
                    vals.push_back(1.0 - alpha);
                }
            } else {
                // Interior points: tridiagonal
                rows.push_back(i);
                cols.push_back(i-1);
                vals.push_back(alpha);

                rows.push_back(i);
                cols.push_back(i);
                vals.push_back(1.0 - 2.0 * alpha);

                rows.push_back(i);
                cols.push_back(i+1);
                vals.push_back(alpha);
            }
        }

        // Create sparse matrix
        systemMatrix_ = std::make_unique<linalg::GPUSparseMatrixCSR<T>>(
            nx_, nx_, rows, cols, vals
        );

        // Create CG solver
        cgSolver_ = std::make_unique<linalg::ConjugateGradient<T>>(
            *systemMatrix_, 1000, 1e-10
        );
        cgSolver_->setVerbose(false);

        matrixBuilt_ = true;
        currentDt_ = dt;
    }

    /**
     * @brief Single time step
     * @param u Current field (in/out)
     * @param dt Time step
     */
    void step(DeviceMemory<T>& u, T dt) {
        // Rebuild matrix if dt changed
        if (!matrixBuilt_ || std::abs(dt - currentDt_) > 1e-10) {
            buildMatrix(dt);
        }

        // Convert to GPU vector
        linalg::GPUVector<T> uVec(u.toHost());
        linalg::GPUVector<T> rhs(nx_);

        if (crankNicolson_) {
            // Crank-Nicolson RHS: (I + dt/2 * D * L) * u
            // This requires matrix-vector product with explicit matrix
            // For simplicity, approximate with Forward Euler step
            DiffusionKernels1D<T> kernels(nx_, dx_);
            DeviceMemory<T> u_rhs(nx_);
            kernels.step(u, u_rhs, dt / 2.0, D_);
            rhs.fromHost(u_rhs.toHost());
        } else {
            // Backward Euler RHS: just u
            rhs.copy(uVec);
        }

        // Apply boundary conditions to RHS
        auto rhsHost = rhs.toHost();
        if (bcLeft_ == BoundaryType::DIRICHLET) {
            rhsHost[0] = valueLeft_;
        }
        if (bcRight_ == BoundaryType::DIRICHLET) {
            rhsHost[nx_-1] = valueRight_;
        }
        rhs.fromHost(rhsHost);

        // Solve: A * u_new = rhs
        linalg::GPUVector<T> uNew(nx_);
        auto stats = cgSolver_->solve(rhs, uNew);

        if (!stats.converged && verbose_) {
            std::cout << "Warning: CG did not converge in " << stats.iterations
                     << " iterations (residual: " << stats.residual << ")" << std::endl;
        }

        // Copy back
        u.copyFromHost(uNew.toHost().data(), nx_);
    }

    /**
     * @brief Solve for multiple time steps
     * @param u Initial/final field
     * @param dt Time step
     * @param n_steps Number of steps
     * @return Solver statistics
     */
    SolverStats solve(DeviceMemory<T>& u, T dt, int n_steps) {
        SolverStats stats;
        stats.steps = n_steps;
        stats.maxCFL = 0.0;  // No CFL restriction for implicit
        stats.stable = true;

        Event startEvent, endEvent;
        startEvent.record();

        for (int i = 0; i < n_steps; ++i) {
            step(u, dt);

            if (verbose_ && (i % 100 == 0)) {
                std::cout << "Step " << i << " / " << n_steps << std::endl;
            }
        }

        endEvent.record();
        endEvent.synchronize();

        stats.elapsedTime = Event::elapsedTime(startEvent, endEvent) / 1000.0;

        return stats;
    }

    /**
     * @brief Solve until final time
     */
    SolverStats solveUntil(DeviceMemory<T>& u, T t_final, T dt) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));
        return solve(u, dt, n_steps);
    }

private:
    int nx_;
    T dx_;
    T D_;
    bool crankNicolson_;

    // Boundary conditions
    BoundaryType bcLeft_, bcRight_;
    T valueLeft_, valueRight_;

    // Solver parameters
    bool verbose_;

    // Linear system
    std::unique_ptr<linalg::GPUSparseMatrixCSR<T>> systemMatrix_;
    std::unique_ptr<linalg::ConjugateGradient<T>> cgSolver_;
    bool matrixBuilt_;
    T currentDt_;
};

/**
 * @class ImplicitDiffusionSolver2D
 * @brief Implicit solver for 2D diffusion equation
 *
 * Solves: du/dt = D * (d²u/dx² + d²u/dy²)
 *
 * Uses 5-point stencil Laplacian.
 * System matrix is sparse (5 diagonals).
 */
template<typename T>
class ImplicitDiffusionSolver2D {
public:
    /**
     * @brief Constructor
     */
    ImplicitDiffusionSolver2D(int nx, int ny, T dx, T dy, T D,
                              bool crankNicolson = false)
        : nx_(nx),
          ny_(ny),
          dx_(dx),
          dy_(dy),
          D_(D),
          crankNicolson_(crankNicolson),
          verbose_(false),
          systemMatrix_(nullptr),
          cgSolver_(nullptr),
          matrixBuilt_(false) {
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Build system matrix
     *
     * 2D Laplacian: 5-point stencil
     * [-1]
     * [-1 4 -1] / h²
     * [-1]
     */
    void buildMatrix(T dt) {
        T theta = crankNicolson_ ? 0.5 : 1.0;
        T alpha_x = -theta * dt * D_ / (dx_ * dx_);
        T alpha_y = -theta * dt * D_ / (dy_ * dy_);
        T alpha_center = 1.0 + 2.0 * theta * dt * D_ * (1.0/(dx_*dx_) + 1.0/(dy_*dy_));

        int n = nx_ * ny_;
        std::vector<int> rows;
        std::vector<int> cols;
        std::vector<T> vals;

        for (int j = 0; j < ny_; ++j) {
            for (int i = 0; i < nx_; ++i) {
                int idx = j * nx_ + i;

                // Boundary points: identity
                if (i == 0 || i == nx_-1 || j == 0 || j == ny_-1) {
                    rows.push_back(idx);
                    cols.push_back(idx);
                    vals.push_back(1.0);
                } else {
                    // Interior: 5-point stencil
                    // Left
                    rows.push_back(idx);
                    cols.push_back(idx - 1);
                    vals.push_back(alpha_x);

                    // Right
                    rows.push_back(idx);
                    cols.push_back(idx + 1);
                    vals.push_back(alpha_x);

                    // Bottom
                    rows.push_back(idx);
                    cols.push_back(idx - nx_);
                    vals.push_back(alpha_y);

                    // Top
                    rows.push_back(idx);
                    cols.push_back(idx + nx_);
                    vals.push_back(alpha_y);

                    // Center
                    rows.push_back(idx);
                    cols.push_back(idx);
                    vals.push_back(alpha_center);
                }
            }
        }

        // Create sparse matrix
        systemMatrix_ = std::make_unique<linalg::GPUSparseMatrixCSR<T>>(
            n, n, rows, cols, vals
        );

        // Create CG solver
        cgSolver_ = std::make_unique<linalg::ConjugateGradient<T>>(
            *systemMatrix_, 1000, 1e-8
        );
        cgSolver_->setVerbose(false);

        matrixBuilt_ = true;
        currentDt_ = dt;
    }

    /**
     * @brief Single time step
     */
    void step(DeviceMemory<T>& u, T dt) {
        if (!matrixBuilt_ || std::abs(dt - currentDt_) > 1e-10) {
            buildMatrix(dt);
        }

        int n = nx_ * ny_;

        // Convert to GPU vector
        linalg::GPUVector<T> uVec(u.toHost());
        linalg::GPUVector<T> rhs(n);

        if (crankNicolson_) {
            // Simplified: use current state
            DiffusionKernels2D<T> kernels(nx_, ny_, dx_, dy_);
            DeviceMemory<T> u_rhs(n);
            kernels.step(u, u_rhs, dt / 2.0, D_);
            rhs.fromHost(u_rhs.toHost());
        } else {
            rhs.copy(uVec);
        }

        // Solve
        linalg::GPUVector<T> uNew(n);
        auto stats = cgSolver_->solve(rhs, uNew);

        if (!stats.converged && verbose_) {
            std::cout << "Warning: CG did not converge" << std::endl;
        }

        // Copy back
        u.copyFromHost(uNew.toHost().data(), n);
    }

    /**
     * @brief Solve for multiple steps
     */
    SolverStats solve(DeviceMemory<T>& u, T dt, int n_steps) {
        SolverStats stats;
        stats.steps = n_steps;
        stats.maxCFL = 0.0;
        stats.stable = true;

        Event startEvent, endEvent;
        startEvent.record();

        for (int i = 0; i < n_steps; ++i) {
            step(u, dt);

            if (verbose_ && (i % 100 == 0)) {
                std::cout << "Step " << i << " / " << n_steps << std::endl;
            }
        }

        endEvent.record();
        endEvent.synchronize();

        stats.elapsedTime = Event::elapsedTime(startEvent, endEvent) / 1000.0;

        return stats;
    }

    /**
     * @brief Solve until final time
     */
    SolverStats solveUntil(DeviceMemory<T>& u, T t_final, T dt) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));
        return solve(u, dt, n_steps);
    }

private:
    int nx_, ny_;
    T dx_, dy_;
    T D_;
    bool crankNicolson_;

    bool verbose_;

    std::unique_ptr<linalg::GPUSparseMatrixCSR<T>> systemMatrix_;
    std::unique_ptr<linalg::ConjugateGradient<T>> cgSolver_;
    bool matrixBuilt_;
    T currentDt_;
};

// Type aliases
using ImplicitDiffusionSolver1DF = ImplicitDiffusionSolver1D<float>;
using ImplicitDiffusionSolver1DD = ImplicitDiffusionSolver1D<double>;
using ImplicitDiffusionSolver2DF = ImplicitDiffusionSolver2D<float>;
using ImplicitDiffusionSolver2DD = ImplicitDiffusionSolver2D<double>;

} // namespace diffusion
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_DIFFUSION_IMPLICIT_SOLVER_H
