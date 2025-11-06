/**
 * @file IChemicalSystem.h
 * @brief Interface for chemical reaction systems
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha2
 * @date 2025-11-06
 *
 * This file defines the IChemicalSystem interface, which provides an abstraction
 * for systems of chemical species and reactions.
 */

#ifndef KOO_CORE_INTERFACES_ICHEMICAL_SYSTEM_H
#define KOO_CORE_INTERFACES_ICHEMICAL_SYSTEM_H

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace koo {
namespace core {

/**
 * @brief Interface for chemical reaction systems
 *
 * IChemicalSystem provides a unified interface for managing chemical species,
 * reactions, and computing reaction rates and source terms for PDE solvers.
 *
 * Key responsibilities:
 * - Managing chemical species and their properties
 * - Defining chemical reactions and kinetics
 * - Computing reaction rates based on current concentrations
 * - Calculating source/sink terms for transport equations
 * - Providing thermodynamic and kinetic properties
 *
 * Design Pattern: Composite Pattern (species and reactions form a system)
 *
 * @note Thread-safe for read operations after initialization
 *
 * Example usage:
 * @code
 * auto chemSystem = createChemicalSystem();
 * chemSystem->addSpecies("H2O", 18.015);
 * chemSystem->addReaction("2H2 + O2 -> 2H2O", 1e-3);
 *
 * std::vector<double> concentrations = {1.0, 0.5, 0.0}; // [H2, O2, H2O]
 * auto rates = chemSystem->computeReactionRates(concentrations, 298.15);
 * @endcode
 */
class IChemicalSystem {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IChemicalSystem() = default;

    /**
     * @brief Add a chemical species to the system
     *
     * Registers a new chemical species with the system.
     *
     * @param name Species name (e.g., "H2O", "Fe2+")
     * @param molecularWeight Molecular weight in g/mol
     * @param charge Electric charge (for ions), default 0
     * @throws std::invalid_argument if species already exists
     * @throws std::invalid_argument if molecularWeight <= 0
     */
    virtual void addSpecies(const std::string& name,
                           double molecularWeight,
                           int charge = 0) = 0;

    /**
     * @brief Add a chemical reaction to the system
     *
     * Parses and adds a chemical reaction. The reaction string should be in
     * the format: "reactants -> products" or "reactants <-> products" for
     * reversible reactions.
     *
     * @param reactionString Reaction equation (e.g., "A + 2B -> C")
     * @param rateConstant Forward reaction rate constant
     * @throws std::invalid_argument if reaction string is malformed
     * @throws std::runtime_error if species in reaction are not defined
     */
    virtual void addReaction(const std::string& reactionString,
                            double rateConstant) = 0;

    /**
     * @brief Get number of species in the system
     *
     * @return Number of chemical species
     */
    virtual size_t getNumSpecies() const = 0;

    /**
     * @brief Get number of reactions in the system
     *
     * @return Number of chemical reactions
     */
    virtual size_t getNumReactions() const = 0;

    /**
     * @brief Get species index by name
     *
     * @param name Species name
     * @return Species index (0-based), or -1 if not found
     */
    virtual int getSpeciesIndex(const std::string& name) const = 0;

    /**
     * @brief Get species name by index
     *
     * @param index Species index (0-based)
     * @return Species name
     * @throws std::out_of_range if index is invalid
     */
    virtual std::string getSpeciesName(size_t index) const = 0;

    /**
     * @brief Get molecular weight of a species
     *
     * @param speciesIndex Species index (0-based)
     * @return Molecular weight in g/mol
     * @throws std::out_of_range if speciesIndex is invalid
     */
    virtual double getMolecularWeight(size_t speciesIndex) const = 0;

    /**
     * @brief Get charge of a species
     *
     * @param speciesIndex Species index (0-based)
     * @return Electric charge
     * @throws std::out_of_range if speciesIndex is invalid
     */
    virtual int getCharge(size_t speciesIndex) const = 0;

    /**
     * @brief Compute reaction rates for all reactions
     *
     * Calculates the rate of each reaction based on current species
     * concentrations and temperature.
     *
     * @param concentrations Vector of species concentrations (mol/L)
     * @param temperature Temperature in Kelvin
     * @return Vector of reaction rates (mol/L/s)
     * @throws std::invalid_argument if concentrations.size() != getNumSpecies()
     * @throws std::invalid_argument if temperature <= 0
     */
    virtual std::vector<double> computeReactionRates(
        const std::vector<double>& concentrations,
        double temperature) const = 0;

    /**
     * @brief Compute source terms for species transport equations
     *
     * Calculates the net production/consumption rate for each species due
     * to all chemical reactions. This is the source term for the PDE.
     *
     * @param concentrations Vector of species concentrations (mol/L)
     * @param temperature Temperature in Kelvin
     * @return Vector of source terms (mol/L/s), one per species
     * @throws std::invalid_argument if concentrations.size() != getNumSpecies()
     */
    virtual std::vector<double> computeSourceTerms(
        const std::vector<double>& concentrations,
        double temperature) const = 0;

    /**
     * @brief Compute Jacobian matrix for source terms
     *
     * Calculates the Jacobian ∂f/∂c where f is the source term vector
     * and c is the concentration vector. This is needed for implicit
     * time integration schemes.
     *
     * @param concentrations Vector of species concentrations (mol/L)
     * @param temperature Temperature in Kelvin
     * @return Jacobian matrix (flattened, row-major order)
     * @throws std::invalid_argument if concentrations.size() != getNumSpecies()
     */
    virtual std::vector<std::vector<double>> computeJacobian(
        const std::vector<double>& concentrations,
        double temperature) const = 0;

    /**
     * @brief Set diffusion coefficient for a species
     *
     * @param speciesIndex Species index (0-based)
     * @param diffusionCoeff Diffusion coefficient (m²/s)
     * @throws std::out_of_range if speciesIndex is invalid
     * @throws std::invalid_argument if diffusionCoeff < 0
     */
    virtual void setDiffusionCoefficient(size_t speciesIndex,
                                        double diffusionCoeff) = 0;

    /**
     * @brief Get diffusion coefficient for a species
     *
     * @param speciesIndex Species index (0-based)
     * @return Diffusion coefficient (m²/s)
     * @throws std::out_of_range if speciesIndex is invalid
     */
    virtual double getDiffusionCoefficient(size_t speciesIndex) const = 0;

    /**
     * @brief Validate the chemical system
     *
     * Checks that:
     * - All reactions reference defined species
     * - Mass and charge are balanced in reactions
     * - Rate constants are positive
     *
     * @return true if system is valid, false otherwise
     */
    virtual bool validate() const = 0;

    /**
     * @brief Get system name/identifier
     *
     * @return System name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Clear all species and reactions
     *
     * Resets the chemical system to empty state.
     */
    virtual void clear() = 0;

    /**
     * @brief Export system to string representation
     *
     * Generates a human-readable string describing all species and reactions.
     *
     * @return String representation of the chemical system
     */
    virtual std::string toString() const = 0;

    /**
     * @brief Check if system contains a specific species
     *
     * @param name Species name
     * @return true if species exists in system
     */
    virtual bool hasSpecies(const std::string& name) const {
        return getSpeciesIndex(name) >= 0;
    }

    /**
     * @brief Get all species names
     *
     * @return Vector of all species names in the system
     */
    virtual std::vector<std::string> getAllSpeciesNames() const = 0;

    /**
     * @brief Set temperature-dependent rate constant (Arrhenius)
     *
     * Sets an Arrhenius-type temperature dependence for a reaction:
     * k(T) = A * exp(-Ea / (R*T))
     *
     * @param reactionIndex Reaction index (0-based)
     * @param preExponential Pre-exponential factor A
     * @param activationEnergy Activation energy Ea (J/mol)
     * @throws std::out_of_range if reactionIndex is invalid
     */
    virtual void setArrheniusParameters(size_t reactionIndex,
                                       double preExponential,
                                       double activationEnergy) = 0;

protected:
    /**
     * @brief Protected default constructor
     */
    IChemicalSystem() = default;

    /**
     * @brief Protected copy constructor (deleted)
     */
    IChemicalSystem(const IChemicalSystem&) = delete;

    /**
     * @brief Protected copy assignment (deleted)
     */
    IChemicalSystem& operator=(const IChemicalSystem&) = delete;

    /**
     * @brief Protected move constructor
     */
    IChemicalSystem(IChemicalSystem&&) = default;

    /**
     * @brief Protected move assignment
     */
    IChemicalSystem& operator=(IChemicalSystem&&) = default;
};

/**
 * @brief Shared pointer type for IChemicalSystem
 */
using IChemicalSystemPtr = std::shared_ptr<IChemicalSystem>;

/**
 * @brief Unique pointer type for IChemicalSystem
 */
using IChemicalSystemUniquePtr = std::unique_ptr<IChemicalSystem>;

} // namespace core
} // namespace koo

#endif // KOO_CORE_INTERFACES_ICHEMICAL_SYSTEM_H
