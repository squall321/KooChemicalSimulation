/**
 * @file ConcentrationField.h
 * @brief Concentration field mapping for PDE variables
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-beta
 * @date 2025-11-06
 *
 * Maps chemical species concentrations to PDE field variables.
 */

#ifndef KOO_CHEMISTRY_CONCENTRATION_FIELD_H
#define KOO_CHEMISTRY_CONCENTRATION_FIELD_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace koo {
namespace chemistry {

/**
 * @brief Concentration field for chemical species
 *
 * Maps species names to field indices and manages
 * concentration data as PDE variables.
 */
class ConcentrationField {
public:
    /**
     * @brief Default constructor
     */
    ConcentrationField() = default;

    /**
     * @brief Constructor with species list
     * @param speciesNames List of species names
     */
    explicit ConcentrationField(const std::vector<std::string>& speciesNames) {
        setSpecies(speciesNames);
    }

    /**
     * @brief Set species list and create mapping
     * @param speciesNames List of species names
     */
    void setSpecies(const std::vector<std::string>& speciesNames) {
        speciesNames_ = speciesNames;
        nameToIndex_.clear();

        for (size_t i = 0; i < speciesNames.size(); ++i) {
            nameToIndex_[speciesNames[i]] = i;
        }

        // Initialize concentrations to zero
        concentrations_.resize(speciesNames.size(), 0.0);
    }

    /**
     * @brief Get number of species
     * @return Number of species
     */
    size_t getSpeciesCount() const {
        return speciesNames_.size();
    }

    /**
     * @brief Get species names
     * @return Vector of species names
     */
    const std::vector<std::string>& getSpeciesNames() const {
        return speciesNames_;
    }

    /**
     * @brief Get species index by name
     * @param name Species name
     * @return Index in field, or -1 if not found
     */
    int getSpeciesIndex(const std::string& name) const {
        auto it = nameToIndex_.find(name);
        if (it != nameToIndex_.end()) {
            return static_cast<int>(it->second);
        }
        return -1;
    }

    /**
     * @brief Get species name by index
     * @param index Field index
     * @return Species name
     */
    std::string getSpeciesName(size_t index) const {
        if (index >= speciesNames_.size()) {
            throw std::out_of_range("Species index out of range");
        }
        return speciesNames_[index];
    }

    /**
     * @brief Set concentration for a species
     * @param name Species name
     * @param concentration Concentration (mol/m³)
     */
    void setConcentration(const std::string& name, double concentration) {
        int idx = getSpeciesIndex(name);
        if (idx < 0) {
            throw std::invalid_argument("Species not found: " + name);
        }
        concentrations_[static_cast<size_t>(idx)] = concentration;
    }

    /**
     * @brief Set concentration by index
     * @param index Field index
     * @param concentration Concentration (mol/m³)
     */
    void setConcentration(size_t index, double concentration) {
        if (index >= concentrations_.size()) {
            throw std::out_of_range("Index out of range");
        }
        concentrations_[index] = concentration;
    }

    /**
     * @brief Get concentration for a species
     * @param name Species name
     * @return Concentration (mol/m³)
     */
    double getConcentration(const std::string& name) const {
        int idx = getSpeciesIndex(name);
        if (idx < 0) {
            throw std::invalid_argument("Species not found: " + name);
        }
        return concentrations_[static_cast<size_t>(idx)];
    }

    /**
     * @brief Get concentration by index
     * @param index Field index
     * @return Concentration (mol/m³)
     */
    double getConcentration(size_t index) const {
        if (index >= concentrations_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return concentrations_[index];
    }

    /**
     * @brief Get all concentrations as vector
     * @return Vector of concentrations
     */
    const std::vector<double>& getConcentrations() const {
        return concentrations_;
    }

    /**
     * @brief Set all concentrations from vector
     * @param concentrations Vector of concentrations
     */
    void setConcentrations(const std::vector<double>& concentrations) {
        if (concentrations.size() != concentrations_.size()) {
            throw std::invalid_argument("Concentration vector size mismatch");
        }
        concentrations_ = concentrations;
    }

    /**
     * @brief Get concentrations as map
     * @return Map of species names to concentrations
     */
    std::map<std::string, double> getConcentrationMap() const {
        std::map<std::string, double> concMap;
        for (size_t i = 0; i < speciesNames_.size(); ++i) {
            concMap[speciesNames_[i]] = concentrations_[i];
        }
        return concMap;
    }

    /**
     * @brief Set concentrations from map
     * @param concMap Map of species names to concentrations
     */
    void setConcentrationMap(const std::map<std::string, double>& concMap) {
        for (const auto& [name, conc] : concMap) {
            int idx = getSpeciesIndex(name);
            if (idx >= 0) {
                concentrations_[static_cast<size_t>(idx)] = conc;
            }
        }
    }

    /**
     * @brief Clear all concentrations to zero
     */
    void clear() {
        std::fill(concentrations_.begin(), concentrations_.end(), 0.0);
    }

    /**
     * @brief Get field info string
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "ConcentrationField:\n";
        oss << "  Species count: " << speciesNames_.size() << "\n";
        oss << "  Field variables:\n";
        for (size_t i = 0; i < speciesNames_.size(); ++i) {
            oss << "    [" << i << "] " << speciesNames_[i]
                << " = " << concentrations_[i] << " mol/m³\n";
        }
        return oss.str();
    }

    /**
     * @brief Print field state
     */
    void print() const {
        std::cout << getInfo();
    }

private:
    std::vector<std::string> speciesNames_;           ///< Species names in order
    std::map<std::string, size_t> nameToIndex_;       ///< Name to index mapping
    std::vector<double> concentrations_;              ///< Concentration values
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_CONCENTRATION_FIELD_H
