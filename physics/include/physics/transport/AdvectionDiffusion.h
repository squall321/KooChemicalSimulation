#ifndef KOO_ADVECTION_DIFFUSION_H
#define KOO_ADVECTION_DIFFUSION_H

#include "VelocityField.h"
#include "../diffusion/DiffusionCoefficient.h"
#include "../diffusion/FickDiffusion.h"
#include <array>
#include <memory>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace physics {
namespace transport {

/**
 * @class AdvectionDiffusion
 * @brief Combined advection-diffusion transport equation
 *
 * Phase 22: Transport Phenomena (v0.5.0-alpha2)
 *
 * Full transport equation:
 *   ∂C/∂t + v·∇C = D∇²C + S
 *
 * Where:
 * - ∂C/∂t: Accumulation (time rate of change)
 * - v·∇C: Advection/convection (transport by flow)
 * - D∇²C: Diffusion (transport by concentration gradients)
 * - S: Source/sink term
 *
 * Dimensionless analysis:
 * - Peclet number Pe = vL/D (advection vs diffusion)
 *   Pe << 1: Diffusion-dominated
 *   Pe >> 1: Advection-dominated
 */
class AdvectionDiffusion {
public:
    /**
     * @brief Constructor
     * @param velocityField Velocity field for advection
     * @param diffusionCoeff Diffusion coefficient model
     */
    AdvectionDiffusion(const VelocityField& velocityField,
                      std::shared_ptr<koo::physics::DiffusionCoefficient> diffusionCoeff)
        : advection_(velocityField),
          diffusion_(diffusionCoeff) {}

    /**
     * @brief Calculate right-hand side of transport equation
     *
     * RHS = -v·∇C + D∇²C + S
     *
     * @param gradient Concentration gradient [∂C/∂x, ∂C/∂y, ∂C/∂z]
     * @param laplacian Concentration Laplacian ∇²C
     * @param sourceTerm Source term S
     * @param position Position vector
     * @param temperature Temperature
     * @param concentration Current concentration
     * @param time Current time
     * @return dC/dt
     */
    double calculateRHS(const std::array<double, 3>& gradient,
                       double laplacian,
                       double sourceTerm,
                       const std::array<double, 3>& position,
                       double temperature,
                       double concentration = 0.0,
                       double time = 0.0) const {
        // Advection term: -v·∇C (negative because of convention)
        double advectionTerm = -advection_.calculateAdvectionTerm(gradient, position, time);

        // Diffusion term: D∇²C
        double diffusionTerm = diffusion_.calculateSourceTerm(laplacian, temperature, concentration);

        return advectionTerm + diffusionTerm + sourceTerm;
    }

    /**
     * @brief Calculate Peclet number at a point
     *
     * Pe = vL/D
     *
     * @param characteristicLength Characteristic length scale
     * @param position Position vector
     * @param temperature Temperature
     * @param concentration Current concentration
     * @param time Current time
     * @return Peclet number
     */
    double getPecletNumber(double characteristicLength,
                          const std::array<double, 3>& position,
                          double temperature,
                          double concentration = 0.0,
                          double time = 0.0) const {
        return diffusion_.getPecletNumber(
            advection_.getVelocityField().getMagnitude(position, time),
            characteristicLength,
            temperature,
            concentration
        );
    }

    /**
     * @brief Check if transport is diffusion-dominated
     * @param threshold Peclet number threshold (default: 0.1)
     * @return True if Pe < threshold
     */
    bool isDiffusionDominated(double characteristicLength,
                             const std::array<double, 3>& position,
                             double temperature,
                             double concentration = 0.0,
                             double time = 0.0,
                             double threshold = 0.1) const {
        return getPecletNumber(characteristicLength, position, temperature,
                              concentration, time) < threshold;
    }

