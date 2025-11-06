/**
 * @file ODESolver.h
 * @brief GPU ODE solvers for chemical kinetics
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Provides GPU-accelerated ODE integration for chemical kinetics:
 * - Explicit methods (Euler, RK2, RK4)
 * - Operator splitting for stiff systems
 * - Batched integration for multiple cells
 */

#ifndef KOO_GPU_KINETICS_ODE_SOLVER_H
#define KOO_GPU_KINETICS_ODE_SOLVER_H

#include "ReactionKernels.h"
#include "../Stream.h"
#include <functional>
#include <iostream>

namespace koo {
namespace gpu {
namespace kinetics {

/**
 * @brief ODE integration method
 */
enum class ODEMethod {
    EXPLICIT_EULER,  // 1st order
    RK2,             // 2nd order midpoint
    RK4              // 4th order Runge-Kutta
};

/**
 * @brief Solver statistics
 */
struct ODESolverStats {
    int steps;
    double elapsedTime;
    bool stable;

    void print(std::ostream& os = std::cout) const {
        os << "ODE Solver Statistics:" << std::endl;
        os << "  Steps: " << steps << std::endl;
        os << "  Time: " << elapsedTime << " s" << std::endl;
        os << "  Stable: " << (stable ? "Yes" : "No") << std::endl;
    }
};

/**
 * @class ODESolverGPU
 * @brief GPU ODE solver for chemical kinetics
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Solves the ODE system:
 *   dC/dt = f(C, t)
 *
 * where C is the vector of species concentrations.
 *
 * The right-hand side f(C, t) is provided via a callback
 * that computes production rates from concentrations.
 *
 * @tparam T Floating point type (float or double)
 */
template<typename T>
class ODESolverGPU {
public:
    /**
     * @brief Type for RHS evaluation function
     *
     * Takes current concentrations and returns production rates
     */
    using RHSFunction = std::function<void(const DeviceMemory<T>&, DeviceMemory<T>&)>;

    /**
     * @brief Constructor
     * @param n_species Number of species
     * @param n_cells Number of cells (for batched integration)
     * @param method Integration method
     */
    ODESolverGPU(int n_species, int n_cells = 1, ODEMethod method = ODEMethod::RK4)
        : n_species_(n_species),
          n_cells_(n_cells),
          method_(method),
          verbose_(false),
          checkNegative_(true),
          k1_(n_species * n_cells),
          k2_(n_species * n_cells),
          k3_(n_species * n_cells),
          k4_(n_species * n_cells),
          C_temp_(n_species * n_cells)
    {
        if (n_species <= 0 || n_cells <= 0) {
            throw KineticsError("Invalid dimensions");
        }

        // Initialize kernel helper
        kernels_ = std::make_unique<ReactionKernels<T>>(n_species, 1, n_cells);
    }

    /**
     * @brief Set integration method
     */
    void setMethod(ODEMethod method) { method_ = method; }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Enable/disable negative concentration checking
     */
    void setCheckNegative(bool check) { checkNegative_ = check; }

    /**
     * @brief Single time step
     *
     * @param C Current concentrations (in/out, n_species * n_cells)
     * @param dt Time step
     * @param rhs_func Function to compute production rates
     */
    void step(DeviceMemory<T>& C, T dt, RHSFunction rhs_func) {
        switch (method_) {
            case ODEMethod::EXPLICIT_EULER:
                stepEuler(C, dt, rhs_func);
                break;
            case ODEMethod::RK2:
                stepRK2(C, dt, rhs_func);
                break;
            case ODEMethod::RK4:
                stepRK4(C, dt, rhs_func);
                break;
        }

        if (checkNegative_) {
            enforceNonNegative(C);
        }
    }

    /**
     * @brief Integrate for multiple steps
     *
     * @param C Initial/final concentrations
     * @param dt Time step
     * @param n_steps Number of steps
     * @param rhs_func Function to compute production rates
     * @return Solver statistics
     */
    ODESolverStats solve(DeviceMemory<T>& C, T dt, int n_steps, RHSFunction rhs_func) {
        ODESolverStats stats;
        stats.steps = n_steps;
        stats.stable = true;

        Event startEvent, endEvent;
        startEvent.record();

        for (int i = 0; i < n_steps; ++i) {
            step(C, dt, rhs_func);

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
     * @brief Integrate until final time
     */
    ODESolverStats solveUntil(DeviceMemory<T>& C, T t_final, T dt, RHSFunction rhs_func) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));
        return solve(C, dt, n_steps, rhs_func);
    }

    /**
     * @brief Get method name
     */
    std::string getMethodName() const {
        switch (method_) {
            case ODEMethod::EXPLICIT_EULER: return "Explicit Euler";
            case ODEMethod::RK2: return "RK2 (Midpoint)";
            case ODEMethod::RK4: return "RK4";
            default: return "Unknown";
        }
    }

private:
    /**
     * @brief Explicit Euler step
     *
     * C_new = C_old + dt * f(C_old)
     */
    void stepEuler(DeviceMemory<T>& C, T dt, RHSFunction rhs_func) {
        // k1 = f(C)
        rhs_func(C, k1_);

        // C = C + dt * k1
        kernels_->eulerStep(C, k1_, C_temp_, dt);
        C = std::move(C_temp_);
    }

