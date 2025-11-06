/**
 * @file test_phase53.cpp
 * @brief Unit tests for Phase 53: GPU Diffusion Solvers
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 */

#include "gpu/diffusion/DiffusionKernels.h"
#include "gpu/diffusion/ExplicitSolver.h"
#include "gpu/diffusion/ImplicitSolver.h"
#include "gpu/diffusion/HeatEquation.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

using namespace koo::gpu;
using namespace koo::gpu::diffusion;

// Test counter
int tests_passed = 0;
int tests_total = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        tests_total++; \
        std::cout << "Test " << tests_total << ": " << #name << " ... "; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << "PASSED" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what() << std::endl; \
        } \
    } \
    void test_##name()

// ============================================
// Phase 53 Tests: Diffusion Kernels
// ============================================

TEST(diffusion_kernels_1d_creation) {
    DiffusionKernels1DD kernels(100, 0.01);

    double cfl = kernels.getCFL(0.001, 1.0);
    assert(cfl > 0);
}

TEST(diffusion_kernels_1d_laplacian) {
    int nx = 10;
    double dx = 0.1;
    DiffusionKernels1DD kernels(nx, dx);

    // Create linear function: u(x) = x
    std::vector<double> u_host(nx);
    for (int i = 0; i < nx; ++i) {
        u_host[i] = i * dx;
    }

    DeviceMemory<double> u(nx);
    u.copyFromHost(u_host.data(), nx);

    DeviceMemory<double> lap(nx);
    kernels.laplacian(u, lap);

    auto lap_host = lap.toHost();

    // Laplacian of linear function should be ~0 in interior
    for (int i = 1; i < nx - 1; ++i) {
        assert(std::abs(lap_host[i]) < 1e-10);
    }
}

TEST(diffusion_kernels_1d_step) {
    int nx = 10;
    double dx = 0.1;
    double D = 1.0;
    double dt = 0.001;

    DiffusionKernels1DD kernels(nx, dx);

    // Initial condition: u = 1.0
    DeviceMemory<double> u(nx);
    u.fill(1.0);

    DeviceMemory<double> u_new(nx);
    kernels.step(u, u_new, dt, D);

    auto u_new_host = u_new.toHost();

    // Constant field should remain constant
    for (int i = 1; i < nx - 1; ++i) {
        assert(std::abs(u_new_host[i] - 1.0) < 1e-10);
    }
}

TEST(diffusion_kernels_1d_boundary_neumann) {
    int nx = 10;
    double dx = 0.1;
    DiffusionKernels1DD kernels(nx, dx);

    std::vector<double> u_host(nx, 1.0);
    u_host[0] = 0.5;
    u_host[nx-1] = 0.5;

    DeviceMemory<double> u(nx);
    u.copyFromHost(u_host.data(), nx);

    kernels.applyBC(u, BoundaryType::NEUMANN, BoundaryType::NEUMANN, 0, 0);

    auto u_result = u.toHost();

    // Neumann BC: u[0] = u[1], u[n-1] = u[n-2]
    assert(std::abs(u_result[0] - u_result[1]) < 1e-10);
    assert(std::abs(u_result[nx-1] - u_result[nx-2]) < 1e-10);
}

TEST(diffusion_kernels_1d_boundary_dirichlet) {
    int nx = 10;
    DiffusionKernels1DD kernels(nx, 0.1);

    DeviceMemory<double> u(nx);
    u.fill(1.0);

    double left_val = 2.0;
    double right_val = 3.0;

    kernels.applyBC(u, BoundaryType::DIRICHLET, BoundaryType::DIRICHLET,
                   left_val, right_val);

    auto u_result = u.toHost();

    assert(std::abs(u_result[0] - left_val) < 1e-10);
    assert(std::abs(u_result[nx-1] - right_val) < 1e-10);
}

TEST(diffusion_kernels_2d_step) {
    int nx = 10, ny = 10;
    double dx = 0.1, dy = 0.1;
    double D = 1.0;
    double dt = 0.0001;

    DiffusionKernels2DD kernels(nx, ny, dx, dy);

    // Constant field
    DeviceMemory<double> u(nx * ny);
    u.fill(1.0);

    DeviceMemory<double> u_new(nx * ny);
    kernels.step(u, u_new, dt, D);

    auto u_new_host = u_new.toHost();

    // Check interior points remain constant
    for (int j = 1; j < ny - 1; ++j) {
        for (int i = 1; i < nx - 1; ++i) {
            int idx = j * nx + i;
            assert(std::abs(u_new_host[idx] - 1.0) < 1e-10);
        }
    }
}

