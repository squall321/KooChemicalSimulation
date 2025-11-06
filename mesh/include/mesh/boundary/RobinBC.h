/**
 * @file RobinBC.h
 * @brief Robin (mixed) boundary conditions
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha3
 * @date 2025-11-06
 *
 * Robin BCs combine Dirichlet and Neumann conditions.
 * Example: α·u + β·∂u/∂n = γ on Γ_R
 */

#ifndef KOO_MESH_BOUNDARY_ROBIN_BC_H
#define KOO_MESH_BOUNDARY_ROBIN_BC_H

#include "BoundaryCondition.h"
#include <cmath>

namespace koo {
namespace mesh {
namespace boundary {

/**
 * @brief Robin boundary condition
 *
 * General form: α·u + β·∂u/∂n = γ on Γ_R
 *
 * Special cases:
 * - Dirichlet: β=0, α=1, γ=prescribed value
 * - Neumann: α=0, β=1, γ=prescribed flux
 * - Convection BC: α=h (heat transfer coeff), β=-k (conductivity), γ=h·T_inf
 */
class RobinBC : public BoundaryCondition {
public:
    /**
     * @brief Create constant Robin BC
     * @param alpha Coefficient for u term
     * @param beta Coefficient for ∂u/∂n term
     * @param gamma Right-hand side value
     */
    static std::shared_ptr<RobinBC> makeConstant(
        const std::string& name, int tag,
        double alpha, double beta, double gamma) {

        auto bc = std::make_shared<RobinBC>(name, tag);
        bc->setCoefficients(alpha, beta, gamma);
        return bc;
    }

    /**
     * @brief Create convection BC (Newton cooling)
     * Heat transfer: -k·∂T/∂n = h·(T - T_inf)
     * Rewritten: h·T + k·∂T/∂n = h·T_inf
     * @param h Heat transfer coefficient
     * @param k Thermal conductivity
     * @param T_inf Ambient temperature
     */
    static std::shared_ptr<RobinBC> makeConvection(
        const std::string& name, int tag,
        double h, double k, double T_inf) {

        auto bc = std::make_shared<RobinBC>(name, tag);
        bc->setCoefficients(h, k, h * T_inf);
        return bc;
    }

    /**
     * @brief Constructor
     */
    RobinBC(const std::string& name, int tag)
        : BoundaryCondition(name, BCType::ROBIN, tag),
          alpha_(1.0), beta_(1.0), gamma_(0.0),
          isConstant_(true),
          hasTimeFunc_(false),
          hasSpatialFunc_(false),
          hasGeneralFunc_(false) {}

    /**
     * @brief Set constant coefficients
     */
    void setCoefficients(double alpha, double beta, double gamma) {
        alpha_ = alpha;
        beta_ = beta;
        gamma_ = gamma;
        isConstant_ = true;
        hasTimeFunc_ = false;
        hasSpatialFunc_ = false;
        hasGeneralFunc_ = false;
    }

    /**
     * @brief Set time-dependent gamma function
     */
    void setTimeFunction(std::function<double(double)> func) {
        timeFunc_ = func;
        isConstant_ = false;
        hasTimeFunc_ = true;
        hasSpatialFunc_ = false;
        hasGeneralFunc_ = false;
    }

    /**
     * @brief Set spatial gamma function
     */
    void setSpatialFunction(std::function<double(double, double, double)> func) {
        spatialFunc_ = func;
        isConstant_ = false;
        hasTimeFunc_ = false;
        hasSpatialFunc_ = true;
        hasGeneralFunc_ = false;
    }

    /**
     * @brief Set general space-time gamma function
     */
    void setFunction(BCFunction func) {
        generalFunc_ = func;
        isConstant_ = false;
        hasTimeFunc_ = false;
        hasSpatialFunc_ = false;
        hasGeneralFunc_ = true;
    }

    /**
     * @brief Get alpha coefficient
     */
    double getAlpha() const { return alpha_; }

    /**
     * @brief Get beta coefficient
     */
    double getBeta() const { return beta_; }

    /**
     * @brief Get gamma value/function
     */
    double getGamma(double x, double y, double z, double t) const {
        if (isConstant_) {
            return gamma_;
        } else if (hasTimeFunc_) {
            return timeFunc_(t);
        } else if (hasSpatialFunc_) {
            return spatialFunc_(x, y, z);
        } else if (hasGeneralFunc_) {
            return generalFunc_(x, y, z, t);
        }
        return gamma_;
    }

