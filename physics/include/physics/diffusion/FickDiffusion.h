/**
 * @file FickDiffusion.h
 * @brief Fick's law of diffusion
 * @author KooChemicalSimulation Development Team
 * @version 0.5.0-alpha1
 * @date 2025-11-06
 *
 * Implements Fick's first and second laws of diffusion.
 */

#ifndef KOO_PHYSICS_FICK_DIFFUSION_H
#define KOO_PHYSICS_FICK_DIFFUSION_H

#include "DiffusionCoefficient.h"
#include <memory>
#include <vector>
#include <array>
#include <cmath>
#include <string>

namespace koo {
namespace physics {

/**
 * @brief Fick's law implementation
 *
 * Fick's first law (flux):
 *   J = -D × ∇C
 *
 * Fick's second law (time evolution):
 *   ∂C/∂t = ∇·(D × ∇C)
 *
 * For constant D:
 *   ∂C/∂t = D × ∇²C
 */
class FickDiffusion {
public:
    /**
     * @brief Constructor with diffusion coefficient
     */
    explicit FickDiffusion(std::shared_ptr<DiffusionCoefficient> diffCoeff)
        : diffCoeff_(diffCoeff) {}

    /**
     * @brief Constructor with constant diffusion coefficient
     */
    explicit FickDiffusion(double D0)
        : diffCoeff_(std::make_shared<ConstantDiffusion>(D0)) {}

    /**
     * @brief Calculate diffusive flux (Fick's first law)
     *
     * J = -D × ∇C
     *
     * @param gradient Concentration gradient ∇C (mol/m⁴)
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Flux vector (mol/(m²·s))
     */
    std::array<double, 3> calculateFlux(
        const std::array<double, 3>& gradient,
        double T, double C = 0.0) const {

        double D = diffCoeff_->calculate(T, C);

        return {
            -D * gradient[0],
            -D * gradient[1],
            -D * gradient[2]
        };
    }

    /**
     * @brief Calculate magnitude of diffusive flux
     *
     * |J| = D × |∇C|
     *
     * @param gradientMagnitude Magnitude of concentration gradient
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Flux magnitude (mol/(m²·s))
     */
    double calculateFluxMagnitude(
        double gradientMagnitude,
        double T, double C = 0.0) const {

        double D = diffCoeff_->calculate(T, C);
        return D * gradientMagnitude;
    }

    /**
     * @brief Calculate diffusion source term (Fick's second law)
     *
     * For constant D:
     *   ∂C/∂t = D × ∇²C
     *
     * @param laplacian Laplacian of concentration ∇²C (mol/m⁵)
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Time derivative ∂C/∂t (mol/(m³·s))
     */
    double calculateSourceTerm(
        double laplacian,
        double T, double C = 0.0) const {

        double D = diffCoeff_->calculate(T, C);
        return D * laplacian;
    }

    /**
     * @brief Calculate characteristic diffusion time
     *
     * τ_diff = L² / D
     *
     * @param length Characteristic length scale (m)
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Diffusion time (s)
     */
    double getDiffusionTime(
        double length,
        double T, double C = 0.0) const {

        double D = diffCoeff_->calculate(T, C);
        if (D < 1.0e-30) {
            throw std::runtime_error("Diffusion coefficient too small");
        }
        return (length * length) / D;
    }

    /**
     * @brief Calculate diffusion length
     *
     * L_diff = √(D × t)
     *
     * @param time Time (s)
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Diffusion length (m)
     */
    double getDiffusionLength(
        double time,
        double T, double C = 0.0) const {

        double D = diffCoeff_->calculate(T, C);
        return std::sqrt(D * time);
    }

    /**
     * @brief Calculate Peclet number
     *
     * Pe = v × L / D
     *
     * @param velocity Characteristic velocity (m/s)
     * @param length Characteristic length (m)
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Peclet number (dimensionless)
     */
    double getPecletNumber(
        double velocity, double length,
        double T, double C = 0.0) const {

        double D = diffCoeff_->calculate(T, C);
        if (D < 1.0e-30) {
            throw std::runtime_error("Diffusion coefficient too small");
        }
        return (velocity * length) / D;
    }

    /**
     * @brief Get diffusion coefficient at given conditions
     */
    double getDiffusionCoefficient(double T, double C = 0.0) const {
        return diffCoeff_->calculate(T, C);
    }

    /**
     * @brief Get diffusion coefficient model
     */
    const DiffusionCoefficient& getDiffusionCoefficientModel() const {
        return *diffCoeff_;
    }

    /**
     * @brief Get model name
     */
    std::string getModelName() const {
        return "Fick's Law (" + diffCoeff_->getName() + ")";
    }

private:
    std::shared_ptr<DiffusionCoefficient> diffCoeff_;  ///< Diffusion coefficient model
};

/**
 * @brief Anisotropic Fick diffusion
 *
 * Uses diffusion tensor for directionally dependent diffusion.
 *
 * Fick's second law with tensor:
 *   ∂C/∂t = ∇·(D̅ × ∇C)
 *
 * where D̅ is the diffusion tensor.
 */
class AnisotropicFickDiffusion {
public:
    /**
     * @brief Constructor with anisotropic diffusion
     */
    explicit AnisotropicFickDiffusion(const AnisotropicDiffusion& diffTensor)
        : diffTensor_(diffTensor) {}

    /**
     * @brief Constructor with diagonal components
     */
    AnisotropicFickDiffusion(double Dx, double Dy, double Dz)
        : diffTensor_(Dx, Dy, Dz) {}

    /**
     * @brief Calculate anisotropic flux
     *
     * J_i = -D_ij × ∂C/∂x_j
     *
     * For diagonal tensor:
     *   Jx = -Dx × ∂C/∂x
     *   Jy = -Dy × ∂C/∂y
     *   Jz = -Dz × ∂C/∂z
     */
    std::array<double, 3> calculateFlux(
        const std::array<double, 3>& gradient) const {

        auto D = diffTensor_.getDiagonal();

        return {
            -D[0] * gradient[0],
            -D[1] * gradient[1],
            -D[2] * gradient[2]
        };
    }

    /**
     * @brief Calculate anisotropic source term
     *
     * For diagonal tensor:
     *   ∂C/∂t = Dx×∂²C/∂x² + Dy×∂²C/∂y² + Dz×∂²C/∂z²
     */
    double calculateSourceTerm(
        const std::array<double, 3>& secondDerivatives) const {

        auto D = diffTensor_.getDiagonal();

        return D[0] * secondDerivatives[0] +
               D[1] * secondDerivatives[1] +
               D[2] * secondDerivatives[2];
    }

    /**
     * @brief Get diffusion tensor
     */
    const AnisotropicDiffusion& getDiffusionTensor() const {
        return diffTensor_;
    }

    /**
     * @brief Check if effectively isotropic
     */
    bool isIsotropic() const {
        return diffTensor_.isIsotropic();
    }

    /**
     * @brief Get effective diffusion coefficient
     */
    double getEffectiveDiffusion() const {
        return diffTensor_.getEffective();
    }

private:
    AnisotropicDiffusion diffTensor_;  ///< Diffusion tensor
};

} // namespace physics
} // namespace koo

#endif // KOO_PHYSICS_FICK_DIFFUSION_H
