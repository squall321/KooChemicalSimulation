/**
 * @file reaction_example.cpp
 * @brief Example: Chemical reaction simulation
 * @author KooChemicalSimulation Development Team
 * @version 5.0.0
 *
 * Phase 48: Example Applications
 *
 * Demonstrates:
 * - Species management
 * - Reaction mechanism definition
 * - Kinetics solving
 * - Result visualization
 */

#include "chemistry/species/Species.h"
#include "chemistry/reaction/Reaction.h"
#include "io/OutputWriter.h"
#include <iostream>
#include <vector>

using namespace koo;

int main() {
    std::cout << "KooChemicalSimulation - Reaction Example" << std::endl;
    std::cout << "=========================================" << std::endl;

    // Create reaction: 2H2 + O2 -> 2H2O
    chemistry::Reaction reaction("H2_O2_combustion", chemistry::ReactionType::ELEMENTARY, false);
    reaction.addReactant("H2", 2.0);
    reaction.addReactant("O2", 1.0);
    reaction.addProduct("H2O", 2.0);

    // Set Arrhenius parameters for forward rate
    chemistry::RateLaw forwardRate;
    forwardRate.A = 1.0e13;      // Pre-exponential factor (1/s)
    forwardRate.beta = 0.0;      // Temperature exponent
    forwardRate.Ea = 150000.0;   // Activation energy (J/mol)
    reaction.setForwardRateLaw(forwardRate);

    std::cout << "\nReaction mechanism:" << std::endl;
    std::cout << "  2H2 + O2 -> 2H2O" << std::endl;
    std::cout << "  A = " << forwardRate.A << " (1/s)" << std::endl;
    std::cout << "  Ea = " << forwardRate.Ea << " J/mol" << std::endl;

    // Initial conditions
    double T = 1000.0;  // Temperature (K)
    std::map<std::string, double> concentrations;
    concentrations["H2"] = 2.0;   // mol/m³
    concentrations["O2"] = 1.0;
    concentrations["H2O"] = 0.0;

    std::cout << "\nInitial concentrations:" << std::endl;
    std::cout << "  [H2]  = " << concentrations["H2"] << " mol/m³" << std::endl;
    std::cout << "  [O2]  = " << concentrations["O2"] << " mol/m³" << std::endl;
    std::cout << "  [H2O] = " << concentrations["H2O"] << " mol/m³" << std::endl;

    // Simple time integration
    double dt = 1.0e-6;  // Time step (s)
    double t_final = 1.0e-3;  // Final time (s)
    int n_steps = static_cast<int>(t_final / dt);

    std::vector<double> times;
    std::vector<std::vector<double>> results(3);  // H2, O2, H2O

    std::cout << "\nSimulating reaction..." << std::endl;
    std::cout << "  Time step: " << dt << " s" << std::endl;
    std::cout << "  Final time: " << t_final << " s" << std::endl;

    // Time integration loop
    for (int step = 0; step <= n_steps; step += 100) {
        double t = step * dt;
        times.push_back(t);
        results[0].push_back(concentrations["H2"]);
        results[1].push_back(concentrations["O2"]);
        results[2].push_back(concentrations["H2O"]);

        if (step < n_steps) {
            // Compute reaction rate (forward rate of progress)
            double rate = reaction.getForwardRateOfProgress(T, concentrations);

            // Update concentrations (simple Euler)
            concentrations["H2"] -= 2.0 * rate * dt;
            concentrations["O2"] -= 1.0 * rate * dt;
            concentrations["H2O"] += 2.0 * rate * dt;

            // Ensure non-negative
            concentrations["H2"] = std::max(0.0, concentrations["H2"]);
            concentrations["O2"] = std::max(0.0, concentrations["O2"]);
        }
    }

    std::cout << "\nFinal concentrations:" << std::endl;
    std::cout << "  [H2]  = " << concentrations["H2"] << " mol/m³" << std::endl;
    std::cout << "  [O2]  = " << concentrations["O2"] << " mol/m³" << std::endl;
    std::cout << "  [H2O] = " << concentrations["H2O"] << " mol/m³" << std::endl;

    // Transpose results for export (from [species][time] to [time][species])
    std::vector<std::vector<double>> transposed(times.size());
    for (size_t t = 0; t < times.size(); ++t) {
        transposed[t] = {results[0][t], results[1][t], results[2][t]};
    }

    // Export results
    io::DataExporter::exportTimeSeries("reaction_results.csv", times, transposed,
                                      {"H2", "O2", "H2O"});

    std::cout << "\nResults exported to: reaction_results.csv" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Simulation complete!" << std::endl;

    return 0;
}
