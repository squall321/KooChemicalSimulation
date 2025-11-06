/**
 * @file diffusion_example.cpp
 * @brief Example: 1D diffusion simulation
 * @author KooChemicalSimulation Development Team
 * @version 5.0.0
 *
 * Phase 48: Example Applications
 *
 * Demonstrates:
 * - Diffusion equation solving
 * - Fick's laws
 * - VTK output for visualization
 */

#include "physics/diffusion/DiffusionCoefficient.h"
#include "physics/diffusion/FickDiffusion.h"
#include "io/VTKWriter.h"
#include "io/Logger.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace koo;

int main() {
    std::cout << "KooChemicalSimulation - Diffusion Example" << std::endl;
    std::cout << "===========================================" << std::endl;

    // Problem setup: 1D diffusion in rod
    int nx = 100;
    double L = 1.0;  // Length (m)
    double dx = L / (nx - 1);

    // Diffusion coefficient
    double D = 1.0e-9;  // m²/s
    auto diffCoeff = std::make_shared<physics::ConstantDiffusion>(D);
    physics::FickDiffusion fick(diffCoeff);

    std::cout << "\nProblem setup:" << std::endl;
    std::cout << "  Domain: [0, " << L << "] m" << std::endl;
    std::cout << "  Grid points: " << nx << std::endl;
    std::cout << "  Spacing: " << dx << " m" << std::endl;
    std::cout << "  Diffusion coefficient: " << D << " m²/s" << std::endl;

    // Initialize concentration (Gaussian pulse)
    std::vector<double> x(nx);
    std::vector<double> C(nx);
    double x0 = L / 2.0;  // Center
    double sigma = 0.1;   // Width

    for (int i = 0; i < nx; ++i) {
        x[i] = i * dx;
        double dist = x[i] - x0;
        C[i] = std::exp(-dist * dist / (2.0 * sigma * sigma));
    }

    std::cout << "\nInitial condition: Gaussian pulse at x = " << x0 << " m" << std::endl;

    // Time integration
    double dt = 0.001;  // Time step (s)
    double t_final = 100.0;
    int n_steps = static_cast<int>(t_final / dt);
    int output_interval = 1000;

    // Check CFL condition
    double CFL = D * dt / (dx * dx);
    std::cout << "\nTime integration:" << std::endl;
    std::cout << "  Time step: " << dt << " s" << std::endl;
    std::cout << "  Final time: " << t_final << " s" << std::endl;
    std::cout << "  CFL number: " << CFL << " (should be < 0.5)" << std::endl;

    if (CFL >= 0.5) {
        std::cout << "  WARNING: CFL condition violated! Reduce dt." << std::endl;
    }

    // Logger
    auto& logger = io::Logger::getInstance();
    logger.info("Starting diffusion simulation");

    // Time loop
    std::vector<double> C_new(nx);
    std::vector<std::string> vtkFiles;

    for (int step = 0; step <= n_steps; ++step) {
        double t = step * dt;

        // Output
        if (step % output_interval == 0) {
            std::string filename = "diffusion_" + std::to_string(step / output_interval);
            io::VTKWriter::write1DStructuredGrid(filename + ".vtk", x, C, "concentration");
            vtkFiles.push_back(filename + ".vtk");

            logger.info("Step " + std::to_string(step) + " / " + std::to_string(n_steps) +
                       " (t = " + std::to_string(t) + " s)");
        }

        // Update (explicit Euler)
        for (int i = 1; i < nx - 1; ++i) {
            // Compute Laplacian (second derivative)
            double laplacian = (C[i+1] - 2.0 * C[i] + C[i-1]) / (dx * dx);

            // Time step
            double dC_dt = fick.calculateSourceTerm(laplacian, 300.0);
            C_new[i] = C[i] + dt * dC_dt;
        }

        // Boundary conditions (zero flux)
        C_new[0] = C_new[1];
        C_new[nx-1] = C_new[nx-2];

        // Update
        C = C_new;
    }

    // Write time series PVD
    std::vector<double> outputTimes;
    for (size_t i = 0; i < vtkFiles.size(); ++i) {
        outputTimes.push_back(i * output_interval * dt);
    }
    io::VTKWriter::writeTimeSeries("diffusion_series.pvd", outputTimes, vtkFiles);

    logger.info("Simulation complete");

    std::cout << "\n===========================================" << std::endl;
    std::cout << "Results exported to VTK format" << std::endl;
    std::cout << "  Time series: diffusion_series.pvd" << std::endl;
    std::cout << "  Open with ParaView or VisIt" << std::endl;
    std::cout << "===========================================" << std::endl;

    return 0;
}
