#ifndef KOO_SURFACE_SPECIES_H
#define KOO_SURFACE_SPECIES_H

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
 * @enum SurfacePhase
 * @brief Phase of surface species
 */
enum class SurfacePhase {
    ADSORBED,    ///< Adsorbed on surface
    PHYSISORBED, ///< Physisorbed (weak bonding)
    CHEMISORBED  ///< Chemisorbed (strong bonding)
};

/**
 * @class SurfaceSpecies
 * @brief Represents a chemical species on a surface
 *
 * Phase 24: Surface Species and Sites
 *
 * Properties:
 * - Name and molecular weight
 * - Surface phase (adsorbed, physisorbed, chemisorbed)
 * - Site occupancy (number of sites occupied)
 * - Binding energy
 * - Sticking coefficient
 */
class SurfaceSpecies {
public:
    /**
     * @brief Constructor
     * @param name Species name
     * @param molecularWeight Molecular weight (kg/mol)
     * @param siteOccupancy Number of surface sites occupied
     * @param bindingEnergy Binding energy (J/mol)
     */
    SurfaceSpecies(const std::string& name,
                   double molecularWeight,
                   int siteOccupancy = 1,
                   double bindingEnergy = 0.0)
        : name_(name),
          molecularWeight_(molecularWeight),
          siteOccupancy_(siteOccupancy),
          bindingEnergy_(bindingEnergy),
          phase_(SurfacePhase::CHEMISORBED),
          stickingCoefficient_(1.0) {}

    // Getters
    std::string getName() const { return name_; }
    double getMolecularWeight() const { return molecularWeight_; }
    int getSiteOccupancy() const { return siteOccupancy_; }
    double getBindingEnergy() const { return bindingEnergy_; }
    SurfacePhase getPhase() const { return phase_; }
    double getStickingCoefficient() const { return stickingCoefficient_; }

    // Setters
    void setPhase(SurfacePhase phase) { phase_ = phase; }
    void setStickingCoefficient(double s) {
        if (s < 0.0 || s > 1.0) {
            throw std::invalid_argument("Sticking coefficient must be between 0 and 1");
        }
        stickingCoefficient_ = s;
    }
    void setBindingEnergy(double E) { bindingEnergy_ = E; }

    /**
     * @brief Get desorption energy (same as binding energy for simple model)
     */
    double getDesorptionEnergy() const { return bindingEnergy_; }

    /**
     * @brief Check if strongly bound (chemisorbed)
     */
    bool isChemisorbed() const { return phase_ == SurfacePhase::CHEMISORBED; }

    /**
     * @brief Check if weakly bound (physisorbed)
     */
    bool isPhysisorbed() const { return phase_ == SurfacePhase::PHYSISORBED; }

private:
    std::string name_;
    double molecularWeight_;
    int siteOccupancy_;       ///< Number of sites occupied (1 for most species)
    double bindingEnergy_;    ///< Binding energy (J/mol)
    SurfacePhase phase_;
    double stickingCoefficient_; ///< Probability of adsorption upon collision
};

/**
 * @class SurfaceSite
 * @brief Represents an active site on a surface
 *
 * Phase 24: Surface Species and Sites
 *
 * Properties:
 * - Site type (terrace, step, kink, etc.)
 * - Site density (sites/m²)
 * - Available sites
 */
class SurfaceSite {
public:
    enum class SiteType {
        TERRACE,  ///< Flat terrace site
        STEP,     ///< Step edge site
        KINK,     ///< Kink site
        DEFECT,   ///< Defect site
        GENERIC   ///< Generic site
    };

    /**
     * @brief Constructor
     * @param type Site type
     * @param density Site density (sites/m²)
     */
    SurfaceSite(SiteType type, double density)
        : type_(type),
          density_(density),
          totalSites_(density),
          occupiedSites_(0.0) {}

    // Getters
    SiteType getType() const { return type_; }
    double getDensity() const { return density_; }
    double getTotalSites() const { return totalSites_; }
    double getOccupiedSites() const { return occupiedSites_; }
    double getAvailableSites() const { return totalSites_ - occupiedSites_; }

    /**
     * @brief Get surface coverage θ = occupied/total
     */
    double getCoverage() const {
        if (totalSites_ < 1.0e-15) return 0.0;
        return occupiedSites_ / totalSites_;
    }

