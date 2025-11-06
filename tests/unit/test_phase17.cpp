/**
 * @file test_phase17.cpp
 * @brief Tests for Phase 17 - Chemical Reaction System
 * @author KooChemicalSimulation Development Team
 * @date 2025-11-06
 */

#include "chemistry/reaction/Reaction.h"
#include "chemistry/reaction/ReactionManager.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace koo::chemistry;

// Test RateLaw functionality
void testRateLaw() {
    std::cout << "\nTesting RateLaw...\n";

    RateLaw rateLaw;
    rateLaw.A = 1.0e13;      // m³/(mol·s)
    rateLaw.beta = 0.0;
    rateLaw.Ea = 100000.0;   // 100 kJ/mol

    // Test rate constant calculation at 1000 K
    double T = 1000.0;
    double k = rateLaw.getRateConstant(T);
    std::cout << "  → k at 1000K = " << k << " m³/(mol·s)\n";
    assert(k > 0.0);

    // Test temperature dependence
    double k500 = rateLaw.getRateConstant(500.0);
    double k1500 = rateLaw.getRateConstant(1500.0);
    assert(k1500 > k500); // Higher temperature should give higher rate constant

    std::cout << "  ✓ RateLaw calculations work\n";
}

// Test basic Reaction construction
void testReactionConstruction() {
    std::cout << "\nTesting Reaction construction...\n";

    Reaction reaction("test_rxn", ReactionType::ELEMENTARY, true);

    assert(reaction.getId() == "test_rxn");
    assert(reaction.getType() == ReactionType::ELEMENTARY);
    assert(reaction.isReversible() == true);

    // Add reactants and products
    reaction.addReactant("H2", 1.0);
    reaction.addReactant("O2", 1.0);
    reaction.addProduct("H2O", 2.0);

    assert(reaction.getReactants().size() == 2);
    assert(reaction.getProducts().size() == 1);
    assert(reaction.hasSpecies("H2"));
    assert(reaction.hasSpecies("O2"));
    assert(reaction.hasSpecies("H2O"));
    assert(!reaction.hasSpecies("N2"));

    std::cout << "  ✓ Reaction construction works\n";
}

// Test stoichiometry
void testStoichiometry() {
    std::cout << "\nTesting stoichiometry...\n";

    // Reaction: 2 H2 + O2 => 2 H2O
    Reaction reaction("combustion", ReactionType::ELEMENTARY, false);
    reaction.addReactant("H2", 2.0);
    reaction.addReactant("O2", 1.0);
    reaction.addProduct("H2O", 2.0);

    // Check stoichiometric coefficients
    assert(reaction.getStoichiometry("H2") == -2.0);  // Consumed
    assert(reaction.getStoichiometry("O2") == -1.0);  // Consumed
    assert(reaction.getStoichiometry("H2O") == 2.0);  // Produced
    assert(reaction.getStoichiometry("N2") == 0.0);   // Not involved

    // Check equation string
    std::string eq = reaction.getEquation();
    std::cout << "  → Equation: " << eq << "\n";
    assert(eq.find("H2") != std::string::npos);
    assert(eq.find("O2") != std::string::npos);
    assert(eq.find("H2O") != std::string::npos);
    assert(eq.find("=>") != std::string::npos);  // Irreversible

    std::cout << "  ✓ Stoichiometry works\n";
}

// Test rate constant calculation
void testRateConstants() {
    std::cout << "\nTesting rate constants...\n";

    Reaction reaction("test", ReactionType::ELEMENTARY, true);
    reaction.addReactant("A", 1.0);
    reaction.addProduct("B", 1.0);

    // Set forward rate law: k = 1e13 * exp(-100000/(RT))
    RateLaw forward;
    forward.A = 1.0e13;
    forward.beta = 0.0;
    forward.Ea = 100000.0;
    reaction.setForwardRateLaw(forward);

    // Set reverse rate law
    RateLaw reverse;
    reverse.A = 1.0e12;
    reverse.beta = 0.0;
    reverse.Ea = 150000.0;
    reaction.setReverseRateLaw(reverse);

    double T = 1000.0;
    double kf = reaction.getForwardRateConstant(T);
    double kr = reaction.getReverseRateConstant(T);

    std::cout << "  → kf at 1000K = " << kf << "\n";
    std::cout << "  → kr at 1000K = " << kr << "\n";

    assert(kf > 0.0);
    assert(kr > 0.0);
    assert(kf > kr);  // Forward has lower activation energy

    std::cout << "  ✓ Rate constant calculations work\n";
}

