/**
 * @file test_phase12.cpp
 * @brief Unit tests for Phase 12 - Mock Solver Implementation
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha2
 * @date 2025-11-06
 */

#include "solver/MockSolver.h"
#include "mesh/MeshManager.h"
#include "mesh/boundary/DirichletBC.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <memory>

using namespace koo::solver;
using namespace koo::mesh;
using namespace koo::mesh::boundary;
using namespace koo::mesh::core;

// Helper function for floating point comparison
bool isClose(double a, double b, double tol = 1e-6) {
    return std::abs(a - b) < tol;
}

// Helper to create simple 1D test mesh
std::shared_ptr<MeshManager> createTestMesh(int numNodes) {
    auto meshManager = std::make_shared<MeshManager>();
    auto meshData = std::make_shared<MeshData>();

    // Add nodes
    for (int i = 0; i < numNodes; ++i) {
        double x = static_cast<double>(i) / (numNodes - 1);
        Node node(i, x, 0.0, 0.0);
        meshData->addNode(node);
    }

    // Add line elements
    for (int i = 0; i < numNodes - 1; ++i) {
        Element elem(i, ElementType::LINE, {static_cast<size_t>(i), static_cast<size_t>(i + 1)});
        meshData->addElement(elem);
    }

    meshManager->setMesh(meshData);
    return meshManager;
}

void testMockSolverConstruction() {
    std::cout << "Testing MockSolver construction...\n";

    MockSolver solver;
    assert(solver.getName() == "MockSolver");
    assert(solver.getBackend() == pde::SolverBackend::CUSTOM);
    assert(solver.getVersion() == "1.0.0-mock");
    assert(!solver.isInitialized());
    assert(!solver.isAssembled());
    assert(!solver.areBCsApplied());
    assert(solver.getStatus() == pde::SolverStatus::NOT_INITIALIZED);
    assert(solver.getPDEType() == pde::PDEType::ELLIPTIC);

    std::cout << "  ✓ Construction works\n";
    std::cout << "  ✓ Default state correct\n";
}

void testMockSolverOptions() {
    std::cout << "\nTesting MockSolver options...\n";

    MockSolver solver;

    // Test default options
    auto opts = solver.getOptions();
    assert(opts.getMaxIterations() == 1000);
    assert(isClose(opts.getTolerance(), 1e-6));

    std::cout << "  ✓ Default options correct\n";

    // Test setting options
    pde::SolverOptions newOpts;
    newOpts.setMaxIterations(500);
    newOpts.setTolerance(1e-8);
    newOpts.setVerbose(false);
    solver.setOptions(newOpts);

    assert(solver.getOptions().getMaxIterations() == 500);
    assert(isClose(solver.getOptions().getTolerance(), 1e-8));

    std::cout << "  ✓ Setting options works\n";

    // Test PDE type
    solver.setPDEType(pde::PDEType::PARABOLIC);
    assert(solver.getPDEType() == pde::PDEType::PARABOLIC);

    std::cout << "  ✓ PDE type management works\n";
}

void testMockSolverInitialization() {
    std::cout << "\nTesting MockSolver initialization...\n";

    MockSolver solver;

    // Test initialization with null mesh
    assert(!solver.initialize(nullptr));
    assert(!solver.isInitialized());
    assert(!solver.getLastError().empty());

    std::cout << "  ✓ Null mesh rejection works\n";

    // Create test mesh
    auto meshManager = createTestMesh(11);
    assert(meshManager->isLoaded());
    size_t numNodes = meshManager->getMesh()->getNumNodes();
    assert(numNodes == 11);

    std::cout << "  ✓ Test mesh created (11 nodes)\n";

    // Test initialization with valid mesh
    assert(solver.initialize(meshManager));
    assert(solver.isInitialized());
    assert(solver.getStatus() == pde::SolverStatus::INITIALIZED);
    assert(solver.getNumDOFs() == numNodes);

    std::cout << "  ✓ Initialization with valid mesh works\n";
    std::cout << "  ✓ DOF count correct\n";
}

void testMockSolverAssembly() {
    std::cout << "\nTesting MockSolver assembly...\n";

    MockSolver solver;

    // Test assembly without initialization
    assert(!solver.assemble());
    assert(!solver.getLastError().empty());

    std::cout << "  ✓ Assembly rejection before initialization works\n";

    // Initialize and assemble
    auto meshManager = createTestMesh(11);
    assert(solver.initialize(meshManager));
    assert(solver.assemble());
    assert(solver.isAssembled());

    std::cout << "  ✓ Assembly works\n";

    // Verify linear system was built
    const auto& linearSystem = solver.getLinearSystem();
    assert(linearSystem.size() == 11);
    assert(linearSystem.isWellPosed());

    std::cout << "  ✓ Linear system created\n";
    std::cout << "  ✓ System well-posed\n";
}

