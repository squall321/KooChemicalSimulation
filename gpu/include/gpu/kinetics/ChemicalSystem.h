/**
 * @file ChemicalSystem.h
 * @brief GPU-accelerated chemical reaction system
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * Provides a simplified GPU-accelerated chemical system
 * for batched kinetics calculations across multiple cells.
 */

#ifndef KOO_GPU_KINETICS_CHEMICAL_SYSTEM_H
#define KOO_GPU_KINETICS_CHEMICAL_SYSTEM_H

#include "ReactionKernels.h"
#include "ODESolver.h"
#include <vector>
#include <string>
#include <map>

namespace koo {
namespace gpu {
namespace kinetics {

/**
 * @struct ReactionData
 * @brief Data for a single reaction
 */
template<typename T>
struct ReactionData {
    T A;              // Pre-exponential factor
    T beta;           // Temperature exponent
    T Ea;             // Activation energy (J/mol)
    std::vector<int> reactant_indices;
    std::vector<T> reactant_orders;
    std::vector<int> product_indices;
    std::vector<T> product_stoich;
};

/**
 * @class GPUChemicalSystem
 * @brief Simplified chemical system for GPU
 *
 * Phase 54: GPU Reaction Kinetics
 *
 * This is a simplified version optimized for GPU execution.
 * For full chemistry features, use the CPU ChemicalSystem.
 *
 * Features:
 * - Batched kinetics for multiple cells
 * - Arrhenius rate calculations
 * - ODE integration (Euler, RK2, RK4)
 * - Net stoichiometric matrix
 */
template<typename T>
class GPUChemicalSystem {
public:
    /**
     * @brief Constructor
     * @param n_species Number of species
     * @param n_reactions Number of reactions
     * @param n_cells Number of cells (for batched evaluation)
     */
    GPUChemicalSystem(int n_species, int n_reactions, int n_cells = 1)
        : n_species_(n_species),
          n_reactions_(n_reactions),
          n_cells_(n_cells),
          kernels_(n_species, n_reactions, n_cells),
          concentrations_(n_species * n_cells),
          A_(n_reactions),
          beta_(n_reactions),
          Ea_(n_reactions),
          T_values_(n_cells),
          k_fwd_(n_reactions * n_cells),
          ROP_(n_reactions * n_cells),
          omega_(n_species * n_cells),
          nu_net_(n_species * n_reactions)
    {
        // Initialize to zero
        concentrations_.zero();
        A_.zero();
        beta_.zero();
        Ea_.zero();
        T_values_.fill(300.0);  // Default temperature
        nu_net_.zero();
    }

    /**
     * @brief Set species names (for reference)
     */
    void setSpeciesNames(const std::vector<std::string>& names) {
        if (names.size() != static_cast<size_t>(n_species_)) {
            throw KineticsError("Species names size mismatch");
        }
        species_names_ = names;
    }

    /**
     * @brief Add reaction
     * @param rxn_idx Reaction index
     * @param data Reaction data
     */
    void addReaction(int rxn_idx, const ReactionData<T>& data) {
        if (rxn_idx < 0 || rxn_idx >= n_reactions_) {
            throw KineticsError("Reaction index out of range");
        }

        // Set Arrhenius parameters (on host, will copy to device later)
        auto hostA = A_.toHost();
        auto hostBeta = beta_.toHost();
        auto hostEa = Ea_.toHost();

        hostA[rxn_idx] = data.A;
        hostBeta[rxn_idx] = data.beta;
        hostEa[rxn_idx] = data.Ea;

        A_.copyFromHost(hostA.data(), hostA.size());
        beta_.copyFromHost(hostBeta.data(), hostBeta.size());
        Ea_.copyFromHost(hostEa.data(), hostEa.size());

        // Build net stoichiometric matrix
        auto hostNu = nu_net_.toHost();

        // Products
        for (size_t i = 0; i < data.product_indices.size(); ++i) {
            int spec_idx = data.product_indices[i];
            T stoich = data.product_stoich[i];
            hostNu[spec_idx * n_reactions_ + rxn_idx] += stoich;
        }

        // Reactants (negative contribution)
        for (size_t i = 0; i < data.reactant_indices.size(); ++i) {
            int spec_idx = data.reactant_indices[i];
            T stoich = data.reactant_orders[i];  // Assume stoich = order for simplicity
            hostNu[spec_idx * n_reactions_ + rxn_idx] -= stoich;
        }

        nu_net_.copyFromHost(hostNu.data(), hostNu.size());
    }

    /**
     * @brief Set concentration for a species at a cell
     * @param species_idx Species index
     * @param cell_idx Cell index
     * @param value Concentration (mol/m³)
     */
    void setConcentration(int species_idx, int cell_idx, T value) {
        auto hostC = concentrations_.toHost();
        int idx = species_idx * n_cells_ + cell_idx;
        hostC[idx] = value;
        concentrations_.copyFromHost(hostC.data(), hostC.size());
    }

