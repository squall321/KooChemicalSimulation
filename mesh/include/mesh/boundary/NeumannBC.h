/**
 * @file NeumannBC.h
 * @brief Neumann (natural) boundary conditions
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha3
 * @date 2025-11-06
 *
 * Neumann BCs prescribe the flux or derivative on the boundary.
 * Example: ∂u/∂n = h on Γ_N
 */

#ifndef KOO_MESH_BOUNDARY_NEUMANN_BC_H
#define KOO_MESH_BOUNDARY_NEUMANN_BC_H

#include "BoundaryCondition.h"
#include <cmath>

namespace koo {
namespace mesh {
namespace boundary {

/**
 * @brief Neumann boundary condition
 *
 * Prescribes the normal derivative (flux) on the boundary:
 *   ∂u/∂n = h(x,t) on Γ_N
 * or
 *   -k ∇u · n = h(x,t) on Γ_N (for diffusion problems)
 */
class NeumannBC : public BoundaryCondition {
public:
    /**
     * @brief Create constant Neumann BC
     */
    static std::shared_ptr<NeumannBC> makeConstant(
        const std::string& name, int tag, double flux) {

        auto bc = std::make_shared<NeumannBC>(name, tag);
        bc->setFlux(flux);
        return bc;
    }

    /**
     * @brief Create time-dependent Neumann BC
     */
    static std::shared_ptr<NeumannBC> makeTimeDependent(
        const std::string& name, int tag,
        std::function<double(double)> timeFunc) {

        auto bc = std::make_shared<NeumannBC>(name, tag);
        bc->setTimeFunction(timeFunc);
        return bc;
    }

    /**
     * @brief Create spatially varying Neumann BC
     */
    static std::shared_ptr<NeumannBC> makeSpatial(
        const std::string& name, int tag,
        std::function<double(double, double, double)> spatialFunc) {

        auto bc = std::make_shared<NeumannBC>(name, tag);
        bc->setSpatialFunction(spatialFunc);
        return bc;
    }

    /**
     * @brief Create general space-time varying Neumann BC
     */
    static std::shared_ptr<NeumannBC> makeGeneral(
        const std::string& name, int tag, BCFunction func) {

        auto bc = std::make_shared<NeumannBC>(name, tag);
        bc->setFunction(func);
        return bc;
    }

    /**
     * @brief Constructor
     */
    NeumannBC(const std::string& name, int tag)
        : BoundaryCondition(name, BCType::NEUMANN, tag),
          constantFlux_(0.0),
          isConstant_(true),
          hasTimeFunc_(false),
          hasSpatialFunc_(false),
          hasGeneralFunc_(false),
          coefficient_(1.0) {}

    /**
     * @brief Set constant flux value
     */
    void setFlux(double flux) {
        constantFlux_ = flux;
        isConstant_ = true;
        hasTimeFunc_ = false;
        hasSpatialFunc_ = false;
        hasGeneralFunc_ = false;
    }

    /**
     * @brief Set time-dependent function
     */
    void setTimeFunction(std::function<double(double)> func) {
        timeFunc_ = func;
        isConstant_ = false;
        hasTimeFunc_ = true;
        hasSpatialFunc_ = false;
        hasGeneralFunc_ = false;
    }

    /**
     * @brief Set spatial function
     */
    void setSpatialFunction(std::function<double(double, double, double)> func) {
        spatialFunc_ = func;
        isConstant_ = false;
        hasTimeFunc_ = false;
        hasSpatialFunc_ = true;
        hasGeneralFunc_ = false;
    }

    /**
     * @brief Set general space-time function
     */
    void setFunction(BCFunction func) {
        generalFunc_ = func;
        isConstant_ = false;
        hasTimeFunc_ = false;
        hasSpatialFunc_ = false;
        hasGeneralFunc_ = true;
    }

    /**
     * @brief Set coefficient (e.g., thermal conductivity)
     */
    void setCoefficient(double coeff) {
        coefficient_ = coeff;
    }

    /**
     * @brief Get coefficient
     */
    double getCoefficient() const {
        return coefficient_;
    }

