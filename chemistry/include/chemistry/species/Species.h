/**
 * @file Species.h
 * @brief Chemical species representation
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha1
 * @date 2025-11-06
 *
 * Defines chemical species with thermodynamic and transport properties.
 */

#ifndef KOO_CHEMISTRY_SPECIES_H
#define KOO_CHEMISTRY_SPECIES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <cmath>

namespace koo {
namespace chemistry {

/**
 * @brief Phase state of a species
 */
enum class PhaseType {
    GAS,      ///< Gas phase
    LIQUID,   ///< Liquid phase
    SOLID,    ///< Solid phase
    PLASMA,   ///< Plasma phase
    ADSORBED  ///< Surface adsorbed species
};

/**
 * @brief Convert phase type to string
 */
inline std::string toString(PhaseType phase) {
    switch (phase) {
        case PhaseType::GAS:      return "Gas";
        case PhaseType::LIQUID:   return "Liquid";
        case PhaseType::SOLID:    return "Solid";
        case PhaseType::PLASMA:   return "Plasma";
        case PhaseType::ADSORBED: return "Adsorbed";
        default:                  return "Unknown";
    }
}

/**
 * @brief Thermodynamic data for a species
 *
 * NASA polynomial format for heat capacity and thermodynamic properties
 */
struct ThermoData {
    double Tmin{298.15};           ///< Minimum temperature [K]
    double Tmax{5000.0};           ///< Maximum temperature [K]
    double Tmid{1000.0};           ///< Middle temperature [K]

    // NASA polynomial coefficients (7 coefficients for low and high T)
    std::vector<double> lowT{7, 0.0};   ///< Low temperature coefficients
    std::vector<double> highT{7, 0.0};  ///< High temperature coefficients

    // Reference values
    double H298{0.0};              ///< Enthalpy at 298.15 K [J/mol]
    double S298{0.0};              ///< Entropy at 298.15 K [J/(mol·K)]
    double Cp298{0.0};             ///< Heat capacity at 298.15 K [J/(mol·K)]

    /**
     * @brief Calculate heat capacity at constant pressure
     * @param T Temperature [K]
     * @return Cp [J/(mol·K)]
     */
    double getCp(double T) const {
        const auto& a = (T <= Tmid) ? lowT : highT;
        double T2 = T * T;
        double T3 = T2 * T;
        double T4 = T3 * T;
        return 8.314 * (a[0] + a[1]*T + a[2]*T2 + a[3]*T3 + a[4]*T4);
    }

    /**
     * @brief Calculate enthalpy
     * @param T Temperature [K]
     * @return H [J/mol]
     */
    double getH(double T) const {
        const auto& a = (T <= Tmid) ? lowT : highT;
        double T2 = T * T;
        double T3 = T2 * T;
        double T4 = T3 * T;
        return 8.314 * T * (a[0] + a[1]*T/2.0 + a[2]*T2/3.0 + a[3]*T3/4.0 + a[4]*T4/5.0 + a[5]/T);
    }

    /**
     * @brief Calculate entropy
     * @param T Temperature [K]
     * @return S [J/(mol·K)]
     */
    double getS(double T) const {
        const auto& a = (T <= Tmid) ? lowT : highT;
        double T2 = T * T;
        double T3 = T2 * T;
        double T4 = T3 * T;
        return 8.314 * (a[0]*std::log(T) + a[1]*T + a[2]*T2/2.0 + a[3]*T3/3.0 + a[4]*T4/4.0 + a[6]);
    }
};

/**
 * @brief Transport properties of a species
 */
struct TransportData {
    double molecularWeight{0.0};     ///< Molecular weight [kg/mol]
    double lennardJonesSigma{0.0};   ///< Lennard-Jones sigma [Angstrom]
    double lennardJonesEpsilon{0.0}; ///< Lennard-Jones epsilon/k_B [K]
    double dipoleMoment{0.0};        ///< Dipole moment [Debye]
    double polarizability{0.0};      ///< Polarizability [Angstrom^3]
    double rotRelaxation{0.0};       ///< Rotational relaxation collision number

