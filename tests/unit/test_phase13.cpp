/**
 * @file test_phase13.cpp
 * @brief Unit tests for Phase 13 - Solver Factory Pattern
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha3
 * @date 2025-11-06
 */

#include "solver/SolverFactory.h"
#include "solver/MockSolver.h"
#include "solver/pde/ISolver.h"
#include "solver/pde/SolverTypes.h"

#include <iostream>
#include <cassert>
#include <memory>
#include <string>

using namespace koo::solver;
using namespace koo::solver::pde;

// Custom test solver for registration tests
class TestSolver : public BaseSolver {
public:
    TestSolver() : BaseSolver() {
        pdeType_ = PDEType::ELLIPTIC;
    }

    SolverBackend getBackend() const override {
        return SolverBackend::CUSTOM;
    }

    std::string getName() const override {
        return "TestSolver";
    }

    std::string getVersion() const override {
        return "1.0.0-test";
    }

    bool initialize(std::shared_ptr<koo::mesh::MeshManager>) override {
        status_ = SolverStatus::INITIALIZED;
        initialized_ = true;
        return true;
    }

    bool assemble() override { return true; }
    bool applyBoundaryConditions() override { return true; }

    ConvergenceInfo solve() override {
        ConvergenceInfo info;
        info.status = SolverStatus::CONVERGED;
        return info;
    }

    double getSolutionAt(double, double, double) const override { return 0.0; }
    bool exportSolution(const std::string&, const std::string&) const override { return true; }
};

void testFactorySingleton() {
    std::cout << "Testing SolverFactory singleton...\n";

    auto& factory1 = SolverFactory::getInstance();
    auto& factory2 = SolverFactory::getInstance();

    // Should be the same instance
    assert(&factory1 == &factory2);

    std::cout << "  ✓ Singleton pattern works\n";
}

void testBuiltInSolvers() {
    std::cout << "\nTesting built-in solver registration...\n";

    auto& factory = SolverFactory::getInstance();

    // Check MockSolver is registered
    assert(factory.isRegistered(SolverBackend::CUSTOM));
    assert(factory.isRegistered("MockSolver"));
    assert(factory.isRegistered("Mock"));

    std::cout << "  ✓ CUSTOM backend registered\n";
    std::cout << "  ✓ MockSolver name registered\n";
    std::cout << "  ✓ Mock alias registered\n";

    // Get registered backends and names
    auto backends = factory.getRegisteredBackends();
    auto names = factory.getRegisteredNames();

    assert(!backends.empty());
    assert(!names.empty());

    std::cout << "  ✓ " << backends.size() << " backend(s) registered\n";
    std::cout << "  ✓ " << names.size() << " name(s) registered\n";
}

void testCreateSolverByBackend() {
    std::cout << "\nTesting solver creation by backend...\n";

    auto& factory = SolverFactory::getInstance();

    // Create MockSolver via CUSTOM backend
    auto solver = factory.createSolver(SolverBackend::CUSTOM);
    assert(solver != nullptr);
    assert(solver->getBackend() == SolverBackend::CUSTOM);
    assert(solver->getName() == "MockSolver");

    std::cout << "  ✓ Created solver via CUSTOM backend\n";
    std::cout << "  → Solver name: " << solver->getName() << "\n";
    std::cout << "  → Solver backend: " << toString(solver->getBackend()) << "\n";
    std::cout << "  → Solver version: " << solver->getVersion() << "\n";
}

void testCreateSolverByName() {
    std::cout << "\nTesting solver creation by name...\n";

    auto& factory = SolverFactory::getInstance();

    // Create by full name
    auto solver1 = factory.createSolver("MockSolver");
    assert(solver1 != nullptr);
    assert(solver1->getName() == "MockSolver");

    std::cout << "  ✓ Created solver by name 'MockSolver'\n";

    // Create by alias
    auto solver2 = factory.createSolver("Mock");
    assert(solver2 != nullptr);
    assert(solver2->getName() == "MockSolver");

    std::cout << "  ✓ Created solver by alias 'Mock'\n";

    // Each call should create a new instance
    assert(solver1.get() != solver2.get());

    std::cout << "  ✓ Each creation produces new instance\n";
}

void testHelperFunctions() {
    std::cout << "\nTesting helper functions...\n";

    // Test createSolver helper by backend
    auto solver1 = createSolver(SolverBackend::CUSTOM);
    assert(solver1 != nullptr);
    assert(solver1->getBackend() == SolverBackend::CUSTOM);

    std::cout << "  ✓ createSolver(backend) helper works\n";

    // Test createSolver helper by name
    auto solver2 = createSolver("MockSolver");
    assert(solver2 != nullptr);
    assert(solver2->getName() == "MockSolver");

    std::cout << "  ✓ createSolver(name) helper works\n";
}

