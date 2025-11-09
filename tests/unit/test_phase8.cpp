/**
 * @file test_phase8.cpp
 * @brief Unit tests for Phase 8 - Boundary Condition Management
 * @author KooChemicalSimulation Development Team
 * @version 0.2.0-alpha3
 * @date 2025-11-06
 */

#include "mesh/boundary/BoundaryCondition.h"
#include "mesh/boundary/DirichletBC.h"
#include "mesh/boundary/NeumannBC.h"
#include "mesh/boundary/RobinBC.h"
#include "mesh/boundary/BCManager.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <memory>

using namespace koo::mesh::boundary;

// Helper function for floating point comparison
bool isClose(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) < tol;
}

void testBoundaryConditionBase() {
    std::cout << "Testing BoundaryCondition base class...\n";

    // Test ConstantBC
    auto constBC = std::make_shared<ConstantBC>("test", BCType::DIRICHLET, 1, 5.0);
    assert(constBC->getName() == "test");
    assert(constBC->getType() == BCType::DIRICHLET);
    assert(constBC->getPhysicalTag() == 1);
    assert(isClose(constBC->evaluate(0, 0, 0, 0), 5.0));
    assert(!constBC->isTimeDependent());
    assert(!constBC->isSpatiallyVarying());
    std::cout << "  ✓ ConstantBC works\n";

    // Test TimeDependentBC
    auto timeBC = std::make_shared<TimeDependentBC>(
        "time_bc", BCType::NEUMANN, 2,
        [](double t) { return 2.0 * t; }
    );
    assert(isClose(timeBC->evaluate(0, 0, 0, 1.5), 3.0));
    assert(timeBC->isTimeDependent());
    assert(!timeBC->isSpatiallyVarying());
    std::cout << "  ✓ TimeDependentBC works\n";

    // Test SpatialBC
    auto spatialBC = std::make_shared<SpatialBC>(
        "spatial_bc", BCType::DIRICHLET, 3,
        [](double x, double y, double z) { return x + y + z; }
    );
    assert(isClose(spatialBC->evaluate(1, 2, 3, 0), 6.0));
    assert(!spatialBC->isTimeDependent());
    assert(spatialBC->isSpatiallyVarying());
    std::cout << "  ✓ SpatialBC works\n";

    // Test GeneralBC
    auto generalBC = std::make_shared<GeneralBC>(
        "general_bc", BCType::ROBIN, 4,
        [](double x, double y, double z, double t) { return x + y + z + t; }
    );
    assert(isClose(generalBC->evaluate(1, 2, 3, 4), 10.0));
    assert(generalBC->isTimeDependent());
    assert(generalBC->isSpatiallyVarying());
    std::cout << "  ✓ GeneralBC works\n";

    // Test component and field settings
    constBC->setComponent(Component::VECTOR_X);
    assert(constBC->getComponent() == Component::VECTOR_X);
    constBC->setFieldName("velocity");
    assert(constBC->getFieldName() == "velocity");
    std::cout << "  ✓ Component and field settings work\n";

    // Test enable/disable
    constBC->setEnabled(false);
    assert(!constBC->isEnabled());
    constBC->setEnabled(true);
    assert(constBC->isEnabled());
    std::cout << "  ✓ Enable/disable works\n";
}