    /**
     * @brief Get vacant site fraction (1 - θ)
     */
    double getVacantFraction() const {
        return 1.0 - getCoverage();
    }

    /**
     * @brief Set occupied sites
     */
    void setOccupiedSites(double occupied) {
        if (occupied < 0.0) occupied = 0.0;
        if (occupied > totalSites_) occupied = totalSites_;
        occupiedSites_ = occupied;
    }

    /**
     * @brief Update coverage directly
     */
    void setCoverage(double theta) {
        if (theta < 0.0) theta = 0.0;
        if (theta > 1.0) theta = 1.0;
        occupiedSites_ = theta * totalSites_;
    }

    /**
     * @brief Get site type name
     */
    std::string getTypeName() const {
        switch (type_) {
            case SiteType::TERRACE: return "Terrace";
            case SiteType::STEP: return "Step";
            case SiteType::KINK: return "Kink";
            case SiteType::DEFECT: return "Defect";
            case SiteType::GENERIC: return "Generic";
            default: return "Unknown";
        }
    }

private:
    SiteType type_;
    double density_;       ///< Site density (sites/m²)
    double totalSites_;    ///< Total number of sites
    double occupiedSites_; ///< Currently occupied sites
};

/**
 * @class SurfaceCoverage
 * @brief Manages surface coverage for multiple species
 *
 * Phase 24: Surface Species and Sites
 */
class SurfaceCoverage {
public:
    /**
     * @brief Constructor
     * @param totalSiteDensity Total site density (sites/m²)
     */
    explicit SurfaceCoverage(double totalSiteDensity)
        : totalSiteDensity_(totalSiteDensity) {}

    /**
     * @brief Add a species to track
     */
    void addSpecies(const std::string& name,
                   std::shared_ptr<SurfaceSpecies> species) {
        species_[name] = species;
        coverage_[name] = 0.0;
    }

    /**
     * @brief Set coverage for a species
     */
    void setCoverage(const std::string& name, double theta) {
        if (species_.find(name) == species_.end()) {
            throw std::runtime_error("Species not found: " + name);
        }
        if (theta < 0.0) theta = 0.0;
        if (theta > 1.0) theta = 1.0;
        coverage_[name] = theta;
    }

    /**
     * @brief Get coverage for a species
     */
    double getCoverage(const std::string& name) const {
        auto it = coverage_.find(name);
        if (it == coverage_.end()) return 0.0;
        return it->second;
    }

    /**
     * @brief Get total coverage (sum over all species)
     */
    double getTotalCoverage() const {
        double total = 0.0;
        for (const auto& pair : coverage_) {
            auto species = species_.at(pair.first);
            total += pair.second * species->getSiteOccupancy();
        }
        return total;
    }

    /**
     * @brief Get vacant site fraction
     */
    double getVacantFraction() const {
        return 1.0 - getTotalCoverage();
    }

    /**
     * @brief Get concentration on surface (mol/m²)
     * Γ = θ × Γ_max
     * where Γ_max = site density / N_A
     */
    double getSurfaceConcentration(const std::string& name) const {
        double theta = getCoverage(name);
        const double N_A = 6.022e23; // Avogadro's number
        return theta * totalSiteDensity_ / N_A;
    }

    /**
     * @brief Check if surface is saturated
     */
    bool isSaturated(double tolerance = 0.99) const {
        return getTotalCoverage() >= tolerance;
    }

    /**
     * @brief Get all species names
     */
    std::vector<std::string> getSpeciesNames() const {
        std::vector<std::string> names;
        for (const auto& pair : species_) {
            names.push_back(pair.first);
        }
        return names;
    }

    /**
     * @brief Clear all coverages
     */
    void clearCoverages() {
        for (auto& pair : coverage_) {
            pair.second = 0.0;
        }
    }

    double getTotalSiteDensity() const { return totalSiteDensity_; }

private:
    double totalSiteDensity_; ///< Total site density (sites/m²)
    std::map<std::string, std::shared_ptr<SurfaceSpecies>> species_;
    std::map<std::string, double> coverage_; ///< θ for each species
};

} // namespace surface
} // namespace physics
} // namespace koo

#endif // KOO_SURFACE_SPECIES_H