// Test rate of progress calculation
void testRateOfProgress() {
    std::cout << "\nTesting rate of progress...\n";

    // Reaction: H2 + O2 => 2 OH
    Reaction reaction("test", ReactionType::ELEMENTARY, true);
    reaction.addReactant("H2", 1.0);
    reaction.addReactant("O2", 1.0);
    reaction.addProduct("OH", 2.0);

    // Set rate laws
    RateLaw forward;
    forward.A = 1.0e13;
    forward.beta = 0.0;
    forward.Ea = 200000.0;
    reaction.setForwardRateLaw(forward);

    RateLaw reverse;
    reverse.A = 1.0e12;
    reverse.beta = 0.0;
    reverse.Ea = 50000.0;
    reaction.setReverseRateLaw(reverse);

    // Set concentrations (mol/m³)
    std::map<std::string, double> conc;
    conc["H2"] = 10.0;
    conc["O2"] = 5.0;
    conc["OH"] = 0.1;

    double T = 1500.0;

    // Calculate rates of progress
    double rop_forward = reaction.getForwardRateOfProgress(T, conc);
    double rop_reverse = reaction.getReverseRateOfProgress(T, conc);
    double rop_net = reaction.getNetRateOfProgress(T, conc);

    std::cout << "  → Forward ROP = " << rop_forward << " mol/(m³·s)\n";
    std::cout << "  → Reverse ROP = " << rop_reverse << " mol/(m³·s)\n";
    std::cout << "  → Net ROP = " << rop_net << " mol/(m³·s)\n";

    assert(rop_forward > 0.0);
    assert(rop_reverse > 0.0);
    assert(std::abs(rop_net - (rop_forward - rop_reverse)) < 1e-10);

    std::cout << "  ✓ Rate of progress calculations work\n";
}

// Test irreversible reaction
void testIrreversibleReaction() {
    std::cout << "\nTesting irreversible reaction...\n";

    Reaction reaction("irrev", ReactionType::IRREVERSIBLE, false);
    reaction.addReactant("A", 1.0);
    reaction.addProduct("B", 1.0);

    RateLaw forward;
    forward.A = 1.0e13;
    forward.beta = 0.0;
    forward.Ea = 50000.0;
    reaction.setForwardRateLaw(forward);

    assert(!reaction.isReversible());

    std::map<std::string, double> conc;
    conc["A"] = 10.0;
    conc["B"] = 5.0;

    double T = 1000.0;
    double kr = reaction.getReverseRateConstant(T);
    double rop_reverse = reaction.getReverseRateOfProgress(T, conc);

    assert(kr == 0.0);
    assert(rop_reverse == 0.0);

    std::string eq = reaction.getEquation();
    assert(eq.find("=>") != std::string::npos);  // One-way arrow

    std::cout << "  ✓ Irreversible reaction works\n";
}

// Test ReactionManager basic operations
void testReactionManager() {
    std::cout << "\nTesting ReactionManager...\n";

    ReactionManager manager;

    // Add first reaction
    Reaction r1("R1", ReactionType::ELEMENTARY, true);
    r1.addReactant("H2", 1.0);
    r1.addProduct("H", 2.0);
    manager.addReaction(r1);

    assert(manager.getReactionCount() == 1);
    assert(manager.hasReaction("R1"));

    // Add second reaction
    Reaction r2("R2", ReactionType::ELEMENTARY, true);
    r2.addReactant("O2", 1.0);
    r2.addProduct("O", 2.0);
    manager.addReaction(r2);

    assert(manager.getReactionCount() == 2);
    assert(manager.hasReaction("R2"));

    // Retrieve reaction
    auto retrieved = manager.getReaction("R1");
    assert(retrieved != nullptr);
    assert(retrieved->getId() == "R1");

    std::cout << "  ✓ ReactionManager basic operations work\n";
}

