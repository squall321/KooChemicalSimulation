#ifndef KOO_REACTION_DIFFUSION_H
#define KOO_REACTION_DIFFUSION_H

#include "../diffusion/FickDiffusion.h"
#include "../diffusion/DiffusionCoefficient.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <cmath>
#include <stdexcept>

namespace koo {
namespace physics {
namespace coupling {

/**
 * @class ReactionDiffusion
 * @brief Reaction-diffusion system for pattern formation
 *
 * Phase 23: Reaction-Diffusion Coupling (v0.5.0-alpha3)
 *
 * Reaction-diffusion equations:
 *   ∂u/∂t = D_u ∇²u + f(u,v)
 *   ∂v/∂t = D_v ∇²v + g(u,v)
 *
 * Where:
 * - u, v: Species concentrations
 * - D_u, D_v: Diffusion coefficients
 * - f(u,v), g(u,v): Reaction terms
 *
 * Famous systems:
 * - Gray-Scott model
 * - Brusselator
 * - FitzHugh-Nagumo
 * - Schnakenberg
 * - Turing patterns
 */
class ReactionDiffusion {
public:
    using ReactionFunction = std::function<double(const std::vector<double>&)>;
    using Vector2D = std::vector<double>;

    /**
     * @brief Constructor for two-species system
     * @param diffCoeff1 Diffusion coefficient for species 1
     * @param diffCoeff2 Diffusion coefficient for species 2
     * @param reaction1 Reaction term for species 1: f(u,v)
     * @param reaction2 Reaction term for species 2: g(u,v)
     */
    ReactionDiffusion(std::shared_ptr<DiffusionCoefficient> diffCoeff1,
                     std::shared_ptr<DiffusionCoefficient> diffCoeff2,
                     ReactionFunction reaction1,
                     ReactionFunction reaction2)
        : diffusion1_(diffCoeff1),
          diffusion2_(diffCoeff2),
          reaction1_(reaction1),
          reaction2_(reaction2),
          temperature_(300.0) {}

    /**
     * @brief Calculate RHS for species 1: du/dt = D_u ∇²u + f(u,v)
     */
    double calculateRHS1(double laplacian_u,
                        const std::vector<double>& concentrations) const {
        double diffusionTerm = diffusion1_.calculateSourceTerm(laplacian_u, temperature_);
        double reactionTerm = reaction1_(concentrations);
        return diffusionTerm + reactionTerm;
    }

    /**
     * @brief Calculate RHS for species 2: dv/dt = D_v ∇²v + g(u,v)
     */
    double calculateRHS2(double laplacian_v,
                        const std::vector<double>& concentrations) const {
        double diffusionTerm = diffusion2_.calculateSourceTerm(laplacian_v, temperature_);
        double reactionTerm = reaction2_(concentrations);
        return diffusionTerm + reactionTerm;
    }

    /**
     * @brief Set temperature
     */
    void setTemperature(double T) { temperature_ = T; }

    /**
     * @brief Get temperature
     */
    double getTemperature() const { return temperature_; }

    /**
     * @brief Calculate Turing instability condition
     *
     * For Turing patterns to form:
     * 1. System must be stable without diffusion
     * 2. Diffusion must destabilize the system
     * 3. D_v > D_u (typically)
     *
     * Turing condition: f_u + g_v < 0 (stable without diffusion)
     *                   f_u g_v - f_v g_u > 0 (det(J) > 0)
     *                   D_v f_u + D_u g_v > 0
     *                   (D_v f_u + D_u g_v)² > 4 D_u D_v (f_u g_v - f_v g_u)
     */
    struct TuringAnalysis {
        bool stableWithoutDiffusion;
        bool unstableWithDiffusion;
        bool canFormPatterns;
        double trace;              // f_u + g_v
        double determinant;        // f_u g_v - f_v g_u
        double diffusionRatio;     // D_v / D_u
        double criticalWavenumber; // k_c for pattern wavelength
    };

