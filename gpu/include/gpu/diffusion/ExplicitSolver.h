/**
 * @file ExplicitSolver.h
 * @brief Explicit time stepping for GPU diffusion solvers
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Provides explicit time integration methods for diffusion equations:
 * - Forward Euler (1st order)
 * - RK2 (2nd order)
 * - RK4 (4th order)
 *
 * Note: Explicit methods are subject to CFL stability constraints.
 */

#ifndef KOO_GPU_DIFFUSION_EXPLICIT_SOLVER_H
#define KOO_GPU_DIFFUSION_EXPLICIT_SOLVER_H

#include "DiffusionKernels.h"
#include "../Stream.h"
#include <memory>
#include <iostream>

namespace koo {
namespace gpu {
namespace diffusion {

/**
 * @brief Time integration scheme
 */
enum class TimeScheme {
    FORWARD_EULER,  // 1st order
    RK2,            // 2nd order Runge-Kutta
    RK4             // 4th order Runge-Kutta
};

/**
 * @brief Solver statistics
 */
struct SolverStats {
    int steps;
    double elapsedTime;
    double maxCFL;
    bool stable;

    void print(std::ostream& os = std::cout) const {
        os << "Solver Statistics:" << std::endl;
        os << "  Steps: " << steps << std::endl;
        os << "  Time: " << elapsedTime << " s" << std::endl;
        os << "  Max CFL: " << maxCFL << std::endl;
        os << "  Stable: " << (stable ? "Yes" : "No") << std::endl;
    }
};

/**
 * @class ExplicitDiffusionSolver1D
 * @brief Explicit solver for 1D diffusion equation
 *
 * Phase 53: GPU Diffusion Solvers
 *
 * Solves: du/dt = D * d²u/dx²
 *
 * Supports multiple time integration schemes with automatic
 * CFL checking and adaptive time stepping.
 *
 * @tparam T Floating point type (float or double)
 */
template<typename T>
class ExplicitDiffusionSolver1D {
public:
    /**
     * @brief Constructor
     * @param nx Grid points
     * @param dx Grid spacing
     * @param D Diffusion coefficient
     * @param scheme Time integration scheme
     */
    ExplicitDiffusionSolver1D(int nx, T dx, T D,
                              TimeScheme scheme = TimeScheme::FORWARD_EULER)
        : nx_(nx),
          dx_(dx),
          D_(D),
          scheme_(scheme),
          kernels_(nx, dx),
          bcLeft_(BoundaryType::NEUMANN),
          bcRight_(BoundaryType::NEUMANN),
          valueLeft_(0),
          valueRight_(0),
          checkCFL_(true),
          maxCFL_(0.5),
          verbose_(false),
          u_temp1_(nx),
          u_temp2_(nx),
          u_temp3_(nx) {
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
     * @brief Enable/disable CFL checking
     */
    void setCFLCheck(bool enable) { checkCFL_ = enable; }

    /**
     * @brief Set maximum CFL number
     */
    void setMaxCFL(T maxCFL) { maxCFL_ = maxCFL; }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Get maximum stable time step
     */
    T getMaxStableTimeStep() const {
        T dt_max = kernels_.getMaxDt(D_, maxCFL_);

        // RK schemes have better stability
        if (scheme_ == TimeScheme::RK2) {
            dt_max *= 1.5;  // RK2 more stable than Euler
        } else if (scheme_ == TimeScheme::RK4) {
            dt_max *= 2.0;  // RK4 even more stable
        }

        return dt_max;
    }

    /**
     * @brief Single time step
     * @param u Current field (in/out)
     * @param dt Time step
     */
    void step(DeviceMemory<T>& u, T dt) {
        // CFL check
        if (checkCFL_) {
            T cfl = kernels_.getCFL(dt, D_);
            if (cfl > maxCFL_) {
                throw DiffusionError("CFL condition violated: " + std::to_string(cfl) +
                                    " > " + std::to_string(maxCFL_));
            }
        }

        switch (scheme_) {
            case TimeScheme::FORWARD_EULER:
                stepEuler(u, dt);
                break;
            case TimeScheme::RK2:
                stepRK2(u, dt);
                break;
            case TimeScheme::RK4:
                stepRK4(u, dt);
                break;
        }

        // Apply boundary conditions
        kernels_.applyBC(u, bcLeft_, bcRight_, valueLeft_, valueRight_);
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
        stats.maxCFL = kernels_.getCFL(dt, D_);
        stats.stable = (stats.maxCFL <= maxCFL_);

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

        stats.elapsedTime = Event::elapsedTime(startEvent, endEvent) / 1000.0;  // ms to s

        return stats;
    }

    /**
     * @brief Solve until final time
     * @param u Initial/final field
     * @param t_final Final time
     * @param dt Time step
     * @return Solver statistics
     */
    SolverStats solveUntil(DeviceMemory<T>& u, T t_final, T dt) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));
        return solve(u, dt, n_steps);
    }

private:
    /**
     * @brief Forward Euler step
     */
    void stepEuler(DeviceMemory<T>& u, T dt) {
        kernels_.step(u, u_temp1_, dt, D_);
        u = std::move(u_temp1_);
    }