    /**
     * @brief Evaluate BC at given point and time
     */
    double evaluate(double x, double y, double z, double t) const override {
        if (isConstant_) {
            return constantFlux_;
        } else if (hasTimeFunc_) {
            return timeFunc_(t);
        } else if (hasSpatialFunc_) {
            return spatialFunc_(x, y, z);
        } else if (hasGeneralFunc_) {
            return generalFunc_(x, y, z, t);
        }
        return 0.0;
    }

    bool isTimeDependent() const override {
        return hasTimeFunc_ || hasGeneralFunc_;
    }

    bool isSpatiallyVarying() const override {
        return hasSpatialFunc_ || hasGeneralFunc_;
    }

    /**
     * @brief Check if this is a homogeneous BC (flux = 0)
     */
    bool isHomogeneous() const {
        return isConstant_ && std::abs(constantFlux_) < 1e-14;
    }

    /**
     * @brief Check if this represents insulation (zero flux)
     */
    bool isInsulated() const {
        return isHomogeneous();
    }

    std::string toString() const override {
        std::string result = BoundaryCondition::toString();
        if (isConstant_) {
            result += ", flux=" + std::to_string(constantFlux_);
        } else if (hasTimeFunc_) {
            result += ", time-dependent flux";
        } else if (hasSpatialFunc_) {
            result += ", spatially-varying flux";
        } else if (hasGeneralFunc_) {
            result += ", space-time-varying flux";
        }
        result += ", coefficient=" + std::to_string(coefficient_);
        return result;
    }

private:
    double constantFlux_;
    bool isConstant_;
    bool hasTimeFunc_;
    bool hasSpatialFunc_;
    bool hasGeneralFunc_;
    double coefficient_;  ///< Coefficient (e.g., thermal conductivity)

    std::function<double(double)> timeFunc_;
    std::function<double(double, double, double)> spatialFunc_;
    BCFunction generalFunc_;
};

/**
 * @brief Common Neumann BC patterns
 */
namespace neumann {

/**
 * @brief Zero flux BC (insulated/symmetry)
 */
inline std::shared_ptr<NeumannBC> makeZeroFlux(const std::string& name, int tag) {
    return NeumannBC::makeConstant(name, tag, 0.0);
}

/**
 * @brief Insulated boundary (alias for zero flux)
 */
inline std::shared_ptr<NeumannBC> makeInsulated(const std::string& name, int tag) {
    return makeZeroFlux(name, tag);
}

/**
 * @brief Constant heat flux BC
 */
inline std::shared_ptr<NeumannBC> makeHeatFlux(
    const std::string& name, int tag, double flux) {
    return NeumannBC::makeConstant(name, tag, flux);
}

/**
 * @brief Pulsating flux BC
 */
inline std::shared_ptr<NeumannBC> makePulsating(
    const std::string& name, int tag,
    double amplitude, double frequency, double phase = 0.0) {

    return NeumannBC::makeTimeDependent(name, tag,
        [amplitude, frequency, phase](double t) {
            return amplitude * std::sin(2.0 * M_PI * frequency * t + phase);
        });
}

/**
 * @brief Exponential decay flux
 */
inline std::shared_ptr<NeumannBC> makeExponentialDecay(
    const std::string& name, int tag,
    double initialFlux, double decayRate) {

    return NeumannBC::makeTimeDependent(name, tag,
        [initialFlux, decayRate](double t) {
            return initialFlux * std::exp(-decayRate * t);
        });
}

/**
 * @brief Gaussian spatial distribution
 */
inline std::shared_ptr<NeumannBC> makeGaussian(
    const std::string& name, int tag,
    double amplitude, double centerX, double centerY, double sigma) {

    return NeumannBC::makeSpatial(name, tag,
        [amplitude, centerX, centerY, sigma](double x, double y, double /*z*/) {
            double dx = x - centerX;
            double dy = y - centerY;
            double r2 = dx*dx + dy*dy;
            return amplitude * std::exp(-r2 / (2.0 * sigma * sigma));
        });
}

} // namespace neumann

} // namespace boundary
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_BOUNDARY_NEUMANN_BC_H
