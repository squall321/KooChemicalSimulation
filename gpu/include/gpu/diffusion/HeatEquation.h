/**
 * @file HeatEquation.h
 * @brief High-level GPU heat equation solver
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Provides a high-level interface for solving the heat equation:
 *   ∂T/∂t = α ∇²T
 *
 * where T is temperature and α is thermal diffusivity.
 *
 * Features:
 * - 1D, 2D, and 3D support
 * - Multiple time integration schemes
 * - Various boundary conditions
 * - Automatic method selection
 * - Easy-to-use API
 */

#ifndef KOO_GPU_DIFFUSION_HEAT_EQUATION_H
#define KOO_GPU_DIFFUSION_HEAT_EQUATION_H

#include "ExplicitSolver.h"
#include "ImplicitSolver.h"
#include <functional>

namespace koo {
namespace gpu {
namespace diffusion {

/**
 * @brief Solution method
 */
enum class SolutionMethod {
    AUTO,              // Automatic selection based on CFL
    EXPLICIT_EULER,    // Forward Euler (explicit)
    EXPLICIT_RK2,      // RK2 (explicit)
    EXPLICIT_RK4,      // RK4 (explicit)
    IMPLICIT_EULER,    // Backward Euler (implicit)
    CRANK_NICOLSON     // Crank-Nicolson (implicit)
};

/**
 * @class HeatEquation1D
 * @brief 1D heat equation solver with automatic method selection
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Example usage:
 * ```cpp
 * // Setup
 * HeatEquation1D<double> solver(nx, L, alpha);
 * solver.setInitialCondition(gaussianPulse);
 * solver.setBoundaryConditions(BoundaryType::NEUMANN, BoundaryType::NEUMANN);
 *
 * // Solve
 * auto stats = solver.solve(t_final, dt);
 *
 * // Get solution
 * auto T = solver.getSolution();
 * ```
 */
template<typename T>
class HeatEquation1D {
public:
    /**
     * @brief Constructor
     * @param nx Grid points
     * @param L Domain length
     * @param alpha Thermal diffusivity
     * @param method Solution method (AUTO = automatic selection)
     */
    HeatEquation1D(int nx, T L, T alpha,
                   SolutionMethod method = SolutionMethod::AUTO)
        : nx_(nx),
          L_(L),
          alpha_(alpha),
          dx_(L / (nx - 1)),
          method_(method),
          u_(nx),
          verbose_(false) {

        u_.zero();
    }

    /**
     * @brief Set initial condition from function
     * @param initialFunc Function: T(x) -> value
     */
    void setInitialCondition(std::function<T(T)> initialFunc) {
        std::vector<T> u_host(nx_);

        for (int i = 0; i < nx_; ++i) {
            T x = i * dx_;
            u_host[i] = initialFunc(x);
        }

        u_.copyFromHost(u_host.data(), nx_);
    }

