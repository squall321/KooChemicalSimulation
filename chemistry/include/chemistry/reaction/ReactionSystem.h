/**
 * @file ReactionSystem.h
 * @brief Complete reaction system integration
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha4
 * @date 2025-11-06
 *
 * Provides high-level interface for managing complete reaction systems.
 */

#ifndef KOO_CHEMISTRY_REACTION_SYSTEM_H
#define KOO_CHEMISTRY_REACTION_SYSTEM_H

#include "Reaction.h"
#include "ReactionManager.h"
#include "ReactionNetwork.h"
#include "chemistry/species/Species.h"
#include "chemistry/species/SpeciesManager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>

namespace koo {
namespace chemistry {

/**
 * @brief Source/sink term for a species
 *
 * Represents the net production/consumption rate from all reactions
 */
struct SourceTerm {
    std::string species;                  ///< Species name
    double productionRate{0.0};           ///< Total production rate (mol/(m³·s))
    double consumptionRate{0.0};          ///< Total consumption rate (mol/(m³·s))
    double netRate{0.0};                  ///< Net rate (production - consumption)

    std::vector<std::string> producingReactions;   ///< Reactions producing this species
    std::vector<std::string> consumingReactions;   ///< Reactions consuming this species

    /**
     * @brief Get info string
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "Species: " << species << "\n";
        oss << "  Production rate: " << productionRate << " mol/(m³·s)\n";
        oss << "  Consumption rate: " << consumptionRate << " mol/(m³·s)\n";
        oss << "  Net rate: " << netRate << " mol/(m³·s)\n";
        oss << "  Produced by: " << producingReactions.size() << " reaction(s)\n";
        oss << "  Consumed by: " << consumingReactions.size() << " reaction(s)";
        return oss.str();
    }
};

/**
 * @brief Complete chemical reaction system
 *
 * Integrates:
 * - Species database (SpeciesManager)
 * - Reaction mechanism (ReactionManager)
 * - Network analysis (ReactionNetwork)
 * - Source term calculations
 */
class ReactionSystem {
public:
    /**
     * @brief Default constructor
     */
    ReactionSystem() = default;

    /**
     * @brief Constructor with managers
     */
    ReactionSystem(const SpeciesManager& speciesManager, const ReactionManager& reactionManager)
        : speciesManager_(speciesManager), reactionManager_(reactionManager),
          network_(reactionManager) {}

    // Getters
    const SpeciesManager& getSpeciesManager() const { return speciesManager_; }
    const ReactionManager& getReactionManager() const { return reactionManager_; }
    const ReactionNetwork& getNetwork() const { return network_; }

    SpeciesManager& getSpeciesManager() { return speciesManager_; }
    ReactionManager& getReactionManager() { return reactionManager_; }
    ReactionNetwork& getNetwork() { return network_; }

    /**
     * @brief Rebuild network (call after adding/removing reactions)
     */
    void rebuildNetwork() {
        network_ = ReactionNetwork(reactionManager_);
    }

    /**
     * @brief Calculate source term for a specific species
     * @param species Species name
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Source term data
     */
    SourceTerm calculateSourceTerm(const std::string& species, double T,
                                   const std::map<std::string, double>& concentrations) const {
        SourceTerm term;
        term.species = species;

        // Get producing reactions
        term.producingReactions = network_.getProducingReactions(species);

        // Get consuming reactions
        term.consumingReactions = network_.getConsumingReactions(species);

        // Calculate production rate
        for (const auto& rxnId : term.producingReactions) {
            auto rxn = reactionManager_.getReaction(rxnId);
            if (rxn) {
                double rop = rxn->getNetRateOfProgress(T, concentrations);
                double stoich = rxn->getStoichiometry(species);
                if (stoich > 0) {  // Production
                    term.productionRate += stoich * rop;
                }
            }
        }

        // Calculate consumption rate
        for (const auto& rxnId : term.consumingReactions) {
            auto rxn = reactionManager_.getReaction(rxnId);
            if (rxn) {
                double rop = rxn->getNetRateOfProgress(T, concentrations);
                double stoich = rxn->getStoichiometry(species);
                if (stoich < 0) {  // Consumption
                    term.consumptionRate += -stoich * rop;  // Make positive
                }
            }
        }

        term.netRate = term.productionRate - term.consumptionRate;

        return term;
    }

    /**
     * @brief Calculate source terms for all species
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Map of species to source terms
     */
    std::map<std::string, SourceTerm> calculateAllSourceTerms(
        double T, const std::map<std::string, double>& concentrations) const {

        std::map<std::string, SourceTerm> sourceTerms;

        for (const auto& species : reactionManager_.getAllSpecies()) {
            sourceTerms[species] = calculateSourceTerm(species, T, concentrations);
        }

        return sourceTerms;
    }

