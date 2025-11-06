/**
 * @file surface_example.cpp
 * @brief Example: Surface catalysis simulation
 * @author KooChemicalSimulation Development Team
 * @version 5.0.0
 *
 * Phase 48: Example Applications
 *
 * Demonstrates:
 * - Surface species and sites
 * - Catalytic reactions (Langmuir-Hinshelwood)
 * - Coverage evolution
 * - Turnover frequency calculation
 */

#include "physics/surface/SurfaceSpecies.h"
#include "physics/surface/SurfaceReaction.h"
#include "io/OutputWriter.h"
#include "io/Logger.h"
#include <iostream>
#include <vector>
#include <map>

using namespace koo;

int main() {
    std::cout << "KooChemicalSimulation - Surface Catalysis Example" << std::endl;
    std::cout << "===================================================" << std::endl;

    // Setup logger
    auto& logger = io::Logger::getInstance();
    logger.info("Starting surface catalysis simulation");

    // Create surface species
    physics::surface::SurfaceSpecies CO("CO", 28.01e-3, 1, 1.5e5);  // MW in kg/mol, 1 site, binding energy
    physics::surface::SurfaceSpecies O("O", 16.00e-3, 1, 2.0e5);    // MW in kg/mol, 1 site, binding energy

    std::cout << "\nSurface species:" << std::endl;
    std::cout << "  CO*: binding energy = " << CO.getBindingEnergy() << " J/mol" << std::endl;
    std::cout << "  O*:  binding energy = " << O.getBindingEnergy() << " J/mol" << std::endl;

    // Create catalytic site
    double siteDensity = 1.0e19;  // sites/m²
    physics::surface::SurfaceSite site(physics::surface::SurfaceSite::SiteType::TERRACE, siteDensity);

    std::cout << "\nCatalytic site:" << std::endl;
    std::cout << "  Type: TERRACE" << std::endl;
    std::cout << "  Density: " << site.getDensity() << " sites/m²" << std::endl;

    // Create surface coverage manager
    physics::surface::SurfaceCoverage coverage(siteDensity);
    coverage.setCoverage("CO", 0.3);   // Initial CO coverage
    coverage.setCoverage("O", 0.2);    // Initial O coverage

    std::cout << "\nInitial coverage:" << std::endl;
    std::cout << "  θ_CO = " << coverage.getCoverage("CO") << std::endl;
    std::cout << "  θ_O  = " << coverage.getCoverage("O") << std::endl;
    std::cout << "  θ_*  = " << coverage.getVacantFraction() << " (vacant sites)" << std::endl;

    // Create Langmuir-Hinshelwood reaction: CO* + O* -> CO2 + 2*
    physics::surface::SurfaceReaction reaction("CO_oxidation",
        physics::surface::ReactionMechanism::LANGMUIR_HINSHELWOOD);
    reaction.addReactant("CO", 1);
    reaction.addReactant("O", 1);
    reaction.addProduct("CO2", 1);

    // Set kinetic parameters
    double Ea_LH = 100000.0;  // Activation energy (J/mol)
    double A_LH = 1.0e13;  // Pre-exponential factor (1/s)
    reaction.setKineticParameters(Ea_LH, A_LH);

    std::cout << "\nReaction mechanism: CO* + O* -> CO2 + 2*" << std::endl;
    std::cout << "  Type: Langmuir-Hinshelwood" << std::endl;
    std::cout << "  A = " << A_LH << " (1/s)" << std::endl;
    std::cout << "  Ea = " << Ea_LH << " J/mol" << std::endl;

    // Simulation parameters
    double T = 500.0;  // Temperature (K)
    double dt = 1.0e-6;  // Time step (s)
    double t_final = 0.01;  // Final time (s)
    int n_steps = static_cast<int>(t_final / dt);
    int output_interval = 1000;

    std::cout << "\nSimulation parameters:" << std::endl;
    std::cout << "  Temperature: " << T << " K" << std::endl;
    std::cout << "  Time step: " << dt << " s" << std::endl;
    std::cout << "  Final time: " << t_final << " s" << std::endl;

    // Storage for results
    std::vector<double> times;
    std::vector<std::vector<double>> coverages(3);  // CO, O, vacant

    // Time integration
    logger.info("Running time integration");

    for (int step = 0; step <= n_steps; ++step) {
        double t = step * dt;

        // Output
        if (step % output_interval == 0) {
            times.push_back(t);
            coverages[0].push_back(coverage.getCoverage("CO"));
            coverages[1].push_back(coverage.getCoverage("O"));
            coverages[2].push_back(coverage.getVacantFraction());

            if (step % (output_interval * 10) == 0) {
                logger.info("Step " + std::to_string(step) + " / " + std::to_string(n_steps) +
                           " (t = " + std::to_string(t) + " s)");
            }
        }

        if (step < n_steps) {
            // Get current coverages
            std::map<std::string, double> currentCoverages;
            currentCoverages["CO"] = coverage.getCoverage("CO");
            currentCoverages["O"] = coverage.getCoverage("O");

            // Compute reaction rate (Langmuir-Hinshelwood)
            double rate = reaction.calculateLHRate(T, currentCoverages);

            // Update coverages
            double dtheta_CO = -rate * dt;
            double dtheta_O = -rate * dt;

            coverage.setCoverage("CO", currentCoverages["CO"] + dtheta_CO);
            coverage.setCoverage("O", currentCoverages["O"] + dtheta_O);

            // Ensure physical bounds
            coverage.setCoverage("CO", std::max(0.0, std::min(1.0, coverage.getCoverage("CO"))));
            coverage.setCoverage("O", std::max(0.0, std::min(1.0, coverage.getCoverage("O"))));
        }
    }

    std::cout << "\nFinal coverage:" << std::endl;
    std::cout << "  θ_CO = " << coverage.getCoverage("CO") << std::endl;
    std::cout << "  θ_O  = " << coverage.getCoverage("O") << std::endl;
    std::cout << "  θ_*  = " << coverage.getVacantFraction() << std::endl;

    // Transpose results for export (from [species][time] to [time][species])
    std::vector<std::vector<double>> transposed(times.size());
    for (size_t t = 0; t < times.size(); ++t) {
        transposed[t] = {coverages[0][t], coverages[1][t], coverages[2][t]};
    }

    // Export results
    io::DataExporter::exportTimeSeries("surface_results.csv", times, transposed,
                                      {"CO_coverage", "O_coverage", "vacant"});

    logger.info("Results exported to surface_results.csv");

    std::cout << "\n===================================================" << std::endl;
    std::cout << "Simulation complete!" << std::endl;
    std::cout << "Results: surface_results.csv" << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
