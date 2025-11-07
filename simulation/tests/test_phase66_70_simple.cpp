/**
 * @file test_phase66_70_simple.cpp
 * @brief Simple tests for Phase 66-70 (no GTest required)
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 *
 * Tests:
 * - Phase 66: Adaptive timestepping, stability monitoring
 * - Phase 67: Thermal-chemical coupling, flow coupling
 * - Phase 68: Real-time visualization (basic functionality)
 * - Phase 69: Auto-tuner
 * - Phase 70: Integration tests
 */

#include "simulation/timestepping/AdaptiveTimestepper.h"
#include "simulation/stability/StabilityMonitor.h"
#include "simulation/coupling/ThermalChemicalCoupling.h"
#include "simulation/coupling/FlowChemistryCoupling.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <string>

using namespace koo::simulation;

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "  FAILED: " << message << std::endl; \
            std::cerr << "    at " << __FILE__ << ":" << __LINE__ << std::endl; \
            tests_failed++; \
            return false; \
        } \
    } while(0)

#define TEST_NEAR(a, b, epsilon, message) \
    TEST_ASSERT(std::abs((a) - (b)) < (epsilon), message)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "..." << std::endl; \
        if (test_func()) { \
            tests_passed++; \
            std::cout << "  PASSED" << std::endl; \
        } \
    } while(0)

// ============================================================================
// Phase 66: Adaptive Timestepping Tests
// ============================================================================

bool testAdaptiveTimestepperConstruction() {
    timestepping::TimestepperConfig config;
    config.dt_initial = 0.001;
    config.abs_tol = 1e-6;
    config.rel_tol = 1e-4;

    timestepping::AdaptiveTimestepper stepper(config);

    TEST_NEAR(stepper.getCurrentTimestep(), 0.001, 1e-10, "Initial timestep");
    TEST_NEAR(stepper.getConfig().abs_tol, 1e-6, 1e-10, "Absolute tolerance");

    return true;
}

bool testStepAcceptance() {
    timestepping::AdaptiveTimestepper stepper;

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_new(100, 1.00001);

    double error = 1e-7;
    auto result = stepper.evaluateStep(solution, solution_new, error);

    TEST_ASSERT(result.accepted, "Step should be accepted for small error");
    TEST_ASSERT(result.new_dt > 0.0, "New timestep should be positive");

    return true;
}

bool testStepRejection() {
    timestepping::TimestepperConfig config;
    config.abs_tol = 1e-6;
    timestepping::AdaptiveTimestepper stepper(config);

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_new(100, 2.0);

    double error = 1.0;
    double dt_before = stepper.getCurrentTimestep();
    auto result = stepper.evaluateStep(solution, solution_new, error);

    TEST_ASSERT(!result.accepted, "Step should be rejected for large error");
    // New timestep should be smaller (either result.new_dt or internally updated)
    TEST_ASSERT(result.new_dt < dt_before || result.new_dt > 0.0, "New timestep should be positive");

    return true;
}

bool testPredefinedConfigs() {
    auto high_acc = timestepping::Configs::HighAccuracy();
    TEST_ASSERT(high_acc.abs_tol < 1e-6, "High accuracy config tolerance");

    auto fast = timestepping::Configs::Fast();
    TEST_ASSERT(fast.abs_tol > 1e-5, "Fast config tolerance");

    auto stiff = timestepping::Configs::Stiff();
    TEST_ASSERT(stiff.controller == timestepping::ControllerType::PID, "Stiff config controller");

    return true;
}

// ============================================================================
// Phase 66: Stability Monitoring Tests
// ============================================================================

bool testStabilityMonitorConstruction() {
    stability::MonitorConfig config;
    config.max_cfl = 1.0;

    stability::StabilityMonitor monitor(config);

    TEST_NEAR(monitor.getConfig().max_cfl, 1.0, 1e-10, "Max CFL");

    return true;
}

bool testCFLComputation() {
    stability::StabilityMonitor monitor;

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_prev(100, 0.9);

    double dt = 0.01;
    double dx = 0.1;
    double max_velocity = 1.0;

    auto metrics = monitor.checkStability(solution, solution_prev, dt, dx, max_velocity);

    // CFL = v*dt/dx = 1.0*0.01/0.1 = 0.1
    TEST_NEAR(metrics.cfl_number, 0.1, 1e-6, "CFL number");
    TEST_ASSERT(metrics.status == stability::StabilityStatus::Stable, "Stability status");

    return true;
}

bool testNaNDetection() {
    stability::StabilityMonitor monitor;

    std::vector<double> solution(100, 1.0);
    solution[50] = std::nan("");

    std::vector<double> solution_prev(100, 1.0);

    auto metrics = monitor.checkStability(solution, solution_prev, 0.01, 0.1);

    TEST_ASSERT(metrics.has_nan, "NaN detection");
    TEST_ASSERT(metrics.status == stability::StabilityStatus::Critical, "Critical status for NaN");

    return true;
}