// Test duplicate handling
void testDuplicateHandling() {
    std::cout << "\nTesting duplicate handling...\n";

    ReactionManager manager;

    Reaction r1("R1", ReactionType::ELEMENTARY, true);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    manager.addReaction(r1);

    // Try to add duplicate
    bool caught = false;
    try {
        manager.addReaction(r1);
    } catch (const std::invalid_argument& e) {
        caught = true;
        std::cout << "  ✓ Caught expected error: " << e.what() << "\n";
    }
    assert(caught);

    std::cout << "  ✓ Duplicate prevention works\n";
}

// Test ReactionManager queries
void testReactionManagerQueries() {
    std::cout << "\nTesting ReactionManager queries...\n";

    ReactionManager manager;

    // Reaction 1: H2 + OH => H2O + H
    Reaction r1("R1", ReactionType::ELEMENTARY, true);
    r1.addReactant("H2", 1.0);
    r1.addReactant("OH", 1.0);
    r1.addProduct("H2O", 1.0);
    r1.addProduct("H", 1.0);
    manager.addReaction(r1);

    // Reaction 2: H + O2 => OH + O
    Reaction r2("R2", ReactionType::ELEMENTARY, true);
    r2.addReactant("H", 1.0);
    r2.addReactant("O2", 1.0);
    r2.addProduct("OH", 1.0);
    r2.addProduct("O", 1.0);
    manager.addReaction(r2);

    // Query by species
    auto h2_reactions = manager.getReactionsBySpecies("H2");
    assert(h2_reactions.size() == 1);
    assert(h2_reactions[0]->getId() == "R1");

    auto oh_reactions = manager.getReactionsBySpecies("OH");
    assert(oh_reactions.size() == 2);  // OH involved in both

    auto n2_reactions = manager.getReactionsBySpecies("N2");
    assert(n2_reactions.empty());  // N2 not involved

    // Get all species
    auto species = manager.getAllSpecies();
    std::cout << "  → Total species in mechanism: " << species.size() << "\n";
    assert(species.size() == 6);  // H2, OH, H2O, H, O2, O
    assert(species.count("H2") > 0);
    assert(species.count("OH") > 0);
    assert(species.count("H") > 0);

    std::cout << "  ✓ ReactionManager queries work\n";
}

// Test reaction removal
void testReactionRemoval() {
    std::cout << "\nTesting reaction removal...\n";

    ReactionManager manager;

    Reaction r1("R1", ReactionType::ELEMENTARY, true);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    manager.addReaction(r1);

    Reaction r2("R2", ReactionType::ELEMENTARY, true);
    r2.addReactant("B", 1.0);
    r2.addProduct("C", 1.0);
    manager.addReaction(r2);

    assert(manager.getReactionCount() == 2);

    // Remove R1
    bool removed = manager.removeReaction("R1");
    assert(removed);
    assert(manager.getReactionCount() == 1);
    assert(!manager.hasReaction("R1"));
    assert(manager.hasReaction("R2"));

    // Try to remove non-existent
    removed = manager.removeReaction("R999");
    assert(!removed);

    // Clear all
    manager.clear();
    assert(manager.getReactionCount() == 0);

    std::cout << "  ✓ Reaction removal works\n";
}

// Test H2-O2 mechanism
void testH2O2Mechanism() {
    std::cout << "\nTesting H2-O2 mechanism...\n";

    ReactionManager manager;
    manager.loadH2O2Mechanism();

    assert(manager.getReactionCount() == 4);
    assert(manager.hasReaction("R1"));
    assert(manager.hasReaction("R2"));
    assert(manager.hasReaction("R3"));
    assert(manager.hasReaction("R4"));

    // Check species
    auto species = manager.getAllSpecies();
    std::cout << "  → Total species: " << species.size() << "\n";
    assert(species.count("H2") > 0);
    assert(species.count("O2") > 0);
    assert(species.count("H2O") > 0);
    assert(species.count("OH") > 0);
    assert(species.count("H") > 0);
    assert(species.count("O") > 0);

    // Get summary
    std::string summary = manager.getSummary();
    std::cout << summary << "\n";
    assert(summary.find("4") != std::string::npos);  // 4 reactions

    std::cout << "  ✓ H2-O2 mechanism loaded correctly\n";
}

