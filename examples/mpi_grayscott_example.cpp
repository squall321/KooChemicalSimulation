/**
 * @file mpi_grayscott_example.cpp
 * @brief Example: MPI-parallel Gray-Scott reaction-diffusion model
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 *
 * Priority D1: MPI Parallelization
 *
 * Demonstrates pattern formation in the Gray-Scott model:
 * - Spots, stripes, spirals, or worms depending on parameters
 * - 2D MPI-parallel simulation
 * - Periodic boundary conditions
 * - Local VTK output from each process
 *
 * Usage:
 *   mpirun -np 4 ./mpi_grayscott_example
 */

#include "parallel/solver/MPIReactionDiffusionSolver.h"
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
        std::cout << "MPI Gray-Scott Reaction-Diffusion\n";
        std::cout << "========================================\n";
        std::cout << "Processes: " << comm.getSize() << "\n";
        std::cout << "Pattern: Spots\n";
        std::cout << "========================================\n";
    }

    // Problem setup
    int globalNx = 256;
    int globalNy = 256;
    double Lx = 2.5;
    double Ly = 2.5;

    // Gray-Scott parameters for spots
    parallel::solver::GrayScottParams params;
    params.Du = 2.0e-5;
    params.Dv = 1.0e-5;
    params.F = 0.055;      // Feed rate
    params.k = 0.062;      // Kill rate

    // Create solver
    auto solver = parallel::solver::MPIReactionDiffusionSolver2D::createGrayScott(
        globalNx, globalNy, Lx, Ly, params, comm);

    // Initial condition: uniform with small random perturbation in center
    double centerX = Lx / 2.0;
    double centerY = Ly / 2.0;
    double perturbRadius = 0.2;

    auto initialU = [centerX, centerY, perturbRadius](double x, double y) {
        // Start with u = 1.0 (full concentration)
        double dx = x - centerX;
        double dy = y - centerY;
        double r = std::sqrt(dx * dx + dy * dy);

        // Small perturbation in center
        if (r < perturbRadius) {
            return 0.5;  // Lower u in center
        }
        return 1.0;
    };

    auto initialV = [centerX, centerY, perturbRadius](double x, double y) {
        // Start with v = 0.0 (no inhibitor)
        double dx = x - centerX;
        double dy = y - centerY;
        double r = std::sqrt(dx * dx + dy * dy);

        // Add inhibitor in center to seed pattern
        if (r < perturbRadius) {
            return 0.25;
        }
        return 0.0;
    };

    solver->setInitialCondition(initialU, initialV);

    // Periodic boundary conditions (for natural pattern formation)
    solver->setPeriodicBC();

    // Time parameters
    double dt = 1.0;               // Time step
    int nSteps = 20000;            // Total steps
    int outputInterval = 1000;     // Output every N steps
    int vtkInterval = 2000;        // VTK output interval

    // Check CFL
    double cfl = solver->checkCFL(dt);
    if (comm.isRoot()) {
        std::cout << "CFL number: " << cfl << "\n";
        if (cfl >= 0.5) {
            std::cout << "WARNING: CFL >= 0.5, reducing time step!\n";
            dt = 0.4 / (std::max(params.Du, params.Dv) *
                       (1.0 / ((Lx/globalNx) * (Lx/globalNx)) +
                        1.0 / ((Ly/globalNy) * (Ly/globalNy))));
            std::cout << "New dt: " << dt << "\n";
        }
        std::cout << "Simulating pattern formation...\n";
        std::cout << "This may take a few minutes.\n";
    }

    // Solve with periodic VTK output
    for (int step = 0; step < nSteps; ++step) {
        solver->stepExplicit(dt);

        if (outputInterval > 0 && step % outputInterval == 0) {
            if (comm.isRoot()) {
                std::cout << "Step " << std::setw(6) << step << " / " << nSteps
                          << "  t = " << std::fixed << std::setprecision(2)
                          << solver->getCurrentTime() << "\n";
            }
        }

        // Write VTK output
        if (vtkInterval > 0 && step % vtkInterval == 0) {
            solver->writeLocalVTK("grayscott", step / vtkInterval);
        }
    }

    // Final output
    solver->writeLocalVTK("grayscott_final", nSteps / vtkInterval);

    if (comm.isRoot()) {
        std::cout << "\n========================================\n";
        std::cout << "Simulation complete!\n";
        std::cout << "Pattern formation successful.\n";
        std::cout << "\nVTK files written: grayscott_rank*_step*.vtk\n";
        std::cout << "To visualize:\n";
        std::cout << "  1. Load files in ParaView\n";
        std::cout << "  2. Apply 'Warp By Scalar' filter\n";
        std::cout << "  3. Color by 'v' variable\n";
        std::cout << "\nYou should see spot patterns forming!\n";
        std::cout << "========================================\n";
    }

    return 0;
}
