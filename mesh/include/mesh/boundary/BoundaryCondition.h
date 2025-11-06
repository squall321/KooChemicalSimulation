/**
 * @file BoundaryCondition.h
 * @brief Abstract base class for boundary conditions
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha3
 * @date 2025-11-06
 *
 * Defines the interface for all boundary condition types.
 */

#ifndef KOO_MESH_BOUNDARY_BOUNDARY_CONDITION_H
#define KOO_MESH_BOUNDARY_BOUNDARY_CONDITION_H

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace koo {
namespace mesh {
namespace boundary {

/**
 * @brief Boundary condition types
 */
enum class BCType {
    DIRICHLET,     ///< Essential BC (prescribed value)
    NEUMANN,       ///< Natural BC (prescribed flux/derivative)
    ROBIN,         ///< Mixed BC (Robin/Cauchy)
    PERIODIC,      ///< Periodic BC
    INTERFACE      ///< Interface condition
};

/**
 * @brief Field component specification
 */
enum class Component {
    SCALAR,        ///< Scalar field
    VECTOR_X,      ///< X component of vector field
    VECTOR_Y,      ///< Y component of vector field
    VECTOR_Z,      ///< Z component of vector field
    VECTOR_ALL,    ///< All components
    TENSOR_XX,     ///< Tensor component
    TENSOR_YY,
    TENSOR_ZZ,
    TENSOR_XY,
    TENSOR_YZ,
    TENSOR_XZ
};

/**
 * @brief Function type for spatially and temporally varying BCs
 *
 * Arguments: x, y, z, t
 * Returns: BC value
 */
using BCFunction = std::function<double(double, double, double, double)>;

/**
 * @brief Abstract base class for boundary conditions
 */
class BoundaryCondition {
public:
    /**
     * @brief Constructor
     * @param name Unique name for this BC
     * @param type BC type
     * @param tag Physical tag from mesh
     */
    BoundaryCondition(const std::string& name, BCType type, int tag)
        : name_(name), type_(type), physicalTag_(tag),
          component_(Component::SCALAR), enabled_(true) {}

    virtual ~BoundaryCondition() = default;

    // Copy and move
    BoundaryCondition(const BoundaryCondition&) = default;
    BoundaryCondition& operator=(const BoundaryCondition&) = default;
    BoundaryCondition(BoundaryCondition&&) = default;
    BoundaryCondition& operator=(BoundaryCondition&&) = default;

    /**
     * @brief Evaluate BC at given point and time
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param t Time
     * @return BC value
     */
    virtual double evaluate(double x, double y, double z, double t) const = 0;

    /**
     * @brief Check if BC is time-dependent
     */
    virtual bool isTimeDependent() const = 0;

    /**
     * @brief Check if BC is spatially varying
     */
    virtual bool isSpatiallyVarying() const = 0;

    /**
     * @brief Get BC type
     */
    BCType getType() const { return type_; }

    /**
     * @brief Get BC name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Get physical tag
     */
    int getPhysicalTag() const { return physicalTag_; }

    /**
     * @brief Set component
     */
    void setComponent(Component comp) { component_ = comp; }

    /**
     * @brief Get component
     */
    Component getComponent() const { return component_; }

    /**
     * @brief Enable/disable this BC
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if BC is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Set field name this BC applies to
     */
    void setFieldName(const std::string& field) { fieldName_ = field; }

    /**
     * @brief Get field name
     */
    const std::string& getFieldName() const { return fieldName_; }

    /**
     * @brief Get string representation
     */
    virtual std::string toString() const {
        std::string result = "BC[" + name_ + "]: ";
        result += "type=" + typeToString(type_);
        result += ", tag=" + std::to_string(physicalTag_);
        result += ", field=" + fieldName_;
        result += ", enabled=" + std::string(enabled_ ? "yes" : "no");
        return result;
    }

protected:
    std::string name_;          ///< BC name
    BCType type_;               ///< BC type
    int physicalTag_;           ///< Physical tag from mesh
    Component component_;       ///< Field component
    bool enabled_;              ///< Whether BC is active
    std::string fieldName_;     ///< Name of field this BC applies to

    /**
     * @brief Convert BC type to string
     */
    static std::string typeToString(BCType type) {
        switch (type) {
            case BCType::DIRICHLET: return "Dirichlet";
            case BCType::NEUMANN:   return "Neumann";
            case BCType::ROBIN:     return "Robin";
            case BCType::PERIODIC:  return "Periodic";
            case BCType::INTERFACE: return "Interface";
            default:                return "Unknown";
        }
    }

    /**
     * @brief Convert component to string
     */
    static std::string componentToString(Component comp) {
        switch (comp) {
            case Component::SCALAR:     return "scalar";
            case Component::VECTOR_X:   return "x";
            case Component::VECTOR_Y:   return "y";
            case Component::VECTOR_Z:   return "z";
            case Component::VECTOR_ALL: return "all";
            case Component::TENSOR_XX:  return "xx";
            case Component::TENSOR_YY:  return "yy";
            case Component::TENSOR_ZZ:  return "zz";
            case Component::TENSOR_XY:  return "xy";
            case Component::TENSOR_YZ:  return "yz";
            case Component::TENSOR_XZ:  return "xz";
            default:                    return "unknown";
        }
    }
};

/**
 * @brief BC for constant value
 */
class ConstantBC : public BoundaryCondition {
public:
    ConstantBC(const std::string& name, BCType type, int tag, double value)
        : BoundaryCondition(name, type, tag), value_(value) {}

    double evaluate(double /*x*/, double /*y*/, double /*z*/, double /*t*/) const override {
        return value_;
    }

    bool isTimeDependent() const override { return false; }
    bool isSpatiallyVarying() const override { return false; }

    double getValue() const { return value_; }
    void setValue(double value) { value_ = value; }

    std::string toString() const override {
        return BoundaryCondition::toString() + ", value=" + std::to_string(value_);
    }

private:
    double value_;
};

/**
 * @brief BC for time-dependent value
 */
class TimeDependentBC : public BoundaryCondition {
public:
    using TimeFunction = std::function<double(double)>;

    TimeDependentBC(const std::string& name, BCType type, int tag, TimeFunction func)
        : BoundaryCondition(name, type, tag), timeFunc_(func) {}

    double evaluate(double /*x*/, double /*y*/, double /*z*/, double t) const override {
        return timeFunc_(t);
    }

    bool isTimeDependent() const override { return true; }
    bool isSpatiallyVarying() const override { return false; }

private:
    TimeFunction timeFunc_;
};

/**
 * @brief BC for spatially varying value
 */
class SpatialBC : public BoundaryCondition {
public:
    using SpatialFunction = std::function<double(double, double, double)>;

    SpatialBC(const std::string& name, BCType type, int tag, SpatialFunction func)
        : BoundaryCondition(name, type, tag), spatialFunc_(func) {}

    double evaluate(double x, double y, double z, double /*t*/) const override {
        return spatialFunc_(x, y, z);
    }

    bool isTimeDependent() const override { return false; }
    bool isSpatiallyVarying() const override { return true; }

private:
    SpatialFunction spatialFunc_;
};

/**
 * @brief BC for general space-time varying value
 */
class GeneralBC : public BoundaryCondition {
public:
    GeneralBC(const std::string& name, BCType type, int tag, BCFunction func)
        : BoundaryCondition(name, type, tag), func_(func) {}

    double evaluate(double x, double y, double z, double t) const override {
        return func_(x, y, z, t);
    }

    bool isTimeDependent() const override { return true; }
    bool isSpatiallyVarying() const override { return true; }

private:
    BCFunction func_;
};

} // namespace boundary
} // namespace mesh
} // namespace koo

#endif // KOO_MESH_BOUNDARY_BOUNDARY_CONDITION_H
