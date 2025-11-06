#ifndef KOO_TRANSPORT_BOUNDARY_H
#define KOO_TRANSPORT_BOUNDARY_H

#include <array>
#include <string>
#include <stdexcept>
#include <cmath>

namespace koo {
namespace physics {
namespace transport {

/**
 * @enum BoundaryType
 * @brief Types of boundary conditions for transport equations
 */
enum class BoundaryType {
    DIRICHLET,         ///< Fixed concentration: C = C_b
    NEUMANN,           ///< Fixed flux: -D(∂C/∂n) = J_b
    ROBIN,             ///< Mixed: -D(∂C/∂n) = h(C - C_∞)
    CONVECTIVE_OUTFLOW ///< Convective outflow: ∂C/∂t + v(∂C/∂n) = 0
};

/**
 * @class TransportBoundary
 * @brief Boundary conditions for transport equations
 *
 * Phase 22: Transport Phenomena (v0.5.0-alpha2)
 *
 * Boundary condition types:
 *
 * 1. Dirichlet (fixed concentration):
 *    C = C_b
 *
 * 2. Neumann (fixed flux):
 *    -D(∂C/∂n) = J_b
 *    where n is the outward normal
 *
 * 3. Robin (mixed/convective):
 *    -D(∂C/∂n) = h(C - C_∞)
 *    where h is mass transfer coefficient
 *
 * 4. Convective outflow:
 *    ∂C/∂t + v·n(∂C/∂n) = 0
 *    Non-reflecting boundary for advection
 */
class TransportBoundary {
public:
    /**
     * @brief Constructor for Dirichlet boundary
     * @param value Boundary concentration C_b
     */
    static TransportBoundary dirichlet(double value) {
        TransportBoundary bc;
        bc.type_ = BoundaryType::DIRICHLET;
        bc.value_ = value;
        return bc;
    }

    /**
     * @brief Constructor for Neumann boundary
     * @param flux Boundary flux J_b (positive = outward)
     */
    static TransportBoundary neumann(double flux) {
        TransportBoundary bc;
        bc.type_ = BoundaryType::NEUMANN;
        bc.flux_ = flux;
        return bc;
    }

    /**
     * @brief Constructor for Robin boundary
     * @param transferCoeff Mass transfer coefficient h
     * @param ambientConc Ambient concentration C_∞
     */
    static TransportBoundary robin(double transferCoeff, double ambientConc) {
        TransportBoundary bc;
        bc.type_ = BoundaryType::ROBIN;
        bc.transferCoeff_ = transferCoeff;
        bc.ambientConc_ = ambientConc;
        return bc;
    }

    /**
     * @brief Constructor for convective outflow boundary
     * @param normalVelocity Normal velocity component v·n
     */
    static TransportBoundary convectiveOutflow(double normalVelocity) {
        TransportBoundary bc;
        bc.type_ = BoundaryType::CONVECTIVE_OUTFLOW;
        bc.normalVelocity_ = normalVelocity;
        return bc;
    }

    /**
     * @brief Get boundary type
     */
    BoundaryType getType() const { return type_; }

    /**
     * @brief Apply Dirichlet boundary condition
     * @return Boundary concentration
     */
    double applyDirichlet() const {
        if (type_ != BoundaryType::DIRICHLET) {
            throw std::runtime_error("Not a Dirichlet boundary");
        }
        return value_;
    }

    /**
     * @brief Apply Neumann boundary condition
     * @param diffusionCoeff Diffusion coefficient D
     * @return Normal gradient ∂C/∂n = -J_b/D
     */
    double applyNeumann(double diffusionCoeff) const {
        if (type_ != BoundaryType::NEUMANN) {
            throw std::runtime_error("Not a Neumann boundary");
        }
        if (std::abs(diffusionCoeff) < 1.0e-15) {
            throw std::runtime_error("Diffusion coefficient too small");
        }
        return -flux_ / diffusionCoeff;
    }

