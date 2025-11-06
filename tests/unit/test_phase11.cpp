/**
 * @file test_phase11.cpp
 * @brief Unit tests for Phase 11 - PDE Solver Interfaces
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha1
 * @date 2025-11-06
 */

#include "solver/pde/SolverTypes.h"
#include "solver/pde/LinearSystem.h"
#include "solver/pde/SolverOptions.h"
#include "solver/pde/ISolver.h"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace koo::solver::pde;

// Helper function for floating point comparison
bool isClose(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) < tol;
}

void testSolverTypes() {
    std::cout << "Testing SolverTypes...\n";

    // Test enum to string conversions
    assert(toString(SolverBackend::NGSOLVE) == "NGSolve");
    assert(toString(SolverBackend::MFEM) == "MFEM");
    std::cout << "  ✓ SolverBackend conversions work\n";

    assert(toString(PDEType::ELLIPTIC) == "Elliptic");
    assert(toString(PDEType::PARABOLIC) == "Parabolic");
    std::cout << "  ✓ PDEType conversions work\n";

    assert(toString(LinearSolverType::CG) == "CG");
    assert(toString(LinearSolverType::GMRES) == "GMRES");
    std::cout << "  ✓ LinearSolverType conversions work\n";

    assert(toString(PreconditionerType::JACOBI) == "Jacobi");
    assert(toString(PreconditionerType::ILU) == "ILU");
    std::cout << "  ✓ PreconditionerType conversions work\n";

    assert(toString(FESpaceType::H1) == "H1");
    assert(toString(FESpaceType::L2) == "L2");
    std::cout << "  ✓ FESpaceType conversions work\n";
}

void testConvergenceInfo() {
    std::cout << "\nTesting ConvergenceInfo...\n";

    ConvergenceInfo info;
    assert(info.status == SolverStatus::NOT_INITIALIZED);
    assert(!info.converged());
    std::cout << "  ✓ Default state correct\n";

    info.status = SolverStatus::CONVERGED;
    info.iterations = 42;
    info.residual = 1e-8;
    info.relativeError = 1e-9;
    info.timeElapsed = 1.5;
    info.message = "Success";

    assert(info.converged());
    assert(info.iterations == 42);
    std::cout << "  ✓ State updates work\n";

    std::string str = info.toString();
    assert(!str.empty());
    assert(str.find("Converged") != std::string::npos);
    std::cout << "  ✓ toString works\n";

    assert(ConvergenceInfo::statusToString(SolverStatus::CONVERGED) == "Converged");
    assert(ConvergenceInfo::statusToString(SolverStatus::DIVERGED) == "Diverged");
    std::cout << "  ✓ Status conversions work\n";
}

void testSolutionVector() {
    std::cout << "\nTesting SolutionVector...\n";

    // Test default construction
    SolutionVector vec1;
    assert(vec1.size() == 0);
    std::cout << "  ✓ Default construction works\n";

    // Test sized construction
    SolutionVector vec2(10);
    assert(vec2.size() == 10);
    std::cout << "  ✓ Sized construction works\n";

    // Test element access
    vec2[0] = 1.0;
    vec2[1] = 2.0;
    assert(isClose(vec2[0], 1.0));
    assert(isClose(vec2[1], 2.0));
    std::cout << "  ✓ Element access works\n";

    // Test zero
    vec2.zero();
    assert(isClose(vec2[0], 0.0));
    assert(isClose(vec2[1], 0.0));
    std::cout << "  ✓ Zero works\n";

    // Test setConstant
    vec2.setConstant(3.14);
    assert(isClose(vec2[0], 3.14));
    assert(isClose(vec2[1], 3.14));
    std::cout << "  ✓ setConstant works\n";

    // Test norm
    SolutionVector vec3(3);
    vec3[0] = 3.0;
    vec3[1] = 4.0;
    vec3[2] = 0.0;
    double norm = vec3.norm();
    assert(isClose(norm, 5.0));  // sqrt(9 + 16) = 5
    std::cout << "  ✓ Norm calculation works\n";

    // Test resize
    vec3.resize(5);
    assert(vec3.size() == 5);
    std::cout << "  ✓ Resize works\n";
}