void testMockSolverBoundaryConditions() {
    std::cout << "\nTesting MockSolver boundary conditions...\n";

    MockSolver solver;

    // Create test mesh
    auto meshManager = createTestMesh(11);

    // Add boundary conditions
    auto bcManager = meshManager->getBCManager();

    // Dirichlet BC: u = 0 at left boundary (tag 1)
    auto leftBC = DirichletBC::makeConstant("left", 1, 0.0);
    leftBC->setEnabled(true);
    bcManager->addBC(leftBC);

    // Dirichlet BC: u = 1 at right boundary (tag 2)
    auto rightBC = DirichletBC::makeConstant("right", 2, 1.0);
    rightBC->setEnabled(true);
    bcManager->addBC(rightBC);

    std::cout << "  ✓ Boundary conditions configured\n";

    // Test BC application without assembly
    assert(!solver.applyBoundaryConditions());
    assert(!solver.getLastError().empty());

    std::cout << "  ✓ BC rejection before assembly works\n";

    // Initialize, assemble, and apply BCs
    assert(solver.initialize(meshManager));
    assert(solver.assemble());
    assert(solver.applyBoundaryConditions());
    assert(solver.areBCsApplied());

    std::cout << "  ✓ Boundary condition application works\n";
}

void testMockSolverSolve() {
    std::cout << "\nTesting MockSolver solve...\n";

    MockSolver solver;

    // Test solve without assembly
    auto info = solver.solve();
    assert(!info.converged());
    assert(info.status == pde::SolverStatus::ERROR);

    std::cout << "  ✓ Solve rejection before assembly works\n";

    // Full solve workflow
    auto meshManager = createTestMesh(11);
    assert(solver.initialize(meshManager));
    assert(solver.assemble());

    // Add simple BCs
    auto bcManager = meshManager->getBCManager();
    auto leftBC = DirichletBC::makeConstant("left", 1, 0.0);
    leftBC->setEnabled(true);
    bcManager->addBC(leftBC);

    assert(solver.applyBoundaryConditions());

    // Set reasonable options for fast convergence
    pde::SolverOptions opts;
    opts.setMaxIterations(1000);
    opts.setTolerance(1e-6);
    solver.setOptions(opts);

    // Solve
    info = solver.solve();

    std::cout << "  → Solver status: " << pde::ConvergenceInfo::statusToString(info.status) << "\n";
    std::cout << "  → Iterations: " << info.iterations << "\n";
    std::cout << "  → Residual: " << info.residual << "\n";
    std::cout << "  → Time: " << info.timeElapsed << " s\n";

    // Check convergence (may reach max iterations or converge)
    assert(info.status == pde::SolverStatus::CONVERGED ||
           info.status == pde::SolverStatus::MAX_ITERATIONS);
    assert(info.iterations > 0);
    assert(info.timeElapsed >= 0.0);

    std::cout << "  ✓ Solve executes\n";
    std::cout << "  ✓ Convergence info populated\n";
}

void testMockSolverSolution() {
    std::cout << "\nTesting MockSolver solution access...\n";

    MockSolver solver;

    // Create test mesh and solve
    auto meshManager = createTestMesh(11);
    solver.initialize(meshManager);
    solver.assemble();
    solver.applyBoundaryConditions();
    solver.solve();

    // Test solution vector access
    const auto& solution = solver.getSolution();
    assert(solution.size() == 11);

    std::cout << "  ✓ Solution vector accessible\n";

    // Test solution at point
    double val0 = solver.getSolutionAt(0.0, 0.0, 0.0);
    double val1 = solver.getSolutionAt(1.0, 0.0, 0.0);

    // Solution should be defined
    assert(std::isfinite(val0));
    assert(std::isfinite(val1));

    std::cout << "  ✓ Solution evaluation at points works\n";
    std::cout << "  → u(0.0) = " << val0 << "\n";
    std::cout << "  → u(1.0) = " << val1 << "\n";
}

