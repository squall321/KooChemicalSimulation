/**
 * @file ReactionNetwork.h
 * @brief Chemical reaction network analysis
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha4
 * @date 2025-11-06
 *
 * Provides network analysis for chemical reaction mechanisms.
 */

#ifndef KOO_CHEMISTRY_REACTION_NETWORK_H
#define KOO_CHEMISTRY_REACTION_NETWORK_H

#include "Reaction.h"
#include "ReactionManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>

namespace koo {
namespace chemistry {

/**
 * @brief Reaction network analysis and management
 *
 * Provides functionality to:
 * - Analyze reaction pathways
 * - Identify intermediate species
 * - Detect reaction loops
 * - Calculate network statistics
 */
class ReactionNetwork {
public:
    /**
     * @brief Default constructor
     */
    ReactionNetwork() = default;

    /**
     * @brief Constructor with reaction manager
     */
    explicit ReactionNetwork(const ReactionManager& reactionManager)
        : reactionManager_(reactionManager) {
        buildNetwork();
    }

    /**
     * @brief Build network from reaction manager
     */
    void buildNetwork() {
        speciesGraph_.clear();
        reactionGraph_.clear();

        // Build species connectivity graph
        for (const auto& reaction : reactionManager_.getAllReactions()) {
            std::string rxnId = reaction->getId();

            // Add edges from reactants to products
            for (const auto& [reactant, coeff] : reaction->getReactants()) {
                for (const auto& [product, pcoeff] : reaction->getProducts()) {
                    if (reactant != product) {
                        speciesGraph_[reactant].insert(product);
                    }
                }
            }

            // Build reaction graph
            for (const auto& [species, coeff] : reaction->getReactants()) {
                reactionGraph_[species].push_back(rxnId);
            }
            for (const auto& [species, coeff] : reaction->getProducts()) {
                reactionGraph_[species].push_back(rxnId);
            }
        }
    }

    /**
     * @brief Get species that are only consumed (no production)
     * @return Set of consumed-only species
     */
    std::set<std::string> getPureReactants() const {
        std::set<std::string> pureReactants;

        for (const auto& reaction : reactionManager_.getAllReactions()) {
            for (const auto& [species, coeff] : reaction->getReactants()) {
                // Check if this species is ever produced
                bool isProduced = false;
                for (const auto& rxn : reactionManager_.getAllReactions()) {
                    if (rxn->getProducts().count(species) > 0) {
                        isProduced = true;
                        break;
                    }
                }
                if (!isProduced) {
                    pureReactants.insert(species);
                }
            }
        }

        return pureReactants;
    }

    /**
     * @brief Get species that are only produced (no consumption)
     * @return Set of produced-only species
     */
    std::set<std::string> getPureProducts() const {
        std::set<std::string> pureProducts;

        for (const auto& reaction : reactionManager_.getAllReactions()) {
            for (const auto& [species, coeff] : reaction->getProducts()) {
                // Check if this species is ever consumed
                bool isConsumed = false;
                for (const auto& rxn : reactionManager_.getAllReactions()) {
                    if (rxn->getReactants().count(species) > 0) {
                        isConsumed = true;
                        break;
                    }
                }
                if (!isConsumed) {
                    pureProducts.insert(species);
                }
            }
        }

        return pureProducts;
    }

    /**
     * @brief Get intermediate species (both produced and consumed)
     * @return Set of intermediate species
     */
    std::set<std::string> getIntermediates() const {
        std::set<std::string> intermediates;
        auto allSpecies = reactionManager_.getAllSpecies();

        for (const auto& species : allSpecies) {
            bool isProduced = false;
            bool isConsumed = false;

            for (const auto& rxn : reactionManager_.getAllReactions()) {
                if (rxn->getProducts().count(species) > 0) {
                    isProduced = true;
                }
                if (rxn->getReactants().count(species) > 0) {
                    isConsumed = true;
                }
                if (isProduced && isConsumed) {
                    break;
                }
            }

            if (isProduced && isConsumed) {
                intermediates.insert(species);
            }
        }

        return intermediates;
    }

    /**
     * @brief Get reactions that produce a given species
     * @param species Species name
     * @return Vector of reaction IDs
     */
    std::vector<std::string> getProducingReactions(const std::string& species) const {
        std::vector<std::string> producing;

        for (const auto& rxn : reactionManager_.getAllReactions()) {
            if (rxn->getProducts().count(species) > 0) {
                producing.push_back(rxn->getId());
            }
        }

        return producing;
    }