// ============================================
// Phase 53 Tests: Explicit Solver
// ============================================

TEST(explicit_solver_1d_stability) {
    int nx = 50;
    double L = 1.0;
    double dx = L / (nx - 1);
    double D = 1.0;

    ExplicitDiffusionSolver1DD solver(nx, dx, D, TimeScheme::FORWARD_EULER);

    // Check max stable dt
    double dt_max = solver.getMaxStableTimeStep();
    assert(dt_max > 0);

    // CFL check should pass
    solver.setCFLCheck(true);
    solver.setMaxCFL(0.5);

    DeviceMemory<double> u(nx);
    u.fill(1.0);

    // Should not throw
    solver.step(u, dt_max * 0.9);
}

TEST(explicit_solver_1d_gaussian_decay) {
    int nx = 100;
    double L = 1.0;
    double dx = L / (nx - 1);
    double D = 1e-3;

    ExplicitDiffusionSolver1DD solver(nx, dx, D, TimeScheme::FORWARD_EULER);
    solver.setBoundaryConditions(BoundaryType::NEUMANN, BoundaryType::NEUMANN);

    // Gaussian initial condition
    std::vector<double> u_init(nx);
    double x0 = L / 2.0;
    double sigma = 0.1;
    for (int i = 0; i < nx; ++i) {
        double x = i * dx;
        double dist = x - x0;
        u_init[i] = std::exp(-dist * dist / (2.0 * sigma * sigma));
    }

    DeviceMemory<double> u(nx);
    u.copyFromHost(u_init.data(), nx);

    // Solve for a few steps
    double dt = solver.getMaxStableTimeStep() * 0.4;
    auto stats = solver.solve(u, dt, 10);

    assert(stats.steps == 10);
    assert(stats.stable);

    // Solution should have diffused (peak should be lower)
    auto u_final = u.toHost();
    assert(u_final[nx/2] < u_init[nx/2]);
}

TEST(explicit_solver_1d_rk2) {
    int nx = 50;
    double dx = 0.01;
    double D = 1.0;

    ExplicitDiffusionSolver1DD solver(nx, dx, D, TimeScheme::RK2);

    DeviceMemory<double> u(nx);
    u.fill(1.0);

    double dt = solver.getMaxStableTimeStep() * 0.5;
    solver.step(u, dt);

    auto u_result = u.toHost();

    // Constant field should remain constant
    for (int i = 1; i < nx - 1; ++i) {
        assert(std::abs(u_result[i] - 1.0) < 1e-8);
    }
}

TEST(explicit_solver_2d_basic) {
    int nx = 20, ny = 20;
    double dx = 0.05, dy = 0.05;
    double D = 1.0;

    ExplicitDiffusionSolver2DD solver(nx, ny, dx, dy, D);

    double dt_max = solver.getMaxStableTimeStep();
    assert(dt_max > 0);

    DeviceMemory<double> u(nx * ny);
    u.fill(1.0);

    solver.step(u, dt_max * 0.4);

    auto u_result = u.toHost();

    // Check reasonable values
    for (size_t i = 0; i < u_result.size(); ++i) {
        assert(std::isfinite(u_result[i]));
    }
}

// ============================================
// Phase 53 Tests: Implicit Solver
// ============================================

TEST(implicit_solver_1d_basic) {
    int nx = 50;
    double dx = 0.01;
    double D = 1.0;

    ImplicitDiffusionSolver1DD solver(nx, dx, D, false);  // Backward Euler
    solver.setBoundaryConditions(BoundaryType::NEUMANN, BoundaryType::NEUMANN);

    DeviceMemory<double> u(nx);
    u.fill(1.0);

    // Implicit is unconditionally stable - can use large dt
    double dt = 0.1;  // Much larger than explicit would allow
    solver.step(u, dt);

    auto u_result = u.toHost();

    // Should remain stable
    for (int i = 0; i < nx; ++i) {
        assert(std::isfinite(u_result[i]));
        assert(u_result[i] >= 0);
    }
}

