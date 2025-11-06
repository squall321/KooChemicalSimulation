/**
 * @file ReactionPDECoupler.h
 * @brief High-level coupling between reactions and PDE systems
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-beta
 * @date 2025-11-06
 *
 * Provides complete coupling between chemical reactions and PDE solvers.
 */

#ifndef KOO_CHEMISTRY_REACTION_PDE_COUPLER_H
#define KOO_CHEMISTRY_REACTION_PDE_COUPLER_H

#include "ConcentrationField.h"
#include "ReactionTerm.h"
#include "chemistry/species/SpeciesManager.h"
#include "chemistry/reaction/ReactionSystem.h"
#include "chemistry/kinetics/ChemicalSystem.h"
#include "chemistry/kinetics/KineticsIntegrator.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>

namespace koo {
namespace chemistry {

/**
 * @brief Coupling mode between reactions and PDE
 */
enum class CouplingMode {
    OPERATOR_SPLITTING,   ///< Sequential: PDE then reactions
    FULLY_COUPLED,        ///< Simultaneous solution
    STRANG_SPLITTING      ///< Second-order splitting: PDE/2, reactions, PDE/2
};

/**
 * @brief Complete reaction-PDE coupling system
 *
 * Integrates:
 * - Concentration field management
 * - Reaction source terms
 * - Chemical kinetics integration
 * - PDE system coupling
 *
 * Supports multiple coupling strategies for reaction-diffusion systems.
 */
class ReactionPDECoupler {
public:
    /**
     * @brief Default constructor
     */
    ReactionPDECoupler() : couplingMode_(CouplingMode::OPERATOR_SPLITTING) {}

    /**
     * @brief Constructor with reaction system
     * @param reactionSystem Reaction system
     */
    explicit ReactionPDECoupler(const ReactionSystem& reactionSystem)
        : reactionSystem_(reactionSystem),
          couplingMode_(CouplingMode::OPERATOR_SPLITTING) {

        // Initialize concentration field from reaction system
        auto speciesNames = std::vector<std::string>(
            reactionSystem.getReactionManager().getAllSpecies().begin(),
            reactionSystem.getReactionManager().getAllSpecies().end()
        );
        concentrationField_.setSpecies(speciesNames);

        // Initialize reaction term
        reactionTerm_.setReactionSystem(reactionSystem);

        // Initialize chemical system for kinetics
        chemicalSystem_ = ChemicalSystem(
            reactionSystem.getSpeciesManager(),
            reactionSystem.getReactionManager()
        );
        chemicalSystem_.updateSpeciesIndex();
    }

    /**
     * @brief Set coupling mode
     * @param mode Coupling mode
     */
    void setCouplingMode(CouplingMode mode) {
        couplingMode_ = mode;
    }

    /**
     * @brief Get coupling mode
     * @return Coupling mode
     */
    CouplingMode getCouplingMode() const {
        return couplingMode_;
    }

    /**
     * @brief Get coupling mode name
     */
    std::string getCouplingModeName() const {
        switch (couplingMode_) {
            case CouplingMode::OPERATOR_SPLITTING:
                return "Operator Splitting";
            case CouplingMode::FULLY_COUPLED:
                return "Fully Coupled";
            case CouplingMode::STRANG_SPLITTING:
                return "Strang Splitting";
            default:
                return "Unknown";
        }
    }

    /**
     * @brief Get concentration field
     */
    const ConcentrationField& getConcentrationField() const {
        return concentrationField_;
    }

    /**
     * @brief Get mutable concentration field
     */
    ConcentrationField& getConcentrationField() {
        return concentrationField_;
    }

    /**
     * @brief Get reaction term calculator
     */
    const ReactionTerm& getReactionTerm() const {
        return reactionTerm_;
    }

    /**
     * @brief Get mutable reaction term calculator
     */
    ReactionTerm& getReactionTerm() {
        return reactionTerm_;
    }

    /**
     * @brief Set temperature
     * @param T Temperature (K)
     */
    void setTemperature(double T) {
        reactionTerm_.setTemperature(T);
        chemicalSystem_.setTemperature(T);
    }

    /**
     * @brief Get temperature
     * @return Temperature (K)
     */
    double getTemperature() const {
        return reactionTerm_.getTemperature();
    }

    /**
     * @brief Calculate reaction source terms
     * @return Vector of source terms (mol/(m³·s))
     */
    std::vector<double> calculateReactionSourceTerms() const {
        return reactionTerm_.calculateSourceTerms(concentrationField_);
    }

