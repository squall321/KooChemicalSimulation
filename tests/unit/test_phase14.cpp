/**
 * @file test_phase14.cpp
 * @brief Unit tests for Phase 14 - Solver Strategy and Configuration
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha4
 * @date 2025-11-06
 */

#include "solver/SolverStrategy.h"
#include "solver/SolverConfig.h"
#include "solver/SolverFactory.h"
#include "solver/MockSolver.h"
#include "mesh/MeshManager.h"

#include <iostream>
#include <cassert>
#include <memory>

using namespace koo::solver;
using namespace koo::solver::pde;
using namespace koo::mesh;

// Helper to create simple test mesh
std::shared_ptr<MeshManager> createTestMesh(int numNodes = 11) {
    auto meshManager = std::make_shared<MeshManager>();
    auto meshData = std::make_shared<koo::mesh::core::MeshData>();

    for (int i = 0; i < numNodes; ++i) {
        double x = static_cast<double>(i) / (numNodes - 1);
        koo::mesh::core::Node node(i, x, 0.0, 0.0);
        meshData->addNode(node);
    }

    for (int i = 0; i < numNodes - 1; ++i) {
        koo::mesh::core::Element elem(i, koo::mesh::core::ElementType::LINE,
                                       {static_cast<size_t>(i), static_cast<size_t>(i + 1)});
        meshData->addElement(elem);
    }

    meshManager->setMesh(meshData);
    return meshManager;
}

void testBasicStrategy() {
    std::cout << "Testing BasicStrategy...\n";

    // Create solver
    auto solver = createSolver("MockSolver");
    auto meshManager = createTestMesh();
    solver->initialize(meshManager);

    // Create basic strategy
    auto strategy = std::make_shared<BasicStrategy>();

    assert(strategy->getName() == "BasicStrategy");
    assert(!strategy->getDescription().empty());

    std::cout << "  ✓ BasicStrategy created\n";
    std::cout << "  → Name: " << strategy->getName() << "\n";
    std::cout << "  → Description: " << strategy->getDescription() << "\n";

    // Execute strategy
    auto info = strategy->execute(solver);

    std::cout << "  ✓ Strategy executed\n";
    std::cout << "  → Status: " << ConvergenceInfo::statusToString(info.status) << "\n";
}

void testBasicStrategyWithHooks() {
    std::cout << "\nTesting BasicStrategy with hooks...\n";

    auto solver = createSolver("MockSolver");
    auto meshManager = createTestMesh();
    solver->initialize(meshManager);

    auto strategy = std::make_shared<BasicStrategy>();

    // Add preprocessing hook
    bool preprocessCalled = false;
    strategy->addPreprocessHook([&preprocessCalled](ISolver& s) {
        preprocessCalled = true;
        std::cout << "  → Preprocessing hook called\n";
        return true;
    });

    // Add postprocessing hook
    bool postprocessCalled = false;
    strategy->addPostprocessHook([&postprocessCalled](ISolver& s, const ConvergenceInfo& info) {
        postprocessCalled = true;
        std::cout << "  → Postprocessing hook called\n";
        return true;
    });

    assert(strategy->getNumPreprocessHooks() == 1);
    assert(strategy->getNumPostprocessHooks() == 1);

    std::cout << "  ✓ Hooks registered (" << strategy->getNumPreprocessHooks()
              << " preprocess, " << strategy->getNumPostprocessHooks() << " postprocess)\n";

    // Execute
    auto info = strategy->execute(solver);

    assert(preprocessCalled);
    assert(postprocessCalled);

    std::cout << "  ✓ Hooks executed\n";

    // Clear hooks
    strategy->clearHooks();
    assert(strategy->getNumPreprocessHooks() == 0);
    assert(strategy->getNumPostprocessHooks() == 0);

    std::cout << "  ✓ Hooks cleared\n";
}

void testTimeSteppingStrategy() {
    std::cout << "\nTesting TimeSteppingStrategy...\n";

    auto solver = createSolver("MockSolver");
    auto meshManager = createTestMesh();
    solver->initialize(meshManager);

    // Create time-stepping strategy
    double tStart = 0.0;
    double tEnd = 0.1;
    double dt = 0.01;

    auto strategy = std::make_shared<TimeSteppingStrategy>(tStart, tEnd, dt);

    assert(strategy->getName() == "TimeSteppingStrategy");
    assert(!strategy->getDescription().empty());
    assert(strategy->getCurrentTime() == tStart);
    assert(strategy->getTimeStep() == dt);

    std::cout << "  ✓ TimeSteppingStrategy created\n";
    std::cout << "  → Time range: [" << tStart << ", " << tEnd << "] dt=" << dt << "\n";

    // Execute
    auto info = strategy->execute(solver);

    std::cout << "  ✓ Strategy executed\n";
    std::cout << "  → Status: " << ConvergenceInfo::statusToString(info.status) << "\n";
    std::cout << "  → Message: " << info.message << "\n";
}