TEST(implicit_solver_1d_crank_nicolson) {
    int nx = 50;
    double dx = 0.01;
    double D = 1.0;

    ImplicitDiffusionSolver1DD solver(nx, dx, D, true);  // Crank-Nicolson
    solver.setBoundaryConditions(BoundaryType::NEUMANN, BoundaryType::NEUMANN);

    DeviceMemory<double> u(nx);
    u.fill(1.0);

    double dt = 0.1;
    auto stats = solver.solve(u, dt, 5);

    assert(stats.steps == 5);
    assert(stats.stable);
}

TEST(implicit_solver_1d_dirichlet_bc) {
    int nx = 50;
    double dx = 0.01;
    double D = 1.0;

    ImplicitDiffusionSolver1DD solver(nx, dx, D, false);
    solver.setBoundaryConditions(BoundaryType::DIRICHLET, BoundaryType::DIRICHLET,
                                 0.0, 0.0);

    // Linear initial condition
    std::vector<double> u_init(nx);
    for (int i = 0; i < nx; ++i) {
        u_init[i] = 1.0 - std::abs(2.0 * i / (nx - 1.0) - 1.0);
    }

    DeviceMemory<double> u(nx);
    u.copyFromHost(u_init.data(), nx);

    double dt = 0.01;
    solver.solve(u, dt, 10);

    auto u_final = u.toHost();

    // Boundaries should be fixed at 0
    assert(std::abs(u_final[0]) < 1e-5);
    assert(std::abs(u_final[nx-1]) < 1e-5);
}

TEST(implicit_solver_2d_basic) {
    int nx = 20, ny = 20;
    double dx = 0.05, dy = 0.05;
    double D = 1.0;

    ImplicitDiffusionSolver2DD solver(nx, ny, dx, dy, D, false);

    DeviceMemory<double> u(nx * ny);
    u.fill(1.0);

    double dt = 0.1;
    solver.step(u, dt);

    auto u_result = u.toHost();

    for (size_t i = 0; i < u_result.size(); ++i) {
        assert(std::isfinite(u_result[i]));
    }
}

// ============================================
// Phase 53 Tests: Heat Equation
// ============================================

TEST(heat_equation_1d_creation) {
    int nx = 100;
    double L = 1.0;
    double alpha = 1e-5;

    HeatEquation1DD solver(nx, L, alpha);

    assert(solver.getGridSize() == nx);
    assert(std::abs(solver.getDomainLength() - L) < 1e-10);
    assert(std::abs(solver.getThermalDiffusivity() - alpha) < 1e-10);
}

TEST(heat_equation_1d_initial_condition_function) {
    int nx = 100;
    double L = 1.0;
    double alpha = 1e-5;

    HeatEquation1DD solver(nx, L, alpha);

    // Set Gaussian initial condition
    auto gaussian = [&](double x) {
        double x0 = L / 2.0;
        double sigma = 0.1;
        double dist = x - x0;
        return std::exp(-dist * dist / (2.0 * sigma * sigma));
    };

    solver.setInitialCondition(gaussian);

    auto T = solver.getSolution();
    auto grid = solver.getGrid();

    // Check values match
    for (size_t i = 0; i < T.size(); ++i) {
        double expected = gaussian(grid[i]);
        assert(std::abs(T[i] - expected) < 1e-10);
    }
}

TEST(heat_equation_1d_initial_condition_array) {
    int nx = 100;
    HeatEquation1DD solver(nx, 1.0, 1e-5);

    std::vector<double> T_init(nx, 1.0);
    T_init[50] = 2.0;  // Hot spot

    solver.setInitialCondition(T_init);

    auto T = solver.getSolution();

    assert(std::abs(T[50] - 2.0) < 1e-10);
}

TEST(heat_equation_1d_solve_explicit) {
    int nx = 100;
    double L = 1.0;
    double alpha = 1e-3;

    HeatEquation1DD solver(nx, L, alpha, SolutionMethod::EXPLICIT_RK4);
    solver.setBoundaryConditions(BoundaryType::NEUMANN, BoundaryType::NEUMANN);

    // Gaussian pulse
    auto gaussian = [](double x) {
        return std::exp(-50.0 * (x - 0.5) * (x - 0.5));
    };
    solver.setInitialCondition(gaussian);

    double T_initial_peak = solver.getSolution()[nx/2];

    // Solve
    double t_final = 0.1;
    auto stats = solver.solve(t_final);

    assert(stats.stable);
    assert(stats.steps > 0);

    // Peak should decrease (diffusion)
    double T_final_peak = solver.getSolution()[nx/2];
    assert(T_final_peak < T_initial_peak);
}