    /**
     * @brief Perform Turing instability analysis
     * @param equilibrium Equilibrium point [u*, v*]
     * @param fu Partial derivative ∂f/∂u
     * @param fv Partial derivative ∂f/∂v
     * @param gu Partial derivative ∂g/∂u
     * @param gv Partial derivative ∂g/∂v
     */
    TuringAnalysis analyzeTuringInstability(const std::vector<double>& equilibrium,
                                           double fu, double fv,
                                           double gu, double gv) const {
        TuringAnalysis result;

        // Get diffusion coefficients at equilibrium
        double Du = diffusion1_.getDiffusionCoefficient(temperature_, equilibrium[0]);
        double Dv = diffusion2_.getDiffusionCoefficient(temperature_, equilibrium[1]);

        // Jacobian trace and determinant
        result.trace = fu + gv;
        result.determinant = fu * gv - fv * gu;
        result.diffusionRatio = Dv / Du;

        // Stability without diffusion: trace < 0 and det > 0
        result.stableWithoutDiffusion = (result.trace < 0) && (result.determinant > 0);

        // Turing instability conditions
        double condition1 = Dv * fu + Du * gv;
        double condition2 = condition1 * condition1 - 4.0 * Du * Dv * result.determinant;

        result.unstableWithDiffusion = (condition1 > 0) && (condition2 > 0);

        // Can form patterns if both conditions met
        result.canFormPatterns = result.stableWithoutDiffusion && result.unstableWithDiffusion;

        // Critical wavenumber for pattern formation
        if (result.canFormPatterns) {
            result.criticalWavenumber = std::sqrt(condition1 / (2.0 * Du * Dv));
        } else {
            result.criticalWavenumber = 0.0;
        }

        return result;
    }

    /**
     * @brief Get characteristic pattern wavelength
     * λ = 2π/k_c
     */
    double getPatternWavelength(double criticalWavenumber) const {
        if (criticalWavenumber <= 0.0) return 0.0;
        return 2.0 * M_PI / criticalWavenumber;
    }

    /**
     * @brief Get diffusion coefficient ratio
     */
    double getDiffusionRatio() const {
        double D1 = diffusion1_.getDiffusionCoefficient(temperature_);
        double D2 = diffusion2_.getDiffusionCoefficient(temperature_);
        return D2 / D1;
    }

private:
    FickDiffusion diffusion1_;      ///< Diffusion for species 1
    FickDiffusion diffusion2_;      ///< Diffusion for species 2
    ReactionFunction reaction1_;    ///< Reaction term f(u,v)
    ReactionFunction reaction2_;    ///< Reaction term g(u,v)
    double temperature_;            ///< System temperature
};

/**
 * @class GrayScottModel
 * @brief Gray-Scott reaction-diffusion model
 *
 * Classic pattern-forming system:
 *   ∂u/∂t = D_u ∇²u - uv² + F(1-u)
 *   ∂v/∂t = D_v ∇²v + uv² - (F+k)v
 *
 * Parameters:
 * - F: Feed rate
 * - k: Kill rate
 * - D_u, D_v: Diffusion coefficients
 *
 * Different parameter regimes produce:
 * - Spots, stripes, spirals, chaos, etc.
 */
class GrayScottModel : public ReactionDiffusion {
public:
    /**
     * @brief Constructor
     * @param Du Diffusion coefficient for u
     * @param Dv Diffusion coefficient for v
     * @param F Feed rate
     * @param k Kill rate
     */
    GrayScottModel(double Du, double Dv, double F, double k)
        : ReactionDiffusion(
            std::make_shared<ConstantDiffusion>(Du),
            std::make_shared<ConstantDiffusion>(Dv),
            [F, k](const std::vector<double>& c) {
                double u = c[0], v = c[1];
                return -u * v * v + F * (1.0 - u);
            },
            [F, k](const std::vector<double>& c) {
                double u = c[0], v = c[1];
                return u * v * v - (F + k) * v;
            }
          ),
          F_(F), k_(k), Du_(Du), Dv_(Dv) {}

    /**
     * @brief Get parameter regime name
     */
    std::string getRegimeName() const {
        // Based on Pearson's classification
        if (F_ < 0.03 && k_ < 0.06) return "Spots";
        if (F_ < 0.04 && k_ > 0.06) return "Stripes";
        if (F_ > 0.05 && k_ < 0.065) return "Spirals";
        if (F_ > 0.06 && k_ > 0.06) return "Chaos";
        return "Uniform";
    }

    /**
     * @brief Get equilibrium point
     * Uniform steady state: u* = 1, v* = 0
     */
    std::vector<double> getEquilibrium() const {
        return {1.0, 0.0};
    }

    /**
     * @brief Get Jacobian at equilibrium
     */
    void getJacobianAtEquilibrium(double& fu, double& fv, double& gu, double& gv) const {
        // At (1, 0):
        fu = -F_;
        fv = 0.0;
        gu = 0.0;
        gv = -(F_ + k_);
    }

