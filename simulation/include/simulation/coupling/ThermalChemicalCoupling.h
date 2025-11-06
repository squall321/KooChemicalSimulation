/**
 * @file ThermalChemicalCoupling.h
 * @brief Thermal-chemical coupling for reactive flows
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * Phase 67: Multi-Physics Coupling
 *
 * Features:
 * - Temperature-dependent reaction rates (Arrhenius)
 * - Heat release from chemical reactions
 * - Energy balance equation
 * - Operator splitting for coupled system
 * - Heat of formation and specific heat
 */

#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace simulation {
namespace coupling {

/**
 * @brief Thermochemical properties
 */
struct ThermochemicalData {
    double molecular_weight;     ///< Molecular weight (kg/mol)
    double heat_of_formation;    ///< Standard heat of formation (J/mol)
    double specific_heat;        ///< Specific heat capacity (J/(kg·K))
    double thermal_conductivity; ///< Thermal conductivity (W/(m·K))

    ThermochemicalData()
        : molecular_weight(0.001),  // 1 g/mol default
          heat_of_formation(0.0),
          specific_heat(1000.0),    // ~1 kJ/(kg·K) default
          thermal_conductivity(0.1) {}
};

/**
 * @brief Energy source/sink from reactions
 */
struct EnergySource {
    double heat_release_rate;    ///< Heat release rate (W/m³)
    double enthalpy_change;      ///< Enthalpy change per reaction (J)
    double reaction_extent;      ///< Extent of reaction (mol/m³/s)

    EnergySource()
        : heat_release_rate(0.0), enthalpy_change(0.0),
          reaction_extent(0.0) {}
};

/**
 * @brief Thermal-chemical coupling manager
 */
class ThermalChemicalCoupling {
public:
    /**
     * @brief Construct coupling manager
     */
    ThermalChemicalCoupling() = default;

    /**
     * @brief Set thermochemical data for species
     */
    void setSpeciesData(int species_id, const ThermochemicalData& data) {
        if (species_id >= static_cast<int>(species_data_.size())) {
            species_data_.resize(species_id + 1);
        }
        species_data_[species_id] = data;
    }

    /**
     * @brief Compute temperature-dependent reaction rate
     *
     * Arrhenius equation: k(T) = A * exp(-Ea / (R*T))
     *
     * @param pre_exp Pre-exponential factor A
     * @param activation_energy Activation energy Ea (J/mol)
     * @param temperature Temperature T (K)
     * @return Rate constant k
     */
    double computeReactionRate(double pre_exp,
                              double activation_energy,
                              double temperature) const {
        const double R = 8.314;  // Universal gas constant (J/(mol·K))

        if (temperature <= 0.0) {
            throw std::invalid_argument("Temperature must be positive");
        }

        return pre_exp * std::exp(-activation_energy / (R * temperature));
    }

    /**
     * @brief Compute heat release from reaction
     *
     * Q = -Δ H_rxn * r
     * where Δ H_rxn is reaction enthalpy change, r is reaction rate
     *
     * @param reactant_ids Reactant species IDs
     * @param product_ids Product species IDs
     * @param stoich_reactants Stoichiometric coefficients (reactants)
     * @param stoich_products Stoichiometric coefficients (products)
     * @param reaction_rate Reaction rate (mol/m³/s)
     * @return Energy source
     */
    EnergySource computeHeatRelease(
        const std::vector<int>& reactant_ids,
        const std::vector<int>& product_ids,
        const std::vector<double>& stoich_reactants,
        const std::vector<double>& stoich_products,
        double reaction_rate) const {

        EnergySource source;

        // Compute enthalpy change: ΔH = Σ(ν_i * H_f,i)_products - Σ(ν_j * H_f,j)_reactants
        double delta_H = 0.0;

        // Products contribution
        for (size_t i = 0; i < product_ids.size(); ++i) {
            if (product_ids[i] < static_cast<int>(species_data_.size())) {
                delta_H += stoich_products[i] * species_data_[product_ids[i]].heat_of_formation;
            }
        }

        // Reactants contribution
        for (size_t i = 0; i < reactant_ids.size(); ++i) {
            if (reactant_ids[i] < static_cast<int>(species_data_.size())) {
                delta_H -= stoich_reactants[i] * species_data_[reactant_ids[i]].heat_of_formation;
            }
        }

        source.enthalpy_change = delta_H;
        source.reaction_extent = reaction_rate;

        // Heat release rate: Q = -ΔH * r (negative because exothermic releases heat)
        source.heat_release_rate = -delta_H * reaction_rate;

        return source;
    }

    /**
     * @brief Compute temperature change due to reaction heat release
     *
     * Using energy balance: ρ * c_p * dT/dt = Q
     *
     * @param heat_release Heat release rate (W/m³)
     * @param density Mixture density (kg/m³)
     * @param specific_heat Mixture specific heat (J/(kg·K))
     * @param dt Timestep (s)
     * @return Temperature change (K)
     */
    double computeTemperatureChange(double heat_release,
                                   double density,
                                   double specific_heat,
                                   double dt) const {
        if (density <= 0.0 || specific_heat <= 0.0) {
            throw std::invalid_argument("Density and specific heat must be positive");
        }

        // dT = (Q * dt) / (ρ * c_p)
        return (heat_release * dt) / (density * specific_heat);
    }