void testRegisterCustomSolver() {
    std::cout << "\nTesting custom solver registration...\n";

    auto& factory = SolverFactory::getInstance();

    // Register TestSolver by backend
    SolverBackend testBackend = SolverBackend::DEALII;  // Use unused backend
    factory.registerSolver(testBackend, []() {
        return std::make_shared<TestSolver>();
    });

    assert(factory.isRegistered(testBackend));
    std::cout << "  ✓ Registered TestSolver by backend\n";

    // Create and verify
    auto solver1 = factory.createSolver(testBackend);
    assert(solver1 != nullptr);
    assert(solver1->getName() == "TestSolver");

    std::cout << "  ✓ Created TestSolver via backend\n";

    // Register by name
    factory.registerSolver("CustomTest", []() {
        return std::make_shared<TestSolver>();
    });

    assert(factory.isRegistered("CustomTest"));
    std::cout << "  ✓ Registered TestSolver by name\n";

    // Create and verify
    auto solver2 = factory.createSolver("CustomTest");
    assert(solver2 != nullptr);
    assert(solver2->getName() == "TestSolver");

    std::cout << "  ✓ Created TestSolver via name\n";

    // Clean up
    factory.unregisterSolver(testBackend);
    factory.unregisterSolver("CustomTest");

    std::cout << "  ✓ Cleaned up registrations\n";
}

void testUnregisterSolver() {
    std::cout << "\nTesting solver unregistration...\n";

    auto& factory = SolverFactory::getInstance();

    // Register temporary solver
    SolverBackend tempBackend = SolverBackend::FENICS;
    factory.registerSolver(tempBackend, []() {
        return std::make_shared<TestSolver>();
    });

    assert(factory.isRegistered(tempBackend));
    std::cout << "  ✓ Registered temporary solver\n";

    // Unregister
    bool result = factory.unregisterSolver(tempBackend);
    assert(result == true);
    assert(!factory.isRegistered(tempBackend));

    std::cout << "  ✓ Unregistered solver by backend\n";

    // Try to unregister non-existent
    result = factory.unregisterSolver(tempBackend);
    assert(result == false);

    std::cout << "  ✓ Unregister non-existent returns false\n";

    // Test name-based unregistration
    factory.registerSolver("TempSolver", []() {
        return std::make_shared<TestSolver>();
    });

    assert(factory.isRegistered("TempSolver"));
    result = factory.unregisterSolver("TempSolver");
    assert(result == true);
    assert(!factory.isRegistered("TempSolver"));

    std::cout << "  ✓ Unregistered solver by name\n";
}

void testErrorHandling() {
    std::cout << "\nTesting error handling...\n";

    auto& factory = SolverFactory::getInstance();

    // Try to create unregistered backend
    bool caught = false;
    try {
        auto solver = factory.createSolver(SolverBackend::MFEM);
    } catch (const std::runtime_error& e) {
        caught = true;
        std::cout << "  ✓ Caught expected error: " << e.what() << "\n";
    }
    assert(caught);

    std::cout << "  ✓ Unregistered backend throws exception\n";

    // Try to create unregistered name
    caught = false;
    try {
        auto solver = factory.createSolver("NonExistentSolver");
    } catch (const std::runtime_error& e) {
        caught = true;
        std::cout << "  ✓ Caught expected error: " << e.what() << "\n";
    }
    assert(caught);

    std::cout << "  ✓ Unregistered name throws exception\n";

    // Try to double-register backend
    SolverBackend dupBackend = SolverBackend::DEALII;
    factory.registerSolver(dupBackend, []() {
        return std::make_shared<TestSolver>();
    });

    caught = false;
    try {
        factory.registerSolver(dupBackend, []() {
            return std::make_shared<TestSolver>();
        });
    } catch (const std::runtime_error& e) {
        caught = true;
        std::cout << "  ✓ Caught expected error: " << e.what() << "\n";
    }
    assert(caught);

    std::cout << "  ✓ Double registration throws exception\n";

    // Clean up
    factory.unregisterSolver(dupBackend);

    // Try to double-register name
    factory.registerSolver("DupTest", []() {
        return std::make_shared<TestSolver>();
    });

    caught = false;
    try {
        factory.registerSolver("DupTest", []() {
            return std::make_shared<TestSolver>();
        });
    } catch (const std::runtime_error& e) {
        caught = true;
        std::cout << "  ✓ Caught expected error: " << e.what() << "\n";
    }
    assert(caught);

    std::cout << "  ✓ Double name registration throws exception\n";

    // Clean up
    factory.unregisterSolver("DupTest");
}

