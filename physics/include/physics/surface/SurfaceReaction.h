#ifndef KOO_SURFACE_REACTION_H
#define KOO_SURFACE_REACTION_H

#include "SurfaceSpecies.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace physics {
namespace surface {

/**
 * @enum ReactionMechanism
 * @brief Surface reaction mechanisms
 */
enum class ReactionMechanism {
    LANGMUIR_HINSHELWOOD, ///< LH: A(ads) + B(ads) → products
    ELEY_RIDEAL,          ///< ER: A(ads) + B(gas) → products
    DISSOCIATIVE,         ///< AB(gas) → A(ads) + B(ads)
    ASSOCIATIVE           ///< A(ads) + B(ads) → AB(gas)
};

/**
 * @class SurfaceReaction
 * @brief Represents a surface chemical reaction
 *
 * Phase 26: Surface Reactions
 *
 * Reaction mechanisms:
 * 1. Langmuir-Hinshelwood (LH):
 *    - Both reactants adsorbed
 *    - Rate ∝ θ_A × θ_B
 *
 * 2. Eley-Rideal (ER):
 *    - One reactant adsorbed, one from gas phase
 *    - Rate ∝ θ_A × P_B
 *
 * 3. Dissociative adsorption:
 *    - Molecule dissociates upon adsorption
 *    - AB(g) + 2* → A* + B*
 *
 * 4. Associative desorption:
 *    - Two adsorbates combine and desorb
 *    - A* + B* → AB(g) + 2*
 */
class SurfaceReaction {
public:
    /**
     * @brief Constructor
     * @param name Reaction name
     * @param mechanism Reaction mechanism
     */
    SurfaceReaction(const std::string& name, ReactionMechanism mechanism)
        : name_(name),
          mechanism_(mechanism),
          activationEnergy_(0.0),
          preExponential_(1.0e13),
          coverageDependence_(0.0) {}

    /**
     * @brief Add reactant
     */
    void addReactant(const std::string& species, int stoichiometry = 1) {
        reactants_[species] = stoichiometry;
    }

    /**
     * @brief Add product
     */
    void addProduct(const std::string& species, int stoichiometry = 1) {
        products_[species] = stoichiometry;
    }

    /**
     * @brief Set kinetic parameters
     */
    void setKineticParameters(double activationEnergy, double preExponential) {
        activationEnergy_ = activationEnergy;
        preExponential_ = preExponential;
    }

    /**
     * @brief Set coverage dependence parameter
     *
     * Modified activation energy: E_a(θ) = E_a0 + α×θ
     */
    void setCoverageDependence(double alpha) {
        coverageDependence_ = alpha;
    }

    /**
     * @brief Calculate rate constant
     *
     * k = A × exp(-E_a / RT)
     */
    double calculateRateConstant(double temperature, double coverage = 0.0) const {
        const double R = 8.314; // J/(mol·K)

        // Coverage-dependent activation energy
        double E_a = activationEnergy_ + coverageDependence_ * coverage;

        return preExponential_ * std::exp(-E_a / (R * temperature));
    }

    /**
     * @brief Calculate reaction rate for Langmuir-Hinshelwood mechanism
     *
     * r = k × θ_A × θ_B
     */
    double calculateLHRate(double temperature,
                          const std::map<std::string, double>& coverages) const {
        if (mechanism_ != ReactionMechanism::LANGMUIR_HINSHELWOOD) {
            throw std::runtime_error("Not a Langmuir-Hinshelwood reaction");
        }

        double k = calculateRateConstant(temperature);

        // Multiply coverages of all reactants
        double rate = k;
        for (const auto& reactant : reactants_) {
            auto it = coverages.find(reactant.first);
            if (it == coverages.end()) {
                throw std::runtime_error("Coverage not found: " + reactant.first);
            }
            rate *= std::pow(it->second, reactant.second);
        }

        return rate;
    }

    /**
     * @brief Calculate reaction rate for Eley-Rideal mechanism
     *
     * r = k × θ_A × P_B
     */
    double calculateERRate(double temperature,
                          const std::map<std::string, double>& coverages,
                          const std::map<std::string, double>& pressures) const {
        if (mechanism_ != ReactionMechanism::ELEY_RIDEAL) {
            throw std::runtime_error("Not an Eley-Rideal reaction");
        }

        double k = calculateRateConstant(temperature);
        double rate = k;

        // Multiply by adsorbed species coverages
        for (const auto& reactant : reactants_) {
            auto cov_it = coverages.find(reactant.first);
            if (cov_it != coverages.end()) {
                rate *= std::pow(cov_it->second, reactant.second);
            } else {
                // Gas phase species
                auto press_it = pressures.find(reactant.first);
                if (press_it == pressures.end()) {
                    throw std::runtime_error("Pressure not found: " + reactant.first);
                }
                rate *= std::pow(press_it->second, reactant.second);
            }
        }

        return rate;
    }

