/**
 * @file test_phase54.cpp
 * @brief Unit tests for Phase 54: GPU Reaction Kinetics
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 */

#include "gpu/kinetics/ReactionKernels.h"
#include "gpu/kinetics/ODESolver.h"
#include "gpu/kinetics/ChemicalSystem.h"
#include "gpu/kinetics/ReactionDiffusion.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

using namespace koo::gpu;
using namespace koo::gpu::kinetics;

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
// Phase 54 Tests: Reaction Kernels
// ============================================

TEST(reaction_kernels_creation) {
    int n_species = 3;
    int n_reactions = 2;
    int n_cells = 10;

    ReactionKernelsD kernels(n_species, n_reactions, n_cells);

    assert(kernels.getSpeciesCount() == n_species);
    assert(kernels.getReactionCount() == n_reactions);
    assert(kernels.getCellCount() == n_cells);
}

TEST(reaction_kernels_arrhenius) {
    int n_reactions = 2;
    int n_cells = 3;

    ReactionKernelsD kernels(1, n_reactions, n_cells);

    // Setup Arrhenius parameters
    std::vector<double> A_host = {1.0e13, 1.0e14};     // Pre-exp factors
    std::vector<double> beta_host = {0.0, 0.5};        // Temperature exponents
    std::vector<double> Ea_host = {50000.0, 80000.0};  // Activation energies (J/mol)
    std::vector<double> T_host = {300.0, 400.0, 500.0}; // Temperatures

    DeviceMemory<double> A(n_reactions);
    DeviceMemory<double> beta(n_reactions);
    DeviceMemory<double> Ea(n_reactions);
    DeviceMemory<double> T_values(n_cells);

    A.copyFromHost(A_host.data(), n_reactions);
    beta.copyFromHost(beta_host.data(), n_reactions);
    Ea.copyFromHost(Ea_host.data(), n_reactions);
    T_values.copyFromHost(T_host.data(), n_cells);

    DeviceMemory<double> k_fwd(n_reactions * n_cells);

    // Calculate rate constants
    kernels.calculateArrheniusRates(A, beta, Ea, T_values, k_fwd);

    auto k_result = k_fwd.toHost();

    // Check that rate constants are positive
    for (size_t i = 0; i < k_result.size(); ++i) {
        assert(k_result[i] > 0.0);
    }

    // Check that rate increases with temperature
    double k_300 = k_result[0];  // Reaction 0 at 300K
    double k_500 = k_result[2];  // Reaction 0 at 500K
    assert(k_500 > k_300);
}

TEST(reaction_kernels_production_rates) {
    int n_species = 2;
    int n_reactions = 2;
    int n_cells = 1;

    ReactionKernelsD kernels(n_species, n_reactions, n_cells);

    // Setup stoichiometric matrix (net)
    // Reaction 0: A -> B (nu_A = -1, nu_B = +1)
    // Reaction 1: B -> A (nu_A = +1, nu_B = -1)
    std::vector<double> nu_host = {
        -1.0, 1.0,   // Species A
         1.0, -1.0   // Species B
    };

    DeviceMemory<double> nu_matrix(n_species * n_reactions);
    nu_matrix.copyFromHost(nu_host.data(), nu_host.size());

    // Rate of progress
    std::vector<double> ROP_host = {1.0, 0.5};  // Reaction rates
    DeviceMemory<double> ROP(n_reactions * n_cells);
    ROP.copyFromHost(ROP_host.data(), n_reactions);

    DeviceMemory<double> omega(n_species * n_cells);

    // Calculate production rates
    kernels.calculateProductionRates(ROP, nu_matrix, omega);

    auto omega_result = omega.toHost();

    // omega_A = -1*1.0 + 1*0.5 = -0.5
    // omega_B = +1*1.0 - 1*0.5 = +0.5
    assert(std::abs(omega_result[0] - (-0.5)) < 1e-10);
    assert(std::abs(omega_result[1] - 0.5) < 1e-10);
}

