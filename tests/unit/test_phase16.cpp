/**
 * @file test_phase16.cpp
 * @brief Unit tests for Phase 16 - Chemical Species System
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha1
 * @date 2025-11-06
 */

#include "chemistry/species/Species.h"
#include "chemistry/species/SpeciesManager.h"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace koo::chemistry;

// Helper for floating point comparison
bool isClose(double a, double b, double tol = 1e-6) {
    return std::abs(a - b) < tol;
}

void testPhaseType() {
    std::cout << "Testing PhaseType enum...\n";

    assert(toString(PhaseType::GAS) == "Gas");
    assert(toString(PhaseType::LIQUID) == "Liquid");
    assert(toString(PhaseType::SOLID) == "Solid");
    assert(toString(PhaseType::PLASMA) == "Plasma");
    assert(toString(PhaseType::ADSORBED) == "Adsorbed");

    std::cout << "  ✓ PhaseType conversions work\n";
}

void testSpeciesConstruction() {
    std::cout << "\nTesting Species construction...\n";

    // Create H2O
    Species H2O("H2O", {{"H", 2}, {"O", 1}}, PhaseType::GAS);

    assert(H2O.getName() == "H2O");
    assert(H2O.getPhase() == PhaseType::GAS);
    assert(H2O.getCharge() == 0);
    assert(H2O.getTotalAtomCount() == 3);

    std::cout << "  ✓ Basic construction works\n";

    // Test composition
    assert(H2O.getElementCount("H") == 2);
    assert(H2O.getElementCount("O") == 1);
    assert(H2O.getElementCount("N") == 0);

    assert(H2O.hasElement("H"));
    assert(H2O.hasElement("O"));
    assert(!H2O.hasElement("N"));

    std::cout << "  ✓ Composition queries work\n";

    // Test formula
    std::string formula = H2O.getFormula();
    assert(!formula.empty());

    std::cout << "  ✓ Formula generation works: " << formula << "\n";
}

void testSpeciesProperties() {
    std::cout << "\nTesting Species properties...\n";

    Species CO2("CO2", {{"C", 1}, {"O", 2}}, PhaseType::GAS);

    // Set molecular weight
    CO2.setMolecularWeight(0.044);
    assert(isClose(CO2.getMolecularWeight(), 0.044));

    std::cout << "  ✓ Molecular weight setting works\n";

    // Set charge
    CO2.setCharge(0);
    assert(CO2.getCharge() == 0);

    std::cout << "  ✓ Charge setting works\n";

    // Test info string
    std::string info = CO2.getInfo();
    assert(!info.empty());
    assert(info.find("CO2") != std::string::npos);

    std::cout << "  ✓ Info string generation works\n";
}

void testTransportData() {
    std::cout << "\nTesting TransportData...\n";

    Species H2("H2", {{"H", 2}}, PhaseType::GAS);

    TransportData transport;
    transport.molecularWeight = 0.002016;
    transport.lennardJonesSigma = 2.92;
    transport.lennardJonesEpsilon = 38.0;

    H2.setTransportData(transport);

    const auto& retrievedTransport = H2.getTransportData();
    assert(isClose(retrievedTransport.molecularWeight, 0.002016));
    assert(isClose(retrievedTransport.lennardJonesSigma, 2.92));
    assert(isClose(retrievedTransport.lennardJonesEpsilon, 38.0));

    std::cout << "  ✓ Transport data storage works\n";

    // Test diffusivity calculation
    double T = 300.0;  // K
    double P = 101325.0;  // Pa
    double D = transport.getDiffusivity(T, P, 0.032, 3.46, 107.4);  // Diffusing into O2

    assert(D > 0.0);
    assert(std::isfinite(D));

    std::cout << "  ✓ Diffusivity calculation works\n";
    std::cout << "  → D(H2 in O2) at 300K = " << D << " m²/s\n";
}

