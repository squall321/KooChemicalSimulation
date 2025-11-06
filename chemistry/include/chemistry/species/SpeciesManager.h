/**
 * @file SpeciesManager.h
 * @brief Manager for chemical species database
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha1
 * @date 2025-11-06
 *
 * Manages a collection of chemical species with lookup and query capabilities.
 */

#ifndef KOO_CHEMISTRY_SPECIES_MANAGER_H
#define KOO_CHEMISTRY_SPECIES_MANAGER_H

#include "Species.h"
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace koo {
namespace chemistry {

/**
 * @brief Manager for chemical species database
 *
 * Maintains a collection of species and provides efficient lookup,
 * filtering, and query capabilities.
 */
class SpeciesManager {
public:
    SpeciesManager() = default;

    // === Species Management ===

    /**
     * @brief Add a species to the database
     * @param species Species to add
     * @throws std::runtime_error if species with same name already exists
     */
    void addSpecies(const Species& species) {
        const std::string& name = species.getName();
        if (speciesByName_.find(name) != speciesByName_.end()) {
            throw std::runtime_error("Species '" + name + "' already exists");
        }

        auto sp = std::make_shared<Species>(species);
        speciesByName_[name] = sp;
        speciesList_.push_back(sp);

        // Index by phase
        speciesByPhase_[species.getPhase()].push_back(sp);

        // Index by elements
        for (const auto& elem : species.getComposition()) {
            speciesByElement_[elem.first].push_back(sp);
        }
    }

    /**
     * @brief Remove a species by name
     * @param name Species name
     * @return True if species was removed
     */
    bool removeSpecies(const std::string& name) {
        auto it = speciesByName_.find(name);
        if (it == speciesByName_.end()) {
            return false;
        }

        auto sp = it->second;

        // Remove from name map
        speciesByName_.erase(it);

        // Remove from list
        speciesList_.erase(
            std::remove(speciesList_.begin(), speciesList_.end(), sp),
            speciesList_.end()
        );

        // Remove from phase index
        auto& phaseVec = speciesByPhase_[sp->getPhase()];
        phaseVec.erase(
            std::remove(phaseVec.begin(), phaseVec.end(), sp),
            phaseVec.end()
        );

        // Remove from element index
        for (const auto& elem : sp->getComposition()) {
            auto& elemVec = speciesByElement_[elem.first];
            elemVec.erase(
                std::remove(elemVec.begin(), elemVec.end(), sp),
                elemVec.end()
            );
        }

        return true;
    }

    /**
     * @brief Clear all species
     */
    void clear() {
        speciesByName_.clear();
        speciesList_.clear();
        speciesByPhase_.clear();
        speciesByElement_.clear();
    }

    // === Queries ===

    /**
     * @brief Get species by name
     * @param name Species name
     * @return Shared pointer to species, or nullptr if not found
     */
    std::shared_ptr<Species> getSpecies(const std::string& name) const {
        auto it = speciesByName_.find(name);
        return (it != speciesByName_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Check if species exists
     * @param name Species name
     * @return True if species exists
     */
    bool hasSpecies(const std::string& name) const {
        return speciesByName_.find(name) != speciesByName_.end();
    }

    /**
     * @brief Get all species
     * @return Vector of all species
     */
    const std::vector<std::shared_ptr<Species>>& getAllSpecies() const {
        return speciesList_;
    }

    /**
     * @brief Get species by phase
     * @param phase Phase type
     * @return Vector of species in the specified phase
     */
    std::vector<std::shared_ptr<Species>> getSpeciesByPhase(PhaseType phase) const {
        auto it = speciesByPhase_.find(phase);
        return (it != speciesByPhase_.end()) ? it->second : std::vector<std::shared_ptr<Species>>();
    }

    /**
     * @brief Get species containing element
     * @param element Element symbol
     * @return Vector of species containing the element
     */
    std::vector<std::shared_ptr<Species>> getSpeciesByElement(const std::string& element) const {
        auto it = speciesByElement_.find(element);
        return (it != speciesByElement_.end()) ? it->second : std::vector<std::shared_ptr<Species>>();
    }

    /**
     * @brief Get number of species
     * @return Total number of species
     */
    size_t getNumSpecies() const {
        return speciesList_.size();
    }

    /**
     * @brief Get number of species by phase
     * @param phase Phase type
     * @return Number of species in phase
     */
    size_t getNumSpeciesByPhase(PhaseType phase) const {
        auto it = speciesByPhase_.find(phase);
        return (it != speciesByPhase_.end()) ? it->second.size() : 0;
    }

    /**
     * @brief Get all species names
     * @return Vector of species names
     */
    std::vector<std::string> getSpeciesNames() const {
        std::vector<std::string> names;
        names.reserve(speciesList_.size());
        for (const auto& sp : speciesList_) {
            names.push_back(sp->getName());
        }
        return names;
    }

    // === Database Operations ===

    /**
     * @brief Create common gas species database
     *
     * Adds commonly used gas species with simplified properties
     */
    void loadCommonGasSpecies() {
        // H2
        Species H2("H2", {{"H", 2}}, PhaseType::GAS);
        H2.setMolecularWeight(0.002016);
        TransportData H2_transport;
        H2_transport.molecularWeight = 0.002016;
        H2_transport.lennardJonesSigma = 2.92;
        H2_transport.lennardJonesEpsilon = 38.0;
        H2.setTransportData(H2_transport);
        addSpecies(H2);

        // O2
        Species O2("O2", {{"O", 2}}, PhaseType::GAS);
        O2.setMolecularWeight(0.032);
        TransportData O2_transport;
        O2_transport.molecularWeight = 0.032;
        O2_transport.lennardJonesSigma = 3.46;
        O2_transport.lennardJonesEpsilon = 107.4;
        O2.setTransportData(O2_transport);
        addSpecies(O2);

        // H2O
        Species H2O("H2O", {{"H", 2}, {"O", 1}}, PhaseType::GAS);
        H2O.setMolecularWeight(0.018015);
        TransportData H2O_transport;
        H2O_transport.molecularWeight = 0.018015;
        H2O_transport.lennardJonesSigma = 2.60;
        H2O_transport.lennardJonesEpsilon = 572.4;
        H2O_transport.dipoleMoment = 1.844;
        H2O.setTransportData(H2O_transport);
        addSpecies(H2O);

        // N2
        Species N2("N2", {{"N", 2}}, PhaseType::GAS);
        N2.setMolecularWeight(0.028014);
        TransportData N2_transport;
        N2_transport.molecularWeight = 0.028014;
        N2_transport.lennardJonesSigma = 3.62;
        N2_transport.lennardJonesEpsilon = 97.5;
        N2.setTransportData(N2_transport);
        addSpecies(N2);

        // CO
        Species CO("CO", {{"C", 1}, {"O", 1}}, PhaseType::GAS);
        CO.setMolecularWeight(0.028010);
        TransportData CO_transport;
        CO_transport.molecularWeight = 0.028010;
        CO_transport.lennardJonesSigma = 3.65;
        CO_transport.lennardJonesEpsilon = 98.1;
        CO.setTransportData(CO_transport);
        addSpecies(CO);

        // CO2
        Species CO2("CO2", {{"C", 1}, {"O", 2}}, PhaseType::GAS);
        CO2.setMolecularWeight(0.044010);
        TransportData CO2_transport;
        CO2_transport.molecularWeight = 0.044010;
        CO2_transport.lennardJonesSigma = 3.76;
        CO2_transport.lennardJonesEpsilon = 244.0;
        CO2.setTransportData(CO2_transport);
        addSpecies(CO2);

        // CH4
        Species CH4("CH4", {{"C", 1}, {"H", 4}}, PhaseType::GAS);
        CH4.setMolecularWeight(0.016043);
        TransportData CH4_transport;
        CH4_transport.molecularWeight = 0.016043;
        CH4_transport.lennardJonesSigma = 3.75;
        CH4_transport.lennardJonesEpsilon = 141.4;
        CH4.setTransportData(CH4_transport);
        addSpecies(CH4);

        // Ar
        Species Ar("Ar", {{"Ar", 1}}, PhaseType::GAS);
        Ar.setMolecularWeight(0.039948);
        TransportData Ar_transport;
        Ar_transport.molecularWeight = 0.039948;
        Ar_transport.lennardJonesSigma = 3.54;
        Ar_transport.lennardJonesEpsilon = 136.5;
        Ar.setTransportData(Ar_transport);
        addSpecies(Ar);
    }

    // === Information ===

    /**
     * @brief Get database summary
     */
    std::string getSummary() const {
        std::string summary = "Species Database Summary:\n";
        summary += "  Total species: " + std::to_string(getNumSpecies()) + "\n";

        for (int i = 0; i < 5; ++i) {
            PhaseType phase = static_cast<PhaseType>(i);
            size_t count = getNumSpeciesByPhase(phase);
            if (count > 0) {
                summary += "  " + toString(phase) + " species: " + std::to_string(count) + "\n";
            }
        }

        return summary;
    }

    /**
     * @brief Print all species information
     */
    void printAllSpecies() const {
        std::cout << "Species Database (" << getNumSpecies() << " species):\n";
        std::cout << "==========================================\n";
        for (const auto& sp : speciesList_) {
            std::cout << sp->getInfo();
            std::cout << "----------\n";
        }
    }

private:
    std::map<std::string, std::shared_ptr<Species>> speciesByName_;
    std::vector<std::shared_ptr<Species>> speciesList_;
    std::map<PhaseType, std::vector<std::shared_ptr<Species>>> speciesByPhase_;
    std::map<std::string, std::vector<std::shared_ptr<Species>>> speciesByElement_;
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_SPECIES_MANAGER_H