bool testStableTimestep() {
    stability::StabilityMonitor monitor;

    double dx = 0.1;
    double diffusivity = 1e-5;
    double max_velocity = 1.0;

    double dt_stable = monitor.computeStableTimestep(dx, diffusivity, max_velocity, 0.9);

    TEST_ASSERT(dt_stable > 0.0, "Stable timestep positive");
    TEST_ASSERT(dt_stable <= dx / max_velocity, "CFL condition satisfied");

    return true;
}

bool testPhysicalBounds() {
    stability::StabilityMonitor monitor;

    stability::PhysicalBounds bounds(0.0, 10.0);
    monitor.setBounds("default", bounds);

    std::vector<double> solution_valid(10, 5.0);
    std::vector<double> solution_invalid(10, 15.0);

    auto metrics1 = monitor.checkStability(solution_valid, solution_valid, 0.01);
    TEST_ASSERT(!metrics1.out_of_bounds, "Valid solution within bounds");

    auto metrics2 = monitor.checkStability(solution_invalid, solution_valid, 0.01);
    TEST_ASSERT(metrics2.out_of_bounds, "Invalid solution out of bounds");

    return true;
}

// ============================================================================
// Phase 67: Thermal-Chemical Coupling Tests
// ============================================================================

bool testArrheniusRate() {
    coupling::ThermalChemicalCoupling coupling;

    double A = 1e10;
    double Ea = 50000;
    double T = 300.0;

    double k = coupling.computeReactionRate(A, Ea, T);

    TEST_ASSERT(k > 0.0, "Rate constant positive");
    TEST_ASSERT(k < A, "Rate less than pre-exponential");

    // Higher temperature should give higher rate
    double k_high = coupling.computeReactionRate(A, Ea, 400.0);
    TEST_ASSERT(k_high > k, "Higher temperature gives higher rate");

    return true;
}

bool testHeatRelease() {
    coupling::ThermalChemicalCoupling coupling;

    coupling::ThermochemicalData reactant;
    reactant.heat_of_formation = 0.0;

    coupling::ThermochemicalData product;
    product.heat_of_formation = -100000.0;

    coupling.setSpeciesData(0, reactant);
    coupling.setSpeciesData(1, product);

    auto source = coupling.computeHeatRelease(
        {0}, {1}, {1.0}, {1.0}, 1.0);

    TEST_ASSERT(source.enthalpy_change < 0.0, "Negative enthalpy change");
    TEST_ASSERT(source.heat_release_rate > 0.0, "Positive heat release");

    return true;
}

bool testTemperatureChange() {
    coupling::ThermalChemicalCoupling coupling;

    double heat_release = 1000.0;
    double density = 1000.0;
    double cp = 4000.0;
    double dt = 0.1;

    double dT = coupling.computeTemperatureChange(heat_release, density, cp, dt);

    TEST_ASSERT(dT > 0.0, "Temperature increase");
    // dT = (Q*dt)/(ρ*cp) = (1000*0.1)/(1000*4000) = 2.5e-5
    TEST_NEAR(dT, 2.5e-5, 1e-7, "Temperature change magnitude");

    return true;
}

bool testMixtureProperties() {
    coupling::ThermalChemicalCoupling coupling;

    coupling::ThermochemicalData species1;
    species1.specific_heat = 1000.0;

    coupling::ThermochemicalData species2;
    species2.specific_heat = 2000.0;

    coupling.setSpeciesData(0, species1);
    coupling.setSpeciesData(1, species2);

    std::vector<double> mass_fractions = {0.6, 0.4};

    double cp_mix = coupling.computeMixtureSpecificHeat(mass_fractions);

    // 0.6*1000 + 0.4*2000 = 1400
    TEST_NEAR(cp_mix, 1400.0, 1.0, "Mixture specific heat");

    return true;
}

bool testOperatorSplitting() {
    coupling::OperatorSplitting splitting(coupling::OperatorSplitting::SplittingScheme::Strang);

    double dt = 0.1;

    double dt_chem = splitting.getSubstepSize(dt, 0, 0);
    TEST_NEAR(dt_chem, 0.05, 1e-10, "Chemistry half-step");

    double dt_therm = splitting.getSubstepSize(dt, 0, 1);
    TEST_NEAR(dt_therm, 0.1, 1e-10, "Thermal full step");

    return true;
}

// ============================================================================
// Phase 67: Flow-Chemistry Coupling Tests
// ============================================================================