TEST(reaction_kernels_euler_step) {
    int n_species = 2;
    int n_reactions = 1;
    int n_cells = 1;

    ReactionKernelsD kernels(n_species, n_reactions, n_cells);

    std::vector<double> C_host = {1.0, 2.0};
    std::vector<double> omega_host = {0.1, -0.2};

    DeviceMemory<double> C_old(n_species);
    DeviceMemory<double> omega(n_species);
    DeviceMemory<double> C_new(n_species);

    C_old.copyFromHost(C_host.data(), n_species);
    omega.copyFromHost(omega_host.data(), n_species);

    double dt = 0.1;
    kernels.eulerStep(C_old, omega, C_new, dt);

    auto C_result = C_new.toHost();

    // C_new[0] = 1.0 + 0.1 * 0.1 = 1.01
    // C_new[1] = 2.0 + 0.1 * (-0.2) = 1.98
    assert(std::abs(C_result[0] - 1.01) < 1e-10);
    assert(std::abs(C_result[1] - 1.98) < 1e-10);
}

TEST(reaction_kernels_saxpy) {
    ReactionKernelsD kernels(1, 1, 1);

    std::vector<double> x_host = {1.0, 2.0, 3.0};
    std::vector<double> y_host = {4.0, 5.0, 6.0};

    DeviceMemory<double> x(3);
    DeviceMemory<double> y(3);

    x.copyFromHost(x_host.data(), 3);
    y.copyFromHost(y_host.data(), 3);

    double alpha = 2.0;
    kernels.saxpy(alpha, x, y);

    auto y_result = y.toHost();

    // y = alpha * x + y = 2*x + y
    assert(std::abs(y_result[0] - (2.0*1.0 + 4.0)) < 1e-10);
    assert(std::abs(y_result[1] - (2.0*2.0 + 5.0)) < 1e-10);
    assert(std::abs(y_result[2] - (2.0*3.0 + 6.0)) < 1e-10);
}

// ============================================
// Phase 54 Tests: ODE Solver
// ============================================

TEST(ode_solver_creation) {
    int n_species = 3;
    int n_cells = 5;

    ODESolverGPUD solver(n_species, n_cells, ODEMethod::RK4);

    assert(solver.getMethodName() == "RK4");
}

TEST(ode_solver_exponential_decay) {
    // Test: dC/dt = -k * C (exponential decay)
    // Exact solution: C(t) = C0 * exp(-k*t)

    int n_species = 1;
    ODESolverGPUD solver(n_species, 1, ODEMethod::RK4);
    solver.setCheckNegative(false);

    double k = 0.1;
    double C0 = 1.0;

    DeviceMemory<double> C(1);
    C.fill(C0);

    // Define RHS: omega = -k * C
    auto rhs_func = [k](const DeviceMemory<double>& C_in, DeviceMemory<double>& omega) {
        auto C_host = C_in.toHost();
        std::vector<double> omega_host(1);
        omega_host[0] = -k * C_host[0];
        omega.copyFromHost(omega_host.data(), 1);
    };

    // Integrate
    double t_final = 10.0;
    double dt = 0.1;
    solver.solveUntil(C, t_final, dt, rhs_func);

    auto C_final = C.toHost();
    double C_exact = C0 * std::exp(-k * t_final);

    // Check accuracy (RK4 should be very accurate)
    assert(std::abs(C_final[0] - C_exact) < 0.01);
}

TEST(ode_solver_linear_growth) {
    // Test: dC/dt = k (linear growth)
    // Exact solution: C(t) = C0 + k*t

    int n_species = 1;
    ODESolverGPUD solver(n_species, 1, ODEMethod::EXPLICIT_EULER);

    double k = 0.5;
    double C0 = 1.0;

    DeviceMemory<double> C(1);
    C.fill(C0);

    auto rhs_func = [k](const DeviceMemory<double>&, DeviceMemory<double>& omega) {
        std::vector<double> omega_host(1, k);
        omega.copyFromHost(omega_host.data(), 1);
    };

    double t_final = 2.0;
    double dt = 0.01;
    solver.solveUntil(C, t_final, dt, rhs_func);

    auto C_final = C.toHost();
    double C_exact = C0 + k * t_final;

    assert(std::abs(C_final[0] - C_exact) < 0.01);
}

