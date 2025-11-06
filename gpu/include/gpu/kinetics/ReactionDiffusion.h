/**
 * @file ReactionDiffusion.h
 * @brief Coupled reaction-diffusion solver on GPU
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Couples reaction kinetics (Phase 54) with diffusion (Phase 53)
 * for reactive transport simulations.
 *
 * Solves: ∂C_i/∂t = D_i ∇²C_i + ω_i(C)
 *
 * Uses operator splitting:
 * 1. Reaction step: ∂C/∂t = ω(C)
 * 2. Diffusion step: ∂C/∂t = D ∇²C
 */

#ifndef KOO_GPU_KINETICS_REACTION_DIFFUSION_H
#define KOO_GPU_KINETICS_REACTION_DIFFUSION_H

#include "ODESolver.h"
#include "../diffusion/ExplicitSolver.h"
#include "../diffusion/ImplicitSolver.h"
#include <vector>
#include <memory>

namespace koo {
namespace gpu {
namespace kinetics {

/**
 * @brief Operator splitting scheme
 */
enum class SplittingScheme {
    GODUNOV,    // First order: R(dt) -> D(dt)
    STRANG      // Second order: D(dt/2) -> R(dt) -> D(dt/2)
};

/**
 * @class ReactionDiffusion1D
 * @brief 1D Reaction-Diffusion solver on GPU
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Solves coupled reaction-diffusion for multiple species:
 *   ∂C_i/∂t = D_i ∂²C_i/∂x² + ω_i(C1, C2, ..., Cn)
 *
 * Features:
 * - Operator splitting (Godunov or Strang)
 * - Multiple species with different diffusion coefficients
 * - GPU-accelerated reaction and diffusion
 *
 * @tparam T Floating point type
 */
template<typename T>
class ReactionDiffusion1D {
public:
    /**
     * @brief Constructor
     * @param n_species Number of chemical species
     * @param nx Grid points
     * @param dx Grid spacing
     */
    ReactionDiffusion1D(int n_species, int nx, T dx,
                        SplittingScheme splitting = SplittingScheme::STRANG)
        : n_species_(n_species),
          nx_(nx),
          dx_(dx),
          splitting_(splitting),
          concentrations_(n_species * nx),
          diffusion_coeffs_(n_species),
          verbose_(false)
    {
        concentrations_.zero();

        // Default diffusion coefficients
        std::vector<T> D_default(n_species, 1.0e-9);
        diffusion_coeffs_.copyFromHost(D_default.data(), n_species);
    }

    /**
     * @brief Set diffusion coefficients for all species
     */
    void setDiffusionCoefficients(const std::vector<T>& D) {
        if (D.size() != static_cast<size_t>(n_species_)) {
            throw KineticsError("Diffusion coefficient size mismatch");
        }
        diffusion_coeffs_.copyFromHost(D.data(), D.size());
    }

    /**
     * @brief Set diffusion coefficient for one species
     */
    void setDiffusionCoefficient(int species_idx, T D) {
        auto hostD = diffusion_coeffs_.toHost();
        hostD[species_idx] = D;
        diffusion_coeffs_.copyFromHost(hostD.data(), hostD.size());
    }

    /**
     * @brief Set concentrations at a grid point
     */
    void setConcentrations(int grid_idx, const std::vector<T>& C) {
        if (C.size() != static_cast<size_t>(n_species_)) {
            throw KineticsError("Concentration vector size mismatch");
        }

        auto hostC = concentrations_.toHost();
        for (int s = 0; s < n_species_; ++s) {
            hostC[s * nx_ + grid_idx] = C[s];
        }
        concentrations_.copyFromHost(hostC.data(), hostC.size());
    }

    /**
     * @brief Get concentrations at a grid point
     */
    std::vector<T> getConcentrations(int grid_idx) const {
        auto hostC = concentrations_.toHost();
        std::vector<T> result(n_species_);
        for (int s = 0; s < n_species_; ++s) {
            result[s] = hostC[s * nx_ + grid_idx];
        }
        return result;
    }