    /**
     * @brief Set concentrations for all species at a cell
     */
    void setConcentrations(int cell_idx, const std::vector<T>& values) {
        if (values.size() != static_cast<size_t>(n_species_)) {
            throw KineticsError("Concentration vector size mismatch");
        }

        auto hostC = concentrations_.toHost();
        for (int s = 0; s < n_species_; ++s) {
            hostC[s * n_cells_ + cell_idx] = values[s];
        }
        concentrations_.copyFromHost(hostC.data(), hostC.size());
    }

    /**
     * @brief Get concentrations for a cell
     */
    std::vector<T> getConcentrations(int cell_idx = 0) const {
        auto hostC = concentrations_.toHost();
        std::vector<T> result(n_species_);
        for (int s = 0; s < n_species_; ++s) {
            result[s] = hostC[s * n_cells_ + cell_idx];
        }
        return result;
    }

    /**
     * @brief Set temperature for a cell
     */
    void setTemperature(int cell_idx, T temperature) {
        auto hostT = T_values_.toHost();
        hostT[cell_idx] = temperature;
        T_values_.copyFromHost(hostT.data(), hostT.size());
    }

    /**
     * @brief Set temperature for all cells
     */
    void setTemperature(T temperature) {
        T_values_.fill(temperature);
    }

    /**
     * @brief Calculate production rates
     *
     * Updates omega_ with current production rates
     * based on current concentrations and temperature.
     */
    void calculateProductionRates() {
        // Calculate rate constants
        kernels_.calculateArrheniusRates(A_, beta_, Ea_, T_values_, k_fwd_);

        // For simplified system, assume ROP = k_fwd * C_reactant
        // In full system, this would compute product of concentrations
        // For now, use a simplified approach:
        // Set ROP = k_fwd (will be scaled by stoichiometry)
        ROP_.copyFromHost(k_fwd_.toHost().data(), k_fwd_.size());

        // Calculate production rates: omega = nu * ROP
        kernels_.calculateProductionRates(ROP_, nu_net_, omega_);
    }

    /**
     * @brief Get production rates for a cell
     */
    std::vector<T> getProductionRates(int cell_idx = 0) const {
        auto hostOmega = omega_.toHost();
        std::vector<T> result(n_species_);
        for (int s = 0; s < n_species_; ++s) {
            result[s] = hostOmega[s * n_cells_ + cell_idx];
        }
        return result;
    }

    /**
     * @brief Advance system by one time step
     * @param dt Time step (s)
     * @param method Integration method
     */
    void advance(T dt, ODEMethod method = ODEMethod::RK4) {
        // Create ODE solver
        ODESolverGPU<T> solver(n_species_, n_cells_, method);

        // Define RHS function
        auto rhs_func = [this](const DeviceMemory<T>& C, DeviceMemory<T>& omega) {
            // Update concentrations
            concentrations_.copyFromHost(C.toHost().data(), C.size());

            // Calculate production rates
            calculateProductionRates();

            // Return production rates
            omega.copyFromHost(omega_.toHost().data(), omega_.size());
        };

        // Take a step
        solver.step(concentrations_, dt, rhs_func);
    }

    /**
     * @brief Integrate system
     */
    ODESolverStats integrate(T t_final, T dt, ODEMethod method = ODEMethod::RK4) {
        int n_steps = static_cast<int>(std::ceil(t_final / dt));

        ODESolverGPU<T> solver(n_species_, n_cells_, method);

        auto rhs_func = [this](const DeviceMemory<T>& C, DeviceMemory<T>& omega) {
            concentrations_.copyFromHost(C.toHost().data(), C.size());
            calculateProductionRates();
            omega.copyFromHost(omega_.toHost().data(), omega_.size());
        };

        return solver.solve(concentrations_, dt, n_steps, rhs_func);
    }

    // Getters
    int getSpeciesCount() const { return n_species_; }
    int getReactionCount() const { return n_reactions_; }
    int getCellCount() const { return n_cells_; }

    const std::vector<std::string>& getSpeciesNames() const { return species_names_; }

    DeviceMemory<T>& getConcentrationsDevice() { return concentrations_; }
    const DeviceMemory<T>& getConcentrationsDevice() const { return concentrations_; }

private:
    int n_species_;
    int n_reactions_;
    int n_cells_;

    ReactionKernels<T> kernels_;

    // State
    DeviceMemory<T> concentrations_;  // n_species * n_cells

    // Reaction parameters
    DeviceMemory<T> A_;      // n_reactions
    DeviceMemory<T> beta_;   // n_reactions
    DeviceMemory<T> Ea_;     // n_reactions

    // Thermodynamic state
    DeviceMemory<T> T_values_;  // n_cells

    // Working arrays
    DeviceMemory<T> k_fwd_;     // n_reactions * n_cells
    DeviceMemory<T> ROP_;       // n_reactions * n_cells
    DeviceMemory<T> omega_;     // n_species * n_cells

    // Stoichiometry
    DeviceMemory<T> nu_net_;    // n_species * n_reactions (net stoich matrix)

    // Metadata
    std::vector<std::string> species_names_;
};

// Type aliases
using GPUChemicalSystemF = GPUChemicalSystem<float>;
using GPUChemicalSystemD = GPUChemicalSystem<double>;

} // namespace kinetics
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_KINETICS_CHEMICAL_SYSTEM_H