    /**
     * @brief Set initial condition from array
     */
    void setInitialCondition(const std::vector<T>& values) {
        if (values.size() != static_cast<size_t>(nx_)) {
            throw DiffusionError("Initial condition size mismatch");
        }
        u_.copyFromHost(values.data(), nx_);
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
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Solve until final time
     * @param t_final Final time
     * @param dt Time step (optional, will be auto-selected if not provided)
     * @return Solver statistics
     */
    SolverStats solve(T t_final, T dt = 0) {
        // Auto-select time step if not provided
        if (dt <= 0) {
            dt = autoSelectTimeStep();
            if (verbose_) {
                std::cout << "Auto-selected dt = " << dt << std::endl;
            }
        }

        // Auto-select method if needed
        SolutionMethod actualMethod = method_;
        if (method_ == SolutionMethod::AUTO) {
            actualMethod = autoSelectMethod(dt);
            if (verbose_) {
                std::cout << "Auto-selected method: " << methodName(actualMethod) << std::endl;
            }
        }

        // Solve using selected method
        return solveWithMethod(actualMethod, t_final, dt);
    }

    /**
     * @brief Get current solution
     */
    std::vector<T> getSolution() const {
        return u_.toHost();
    }

    /**
     * @brief Get solution on device (for further GPU processing)
     */
    const DeviceMemory<T>& getSolutionDevice() const {
        return u_;
    }

    /**
     * @brief Get grid coordinates
     */
    std::vector<T> getGrid() const {
        std::vector<T> x(nx_);
        for (int i = 0; i < nx_; ++i) {
            x[i] = i * dx_;
        }
        return x;
    }

    /**
     * @brief Get number of grid points
     */
    int getGridSize() const { return nx_; }

    /**
     * @brief Get domain length
     */
    T getDomainLength() const { return L_; }

    /**
     * @brief Get grid spacing
     */
    T getGridSpacing() const { return dx_; }

    /**
     * @brief Get thermal diffusivity
     */
    T getThermalDiffusivity() const { return alpha_; }

private:
    /**
     * @brief Auto-select time step based on CFL
     */
    T autoSelectTimeStep() const {
        DiffusionKernels1D<T> kernels(nx_, dx_);
        T dt_max = kernels.getMaxDt(alpha_, 0.4);  // Conservative CFL = 0.4
        return dt_max;
    }

    /**
     * @brief Auto-select solution method based on CFL
     */
    SolutionMethod autoSelectMethod(T dt) const {
        DiffusionKernels1D<T> kernels(nx_, dx_);
        T cfl = kernels.getCFL(dt, alpha_);

        if (cfl < 0.3) {
            // Stable for explicit, use RK4 for accuracy
            return SolutionMethod::EXPLICIT_RK4;
        } else if (cfl < 0.5) {
            // Marginally stable, use RK2
            return SolutionMethod::EXPLICIT_RK2;
        } else {
            // Unstable for explicit, use implicit
            return SolutionMethod::CRANK_NICOLSON;
        }
    }

    /**
     * @brief Solve with specific method
     */
    SolverStats solveWithMethod(SolutionMethod method, T t_final, T dt) {
        switch (method) {
            case SolutionMethod::EXPLICIT_EULER: {
                ExplicitDiffusionSolver1D<T> solver(nx_, dx_, alpha_, TimeScheme::FORWARD_EULER);
                solver.setBoundaryConditions(bcLeft_, bcRight_, valueLeft_, valueRight_);
                solver.setVerbose(verbose_);
                return solver.solveUntil(u_, t_final, dt);
            }

            case SolutionMethod::EXPLICIT_RK2: {
                ExplicitDiffusionSolver1D<T> solver(nx_, dx_, alpha_, TimeScheme::RK2);
                solver.setBoundaryConditions(bcLeft_, bcRight_, valueLeft_, valueRight_);
                solver.setVerbose(verbose_);
                return solver.solveUntil(u_, t_final, dt);
            }

            case SolutionMethod::EXPLICIT_RK4: {
                ExplicitDiffusionSolver1D<T> solver(nx_, dx_, alpha_, TimeScheme::RK4);
                solver.setBoundaryConditions(bcLeft_, bcRight_, valueLeft_, valueRight_);
                solver.setVerbose(verbose_);
                return solver.solveUntil(u_, t_final, dt);
            }

            case SolutionMethod::IMPLICIT_EULER: {
                ImplicitDiffusionSolver1D<T> solver(nx_, dx_, alpha_, false);
                solver.setBoundaryConditions(bcLeft_, bcRight_, valueLeft_, valueRight_);
                solver.setVerbose(verbose_);
                return solver.solveUntil(u_, t_final, dt);
            }

            case SolutionMethod::CRANK_NICOLSON: {
                ImplicitDiffusionSolver1D<T> solver(nx_, dx_, alpha_, true);
                solver.setBoundaryConditions(bcLeft_, bcRight_, valueLeft_, valueRight_);
                solver.setVerbose(verbose_);
                return solver.solveUntil(u_, t_final, dt);
            }

            default:
                throw DiffusionError("Invalid solution method");
        }
    }

    /**
     * @brief Get method name as string
     */
    std::string methodName(SolutionMethod method) const {
        switch (method) {
            case SolutionMethod::EXPLICIT_EULER: return "Explicit Euler";
            case SolutionMethod::EXPLICIT_RK2: return "RK2";
            case SolutionMethod::EXPLICIT_RK4: return "RK4";
            case SolutionMethod::IMPLICIT_EULER: return "Implicit Euler";
            case SolutionMethod::CRANK_NICOLSON: return "Crank-Nicolson";
            default: return "Unknown";
        }
    }

private:
    int nx_;
    T L_;
    T alpha_;
    T dx_;
    SolutionMethod method_;

    DeviceMemory<T> u_;

    BoundaryType bcLeft_ = BoundaryType::NEUMANN;
    BoundaryType bcRight_ = BoundaryType::NEUMANN;
    T valueLeft_ = 0;
    T valueRight_ = 0;

    bool verbose_;
};

/**
 * @class HeatEquation2D
 * @brief 2D heat equation solver
 *
 * Solves: ∂T/∂t = α (∂²T/∂x² + ∂²T/∂y²)
 */
template<typename T>
class HeatEquation2D {
public:
    /**
     * @brief Constructor
     * @param nx Grid points in x
     * @param ny Grid points in y
     * @param Lx Domain length in x
     * @param Ly Domain length in y
     * @param alpha Thermal diffusivity
     * @param method Solution method
     */
    HeatEquation2D(int nx, int ny, T Lx, T Ly, T alpha,
                   SolutionMethod method = SolutionMethod::AUTO)
        : nx_(nx),
          ny_(ny),
          Lx_(Lx),
          Ly_(Ly),
          alpha_(alpha),
          dx_(Lx / (nx - 1)),
          dy_(Ly / (ny - 1)),
          method_(method),
          u_(nx * ny),
          verbose_(false) {

        u_.zero();
    }