    /**
     * @brief Get system statistics
     */
    std::map<std::string, int> getSystemStats() const {
        auto stats = network_.getNetworkStats();

        // Add species count by phase if available
        int gasSpecies = 0;
        int liquidSpecies = 0;
        int solidSpecies = 0;

        for (const auto& speciesName : reactionManager_.getAllSpecies()) {
            auto species = speciesManager_.getSpecies(speciesName);
            if (species) {
                switch (species->getPhase()) {
                    case PhaseType::GAS:
                        gasSpecies++;
                        break;
                    case PhaseType::LIQUID:
                        liquidSpecies++;
                        break;
                    case PhaseType::SOLID:
                        solidSpecies++;
                        break;
                    default:
                        break;
                }
            }
        }

        stats["gas_species"] = gasSpecies;
        stats["liquid_species"] = liquidSpecies;
        stats["solid_species"] = solidSpecies;

        return stats;
    }

    /**
     * @brief Get system summary
     */
    std::string getSummary() const {
        std::ostringstream oss;
        auto stats = getSystemStats();

        oss << "Reaction System Summary:\n";
        oss << "========================\n";
        oss << "Species: " << stats["total_species"] << " total\n";
        if (stats["gas_species"] > 0) {
            oss << "  - Gas: " << stats["gas_species"] << "\n";
        }
        if (stats["liquid_species"] > 0) {
            oss << "  - Liquid: " << stats["liquid_species"] << "\n";
        }
        if (stats["solid_species"] > 0) {
            oss << "  - Solid: " << stats["solid_species"] << "\n";
        }
        oss << "\n";
        oss << "Reactions: " << stats["total_reactions"] << " total\n";
        oss << "  - Reversible: " << stats["reversible_reactions"] << "\n";
        oss << "  - Irreversible: " << stats["irreversible_reactions"] << "\n";
        oss << "\n";
        oss << "Network:\n";
        oss << "  - Pure reactants: " << stats["pure_reactants"] << "\n";
        oss << "  - Intermediates: " << stats["intermediates"] << "\n";
        oss << "  - Pure products: " << stats["pure_products"] << "\n";

        return oss.str();
    }

    /**
     * @brief Print complete system analysis
     */
    void printSystemAnalysis() const {
        std::cout << getSummary() << "\n";
        std::cout << "========================\n\n";
        network_.printNetworkAnalysis();
    }

    /**
     * @brief Validate system consistency
     * @return True if system is valid
     */
    bool validate() const {
        // Check that all species in reactions exist in species manager
        for (const auto& reaction : reactionManager_.getAllReactions()) {
            for (const auto& [species, coeff] : reaction->getReactants()) {
                if (!speciesManager_.hasSpecies(species)) {
                    std::cerr << "Error: Species '" << species
                             << "' in reaction '" << reaction->getId()
                             << "' not found in species manager\n";
                    return false;
                }
            }
            for (const auto& [species, coeff] : reaction->getProducts()) {
                if (!speciesManager_.hasSpecies(species)) {
                    std::cerr << "Error: Species '" << species
                             << "' in reaction '" << reaction->getId()
                             << "' not found in species manager\n";
                    return false;
                }
            }
        }

        // Check for isolated species (not in any reaction)
        auto allSpeciesInMgr = speciesManager_.getAllSpecies();
        auto allSpeciesInRxn = reactionManager_.getAllSpecies();

        // This is just a warning, not an error
        bool hasIsolated = false;
        for (const auto& spPtr : allSpeciesInMgr) {
            if (allSpeciesInRxn.count(spPtr->getName()) == 0) {
                if (!hasIsolated) {
                    std::cout << "Warning: Isolated species (not in any reaction):\n";
                    hasIsolated = true;
                }
                std::cout << "  - " << spPtr->getName() << "\n";
            }
        }

        return true;
    }

    /**
     * @brief Get species involved in fast reactions
     * @param T Temperature (K)
     * @param threshold Rate constant threshold
     * @return Set of species names
     */
    std::set<std::string> getFastReactionSpecies(double T, double threshold = 1.0e10) const {
        std::set<std::string> fastSpecies;

        for (const auto& reaction : reactionManager_.getAllReactions()) {
            double kf = reaction->getForwardRateConstant(T);
            if (kf > threshold) {
                // Add all species involved in this fast reaction
                for (const auto& [species, coeff] : reaction->getReactants()) {
                    fastSpecies.insert(species);
                }
                for (const auto& [species, coeff] : reaction->getProducts()) {
                    fastSpecies.insert(species);
                }
            }
        }

        return fastSpecies;
    }

private:
    SpeciesManager speciesManager_;       ///< Species database
    ReactionManager reactionManager_;     ///< Reaction mechanism
    ReactionNetwork network_;             ///< Network analysis
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_REACTION_SYSTEM_H