void testDirichletBC() {
    std::cout << "\nTesting DirichletBC...\n";

    // Test constant Dirichlet BC
    auto bc1 = DirichletBC::makeConstant("wall_temp", 10, 300.0);
    assert(bc1->getName() == "wall_temp");
    assert(bc1->getType() == BCType::DIRICHLET);
    assert(isClose(bc1->evaluate(0, 0, 0, 0), 300.0));
    assert(!bc1->isTimeDependent());
    assert(!bc1->isSpatiallyVarying());
    std::cout << "  ✓ Constant Dirichlet BC works\n";

    // Test zero BC (homogeneous)
    auto bc2 = dirichlet::makeZero("zero_bc", 11);
    assert(isClose(bc2->evaluate(0, 0, 0, 0), 0.0));
    assert(bc2->isHomogeneous());
    std::cout << "  ✓ Zero (homogeneous) BC works\n";

    // Test sinusoidal BC
    auto bc3 = dirichlet::makeSinusoidal("sine_bc", 12, 10.0, 1.0, 0.0);
    assert(isClose(bc3->evaluate(0, 0, 0, 0.0), 0.0));
    assert(isClose(bc3->evaluate(0, 0, 0, 0.25), 10.0));  // sin(π/2) = 1
    assert(bc3->isTimeDependent());
    std::cout << "  ✓ Sinusoidal BC works\n";

    // Test ramp BC
    auto bc4 = dirichlet::makeRamp("ramp_bc", 13, 0.0, 100.0, 10.0);
    assert(isClose(bc4->evaluate(0, 0, 0, 0.0), 0.0));
    assert(isClose(bc4->evaluate(0, 0, 0, 5.0), 50.0));
    assert(isClose(bc4->evaluate(0, 0, 0, 10.0), 100.0));
    assert(isClose(bc4->evaluate(0, 0, 0, 15.0), 100.0));  // Clamped
    std::cout << "  ✓ Ramp BC works\n";

    // Test step BC
    auto bc5 = dirichlet::makeStep("step_bc", 14, 0.0, 1.0, 5.0);
    assert(isClose(bc5->evaluate(0, 0, 0, 3.0), 0.0));
    assert(isClose(bc5->evaluate(0, 0, 0, 7.0), 1.0));
    std::cout << "  ✓ Step BC works\n";

    // Test parabolic profile
    auto bc6 = dirichlet::makeParabolicProfile("parabolic", 15, 1.0, 0.0, 1.0, 'y');
    assert(isClose(bc6->evaluate(0, 0.0, 0, 0), 1.0));  // Center
    assert(isClose(bc6->evaluate(0, 1.0, 0, 0), 0.0));  // Edge
    assert(bc6->isSpatiallyVarying());
    std::cout << "  ✓ Parabolic profile BC works\n";

    // Test custom spatial BC
    auto bc7 = DirichletBC::makeSpatial("custom", 16,
        [](double x, double y, double z) { return x * x + y * y; }
    );
    assert(isClose(bc7->evaluate(3, 4, 0, 0), 25.0));
    std::cout << "  ✓ Custom spatial BC works\n";
}

void testNeumannBC() {
    std::cout << "\nTesting NeumannBC...\n";

    // Test constant flux BC
    auto bc1 = NeumannBC::makeConstant("heat_flux", 20, 1000.0);
    assert(isClose(bc1->evaluate(0, 0, 0, 0), 1000.0));
    assert(!bc1->isTimeDependent());
    std::cout << "  ✓ Constant flux BC works\n";

    // Test zero flux (insulated)
    auto bc2 = neumann::makeZeroFlux("insulated", 21);
    assert(isClose(bc2->evaluate(0, 0, 0, 0), 0.0));
    assert(bc2->isHomogeneous());
    assert(bc2->isInsulated());
    std::cout << "  ✓ Zero flux (insulated) BC works\n";

    // Test pulsating flux
    auto bc3 = neumann::makePulsating("pulse", 22, 100.0, 1.0, 0.0);
    assert(isClose(bc3->evaluate(0, 0, 0, 0.0), 0.0));
    assert(isClose(bc3->evaluate(0, 0, 0, 0.25), 100.0));
    assert(bc3->isTimeDependent());
    std::cout << "  ✓ Pulsating flux BC works\n";

    // Test exponential decay
    auto bc4 = neumann::makeExponentialDecay("decay", 23, 1000.0, 0.1);
    assert(isClose(bc4->evaluate(0, 0, 0, 0.0), 1000.0));
    double expected = 1000.0 * std::exp(-0.1 * 5.0);
    (void)expected;  // Used in assert
    assert(isClose(bc4->evaluate(0, 0, 0, 5.0), expected));
    std::cout << "  ✓ Exponential decay BC works\n";

    // Test Gaussian spatial distribution
    auto bc5 = neumann::makeGaussian("gaussian", 24, 100.0, 0.0, 0.0, 1.0);
    assert(isClose(bc5->evaluate(0, 0, 0, 0), 100.0));  // Center
    assert(bc5->isSpatiallyVarying());
    std::cout << "  ✓ Gaussian spatial BC works\n";

    // Test coefficient
    auto bc6 = NeumannBC::makeConstant("test", 25, 10.0);
    bc6->setCoefficient(5.0);
    assert(isClose(bc6->getCoefficient(), 5.0));
    std::cout << "  ✓ Coefficient setting works\n";
}