    /**
     * @brief Set reaction rate function
     *
     * Function takes concentrations and returns production rates
     */
    void setReactionRates(typename ODESolverGPU<T>::RHSFunction func) {
        reaction_func_ = func;
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Single time step with operator splitting
     */
    void step(T dt) {
        if (!reaction_func_) {
            throw KineticsError("Reaction function not set");
        }

        switch (splitting_) {
            case SplittingScheme::GODUNOV:
                stepGodunov(dt);
                break;
            case SplittingScheme::STRANG:
                stepStrang(dt);
                break;
        }
    }

    /**
     * @brief Integrate for multiple steps
     */
    void solve(T t_final, T dt) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));

        for (int i = 0; i < n_steps; ++i) {
            step(dt);

            if (verbose_ && (i % 100 == 0)) {
                std::cout << "Step " << i << " / " << n_steps << std::endl;
            }
        }
    }

    /**
     * @brief Get all concentrations (for visualization)
     */
    std::vector<std::vector<T>> getAllConcentrations() const {
        auto hostC = concentrations_.toHost();
        std::vector<std::vector<T>> result(n_species_, std::vector<T>(nx_));

        for (int s = 0; s < n_species_; ++s) {
            for (int x = 0; x < nx_; ++x) {
                result[s][x] = hostC[s * nx_ + x];
            }
        }

        return result;
    }

    // Getters
    int getSpeciesCount() const { return n_species_; }
    int getGridSize() const { return nx_; }
    T getGridSpacing() const { return dx_; }

private:
    /**
     * @brief Godunov splitting: R(dt) -> D(dt)
     */
    void stepGodunov(T dt) {
        // Step 1: Reaction
        stepReaction(dt);

        // Step 2: Diffusion
        stepDiffusion(dt);
    }

    /**
     * @brief Strang splitting: D(dt/2) -> R(dt) -> D(dt/2)
     */
    void stepStrang(T dt) {
        // Step 1: Half diffusion step
        stepDiffusion(dt / 2.0);

        // Step 2: Full reaction step
        stepReaction(dt);

        // Step 3: Half diffusion step
        stepDiffusion(dt / 2.0);
    }

    /**
     * @brief Reaction step
     */
    void stepReaction(T dt) {
        // Create ODE solver
        ODESolverGPU<T> ode_solver(n_species_, nx_, ODEMethod::RK4);
        ode_solver.setCheckNegative(true);

        // Take a step
        ode_solver.step(concentrations_, dt, reaction_func_);
    }

    /**
     * @brief Diffusion step for all species
     */
    void stepDiffusion(T dt) {
        auto hostD = diffusion_coeffs_.toHost();

        // Apply diffusion to each species
        for (int s = 0; s < n_species_; ++s) {
            // Extract this species' concentration
            DeviceMemory<T> C_species(nx_);
            auto hostC = concentrations_.toHost();
            std::vector<T> C_s(nx_);
            for (int x = 0; x < nx_; ++x) {
                C_s[x] = hostC[s * nx_ + x];
            }
            C_species.copyFromHost(C_s.data(), nx_);

            // Apply diffusion
            using namespace diffusion;
            ExplicitDiffusionSolver1D<T> diff_solver(
                nx_, dx_, hostD[s], TimeScheme::FORWARD_EULER
            );
            diff_solver.setBoundaryConditions(
                BoundaryType::NEUMANN, BoundaryType::NEUMANN
            );
            diff_solver.setMaxCFL(0.4);
            diff_solver.step(C_species, dt);

            // Put back
            auto C_s_new = C_species.toHost();
            for (int x = 0; x < nx_; ++x) {
                hostC[s * nx_ + x] = C_s_new[x];
            }
            concentrations_.copyFromHost(hostC.data(), hostC.size());
        }
    }

private:
    int n_species_;
    int nx_;
    T dx_;
    SplittingScheme splitting_;

    DeviceMemory<T> concentrations_;     // n_species * nx
    DeviceMemory<T> diffusion_coeffs_;   // n_species

    typename ODESolverGPU<T>::RHSFunction reaction_func_;

