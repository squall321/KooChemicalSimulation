/**
 * @file DiffusionCoefficient.h
 * @brief Diffusion coefficient models
 * @author KooChemicalSimulation Development Team
 * @version 0.5.0-alpha1
 * @date 2025-11-06
 *
 * Provides various diffusion coefficient models including constant,
 * temperature-dependent, concentration-dependent, and anisotropic.
 */

#ifndef KOO_PHYSICS_DIFFUSION_COEFFICIENT_H
#define KOO_PHYSICS_DIFFUSION_COEFFICIENT_H

#include <array>
#include <cmath>
#include <string>
#include <stdexcept>

namespace koo {
namespace physics {

/**
 * @brief Base class for diffusion coefficient models
 *
 * D can depend on:
 * - Temperature T
 * - Concentration C
 * - Position (for anisotropic media)
 */
class DiffusionCoefficient {
public:
    virtual ~DiffusionCoefficient() = default;

    /**
     * @brief Calculate diffusion coefficient
     * @param T Temperature (K)
     * @param C Concentration (mol/m³)
     * @return Diffusion coefficient (m²/s)
     */
    virtual double calculate(double T, double C = 0.0) const = 0;

    /**
     * @brief Get model name
     */
    virtual std::string getName() const = 0;
};

/**
 * @brief Constant diffusion coefficient
 *
 * D = D₀ (independent of T and C)
 */
class ConstantDiffusion : public DiffusionCoefficient {
public:
    explicit ConstantDiffusion(double D0) : D0_(D0) {
        if (D0 < 0.0) {
            throw std::invalid_argument("Diffusion coefficient must be non-negative");
        }
    }

    double calculate(double T, double C = 0.0) const override {
        (void)T; (void)C;  // Unused
        return D0_;
    }

    std::string getName() const override {
        return "Constant";
    }

    double getD0() const { return D0_; }

private:
    double D0_;  ///< Constant diffusion coefficient (m²/s)
};

/**
 * @brief Arrhenius temperature-dependent diffusion
 *
 * D = D₀ × exp(-Ea/(R×T))
 *
 * Common for thermally activated diffusion processes.
 */
class ArrheniusDiffusion : public DiffusionCoefficient {
public:
    ArrheniusDiffusion(double D0, double Ea)
        : D0_(D0), Ea_(Ea) {
        if (D0 < 0.0) {
            throw std::invalid_argument("D0 must be non-negative");
        }
    }

    double calculate(double T, double C = 0.0) const override {
        (void)C;  // Unused
        const double R = 8.314;  // J/(mol·K)
        return D0_ * std::exp(-Ea_ / (R * T));
    }

    std::string getName() const override {
        return "Arrhenius";
    }

    double getD0() const { return D0_; }
    double getEa() const { return Ea_; }

private:
    double D0_;  ///< Pre-exponential factor (m²/s)
    double Ea_;  ///< Activation energy (J/mol)
};

/**
 * @brief Chapman-Enskog diffusion for gas mixtures
 *
 * D = (3/16) × √(2πkT(m1+m2)/(m1×m2)) / (π×σ²×Ω)
 *
 * Simplified: D = A × T^(3/2) / P
 */
class ChapmanEnskogDiffusion : public DiffusionCoefficient {
public:
    ChapmanEnskogDiffusion(double A, double P)
        : A_(A), P_(P) {}

    double calculate(double T, double C = 0.0) const override {
        (void)C;  // Unused
        return A_ * std::pow(T, 1.5) / P_;
    }

    std::string getName() const override {
        return "Chapman-Enskog";
    }

private:
    double A_;  ///< Pre-factor
    double P_;  ///< Pressure (Pa)
};

/**
 * @brief Concentration-dependent diffusion
 *
 * D = D₀ × (1 + α×C + β×C²)
 *
 * Common in non-ideal solutions.
 */
class ConcentrationDependentDiffusion : public DiffusionCoefficient {
public:
    ConcentrationDependentDiffusion(double D0, double alpha = 0.0, double beta = 0.0)
        : D0_(D0), alpha_(alpha), beta_(beta) {
        if (D0 < 0.0) {
            throw std::invalid_argument("D0 must be non-negative");
        }
    }

    double calculate(double T, double C = 0.0) const override {
        (void)T;  // Unused
        double factor = 1.0 + alpha_ * C + beta_ * C * C;
        return D0_ * factor;
    }

    std::string getName() const override {
        return "Concentration-Dependent";
    }

    double getD0() const { return D0_; }
    double getAlpha() const { return alpha_; }
    double getBeta() const { return beta_; }

private:
    double D0_;     ///< Base diffusion coefficient (m²/s)
    double alpha_;  ///< Linear coefficient (m³/mol)
    double beta_;   ///< Quadratic coefficient (m⁶/mol²)
};

/**
 * @brief Anisotropic diffusion tensor
 *
 * D is a 3×3 tensor for directionally dependent diffusion.
 * In principal axes: D = diag(Dx, Dy, Dz)
 */
class AnisotropicDiffusion {
public:
    /**
     * @brief Default constructor (isotropic)
     */
    AnisotropicDiffusion()
        : Dx_(1.0e-9), Dy_(1.0e-9), Dz_(1.0e-9) {}

    /**
     * @brief Constructor with principal components
     */
    AnisotropicDiffusion(double Dx, double Dy, double Dz)
        : Dx_(Dx), Dy_(Dy), Dz_(Dz) {
        if (Dx < 0.0 || Dy < 0.0 || Dz < 0.0) {
            throw std::invalid_argument("Diffusion coefficients must be non-negative");
        }
    }

    /**
     * @brief Get diffusion tensor as diagonal components
     * @return Array [Dx, Dy, Dz]
     */
    std::array<double, 3> getDiagonal() const {
        return {Dx_, Dy_, Dz_};
    }

    /**
     * @brief Get full 3x3 tensor (diagonal for principal axes)
     * @return 3x3 array
     */
    std::array<std::array<double, 3>, 3> getTensor() const {
        return {{
            {{Dx_, 0.0, 0.0}},
            {{0.0, Dy_, 0.0}},
            {{0.0, 0.0, Dz_}}
        }};
    }

    /**
     * @brief Check if isotropic
     */
    bool isIsotropic() const {
        const double tol = 1.0e-12;
        return (std::abs(Dx_ - Dy_) < tol) && (std::abs(Dy_ - Dz_) < tol);
    }

    /**
     * @brief Get effective (average) diffusion coefficient
     */
    double getEffective() const {
        return (Dx_ + Dy_ + Dz_) / 3.0;
    }

    // Getters
    double getDx() const { return Dx_; }
    double getDy() const { return Dy_; }
    double getDz() const { return Dz_; }

    // Setters
    void setDx(double Dx) {
        if (Dx < 0.0) throw std::invalid_argument("Dx must be non-negative");
        Dx_ = Dx;
    }
    void setDy(double Dy) {
        if (Dy < 0.0) throw std::invalid_argument("Dy must be non-negative");
        Dy_ = Dy;
    }
    void setDz(double Dz) {
        if (Dz < 0.0) throw std::invalid_argument("Dz must be non-negative");
        Dz_ = Dz;
    }

private:
    double Dx_;  ///< Diffusion in x-direction (m²/s)
    double Dy_;  ///< Diffusion in y-direction (m²/s)
    double Dz_;  ///< Diffusion in z-direction (m²/s)
};

} // namespace physics
} // namespace koo

#endif // KOO_PHYSICS_DIFFUSION_COEFFICIENT_H
