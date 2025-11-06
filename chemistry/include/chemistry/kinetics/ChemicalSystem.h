/**
 * @file ChemicalSystem.h
 * @brief Integrated chemical system with species and reactions
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha3
 * @date 2025-11-06
 *
 * Combines species database and reaction mechanism into a unified system.
 */

#ifndef KOO_CHEMISTRY_CHEMICAL_SYSTEM_H
#define KOO_CHEMISTRY_CHEMICAL_SYSTEM_H

#include "chemistry/species/Species.h"
#include "chemistry/species/SpeciesManager.h"
#include "chemistry/reaction/Reaction.h"
#include "chemistry/reaction/ReactionManager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace koo {
namespace chemistry {

/**
 * @brief Complete chemical system with species and reactions
 *
 * Integrates:
 * - Species database (SpeciesManager)
 * - Reaction mechanism (ReactionManager)
 * - State vector management (concentrations)
 * - Production rate calculations
 */
class ChemicalSystem {
public:
    /**
     * @brief Default constructor
     */
    ChemicalSystem() = default;

    /**
     * @brief Constructor with managers
     */
    ChemicalSystem(const SpeciesManager& speciesManager, const ReactionManager& reactionManager)
        : speciesManager_(speciesManager), reactionManager_(reactionManager) {
        updateSpeciesIndex();
    }

    // Getters
    const SpeciesManager& getSpeciesManager() const { return speciesManager_; }
    const ReactionManager& getReactionManager() const { return reactionManager_; }
    SpeciesManager& getSpeciesManager() { return speciesManager_; }
    ReactionManager& getReactionManager() { return reactionManager_; }

    /**
     * @brief Get number of species in system
     */
    size_t getSpeciesCount() const {
        return speciesIndex_.size();
    }

    /**
     * @brief Get number of reactions in system
     */
    size_t getReactionCount() const {
        return reactionManager_.getReactionCount();
    }

    /**
     * @brief Get species names in order
     */
    const std::vector<std::string>& getSpeciesNames() const {
        return speciesIndex_;
    }

    /**
     * @brief Get species index by name
     * @param name Species name
     * @return Index in state vector, or -1 if not found
     */
    int getSpeciesIndex(const std::string& name) const {
        for (size_t i = 0; i < speciesIndex_.size(); ++i) {
            if (speciesIndex_[i] == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /**
     * @brief Set state vector (concentrations)
     * @param concentrations Vector of concentrations (mol/m³)
     */
    void setState(const std::vector<double>& concentrations) {
        if (concentrations.size() != speciesIndex_.size()) {
            throw std::invalid_argument("State vector size must match number of species");
        }
        state_ = concentrations;
        updateConcentrationMap();
    }

    /**
     * @brief Get state vector (concentrations)
     * @return Vector of concentrations (mol/m³)
     */
    const std::vector<double>& getState() const {
        return state_;
    }

    /**
     * @brief Set concentration for a specific species
     * @param name Species name
     * @param concentration Concentration (mol/m³)
     */
    void setConcentration(const std::string& name, double concentration) {
        int idx = getSpeciesIndex(name);
        if (idx < 0) {
            throw std::invalid_argument("Species not found: " + name);
        }
        state_[idx] = concentration;
        concentrationMap_[name] = concentration;
    }

    /**
     * @brief Get concentration for a specific species
     * @param name Species name
     * @return Concentration (mol/m³)
     */
    double getConcentration(const std::string& name) const {
        int idx = getSpeciesIndex(name);
        if (idx < 0) {
            throw std::invalid_argument("Species not found: " + name);
        }
        return state_[idx];
    }

    /**
     * @brief Set temperature
     * @param T Temperature (K)
     */
    void setTemperature(double T) {
        if (T <= 0.0) {
            throw std::invalid_argument("Temperature must be positive");
        }
        temperature_ = T;
    }

    /**
     * @brief Get temperature
     * @return Temperature (K)
     */
    double getTemperature() const {
        return temperature_;
    }

    /**
     * @brief Calculate production rates for all species
     * @return Vector of production rates (mol/(m³·s))
     */
    std::vector<double> getProductionRates() const {
        std::vector<double> rates(speciesIndex_.size(), 0.0);

        auto rateMap = reactionManager_.getAllProductionRates(temperature_, concentrationMap_);

        for (size_t i = 0; i < speciesIndex_.size(); ++i) {
            const std::string& name = speciesIndex_[i];
            auto it = rateMap.find(name);
            if (it != rateMap.end()) {
                rates[i] = it->second;
            }
        }

        return rates;
    }

    /**
     * @brief Update species index from species manager
     */
    void updateSpeciesIndex() {
        speciesIndex_.clear();
        auto species = reactionManager_.getAllSpecies();
        for (const auto& sp : species) {
            speciesIndex_.push_back(sp);
        }

        // Initialize state vector
        state_.resize(speciesIndex_.size(), 0.0);
        updateConcentrationMap();
    }

    /**
     * @brief Get system info string
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "Chemical System:\n";
        oss << "  Species: " << getSpeciesCount() << "\n";
        oss << "  Reactions: " << getReactionCount() << "\n";
        oss << "  Temperature: " << temperature_ << " K\n";
        return oss.str();
    }

    /**
     * @brief Print current state
     */
    void printState() const {
        std::cout << "Current State (T = " << temperature_ << " K):\n";
        std::cout << "======================================\n";
        for (size_t i = 0; i < speciesIndex_.size(); ++i) {
            std::cout << "  " << speciesIndex_[i] << ": "
                     << state_[i] << " mol/m³\n";
        }
    }

private:
    /**
     * @brief Update concentration map from state vector
     */
    void updateConcentrationMap() {
        concentrationMap_.clear();
        for (size_t i = 0; i < speciesIndex_.size(); ++i) {
            concentrationMap_[speciesIndex_[i]] = state_[i];
        }
    }

    SpeciesManager speciesManager_;                  ///< Species database
    ReactionManager reactionManager_;                ///< Reaction mechanism
    std::vector<std::string> speciesIndex_;          ///< Species names in order
    std::vector<double> state_;                      ///< Concentration state vector
    std::map<std::string, double> concentrationMap_; ///< Concentration map for rate calculations
    double temperature_{300.0};                      ///< System temperature (K)
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_CHEMICAL_SYSTEM_H
