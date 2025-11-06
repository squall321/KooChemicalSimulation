/**
 * @file ReactionManager.h
 * @brief Chemical reaction mechanism management
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha2
 * @date 2025-11-06
 *
 * Manages collections of chemical reactions and calculates species production rates.
 */

#ifndef KOO_CHEMISTRY_REACTION_MANAGER_H
#define KOO_CHEMISTRY_REACTION_MANAGER_H

#include "Reaction.h"
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>
#include <iostream>

namespace koo {
namespace chemistry {

/**
 * @brief Manages a collection of chemical reactions
 *
 * Provides functionality to:
 * - Add/remove reactions
 * - Query reactions by species involvement
 * - Calculate production/destruction rates for all species
 */
class ReactionManager {
public:
    /**
     * @brief Default constructor
     */
    ReactionManager() = default;

    /**
     * @brief Add a reaction to the mechanism
     * @param reaction Reaction to add
     * @throws std::invalid_argument if reaction ID already exists
     */
    void addReaction(const Reaction& reaction) {
        std::string id = reaction.getId();
        if (reactionsByName_.count(id) > 0) {
            throw std::invalid_argument("Reaction '" + id + "' already exists");
        }

        auto reactionPtr = std::make_shared<Reaction>(reaction);
        reactionsByName_[id] = reactionPtr;
        reactionsList_.push_back(reactionPtr);

        // Index by species involvement
        for (const auto& [species, coeff] : reaction.getReactants()) {
            reactionsBySpecies_[species].push_back(reactionPtr);
        }
        for (const auto& [species, coeff] : reaction.getProducts()) {
            reactionsBySpecies_[species].push_back(reactionPtr);
        }
    }

    /**
     * @brief Remove a reaction by ID
     * @param id Reaction ID
     * @return True if reaction was removed, false if not found
     */
    bool removeReaction(const std::string& id) {
        auto it = reactionsByName_.find(id);
        if (it == reactionsByName_.end()) {
            return false;
        }

        auto reactionPtr = it->second;

        // Remove from name index
        reactionsByName_.erase(it);

        // Remove from list
        reactionsList_.erase(
            std::remove(reactionsList_.begin(), reactionsList_.end(), reactionPtr),
            reactionsList_.end()
        );

        // Remove from species index
        for (auto& [species, reactions] : reactionsBySpecies_) {
            reactions.erase(
                std::remove(reactions.begin(), reactions.end(), reactionPtr),
                reactions.end()
            );
        }

        return true;
    }

    /**
     * @brief Clear all reactions
     */
    void clear() {
        reactionsByName_.clear();
        reactionsList_.clear();
        reactionsBySpecies_.clear();
    }

