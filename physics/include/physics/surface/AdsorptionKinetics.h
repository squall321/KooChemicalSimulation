#ifndef KOO_ADSORPTION_KINETICS_H
#define KOO_ADSORPTION_KINETICS_H

#include "SurfaceSpecies.h"
#include <cmath>
#include <memory>
#include <stdexcept>

namespace koo {
namespace physics {
namespace surface {

/**
 * @class AdsorptionKinetics
 * @brief Adsorption and desorption kinetics models
 *
 * Phase 25: Adsorption Kinetics
 *
 * Kinetic theory of gases:
 * - Collision rate: Z = P / √(2πmkT)
 * - Adsorption rate: r_ads = s × Z × (1-θ)ⁿ
 * - Desorption rate: r_des = k_des × θⁿ
 *
 * where:
 * - s: sticking coefficient
 * - θ: surface coverage
 * - n: order of reaction
 */
class AdsorptionKinetics {
public:
    /**
     * @brief Constructor
     * @param species Surface species
     */
    explicit AdsorptionKinetics(std::shared_ptr<SurfaceSpecies> species)
        : species_(species) {}

    /**
     * @brief Calculate collision rate (collisions per site per second)
     *
     * Z = P / √(2πmkT)
     *
     * @param pressure Gas pressure (Pa)
     * @param temperature Temperature (K)
     * @param siteDensity Site density (sites/m²)
     * @return Collision rate (collisions/(site·s))
     */
    double calculateCollisionRate(double pressure,
                                  double temperature,
                                  double siteDensity) const {
        const double k_B = 1.380649e-23; // Boltzmann constant (J/K)
        const double N_A = 6.022e23;     // Avogadro's number

        double mass = species_->getMolecularWeight() / N_A; // kg per molecule

        // Kinetic theory: collision rate per unit area
        // Φ = P / √(2πmkT) [molecules/(m²·s)]
        double flux = pressure / std::sqrt(2.0 * M_PI * mass * k_B * temperature);

        // Convert to collisions per site per second
        return flux / siteDensity;
    }

    /**
     * @brief Calculate adsorption rate
     *
     * r_ads = s × Z × (1-θ)ⁿ
     *
     * @param pressure Gas pressure (Pa)
     * @param temperature Temperature (K)
     * @param coverage Surface coverage θ
     * @param siteDensity Site density (sites/m²)
     * @param order Reaction order (default: 1)
     * @return Adsorption rate (1/s)
     */
    double calculateAdsorptionRate(double pressure,
                                   double temperature,
                                   double coverage,
                                   double siteDensity,
                                   int order = 1) const {
        double Z = calculateCollisionRate(pressure, temperature, siteDensity);
        double s = species_->getStickingCoefficient();

        // Available site fraction
        double available = 1.0 - coverage;
        if (available < 0.0) available = 0.0;

        // Rate with order dependence
        double rate = s * Z * std::pow(available, order);

        return rate;
    }

    /**
     * @brief Calculate desorption rate
     *
     * r_des = k_des × θⁿ
     * k_des = ν × exp(-E_des / RT)
     *
     * @param temperature Temperature (K)
     * @param coverage Surface coverage θ
     * @param preExponential Pre-exponential factor ν (1/s)
     * @param order Reaction order (default: 1)
     * @return Desorption rate (1/s)
     */
    double calculateDesorptionRate(double temperature,
                                   double coverage,
                                   double preExponential = 1.0e13,
                                   int order = 1) const {
        const double R = 8.314; // Gas constant (J/(mol·K))

        double E_des = species_->getDesorptionEnergy();

        // Arrhenius rate constant
        double k_des = preExponential * std::exp(-E_des / (R * temperature));

        // Rate with order dependence
        double rate = k_des * std::pow(coverage, order);

        return rate;
    }

    /**
     * @brief Calculate net adsorption rate
     *
     * r_net = r_ads - r_des
     *
     * @return Net rate (1/s)
     */
    double calculateNetRate(double pressure,
                           double temperature,
                           double coverage,
                           double siteDensity,
                           double preExponential = 1.0e13) const {
        double r_ads = calculateAdsorptionRate(pressure, temperature, coverage, siteDensity);
        double r_des = calculateDesorptionRate(temperature, coverage, preExponential);

        return r_ads - r_des;
    }

    /**
     * @brief Get species
     */
    std::shared_ptr<SurfaceSpecies> getSpecies() const { return species_; }

private:
    std::shared_ptr<SurfaceSpecies> species_;
};

/**
 * @class LangmuirIsotherm
 * @brief Langmuir adsorption isotherm model
 *
 * Phase 25: Adsorption Kinetics
 *
 * Equilibrium coverage:
 * θ = KP / (1 + KP)
 *
 * where K = equilibrium constant = k_ads / k_des
 */
class LangmuirIsotherm {
public:
    /**
     * @brief Constructor
     * @param species Surface species
     */
    explicit LangmuirIsotherm(std::shared_ptr<SurfaceSpecies> species)
        : species_(species) {}