TEST(heat_equation_1d_solve_implicit) {
    int nx = 100;
    double L = 1.0;
    double alpha = 1e-3;

    HeatEquation1DD solver(nx, L, alpha, SolutionMethod::CRANK_NICOLSON);
    solver.setBoundaryConditions(BoundaryType::NEUMANN, BoundaryType::NEUMANN);

    auto gaussian = [](double x) {
        return std::exp(-50.0 * (x - 0.5) * (x - 0.5));
    };
    solver.setInitialCondition(gaussian);

    // Solve with large time steps (implicit is stable)
    double t_final = 1.0;
    double dt = 0.1;  // Large time step
    auto stats = solver.solve(t_final, dt);

    assert(stats.stable);
}

TEST(heat_equation_1d_auto_method) {
    int nx = 100;
    double L = 1.0;
    double alpha = 1e-3;

    HeatEquation1DD solver(nx, L, alpha, SolutionMethod::AUTO);

    auto constant = [](double) { return 1.0; };
    solver.setInitialCondition(constant);

    // Auto-select method and time step
    double t_final = 0.1;
    auto stats = solver.solve(t_final);

    assert(stats.stable);
    assert(stats.steps > 0);
}

TEST(heat_equation_2d_creation) {
    int nx = 50, ny = 50;
    double Lx = 1.0, Ly = 1.0;
    double alpha = 1e-5;

    HeatEquation2DD solver(nx, ny, Lx, Ly, alpha);

    auto gridSize = solver.getGridSize();
    assert(gridSize.first == nx);
    assert(gridSize.second == ny);
}

TEST(heat_equation_2d_solve) {
    int nx = 30, ny = 30;
    double Lx = 1.0, Ly = 1.0;
    double alpha = 1e-3;

    HeatEquation2DD solver(nx, ny, Lx, Ly, alpha, SolutionMethod::EXPLICIT_EULER);

    // 2D Gaussian
    auto gaussian2d = [](double x, double y) {
        double x0 = 0.5, y0 = 0.5;
        double sigma = 0.1;
        double dx = x - x0, dy = y - y0;
        return std::exp(-(dx*dx + dy*dy) / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(gaussian2d);

    double t_final = 0.01;
    auto stats = solver.solve(t_final);

    assert(stats.stable);
    assert(stats.steps > 0);
}

// ============================================
// Main
// ============================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Phase 53: GPU Diffusion Solvers Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Diffusion Kernels
    run_test_diffusion_kernels_1d_creation();
    run_test_diffusion_kernels_1d_laplacian();
    run_test_diffusion_kernels_1d_step();
    run_test_diffusion_kernels_1d_boundary_neumann();
    run_test_diffusion_kernels_1d_boundary_dirichlet();
    run_test_diffusion_kernels_2d_step();

    // Explicit Solver
    run_test_explicit_solver_1d_stability();
    run_test_explicit_solver_1d_gaussian_decay();
    run_test_explicit_solver_1d_rk2();
    run_test_explicit_solver_2d_basic();

    // Implicit Solver
    run_test_implicit_solver_1d_basic();
    run_test_implicit_solver_1d_crank_nicolson();
    run_test_implicit_solver_1d_dirichlet_bc();
    run_test_implicit_solver_2d_basic();

    // Heat Equation
    run_test_heat_equation_1d_creation();
    run_test_heat_equation_1d_initial_condition_function();
    run_test_heat_equation_1d_initial_condition_array();
    run_test_heat_equation_1d_solve_explicit();
    run_test_heat_equation_1d_solve_implicit();
    run_test_heat_equation_1d_auto_method();
    run_test_heat_equation_2d_creation();
    run_test_heat_equation_2d_solve();

    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << " / " << tests_total << std::endl;
    std::cout << "========================================" << std::endl;

    if (tests_passed == tests_total) {
        std::cout << "✓ All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some tests FAILED" << std::endl;
        return 1;
    }
}
