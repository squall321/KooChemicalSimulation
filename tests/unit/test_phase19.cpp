/**
 * @file test_phase19.cpp
 * @brief Tests for Phase 19 - Reaction System Integration
 * @author KooChemicalSimulation Development Team
 * @date 2025-11-06
 */

#include "chemistry/species/Species.h"
#include "chemistry/species/SpeciesManager.h"
#include "chemistry/reaction/Reaction.h"
#include "chemistry/reaction/ReactionManager.h"
#include "chemistry/reaction/ReactionNetwork.h"
#include "chemistry/reaction/ReactionSystem.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace koo::chemistry;

// Helper: Create simple reaction mechanism (A -> B -> C)
ReactionManager createSimpleMechanism() {
    ReactionManager manager;

    // R1: A => B
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate1;
    rate1.A = 1.0e10;
    rate1.beta = 0.0;
    rate1.Ea = 50000.0;
    r1.setForwardRateLaw(rate1);
    manager.addReaction(r1);

    // R2: B => C
    Reaction r2("R2", ReactionType::IRREVERSIBLE, false);
    r2.addReactant("B", 1.0);
    r2.addProduct("C", 1.0);
    RateLaw rate2;
    rate2.A = 5.0e9;
    rate2.beta = 0.0;
    rate2.Ea = 40000.0;
    r2.setForwardRateLaw(rate2);
    manager.addReaction(r2);

    return manager;
}

// Test ReactionNetwork construction
void testNetworkConstruction() {
    std::cout << "\nTesting ReactionNetwork construction...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    auto stats = network.getNetworkStats();
    std::cout << "  → Total species: " << stats["total_species"] << "\n";
    std::cout << "  → Total reactions: " << stats["total_reactions"] << "\n";

    assert(stats["total_species"] == 3);  // A, B, C
    assert(stats["total_reactions"] == 2);  // R1, R2

    std::cout << "  ✓ ReactionNetwork construction works\n";
}

// Test pure reactants identification
void testPureReactants() {
    std::cout << "\nTesting pure reactants identification...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    auto pureReactants = network.getPureReactants();
    std::cout << "  → Pure reactants: ";
    for (const auto& sp : pureReactants) {
        std::cout << sp << " ";
    }
    std::cout << "\n";

    assert(pureReactants.size() == 1);
    assert(pureReactants.count("A") > 0);  // A is only consumed

    std::cout << "  ✓ Pure reactants identification works\n";
}

// Test pure products identification
void testPureProducts() {
    std::cout << "\nTesting pure products identification...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    auto pureProducts = network.getPureProducts();
    std::cout << "  → Pure products: ";
    for (const auto& sp : pureProducts) {
        std::cout << sp << " ";
    }
    std::cout << "\n";

    assert(pureProducts.size() == 1);
    assert(pureProducts.count("C") > 0);  // C is only produced

    std::cout << "  ✓ Pure products identification works\n";
}

// Test intermediates identification
void testIntermediates() {
    std::cout << "\nTesting intermediates identification...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    auto intermediates = network.getIntermediates();
    std::cout << "  → Intermediates: ";
    for (const auto& sp : intermediates) {
        std::cout << sp << " ";
    }
    std::cout << "\n";

    assert(intermediates.size() == 1);
    assert(intermediates.count("B") > 0);  // B is produced and consumed

    std::cout << "  ✓ Intermediates identification works\n";
}

// Test producing/consuming reactions
void testProducingConsumingReactions() {
    std::cout << "\nTesting producing/consuming reactions...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    // B is produced by R1
    auto producing = network.getProducingReactions("B");
    std::cout << "  → Reactions producing B: ";
    for (const auto& rxn : producing) {
        std::cout << rxn << " ";
    }
    std::cout << "\n";
    assert(producing.size() == 1);
    assert(producing[0] == "R1");

    // B is consumed by R2
    auto consuming = network.getConsumingReactions("B");
    std::cout << "  → Reactions consuming B: ";
    for (const auto& rxn : consuming) {
        std::cout << rxn << " ";
    }
    std::cout << "\n";
    assert(consuming.size() == 1);
    assert(consuming[0] == "R2");

    std::cout << "  ✓ Producing/consuming reactions work\n";
}

// Test pathway analysis
void testPathwayAnalysis() {
    std::cout << "\nTesting pathway analysis...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    // A can convert to C through B
    assert(network.hasPathway("A", "A"));  // Self
    assert(network.hasPathway("A", "B"));  // Direct
    assert(network.hasPathway("A", "C"));  // Through B
    assert(network.hasPathway("B", "C"));  // Direct

    // But C cannot convert back (irreversible)
    assert(!network.hasPathway("C", "A"));
    assert(!network.hasPathway("C", "B"));

    std::cout << "  ✓ Pathway analysis works\n";
}