    /**
     * @brief Calculate rate for dissociative adsorption
     *
     * r = k × P_AB × (1-θ)²
     */
    double calculateDissociativeRate(double temperature,
                                     double pressure,
                                     double totalCoverage) const {
        if (mechanism_ != ReactionMechanism::DISSOCIATIVE) {
            throw std::runtime_error("Not a dissociative adsorption");
        }

        double k = calculateRateConstant(temperature);

        // Need two vacant sites
        double vacantFraction = 1.0 - totalCoverage;
        if (vacantFraction < 0.0) vacantFraction = 0.0;

        return k * pressure * vacantFraction * vacantFraction;
    }

    /**
     * @brief Calculate rate for associative desorption
     *
     * r = k × θ_A × θ_B
     */
    double calculateAssociativeRate(double temperature,
                                    const std::map<std::string, double>& coverages) const {
        if (mechanism_ != ReactionMechanism::ASSOCIATIVE) {
            throw std::runtime_error("Not an associative desorption");
        }

        // Same as LH mechanism
        double k = calculateRateConstant(temperature);
        double rate = k;

        for (const auto& reactant : reactants_) {
            auto it = coverages.find(reactant.first);
            if (it == coverages.end()) {
                throw std::runtime_error("Coverage not found: " + reactant.first);
            }
            rate *= std::pow(it->second, reactant.second);
        }

        return rate;
    }

    // Getters
    std::string getName() const { return name_; }
    ReactionMechanism getMechanism() const { return mechanism_; }
    double getActivationEnergy() const { return activationEnergy_; }
    double getPreExponential() const { return preExponential_; }
    const std::map<std::string, int>& getReactants() const { return reactants_; }
    const std::map<std::string, int>& getProducts() const { return products_; }

    /**
     * @brief Get mechanism name
     */
    std::string getMechanismName() const {
        switch (mechanism_) {
            case ReactionMechanism::LANGMUIR_HINSHELWOOD:
                return "Langmuir-Hinshelwood";
            case ReactionMechanism::ELEY_RIDEAL:
                return "Eley-Rideal";
            case ReactionMechanism::DISSOCIATIVE:
                return "Dissociative";
            case ReactionMechanism::ASSOCIATIVE:
                return "Associative";
            default:
                return "Unknown";
        }
    }

private:
    std::string name_;
    ReactionMechanism mechanism_;
    std::map<std::string, int> reactants_;  ///< Species name → stoichiometry
    std::map<std::string, int> products_;   ///< Species name → stoichiometry
    double activationEnergy_;               ///< Activation energy (J/mol)
    double preExponential_;                 ///< Pre-exponential factor (1/s or m²/(mol·s))
    double coverageDependence_;             ///< Coverage dependence α (J/mol)
};

/**
 * @class CatalyticCycle
 * @brief Represents a catalytic reaction cycle
 *
 * Phase 26: Surface Reactions
 *
 * Example: CO oxidation on Pt
 * 1. O2(g) + 2* → 2O*        (dissociative adsorption)
 * 2. CO(g) + * → CO*         (adsorption)
 * 3. CO* + O* → CO2(g) + 2*  (LH reaction)
 */
class CatalyticCycle {
public:
    /**
     * @brief Constructor
     * @param name Cycle name
     */
    explicit CatalyticCycle(const std::string& name)
        : name_(name) {}

    /**
     * @brief Add reaction step
     */
    void addReaction(std::shared_ptr<SurfaceReaction> reaction) {
        reactions_.push_back(reaction);
    }

    /**
     * @brief Get number of steps
     */
    size_t getNumberOfSteps() const { return reactions_.size(); }

    /**
     * @brief Get reaction at step
     */
    std::shared_ptr<SurfaceReaction> getReaction(size_t step) const {
        if (step >= reactions_.size()) {
            throw std::out_of_range("Step index out of range");
        }
        return reactions_[step];
    }

    /**
     * @brief Identify rate-limiting step
     *
     * Finds the step with lowest rate
     */
    size_t findRateLimitingStep(double temperature,
                                const std::map<std::string, double>& coverages) const {
        if (reactions_.empty()) return 0;

        size_t rls = 0;
        double minRate = 1.0e100;

        for (size_t i = 0; i < reactions_.size(); ++i) {
            auto& rxn = reactions_[i];
            double rate = 0.0;

            // Calculate rate based on mechanism
            if (rxn->getMechanism() == ReactionMechanism::LANGMUIR_HINSHELWOOD ||
                rxn->getMechanism() == ReactionMechanism::ASSOCIATIVE) {
                rate = rxn->calculateLHRate(temperature, coverages);
            }

            if (rate < minRate) {
                minRate = rate;
                rls = i;
            }
        }

        return rls;
    }

