#ifndef KOO_VELOCITY_FIELD_H
#define KOO_VELOCITY_FIELD_H

#include <array>
#include <vector>
#include <functional>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace physics {
namespace transport {

/**
 * @class VelocityField
 * @brief Represents a velocity field for transport phenomena
 *
 * Phase 22: Transport Phenomena (v0.5.0-alpha2)
 *
 * Supports:
 * - Uniform velocity fields
 * - Spatially varying velocity fields
 * - Time-dependent velocity fields
 */
class VelocityField {
public:
    using Vector3D = std::array<double, 3>;
    using VelocityFunction = std::function<Vector3D(const Vector3D&, double)>;

    /**
     * @brief Default constructor (zero velocity)
     */
    VelocityField()
        : uniform_(true),
          velocity_({0.0, 0.0, 0.0}),
          velocityFunction_(nullptr) {}

    /**
     * @brief Constructor with uniform velocity
     * @param velocity Uniform velocity vector [vx, vy, vz]
     */
    explicit VelocityField(const Vector3D& velocity)
        : uniform_(true),
          velocity_(velocity),
          velocityFunction_(nullptr) {}

    /**
     * @brief Constructor with velocity function
     * @param velocityFunc Function v(x,y,z,t) -> [vx, vy, vz]
     */
    explicit VelocityField(VelocityFunction velocityFunc)
        : uniform_(false),
          velocity_({0.0, 0.0, 0.0}),
          velocityFunction_(velocityFunc) {}

    /**
     * @brief Get velocity at a point
     * @param position Position vector [x, y, z]
     * @param time Current time
     * @return Velocity vector [vx, vy, vz]
     */
    Vector3D getVelocity(const Vector3D& position, double time = 0.0) const {
        if (uniform_) {
            return velocity_;
        } else if (velocityFunction_) {
            return velocityFunction_(position, time);
        }
        return {0.0, 0.0, 0.0};
    }

    /**
     * @brief Check if velocity field is uniform
     */
    bool isUniform() const { return uniform_; }

    /**
     * @brief Get magnitude of velocity
     */
    double getMagnitude(const Vector3D& position, double time = 0.0) const {
        auto v = getVelocity(position, time);
        return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    }

    /**
     * @brief Set uniform velocity
     */
    void setUniformVelocity(const Vector3D& velocity) {
        uniform_ = true;
        velocity_ = velocity;
        velocityFunction_ = nullptr;
    }

    /**
     * @brief Set velocity function
     */
    void setVelocityFunction(VelocityFunction func) {
        uniform_ = false;
        velocityFunction_ = func;
    }

private:
    bool uniform_;                      ///< True if velocity is uniform
    Vector3D velocity_;                 ///< Uniform velocity vector
    VelocityFunction velocityFunction_; ///< Spatially/temporally varying velocity
};

/**
 * @class Advection
 * @brief Advection/convection operator for transport equations
 *
 * Implements the advection term: v·∇C
 *
 * Discretization schemes:
 * - Upwind (first-order, stable but diffusive)
 * - Central (second-order, may oscillate)
 * - QUICK (third-order, Quadratic Upstream Interpolation)
 */
class Advection {
public:
    enum class Scheme {
        UPWIND,   ///< First-order upwind (stable, diffusive)
        CENTRAL,  ///< Second-order central (accurate, may oscillate)
        QUICK     ///< Third-order QUICK (accurate, bounded)
    };

    /**
     * @brief Constructor
     * @param velocityField Velocity field for advection
     * @param scheme Discretization scheme
     */
    explicit Advection(const VelocityField& velocityField,
                      Scheme scheme = Scheme::UPWIND)
        : velocityField_(velocityField),
          scheme_(scheme) {}

    /**
     * @brief Calculate advection term: v·∇C
     * @param gradient Concentration gradient [∂C/∂x, ∂C/∂y, ∂C/∂z]
     * @param position Position vector
     * @param time Current time
     * @return Advection term value
     */
    double calculateAdvectionTerm(const std::array<double, 3>& gradient,
                                  const std::array<double, 3>& position,
                                  double time = 0.0) const {
        auto v = velocityField_.getVelocity(position, time);
        return v[0] * gradient[0] + v[1] * gradient[1] + v[2] * gradient[2];
    }

    /**
     * @brief Calculate material derivative: DC/Dt = ∂C/∂t + v·∇C
     * @param dCdt Time derivative ∂C/∂t
     * @param gradient Spatial gradient ∇C
     * @param position Position vector
     * @param time Current time
     * @return Material derivative
     */
    double calculateMaterialDerivative(double dCdt,
                                      const std::array<double, 3>& gradient,
                                      const std::array<double, 3>& position,
                                      double time = 0.0) const {
        return dCdt + calculateAdvectionTerm(gradient, position, time);
    }

    /**
     * @brief Get Courant number: Co = |v|Δt/Δx
     * @param velocity Velocity magnitude
     * @param dt Time step
     * @param dx Grid spacing
     * @return Courant number
     */
    static double getCourantNumber(double velocity, double dt, double dx) {
        if (dx <= 0.0) {
            throw std::invalid_argument("Grid spacing must be positive");
        }
        return std::abs(velocity) * dt / dx;
    }

    /**
     * @brief Check CFL stability condition: Co ≤ 1
     * @param velocity Velocity magnitude
     * @param dt Time step
     * @param dx Grid spacing
     * @return True if stable
     */
    static bool isCFLStable(double velocity, double dt, double dx) {
        return getCourantNumber(velocity, dt, dx) <= 1.0;
    }

    /**
     * @brief Get maximum stable time step: Δt_max = Δx/|v|
     */
    static double getMaxTimeStep(double velocity, double dx) {
        if (std::abs(velocity) < 1.0e-15) {
            return 1.0e10; // Very large for zero velocity
        }
        return dx / std::abs(velocity);
    }

    /**
     * @brief Set discretization scheme
     */
    void setScheme(Scheme scheme) { scheme_ = scheme; }

    /**
     * @brief Get discretization scheme
     */
    Scheme getScheme() const { return scheme_; }

    /**
     * @brief Get velocity field
     */
    const VelocityField& getVelocityField() const { return velocityField_; }

private:
    VelocityField velocityField_; ///< Velocity field
    Scheme scheme_;               ///< Discretization scheme
};

} // namespace transport
} // namespace physics
} // namespace koo

#endif // KOO_VELOCITY_FIELD_H