// Test network statistics
void testNetworkStats() {
    std::cout << "\nTesting network statistics...\n";

    auto manager = createSimpleMechanism();
    ReactionNetwork network(manager);

    auto stats = network.getNetworkStats();

    std::cout << "  → Statistics:\n";
    for (const auto& [key, value] : stats) {
        std::cout << "      " << key << ": " << value << "\n";
    }

    assert(stats["total_species"] == 3);
    assert(stats["total_reactions"] == 2);
    assert(stats["pure_reactants"] == 1);
    assert(stats["intermediates"] == 1);
    assert(stats["pure_products"] == 1);
    assert(stats["irreversible_reactions"] == 2);

    std::cout << "  ✓ Network statistics work\n";
}

// Test ReactionSystem construction
void testReactionSystemConstruction() {
    std::cout << "\nTesting ReactionSystem construction...\n";

    SpeciesManager speciesManager;
    auto reactionManager = createSimpleMechanism();

    ReactionSystem system(speciesManager, reactionManager);

    assert(system.getReactionManager().getReactionCount() == 2);

    std::cout << "  ✓ ReactionSystem construction works\n";
}

// Test source term calculation
void testSourceTermCalculation() {
    std::cout << "\nTesting source term calculation...\n";

    SpeciesManager speciesManager;
    auto reactionManager = createSimpleMechanism();
    ReactionSystem system(speciesManager, reactionManager);

    // Set concentrations
    std::map<std::string, double> conc;
    conc["A"] = 10.0;
    conc["B"] = 5.0;
    conc["C"] = 0.0;

    double T = 1000.0;

    // Calculate source term for B
    auto termB = system.calculateSourceTerm("B", T, conc);

    std::cout << "  → Source term for B:\n";
    std::cout << "      Production: " << termB.productionRate << " mol/(m³·s)\n";
    std::cout << "      Consumption: " << termB.consumptionRate << " mol/(m³·s)\n";
    std::cout << "      Net: " << termB.netRate << " mol/(m³·s)\n";

    // B is produced by R1 and consumed by R2
    assert(termB.productionRate > 0.0);
    assert(termB.consumptionRate > 0.0);

    std::cout << "  ✓ Source term calculation works\n";
}

// Test all source terms calculation
void testAllSourceTerms() {
    std::cout << "\nTesting all source terms calculation...\n";

    SpeciesManager speciesManager;
    auto reactionManager = createSimpleMechanism();
    ReactionSystem system(speciesManager, reactionManager);

    std::map<std::string, double> conc;
    conc["A"] = 10.0;
    conc["B"] = 5.0;
    conc["C"] = 0.0;

    double T = 1000.0;

    auto allTerms = system.calculateAllSourceTerms(T, conc);

    std::cout << "  → Source terms:\n";
    for (const auto& [species, term] : allTerms) {
        std::cout << "      " << species << ": net = " << term.netRate << " mol/(m³·s)\n";
    }

    assert(allTerms.size() == 3);
    assert(allTerms["A"].netRate < 0.0);  // A consumed
    assert(allTerms["C"].netRate > 0.0);  // C produced

    std::cout << "  ✓ All source terms calculation works\n";
}

// Test reversible mechanism
void testReversibleMechanism() {
    std::cout << "\nTesting reversible mechanism...\n";

    ReactionManager manager;

    // A <=> B (reversible)
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

    manager.addReaction(r1);

    ReactionNetwork network(manager);

    // In reversible mechanism, A and B are both intermediates
    auto intermediates = network.getIntermediates();
    std::cout << "  → Intermediates: ";
    for (const auto& sp : intermediates) {
        std::cout << sp << " ";
    }
    std::cout << "\n";

    assert(intermediates.size() == 2);
    assert(intermediates.count("A") > 0);
    assert(intermediates.count("B") > 0);

    std::cout << "  ✓ Reversible mechanism works\n";
}