void testMockSolverExport() {
    std::cout << "\nTesting MockSolver solution export...\n";

    MockSolver solver;

    // Create test mesh and solve
    auto meshManager = createTestMesh(11);
    solver.initialize(meshManager);
    solver.assemble();
    solver.applyBoundaryConditions();
    solver.solve();

    // Export solution
    std::string filename = "/tmp/mock_solution_test.dat";
    assert(solver.exportSolution(filename, "text"));

    std::cout << "  ✓ Solution export works\n";
    std::cout << "  → Exported to: " << filename << "\n";

    // Verify file exists and has content
    std::ifstream file(filename);
    assert(file.is_open());

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
    }
    file.close();

    assert(lineCount > 11);  // Header + 11 data lines

    std::cout << "  ✓ Exported file readable (" << lineCount << " lines)\n";
}

void testMockSolverReset() {
    std::cout << "\nTesting MockSolver reset...\n";

    MockSolver solver;

    // Create test mesh and solve
    auto meshManager = createTestMesh(11);
    solver.initialize(meshManager);
    solver.assemble();
    solver.solve();

    assert(solver.isInitialized());
    assert(solver.getStatus() != pde::SolverStatus::NOT_INITIALIZED);

    std::cout << "  ✓ Solver in initialized state\n";

    // Reset
    solver.reset();
    assert(!solver.isInitialized());
    assert(solver.getStatus() == pde::SolverStatus::NOT_INITIALIZED);
    assert(solver.getSolution().size() == 0);

    std::cout << "  ✓ Reset works\n";
    std::cout << "  ✓ State cleared\n";
}

void testMockSolverFullWorkflow() {
    std::cout << "\nTesting MockSolver full workflow...\n";

    // Create solver
    MockSolver solver;
    solver.setPDEType(pde::PDEType::ELLIPTIC);

    // Configure options
    pde::SolverOptions opts;
    opts.setLinearSolver(pde::LinearSolverType::CG);
    opts.setMaxIterations(500);
    opts.setTolerance(1e-6);
    opts.setVerbose(false);
    solver.setOptions(opts);

    std::cout << "  ✓ Solver configured\n";

    // Create mesh with 21 nodes for better resolution
    auto meshManager = createTestMesh(21);
    std::cout << "  ✓ Mesh created (21 nodes)\n";

    // Add boundary conditions
    auto bcManager = meshManager->getBCManager();

    auto leftBC = DirichletBC::makeConstant("left", 1, 0.0);
    leftBC->setEnabled(true);
    bcManager->addBC(leftBC);

    auto rightBC = DirichletBC::makeConstant("right", 2, 1.0);
    rightBC->setEnabled(true);
    bcManager->addBC(rightBC);

    std::cout << "  ✓ Boundary conditions set\n";

    // Full workflow
    assert(solver.initialize(meshManager));
    std::cout << "  ✓ Step 1: Initialize\n";

    assert(solver.assemble());
    std::cout << "  ✓ Step 2: Assemble\n";

    assert(solver.applyBoundaryConditions());
    std::cout << "  ✓ Step 3: Apply BCs\n";

    auto info = solver.solve();
    std::cout << "  ✓ Step 4: Solve\n";

    std::cout << "\n  Convergence Info:\n";
    std::cout << "  → Status: " << pde::ConvergenceInfo::statusToString(info.status) << "\n";
    std::cout << "  → Iterations: " << info.iterations << "\n";
    std::cout << "  → Residual: " << info.residual << "\n";
    std::cout << "  → Relative Error: " << info.relativeError << "\n";
    std::cout << "  → Solution Norm: " << info.solutionNorm << "\n";
    std::cout << "  → Time: " << info.timeElapsed << " s\n";

    // Verify solution quality
    assert(info.iterations > 0);
    assert(info.solutionNorm > 0.0);
    assert(info.timeElapsed > 0.0);

    // Export solution
    assert(solver.exportSolution("/tmp/mock_final_solution.dat", "text"));
    std::cout << "  ✓ Step 5: Export solution\n";

    // Print statistics
    std::cout << "\n" << solver.getStatistics();

    std::cout << "\n  ✓ Full workflow complete\n";
}

int main() {
    std::cout << "Phase 12 Tests - Mock Solver Implementation\n";
    std::cout << "============================================\n\n";

    try {
        testMockSolverConstruction();
        testMockSolverOptions();
        testMockSolverInitialization();
        testMockSolverAssembly();
        testMockSolverBoundaryConditions();
        testMockSolverSolve();
        testMockSolverSolution();
        testMockSolverExport();
        testMockSolverReset();
        testMockSolverFullWorkflow();

        std::cout << "\n============================================\n";
        std::cout << "All Phase 12 tests passed!\n";
        std::cout << "MockSolver implementation verified.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