    /**
     * @brief Calculate equilibrium constant
     *
     * K = (s / ν) × √(2πmkT) × exp(E_des / RT)
     *
     * @param temperature Temperature (K)
     * @param preExponential Pre-exponential factor ν (1/s)
     * @return Equilibrium constant K (1/Pa)
     */
    double calculateEquilibriumConstant(double temperature,
                                       double preExponential = 1.0e13) const {
        const double k_B = 1.380649e-23;
        const double N_A = 6.022e23;
        const double R = 8.314;

        double mass = species_->getMolecularWeight() / N_A;
        double s = species_->getStickingCoefficient();
        double E_des = species_->getDesorptionEnergy();

        // K = k_ads / k_des
        double K = (s / preExponential) * std::sqrt(2.0 * M_PI * mass * k_B * temperature)
                   * std::exp(E_des / (R * temperature));

        return K;
    }

    /**
     * @brief Calculate equilibrium coverage
     *
     * θ = KP / (1 + KP)
     *
     * @param pressure Gas pressure (Pa)
     * @param temperature Temperature (K)
     * @param preExponential Pre-exponential factor (1/s)
     * @return Equilibrium coverage θ
     */
    double calculateCoverage(double pressure,
                            double temperature,
                            double preExponential = 1.0e13) const {
        double K = calculateEquilibriumConstant(temperature, preExponential);
        double KP = K * pressure;

        return KP / (1.0 + KP);
    }

    /**
     * @brief Calculate pressure for given coverage
     *
     * P = θ / (K(1-θ))
     *
     * @param coverage Target coverage θ
     * @param temperature Temperature (K)
     * @param preExponential Pre-exponential factor (1/s)
     * @return Required pressure (Pa)
     */
    double calculatePressure(double coverage,
                            double temperature,
                            double preExponential = 1.0e13) const {
        if (coverage >= 1.0) {
            throw std::invalid_argument("Coverage must be less than 1");
        }

        double K = calculateEquilibriumConstant(temperature, preExponential);

        return coverage / (K * (1.0 - coverage));
    }

    /**
     * @brief Calculate half-coverage pressure
     *
     * P_half = 1/K (when θ = 0.5)
     */
    double calculateHalfCoveragePressure(double temperature,
                                        double preExponential = 1.0e13) const {
        double K = calculateEquilibriumConstant(temperature, preExponential);
        return 1.0 / K;
    }

    /**
     * @brief Check if Langmuir assumptions are valid
     *
     * Assumptions:
     * - Monolayer adsorption
     * - Homogeneous surface
     * - No lateral interactions
     * - Equilibrium between adsorption and desorption
     */
    bool checkAssumptions(double coverage) const {
        // Check if coverage is in valid range
        if (coverage < 0.0 || coverage > 1.0) return false;

        // For simple check, just verify coverage is reasonable
        return true;
    }

private:
    std::shared_ptr<SurfaceSpecies> species_;
};

/**
 * @class BETIsotherm
 * @brief BET (Brunauer-Emmett-Teller) multilayer adsorption
 *
 * Phase 25: Adsorption Kinetics
 *
 * For multilayer adsorption:
 * θ = CP / ((P0 - P)(1 + (C-1)P/P0))
 *
 * where:
 * - P0: saturation pressure
 * - C: BET constant
 */
class BETIsotherm {
public:
    /**
     * @brief Constructor
     * @param species Surface species
     * @param C BET constant
     * @param P0 Saturation pressure (Pa)
     */
    BETIsotherm(std::shared_ptr<SurfaceSpecies> species,
                double C,
                double P0)
        : species_(species), C_(C), P0_(P0) {}

    /**
     * @brief Calculate coverage using BET equation
     */
    double calculateCoverage(double pressure) const {
        if (pressure >= P0_) {
            throw std::invalid_argument("Pressure must be less than saturation pressure");
        }

        double P_ratio = pressure / P0_;
        double numerator = C_ * P_ratio;
        double denominator = (1.0 - P_ratio) * (1.0 + (C_ - 1.0) * P_ratio);

        return numerator / denominator;
    }

    /**
     * @brief Get BET constant
     */
    double getBETConstant() const { return C_; }

    /**
     * @brief Get saturation pressure
     */
    double getSaturationPressure() const { return P0_; }

private:
    std::shared_ptr<SurfaceSpecies> species_;
    double C_;  ///< BET constant
    double P0_; ///< Saturation pressure
};

/**
 * @class FreundlichIsotherm
 * @brief Freundlich adsorption isotherm (empirical)
 *
 * Phase 25: Adsorption Kinetics
 *
 * θ = K × P^(1/n)
 *
 * where:
 * - K: Freundlich constant
 * - n: heterogeneity factor
 */
class FreundlichIsotherm {
public:
    /**
     * @brief Constructor
     * @param K Freundlich constant
     * @param n Heterogeneity factor
     */
    FreundlichIsotherm(double K, double n)
        : K_(K), n_(n) {}

    /**
     * @brief Calculate coverage
     */
    double calculateCoverage(double pressure) const {
        return K_ * std::pow(pressure, 1.0 / n_);
    }

    double getK() const { return K_; }
    double getn() const { return n_; }

private:
    double K_; ///< Freundlich constant
    double n_; ///< Heterogeneity factor
};

} // namespace surface
} // namespace physics
} // namespace koo

#endif // KOO_ADSORPTION_KINETICS_H