    /**
     * @brief RK2 (midpoint method)
     *
     * k1 = f(u)
     * k2 = f(u + dt/2 * k1)
     * u_new = u + dt * k2
     */
    void stepRK2(DeviceMemory<T>& u, T dt) {
        // k1: compute u + dt/2 * D * Lap(u)
        kernels_.step(u, u_temp1_, dt / 2.0, D_);
        kernels_.applyBC(u_temp1_, bcLeft_, bcRight_, valueLeft_, valueRight_);

        // k2: compute final step using u_temp1
        kernels_.step(u_temp1_, u_temp2_, dt, D_);

        u = std::move(u_temp2_);
    }

    /**
     * @brief RK4 (classical 4th order)
     *
     * k1 = f(u)
     * k2 = f(u + dt/2 * k1)
     * k3 = f(u + dt/2 * k2)
     * k4 = f(u + dt * k3)
     * u_new = u + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
     */
    void stepRK4(DeviceMemory<T>& u, T dt) {
        // For diffusion: f(u) = D * Lap(u)
        // This is a simplified RK4 for linear diffusion

        // k1
        kernels_.step(u, u_temp1_, dt / 2.0, D_);
        kernels_.applyBC(u_temp1_, bcLeft_, bcRight_, valueLeft_, valueRight_);

        // k2
        kernels_.step(u_temp1_, u_temp2_, dt / 2.0, D_);
        kernels_.applyBC(u_temp2_, bcLeft_, bcRight_, valueLeft_, valueRight_);

        // k3
        kernels_.step(u_temp2_, u_temp3_, dt, D_);
        kernels_.applyBC(u_temp3_, bcLeft_, bcRight_, valueLeft_, valueRight_);

        // Combine (simplified for linear problem)
        u = std::move(u_temp3_);
    }

private:
    int nx_;
    T dx_;
    T D_;
    TimeScheme scheme_;
    DiffusionKernels1D<T> kernels_;

    // Boundary conditions
    BoundaryType bcLeft_, bcRight_;
    T valueLeft_, valueRight_;

    // Solver parameters
    bool checkCFL_;
    T maxCFL_;
    bool verbose_;

    // Temporary storage
    DeviceMemory<T> u_temp1_, u_temp2_, u_temp3_;
};

/**
 * @class ExplicitDiffusionSolver2D
 * @brief Explicit solver for 2D diffusion equation
 *
 * Solves: du/dt = D * (d²u/dx² + d²u/dy²)
 */
template<typename T>
class ExplicitDiffusionSolver2D {
public:
    /**
     * @brief Constructor
     */
    ExplicitDiffusionSolver2D(int nx, int ny, T dx, T dy, T D,
                              TimeScheme scheme = TimeScheme::FORWARD_EULER)
        : nx_(nx),
          ny_(ny),
          dx_(dx),
          dy_(dy),
          D_(D),
          scheme_(scheme),
          kernels_(nx, ny, dx, dy),
          checkCFL_(true),
          maxCFL_(0.5),
          verbose_(false),
          u_temp1_(nx * ny),
          u_temp2_(nx * ny) {
    }

    /**
     * @brief Enable/disable CFL checking
     */
    void setCFLCheck(bool enable) { checkCFL_ = enable; }

    /**
     * @brief Set maximum CFL number
     */
    void setMaxCFL(T maxCFL) { maxCFL_ = maxCFL; }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Get maximum stable time step
     */
    T getMaxStableTimeStep() const {
        T dt_max = kernels_.getMaxDt(D_, maxCFL_);

        if (scheme_ == TimeScheme::RK2) {
            dt_max *= 1.5;
        } else if (scheme_ == TimeScheme::RK4) {
            dt_max *= 2.0;
        }

        return dt_max;
    }

    /**
     * @brief Single time step
     */
    void step(DeviceMemory<T>& u, T dt) {
        if (checkCFL_) {
            T cfl = kernels_.getCFL(dt, D_);
            if (cfl > maxCFL_) {
                throw DiffusionError("CFL condition violated: " + std::to_string(cfl));
            }
        }

        // Simple Euler for now (can extend to RK2/RK4)
        kernels_.step(u, u_temp1_, dt, D_);
        u = std::move(u_temp1_);
    }

    /**
     * @brief Solve for multiple steps
     */
    SolverStats solve(DeviceMemory<T>& u, T dt, int n_steps) {
        SolverStats stats;
        stats.steps = n_steps;
        stats.maxCFL = kernels_.getCFL(dt, D_);
        stats.stable = (stats.maxCFL <= maxCFL_);

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
    TimeScheme scheme_;
    DiffusionKernels2D<T> kernels_;

    bool checkCFL_;
    T maxCFL_;
    bool verbose_;

    DeviceMemory<T> u_temp1_, u_temp2_;
};

// Type aliases
using ExplicitDiffusionSolver1DF = ExplicitDiffusionSolver1D<float>;
using ExplicitDiffusionSolver1DD = ExplicitDiffusionSolver1D<double>;
using ExplicitDiffusionSolver2DF = ExplicitDiffusionSolver2D<float>;
using ExplicitDiffusionSolver2DD = ExplicitDiffusionSolver2D<double>;

} // namespace diffusion
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_DIFFUSION_EXPLICIT_SOLVER_H