    /**
     * @brief Compute mixture specific heat
     *
     * Mass-weighted average: c_p,mix = Σ(Y_i * c_p,i)
     *
     * @param mass_fractions Mass fractions Y_i
     * @return Mixture specific heat (J/(kg·K))
     */
    double computeMixtureSpecificHeat(const std::vector<double>& mass_fractions) const {
        double cp_mix = 0.0;

        for (size_t i = 0; i < mass_fractions.size(); ++i) {
            if (i < species_data_.size()) {
                cp_mix += mass_fractions[i] * species_data_[i].specific_heat;
            }
        }

        return cp_mix;
    }

    /**
     * @brief Compute mixture thermal conductivity
     *
     * Simple mass-weighted average (more sophisticated models available)
     *
     * @param mass_fractions Mass fractions Y_i
     * @return Mixture thermal conductivity (W/(m·K))
     */
    double computeMixtureThermalConductivity(
        const std::vector<double>& mass_fractions) const {

        double k_mix = 0.0;

        for (size_t i = 0; i < mass_fractions.size(); ++i) {
            if (i < species_data_.size()) {
                k_mix += mass_fractions[i] * species_data_[i].thermal_conductivity;
            }
        }

        return k_mix;
    }

    /**
     * @brief Convert mass fractions to mole fractions
     */
    std::vector<double> massToMoleFractions(
        const std::vector<double>& mass_fractions) const {

        std::vector<double> mole_fractions(mass_fractions.size());

        // Compute mean molecular weight
        double mean_MW = 0.0;
        for (size_t i = 0; i < mass_fractions.size(); ++i) {
            if (i < species_data_.size()) {
                mean_MW += mass_fractions[i] / species_data_[i].molecular_weight;
            }
        }
        mean_MW = 1.0 / mean_MW;

        // Convert to mole fractions
        for (size_t i = 0; i < mass_fractions.size(); ++i) {
            if (i < species_data_.size()) {
                mole_fractions[i] = mass_fractions[i] * mean_MW /
                                   species_data_[i].molecular_weight;
            }
        }

        return mole_fractions;
    }

    /**
     * @brief Convert mole fractions to mass fractions
     */
    std::vector<double> moleToMassFractions(
        const std::vector<double>& mole_fractions) const {

        std::vector<double> mass_fractions(mole_fractions.size());

        // Compute mean molecular weight
        double mean_MW = 0.0;
        for (size_t i = 0; i < mole_fractions.size(); ++i) {
            if (i < species_data_.size()) {
                mean_MW += mole_fractions[i] * species_data_[i].molecular_weight;
            }
        }

        // Convert to mass fractions
        for (size_t i = 0; i < mole_fractions.size(); ++i) {
            if (i < species_data_.size()) {
                mass_fractions[i] = mole_fractions[i] *
                                   species_data_[i].molecular_weight / mean_MW;
            }
        }

        return mass_fractions;
    }

private:
    std::vector<ThermochemicalData> species_data_;
};

/**
 * @brief Operator splitting for coupled thermal-chemical system
 *
 * Strang splitting: L(dt) = L_chem(dt/2) ∘ L_therm(dt) ∘ L_chem(dt/2)
 */
class OperatorSplitting {
public:
    enum class SplittingScheme {
        Sequential,    ///< Sequential: L_chem ∘ L_therm
        Strang        ///< Strang: L_chem(dt/2) ∘ L_therm(dt) ∘ L_chem(dt/2)
    };

    /**
     * @brief Construct with splitting scheme
     */
    explicit OperatorSplitting(SplittingScheme scheme = SplittingScheme::Strang)
        : scheme_(scheme) {}

    /**
     * @brief Get splitting scheme
     */
    SplittingScheme getScheme() const { return scheme_; }

    /**
     * @brief Set splitting scheme
     */
    void setScheme(SplittingScheme scheme) { scheme_ = scheme; }

    /**
     * @brief Compute substep size for given operator
     *
     * @param dt Full timestep
     * @param step_number Substep number (0, 1, 2, ...)
     * @param operator_type 0=chemistry, 1=thermal
     * @return Substep size
     */
    double getSubstepSize(double dt, int step_number, int operator_type) const {
        switch (scheme_) {
            case SplittingScheme::Sequential:
                return dt;  // Full step for each operator

            case SplittingScheme::Strang:
                if (operator_type == 0) {  // Chemistry
                    return dt / 2.0;        // Half steps
                } else {                   // Thermal
                    return dt;              // Full step
                }
        }

        return dt;
    }

    /**
     * @brief Get number of substeps for each operator
     */
    int getNumSubsteps(int operator_type) const {
        switch (scheme_) {
            case SplittingScheme::Sequential:
                return 1;

            case SplittingScheme::Strang:
                if (operator_type == 0) {  // Chemistry
                    return 2;               // Two half-steps
                } else {                   // Thermal
                    return 1;               // One full step
                }
        }

        return 1;
    }

private:
    SplittingScheme scheme_;
};

}  // namespace coupling
}  // namespace simulation
}  // namespace koo