void testFactoryInfo() {
    std::cout << "\nTesting factory information...\n";

    auto& factory = SolverFactory::getInstance();

    // Get info string
    std::string info = factory.getInfo();
    assert(!info.empty());

    std::cout << "\n" << info;

    // Test count functions
    size_t totalCount = factory.getNumRegistered();
    auto backends = factory.getRegisteredBackends();
    auto names = factory.getRegisteredNames();

    assert(totalCount == backends.size() + names.size());

    std::cout << "  ✓ Total registered: " << totalCount << "\n";
    std::cout << "  ✓ Info string generation works\n";
}

void testSolverUsage() {
    std::cout << "\nTesting solver usage via factory...\n";

    // Create solver through factory
    auto solver = createSolver("MockSolver");
    assert(solver != nullptr);

    std::cout << "  ✓ Solver created via factory\n";

    // Verify it's a working MockSolver
    assert(solver->getName() == "MockSolver");
    assert(solver->getBackend() == SolverBackend::CUSTOM);
    assert(!solver->isInitialized());

    std::cout << "  ✓ Solver properties correct\n";

    // Test that we can use the solver
    auto options = solver->getOptions();
    assert(options.getMaxIterations() > 0);

    std::cout << "  ✓ Solver options accessible\n";

    // Set new options
    SolverOptions newOpts;
    newOpts.setMaxIterations(500);
    newOpts.setTolerance(1e-8);
    solver->setOptions(newOpts);

    assert(solver->getOptions().getMaxIterations() == 500);

    std::cout << "  ✓ Solver configuration works\n";
}

void testMultipleSolverTypes() {
    std::cout << "\nTesting multiple solver types...\n";

    auto& factory = SolverFactory::getInstance();

    // Register multiple custom solvers
    factory.registerSolver("Solver1", []() {
        return std::make_shared<TestSolver>();
    });

    factory.registerSolver("Solver2", []() {
        return std::make_shared<MockSolver>();
    });

    factory.registerSolver("Solver3", []() {
        return std::make_shared<TestSolver>();
    });

    std::cout << "  ✓ Registered 3 custom solvers\n";

    // Create each one
    auto s1 = factory.createSolver("Solver1");
    auto s2 = factory.createSolver("Solver2");
    auto s3 = factory.createSolver("Solver3");

    assert(s1 != nullptr);
    assert(s2 != nullptr);
    assert(s3 != nullptr);

    assert(s1->getName() == "TestSolver");
    assert(s2->getName() == "MockSolver");
    assert(s3->getName() == "TestSolver");

    std::cout << "  ✓ Created all 3 solver instances\n";
    std::cout << "  ✓ Each has correct type\n";

    // Clean up
    factory.unregisterSolver("Solver1");
    factory.unregisterSolver("Solver2");
    factory.unregisterSolver("Solver3");

    std::cout << "  ✓ Cleaned up registrations\n";
}

void testRegistrationRAII() {
    std::cout << "\nTesting RAII registration...\n";

    auto& factory = SolverFactory::getInstance();

    // Test scope-based registration
    {
        SolverRegistration reg("ScopedSolver", []() {
            return std::make_shared<TestSolver>();
        });

        assert(factory.isRegistered("ScopedSolver"));
        std::cout << "  ✓ RAII registration works\n";

        // Can create solver within scope
        auto solver = factory.createSolver("ScopedSolver");
        assert(solver != nullptr);
        std::cout << "  ✓ Can create solver within scope\n";
    }

    // Should be unregistered after scope
    assert(!factory.isRegistered("ScopedSolver"));
    std::cout << "  ✓ RAII unregistration on scope exit\n";

    // Verify can't create anymore
    bool caught = false;
    try {
        auto solver = factory.createSolver("ScopedSolver");
    } catch (const std::runtime_error&) {
        caught = true;
    }
    assert(caught);

    std::cout << "  ✓ Solver unavailable after scope exit\n";
}

int main() {
    std::cout << "Phase 13 Tests - Solver Factory Pattern\n";
    std::cout << "========================================\n\n";

    try {
        testFactorySingleton();
        testBuiltInSolvers();
        testCreateSolverByBackend();
        testCreateSolverByName();
        testHelperFunctions();
        testRegisterCustomSolver();
        testUnregisterSolver();
        testErrorHandling();
        testFactoryInfo();
        testSolverUsage();
        testMultipleSolverTypes();
        testRegistrationRAII();

        std::cout << "\n========================================\n";
        std::cout << "All Phase 13 tests passed!\n";
        std::cout << "Solver factory pattern verified.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
