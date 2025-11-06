/**
 * @file Reaction.h
 * @brief Chemical reaction representation with kinetics
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha2
 * @date 2025-11-06
 *
 * Defines chemical reactions with rate laws and thermodynamic consistency.
 */

#ifndef KOO_CHEMISTRY_REACTION_H
#define KOO_CHEMISTRY_REACTION_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <sstream>

namespace koo {
namespace chemistry {

// Forward declaration
class Species;

/**
 * @brief Type of chemical reaction
 */
enum class ReactionType {
    ELEMENTARY,      ///< Elementary reaction
    THREE_BODY,      ///< Three-body reaction
    FALLOFF,         ///< Pressure-dependent falloff
    REVERSIBLE,      ///< Reversible reaction
    IRREVERSIBLE     ///< Irreversible reaction
};

/**
 * @brief Arrhenius rate law parameters
 *
 * Rate constant: k = A * T^beta * exp(-Ea/(R*T))
 */
struct RateLaw {
    double A{0.0};        ///< Pre-exponential factor
    double beta{0.0};     ///< Temperature exponent
    double Ea{0.0};       ///< Activation energy (J/mol)

    /**
     * @brief Calculate rate constant at given temperature
     * @param T Temperature (K)
     * @return Rate constant
     */
    double getRateConstant(double T) const {
        const double R = 8.314;  // J/(mol·K)
        return A * std::pow(T, beta) * std::exp(-Ea / (R * T));
    }

    /**
     * @brief Get info string
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "A=" << A << ", beta=" << beta << ", Ea=" << Ea << " J/mol";
        return oss.str();
    }
};

/**
 * @brief Chemical reaction with kinetics
 *
 * Represents a chemical reaction:
 * sum(nu_i * R_i) -> sum(nu_j * P_j)
 *
 * where nu_i are stoichiometric coefficients, R_i are reactants, P_j are products.
 */
class Reaction {
public:
    /**
     * @brief Default constructor
     */
    Reaction() : type_(ReactionType::ELEMENTARY), reversible_(true) {}

    /**
     * @brief Constructor with reaction ID and type
     */
    Reaction(const std::string& id, ReactionType type, bool reversible = true)
        : id_(id), type_(type), reversible_(reversible) {}

    // Getters
    std::string getId() const { return id_; }
    ReactionType getType() const { return type_; }
    bool isReversible() const { return reversible_; }
    const RateLaw& getForwardRateLaw() const { return forwardRateLaw_; }
    const RateLaw& getReverseRateLaw() const { return reverseRateLaw_; }

    const std::map<std::string, double>& getReactants() const { return reactants_; }
    const std::map<std::string, double>& getProducts() const { return products_; }

    // Setters
    void setId(const std::string& id) { id_ = id; }
    void setType(ReactionType type) { type_ = type; }
    void setReversible(bool reversible) { reversible_ = reversible; }
    void setForwardRateLaw(const RateLaw& rateLaw) { forwardRateLaw_ = rateLaw; }
    void setReverseRateLaw(const RateLaw& rateLaw) { reverseRateLaw_ = rateLaw; }

    /**
     * @brief Add a reactant with stoichiometric coefficient
     * @param species Species name
     * @param coefficient Stoichiometric coefficient
     */
    void addReactant(const std::string& species, double coefficient = 1.0) {
        if (coefficient <= 0.0) {
            throw std::invalid_argument("Stoichiometric coefficient must be positive");
        }
        reactants_[species] = coefficient;
    }

    /**
     * @brief Add a product with stoichiometric coefficient
     * @param species Species name
     * @param coefficient Stoichiometric coefficient
     */
    void addProduct(const std::string& species, double coefficient = 1.0) {
        if (coefficient <= 0.0) {
            throw std::invalid_argument("Stoichiometric coefficient must be positive");
        }
        products_[species] = coefficient;
    }

    /**
     * @brief Get stoichiometric coefficient for a species
     * @param species Species name
     * @return Stoichiometric coefficient (positive for products, negative for reactants, 0 if not involved)
     */
    double getStoichiometry(const std::string& species) const {
        double coeff = 0.0;
        auto it_prod = products_.find(species);
        if (it_prod != products_.end()) {
            coeff += it_prod->second;
        }
        auto it_react = reactants_.find(species);
        if (it_react != reactants_.end()) {
            coeff -= it_react->second;
        }
        return coeff;
    }