void testThermoData() {
    std::cout << "\nTesting ThermoData...\n";

    Species N2("N2", {{"N", 2}}, PhaseType::GAS);

    ThermoData thermo;
    thermo.Tmin = 298.15;
    thermo.Tmax = 5000.0;
    thermo.Tmid = 1000.0;
    thermo.H298 = 0.0;
    thermo.S298 = 191.5;
    thermo.Cp298 = 29.1;

    // Set NASA polynomial coefficients (simplified example)
    thermo.lowT = {3.298677, 0.0014082, -3.963222e-06, 5.641515e-09, -2.444855e-12, -1020.9, 3.950372};
    thermo.highT = {2.92664, 0.0014879, -5.68476e-07, 1.0097e-10, -6.753e-15, -922.8, 5.980528};

    N2.setThermoData(thermo);

    const auto& retrievedThermo = N2.getThermoData();
    assert(isClose(retrievedThermo.H298, 0.0));
    assert(isClose(retrievedThermo.S298, 191.5));

    std::cout << "  ✓ Thermo data storage works\n";

    // Test thermodynamic property calculations
    double T = 500.0;  // K
    double Cp = N2.getCp(T);
    double H = N2.getEnthalpy(T);
    double S = N2.getEntropy(T);
    double G = N2.getGibbsEnergy(T);

    assert(std::isfinite(Cp));
    assert(std::isfinite(H));
    assert(std::isfinite(S));
    assert(std::isfinite(G));

    std::cout << "  ✓ Thermodynamic calculations work\n";
    std::cout << "  → Cp at 500K = " << Cp << " J/(mol·K)\n";
    std::cout << "  → H at 500K  = " << H << " J/mol\n";
}

void testSpeciesManager() {
    std::cout << "\nTesting SpeciesManager...\n";

    SpeciesManager manager;

    // Add species
    Species H2("H2", {{"H", 2}}, PhaseType::GAS);
    H2.setMolecularWeight(0.002016);
    manager.addSpecies(H2);

    Species O2("O2", {{"O", 2}}, PhaseType::GAS);
    O2.setMolecularWeight(0.032);
    manager.addSpecies(O2);

    assert(manager.getNumSpecies() == 2);
    std::cout << "  ✓ Species addition works\n";

    // Test retrieval
    auto h2_ptr = manager.getSpecies("H2");
    assert(h2_ptr != nullptr);
    assert(h2_ptr->getName() == "H2");

    auto notFound = manager.getSpecies("N2");
    assert(notFound == nullptr);

    std::cout << "  ✓ Species retrieval works\n";

    // Test existence check
    assert(manager.hasSpecies("H2"));
    assert(manager.hasSpecies("O2"));
    assert(!manager.hasSpecies("N2"));

    std::cout << "  ✓ Existence checks work\n";
}

void testSpeciesManagerQueries() {
    std::cout << "\nTesting SpeciesManager queries...\n";

    SpeciesManager manager;

    // Add different phases
    Species H2O_gas("H2O", {{"H", 2}, {"O", 1}}, PhaseType::GAS);
    manager.addSpecies(H2O_gas);

    Species H2O_liquid("H2O_liquid", {{"H", 2}, {"O", 1}}, PhaseType::LIQUID);
    manager.addSpecies(H2O_liquid);

    Species Fe("Fe", {{"Fe", 1}}, PhaseType::SOLID);
    manager.addSpecies(Fe);

    // Query by phase
    auto gasSpecies = manager.getSpeciesByPhase(PhaseType::GAS);
    assert(gasSpecies.size() == 1);

    auto liquidSpecies = manager.getSpeciesByPhase(PhaseType::LIQUID);
    assert(liquidSpecies.size() == 1);

    auto solidSpecies = manager.getSpeciesByPhase(PhaseType::SOLID);
    assert(solidSpecies.size() == 1);

    std::cout << "  ✓ Phase queries work\n";

    // Query by element
    auto oxygenSpecies = manager.getSpeciesByElement("O");
    assert(oxygenSpecies.size() == 2);

    auto ironSpecies = manager.getSpeciesByElement("Fe");
    assert(ironSpecies.size() == 1);

    std::cout << "  ✓ Element queries work\n";

    // Get all names
    auto names = manager.getSpeciesNames();
    assert(names.size() == 3);

    std::cout << "  ✓ Name listing works\n";
}

void testSpeciesManagerRemoval() {
    std::cout << "\nTesting SpeciesManager removal...\n";

    SpeciesManager manager;

    Species H2("H2", {{"H", 2}}, PhaseType::GAS);
    Species O2("O2", {{"O", 2}}, PhaseType::GAS);
    Species N2("N2", {{"N", 2}}, PhaseType::GAS);

    manager.addSpecies(H2);
    manager.addSpecies(O2);
    manager.addSpecies(N2);

    assert(manager.getNumSpecies() == 3);

    // Remove one species
    bool removed = manager.removeSpecies("O2");
    assert(removed);
    assert(manager.getNumSpecies() == 2);
    assert(!manager.hasSpecies("O2"));

    std::cout << "  ✓ Species removal works\n";

    // Try to remove non-existent
    removed = manager.removeSpecies("CO2");
    assert(!removed);

    std::cout << "  ✓ Non-existent removal handling works\n";

    // Clear all
    manager.clear();
    assert(manager.getNumSpecies() == 0);

    std::cout << "  ✓ Clear all works\n";
}