    /**
     * @brief Check if transport is advection-dominated
     * @param threshold Peclet number threshold (default: 10.0)
     * @return True if Pe > threshold
     */
    bool isAdvectionDominated(double characteristicLength,
                             const std::array<double, 3>& position,
                             double temperature,
                             double concentration = 0.0,
                             double time = 0.0,
                             double threshold = 10.0) const {
        return getPecletNumber(characteristicLength, position, temperature,
                              concentration, time) > threshold;
    }

    /**
     * @brief Get characteristic time for advection: τ_adv = L/v
     */
    double getAdvectionTime(double characteristicLength,
                           const std::array<double, 3>& position,
                           double time = 0.0) const {
        double velocity = advection_.getVelocityField().getMagnitude(position, time);
        if (velocity < 1.0e-15) {
            return 1.0e10; // Very large for zero velocity
        }
        return characteristicLength / velocity;
    }

    /**
     * @brief Get characteristic time for diffusion: τ_diff = L²/D
     */
    double getDiffusionTime(double characteristicLength,
                           double temperature,
                           double concentration = 0.0) const {
        return diffusion_.getDiffusionTime(characteristicLength, temperature, concentration);
    }

    /**
     * @brief Get time scale ratio: τ_diff/τ_adv = Pe
     */
    double getTimeScaleRatio(double characteristicLength,
                            const std::array<double, 3>& position,
                            double temperature,
                            double concentration = 0.0,
                            double time = 0.0) const {
        return getPecletNumber(characteristicLength, position, temperature,
                              concentration, time);
    }

    /**
     * @brief Operator splitting: advection step
     *
     * Updates concentration using pure advection:
     *   ∂C/∂t = -v·∇C
     *
     * @param dt Time step
     * @param gradient Concentration gradient
     * @param currentConcentration Current concentration
     * @param position Position vector
     * @param time Current time
     * @return Updated concentration
     */
    double advectionStep(double dt,
                        const std::array<double, 3>& gradient,
                        double currentConcentration,
                        const std::array<double, 3>& position,
                        double time = 0.0) const {
        double advectionTerm = -advection_.calculateAdvectionTerm(gradient, position, time);
        return currentConcentration + dt * advectionTerm;
    }

    /**
     * @brief Operator splitting: diffusion step
     *
     * Updates concentration using pure diffusion:
     *   ∂C/∂t = D∇²C
     *
     * @param dt Time step
     * @param laplacian Concentration Laplacian
     * @param currentConcentration Current concentration
     * @param temperature Temperature
     * @return Updated concentration
     */
    double diffusionStep(double dt,
                        double laplacian,
                        double currentConcentration,
                        double temperature) const {
        double diffusionTerm = diffusion_.calculateSourceTerm(laplacian, temperature,
                                                              currentConcentration);
        return currentConcentration + dt * diffusionTerm;
    }

    /**
     * @brief Strang splitting step
     *
     * Second-order accurate operator splitting:
     *   C^(n+1) = D(Δt/2) ∘ A(Δt) ∘ D(Δt/2) [C^n]
     *
     * Where D = diffusion operator, A = advection operator
     */
    double strangSplittingStep(double dt,
                              const std::array<double, 3>& gradient,
                              double laplacian,
                              double currentConcentration,
                              const std::array<double, 3>& position,
                              double temperature,
                              double time = 0.0) const {
        // Half diffusion step
        double C1 = diffusionStep(0.5 * dt, laplacian, currentConcentration, temperature);

        // Full advection step
        double C2 = advectionStep(dt, gradient, C1, position, time);

        // Half diffusion step
        double C3 = diffusionStep(0.5 * dt, laplacian, C2, temperature);

        return C3;
    }

    /**
     * @brief Get advection operator
     */
    const Advection& getAdvection() const { return advection_; }

    /**
     * @brief Get diffusion operator
     */
    const koo::physics::FickDiffusion& getDiffusion() const { return diffusion_; }

private:
    Advection advection_;                  ///< Advection operator
    koo::physics::FickDiffusion diffusion_; ///< Diffusion operator
};

} // namespace transport
} // namespace physics
} // namespace koo

#endif // KOO_ADVECTION_DIFFUSION_H