void testSparseMatrix() {
    std::cout << "\nTesting SparseMatrix...\n";

    // Create a simple 3x3 matrix
    SparseMatrix mat(3, 3);
    assert(mat.rows() == 3);
    assert(mat.cols() == 3);
    assert(mat.nnz() == 0);
    std::cout << "  ✓ Construction works\n";

    // Add entries
    mat.addEntry(0, 0, 2.0);
    mat.addEntry(0, 1, -1.0);
    mat.addEntry(1, 0, -1.0);
    mat.addEntry(1, 1, 2.0);
    mat.addEntry(1, 2, -1.0);
    mat.addEntry(2, 1, -1.0);
    mat.addEntry(2, 2, 2.0);
    assert(mat.nnz() == 7);
    std::cout << "  ✓ Entry addition works\n";

    // Finalize to CSR format
    mat.finalize();
    assert(mat.getFormat() == MatrixFormat::CSR);
    std::cout << "  ✓ Finalization to CSR works\n";

    // Test matrix-vector multiplication
    std::vector<double> x = {1.0, 2.0, 3.0};
    std::vector<double> y;
    mat.multiply(x, y);

    assert(y.size() == 3);
    assert(isClose(y[0], 0.0));   // 2*1 + (-1)*2 = 0
    assert(isClose(y[1], -1.0));  // (-1)*1 + 2*2 + (-1)*3 = -1
    assert(isClose(y[2], 1.0));   // (-1)*2 + 2*3 = 1
    std::cout << "  ✓ Matrix-vector multiplication works\n";

    // Test getValue
    assert(isClose(mat.getValue(0, 0), 2.0));
    assert(isClose(mat.getValue(0, 1), -1.0));
    assert(isClose(mat.getValue(1, 1), 2.0));
    std::cout << "  ✓ getValue works\n";
}

void testLinearSystem() {
    std::cout << "\nTesting LinearSystem...\n";

    // Create linear system
    LinearSystem system(3);
    assert(system.size() == 3);
    std::cout << "  ✓ Construction works\n";

    // Access components
    auto& A = system.getMatrix();
    auto& b = system.getRHS();
    auto& x = system.getSolution();

    assert(A.rows() == 3);
    assert(b.size() == 3);
    assert(x.size() == 3);
    std::cout << "  ✓ Component access works\n";

    // Build a simple system: Ax = b
    A.addEntry(0, 0, 2.0);
    A.addEntry(1, 1, 2.0);
    A.addEntry(2, 2, 2.0);
    A.finalize();

    b[0] = 2.0;
    b[1] = 4.0;
    b[2] = 6.0;

    x[0] = 1.0;
    x[1] = 2.0;
    x[2] = 3.0;

    std::cout << "  ✓ System setup works\n";

    // Test residual computation
    double residual = system.computeResidual();
    assert(isClose(residual, 0.0, 1e-10));  // Exact solution
    std::cout << "  ✓ Residual computation works\n";

    // Test well-posed check
    assert(system.isWellPosed());
    std::cout << "  ✓ Well-posed check works\n";

    // Test resize
    system.resize(5);
    assert(system.size() == 5);
    std::cout << "  ✓ Resize works\n";
}

void testSolverOptions() {
    std::cout << "\nTesting SolverOptions...\n";

    // Test default construction
    SolverOptions opts;
    assert(opts.getLinearSolver() == LinearSolverType::GMRES);
    assert(opts.getPreconditioner() == PreconditionerType::NONE);
    assert(opts.getMaxIterations() == 1000);
    assert(isClose(opts.getTolerance(), 1e-6));
    std::cout << "  ✓ Default construction works\n";

    // Test setters
    opts.setLinearSolver(LinearSolverType::CG);
    assert(opts.getLinearSolver() == LinearSolverType::CG);

    opts.setPreconditioner(PreconditionerType::JACOBI);
    assert(opts.getPreconditioner() == PreconditionerType::JACOBI);

    opts.setMaxIterations(500);
    assert(opts.getMaxIterations() == 500);

    opts.setTolerance(1e-8);
    assert(isClose(opts.getTolerance(), 1e-8));

    opts.setVerbose(true);
    assert(opts.isVerbose());

    std::cout << "  ✓ Setters work\n";

    // Test time integration options
    opts.setTimeIntegrationScheme(TimeIntegrationScheme::CRANK_NICOLSON);
    assert(opts.getTimeIntegrationScheme() == TimeIntegrationScheme::CRANK_NICOLSON);

    opts.setTimeStep(0.01);
    assert(isClose(opts.getTimeStep(), 0.01));

    opts.setNumTimeSteps(100);
    assert(opts.getNumTimeSteps() == 100);

    std::cout << "  ✓ Time integration options work\n";

    // Test FE space options
    opts.setFESpaceType(FESpaceType::H1);
    assert(opts.getFESpaceType() == FESpaceType::H1);

    opts.setPolynomialOrder(2);
    assert(opts.getPolynomialOrder() == 2);

    std::cout << "  ✓ FE space options work\n";

    // Test custom parameters
    opts.setParameter("diffusion_coeff", 0.5);
    assert(isClose(opts.getParameter("diffusion_coeff"), 0.5));
    assert(opts.hasParameter("diffusion_coeff"));
    assert(!opts.hasParameter("nonexistent"));

    std::cout << "  ✓ Custom parameters work\n";

    // Test validation
    std::string errorMsg;
    assert(opts.validate(errorMsg));
    std::cout << "  ✓ Validation works\n";

    // Test invalid options
    SolverOptions badOpts;
    badOpts.setMaxIterations(-1);
    assert(!badOpts.validate(errorMsg));
    assert(!errorMsg.empty());
    std::cout << "  ✓ Invalid option detection works\n";

    // Test toString
    std::string str = opts.toString();
    assert(!str.empty());
    std::cout << "  ✓ toString works\n";
}