// Test production rate calculations
void testProductionRates() {
    std::cout << "\nTesting production rates...\n";

    ReactionManager manager;

    // Simple mechanism: A => B => C
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw r1_rate;
    r1_rate.A = 1.0e10;
    r1_rate.beta = 0.0;
    r1_rate.Ea = 50000.0;
    r1.setForwardRateLaw(r1_rate);
    manager.addReaction(r1);

    Reaction r2("R2", ReactionType::IRREVERSIBLE, false);
    r2.addReactant("B", 1.0);
    r2.addProduct("C", 1.0);
    RateLaw r2_rate;
    r2_rate.A = 5.0e9;
    r2_rate.beta = 0.0;
    r2_rate.Ea = 40000.0;
    r2.setForwardRateLaw(r2_rate);
    manager.addReaction(r2);

    // Set concentrations
    std::map<std::string, double> conc;
    conc["A"] = 10.0;
    conc["B"] = 5.0;
    conc["C"] = 1.0;

    double T = 1000.0;

    // Calculate production rates
    auto rates = manager.getAllProductionRates(T, conc);

    std::cout << "  → Production rate of A: " << rates["A"] << " mol/(m³·s)\n";
    std::cout << "  → Production rate of B: " << rates["B"] << " mol/(m³·s)\n";
    std::cout << "  → Production rate of C: " << rates["C"] << " mol/(m³·s)\n";

    // A is consumed, so rate should be negative
    assert(rates["A"] < 0.0);
    // C is only produced, so rate should be positive
    assert(rates["C"] > 0.0);

    // Test individual species production rate
    double rate_B = manager.getProductionRate("B", T, conc);
    assert(std::abs(rate_B - rates["B"]) < 1e-10);

    std::cout << "  ✓ Production rate calculations work\n";
}

// Test complex stoichiometry
void testComplexStoichiometry() {
    std::cout << "\nTesting complex stoichiometry...\n";

    // Reaction: 2 H2 + O2 => 2 H2O
    Reaction reaction("combustion", ReactionType::ELEMENTARY, false);
    reaction.addReactant("H2", 2.0);
    reaction.addReactant("O2", 1.0);
    reaction.addProduct("H2O", 2.0);

    RateLaw forward;
    forward.A = 1.0e13;
    forward.beta = 0.0;
    forward.Ea = 150000.0;
    reaction.setForwardRateLaw(forward);

    std::map<std::string, double> conc;
    conc["H2"] = 20.0;
    conc["O2"] = 10.0;
    conc["H2O"] = 0.1;

    double T = 1500.0;

    // Calculate ROP
    double rop = reaction.getForwardRateOfProgress(T, conc);
    std::cout << "  → ROP = " << rop << " mol/(m³·s)\n";

    // With stoichiometry 2:1:2, the rates should follow
    // d[H2]/dt = -2 * ROP
    // d[O2]/dt = -1 * ROP
    // d[H2O]/dt = +2 * ROP

    double rate_H2 = reaction.getStoichiometry("H2") * rop;
    double rate_O2 = reaction.getStoichiometry("O2") * rop;
    double rate_H2O = reaction.getStoichiometry("H2O") * rop;

    std::cout << "  → d[H2]/dt = " << rate_H2 << " mol/(m³·s)\n";
    std::cout << "  → d[O2]/dt = " << rate_O2 << " mol/(m³·s)\n";
    std::cout << "  → d[H2O]/dt = " << rate_H2O << " mol/(m³·s)\n";

    assert(rate_H2 < 0.0);  // Consumed
    assert(rate_O2 < 0.0);  // Consumed
    assert(rate_H2O > 0.0);  // Produced
    assert(std::abs(rate_H2 / rate_O2) - 2.0 < 0.01);  // 2:1 ratio

    std::cout << "  ✓ Complex stoichiometry works\n";
}

int main() {
    std::cout << "Phase 17 Tests - Chemical Reaction System\n";
    std::cout << "=========================================\n";

    testRateLaw();
    testReactionConstruction();
    testStoichiometry();
    testRateConstants();
    testRateOfProgress();
    testIrreversibleReaction();
    testReactionManager();
    testDuplicateHandling();
    testReactionManagerQueries();
    testReactionRemoval();
    testH2O2Mechanism();
    testProductionRates();
    testComplexStoichiometry();

    std::cout << "\n=========================================\n";
    std::cout << "All Phase 17 tests passed!\n";
    std::cout << "Chemical reaction system verified.\n";

    return 0;
}