    /**
     * @brief Get a reaction by ID
     * @param id Reaction ID
     * @return Shared pointer to reaction, or nullptr if not found
     */
    std::shared_ptr<Reaction> getReaction(const std::string& id) const {
        auto it = reactionsByName_.find(id);
        if (it != reactionsByName_.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief Check if reaction exists
     * @param id Reaction ID
     * @return True if reaction exists
     */
    bool hasReaction(const std::string& id) const {
        return reactionsByName_.count(id) > 0;
    }

    /**
     * @brief Get all reactions
     * @return Vector of all reactions
     */
    const std::vector<std::shared_ptr<Reaction>>& getAllReactions() const {
        return reactionsList_;
    }

    /**
     * @brief Get reactions involving a specific species
     * @param species Species name
     * @return Vector of reactions involving the species
     */
    std::vector<std::shared_ptr<Reaction>> getReactionsBySpecies(const std::string& species) const {
        auto it = reactionsBySpecies_.find(species);
        if (it != reactionsBySpecies_.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Get number of reactions
     * @return Number of reactions in mechanism
     */
    size_t getReactionCount() const {
        return reactionsList_.size();
    }

    /**
     * @brief Get all species involved in the mechanism
     * @return Set of species names
     */
    std::set<std::string> getAllSpecies() const {
        std::set<std::string> species;
        for (const auto& reaction : reactionsList_) {
            for (const auto& [sp, coeff] : reaction->getReactants()) {
                species.insert(sp);
            }
            for (const auto& [sp, coeff] : reaction->getProducts()) {
                species.insert(sp);
            }
        }
        return species;
    }

    /**
     * @brief Calculate production rate for a specific species
     * @param species Species name
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Net production rate (mol/(m³·s))
     */
    double getProductionRate(const std::string& species, double T,
                            const std::map<std::string, double>& concentrations) const {
        double rate = 0.0;

        for (const auto& reaction : reactionsList_) {
            if (reaction->hasSpecies(species)) {
                double rop = reaction->getNetRateOfProgress(T, concentrations);
                double stoich = reaction->getStoichiometry(species);
                rate += stoich * rop;
            }
        }

        return rate;
    }

    /**
     * @brief Calculate production rates for all species
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Map of species to production rates (mol/(m³·s))
     */
    std::map<std::string, double> getAllProductionRates(double T,
                                                         const std::map<std::string, double>& concentrations) const {
        std::map<std::string, double> rates;

        // Initialize all species to zero
        for (const auto& species : getAllSpecies()) {
            rates[species] = 0.0;
        }

        // Sum contributions from all reactions
        for (const auto& reaction : reactionsList_) {
            double rop = reaction->getNetRateOfProgress(T, concentrations);

            for (const auto& [species, coeff] : reaction->getReactants()) {
                rates[species] -= coeff * rop;
            }
            for (const auto& [species, coeff] : reaction->getProducts()) {
                rates[species] += coeff * rop;
            }
        }

        return rates;
    }

    /**
     * @brief Calculate all rates of progress
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Map of reaction IDs to rates of progress (mol/(m³·s))
     */
    std::map<std::string, double> getAllRatesOfProgress(double T,
                                                         const std::map<std::string, double>& concentrations) const {
        std::map<std::string, double> rops;

        for (const auto& reaction : reactionsList_) {
            rops[reaction->getId()] = reaction->getNetRateOfProgress(T, concentrations);
        }

        return rops;
    }

    /**
     * @brief Get summary string
     * @return Summary of reaction mechanism
     */
    std::string getSummary() const {
        std::ostringstream oss;
        oss << "Reaction Mechanism Summary:\n";
        oss << "  Total reactions: " << reactionsList_.size() << "\n";
        oss << "  Total species: " << getAllSpecies().size() << "\n";

        // Count reaction types
        std::map<ReactionType, int> typeCounts;
        for (const auto& reaction : reactionsList_) {
            typeCounts[reaction->getType()]++;
        }

        if (!typeCounts.empty()) {
            oss << "  Reaction types:\n";
            for (const auto& [type, count] : typeCounts) {
                oss << "    ";
                switch (type) {
                    case ReactionType::ELEMENTARY: oss << "Elementary"; break;
                    case ReactionType::THREE_BODY: oss << "Three-body"; break;
                    case ReactionType::FALLOFF: oss << "Falloff"; break;
                    case ReactionType::REVERSIBLE: oss << "Reversible"; break;
                    case ReactionType::IRREVERSIBLE: oss << "Irreversible"; break;
                }
                oss << ": " << count << "\n";
            }
        }

        return oss.str();
    }

    /**
     * @brief Print all reactions
     */
    void printAllReactions() const {
        std::cout << "All Reactions (" << reactionsList_.size() << "):\n";
        std::cout << "=====================================\n";
        for (const auto& reaction : reactionsList_) {
            std::cout << reaction->getInfo() << "\n";
            std::cout << "-------------------------------------\n";
        }
    }

    /**
     * @brief Load common hydrogen-oxygen mechanism
     *
     * Loads a simplified H2-O2 combustion mechanism:
     * 1. H2 + O2 <=> 2 OH
     * 2. OH + H2 <=> H2O + H
     * 3. H + O2 <=> OH + O
     * 4. O + H2 <=> OH + H
     */
    void loadH2O2Mechanism() {
        // Reaction 1: H2 + O2 <=> 2 OH
        Reaction r1("R1", ReactionType::ELEMENTARY, true);
        r1.addReactant("H2", 1.0);
        r1.addReactant("O2", 1.0);
        r1.addProduct("OH", 2.0);

        RateLaw r1_forward;
        r1_forward.A = 1.7e13;
        r1_forward.beta = 0.0;
        r1_forward.Ea = 200000.0;  // 200 kJ/mol
        r1.setForwardRateLaw(r1_forward);

        RateLaw r1_reverse;
        r1_reverse.A = 3.5e12;
        r1_reverse.beta = 0.0;
        r1_reverse.Ea = 50000.0;  // 50 kJ/mol
        r1.setReverseRateLaw(r1_reverse);

        addReaction(r1);

        // Reaction 2: OH + H2 <=> H2O + H
        Reaction r2("R2", ReactionType::ELEMENTARY, true);
        r2.addReactant("OH", 1.0);
        r2.addReactant("H2", 1.0);
        r2.addProduct("H2O", 1.0);
        r2.addProduct("H", 1.0);

        RateLaw r2_forward;
        r2_forward.A = 1.0e14;
        r2_forward.beta = 0.0;
        r2_forward.Ea = 30000.0;  // 30 kJ/mol
        r2.setForwardRateLaw(r2_forward);

        RateLaw r2_reverse;
        r2_reverse.A = 4.0e13;
        r2_reverse.beta = 0.0;
        r2_reverse.Ea = 80000.0;  // 80 kJ/mol
        r2.setReverseRateLaw(r2_reverse);

        addReaction(r2);

        // Reaction 3: H + O2 <=> OH + O
        Reaction r3("R3", ReactionType::ELEMENTARY, true);
        r3.addReactant("H", 1.0);
        r3.addReactant("O2", 1.0);
        r3.addProduct("OH", 1.0);
        r3.addProduct("O", 1.0);

        RateLaw r3_forward;
        r3_forward.A = 2.0e14;
        r3_forward.beta = 0.0;
        r3_forward.Ea = 70000.0;  // 70 kJ/mol
        r3.setForwardRateLaw(r3_forward);

        RateLaw r3_reverse;
        r3_reverse.A = 1.8e13;
        r3_reverse.beta = 0.0;
        r3_reverse.Ea = 15000.0;  // 15 kJ/mol
        r3.setReverseRateLaw(r3_reverse);

        addReaction(r3);

        // Reaction 4: O + H2 <=> OH + H
        Reaction r4("R4", ReactionType::ELEMENTARY, true);
        r4.addReactant("O", 1.0);
        r4.addReactant("H2", 1.0);
        r4.addProduct("OH", 1.0);
        r4.addProduct("H", 1.0);

        RateLaw r4_forward;
        r4_forward.A = 5.0e13;
        r4_forward.beta = 0.0;
        r4_forward.Ea = 40000.0;  // 40 kJ/mol
        r4.setForwardRateLaw(r4_forward);

        RateLaw r4_reverse;
        r4_reverse.A = 2.2e13;
        r4_reverse.beta = 0.0;
        r4_reverse.Ea = 25000.0;  // 25 kJ/mol
        r4.setReverseRateLaw(r4_reverse);

        addReaction(r4);
    }

private:
    std::map<std::string, std::shared_ptr<Reaction>> reactionsByName_;  ///< Reactions indexed by ID
    std::vector<std::shared_ptr<Reaction>> reactionsList_;              ///< All reactions in order
    std::map<std::string, std::vector<std::shared_ptr<Reaction>>> reactionsBySpecies_;  ///< Reactions indexed by species
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_REACTION_MANAGER_H