void testAdaptiveStrategy() {
    std::cout << "\nTesting AdaptiveStrategy...\n";

    auto solver = createSolver("MockSolver");
    auto meshManager = createTestMesh();
    solver->initialize(meshManager);

    // Create adaptive strategy
    auto strategy = std::make_shared<AdaptiveStrategy>();
    strategy->setMaxRetries(3);

    assert(strategy->getName() == "AdaptiveStrategy");
    assert(!strategy->getDescription().empty());

    std::cout << "  ✓ AdaptiveStrategy created\n";
    std::cout << "  → Max retries: 3\n";

    // Execute
    auto info = strategy->execute(solver);

    std::cout << "  ✓ Strategy executed\n";
    std::cout << "  → Status: " << ConvergenceInfo::statusToString(info.status) << "\n";
    std::cout << "  → Retry count: " << strategy->getRetryCount() << "\n";
}

void testStrategyManager() {
    std::cout << "\nTesting StrategyManager...\n";

    auto solver = createSolver("MockSolver");
    auto meshManager = createTestMesh();
    solver->initialize(meshManager);

    StrategyManager manager;

    // Test with basic strategy
    auto basicStrategy = StrategyManager::createBasicStrategy();
    auto info1 = manager.execute(solver, basicStrategy);

    assert(info1.status != SolverStatus::ERROR);
    std::cout << "  ✓ Executed with BasicStrategy\n";

    // Reset solver
    solver->reset();
    solver->initialize(meshManager);

    // Test with adaptive strategy
    auto adaptiveStrategy = StrategyManager::createAdaptiveStrategy();
    auto info2 = manager.execute(solver, adaptiveStrategy);

    std::cout << "  ✓ Executed with AdaptiveStrategy\n";

    // Test with null solver
    auto info3 = manager.execute(nullptr, basicStrategy);
    assert(info3.status == SolverStatus::ERROR);

    std::cout << "  ✓ Error handling for null solver\n";

    // Test with null strategy
    auto info4 = manager.execute(solver, nullptr);
    assert(info4.status == SolverStatus::ERROR);

    std::cout << "  ✓ Error handling for null strategy\n";
}

void testSolverConfig() {
    std::cout << "\nTesting SolverConfig...\n";

    SolverConfig config;

    // Test default construction
    assert(config.getBackend() == SolverBackend::CUSTOM);
    assert(config.getStrategy() == StrategyType::BASIC);

    std::cout << "  ✓ Default configuration created\n";

    // Set backend
    config.setBackend(SolverBackend::CUSTOM);
    assert(config.getBackend() == SolverBackend::CUSTOM);

    std::cout << "  ✓ Backend configuration works\n";

    // Set solver name
    config.setSolverName("MockSolver");
    assert(config.getSolverName() == "MockSolver");

    std::cout << "  ✓ Solver name configuration works\n";

    // Set options
    SolverOptions opts;
    opts.setMaxIterations(500);
    opts.setTolerance(1e-8);
    config.setOptions(opts);

    assert(config.getOptions().getMaxIterations() == 500);

    std::cout << "  ✓ Options configuration works\n";

    // Set strategy
    config.setStrategy(StrategyType::ADAPTIVE);
    assert(config.getStrategy() == StrategyType::ADAPTIVE);

    std::cout << "  ✓ Strategy configuration works\n";

    // Set time range
    config.setTimeRange(0.0, 1.0, 0.01);
    assert(config.getStartTime() == 0.0);
    assert(config.getEndTime() == 1.0);
    assert(config.getTimeStep() == 0.01);

    std::cout << "  ✓ Time range configuration works\n";

    // Set max retries
    config.setMaxRetries(5);
    assert(config.getMaxRetries() == 5);

    std::cout << "  ✓ Max retries configuration works\n";

    // Get summary
    std::string summary = config.getSummary();
    assert(!summary.empty());

    std::cout << "  ✓ Configuration summary:\n";
    std::cout << summary;
}

void testSolverConfigCreation() {
    std::cout << "\nTesting SolverConfig solver creation...\n";

    SolverConfig config;
    config.setSolverName("MockSolver");

    // Create solver
    auto solver = config.createSolver();
    assert(solver != nullptr);
    assert(solver->getName() == "MockSolver");

    std::cout << "  ✓ Solver created from config\n";

    // Create strategy
    config.setStrategy(StrategyType::BASIC);
    auto strategy = config.createStrategy();
    assert(strategy != nullptr);

    std::cout << "  ✓ Strategy created from config\n";
}