    double getFeedRate() const { return F_; }
    double getKillRate() const { return k_; }

private:
    double F_;  ///< Feed rate
    double k_;  ///< Kill rate
    double Du_; ///< Diffusion coefficient u
    double Dv_; ///< Diffusion coefficient v
};

/**
 * @class Brusselator
 * @brief Brusselator reaction-diffusion model
 *
 * Oscillatory chemical reaction:
 *   ∂u/∂t = D_u ∇²u + A - (B+1)u + u²v
 *   ∂v/∂t = D_v ∇²v + Bu - u²v
 *
 * Parameters:
 * - A, B: Reaction parameters
 * - Oscillations occur when B > 1 + A²
 */
class Brusselator : public ReactionDiffusion {
public:
    /**
     * @brief Constructor
     * @param Du Diffusion coefficient for u
     * @param Dv Diffusion coefficient for v
     * @param A Parameter A
     * @param B Parameter B
     */
    Brusselator(double Du, double Dv, double A, double B)
        : ReactionDiffusion(
            std::make_shared<ConstantDiffusion>(Du),
            std::make_shared<ConstantDiffusion>(Dv),
            [A, B](const std::vector<double>& c) {
                double u = c[0], v = c[1];
                return A - (B + 1.0) * u + u * u * v;
            },
            [A, B](const std::vector<double>& c) {
                double u = c[0], v = c[1];
                return B * u - u * u * v;
            }
          ),
          A_(A), B_(B), Du_(Du), Dv_(Dv) {}

    /**
     * @brief Check if system oscillates (Hopf bifurcation)
     */
    bool isOscillatory() const {
        return B_ > 1.0 + A_ * A_;
    }

    /**
     * @brief Get equilibrium point
     * Uniform steady state: u* = A, v* = B/A
     */
    std::vector<double> getEquilibrium() const {
        return {A_, B_ / A_};
    }

    /**
     * @brief Get Jacobian at equilibrium
     */
    void getJacobianAtEquilibrium(double& fu, double& fv, double& gu, double& gv) const {
        // Equilibrium point: u* = A, v* = B/A
        // (variables calculated but not used in current implementation)

        fu = B_ - 1.0;
        fv = A_ * A_;
        gu = -B_;
        gv = -A_ * A_;
    }

    double getA() const { return A_; }
    double getB() const { return B_; }

private:
    double A_;  ///< Parameter A
    double B_;  ///< Parameter B
    double Du_; ///< Diffusion coefficient u
    double Dv_; ///< Diffusion coefficient v
};

/**
 * @class SchnakenbergModel
 * @brief Schnakenberg reaction-diffusion model
 *
 * Autocatalytic reaction:
 *   ∂u/∂t = D_u ∇²u + a - u + u²v
 *   ∂v/∂t = D_v ∇²v + b - u²v
 *
 * Parameters:
 * - a, b: Reaction parameters
 * - Turing patterns form for certain a, b values
 */
class SchnakenbergModel : public ReactionDiffusion {
public:
    /**
     * @brief Constructor
     */
    SchnakenbergModel(double Du, double Dv, double a, double b)
        : ReactionDiffusion(
            std::make_shared<ConstantDiffusion>(Du),
            std::make_shared<ConstantDiffusion>(Dv),
            [a, b](const std::vector<double>& c) {
                double u = c[0], v = c[1];
                return a - u + u * u * v;
            },
            [a, b](const std::vector<double>& c) {
                double u = c[0], v = c[1];
                return b - u * u * v;
            }
          ),
          a_(a), b_(b), Du_(Du), Dv_(Dv) {}

    /**
     * @brief Get equilibrium point
     * u* = a + b, v* = b/(a+b)²
     */
    std::vector<double> getEquilibrium() const {
        double ustar = a_ + b_;
        double vstar = b_ / (ustar * ustar);
        return {ustar, vstar};
    }

    /**
     * @brief Get Jacobian at equilibrium
     */
    void getJacobianAtEquilibrium(double& fu, double& fv, double& gu, double& gv) const {
        double ustar = a_ + b_;
        double vstar = b_ / (ustar * ustar);

        fu = -1.0 + 2.0 * ustar * vstar;
        fv = ustar * ustar;
        gu = -2.0 * ustar * vstar;
        gv = -ustar * ustar;
    }

    double geta() const { return a_; }
    double getb() const { return b_; }

private:
    double a_;  ///< Parameter a
    double b_;  ///< Parameter b
    double Du_; ///< Diffusion coefficient u
    double Dv_; ///< Diffusion coefficient v
};

} // namespace coupling
} // namespace physics
} // namespace koo

#endif // KOO_REACTION_DIFFUSION_H