    /**
     * @brief Set alpha coefficient
     */
    void setAlpha(double alpha) { alpha_ = alpha; }

    /**
     * @brief Set beta coefficient
     */
    void setBeta(double beta) { beta_ = beta; }

    /**
     * @brief Set constant gamma
     */
    void setGamma(double gamma) {
        gamma_ = gamma;
        isConstant_ = true;
    }

    /**
     * @brief Evaluate BC (returns gamma for compatibility)
     */
    double evaluate(double x, double y, double z, double t) const override {
        return getGamma(x, y, z, t);
    }

    bool isTimeDependent() const override {
        return hasTimeFunc_ || hasGeneralFunc_;
    }

    bool isSpatiallyVarying() const override {
        return hasSpatialFunc_ || hasGeneralFunc_;
    }

    /**
     * @brief Check if reduces to Dirichlet (beta = 0)
     */
    bool isDirichletLike() const {
        return std::abs(beta_) < 1e-14 && std::abs(alpha_) > 1e-14;
    }

    /**
     * @brief Check if reduces to Neumann (alpha = 0)
     */
    bool isNeumannLike() const {
        return std::abs(alpha_) < 1e-14 && std::abs(beta_) > 1e-14;
    }

    std::string toString() const override {
        std::string result = BoundaryCondition::toString();
        result += ", α=" + std::to_string(alpha_);
        result += ", β=" + std::to_string(beta_);
        if (isConstant_) {
            result += ", γ=" + std::to_string(gamma_);
        } else {
            result += ", γ=function";
        }
        return result;
    }

private:
    double alpha_;  ///< Coefficient for u
    double beta_;   ///< Coefficient for ∂u/∂n
    double gamma_;  ///< Constant RHS value

    bool isConstant_;
    bool hasTimeFunc_;
    bool hasSpatialFunc_;
    bool hasGeneralFunc_;

    std::function<double(double)> timeFunc_;
    std::function<double(double, double, double)> spatialFunc_;
    BCFunction generalFunc_;
};

/**
 * @brief Common Robin BC patterns
 */
namespace robin {

/**
 * @brief Convective heat transfer BC
 * -k·∂T/∂n = h·(T - T_inf)
 */
inline std::shared_ptr<RobinBC> makeConvection(
    const std::string& name, int tag,
    double heatTransferCoeff, double conductivity, double ambientTemp) {
    return RobinBC::makeConvection(name, tag, heatTransferCoeff, conductivity, ambientTemp);
}

/**
 * @brief Radiation BC (linearized)
 * -k·∂T/∂n = σ·ε·(T^4 - T_inf^4) ≈ h_rad·(T - T_inf)
 */
inline std::shared_ptr<RobinBC> makeRadiation(
    const std::string& name, int tag,
    double radiationCoeff, double conductivity, double ambientTemp) {
    return RobinBC::makeConvection(name, tag, radiationCoeff, conductivity, ambientTemp);
}

/**
 * @brief Impedance BC (for wave problems)
 * ∂u/∂n + Z·u = 0
 */
inline std::shared_ptr<RobinBC> makeImpedance(
    const std::string& name, int tag, double impedance) {
    return RobinBC::makeConstant(name, tag, impedance, 1.0, 0.0);
}

/**
 * @brief Absorbing BC (for wave problems)
 * ∂u/∂n + c·u = 0
 */
inline std::shared_ptr<RobinBC> makeAbsorbing(
    const std::string& name, int tag, double waveSpeed) {
    return RobinBC::makeConstant(name, tag, waveSpeed, 1.0, 0.0);
}

/**
 * @brief Time-varying convection BC
 */
inline std::shared_ptr<RobinBC> makeTimeVaryingConvection(
    const std::string& name, int tag,
    double h, double k,
    std::function<double(double)> ambientTempFunc) {

    auto bc = RobinBC::makeConvection(name, tag, h, k, 0.0);
    bc->setTimeFunction([h, ambientTempFunc](double t) {
        return h * ambientTempFunc(t);
    });
    return bc;
}

} // namespace robin

} // namespace boundary
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_BOUNDARY_ROBIN_BC_H