    /**
     * @brief Calculate turnover frequency (TOF)
     *
     * TOF = rate of product formation per active site
     */
    double calculateTOF(double temperature,
                       const std::map<std::string, double>& coverages) const {
        // For simple case, use rate of slowest step
        size_t rls = findRateLimitingStep(temperature, coverages);
        auto& rxn = reactions_[rls];

        if (rxn->getMechanism() == ReactionMechanism::LANGMUIR_HINSHELWOOD ||
            rxn->getMechanism() == ReactionMechanism::ASSOCIATIVE) {
            return rxn->calculateLHRate(temperature, coverages);
        }

        return 0.0;
    }

    std::string getName() const { return name_; }

private:
    std::string name_;
    std::vector<std::shared_ptr<SurfaceReaction>> reactions_;
};

/**
 * @class MicrokineticsModel
 * @brief Mean-field microkinetics model
 *
 * Phase 26: Surface Reactions
 *
 * Solves coupled ODEs for surface coverages:
 * dθ_i/dt = Σ(r_j × ν_ij)
 *
 * where:
 * - θ_i: coverage of species i
 * - r_j: rate of reaction j
 * - ν_ij: stoichiometric coefficient
 */
class MicrokineticsModel {
public:
    /**
     * @brief Constructor
     */
    MicrokineticsModel() = default;

    /**
     * @brief Add reaction
     */
    void addReaction(std::shared_ptr<SurfaceReaction> reaction) {
        reactions_.push_back(reaction);
    }

    /**
     * @brief Calculate coverage time derivatives
     *
     * dθ/dt = Σ(ν × r)
     */
    std::map<std::string, double> calculateDerivatives(
        double temperature,
        const std::map<std::string, double>& coverages,
        const std::map<std::string, double>& pressures = {}) const {

        std::map<std::string, double> derivatives;

        // Initialize all derivatives to zero
        for (const auto& pair : coverages) {
            derivatives[pair.first] = 0.0;
        }

        // Sum contributions from all reactions
        for (const auto& rxn : reactions_) {
            double rate = 0.0;

            // Calculate rate based on mechanism
            switch (rxn->getMechanism()) {
                case ReactionMechanism::LANGMUIR_HINSHELWOOD:
                case ReactionMechanism::ASSOCIATIVE:
                    rate = rxn->calculateLHRate(temperature, coverages);
                    break;
                case ReactionMechanism::ELEY_RIDEAL:
                    rate = rxn->calculateERRate(temperature, coverages, pressures);
                    break;
                default:
                    continue;
            }

            // Update derivatives for reactants (negative) and products (positive)
            for (const auto& reactant : rxn->getReactants()) {
                derivatives[reactant.first] -= reactant.second * rate;
            }

            for (const auto& product : rxn->getProducts()) {
                derivatives[product.first] += product.second * rate;
            }
        }

        return derivatives;
    }

    /**
     * @brief Find steady state coverages
     *
     * Solves dθ/dt = 0
     * (Simple fixed-point iteration)
     */
    std::map<std::string, double> findSteadyState(
        double temperature,
        std::map<std::string, double> initialCoverages,
        const std::map<std::string, double>& pressures = {},
        double tolerance = 1.0e-6,
        int maxIterations = 1000) const {

        auto coverages = initialCoverages;

        for (int iter = 0; iter < maxIterations; ++iter) {
            auto derivatives = calculateDerivatives(temperature, coverages, pressures);

            // Simple Euler step with small dt
            double dt = 1.0e-3;
            double maxChange = 0.0;

            for (auto& pair : coverages) {
                double dtheta = derivatives[pair.first] * dt;
                pair.second += dtheta;

                // Keep in [0,1]
                if (pair.second < 0.0) pair.second = 0.0;
                if (pair.second > 1.0) pair.second = 1.0;

                maxChange = std::max(maxChange, std::abs(dtheta));
            }

            if (maxChange < tolerance) break;
        }

        return coverages;
    }

    size_t getNumberOfReactions() const { return reactions_.size(); }

private:
    std::vector<std::shared_ptr<SurfaceReaction>> reactions_;
};

} // namespace surface
} // namespace physics
} // namespace koo

#endif // KOO_SURFACE_REACTION_H
