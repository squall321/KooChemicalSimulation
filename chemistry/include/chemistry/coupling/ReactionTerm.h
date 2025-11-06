/**
 * @file ReactionTerm.h
 * @brief Reaction source term for PDE systems
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-beta
 * @date 2025-11-06
 *
 * Converts chemical reactions to PDE source terms.
 */

#ifndef KOO_CHEMISTRY_REACTION_TERM_H
#define KOO_CHEMISTRY_REACTION_TERM_H

#include "ConcentrationField.h"
#include "chemistry/reaction/ReactionSystem.h"
#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace koo {
namespace chemistry {

/**
 * @brief Reaction source term for PDE coupling
 *
 * Calculates source terms from chemical reactions:
 * S_i = Σ_j (ν_ij × ROP_j)
 *
 * where S_i is the source term for species i,
 * ν_ij is stoichiometric coefficient, and
 * ROP_j is rate of progress for reaction j.
 */
class ReactionTerm {
public:
    /**
     * @brief Default constructor
     */
    ReactionTerm() : temperature_(300.0) {}

    /**
     * @brief Constructor with reaction system
     * @param reactionSystem Reaction system
     */
    explicit ReactionTerm(const ReactionSystem& reactionSystem)
        : reactionSystem_(reactionSystem), temperature_(300.0) {}

    /**
     * @brief Set reaction system
     * @param reactionSystem Reaction system
     */
    void setReactionSystem(const ReactionSystem& reactionSystem) {
        reactionSystem_ = reactionSystem;
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
     * @brief Calculate source terms for all species
     * @param field Concentration field
     * @return Vector of source terms (mol/(m³·s))
     */
    std::vector<double> calculateSourceTerms(const ConcentrationField& field) const {
        auto concMap = field.getConcentrationMap();
        auto sourceTerms = reactionSystem_.calculateAllSourceTerms(temperature_, concMap);

        std::vector<double> sources(field.getSpeciesCount(), 0.0);

        for (size_t i = 0; i < field.getSpeciesCount(); ++i) {
            std::string species = field.getSpeciesName(i);
            auto it = sourceTerms.find(species);
            if (it != sourceTerms.end()) {
                sources[i] = it->second.netRate;
            }
        }

        return sources;
    }

    /**
     * @brief Calculate source term for a specific species
     * @param field Concentration field
     * @param speciesIndex Species index in field
     * @return Source term (mol/(m³·s))
     */
    double calculateSourceTerm(const ConcentrationField& field, size_t speciesIndex) const {
        if (speciesIndex >= field.getSpeciesCount()) {
            throw std::out_of_range("Species index out of range");
        }

        std::string species = field.getSpeciesName(speciesIndex);
        auto concMap = field.getConcentrationMap();
        auto sourceTerm = reactionSystem_.calculateSourceTerm(species, temperature_, concMap);

        return sourceTerm.netRate;
    }

    /**
     * @brief Estimate Jacobian matrix numerically
     * @param field Concentration field
     * @param epsilon Perturbation size (default 1e-8)
     * @return Jacobian matrix J[i][j] = ∂S_i/∂C_j
     */
    std::vector<std::vector<double>> calculateJacobian(
        const ConcentrationField& field, double epsilon = 1.0e-8) const {

        size_t n = field.getSpeciesCount();
        std::vector<std::vector<double>> jacobian(n, std::vector<double>(n, 0.0));

        // Calculate baseline source terms
        auto S0 = calculateSourceTerms(field);

        // Perturb each concentration and calculate derivatives
        for (size_t j = 0; j < n; ++j) {
            ConcentrationField perturbedField = field;
            double C_j = perturbedField.getConcentration(j);
            double delta = epsilon * std::max(std::abs(C_j), 1.0);

            perturbedField.setConcentration(j, C_j + delta);
            auto S_perturbed = calculateSourceTerms(perturbedField);

            // ∂S_i/∂C_j ≈ (S_i(C_j + δ) - S_i(C_j)) / δ
            for (size_t i = 0; i < n; ++i) {
                jacobian[i][j] = (S_perturbed[i] - S0[i]) / delta;
            }
        }

        return jacobian;
    }

    /**
     * @brief Calculate system stiffness ratio
     * @param field Concentration field
     * @return Stiffness ratio (max eigenvalue / min eigenvalue estimate)
     */
    double estimateStiffness(const ConcentrationField& field) const {
        auto jacobian = calculateJacobian(field);
        size_t n = jacobian.size();

        // Simple estimate: max/min diagonal elements
        double maxDiag = 0.0;
        double minDiag = 1.0e100;

        for (size_t i = 0; i < n; ++i) {
            double diag = std::abs(jacobian[i][i]);
            if (diag > 1.0e-12) {  // Avoid zeros
                maxDiag = std::max(maxDiag, diag);
                minDiag = std::min(minDiag, diag);
            }
        }

        if (minDiag > 0.0) {
            return maxDiag / minDiag;
        }
        return 1.0;
    }

    /**
     * @brief Check if system is stiff
     * @param field Concentration field
     * @param threshold Stiffness threshold (default 1000)
     * @return True if stiff
     */
    bool isStiff(const ConcentrationField& field, double threshold = 1000.0) const {
        return estimateStiffness(field) > threshold;
    }

    /**
     * @brief Get info string
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "ReactionTerm:\n";
        oss << "  Temperature: " << temperature_ << " K\n";
        oss << "  Reaction system: " << reactionSystem_.getReactionManager().getReactionCount()
            << " reactions\n";
        return oss.str();
    }

private:
    ReactionSystem reactionSystem_;    ///< Reaction system
    double temperature_;               ///< Temperature (K)
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_REACTION_TERM_H