void testSolverConfigPresets() {
    std::cout << "\nTesting SolverConfig presets...\n";

    // Fast config
    auto fastConfig = SolverConfig::createFastConfig();
    assert(fastConfig.getBackend() == SolverBackend::CUSTOM);
    assert(fastConfig.getStrategy() == StrategyType::BASIC);

    std::cout << "  ✓ Fast config created\n";

    // Accurate config
    auto accurateConfig = SolverConfig::createAccurateConfig();
    assert(accurateConfig.getBackend() == SolverBackend::CUSTOM);

    std::cout << "  ✓ Accurate config created\n";

    // Time-dependent config
    auto timeConfig = SolverConfig::createTimeDependentConfig(0.0, 1.0, 0.01);
    assert(timeConfig.getStrategy() == StrategyType::TIME_STEPPING);
    assert(timeConfig.getStartTime() == 0.0);
    assert(timeConfig.getEndTime() == 1.0);
    assert(timeConfig.getTimeStep() == 0.01);

    std::cout << "  ✓ Time-dependent config created\n";

    // Robust config
    auto robustConfig = SolverConfig::createRobustConfig();
    assert(robustConfig.getStrategy() == StrategyType::ADAPTIVE);
    assert(robustConfig.getMaxRetries() == 5);

    std::cout << "  ✓ Robust config created\n";
}

void testSolverBuilder() {
    std::cout << "\nTesting SolverBuilder...\n";

    // Build configuration using fluent interface
    auto config = SolverBuilder()
        .withName("MockSolver")
        .withStrategy(StrategyType::ADAPTIVE)
        .withMaxRetries(3)
        .build();

    assert(config.getSolverName() == "MockSolver");
    assert(config.getStrategy() == StrategyType::ADAPTIVE);
    assert(config.getMaxRetries() == 3);

    std::cout << "  ✓ SolverBuilder fluent interface works\n";

    // Build solver directly
    auto solver = SolverBuilder()
        .withName("MockSolver")
        .buildSolver();

    assert(solver != nullptr);
    assert(solver->getName() == "MockSolver");

    std::cout << "  ✓ Direct solver building works\n";

    // Build strategy directly
    auto strategy = SolverBuilder()
        .withStrategy(StrategyType::BASIC)
        .buildStrategy();

    assert(strategy != nullptr);

    std::cout << "  ✓ Direct strategy building works\n";
}

void testIntegratedWorkflow() {
    std::cout << "\nTesting integrated workflow...\n";

    // Create mesh
    auto meshManager = createTestMesh(21);

    // Build solver with configuration
    auto solver = SolverBuilder()
        .withName("MockSolver")
        .buildSolver();

    solver->initialize(meshManager);

    std::cout << "  ✓ Solver initialized\n";

    // Create strategy with hooks
    auto strategy = StrategyManager::createBasicStrategy();

    int stepCount = 0;
    strategy->addPreprocessHook([&stepCount](ISolver&) {
        stepCount++;
        return true;
    });

    strategy->addPostprocessHook([](ISolver&, const ConvergenceInfo& info) {
        std::cout << "    → Solve completed with status: "
                  << ConvergenceInfo::statusToString(info.status) << "\n";
        return true;
    });

    std::cout << "  ✓ Strategy configured with hooks\n";

    // Execute
    StrategyManager manager;
    auto info = manager.execute(solver, strategy);

    assert(stepCount > 0);

    std::cout << "  ✓ Workflow executed\n";
    std::cout << "  → Final status: " << ConvergenceInfo::statusToString(info.status) << "\n";
}

void testComplexConfiguration() {
    std::cout << "\nTesting complex configuration...\n";

    // Create a complex configuration
    auto config = SolverBuilder()
        .withBackend(SolverBackend::CUSTOM)
        .withStrategy(StrategyType::TIME_STEPPING)
        .withTimeRange(0.0, 1.0, 0.1)
        .build();

    std::cout << "  ✓ Complex configuration built\n";

    // Display summary
    std::cout << "\n" << config.getSummary();

    // Create solver and strategy
    auto solver = config.createSolver();
    auto strategy = config.createStrategy();

    assert(solver != nullptr);
    assert(strategy != nullptr);

    std::cout << "  ✓ Solver and strategy created from complex config\n";

    // Initialize and execute
    auto meshManager = createTestMesh();
    solver->initialize(meshManager);

    StrategyManager manager;
    auto info = manager.execute(solver, strategy);

    std::cout << "  ✓ Complex workflow executed\n";
    std::cout << "  → Result: " << info.message << "\n";
}

int main() {
    std::cout << "Phase 14 Tests - Solver Strategy and Configuration\n";
    std::cout << "===================================================\n\n";

    try {
        testBasicStrategy();
        testBasicStrategyWithHooks();
        testTimeSteppingStrategy();
        testAdaptiveStrategy();
        testStrategyManager();
        testSolverConfig();
        testSolverConfigCreation();
        testSolverConfigPresets();
        testSolverBuilder();
        testIntegratedWorkflow();
        testComplexConfiguration();

        std::cout << "\n===================================================\n";
        std::cout << "All Phase 14 tests passed!\n";
        std::cout << "Solver strategy and configuration system verified.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