void testRobinBC() {
    std::cout << "\nTesting RobinBC...\n";

    // Test constant Robin BC
    auto bc1 = RobinBC::makeConstant("robin", 30, 1.0, 2.0, 3.0);
    assert(isClose(bc1->getAlpha(), 1.0));
    assert(isClose(bc1->getBeta(), 2.0));
    assert(isClose(bc1->getGamma(0, 0, 0, 0), 3.0));
    std::cout << "  ✓ Constant Robin BC works\n";

    // Test convection BC
    double h = 10.0;   // Heat transfer coefficient
    double k = 0.5;    // Thermal conductivity
    double Tinf = 300.0;  // Ambient temperature
    auto bc2 = RobinBC::makeConvection("convection", 31, h, k, Tinf);
    assert(isClose(bc2->getAlpha(), h));
    assert(isClose(bc2->getBeta(), k));
    assert(isClose(bc2->getGamma(0, 0, 0, 0), h * Tinf));
    std::cout << "  ✓ Convection BC works\n";

    // Test helper functions
    auto bc3 = robin::makeConvection("conv2", 32, 5.0, 1.0, 273.0);
    assert(bc3->getType() == BCType::ROBIN);
    std::cout << "  ✓ Robin helper functions work\n";

    // Test impedance BC
    auto bc4 = robin::makeImpedance("impedance", 33, 377.0);
    assert(isClose(bc4->getAlpha(), 377.0));
    assert(isClose(bc4->getBeta(), 1.0));
    std::cout << "  ✓ Impedance BC works\n";

    // Test absorbing BC
    auto bc5 = robin::makeAbsorbing("absorb", 34, 343.0);
    assert(isClose(bc5->getAlpha(), 343.0));
    std::cout << "  ✓ Absorbing BC works\n";

    // Test Dirichlet-like check (beta = 0)
    auto bc6 = RobinBC::makeConstant("dirichlet_like", 35, 1.0, 0.0, 5.0);
    assert(bc6->isDirichletLike());
    assert(!bc6->isNeumannLike());
    std::cout << "  ✓ Dirichlet-like check works\n";

    // Test Neumann-like check (alpha = 0)
    auto bc7 = RobinBC::makeConstant("neumann_like", 36, 0.0, 1.0, 5.0);
    assert(!bc7->isDirichletLike());
    assert(bc7->isNeumannLike());
    std::cout << "  ✓ Neumann-like check works\n";

    // Test time-varying convection
    auto bc8 = robin::makeTimeVaryingConvection("time_conv", 37, 10.0, 1.0,
        [](double t) { return 300.0 + 50.0 * std::sin(t); }
    );
    assert(bc8->isTimeDependent());
    std::cout << "  ✓ Time-varying convection BC works\n";
}