    bool verbose_;
};

/**
 * @class ReactionDiffusion2D
 * @brief 2D Reaction-Diffusion solver on GPU
 *
 * Similar to 1D but for 2D domains
 */
template<typename T>
class ReactionDiffusion2D {
public:
    /**
     * @brief Constructor
     */
    ReactionDiffusion2D(int n_species, int nx, int ny, T dx, T dy,
                        SplittingScheme splitting = SplittingScheme::STRANG)
        : n_species_(n_species),
          nx_(nx),
          ny_(ny),
          dx_(dx),
          dy_(dy),
          splitting_(splitting),
          concentrations_(n_species * nx * ny),
          diffusion_coeffs_(n_species),
          verbose_(false)
    {
        concentrations_.zero();

        std::vector<T> D_default(n_species, 1.0e-9);
        diffusion_coeffs_.copyFromHost(D_default.data(), n_species);
    }

    /**
     * @brief Set diffusion coefficients
     */
    void setDiffusionCoefficients(const std::vector<T>& D) {
        if (D.size() != static_cast<size_t>(n_species_)) {
            throw KineticsError("Diffusion coefficient size mismatch");
        }
        diffusion_coeffs_.copyFromHost(D.data(), D.size());
    }

    /**
     * @brief Set reaction rate function
     */
    void setReactionRates(typename ODESolverGPU<T>::RHSFunction func) {
        reaction_func_ = func;
    }

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Single time step
     */
    void step(T dt) {
        if (!reaction_func_) {
            throw KineticsError("Reaction function not set");
        }

        switch (splitting_) {
            case SplittingScheme::GODUNOV:
                stepGodunov(dt);
                break;
            case SplittingScheme::STRANG:
                stepStrang(dt);
                break;
        }
    }

    /**
     * @brief Integrate
     */
    void solve(T t_final, T dt) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));

        for (int i = 0; i < n_steps; ++i) {
            step(dt);

            if (verbose_ && (i % 100 == 0)) {
                std::cout << "Step " << i << " / " << n_steps << std::endl;
            }
        }
    }

private:
    void stepGodunov(T dt) {
        stepReaction(dt);
        stepDiffusion(dt);
    }

    void stepStrang(T dt) {
        stepDiffusion(dt / 2.0);
        stepReaction(dt);
        stepDiffusion(dt / 2.0);
    }

    void stepReaction(T dt) {
        ODESolverGPU<T> ode_solver(n_species_, nx_ * ny_, ODEMethod::RK4);
        ode_solver.step(concentrations_, dt, reaction_func_);
    }

    void stepDiffusion(T dt) {
        auto hostD = diffusion_coeffs_.toHost();

        for (int s = 0; s < n_species_; ++s) {
            // Extract species concentration field
            DeviceMemory<T> C_species(nx_ * ny_);
            auto hostC = concentrations_.toHost();
            std::vector<T> C_s(nx_ * ny_);
            for (int i = 0; i < nx_ * ny_; ++i) {
                C_s[i] = hostC[s * nx_ * ny_ + i];
            }
            C_species.copyFromHost(C_s.data(), nx_ * ny_);

            // Apply diffusion
            using namespace diffusion;
            ExplicitDiffusionSolver2D<T> diff_solver(
                nx_, ny_, dx_, dy_, hostD[s], TimeScheme::FORWARD_EULER
            );
            diff_solver.setMaxCFL(0.3);
            diff_solver.step(C_species, dt);

            // Put back
            auto C_s_new = C_species.toHost();
            for (int i = 0; i < nx_ * ny_; ++i) {
                hostC[s * nx_ * ny_ + i] = C_s_new[i];
            }
            concentrations_.copyFromHost(hostC.data(), hostC.size());
        }
    }

private:
    int n_species_;
    int nx_, ny_;
    T dx_, dy_;
    SplittingScheme splitting_;

    DeviceMemory<T> concentrations_;
    DeviceMemory<T> diffusion_coeffs_;

    typename ODESolverGPU<T>::RHSFunction reaction_func_;

    bool verbose_;
};

// Type aliases
using ReactionDiffusion1DF = ReactionDiffusion1D<float>;
using ReactionDiffusion1DD = ReactionDiffusion1D<double>;
using ReactionDiffusion2DF = ReactionDiffusion2D<float>;
using ReactionDiffusion2DD = ReactionDiffusion2D<double>;

} // namespace kinetics
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_KINETICS_REACTION_DIFFUSION_H
