/**
 * @file FlowChemistryCoupling.h
 * @brief Flow-chemistry coupling for convective reactive transport
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * Phase 67: Multi-Physics Coupling
 *
 * Features:
 * - Convective transport of chemical species
 * - Velocity field coupling
 * - Advection-diffusion-reaction equations
 * - Upwind and central difference schemes
 * - Source term handling
 */

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace koo {
namespace simulation {
namespace coupling {

/**
 * @brief Velocity field
 */
struct VelocityField {
    std::vector<double> u;  ///< x-component velocity (m/s)
    std::vector<double> v;  ///< y-component velocity (m/s)
    std::vector<double> w;  ///< z-component velocity (m/s)

    VelocityField() = default;

    explicit VelocityField(size_t size)
        : u(size, 0.0), v(size, 0.0), w(size, 0.0) {}

    size_t size() const { return u.size(); }

    void resize(size_t new_size) {
        u.resize(new_size, 0.0);
        v.resize(new_size, 0.0);
        w.resize(new_size, 0.0);
    }
};

/**
 * @brief Advection scheme
 */
enum class AdvectionScheme {
    Upwind,         ///< First-order upwind (stable, diffusive)
    Central,        ///< Central difference (second-order, may oscillate)
    QUICK,          ///< Quadratic upwind (third-order)
    WENO           ///< Weighted ENO (high-order, shock-capturing)
};

/**
 * @brief Flow-chemistry coupling configuration
 */
struct FlowCouplingConfig {
    AdvectionScheme scheme;     ///< Advection discretization scheme
    double peclet_number;       ///< Peclet number (advection/diffusion ratio)
    bool use_limiters;          ///< Use flux limiters for stability
    double limiter_param;       ///< Limiter parameter

    FlowCouplingConfig()
        : scheme(AdvectionScheme::Upwind),
          peclet_number(1.0),
          use_limiters(true),
          limiter_param(1.0) {}
};

/**
 * @brief Flow-chemistry coupling manager
 */
class FlowChemistryCoupling {
public:
    /**
     * @brief Construct with configuration
     */
    explicit FlowChemistryCoupling(const FlowCouplingConfig& config = FlowCouplingConfig())
        : config_(config) {}

    /**
     * @brief Compute advective flux at cell face
     *
     * @param concentration_left Left cell concentration
     * @param concentration_right Right cell concentration
     * @param velocity Velocity at face (m/s)
     * @return Advective flux (mol/m²/s or kg/m²/s)
     */
    double computeAdvectiveFlux(double concentration_left,
                               double concentration_right,
                               double velocity) const {
        switch (config_.scheme) {
            case AdvectionScheme::Upwind:
                return upwindFlux(concentration_left, concentration_right, velocity);

            case AdvectionScheme::Central:
                return centralFlux(concentration_left, concentration_right, velocity);

            case AdvectionScheme::QUICK:
                // QUICK requires more neighboring cells; fall back to upwind
                return upwindFlux(concentration_left, concentration_right, velocity);

            case AdvectionScheme::WENO:
                // WENO requires stencil; fall back to upwind
                return upwindFlux(concentration_left, concentration_right, velocity);
        }

        return 0.0;
    }

    /**
     * @brief Compute convective transport term
     *
     * For 1D: -∂(u*c)/∂x = -(u * ∂c/∂x) [assuming incompressible ∂u/∂x=0]
     *
     * @param concentration Concentration field
     * @param velocity Velocity field
     * @param dx Grid spacing
     * @param i Cell index
     * @return Convective transport rate
     */
    double computeConvectiveTerm1D(const std::vector<double>& concentration,
                                  const std::vector<double>& velocity,
                                  double dx,
                                  size_t i) const {
        if (i == 0 || i >= concentration.size() - 1) {
            return 0.0;  // Boundary cells
        }

        // Compute face velocities (average)
        double u_left = (velocity[i-1] + velocity[i]) / 2.0;
        double u_right = (velocity[i] + velocity[i+1]) / 2.0;

        // Compute fluxes at faces
        double flux_left = computeAdvectiveFlux(
            concentration[i-1], concentration[i], u_left);
        double flux_right = computeAdvectiveFlux(
            concentration[i], concentration[i+1], u_right);

        // Divergence: -∂F/∂x
        return -(flux_right - flux_left) / dx;
    }

    /**
     * @brief Compute full advection-diffusion-reaction term
     *
     * dc/dt = -∇·(u*c) + D*∇²c + R(c)
     *
     * @param concentration Concentration field
     * @param velocity Velocity field
     * @param diffusivity Diffusion coefficient (m²/s)
     * @param reaction_rate Reaction source term (mol/m³/s)
     * @param dx Grid spacing
     * @param i Cell index
     * @return Total rate of change
     */
    double computeADRTerm1D(const std::vector<double>& concentration,
                           const std::vector<double>& velocity,
                           double diffusivity,
                           double reaction_rate,
                           double dx,
                           size_t i) const {
        // Advection term
        double advection = computeConvectiveTerm1D(concentration, velocity, dx, i);

        // Diffusion term (central difference)
        double diffusion = 0.0;
        if (i > 0 && i < concentration.size() - 1) {
            diffusion = diffusivity * (concentration[i+1] - 2*concentration[i] + concentration[i-1]) / (dx * dx);
        }

        // Total: advection + diffusion + reaction
        return advection + diffusion + reaction_rate;
    }

    /**
     * @brief Compute Peclet number
     *
     * Pe = u*L / D (ratio of advection to diffusion)
     *
     * @param velocity Characteristic velocity
     * @param length Characteristic length
     * @param diffusivity Diffusion coefficient
     * @return Peclet number
     */
    double computePecletNumber(double velocity,
                              double length,
                              double diffusivity) const {
        if (diffusivity <= 0.0) return std::numeric_limits<double>::infinity();
        return (velocity * length) / diffusivity;
    }

    /**
     * @brief Check if advection-dominated (high Peclet)
     */
    bool isAdvectionDominated(double peclet_number) const {
        return peclet_number > 10.0;
    }

    /**
     * @brief Check if diffusion-dominated (low Peclet)
     */
    bool isDiffusionDominated(double peclet_number) const {
        return peclet_number < 0.1;
    }

    /**
     * @brief Compute optimal grid resolution for Peclet number
     *
     * To avoid numerical oscillations: dx < 2*D/|u|
     *
     * @param velocity Velocity magnitude
     * @param diffusivity Diffusion coefficient
     * @return Recommended grid spacing
     */
    double computeOptimalGridSpacing(double velocity,
                                    double diffusivity) const {
        if (velocity == 0.0) {
            return std::numeric_limits<double>::max();
        }

        // Cell Peclet number criterion: Pe_cell < 2
        return 2.0 * diffusivity / std::abs(velocity);
    }

    /**
     * @brief Set velocity field
     */
    void setVelocityField(const VelocityField& velocity) {
        velocity_field_ = velocity;
    }

    /**
     * @brief Get velocity field
     */
    const VelocityField& getVelocityField() const {
        return velocity_field_;
    }

    /**
     * @brief Get configuration
     */
    const FlowCouplingConfig& getConfig() const { return config_; }

    /**
     * @brief Set configuration
     */
    void setConfig(const FlowCouplingConfig& config) { config_ = config; }

private:
    FlowCouplingConfig config_;
    VelocityField velocity_field_;

    /**
     * @brief Upwind flux scheme
     */
    double upwindFlux(double c_left, double c_right, double velocity) const {
        if (velocity > 0.0) {
            return velocity * c_left;
        } else {
            return velocity * c_right;
        }
    }

    /**
     * @brief Central difference flux scheme
     */
    double centralFlux(double c_left, double c_right, double velocity) const {
        double c_face = 0.5 * (c_left + c_right);
        return velocity * c_face;
    }

    /**
     * @brief Flux limiter (van Leer)
     */
    double vanLeerLimiter(double r) const {
        if (r <= 0.0) return 0.0;
        return (r + std::abs(r)) / (1.0 + std::abs(r));
    }
};

/**
 * @brief Species transport equation manager
 *
 * Solves: ∂c/∂t + ∇·(u*c) = D*∇²c + R(c,T)
 */
class SpeciesTransport {
public:
    /**
     * @brief Construct transport solver
     */
    SpeciesTransport() = default;

    /**
     * @brief Set number of species
     */
    void setNumSpecies(size_t n_species) {
        n_species_ = n_species;
        diffusivities_.resize(n_species, 1e-5);  // Default ~10^-5 m²/s
    }

    /**
     * @brief Set diffusivity for species
     */
    void setDiffusivity(size_t species_id, double diffusivity) {
        if (species_id < diffusivities_.size()) {
            diffusivities_[species_id] = diffusivity;
        }
    }

    /**
     * @brief Get diffusivity for species
     */
    double getDiffusivity(size_t species_id) const {
        if (species_id < diffusivities_.size()) {
            return diffusivities_[species_id];
        }
        return 0.0;
    }

    /**
     * @brief Compute Schmidt number
     *
     * Sc = ν / D (ratio of momentum to mass diffusion)
     *
     * @param kinematic_viscosity Kinematic viscosity (m²/s)
     * @param mass_diffusivity Mass diffusivity (m²/s)
     * @return Schmidt number
     */
    double computeSchmidtNumber(double kinematic_viscosity,
                               double mass_diffusivity) const {
        if (mass_diffusivity <= 0.0) return std::numeric_limits<double>::infinity();
        return kinematic_viscosity / mass_diffusivity;
    }

    /**
     * @brief Estimate turbulent diffusivity from eddy viscosity
     *
     * D_t = ν_t / Sc_t
     *
     * @param eddy_viscosity Turbulent eddy viscosity
     * @param turbulent_schmidt Turbulent Schmidt number (typically 0.7-1.0)
     * @return Turbulent diffusivity
     */
    double computeTurbulentDiffusivity(double eddy_viscosity,
                                      double turbulent_schmidt = 0.7) const {
        return eddy_viscosity / turbulent_schmidt;
    }

private:
    size_t n_species_;
    std::vector<double> diffusivities_;
};

}  // namespace coupling
}  // namespace simulation
}  // namespace koo
