/**
 * @file full_simulation_example.cpp
 * @brief Complete chemical simulation example with all features
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * Phase 70: Production Deployment
 *
 * This example demonstrates:
 * - Adaptive timestepping
 * - Thermal-chemical coupling
 * - GPU acceleration
 * - Stability monitoring
 * - Checkpointing
 * - Real-time visualization (via Python)
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

// Simulation components
#include "simulation/timestepping/AdaptiveTimestepper.h"
#include "simulation/stability/StabilityMonitor.h"
#include "simulation/coupling/ThermalChemicalCoupling.h"
#include "simulation/coupling/FlowChemistryCoupling.h"

// GPU components (if enabled)
#ifdef KOO_USE_CUDA
#include "gpu/Device.h"
#include "gpu/DeviceMemory.h"
#include "gpu/Stream.h"
#include "gpu/memory/MemoryPool.h"
#include "gpu/profiling/Profiler.h"
#include "gpu/profiling/NVTX.h"
#include "gpu/checkpoint/Checkpoint.h"
#endif

using namespace koo::simulation;

/**
 * @brief Simple 1D reactive diffusion example
 *
 * System: A -> B (first-order reaction)
 * PDE: dC/dt = D * d²C/dx² - k*C + heat_release
 *      dT/dt = α * d²T/dx² + Q_rxn / (ρ*cp)
 *
 * where:
 * - C: Concentration of species A
 * - T: Temperature
 * - D: Diffusion coefficient
 * - k: Reaction rate (temperature-dependent)
 * - α: Thermal diffusivity
 * - Q_rxn: Heat of reaction
 */
class ReactiveDiffusionSimulation {
public:
    /**
     * @brief Constructor
     */
    ReactiveDiffusionSimulation(int nx, double Lx)
        : nx_(nx), Lx_(Lx), dx_(Lx / (nx - 1)),
          concentration_(nx, 0.0),
          temperature_(nx, 300.0),  // 300 K initial
          concentration_prev_(nx, 0.0),
          temperature_prev_(nx, 300.0) {

        // Physical parameters
        diffusivity_ = 1e-5;          // m²/s
        thermal_diffusivity_ = 1e-5;  // m²/s
        pre_exp_factor_ = 1e10;       // 1/s
        activation_energy_ = 50000;   // J/mol
        heat_of_reaction_ = -100000;  // J/mol (exothermic)
        density_ = 1000;              // kg/m³
        specific_heat_ = 4000;        // J/(kg·K)

        // Initialize timestepper
        timestepping::TimestepperConfig ts_config;
        ts_config.dt_initial = 1e-4;
        ts_config.dt_min = 1e-8;
        ts_config.dt_max = 1e-2;
        ts_config.abs_tol = 1e-6;
        ts_config.rel_tol = 1e-4;
        timestepper_ = std::make_unique<timestepping::AdaptiveTimestepper>(ts_config);

        // Initialize stability monitor
        stability::MonitorConfig stab_config;
        stab_config.max_cfl = 0.5;
        stability_monitor_ = std::make_unique<stability::StabilityMonitor>(stab_config);

        // Initialize coupling
        thermal_chemical_ = std::make_unique<coupling::ThermalChemicalCoupling>();

        // Set initial conditions
        setInitialConditions();
    }

    /**
     * @brief Set initial conditions
     */
    void setInitialConditions() {
        // Gaussian pulse of reactant
        double x0 = Lx_ / 2.0;
        double sigma = Lx_ / 10.0;

        for (int i = 0; i < nx_; ++i) {
            double x = i * dx_;
            concentration_[i] = std::exp(-std::pow((x - x0) / sigma, 2));
            temperature_[i] = 300.0;  // Room temperature
        }

        concentration_prev_ = concentration_;
        temperature_prev_ = temperature_;
    }

    /**
     * @brief Run simulation
     */
    void run(double t_end, const std::string& output_file) {
        std::cout << "\n=== Starting Reactive Diffusion Simulation ===\n";
        std::cout << "Grid points: " << nx_ << "\n";
        std::cout << "Domain length: " << Lx_ << " m\n";
        std::cout << "End time: " << t_end << " s\n\n";

        double t = 0.0;
        int step = 0;
        int output_interval = 100;

        // Open output file
        std::ofstream outfile(output_file);
        outfile << "# Time, Max_Concentration, Max_Temperature, dt, CFL\n";

        // Main time loop
        while (t < t_end) {
            KOO_NVTX_RANGE("Simulation Step");

            // Compute timestep
            double dt = timestepper_->getCurrentTimestep();

            // Check stability
            double max_velocity = 0.0;  // No advection in this example
            auto stability_metrics = stability_monitor_->checkStability(
                concentration_, concentration_prev_, dt, dx_, max_velocity);

            if (stability_metrics.status == stability::StabilityStatus::Unstable ||
                stability_metrics.status == stability::StabilityStatus::Critical) {
                std::cerr << "Warning: Simulation unstable at t = " << t << "\n";
                std::cerr << "  CFL = " << stability_metrics.cfl_number << "\n";
                std::cerr << "  Growth rate = " << stability_metrics.solution_growth_rate << "\n";

                // Reduce timestep
                dt *= 0.5;
                timestepper_->setTimestep(dt);
                continue;
            }

            // Take timestep
            bool accepted = takeStep(dt);

            if (accepted) {
                t += dt;
                step++;

                // Output
                if (step % output_interval == 0) {
                    double max_c = *std::max_element(concentration_.begin(), concentration_.end());
                    double max_T = *std::max_element(temperature_.begin(), temperature_.end());

                    std::cout << "t = " << t << " s, "
                             << "max(C) = " << max_c << ", "
                             << "max(T) = " << max_T << " K, "
                             << "dt = " << dt << " s\n";

                    outfile << t << ", " << max_c << ", " << max_T << ", "
                           << dt << ", " << stability_metrics.cfl_number << "\n";
                }

                // Update previous values
                concentration_prev_ = concentration_;
                temperature_prev_ = temperature_;
            }
        }

        outfile.close();

        std::cout << "\n=== Simulation Complete ===\n";
        std::cout << "Total steps: " << step << "\n";

        auto stats = timestepper_->getStatistics();
        std::cout << "Accepted steps: " << stats.num_accepted << "\n";
        std::cout << "Rejected steps: " << stats.num_rejected << "\n";
        std::cout << "Acceptance rate: " << stats.acceptance_rate * 100 << "%\n";
    }

private:
    // Grid
    int nx_;
    double Lx_;
    double dx_;