void testBCManager() {
    std::cout << "\nTesting BCManager...\n";

    BCManager manager;

    // Add Dirichlet BCs
    auto bc1 = DirichletBC::makeConstant("inlet", 10, 1.0);
    bc1->setFieldName("velocity");
    manager.addBC(bc1);

    auto bc2 = DirichletBC::makeConstant("wall", 11, 0.0);
    bc2->setFieldName("velocity");
    manager.addBC(bc2);

    // Add Neumann BC
    auto bc3 = NeumannBC::makeConstant("outlet", 12, 0.0);
    bc3->setFieldName("pressure");
    manager.addBC(bc3);

    // Add Robin BC
    auto bc4 = RobinBC::makeConvection("surface", 13, 10.0, 1.0, 300.0);
    bc4->setFieldName("temperature");
    manager.addBC(bc4);

    // Test basic queries
    assert(manager.getNumBCs() == 4);
    assert(manager.hasBC("inlet"));
    assert(!manager.hasBC("nonexistent"));
    std::cout << "  ✓ Basic queries work\n";

    // Test get by name
    auto retrieved = manager.getBC("inlet");
    assert(retrieved != nullptr);
    assert(retrieved->getName() == "inlet");
    std::cout << "  ✓ Get by name works\n";

    // Test get by type
    auto dirichletBCs = manager.getBCsByType(BCType::DIRICHLET);
    assert(dirichletBCs.size() == 2);

    auto neumannBCs = manager.getBCsByType(BCType::NEUMANN);
    assert(neumannBCs.size() == 1);

    auto robinBCs = manager.getBCsByType(BCType::ROBIN);
    assert(robinBCs.size() == 1);
    std::cout << "  ✓ Get by type works\n";

    // Test get by tag
    auto tag10BCs = manager.getBCsByTag(10);
    assert(tag10BCs.size() == 1);
    assert(tag10BCs[0]->getName() == "inlet");
    std::cout << "  ✓ Get by tag works\n";

    // Test get by field
    auto velocityBCs = manager.getBCsByField("velocity");
    assert(velocityBCs.size() == 2);

    auto pressureBCs = manager.getBCsByField("pressure");
    assert(pressureBCs.size() == 1);

    auto tempBCs = manager.getBCsByField("temperature");
    assert(tempBCs.size() == 1);
    std::cout << "  ✓ Get by field works\n";

    // Test typed getters
    auto dBCs = manager.getDirichletBCs();
    assert(dBCs.size() == 2);

    auto nBCs = manager.getNeumannBCs();
    assert(nBCs.size() == 1);

    auto rBCs = manager.getRobinBCs();
    assert(rBCs.size() == 1);
    std::cout << "  ✓ Typed getters work\n";

    // Test enable/disable
    manager.setEnabled("inlet", false);
    auto enabledBCs = manager.getEnabledBCs();
    assert(enabledBCs.size() == 3);
    manager.setEnabled("inlet", true);
    std::cout << "  ✓ Enable/disable works\n";

    // Test remove
    bool removed = manager.removeBC("inlet");
    (void)removed;  // Used in assert
    assert(removed);
    assert(manager.getNumBCs() == 3);
    assert(!manager.hasBC("inlet"));
    std::cout << "  ✓ Remove BC works\n";

    // Test validation
    auto errors = manager.validate();
    assert(errors.empty());  // No conflicts
    std::cout << "  ✓ Validation works\n";

    // Test clear
    manager.clear();
    assert(manager.getNumBCs() == 0);
    std::cout << "  ✓ Clear works\n";

    // Test duplicate name error
    manager.addBC(bc2);  // wall
    bool caught = false;
    try {
        manager.addBC(bc2);  // Try to add same BC again
    } catch (const std::runtime_error&) {
        caught = true;
    }
    (void)caught;  // Used in assert
    assert(caught);
    std::cout << "  ✓ Duplicate name detection works\n";
}

void testBCManagerConflicts() {
    std::cout << "\nTesting BCManager conflict detection...\n";

    BCManager manager;

    // Add multiple BCs for same field on same tag
    auto bc1 = DirichletBC::makeConstant("bc1", 100, 1.0);
    bc1->setFieldName("velocity");
    manager.addBC(bc1);

    auto bc2 = DirichletBC::makeConstant("bc2", 100, 2.0);
    bc2->setFieldName("velocity");
    manager.addBC(bc2);

    // This should produce a validation error
    auto errors = manager.validate();
    assert(errors.size() > 0);
    std::cout << "  ✓ Conflict detection works\n";
    std::cout << "    Detected conflict: " << errors[0] << "\n";
}

int main() {
    std::cout << "Phase 8 Tests - Boundary Condition Management\n";
    std::cout << "==============================================\n\n";

    testBoundaryConditionBase();
    testDirichletBC();
    testNeumannBC();
    testRobinBC();
    testBCManager();
    testBCManagerConflicts();

    std::cout << "\n==============================================\n";
    std::cout << "All Phase 8 tests passed!\n";
    return 0;
}