void testCommonGasDatabase() {
    std::cout << "\nTesting common gas database...\n";

    SpeciesManager manager;
    manager.loadCommonGasSpecies();

    assert(manager.getNumSpecies() > 0);

    std::cout << "  ✓ Common gas database loaded\n";
    std::cout << "  → Total species: " << manager.getNumSpecies() << "\n";

    // Check some common species
    assert(manager.hasSpecies("H2"));
    assert(manager.hasSpecies("O2"));
    assert(manager.hasSpecies("H2O"));
    assert(manager.hasSpecies("N2"));
    assert(manager.hasSpecies("CO"));
    assert(manager.hasSpecies("CO2"));
    assert(manager.hasSpecies("CH4"));
    assert(manager.hasSpecies("Ar"));

    std::cout << "  ✓ Common species present\n";

    // Verify properties
    auto h2o = manager.getSpecies("H2O");
    assert(h2o != nullptr);
    assert(h2o->hasElement("H"));
    assert(h2o->hasElement("O"));
    assert(h2o->getElementCount("H") == 2);
    assert(h2o->getElementCount("O") == 1);

    std::cout << "  ✓ Species properties correct\n";

    // Test transport data
    const auto& transport = h2o->getTransportData();
    assert(transport.molecularWeight > 0.0);
    assert(transport.lennardJonesSigma > 0.0);
    assert(transport.lennardJonesEpsilon > 0.0);

    std::cout << "  ✓ Transport data available\n";

    // Print summary
    std::cout << "\n" << manager.getSummary();
}

void testDuplicateHandling() {
    std::cout << "\nTesting duplicate handling...\n";

    SpeciesManager manager;

    Species H2("H2", {{"H", 2}}, PhaseType::GAS);
    manager.addSpecies(H2);

    // Try to add duplicate
    bool caught = false;
    try {
        manager.addSpecies(H2);
    } catch (const std::runtime_error& e) {
        caught = true;
        std::cout << "  ✓ Caught expected error: " << e.what() << "\n";
    }

    assert(caught);
    std::cout << "  ✓ Duplicate prevention works\n";
}

void testMultiElementSpecies() {
    std::cout << "\nTesting multi-element species...\n";

    // Create CH3OH (methanol)
    Species CH3OH("CH3OH", {{"C", 1}, {"H", 4}, {"O", 1}}, PhaseType::LIQUID);
    CH3OH.setMolecularWeight(0.032042);

    assert(CH3OH.getTotalAtomCount() == 6);
    assert(CH3OH.getElementCount("C") == 1);
    assert(CH3OH.getElementCount("H") == 4);
    assert(CH3OH.getElementCount("O") == 1);

    std::cout << "  ✓ Multi-element composition works\n";

    // Test formula
    std::string formula = CH3OH.getFormula();
    std::cout << "  → Formula: " << formula << "\n";

    std::cout << "  ✓ Complex species handling works\n";
}

void testIonSpecies() {
    std::cout << "\nTesting ion species...\n";

    // Create H+ ion
    Species Hplus("H+", {{"H", 1}}, PhaseType::PLASMA);
    Hplus.setCharge(1);
    Hplus.setMolecularWeight(0.001008);

    assert(Hplus.getCharge() == 1);
    assert(Hplus.getPhase() == PhaseType::PLASMA);

    std::cout << "  ✓ Positive ion works\n";

    // Create OH- ion
    Species OHminus("OH-", {{"O", 1}, {"H", 1}}, PhaseType::PLASMA);
    OHminus.setCharge(-1);

    assert(OHminus.getCharge() == -1);

    std::cout << "  ✓ Negative ion works\n";

    // Test formula with charge
    std::string formula = Hplus.getFormula();
    std::cout << "  → H+ formula: " << formula << "\n";

    std::cout << "  ✓ Ion species handling works\n";
}

int main() {
    std::cout << "Phase 16 Tests - Chemical Species System\n";
    std::cout << "=========================================\n\n";

    try {
        testPhaseType();
        testSpeciesConstruction();
        testSpeciesProperties();
        testTransportData();
        testThermoData();
        testSpeciesManager();
        testSpeciesManagerQueries();
        testSpeciesManagerRemoval();
        testCommonGasDatabase();
        testDuplicateHandling();
        testMultiElementSpecies();
        testIonSpecies();

        std::cout << "\n=========================================\n";
        std::cout << "All Phase 16 tests passed!\n";
        std::cout << "Chemical species system verified.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