void testSolverPresets() {
    std::cout << "\nTesting SolverOptions presets...\n";

    // Test default preset
    auto defaultOpts = presets::getDefault();
    assert(defaultOpts.getLinearSolver() == LinearSolverType::GMRES);
    std::cout << "  ✓ Default preset works\n";

    // Test fast preset
    auto fastOpts = presets::getFast();
    assert(fastOpts.getLinearSolver() == LinearSolverType::CG);
    assert(fastOpts.getMaxIterations() == 100);
    std::cout << "  ✓ Fast preset works\n";

    // Test accurate preset
    auto accurateOpts = presets::getAccurate();
    assert(accurateOpts.getLinearSolver() == LinearSolverType::GMRES);
    assert(accurateOpts.getMaxIterations() == 5000);
    assert(isClose(accurateOpts.getTolerance(), 1e-10));
    std::cout << "  ✓ Accurate preset works\n";

    // Test time-dependent preset
    auto timeOpts = presets::getTimeDependent(0.01, 1.0);
    assert(isClose(timeOpts.getTimeStep(), 0.01));
    assert(isClose(timeOpts.getFinalTime(), 1.0));
    assert(timeOpts.getNumTimeSteps() == 100);
    std::cout << "  ✓ Time-dependent preset works\n";
}

void testBaseSolver() {
    std::cout << "\nTesting BaseSolver...\n";

    // BaseSolver is abstract, but we can test through its interface
    // by creating a simple concrete implementation

    class TestSolver : public BaseSolver {
    public:
        SolverBackend getBackend() const override {
            return SolverBackend::CUSTOM;
        }

        std::string getName() const override {
            return "TestSolver";
        }

        bool initialize(std::shared_ptr<koo::mesh::MeshManager>) override {
            initialized_ = true;
            status_ = SolverStatus::INITIALIZED;
            return true;
        }

        bool assemble() override { return true; }
        bool applyBoundaryConditions() override { return true; }

        ConvergenceInfo solve() override {
            ConvergenceInfo info;
            info.status = SolverStatus::CONVERGED;
            info.iterations = 10;
            info.residual = 1e-8;
            lastConvergenceInfo_ = info;
            return info;
        }

        double getSolutionAt(double, double, double) const override {
            return 0.0;
        }

        bool exportSolution(const std::string&, const std::string&) const override {
            return true;
        }
    };

    TestSolver solver;
    assert(solver.getName() == "TestSolver");
    assert(solver.getBackend() == SolverBackend::CUSTOM);
    assert(!solver.isInitialized());
    std::cout << "  ✓ BaseSolver construction works\n";

    // Test initialization
    solver.initialize(nullptr);
    assert(solver.isInitialized());
    assert(solver.getStatus() == SolverStatus::INITIALIZED);
    std::cout << "  ✓ Initialization works\n";

    // Test options
    SolverOptions opts;
    opts.setMaxIterations(500);
    solver.setOptions(opts);
    assert(solver.getOptions().getMaxIterations() == 500);
    std::cout << "  ✓ Options management works\n";

    // Test PDE type
    solver.setPDEType(PDEType::PARABOLIC);
    assert(solver.getPDEType() == PDEType::PARABOLIC);
    std::cout << "  ✓ PDE type management works\n";

    // Test solve
    auto info = solver.solve();
    assert(info.converged());
    assert(info.iterations == 10);
    std::cout << "  ✓ Solve works\n";

    // Test reset
    solver.reset();
    assert(!solver.isInitialized());
    assert(solver.getStatus() == SolverStatus::NOT_INITIALIZED);
    std::cout << "  ✓ Reset works\n";
}

int main() {
    std::cout << "Phase 11 Tests - PDE Solver Interfaces\n";
    std::cout << "======================================\n\n";

    testSolverTypes();
    testConvergenceInfo();
    testSolutionVector();
    testSparseMatrix();
    testLinearSystem();
    testSolverOptions();
    testSolverPresets();
    testBaseSolver();

    std::cout << "\n======================================\n";
    std::cout << "All Phase 11 tests passed!\n";
    return 0;
}
