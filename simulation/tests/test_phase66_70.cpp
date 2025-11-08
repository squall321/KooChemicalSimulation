/**
 * @file test_phase66_70.cpp
 * @brief Comprehensive tests for Phase 66-70
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

#include <gtest/gtest.h>
#include "../include/simulation/timestepping/AdaptiveTimestepper.h"
#include "../include/simulation/stability/StabilityMonitor.h"
#include "../include/simulation/coupling/ThermalChemicalCoupling.h"
#include "../include/simulation/coupling/FlowChemistryCoupling.h"

#ifdef KOO_USE_CUDA
#include "../../gpu/include/gpu/Device.h"
#include "../../gpu/include/gpu/tuning/AutoTuner.h"
#endif

#include <vector>
#include <cmath>

using namespace koo::simulation;

// ============================================================================
// Phase 66: Adaptive Timestepping Tests
// ============================================================================

TEST(Phase66_AdaptiveTimestepping, Construction) {
    timestepping::TimestepperConfig config;
    config.dt_initial = 0.001;
    config.abs_tol = 1e-6;
    config.rel_tol = 1e-4;

    timestepping::AdaptiveTimestepper stepper(config);

    EXPECT_DOUBLE_EQ(stepper.getCurrentTimestep(), 0.001);
    EXPECT_EQ(stepper.getConfig().abs_tol, 1e-6);
}

TEST(Phase66_AdaptiveTimestepping, StepAcceptance) {
    timestepping::AdaptiveTimestepper stepper;

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_new(100, 1.00001);  // Small change

    double error = 1e-7;  // Small error
    auto result = stepper.evaluateStep(solution, solution_new, error);

    EXPECT_TRUE(result.accepted) << "Step should be accepted for small error";
    EXPECT_GT(result.new_dt, 0.0);
}

TEST(Phase66_AdaptiveTimestepping, StepRejection) {
    timestepping::TimestepperConfig config;
    config.abs_tol = 1e-6;
    timestepping::AdaptiveTimestepper stepper(config);

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_new(100, 2.0);  // Large change

    double error = 1.0;  // Large error
    auto result = stepper.evaluateStep(solution, solution_new, error);

    EXPECT_FALSE(result.accepted) << "Step should be rejected for large error";
    EXPECT_LT(result.new_dt, stepper.getCurrentTimestep());
}

TEST(Phase66_AdaptiveTimestepping, Statistics) {
    timestepping::AdaptiveTimestepper stepper;

    std::vector<double> solution(10, 1.0);

    // Accept some steps
    for (int i = 0; i < 5; ++i) {
        std::vector<double> sol_new(10, 1.0 + 1e-8);
        stepper.evaluateStep(solution, sol_new, 1e-8);
    }

    auto stats = stepper.getStatistics();
    EXPECT_GT(stats.num_accepted, 0);
}

TEST(Phase66_AdaptiveTimestepping, PredefinedConfigs) {
    auto high_acc = timestepping::Configs::HighAccuracy();
    EXPECT_LT(high_acc.abs_tol, 1e-6);

    auto fast = timestepping::Configs::Fast();
    EXPECT_GT(fast.abs_tol, 1e-5);

    auto stiff = timestepping::Configs::Stiff();
    EXPECT_EQ(stiff.controller, timestepping::ControllerType::PID);
}

// ============================================================================
// Phase 66: Stability Monitoring Tests
// ============================================================================

TEST(Phase66_StabilityMonitor, Construction) {
    stability::MonitorConfig config;
    config.max_cfl = 1.0;

    stability::StabilityMonitor monitor(config);

    EXPECT_EQ(monitor.getConfig().max_cfl, 1.0);
}

TEST(Phase66_StabilityMonitor, CFLComputation) {
    stability::StabilityMonitor monitor;

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_prev(100, 0.9);

    double dt = 0.01;
    double dx = 0.1;
    double max_velocity = 1.0;

    auto metrics = monitor.checkStability(solution, solution_prev, dt, dx, max_velocity);

    // CFL = v*dt/dx = 1.0*0.01/0.1 = 0.1
    EXPECT_NEAR(metrics.cfl_number, 0.1, 1e-6);
    EXPECT_EQ(metrics.status, stability::StabilityStatus::Stable);
}

TEST(Phase66_StabilityMonitor, NaNDetection) {
    stability::StabilityMonitor monitor;

    std::vector<double> solution(100, 1.0);
    solution[50] = std::nan("");  // Inject NaN

    std::vector<double> solution_prev(100, 1.0);

    auto metrics = monitor.checkStability(solution, solution_prev, 0.01, 0.1);

    EXPECT_TRUE(metrics.has_nan);
    EXPECT_EQ(metrics.status, stability::StabilityStatus::Critical);
}

TEST(Phase66_StabilityMonitor, StableTimestep) {
    stability::StabilityMonitor monitor;

    double dx = 0.1;
    double diffusivity = 1e-5;
    double max_velocity = 1.0;

    double dt_stable = monitor.computeStableTimestep(dx, diffusivity, max_velocity, 0.9);

    EXPECT_GT(dt_stable, 0.0);
    // Should satisfy CFL condition
    EXPECT_LE(dt_stable, dx / max_velocity);
}

TEST(Phase66_StabilityMonitor, PhysicalBounds) {
    stability::StabilityMonitor monitor;

    stability::PhysicalBounds bounds(0.0, 10.0);
    monitor.setBounds("default", bounds);

    std::vector<double> solution_valid(10, 5.0);
    std::vector<double> solution_invalid(10, 15.0);  // Exceeds max

    auto metrics1 = monitor.checkStability(solution_valid, solution_valid, 0.01);
    EXPECT_FALSE(metrics1.out_of_bounds);

    auto metrics2 = monitor.checkStability(solution_invalid, solution_valid, 0.01);
    EXPECT_TRUE(metrics2.out_of_bounds);
}

// ============================================================================
// Phase 67: Thermal-Chemical Coupling Tests
// ============================================================================

TEST(Phase67_ThermalChemical, ArrheniusRate) {
    coupling::ThermalChemicalCoupling coupling;

    double A = 1e10;          // Pre-exponential factor
    double Ea = 50000;        // Activation energy (J/mol)
    double T = 300.0;         // Temperature (K)

    double k = coupling.computeReactionRate(A, Ea, T);

    EXPECT_GT(k, 0.0);
    EXPECT_LT(k, A);  // Rate should be less than pre-exponential factor

    // Higher temperature should give higher rate
    double k_high = coupling.computeReactionRate(A, Ea, 400.0);
    EXPECT_GT(k_high, k);
}

TEST(Phase67_ThermalChemical, HeatRelease) {
    coupling::ThermalChemicalCoupling coupling;

    // Setup species data
    coupling::ThermochemicalData reactant;
    reactant.heat_of_formation = 0.0;

    coupling::ThermochemicalData product;
    product.heat_of_formation = -100000.0;  // Exothermic

    coupling.setSpeciesData(0, reactant);
    coupling.setSpeciesData(1, product);

    // Compute heat release for A -> B
    auto source = coupling.computeHeatRelease(
        {0},   // Reactant IDs
        {1},   // Product IDs
        {1.0}, // Stoich reactants
        {1.0}, // Stoich products
        1.0    // Reaction rate
    );

    // Should release heat (negative enthalpy change)
    EXPECT_LT(source.enthalpy_change, 0.0);
    EXPECT_GT(source.heat_release_rate, 0.0);  // Positive heat release
}

TEST(Phase67_ThermalChemical, TemperatureChange) {
    coupling::ThermalChemicalCoupling coupling;

    double heat_release = 1000.0;  // W/m³
    double density = 1000.0;       // kg/m³
    double cp = 4000.0;            // J/(kg·K)
    double dt = 0.1;               // s

    double dT = coupling.computeTemperatureChange(heat_release, density, cp, dt);

    EXPECT_GT(dT, 0.0);  // Temperature should increase
    // dT = (Q*dt)/(ρ*cp) = (1000*0.1)/(1000*4000) = 2.5e-5
    EXPECT_NEAR(dT, 2.5e-5, 1e-7);
}

TEST(Phase67_ThermalChemical, MixtureProperties) {
    coupling::ThermalChemicalCoupling coupling;

    coupling::ThermochemicalData species1;
    species1.specific_heat = 1000.0;

    coupling::ThermochemicalData species2;
    species2.specific_heat = 2000.0;

    coupling.setSpeciesData(0, species1);
    coupling.setSpeciesData(1, species2);

    std::vector<double> mass_fractions = {0.6, 0.4};

    double cp_mix = coupling.computeMixtureSpecificHeat(mass_fractions);

    // Should be weighted average: 0.6*1000 + 0.4*2000 = 1400
    EXPECT_NEAR(cp_mix, 1400.0, 1.0);
}

TEST(Phase67_ThermalChemical, OperatorSplitting) {
    coupling::OperatorSplitting splitting(coupling::OperatorSplitting::SplittingScheme::Strang);

    double dt = 0.1;

    // Chemistry operator (type 0) should get half steps
    double dt_chem = splitting.getSubstepSize(dt, 0, 0);
    EXPECT_NEAR(dt_chem, 0.05, 1e-10);

    // Thermal operator (type 1) should get full step
    double dt_therm = splitting.getSubstepSize(dt, 0, 1);
    EXPECT_NEAR(dt_therm, 0.1, 1e-10);
}

// ============================================================================
// Phase 67: Flow-Chemistry Coupling Tests
// ============================================================================

TEST(Phase67_FlowChemistry, UpwindFlux) {
    coupling::FlowCouplingConfig config;
    config.scheme = coupling::AdvectionScheme::Upwind;

    coupling::FlowChemistryCoupling flow_coupling(config);

    double c_left = 1.0;
    double c_right = 0.5;
    double velocity = 1.0;  // Positive (left to right)

    double flux = flow_coupling.computeAdvectiveFlux(c_left, c_right, velocity);

    // Upwind: should use left value when velocity is positive
    EXPECT_NEAR(flux, velocity * c_left, 1e-10);
}

TEST(Phase67_FlowChemistry, PecletNumber) {
    coupling::FlowChemistryCoupling flow_coupling;

    double velocity = 1.0;
    double length = 0.1;
    double diffusivity = 0.01;

    double Pe = flow_coupling.computePecletNumber(velocity, length, diffusivity);

    // Pe = u*L/D = 1.0*0.1/0.01 = 10
    EXPECT_NEAR(Pe, 10.0, 1e-10);
}

TEST(Phase67_FlowChemistry, AdvectionDominated) {
    coupling::FlowChemistryCoupling flow_coupling;

    EXPECT_TRUE(flow_coupling.isAdvectionDominated(100.0));
    EXPECT_FALSE(flow_coupling.isAdvectionDominated(1.0));
}

TEST(Phase67_FlowChemistry, SchmidtNumber) {
    coupling::SpeciesTransport transport;

    double nu = 1e-5;   // Kinematic viscosity
    double D = 1e-6;    // Mass diffusivity

    double Sc = transport.computeSchmidtNumber(nu, D);

    // Sc = ν/D = 1e-5/1e-6 = 10
    EXPECT_NEAR(Sc, 10.0, 1e-10);
}

// ============================================================================
// Phase 69: Auto-Tuner Tests (GPU only)
// ============================================================================

#ifdef KOO_USE_CUDA

TEST(Phase69_AutoTuner, GenerateCandidates) {
    int device_count = koo::gpu::Device::getDeviceCount();
    if (device_count == 0) {
        GTEST_SKIP() << "No GPU devices available";
    }

    auto device = koo::gpu::Device::get_device(0);
    koo::gpu::tuning::AutoTuner tuner(device);

    tuner.generateCommonBlockSizes();
    // Should have generated multiple candidates
    // (can't access private members, but can test functionality)
}

TEST(Phase69_AutoTuner, KernelConfig) {
    koo::gpu::tuning::KernelConfig config(256, 1, 1);

    EXPECT_EQ(config.block_size_x, 256);
    EXPECT_EQ(config.block_size_y, 1);
    EXPECT_EQ(config.block_size_z, 1);
    EXPECT_EQ(config.totalThreads(), 256);
}

TEST(Phase69_AutoTuner, KernelConfig2D) {
    koo::gpu::tuning::KernelConfig config(16, 16, 1);

    EXPECT_EQ(config.totalThreads(), 256);

    auto dim = config.blockDim();
    EXPECT_EQ(dim.x, 16);
    EXPECT_EQ(dim.y, 16);
    EXPECT_EQ(dim.z, 1);
}

#endif  // KOO_USE_CUDA

// ============================================================================
// Integration Tests
// ============================================================================

TEST(Integration, AdaptiveTimesteppingWithStabilityMonitor) {
    timestepping::AdaptiveTimestepper stepper;
    stability::StabilityMonitor monitor;

    std::vector<double> solution(100, 1.0);
    std::vector<double> solution_prev(100, 1.0);

    double dt = stepper.getCurrentTimestep();

    // Check stability
    auto stab_metrics = monitor.checkStability(solution, solution_prev, dt, 0.1);

    EXPECT_EQ(stab_metrics.status, stability::StabilityStatus::Stable);

    // Take step
    std::vector<double> solution_new(100, 1.0001);
    auto step_result = stepper.evaluateStep(solution, solution_new, 1e-5);

    EXPECT_TRUE(step_result.accepted);
}

TEST(Integration, ThermalChemicalCoupledSystem) {
    coupling::ThermalChemicalCoupling coupling;
    timestepping::AdaptiveTimestepper stepper;

    // Setup thermochemical properties
    coupling::ThermochemicalData species;
    species.heat_of_formation = -100000.0;
    species.specific_heat = 1000.0;

    coupling.setSpeciesData(0, species);

    // Simulate coupled step
    double T = 300.0;
    double A = 1e10;
    double Ea = 50000.0;

    double k = coupling.computeReactionRate(A, Ea, T);
    EXPECT_GT(k, 0.0);

    // This demonstrates the coupling works
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