    /**
     * @brief Estimate binary diffusion coefficient
     * @param T Temperature [K]
     * @param P Pressure [Pa]
     * @param otherMW Other species molecular weight [kg/mol]
     * @param otherSigma Other species sigma [Angstrom]
     * @param otherEpsilon Other species epsilon/k_B [K]
     * @return Diffusion coefficient [m^2/s]
     */
    double getDiffusivity(double T, double P, double otherMW, double otherSigma, double otherEpsilon) const {
        // Simplified Chapman-Enskog equation
        double sigma12 = (lennardJonesSigma + otherSigma) / 2.0;
        double epsilon12 = std::sqrt(lennardJonesEpsilon * otherEpsilon);
        double Tstar = T / epsilon12;

        // Collision integral (simplified)
        double omega = 1.16145 / std::pow(Tstar, 0.14874) + 0.52487 / std::exp(0.77320 * Tstar);

        // Molecular weight in g/mol for the formula
        double M1 = molecularWeight * 1000.0;
        double M2 = otherMW * 1000.0;
        double M12 = 2.0 / (1.0/M1 + 1.0/M2);

        // D12 = 0.00266 * T^1.5 / (P * sigma12^2 * omega * sqrt(M12))
        // Result in cm^2/s, convert to m^2/s
        double D12 = 0.00266 * std::pow(T, 1.5) / (P * 1e-5 * sigma12 * sigma12 * omega * std::sqrt(M12));
        return D12 * 1e-4;  // cm^2/s to m^2/s
    }
};

/**
 * @brief Chemical species class
 *
 * Represents a chemical species with composition, thermodynamic properties,
 * and transport properties.
 */
class Species {
public:
    /**
     * @brief Constructor
     * @param name Species name
     * @param composition Elemental composition (e.g., {"C": 1, "O": 2} for CO2)
     * @param phase Phase type
     */
    Species(const std::string& name,
            const std::map<std::string, int>& composition,
            PhaseType phase = PhaseType::GAS)
        : name_(name),
          composition_(composition),
          phase_(phase),
          charge_(0),
          molecularWeight_(0.0) {}

    /**
     * @brief Default constructor
     */
    Species() : phase_(PhaseType::GAS), charge_(0), molecularWeight_(0.0) {}

    // === Getters ===

    const std::string& getName() const { return name_; }
    const std::map<std::string, int>& getComposition() const { return composition_; }
    PhaseType getPhase() const { return phase_; }
    int getCharge() const { return charge_; }
    double getMolecularWeight() const { return molecularWeight_; }

    const ThermoData& getThermoData() const { return thermoData_; }
    const TransportData& getTransportData() const { return transportData_; }

    // === Setters ===

    void setName(const std::string& name) { name_ = name; }
    void setComposition(const std::map<std::string, int>& comp) { composition_ = comp; }
    void setPhase(PhaseType phase) { phase_ = phase; }
    void setCharge(int charge) { charge_ = charge; }
    void setMolecularWeight(double mw) { molecularWeight_ = mw; }

    void setThermoData(const ThermoData& data) { thermoData_ = data; }
    void setTransportData(const TransportData& data) { transportData_ = data; }

    // === Composition Queries ===

    /**
     * @brief Get number of atoms of a specific element
     * @param element Element symbol
     * @return Number of atoms
     */
    int getElementCount(const std::string& element) const {
        auto it = composition_.find(element);
        return (it != composition_.end()) ? it->second : 0;
    }

    /**
     * @brief Check if species contains element
     * @param element Element symbol
     * @return True if element is present
     */
    bool hasElement(const std::string& element) const {
        return composition_.find(element) != composition_.end();
    }

    /**
     * @brief Get total number of atoms
     * @return Total atom count
     */
    int getTotalAtomCount() const {
        int total = 0;
        for (const auto& pair : composition_) {
            total += pair.second;
        }
        return total;
    }

    // === Thermodynamic Properties ===

    /**
     * @brief Get heat capacity at temperature
     * @param T Temperature [K]
     * @return Cp [J/(mol·K)]
     */
    double getCp(double T) const {
        return thermoData_.getCp(T);
    }

    /**
     * @brief Get enthalpy at temperature
     * @param T Temperature [K]
     * @return H [J/mol]
     */
    double getEnthalpy(double T) const {
        return thermoData_.getH(T);
    }

    /**
     * @brief Get entropy at temperature
     * @param T Temperature [K]
     * @return S [J/(mol·K)]
     */
    double getEntropy(double T) const {
        return thermoData_.getS(T);
    }

    /**
     * @brief Get Gibbs free energy
     * @param T Temperature [K]
     * @return G [J/mol]
     */
    double getGibbsEnergy(double T) const {
        return getEnthalpy(T) - T * getEntropy(T);
    }

    // === Information ===

    /**
     * @brief Get species information string
     */
    std::string getInfo() const {
        std::string info = "Species: " + name_ + "\n";
        info += "  Phase: " + toString(phase_) + "\n";
        info += "  Molecular Weight: " + std::to_string(molecularWeight_) + " kg/mol\n";
        info += "  Charge: " + std::to_string(charge_) + "\n";
        info += "  Composition: ";
        for (const auto& pair : composition_) {
            info += pair.first + std::to_string(pair.second) + " ";
        }
        info += "\n";
        return info;
    }

    /**
     * @brief Get formula string
     * @return Chemical formula (e.g., "H2O")
     */
    std::string getFormula() const {
        std::string formula;
        for (const auto& pair : composition_) {
            formula += pair.first;
            if (pair.second > 1) {
                formula += std::to_string(pair.second);
            }
        }
        if (charge_ != 0) {
            formula += (charge_ > 0) ? "+" : "";
            formula += std::to_string(charge_);
        }
        return formula;
    }

private:
    std::string name_;
    std::map<std::string, int> composition_;
    PhaseType phase_;
    int charge_;
    double molecularWeight_;

    ThermoData thermoData_;
    TransportData transportData_;
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_SPECIES_H