TEST(ode_solver_batched) {
    // Test batched ODE solve (multiple cells)
    int n_species = 1;
    int n_cells = 3;

    ODESolverGPUD solver(n_species, n_cells, ODEMethod::RK4);

    // Different initial conditions for each cell
    std::vector<double> C_init = {1.0, 2.0, 3.0};
    DeviceMemory<double> C(n_cells);
    C.copyFromHost(C_init.data(), n_cells);

    double k = 0.1;
    auto rhs_func = [k, n_cells](const DeviceMemory<double>& C_in, DeviceMemory<double>& omega) {
        auto C_host = C_in.toHost();
        std::vector<double> omega_host(n_cells);
        for (int i = 0; i < n_cells; ++i) {
            omega_host[i] = -k * C_host[i];
        }
        omega.copyFromHost(omega_host.data(), n_cells);
    };

    double t_final = 5.0;
    double dt = 0.1;
    solver.solveUntil(C, t_final, dt, rhs_func);

    auto C_final = C.toHost();

    // Each cell should decay exponentially
    for (int i = 0; i < n_cells; ++i) {
        double C_exact = C_init[i] * std::exp(-k * t_final);
        assert(std::abs(C_final[i] - C_exact) < 0.01);
    }
}

// ============================================
// Phase 54 Tests: Chemical System
// ============================================

TEST(chemical_system_creation) {
    int n_species = 3;
    int n_reactions = 2;
    int n_cells = 5;

    GPUChemicalSystemD system(n_species, n_reactions, n_cells);

    assert(system.getSpeciesCount() == n_species);
    assert(system.getReactionCount() == n_reactions);
    assert(system.getCellCount() == n_cells);
}

TEST(chemical_system_set_get_concentration) {
    GPUChemicalSystemD system(2, 1, 1);

    std::vector<double> C = {1.5, 2.5};
    system.setConcentrations(0, C);

    auto C_retrieved = system.getConcentrations(0);

    assert(std::abs(C_retrieved[0] - 1.5) < 1e-10);
    assert(std::abs(C_retrieved[1] - 2.5) < 1e-10);
}

TEST(chemical_system_temperature) {
    GPUChemicalSystemD system(1, 1, 3);

    system.setTemperature(0, 300.0);
    system.setTemperature(1, 400.0);
    system.setTemperature(2, 500.0);

    // No direct getter, but can verify through rate calculations
    // Just checking it doesn't throw
}

// ============================================
// Phase 54 Tests: Reaction-Diffusion
// ============================================

TEST(reaction_diffusion_1d_creation) {
    int n_species = 2;
    int nx = 100;
    double dx = 0.01;

    ReactionDiffusion1DD solver(n_species, nx, dx);

    assert(solver.getSpeciesCount() == n_species);
    assert(solver.getGridSize() == nx);
    assert(std::abs(solver.getGridSpacing() - dx) < 1e-10);
}

TEST(reaction_diffusion_1d_diffusion_coefficients) {
    int n_species = 2;
    ReactionDiffusion1DD solver(n_species, 50, 0.01);

    std::vector<double> D = {1.0e-9, 2.0e-9};
    solver.setDiffusionCoefficients(D);

    // No direct getter, but check it doesn't throw
}

TEST(reaction_diffusion_1d_set_get_concentrations) {
    int n_species = 2;
    int nx = 50;
    ReactionDiffusion1DD solver(n_species, nx, 0.01);

    std::vector<double> C = {1.0, 2.0};
    solver.setConcentrations(25, C);

    auto C_retrieved = solver.getConcentrations(25);

    assert(std::abs(C_retrieved[0] - 1.0) < 1e-10);
    assert(std::abs(C_retrieved[1] - 2.0) < 1e-10);
}

