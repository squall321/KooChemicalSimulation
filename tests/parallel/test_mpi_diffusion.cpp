/**
 * @file test_mpi_diffusion.cpp
 * @brief Unit tests for MPI diffusion solvers
 */

#include "parallel/solver/MPIDiffusionSolver.h"
#include "parallel/mpi/MPIWrapper.h"
#include <gtest/gtest.h>
#include <cmath>

using namespace koo;

class MPIDiffusionTest : public ::testing::Test {
protected:
    void SetUp() override {
        comm_ = std::make_unique<parallel::mpi::MPIComm>();
    }

    std::unique_ptr<parallel::mpi::MPIComm> comm_;
};

TEST_F(MPIDiffusionTest, Constructor1D) {
    int globalNx = 100;
    double L = 1.0;
    double D = 1.0e-9;

    ASSERT_NO_THROW({
        parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, *comm_);
    });
}

TEST_F(MPIDiffusionTest, DomainDecomposition1D) {
    int globalNx = 100;
    double L = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, *comm_);

    auto bounds = solver.getLocalBounds();
    EXPECT_GE(bounds.first, 0);
    EXPECT_LE(bounds.second, globalNx);
    EXPECT_LT(bounds.first, bounds.second);  // Non-empty domain
}

TEST_F(MPIDiffusionTest, InitialCondition1D) {
    int globalNx = 100;
    double L = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, *comm_);

    // Set constant initial condition
    auto ic = [](double x) { return 1.0; };
    ASSERT_NO_THROW({
        solver.setInitialCondition(ic);
    });

    const auto& C = solver.getLocalConcentration();
    // Check that interior points are set (skip ghosts)
    for (size_t i = 1; i < C.size() - 1; ++i) {
        EXPECT_NEAR(C[i], 1.0, 1e-10);
    }
}

TEST_F(MPIDiffusionTest, CFLCheck1D) {
    int globalNx = 100;
    double L = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, *comm_);

    double dx = L / (globalNx - 1);
    double dt_safe = 0.4 * dx * dx / D;
    double dt_unsafe = 0.6 * dx * dx / D;

    EXPECT_LT(solver.checkCFL(dt_safe), 0.5);
    EXPECT_GT(solver.checkCFL(dt_unsafe), 0.5);
}

TEST_F(MPIDiffusionTest, SingleStep1D) {
    int globalNx = 100;
    double L = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, *comm_);

    // Gaussian initial condition
    auto ic = [L](double x) {
        double x0 = L / 2.0;
        double sigma = 0.1;
        return std::exp(-(x - x0) * (x - x0) / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(ic);
    solver.setNeumannBC(0.0, 0.0);

    double dx = L / (globalNx - 1);
    double dt = 0.4 * dx * dx / D;  // Safe time step

    ASSERT_NO_THROW({
        solver.stepExplicit(dt);
    });

    EXPECT_NEAR(solver.getCurrentTime(), dt, 1e-10);
}

TEST_F(MPIDiffusionTest, ConservationOfMass1D) {
    int globalNx = 100;
    double L = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver1D solver(globalNx, L, D, *comm_);

    // Uniform initial condition
    auto ic = [](double x) { return 1.0; };
    solver.setInitialCondition(ic);
    solver.setNeumannBC(0.0, 0.0);  // Zero flux BC

    // Compute initial mass (local)
    const auto& C_init = solver.getLocalConcentration();
    double localMass_init = 0.0;
    auto bounds = solver.getLocalBounds();
    int localSize = bounds.second - bounds.first;
    for (int i = 1; i <= localSize; ++i) {
        localMass_init += C_init[i];
    }

    // Global sum
    double globalMass_init = comm_->sum(localMass_init);

    // Evolve
    double dx = L / (globalNx - 1);
    double dt = 0.4 * dx * dx / D;
    for (int i = 0; i < 10; ++i) {
        solver.stepExplicit(dt);
    }

    // Compute final mass
    const auto& C_final = solver.getLocalConcentration();
    double localMass_final = 0.0;
    for (int i = 1; i <= localSize; ++i) {
        localMass_final += C_final[i];
    }
    double globalMass_final = comm_->sum(localMass_final);

    // Mass should be conserved (within numerical error)
    if (comm_->isRoot()) {
        EXPECT_NEAR(globalMass_final, globalMass_init, 1e-6 * globalMass_init);
    }
}

TEST_F(MPIDiffusionTest, Constructor2D) {
    int globalNx = 50;
    int globalNy = 50;
    double Lx = 1.0;
    double Ly = 1.0;
    double D = 1.0e-9;

    ASSERT_NO_THROW({
        parallel::solver::MPIDiffusionSolver2D solver(globalNx, globalNy, Lx, Ly, D, *comm_);
    });
}

TEST_F(MPIDiffusionTest, InitialCondition2D) {
    int globalNx = 50;
    int globalNy = 50;
    double Lx = 1.0;
    double Ly = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver2D solver(globalNx, globalNy, Lx, Ly, D, *comm_);

    auto ic = [](double x, double y) { return 1.0; };
    ASSERT_NO_THROW({
        solver.setInitialCondition(ic);
    });
}

TEST_F(MPIDiffusionTest, SingleStep2D) {
    int globalNx = 50;
    int globalNy = 50;
    double Lx = 1.0;
    double Ly = 1.0;
    double D = 1.0e-9;

    parallel::solver::MPIDiffusionSolver2D solver(globalNx, globalNy, Lx, Ly, D, *comm_);

    // Gaussian initial condition
    auto ic = [Lx, Ly](double x, double y) {
        double x0 = Lx / 2.0;
        double y0 = Ly / 2.0;
        double sigma = 0.1;
        double r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
        return std::exp(-r2 / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(ic);
    solver.setDirichletBC(0.0);

    double dx = Lx / (globalNx - 1);
    double dy = Ly / (globalNy - 1);
    double dt = 0.2 / (D * (1.0 / (dx * dx) + 1.0 / (dy * dy)));

    ASSERT_NO_THROW({
        solver.stepExplicit(dt);
    });

    EXPECT_NEAR(solver.getCurrentTime(), dt, 1e-10);
}

// Main function for running tests with MPI
int main(int argc, char** argv) {
    // Initialize MPI
    parallel::mpi::MPIEnvironment mpiEnv(&argc, &argv);

    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Run tests
    int result = RUN_ALL_TESTS();

    return result;
}