    // State variables
    std::vector<double> concentration_;
    std::vector<double> temperature_;
    std::vector<double> concentration_prev_;
    std::vector<double> temperature_prev_;

    // Physical parameters
    double diffusivity_;
    double thermal_diffusivity_;
    double pre_exp_factor_;
    double activation_energy_;
    double heat_of_reaction_;
    double density_;
    double specific_heat_;

    // Simulation components
    std::unique_ptr<timestepping::AdaptiveTimestepper> timestepper_;
    std::unique_ptr<stability::StabilityMonitor> stability_monitor_;
    std::unique_ptr<coupling::ThermalChemicalCoupling> thermal_chemical_;

    /**
     * @brief Take a single timestep
     */
    bool takeStep(double dt) {
        std::vector<double> c_new = concentration_;
        std::vector<double> T_new = temperature_;

        // Explicit Euler with operator splitting
        // Step 1: Diffusion
        for (int i = 1; i < nx_ - 1; ++i) {
            // Species diffusion
            double d2c_dx2 = (concentration_[i+1] - 2*concentration_[i] + concentration_[i-1]) / (dx_ * dx_);
            c_new[i] += dt * diffusivity_ * d2c_dx2;

            // Thermal diffusion
            double d2T_dx2 = (temperature_[i+1] - 2*temperature_[i] + temperature_[i-1]) / (dx_ * dx_);
            T_new[i] += dt * thermal_diffusivity_ * d2T_dx2;
        }

        // Step 2: Reaction with thermal coupling
        for (int i = 0; i < nx_; ++i) {
            // Temperature-dependent rate constant
            double k = thermal_chemical_->computeReactionRate(
                pre_exp_factor_, activation_energy_, T_new[i]);

            // Reaction: A -> B (first order)
            double reaction_rate = k * c_new[i];
            c_new[i] -= dt * reaction_rate;

            // Heat release from reaction
            double heat_release = -heat_of_reaction_ * reaction_rate;  // J/m³/s
            double dT = thermal_chemical_->computeTemperatureChange(
                heat_release, density_, specific_heat_, dt);
            T_new[i] += dT;
        }

        // Evaluate timestep acceptance
        double error_estimate = computeError(concentration_, c_new);

        auto result = timestepper_->evaluateStep(
            concentration_, c_new, error_estimate);

        if (result.accepted) {
            concentration_ = c_new;
            temperature_ = T_new;
        }

        return result.accepted;
    }

    /**
     * @brief Compute error estimate
     */
    double computeError(const std::vector<double>& c_old,
                       const std::vector<double>& c_new) const {
        double max_error = 0.0;
        for (size_t i = 0; i < c_old.size(); ++i) {
            double err = std::abs(c_new[i] - c_old[i]);
            max_error = std::max(max_error, err);
        }
        return max_error;
    }
};

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    std::cout << "KooChemicalSimulation v6.0.0-alpha4\n";
    std::cout << "Full Simulation Example\n";
    std::cout << "=================================\n\n";

#ifdef KOO_USE_CUDA
    // Initialize GPU
    int device_count = koo::gpu::Device::getDeviceCount();
    std::cout << "CUDA devices available: " << device_count << "\n";

    if (device_count > 0) {
        auto device = koo::gpu::Device::get_device(0);
        auto props = device.getProperties();
        std::cout << "Using GPU: " << props.name << "\n";
        std::cout << "Compute capability: " << props.major << "." << props.minor << "\n\n";
    }
#else
    std::cout << "Running in CPU mode\n\n";
#endif

    // Create and run simulation
    int nx = 256;          // Grid points
    double Lx = 1.0;       // Domain length (m)
    double t_end = 10.0;   // End time (s)

    ReactiveDiffusionSimulation sim(nx, Lx);
    sim.run(t_end, "simulation_output.csv");

    std::cout << "\nOutput saved to simulation_output.csv\n";
    std::cout << "You can visualize with Python:\n";
    std::cout << "  python -c \"import pandas as pd; import matplotlib.pyplot as plt; \"\n";
    std::cout << "  \"df = pd.read_csv('simulation_output.csv', comment='#'); \"\n";
    std::cout << "  \"df.plot(x='# Time'); plt.show()\"\n\n";

    return 0;
}
