/**
 * @file mpi_diffusion_2d_example.cpp
 * @brief Example: MPI-parallel 2D diffusion simulation
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 *
 * Priority D1: MPI Parallelization
 *
 * Demonstrates:
 * - 2D MPI-parallel diffusion
 * - Cartesian domain decomposition
 * - 2D ghost cell exchange
 *
 * Usage:
 *   mpirun -np 4 ./mpi_diffusion_2d_example
 */

#include "parallel/solver/MPIDiffusionSolver.h"
#include "parallel/mpi/MPIWrapper.h"
#include <iostream>
#include <cmath>

using namespace koo;

int main(int argc, char** argv) {
    // Initialize MPI
    parallel::mpi::MPIEnvironment mpiEnv(&argc, &argv);
    auto comm = parallel::mpi::MPIEnvironment::getWorldComm();

    if (comm.isRoot()) {
        std::cout << "========================================\n";
        std::cout << "MPI Diffusion Example (2D)\n";
        std::cout << "========================================\n";
        std::cout << "Processes: " << comm.getSize() << "\n";
        std::cout << "========================================\n";
    }

    // Problem setup
    int globalNx = 200;            // Global grid points in x
    int globalNy = 200;            // Global grid points in y
    double Lx = 1.0;               // Domain length x (m)
    double Ly = 1.0;               // Domain length y (m)
    double D = 1.0e-9;             // Diffusion coefficient (m²/s)

    // Create solver
    parallel::solver::MPIDiffusionSolver2D solver(globalNx, globalNy, Lx, Ly, D, comm);

    // Initial condition: Gaussian pulse in center
    double x0 = Lx / 2.0;
    double y0 = Ly / 2.0;
    double sigma = 0.1;
    auto initialCondition = [x0, y0, sigma](double x, double y) {
        double dx = x - x0;
        double dy = y - y0;
        double r2 = dx * dx + dy * dy;
        return std::exp(-r2 / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(initialCondition);

    // Boundary conditions: Dirichlet (zero on boundaries)
    solver.setDirichletBC(0.0);

    // Time parameters
    double dt = 0.00005;           // Time step (s)
    int nSteps = 5000;             // Number of steps
    int outputInterval = 500;      // Output every N steps

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

    if (comm.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "Simulation complete!\n";
        std::cout << "Total time: " << solver.getCurrentTime() << " s\n";
        std::cout << "========================================\n";
    }

    // Note: 2D parallel VTK output would require more complex implementation
    // For now, each process could write its own subdomain

    return 0;
}