TEST(reaction_diffusion_1d_pure_diffusion) {
    // Test pure diffusion (no reaction)
    int n_species = 1;
    int nx = 50;
    double dx = 0.01;

    ReactionDiffusion1DD solver(n_species, nx, dx);

    std::vector<double> D = {1.0e-9};
    solver.setDiffusionCoefficients(D);

    // Initial condition: Gaussian pulse
    for (int i = 0; i < nx; ++i) {
        double x = i * dx;
        double x0 = 0.25;
        double sigma = 0.05;
        double C = std::exp(-(x - x0) * (x - x0) / (2.0 * sigma * sigma));
        solver.setConcentrations(i, {C});
    }

    // Set zero reaction
    auto zero_reaction = [](const DeviceMemory<double>& C, DeviceMemory<double>& omega) {
        omega.zero();
    };
    solver.setReactionRates(zero_reaction);

    // Solve
    double t_final = 1.0;
    double dt = 0.01;
    solver.solve(t_final, dt);

    // Check that peak has diffused (should be lower)
    auto C_final = solver.getConcentrations(nx/4);
    assert(C_final[0] < 1.0);  // Peak should have decreased
}

TEST(reaction_diffusion_1d_pure_reaction) {
    // Test pure reaction (decay): dC/dt = -k*C
    int n_species = 1;
    int nx = 10;
    double dx = 0.1;

    ReactionDiffusion1DD solver(n_species, nx, dx);

    // Zero diffusion
    std::vector<double> D = {0.0};
    solver.setDiffusionCoefficients(D);

    // Initial condition: uniform
    for (int i = 0; i < nx; ++i) {
        solver.setConcentrations(i, {1.0});
    }

    // Decay reaction
    double k_decay = 0.1;
    auto decay_reaction = [k_decay, nx](const DeviceMemory<double>& C, DeviceMemory<double>& omega) {
        auto C_host = C.toHost();
        std::vector<double> omega_host(nx);
        for (int i = 0; i < nx; ++i) {
            omega_host[i] = -k_decay * C_host[i];
        }
        omega.copyFromHost(omega_host.data(), nx);
    };
    solver.setReactionRates(decay_reaction);

    // Solve
    double t_final = 5.0;
    double dt = 0.1;
    solver.solve(t_final, dt);

    // Check exponential decay
    auto C_final = solver.getConcentrations(5);
    double C_exact = std::exp(-k_decay * t_final);
    assert(std::abs(C_final[0] - C_exact) < 0.1);
}

TEST(reaction_diffusion_2d_creation) {
    int n_species = 2;
    int nx = 30, ny = 30;
    double dx = 0.05, dy = 0.05;

    ReactionDiffusion2DD solver(n_species, nx, ny, dx, dy);

    // Just check creation
}

// ============================================
// Main
// ============================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Phase 54: GPU Reaction Kinetics Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Reaction Kernels
    run_test_reaction_kernels_creation();
    run_test_reaction_kernels_arrhenius();
    run_test_reaction_kernels_production_rates();
    run_test_reaction_kernels_euler_step();
    run_test_reaction_kernels_saxpy();

    // ODE Solver
    run_test_ode_solver_creation();
    run_test_ode_solver_exponential_decay();
    run_test_ode_solver_linear_growth();
    run_test_ode_solver_batched();

    // Chemical System
    run_test_chemical_system_creation();
    run_test_chemical_system_set_get_concentration();
    run_test_chemical_system_temperature();

    // Reaction-Diffusion
    run_test_reaction_diffusion_1d_creation();
    run_test_reaction_diffusion_1d_diffusion_coefficients();
    run_test_reaction_diffusion_1d_set_get_concentrations();
    run_test_reaction_diffusion_1d_pure_diffusion();
    run_test_reaction_diffusion_1d_pure_reaction();
    run_test_reaction_diffusion_2d_creation();

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
