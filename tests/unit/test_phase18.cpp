/**
 * @file test_phase18.cpp
 * @brief Tests for Phase 18 - Chemical Kinetics Solver
 * @author KooChemicalSimulation Development Team
 * @date 2025-11-06
 */

#include "chemistry/species/Species.h"
#include "chemistry/species/SpeciesManager.h"
#include "chemistry/reaction/Reaction.h"
#include "chemistry/reaction/ReactionManager.h"
#include "chemistry/kinetics/ChemicalSystem.h"
#include "chemistry/kinetics/KineticsIntegrator.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace koo::chemistry;

// Test ChemicalSystem construction
void testChemicalSystemConstruction() {
    std::cout << "\nTesting ChemicalSystem construction...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // Add simple reaction: A => B
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();

    assert(system.getSpeciesCount() == 2);  // A and B
    assert(system.getReactionCount() == 1);

    auto names = system.getSpeciesNames();
    std::cout << "  → Species in system: ";
    for (const auto& name : names) {
        std::cout << name << " ";
    }
    std::cout << "\n";

    std::cout << "  ✓ ChemicalSystem construction works\n";
}

// Test state vector operations
void testStateVectorOperations() {
    std::cout << "\nTesting state vector operations...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // Simple reaction: A => B
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();

    // Set concentrations
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);

    assert(system.getConcentration("A") == 10.0);
    assert(system.getConcentration("B") == 0.0);

    auto state = system.getState();
    assert(state.size() == 2);

    std::cout << "  ✓ State vector operations work\n";
}

// Test temperature setting
void testTemperature() {
    std::cout << "\nTesting temperature...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();

    system.setTemperature(1000.0);
    assert(system.getTemperature() == 1000.0);

    std::cout << "  ✓ Temperature setting works\n";
}

// Test production rate calculation
void testProductionRates() {
    std::cout << "\nTesting production rates...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // Reaction: A => B (k = 1.0e10 * exp(-50000/(R*T)))
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();
    system.setTemperature(1000.0);
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);

    auto rates = system.getProductionRates();
    std::cout << "  → Production rate of A: " << rates[0] << " mol/(m³·s)\n";
    std::cout << "  → Production rate of B: " << rates[1] << " mol/(m³·s)\n";

    // A is consumed, B is produced
    assert(rates[0] < 0.0);
    assert(rates[1] > 0.0);
    // Conservation: rate(A) + rate(B) = 0
    assert(std::abs(rates[0] + rates[1]) < 1e-10);

    std::cout << "  ✓ Production rate calculations work\n";
}

// Test KineticsIntegrator construction
void testIntegratorConstruction() {
    std::cout << "\nTesting KineticsIntegrator construction...\n";

    KineticsIntegrator integrator(IntegrationMethod::RK4);

    assert(integrator.getMethod() == IntegrationMethod::RK4);
    assert(integrator.getTime() == 0.0);

    integrator.setTimeStep(1.0e-5);
    assert(integrator.getTimeStep() == 1.0e-5);

    std::cout << "  → Method: " << integrator.getMethodName() << "\n";
    std::cout << "  ✓ KineticsIntegrator construction works\n";
}

// Test integration method switching
void testIntegrationMethods() {
    std::cout << "\nTesting integration methods...\n";

    KineticsIntegrator integrator;

    integrator.setMethod(IntegrationMethod::EXPLICIT_EULER);
    assert(integrator.getMethodName() == "Explicit Euler");

    integrator.setMethod(IntegrationMethod::RK2);
    assert(integrator.getMethodName() == "RK2 (Midpoint)");

    integrator.setMethod(IntegrationMethod::RK4);
    assert(integrator.getMethodName() == "RK4");

    std::cout << "  ✓ Integration method switching works\n";
}