    /**
     * @brief Apply Robin boundary condition
     * @param concentration Current concentration at boundary
     * @param diffusionCoeff Diffusion coefficient D
     * @return Normal gradient ∂C/∂n = -h(C - C_∞)/D
     */
    double applyRobin(double concentration, double diffusionCoeff) const {
        if (type_ != BoundaryType::ROBIN) {
            throw std::runtime_error("Not a Robin boundary");
        }
        if (std::abs(diffusionCoeff) < 1.0e-15) {
            throw std::runtime_error("Diffusion coefficient too small");
        }
        return -transferCoeff_ * (concentration - ambientConc_) / diffusionCoeff;
    }

    /**
     * @brief Apply convective outflow boundary condition
     * @param concentration Current concentration
     * @param upwindConc Upwind concentration (interior)
     * @param dt Time step
     * @param dn Grid spacing in normal direction
     * @return Updated boundary concentration
     */
    double applyConvectiveOutflow(double concentration,
                                  double upwindConc,
                                  double dt,
                                  double dn) const {
        if (type_ != BoundaryType::CONVECTIVE_OUTFLOW) {
            throw std::runtime_error("Not a convective outflow boundary");
        }
        if (dn <= 0.0) {
            throw std::invalid_argument("Grid spacing must be positive");
        }

        // Upwind discretization: ∂C/∂n ≈ (C - C_upwind)/dn
        // ∂C/∂t = -v·n(∂C/∂n)
        double normalGradient = (concentration - upwindConc) / dn;
        double dCdt = -normalVelocity_ * normalGradient;

        return concentration + dt * dCdt;
    }

    /**
     * @brief Calculate flux through boundary
     * @param concentration Concentration at boundary
     * @param normalGradient Normal gradient ∂C/∂n
     * @param diffusionCoeff Diffusion coefficient
     * @param normalVelocity Normal velocity component
     * @return Total flux (diffusive + advective)
     */
    static double calculateFlux(double concentration,
                               double normalGradient,
                               double diffusionCoeff,
                               double normalVelocity) {
        // Diffusive flux: -D(∂C/∂n)
        double diffusiveFlux = -diffusionCoeff * normalGradient;

        // Advective flux: v·n × C
        double advectiveFlux = normalVelocity * concentration;

        return diffusiveFlux + advectiveFlux;
    }

    /**
     * @brief Get Biot number: Bi = hL/D
     *
     * For Robin boundary conditions
     * Bi << 1: Surface resistance negligible
     * Bi >> 1: Internal resistance negligible
     */
    static double getBiotNumber(double transferCoeff,
                               double characteristicLength,
                               double diffusionCoeff) {
        if (std::abs(diffusionCoeff) < 1.0e-15) {
            return 1.0e10;
        }
        return transferCoeff * characteristicLength / diffusionCoeff;
    }

    /**
     * @brief Get boundary value (for Dirichlet)
     */
    double getValue() const { return value_; }

    /**
     * @brief Get flux (for Neumann)
     */
    double getFlux() const { return flux_; }

    /**
     * @brief Get transfer coefficient (for Robin)
     */
    double getTransferCoeff() const { return transferCoeff_; }

    /**
     * @brief Get ambient concentration (for Robin)
     */
    double getAmbientConc() const { return ambientConc_; }

    /**
     * @brief Get normal velocity (for convective outflow)
     */
    double getNormalVelocity() const { return normalVelocity_; }

    /**
     * @brief Get string representation of boundary type
     */
    std::string getTypeName() const {
        switch (type_) {
            case BoundaryType::DIRICHLET:
                return "Dirichlet";
            case BoundaryType::NEUMANN:
                return "Neumann";
            case BoundaryType::ROBIN:
                return "Robin";
            case BoundaryType::CONVECTIVE_OUTFLOW:
                return "ConvectiveOutflow";
            default:
                return "Unknown";
        }
    }

private:
    TransportBoundary()
        : type_(BoundaryType::DIRICHLET),
          value_(0.0),
          flux_(0.0),
          transferCoeff_(0.0),
          ambientConc_(0.0),
          normalVelocity_(0.0) {}

    BoundaryType type_;         ///< Boundary condition type
    double value_;              ///< Dirichlet value
    double flux_;               ///< Neumann flux
    double transferCoeff_;      ///< Robin transfer coefficient
    double ambientConc_;        ///< Robin ambient concentration
    double normalVelocity_;     ///< Convective outflow normal velocity
};

} // namespace transport
} // namespace physics
} // namespace koo

#endif // KOO_TRANSPORT_BOUNDARY_H