    /**
     * @brief Get reactions that consume a given species
     * @param species Species name
     * @return Vector of reaction IDs
     */
    std::vector<std::string> getConsumingReactions(const std::string& species) const {
        std::vector<std::string> consuming;

        for (const auto& rxn : reactionManager_.getAllReactions()) {
            if (rxn->getReactants().count(species) > 0) {
                consuming.push_back(rxn->getId());
            }
        }

        return consuming;
    }

    /**
     * @brief Check if species A can convert to species B through reactions
     * @param from Starting species
     * @param to Target species
     * @return True if pathway exists
     */
    bool hasPathway(const std::string& from, const std::string& to) const {
        if (from == to) return true;

        std::set<std::string> visited;
        return dfsSearch(from, to, visited);
    }

    /**
     * @brief Get network statistics
     * @return Map of statistic names to values
     */
    std::map<std::string, int> getNetworkStats() const {
        std::map<std::string, int> stats;

        stats["total_species"] = reactionManager_.getAllSpecies().size();
        stats["total_reactions"] = reactionManager_.getReactionCount();
        stats["pure_reactants"] = getPureReactants().size();
        stats["pure_products"] = getPureProducts().size();
        stats["intermediates"] = getIntermediates().size();

        // Count reversible reactions
        int reversible = 0;
        for (const auto& rxn : reactionManager_.getAllReactions()) {
            if (rxn->isReversible()) {
                reversible++;
            }
        }
        stats["reversible_reactions"] = reversible;
        stats["irreversible_reactions"] = stats["total_reactions"] - reversible;

        return stats;
    }

    /**
     * @brief Get network summary string
     */
    std::string getSummary() const {
        std::ostringstream oss;
        auto stats = getNetworkStats();

        oss << "Reaction Network Summary:\n";
        oss << "========================\n";
        oss << "Total species: " << stats["total_species"] << "\n";
        oss << "  - Pure reactants: " << stats["pure_reactants"] << "\n";
        oss << "  - Intermediates: " << stats["intermediates"] << "\n";
        oss << "  - Pure products: " << stats["pure_products"] << "\n";
        oss << "\n";
        oss << "Total reactions: " << stats["total_reactions"] << "\n";
        oss << "  - Reversible: " << stats["reversible_reactions"] << "\n";
        oss << "  - Irreversible: " << stats["irreversible_reactions"] << "\n";

        return oss.str();
    }

    /**
     * @brief Print detailed network analysis
     */
    void printNetworkAnalysis() const {
        std::cout << getSummary() << "\n";

        auto pureReactants = getPureReactants();
        if (!pureReactants.empty()) {
            std::cout << "Pure Reactants (consumed only):\n";
            for (const auto& sp : pureReactants) {
                std::cout << "  - " << sp << "\n";
            }
            std::cout << "\n";
        }

        auto intermediates = getIntermediates();
        if (!intermediates.empty()) {
            std::cout << "Intermediates (produced and consumed):\n";
            for (const auto& sp : intermediates) {
                auto producing = getProducingReactions(sp);
                auto consuming = getConsumingReactions(sp);
                std::cout << "  - " << sp << ": produced by " << producing.size()
                         << " rxn(s), consumed by " << consuming.size() << " rxn(s)\n";
            }
            std::cout << "\n";
        }

        auto pureProducts = getPureProducts();
        if (!pureProducts.empty()) {
            std::cout << "Pure Products (produced only):\n";
            for (const auto& sp : pureProducts) {
                std::cout << "  - " << sp << "\n";
            }
        }
    }

private:
    /**
     * @brief DFS search for pathway
     */
    bool dfsSearch(const std::string& current, const std::string& target,
                   std::set<std::string>& visited) const {
        if (current == target) return true;
        if (visited.count(current) > 0) return false;

        visited.insert(current);

        auto it = speciesGraph_.find(current);
        if (it != speciesGraph_.end()) {
            for (const auto& neighbor : it->second) {
                if (dfsSearch(neighbor, target, visited)) {
                    return true;
                }
            }
        }

        return false;
    }

    ReactionManager reactionManager_;                              ///< Reaction manager
    std::map<std::string, std::set<std::string>> speciesGraph_;   ///< Species connectivity graph
    std::map<std::string, std::vector<std::string>> reactionGraph_; ///< Species to reactions map
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_REACTION_NETWORK_H