// Test single step advance
void testSingleStepAdvance() {
    std::cout << "\nTesting single step advance...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // Simple decay: A => B
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();
    system.setTemperature(1000.0);
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);

    double initial_A = system.getConcentration("A");

    KineticsIntegrator integrator(IntegrationMethod::EXPLICIT_EULER);
    integrator.setTimeStep(1.0e-6);
    integrator.advance(system);

    double final_A = system.getConcentration("A");
    double final_B = system.getConcentration("B");

    std::cout << "  → A: " << initial_A << " → " << final_A << " mol/m³\n";
    std::cout << "  → B: 0.0 → " << final_B << " mol/m³\n";

    // A should decrease, B should increase
    assert(final_A < initial_A);
    assert(final_B > 0.0);

    std::cout << "  ✓ Single step advance works\n";
}

// Test time integration
void testTimeIntegration() {
    std::cout << "\nTesting time integration...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // Decay: A => B
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();
    system.setTemperature(1000.0);
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);

    KineticsIntegrator integrator(IntegrationMethod::RK4);
    integrator.setTimeStep(1.0e-6);

    double t0 = 0.0;
    double tf = 1.0e-4;

    auto trajectory = integrator.integrate(system, t0, tf, 2.0e-5);

    std::cout << "  → Trajectory points: " << trajectory.size() << "\n";
    std::cout << "  → Initial A: " << trajectory.front().second[0] << " mol/m³\n";
    std::cout << "  → Final A: " << trajectory.back().second[0] << " mol/m³\n";
    std::cout << "  → Final B: " << trajectory.back().second[1] << " mol/m³\n";

    // A should decrease over time
    assert(trajectory.back().second[0] < trajectory.front().second[0]);
    // B should increase
    assert(trajectory.back().second[1] > 0.0);

    std::cout << "  ✓ Time integration works\n";
}

// Test reversible reaction
void testReversibleReaction() {
    std::cout << "\nTesting reversible reaction...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // A <=> B
    Reaction r1("R1", ReactionType::REVERSIBLE, true);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);

    RateLaw forward;
    forward.A = 1.0e10;
    forward.beta = 0.0;
    forward.Ea = 50000.0;
    r1.setForwardRateLaw(forward);

    RateLaw reverse;
    reverse.A = 5.0e9;
    reverse.beta = 0.0;
    reverse.Ea = 60000.0;
    r1.setReverseRateLaw(reverse);

    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();
    system.setTemperature(1000.0);
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);

    KineticsIntegrator integrator(IntegrationMethod::RK4);
    integrator.setTimeStep(1.0e-6);

    auto trajectory = integrator.integrate(system, 0.0, 1.0e-3, 2.0e-4);

    std::cout << "  → Initial: A=" << trajectory.front().second[0]
              << ", B=" << trajectory.front().second[1] << "\n";
    std::cout << "  → Final: A=" << trajectory.back().second[0]
              << ", B=" << trajectory.back().second[1] << "\n";

    // System should approach equilibrium
    assert(trajectory.back().second[0] > 0.0);  // Some A remains
    assert(trajectory.back().second[1] > 0.0);  // Some B formed

    std::cout << "  ✓ Reversible reaction works\n";
}

// Test conservation of mass
void testMassConservation() {
    std::cout << "\nTesting mass conservation...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // A => B (should conserve total moles)
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();
    system.setTemperature(1000.0);
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);

    double initial_total = system.getConcentration("A") + system.getConcentration("B");

    KineticsIntegrator integrator(IntegrationMethod::RK4);
    integrator.setTimeStep(1.0e-6);
    integrator.integrate(system, 0.0, 1.0e-4, 0.0);

    double final_total = system.getConcentration("A") + system.getConcentration("B");

    std::cout << "  → Initial total: " << initial_total << " mol/m³\n";
    std::cout << "  → Final total: " << final_total << " mol/m³\n";
    std::cout << "  → Error: " << std::abs(final_total - initial_total) / initial_total * 100 << "%\n";

    // Should conserve mass to within numerical error
    assert(std::abs(final_total - initial_total) / initial_total < 0.01);  // < 1% error

    std::cout << "  ✓ Mass conservation works\n";
}