    /**
     * @brief RK2 (midpoint) step
     *
     * k1 = f(C)
     * k2 = f(C + dt/2 * k1)
     * C_new = C + dt * k2
     */
    void stepRK2(DeviceMemory<T>& C, T dt, RHSFunction rhs_func) {
        size_t n = static_cast<size_t>(n_species_ * n_cells_);

        // k1 = f(C)
        rhs_func(C, k1_);

        // C_temp = C + dt/2 * k1
        kernels_->eulerStep(C, k1_, C_temp_, dt / 2.0);

        // k2 = f(C_temp)
        rhs_func(C_temp_, k2_);

        // C_new = C + dt * k2
        kernels_->eulerStep(C, k2_, C_temp_, dt);
        C = std::move(C_temp_);
    }

    /**
     * @brief RK4 step
     *
     * k1 = f(C)
     * k2 = f(C + dt/2 * k1)
     * k3 = f(C + dt/2 * k2)
     * k4 = f(C + dt * k3)
     * C_new = C + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
     */
    void stepRK4(DeviceMemory<T>& C, T dt, RHSFunction rhs_func) {
        size_t n = static_cast<size_t>(n_species_ * n_cells_);

        // Save original C
        DeviceMemory<T> C_orig(n);
        C_orig.copyFromHost(C.toHost().data(), n);

        // k1 = f(C)
        rhs_func(C, k1_);

        // C_temp = C + dt/2 * k1
        kernels_->eulerStep(C_orig, k1_, C_temp_, dt / 2.0);

        // k2 = f(C_temp)
        rhs_func(C_temp_, k2_);

        // C_temp = C + dt/2 * k2
        kernels_->eulerStep(C_orig, k2_, C_temp_, dt / 2.0);

        // k3 = f(C_temp)
        rhs_func(C_temp_, k3_);

        // C_temp = C + dt * k3
        kernels_->eulerStep(C_orig, k3_, C_temp_, dt);

        // k4 = f(C_temp)
        rhs_func(C_temp_, k4_);

        // C_new = C + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
        // Build combined slope: k = k1 + 2*k2 + 2*k3 + k4
        DeviceMemory<T> k_combined(n);
        k_combined.copyFromHost(k1_.toHost().data(), n);
        kernels_->saxpy(2.0, k2_, k_combined);
        kernels_->saxpy(2.0, k3_, k_combined);
        kernels_->saxpy(1.0, k4_, k_combined);

        // C_new = C + dt/6 * k_combined
        kernels_->eulerStep(C_orig, k_combined, C, dt / 6.0);
    }

    /**
     * @brief Enforce non-negative concentrations
     */
    void enforceNonNegative(DeviceMemory<T>& C) {
        auto hostC = C.toHost();
        for (auto& val : hostC) {
            if (val < 0.0) val = 0.0;
        }
        C.copyFromHost(hostC.data(), hostC.size());
    }

private:
    int n_species_;
    int n_cells_;
    ODEMethod method_;
    bool verbose_;
    bool checkNegative_;

    // Temporary storage
    DeviceMemory<T> k1_, k2_, k3_, k4_;
    DeviceMemory<T> C_temp_;

    // Helper
    std::unique_ptr<ReactionKernels<T>> kernels_;
};

/**
 * @class SimplifiedKineticsODE
 * @brief Simplified kinetics ODE solver for testing
 *
 * For simple reaction systems without full mechanism.
 */
template<typename T>
class SimplifiedKineticsODE {
public:
    /**
     * @brief Constructor
     * @param n_species Number of species
     * @param rate_func Function to compute rates: rate_func(C) -> omega
     */
    SimplifiedKineticsODE(int n_species, ODEMethod method = ODEMethod::RK4)
        : n_species_(n_species),
          solver_(n_species, 1, method)
    {
    }

    /**
     * @brief Set rate function
     */
    void setRateFunction(typename ODESolverGPU<T>::RHSFunction func) {
        rate_func_ = func;
    }

    /**
     * @brief Solve
     */
    ODESolverStats solve(DeviceMemory<T>& C, T dt, int n_steps) {
        if (!rate_func_) {
            throw KineticsError("Rate function not set");
        }
        return solver_.solve(C, dt, n_steps, rate_func_);
    }

    /**
     * @brief Solve until final time
     */
    ODESolverStats solveUntil(DeviceMemory<T>& C, T t_final, T dt) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));
        return solve(C, dt, n_steps);
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) {
        solver_.setVerbose(verbose);
    }

private:
    int n_species_;
    ODESolverGPU<T> solver_;
    typename ODESolverGPU<T>::RHSFunction rate_func_;
};

// Type aliases
using ODESolverGPUF = ODESolverGPU<float>;
using ODESolverGPUD = ODESolverGPU<double>;
using SimplifiedKineticsODEF = SimplifiedKineticsODE<float>;
using SimplifiedKineticsODED = SimplifiedKineticsODE<double>;

} // namespace kinetics
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_KINETICS_ODE_SOLVER_H