    /**
     * @brief Check if species is involved in this reaction
     * @param species Species name
     * @return True if species is reactant or product
     */
    bool hasSpecies(const std::string& species) const {
        return reactants_.count(species) > 0 || products_.count(species) > 0;
    }

    /**
     * @brief Calculate forward rate constant at given temperature
     * @param T Temperature (K)
     * @return Forward rate constant
     */
    double getForwardRateConstant(double T) const {
        return forwardRateLaw_.getRateConstant(T);
    }

    /**
     * @brief Calculate reverse rate constant at given temperature
     * @param T Temperature (K)
     * @return Reverse rate constant
     */
    double getReverseRateConstant(double T) const {
        if (!reversible_) {
            return 0.0;
        }
        return reverseRateLaw_.getRateConstant(T);
    }

    /**
     * @brief Calculate forward rate of progress
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Forward rate of progress (mol/(m³·s))
     */
    double getForwardRateOfProgress(double T, const std::map<std::string, double>& concentrations) const {
        double kf = getForwardRateConstant(T);
        double rate = kf;

        for (const auto& [species, coeff] : reactants_) {
            auto it = concentrations.find(species);
            if (it == concentrations.end()) {
                throw std::runtime_error("Concentration not provided for species: " + species);
            }
            rate *= std::pow(it->second, coeff);
        }

        return rate;
    }

    /**
     * @brief Calculate reverse rate of progress
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Reverse rate of progress (mol/(m³·s))
     */
    double getReverseRateOfProgress(double T, const std::map<std::string, double>& concentrations) const {
        if (!reversible_) {
            return 0.0;
        }

        double kr = getReverseRateConstant(T);
        double rate = kr;

        for (const auto& [species, coeff] : products_) {
            auto it = concentrations.find(species);
            if (it == concentrations.end()) {
                throw std::runtime_error("Concentration not provided for species: " + species);
            }
            rate *= std::pow(it->second, coeff);
        }

        return rate;
    }

    /**
     * @brief Calculate net rate of progress
     * @param T Temperature (K)
     * @param concentrations Species concentrations (mol/m³)
     * @return Net rate of progress (mol/(m³·s))
     */
    double getNetRateOfProgress(double T, const std::map<std::string, double>& concentrations) const {
        double forward = getForwardRateOfProgress(T, concentrations);
        double reverse = getReverseRateOfProgress(T, concentrations);
        return forward - reverse;
    }

    /**
     * @brief Get reaction equation string
     * @return Reaction equation (e.g., "H2 + O2 <=> 2 H2O")
     */
    std::string getEquation() const {
        std::ostringstream oss;

        // Reactants
        bool first = true;
        for (const auto& [species, coeff] : reactants_) {
            if (!first) oss << " + ";
            if (coeff != 1.0) {
                oss << coeff << " ";
            }
            oss << species;
            first = false;
        }

        // Arrow
        if (reversible_) {
            oss << " <=> ";
        } else {
            oss << " => ";
        }

        // Products
        first = true;
        for (const auto& [species, coeff] : products_) {
            if (!first) oss << " + ";
            if (coeff != 1.0) {
                oss << coeff << " ";
            }
            oss << species;
            first = false;
        }

        return oss.str();
    }

    /**
     * @brief Get detailed info string
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "Reaction [" << id_ << "]: " << getEquation() << "\n";
        oss << "  Type: ";
        switch (type_) {
            case ReactionType::ELEMENTARY: oss << "Elementary"; break;
            case ReactionType::THREE_BODY: oss << "Three-body"; break;
            case ReactionType::FALLOFF: oss << "Falloff"; break;
            case ReactionType::REVERSIBLE: oss << "Reversible"; break;
            case ReactionType::IRREVERSIBLE: oss << "Irreversible"; break;
        }
        oss << "\n";
        oss << "  Forward rate: " << forwardRateLaw_.getInfo() << "\n";
        if (reversible_) {
            oss << "  Reverse rate: " << reverseRateLaw_.getInfo();
        } else {
            oss << "  Reverse rate: N/A (irreversible)";
        }
        return oss.str();
    }

private:
    std::string id_;                           ///< Reaction identifier
    ReactionType type_;                        ///< Reaction type
    bool reversible_;                          ///< Is reaction reversible?

    std::map<std::string, double> reactants_;  ///< Reactants with stoichiometric coefficients
    std::map<std::string, double> products_;   ///< Products with stoichiometric coefficients

    RateLaw forwardRateLaw_;                   ///< Forward rate law
    RateLaw reverseRateLaw_;                   ///< Reverse rate law
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_REACTION_H