// Test multi-step mechanism
void testMultiStepMechanism() {
    std::cout << "\nTesting multi-step mechanism...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // A => B => C
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate1;
    rate1.A = 1.0e10;
    rate1.beta = 0.0;
    rate1.Ea = 50000.0;
    r1.setForwardRateLaw(rate1);
    reactionManager.addReaction(r1);

    Reaction r2("R2", ReactionType::IRREVERSIBLE, false);
    r2.addReactant("B", 1.0);
    r2.addProduct("C", 1.0);
    RateLaw rate2;
    rate2.A = 5.0e9;
    rate2.beta = 0.0;
    rate2.Ea = 40000.0;
    r2.setForwardRateLaw(rate2);
    reactionManager.addReaction(r2);

    ChemicalSystem system(speciesManager, reactionManager);
    system.updateSpeciesIndex();
    system.setTemperature(1000.0);
    system.setConcentration("A", 10.0);
    system.setConcentration("B", 0.0);
    system.setConcentration("C", 0.0);

    KineticsIntegrator integrator(IntegrationMethod::RK4);
    integrator.setTimeStep(1.0e-6);

    auto trajectory = integrator.integrate(system, 0.0, 5.0e-4, 1.0e-4);

    std::cout << "  → Time evolution:\n";
    for (const auto& [t, state] : trajectory) {
        std::cout << "    t=" << t*1e6 << " µs: A=" << state[0]
                  << ", B=" << state[1] << ", C=" << state[2] << "\n";
    }

    // A should decrease, C should increase
    assert(trajectory.back().second[0] < trajectory.front().second[0]);
    assert(trajectory.back().second[2] > 0.0);

    std::cout << "  ✓ Multi-step mechanism works\n";
}

// Test method comparison
void testMethodComparison() {
    std::cout << "\nTesting method comparison...\n";

    SpeciesManager speciesManager;
    ReactionManager reactionManager;

    // Simple decay
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate;
    rate.A = 1.0e10;
    rate.beta = 0.0;
    rate.Ea = 50000.0;
    r1.setForwardRateLaw(rate);
    reactionManager.addReaction(r1);

    std::vector<IntegrationMethod> methods = {
        IntegrationMethod::EXPLICIT_EULER,
        IntegrationMethod::RK2,
        IntegrationMethod::RK4
    };

    std::cout << "  Method comparison (A→B decay):\n";
    for (auto method : methods) {
        ChemicalSystem system(speciesManager, reactionManager);
        system.updateSpeciesIndex();
        system.setTemperature(1000.0);
        system.setConcentration("A", 10.0);
        system.setConcentration("B", 0.0);

        KineticsIntegrator integrator(method);
        integrator.setTimeStep(1.0e-6);
        integrator.integrate(system, 0.0, 1.0e-4, 0.0);

        std::cout << "    " << integrator.getMethodName() << ": A="
                  << system.getConcentration("A") << ", B="
                  << system.getConcentration("B") << "\n";
    }

    std::cout << "  ✓ Method comparison works\n";
}

int main() {
    std::cout << "Phase 18 Tests - Chemical Kinetics Solver\n";
    std::cout << "=========================================\n";

    testChemicalSystemConstruction();
    testStateVectorOperations();
    testTemperature();
    testProductionRates();
    testIntegratorConstruction();
    testIntegrationMethods();
    testSingleStepAdvance();
    testTimeIntegration();
    testReversibleReaction();
    testMassConservation();
    testMultiStepMechanism();
    testMethodComparison();

    std::cout << "\n=========================================\n";
    std::cout << "All Phase 18 tests passed!\n";
    std::cout << "Chemical kinetics solver verified.\n";

    return 0;
}
