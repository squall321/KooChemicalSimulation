/**
 * @file DirichletBC.h
 * @brief Dirichlet (essential) boundary conditions
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha3
 * @date 2025-11-06
 *
 * Dirichlet BCs prescribe the value of the solution on the boundary.
 * Example: u = g on Γ_D
 */

#ifndef KOO_MESH_BOUNDARY_DIRICHLET_BC_H
#define KOO_MESH_BOUNDARY_DIRICHLET_BC_H

#include "BoundaryCondition.h"
#include <vector>
#include <cmath>

namespace koo {
namespace mesh {
namespace boundary {

/**
 * @brief Dirichlet boundary condition
 *
 * Prescribes the value of the solution on the boundary:
 *   u(x,t) = g(x,t) on Γ_D
 */
class DirichletBC : public BoundaryCondition {
public:
    /**
     * @brief Create constant Dirichlet BC
     */
    static std::shared_ptr<DirichletBC> makeConstant(
        const std::string& name, int tag, double value) {

        auto bc = std::make_shared<DirichletBC>(name, tag);
        bc->setValue(value);
        return bc;
    }

    /**
     * @brief Create time-dependent Dirichlet BC
     */
    static std::shared_ptr<DirichletBC> makeTimeDependent(
        const std::string& name, int tag,
        std::function<double(double)> timeFunc) {

        auto bc = std::make_shared<DirichletBC>(name, tag);
        bc->setTimeFunction(timeFunc);
        return bc;
    }

    /**
     * @brief Create spatially varying Dirichlet BC
     */
    static std::shared_ptr<DirichletBC> makeSpatial(
        const std::string& name, int tag,
        std::function<double(double, double, double)> spatialFunc) {

        auto bc = std::make_shared<DirichletBC>(name, tag);
        bc->setSpatialFunction(spatialFunc);
        return bc;
    }

    /**
     * @brief Create general space-time varying Dirichlet BC
     */
    static std::shared_ptr<DirichletBC> makeGeneral(
        const std::string& name, int tag, BCFunction func) {

        auto bc = std::make_shared<DirichletBC>(name, tag);
        bc->setFunction(func);
        return bc;
    }

    /**
     * @brief Constructor
     */
    DirichletBC(const std::string& name, int tag)
        : BoundaryCondition(name, BCType::DIRICHLET, tag),
          constantValue_(0.0),
          isConstant_(true),
          hasTimeFunc_(false),
          hasSpatialFunc_(false),
          hasGeneralFunc_(false) {}

    /**
     * @brief Set constant value
     */
    void setValue(double value) {
        constantValue_ = value;
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
     * @brief Evaluate BC at given point and time
     */
    double evaluate(double x, double y, double z, double t) const override {
        if (isConstant_) {
            return constantValue_;
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
     * @brief Check if this is a homogeneous BC (value = 0)
     */
    bool isHomogeneous() const {
        return isConstant_ && std::abs(constantValue_) < 1e-14;
    }

    std::string toString() const override {
        std::string result = BoundaryCondition::toString();
        if (isConstant_) {
            result += ", value=" + std::to_string(constantValue_);
        } else if (hasTimeFunc_) {
            result += ", time-dependent";
        } else if (hasSpatialFunc_) {
            result += ", spatially-varying";
        } else if (hasGeneralFunc_) {
            result += ", space-time-varying";
        }
        return result;
    }

private:
    double constantValue_;
    bool isConstant_;
    bool hasTimeFunc_;
    bool hasSpatialFunc_;
    bool hasGeneralFunc_;

    std::function<double(double)> timeFunc_;
    std::function<double(double, double, double)> spatialFunc_;
    BCFunction generalFunc_;
};

/**
 * @brief Common Dirichlet BC patterns
 */
namespace dirichlet {

/**
 * @brief Zero Dirichlet BC (homogeneous)
 */
inline std::shared_ptr<DirichletBC> makeZero(const std::string& name, int tag) {
    return DirichletBC::makeConstant(name, tag, 0.0);
}

/**
 * @brief Sinusoidal time-varying BC
 */
inline std::shared_ptr<DirichletBC> makeSinusoidal(
    const std::string& name, int tag,
    double amplitude, double frequency, double phase = 0.0) {

    return DirichletBC::makeTimeDependent(name, tag,
        [amplitude, frequency, phase](double t) {
            return amplitude * std::sin(2.0 * M_PI * frequency * t + phase);
        });
}

/**
 * @brief Linear ramp BC
 */
inline std::shared_ptr<DirichletBC> makeRamp(
    const std::string& name, int tag,
    double startValue, double endValue, double duration) {

    return DirichletBC::makeTimeDependent(name, tag,
        [startValue, endValue, duration](double t) {
            if (t >= duration) return endValue;
            if (t <= 0.0) return startValue;
            return startValue + (endValue - startValue) * t / duration;
        });
}

/**
 * @brief Step function BC
 */
inline std::shared_ptr<DirichletBC> makeStep(
    const std::string& name, int tag,
    double beforeValue, double afterValue, double stepTime) {

    return DirichletBC::makeTimeDependent(name, tag,
        [beforeValue, afterValue, stepTime](double t) {
            return (t < stepTime) ? beforeValue : afterValue;
        });
}

/**
 * @brief Parabolic profile (e.g., for inlet velocity)
 */
inline std::shared_ptr<DirichletBC> makeParabolicProfile(
    const std::string& name, int tag,
    double maxValue, double center, double width, char axis = 'y') {

    return DirichletBC::makeSpatial(name, tag,
        [maxValue, center, width, axis](double x, double y, double z) {
            double coord = (axis == 'x') ? x : (axis == 'y') ? y : z;
            double r = (coord - center) / width;
            return maxValue * (1.0 - r * r);
        });
}

} // namespace dirichlet

} // namespace boundary
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_BOUNDARY_DIRICHLET_BC_H
