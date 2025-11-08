/**
 * @file mpi_diffusion_1d_example.cpp
 * @brief Example: MPI-parallel 1D diffusion simulation
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 *
 * Priority D1: MPI Parallelization
 *
 * Demonstrates:
 * - MPI-parallel diffusion solver
 * - Domain decomposition
 * - Ghost cell communication
 * - Performance statistics
 *
 * Usage:
 *   mpirun -np 4 ./mpi_diffusion_1d_example
 */

#include "parallel/solver/MPIDiffusionSolver.h"
#include "parallel/mpi/MPIWrapper.h"
#include "io/VTKWriter.h"
#include <iostream>
#include <cmath>
#include <memory>

using namespace koo;

int main(int argc, char** argv) {
    // Initialize MPI
    parallel::mpi::MPIEnvironment mpiEnv(&argc, &argv);
    auto comm = parallel::mpi::MPIEnvironment::getWorldComm();

    if (comm.isRoot()) {
        std::cout << "========================================\n";
        std::cout << "MPI Diffusion Example (1D)\n";
        std::cout << "========================================\n";
        std::cout << "Processes: " << comm.getSize() << "\n";
        std::cout << "========================================\n";
    }

    // Problem setup
    int globalNx = 1000;           // Global grid points
    double L = 1.0;                // Domain length (m)
    double D = 1.0e-9;             // Diffusion coefficient (m²/s)

    // Create solver
    parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, comm);

    // Initial condition: Gaussian pulse
    double x0 = L / 2.0;
    double sigma = 0.1;
    auto initialCondition = [x0, sigma](double x) {
        double dist = x - x0;
        return std::exp(-dist * dist / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(initialCondition);

    // Boundary conditions: zero flux (Neumann)
    solver.setNeumannBC(0.0, 0.0);

    // Time parameters
    double dt = 0.0001;             // Time step (s)
    int nSteps = 10000;             // Number of steps
    int outputInterval = 1000;      // Output every N steps

    // Check CFL
    double cfl = solver.checkCFL(dt);
    if (comm.isRoot()) {
        std::cout << "CFL number: " << cfl << "\n";
        if (cfl >= 0.5) {
            std::cout << "WARNING: CFL >= 0.5, simulation may be unstable!\n";
        }
    }

    // Solve
    solver.solve(dt, nSteps, outputInterval);

    // Gather and output final solution
    std::vector<double> globalC, globalX;
    solver.gatherGlobalSolution(globalC, globalX);

    if (comm.isRoot()) {
        // Write VTK output
        io::VTKWriter::write1DStructuredGrid("mpi_diffusion_1d_final.vtk",
                                             globalX, globalC, "concentration");

        std::cout << "\n========================================\n";
        std::cout << "Results written to mpi_diffusion_1d_final.vtk\n";
        std::cout << "Open with ParaView or VisIt\n";
        std::cout << "========================================\n";

        // Print some statistics
        std::cout << "\nFinal solution statistics:\n";
        double sum = 0.0, min_val = globalC[0], max_val = globalC[0];
        for (size_t i = 0; i < globalC.size(); ++i) {
            sum += globalC[i];
            if (globalC[i] < min_val) min_val = globalC[i];
            if (globalC[i] > max_val) max_val = globalC[i];
        }
        double avg = sum / globalC.size();

        std::cout << "  Min: " << min_val << "\n";
        std::cout << "  Max: " << max_val << "\n";
        std::cout << "  Avg: " << avg << "\n";
        std::cout << "  Sum: " << sum << "\n";
    }

    // Performance statistics are printed by solver.solve()

    return 0;
}