    /**
     * @brief Set initial condition from function
     * @param initialFunc Function: T(x, y) -> value
     */
    void setInitialCondition(std::function<T(T, T)> initialFunc) {
        std::vector<T> u_host(nx_ * ny_);

        for (int j = 0; j < ny_; ++j) {
            for (int i = 0; i < nx_; ++i) {
                T x = i * dx_;
                T y = j * dy_;
                u_host[j * nx_ + i] = initialFunc(x, y);
            }
        }

        u_.copyFromHost(u_host.data(), nx_ * ny_);
    }

    /**
     * @brief Set initial condition from array (row-major)
     */
    void setInitialCondition(const std::vector<T>& values) {
        if (values.size() != static_cast<size_t>(nx_ * ny_)) {
            throw DiffusionError("Initial condition size mismatch");
        }
        u_.copyFromHost(values.data(), nx_ * ny_);
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Solve until final time
     */
    SolverStats solve(T t_final, T dt = 0) {
        if (dt <= 0) {
            dt = autoSelectTimeStep();
            if (verbose_) {
                std::cout << "Auto-selected dt = " << dt << std::endl;
            }
        }

        SolutionMethod actualMethod = method_;
        if (method_ == SolutionMethod::AUTO) {
            actualMethod = autoSelectMethod(dt);
            if (verbose_) {
                std::cout << "Auto-selected method: " << methodName(actualMethod) << std::endl;
            }
        }

        return solveWithMethod(actualMethod, t_final, dt);
    }

    /**
     * @brief Get solution
     */
    std::vector<T> getSolution() const {
        return u_.toHost();
    }

    /**
     * @brief Get solution on device
     */
    const DeviceMemory<T>& getSolutionDevice() const {
        return u_;
    }

    /**
     * @brief Get grid size
     */
    std::pair<int, int> getGridSize() const { return {nx_, ny_}; }

private:
    T autoSelectTimeStep() const {
        DiffusionKernels2D<T> kernels(nx_, ny_, dx_, dy_);
        return kernels.getMaxDt(alpha_, 0.4);
    }

    SolutionMethod autoSelectMethod(T dt) const {
        DiffusionKernels2D<T> kernels(nx_, ny_, dx_, dy_);
        T cfl = kernels.getCFL(dt, alpha_);

        if (cfl < 0.4) {
            return SolutionMethod::EXPLICIT_EULER;
        } else {
            return SolutionMethod::CRANK_NICOLSON;
        }
    }

    SolverStats solveWithMethod(SolutionMethod method, T t_final, T dt) {
        if (method == SolutionMethod::EXPLICIT_EULER ||
            method == SolutionMethod::EXPLICIT_RK2 ||
            method == SolutionMethod::EXPLICIT_RK4) {
            ExplicitDiffusionSolver2D<T> solver(nx_, ny_, dx_, dy_, alpha_);
            solver.setVerbose(verbose_);
            return solver.solveUntil(u_, t_final, dt);
        } else {
            ImplicitDiffusionSolver2D<T> solver(nx_, ny_, dx_, dy_, alpha_,
                                               method == SolutionMethod::CRANK_NICOLSON);
            solver.setVerbose(verbose_);
            return solver.solveUntil(u_, t_final, dt);
        }
    }

    std::string methodName(SolutionMethod method) const {
        switch (method) {
            case SolutionMethod::EXPLICIT_EULER: return "Explicit Euler";
            case SolutionMethod::CRANK_NICOLSON: return "Crank-Nicolson";
            default: return "Unknown";
        }
    }

private:
    int nx_, ny_;
    T Lx_, Ly_;
    T alpha_;
    T dx_, dy_;
    SolutionMethod method_;

    DeviceMemory<T> u_;
    bool verbose_;
};

// Type aliases
using HeatEquation1DF = HeatEquation1D<float>;
using HeatEquation1DD = HeatEquation1D<double>;
using HeatEquation2DF = HeatEquation2D<float>;
using HeatEquation2DD = HeatEquation2D<double>;

} // namespace diffusion
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_DIFFUSION_HEAT_EQUATION_H