    /**
     * @brief Advance reactions by time step (operator splitting)
     * @param dt Time step (s)
     * @param integrator Kinetics integrator
     */
    void advanceReactions(double dt, KineticsIntegrator& integrator) {
        // Sync chemical system state with concentration field
        chemicalSystem_.setState(concentrationField_.getConcentrations());
        chemicalSystem_.setTemperature(reactionTerm_.getTemperature());

        // Integrate reactions
        double t0 = integrator.getTime();
        integrator.setTimeStep(dt);
        integrator.advance(chemicalSystem_);

        // Update concentration field
        concentrationField_.setConcentrations(chemicalSystem_.getState());
    }

    /**
     * @brief Integrate reactions over time period
     * @param t0 Initial time (s)
     * @param tf Final time (s)
     * @param integrator Kinetics integrator
     * @return Trajectory (time, concentrations)
     */
    std::vector<std::pair<double, std::vector<double>>> integrateReactions(
        double t0, double tf, KineticsIntegrator& integrator) {

        // Sync state
        chemicalSystem_.setState(concentrationField_.getConcentrations());
        chemicalSystem_.setTemperature(reactionTerm_.getTemperature());

        // Integrate
        auto trajectory = integrator.integrate(chemicalSystem_, t0, tf, 0.0);

        // Update final state
        if (!trajectory.empty()) {
            concentrationField_.setConcentrations(trajectory.back().second);
        }

        return trajectory;
    }

    /**
     * @brief Apply operator splitting step
     * @param dt Time step (s)
     * @param integrator Kinetics integrator
     * @param applyDiffusion Callback for diffusion step
     */
    void operatorSplittingStep(
        double dt, KineticsIntegrator& integrator,
        std::function<void(double)> applyDiffusion = nullptr) {

        switch (couplingMode_) {
            case CouplingMode::OPERATOR_SPLITTING:
                // Step 1: Diffusion
                if (applyDiffusion) {
                    applyDiffusion(dt);
                }
                // Step 2: Reactions
                advanceReactions(dt, integrator);
                break;

            case CouplingMode::STRANG_SPLITTING:
                // Step 1: Half diffusion
                if (applyDiffusion) {
                    applyDiffusion(dt / 2.0);
                }
                // Step 2: Full reaction
                advanceReactions(dt, integrator);
                // Step 3: Half diffusion
                if (applyDiffusion) {
                    applyDiffusion(dt / 2.0);
                }
                break;

            case CouplingMode::FULLY_COUPLED:
                // This requires simultaneous solution (not implemented here)
                throw std::runtime_error("Fully coupled mode requires implicit solver");
        }
    }

    /**
     * @brief Check if reactions are stiff
     * @param threshold Stiffness threshold
     * @return True if stiff
     */
    bool isStiff(double threshold = 1000.0) const {
        return reactionTerm_.isStiff(concentrationField_, threshold);
    }

    /**
     * @brief Estimate stiffness ratio
     * @return Stiffness ratio
     */
    double estimateStiffness() const {
        return reactionTerm_.estimateStiffness(concentrationField_);
    }

    /**
     * @brief Get system info
     */
    std::string getInfo() const {
        std::ostringstream oss;
        oss << "ReactionPDECoupler:\n";
        oss << "  Coupling mode: " << getCouplingModeName() << "\n";
        oss << "  Temperature: " << getTemperature() << " K\n";
        oss << "  Species: " << concentrationField_.getSpeciesCount() << "\n";
        oss << "  Reactions: " << reactionSystem_.getReactionManager().getReactionCount() << "\n";
        oss << "  Stiffness ratio: " << estimateStiffness() << "\n";
        return oss.str();
    }

    /**
     * @brief Print system info
     */
    void printInfo() const {
        std::cout << getInfo();
    }

    /**
     * @brief Get Jacobian matrix
     * @return Jacobian J[i][j] = ∂S_i/∂C_j
     */
    std::vector<std::vector<double>> getJacobian() const {
        return reactionTerm_.calculateJacobian(concentrationField_);
    }

private:
    ReactionSystem reactionSystem_;           ///< Reaction system
    ConcentrationField concentrationField_;   ///< Concentration field
    ReactionTerm reactionTerm_;               ///< Reaction source term
    ChemicalSystem chemicalSystem_;           ///< Chemical system for kinetics
    CouplingMode couplingMode_;               ///< Coupling strategy
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_REACTION_PDE_COUPLER_H