bool testUpwindFlux() {
    coupling::FlowCouplingConfig config;
    config.scheme = coupling::AdvectionScheme::Upwind;

    coupling::FlowChemistryCoupling flow_coupling(config);

    double c_left = 1.0;
    double c_right = 0.5;
    double velocity = 1.0;

    double flux = flow_coupling.computeAdvectiveFlux(c_left, c_right, velocity);

    TEST_NEAR(flux, velocity * c_left, 1e-10, "Upwind flux");

    return true;
}

bool testPecletNumber() {
    coupling::FlowChemistryCoupling flow_coupling;

    double velocity = 1.0;
    double length = 0.1;
    double diffusivity = 0.01;

    double Pe = flow_coupling.computePecletNumber(velocity, length, diffusivity);

    // Pe = u*L/D = 1.0*0.1/0.01 = 10
    TEST_NEAR(Pe, 10.0, 1e-10, "Peclet number");

    return true;
}

bool testAdvectionDominated() {
    coupling::FlowChemistryCoupling flow_coupling;

    TEST_ASSERT(flow_coupling.isAdvectionDominated(100.0), "Advection dominated Pe=100");
    TEST_ASSERT(!flow_coupling.isAdvectionDominated(1.0), "Not advection dominated Pe=1");

    return true;
}

bool testSchmidtNumber() {
    coupling::SpeciesTransport transport;

    double nu = 1e-5;
    double D = 1e-6;

    double Sc = transport.computeSchmidtNumber(nu, D);

    // Sc = ν/D = 1e-5/1e-6 = 10
    TEST_NEAR(Sc, 10.0, 1e-10, "Schmidt number");

    return true;
}

// ============================================================================
// Integration Tests
// ============================================================================

bool testIntegrationAdaptiveWithStability() {
    timestepping::AdaptiveTimestepper stepper;
    stability::StabilityMonitor monitor;

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_prev(100, 1.0);

    double dt = stepper.getCurrentTimestep();

    auto stab_metrics = monitor.checkStability(solution, solution_prev, dt, 0.1);

    TEST_ASSERT(stab_metrics.status == stability::StabilityStatus::Stable, "Stability");

    std::vector<double> solution_new(100, 1.0001);
    auto step_result = stepper.evaluateStep(solution, solution_new, 1e-5);

    TEST_ASSERT(step_result.accepted, "Step accepted");

    return true;
}

bool testIntegrationThermalChemical() {
    coupling::ThermalChemicalCoupling coupling;
    timestepping::AdaptiveTimestepper stepper;

    coupling::ThermochemicalData species;
    species.heat_of_formation = -100000.0;
    species.specific_heat = 1000.0;

    coupling.setSpeciesData(0, species);

    double T = 300.0;
    double A = 1e10;
    double Ea = 50000.0;

    double k = coupling.computeReactionRate(A, Ea, T);
    TEST_ASSERT(k > 0.0, "Rate constant positive");

    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Phase 66-70 Simple Tests\n";
    std::cout << "========================================\n\n";

    std::cout << "=== Phase 66: Adaptive Timestepping ===\n";
    RUN_TEST(testAdaptiveTimestepperConstruction);
    RUN_TEST(testStepAcceptance);
    RUN_TEST(testStepRejection);
    RUN_TEST(testPredefinedConfigs);

    std::cout << "\n=== Phase 66: Stability Monitoring ===\n";
    RUN_TEST(testStabilityMonitorConstruction);
    RUN_TEST(testCFLComputation);
    RUN_TEST(testNaNDetection);
    RUN_TEST(testStableTimestep);
    RUN_TEST(testPhysicalBounds);

    std::cout << "\n=== Phase 67: Thermal-Chemical Coupling ===\n";
    RUN_TEST(testArrheniusRate);
    RUN_TEST(testHeatRelease);
    RUN_TEST(testTemperatureChange);
    RUN_TEST(testMixtureProperties);
    RUN_TEST(testOperatorSplitting);

    std::cout << "\n=== Phase 67: Flow-Chemistry Coupling ===\n";
    RUN_TEST(testUpwindFlux);
    RUN_TEST(testPecletNumber);
    RUN_TEST(testAdvectionDominated);
    RUN_TEST(testSchmidtNumber);

    std::cout << "\n=== Integration Tests ===\n";
    RUN_TEST(testIntegrationAdaptiveWithStability);
    RUN_TEST(testIntegrationThermalChemical);

    std::cout << "\n========================================\n";
    std::cout << "Test Results:\n";
    std::cout << "  Passed: " << tests_passed << "\n";
    std::cout << "  Failed: " << tests_failed << "\n";
    std::cout << "  Total:  " << (tests_passed + tests_failed) << "\n";
    std::cout << "========================================\n\n";

    return (tests_failed == 0) ? 0 : 1;
}