// Test complex network (branching reactions)
void testComplexNetwork() {
    std::cout << "\nTesting complex network...\n";

    ReactionManager manager;

    // A => B
    Reaction r1("R1", ReactionType::IRREVERSIBLE, false);
    r1.addReactant("A", 1.0);
    r1.addProduct("B", 1.0);
    RateLaw rate1;
    rate1.A = 1.0e10;
    rate1.beta = 0.0;
    rate1.Ea = 50000.0;
    r1.setForwardRateLaw(rate1);
    manager.addReaction(r1);

    // A => C (branching)
    Reaction r2("R2", ReactionType::IRREVERSIBLE, false);
    r2.addReactant("A", 1.0);
    r2.addProduct("C", 1.0);
    RateLaw rate2;
    rate2.A = 5.0e9;
    rate2.beta = 0.0;
    rate2.Ea = 60000.0;
    r2.setForwardRateLaw(rate2);
    manager.addReaction(r2);

    // B + C => D
    Reaction r3("R3", ReactionType::IRREVERSIBLE, false);
    r3.addReactant("B", 1.0);
    r3.addReactant("C", 1.0);
    r3.addProduct("D", 1.0);
    RateLaw rate3;
    rate3.A = 1.0e11;
    rate3.beta = 0.0;
    rate3.Ea = 40000.0;
    r3.setForwardRateLaw(rate3);
    manager.addReaction(r3);

    ReactionNetwork network(manager);

    auto stats = network.getNetworkStats();
    std::cout << "  → Complex network stats:\n";
    std::cout << "      Species: " << stats["total_species"] << "\n";
    std::cout << "      Reactions: " << stats["total_reactions"] << "\n";
    std::cout << "      Intermediates: " << stats["intermediates"] << "\n";

    assert(stats["total_species"] == 4);  // A, B, C, D
    assert(stats["total_reactions"] == 3);

    // A is consumed by 2 reactions
    auto consuming = network.getConsumingReactions("A");
    assert(consuming.size() == 2);

    std::cout << "  ✓ Complex network works\n";
}

// Test system validation
void testSystemValidation() {
    std::cout << "\nTesting system validation...\n";

    SpeciesManager speciesManager;

    // Add species
    Species A("A", {{"A", 1}}, PhaseType::GAS);
    Species B("B", {{"B", 1}}, PhaseType::GAS);
    Species C("C", {{"C", 1}}, PhaseType::GAS);
    speciesManager.addSpecies(A);
    speciesManager.addSpecies(B);
    speciesManager.addSpecies(C);

    auto reactionManager = createSimpleMechanism();
    ReactionSystem system(speciesManager, reactionManager);

    bool valid = system.validate();
    std::cout << "  → Validation: " << (valid ? "PASS" : "FAIL") << "\n";
    assert(valid);

    std::cout << "  ✓ System validation works\n";
}

// Test system summary
void testSystemSummary() {
    std::cout << "\nTesting system summary...\n";

    SpeciesManager speciesManager;
    speciesManager.loadCommonGasSpecies();

    ReactionManager reactionManager;
    reactionManager.loadH2O2Mechanism();

    ReactionSystem system(speciesManager, reactionManager);

    std::string summary = system.getSummary();
    std::cout << summary << "\n";

    assert(summary.find("Species") != std::string::npos);
    assert(summary.find("Reactions") != std::string::npos);

    std::cout << "  ✓ System summary works\n";
}

// Test fast reaction species
void testFastReactionSpecies() {
    std::cout << "\nTesting fast reaction species...\n";

    SpeciesManager speciesManager;
    auto reactionManager = createSimpleMechanism();
    ReactionSystem system(speciesManager, reactionManager);

    double T = 1000.0;
    double threshold = 1.0e9;  // Fast reactions above this

    auto fastSpecies = system.getFastReactionSpecies(T, threshold);
    std::cout << "  → Fast reaction species: ";
    for (const auto& sp : fastSpecies) {
        std::cout << sp << " ";
    }
    std::cout << "\n";

    // All species involved in fast reactions
    assert(fastSpecies.size() >= 2);

    std::cout << "  ✓ Fast reaction species identification works\n";
}

int main() {
    std::cout << "Phase 19 Tests - Reaction System Integration\n";
    std::cout << "=========================================\n";

    testNetworkConstruction();
    testPureReactants();
    testPureProducts();
    testIntermediates();
    testProducingConsumingReactions();
    testPathwayAnalysis();
    testNetworkStats();
    testReactionSystemConstruction();
    testSourceTermCalculation();
    testAllSourceTerms();
    testReversibleMechanism();
    testComplexNetwork();
    testSystemValidation();
    testSystemSummary();
    testFastReactionSpecies();

    std::cout << "\n=========================================\n";
    std::cout << "All Phase 19 tests passed!\n";
    std::cout << "Reaction system integration verified.\n";

    return 0;
}
