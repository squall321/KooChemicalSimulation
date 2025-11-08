# MPI Parallelization Guide

**Version**: 6.0.0-alpha5
**Priority**: D1
**Status**: Initial Implementation
**Last Updated**: 2025-11-08

---

## Overview

KooLab now supports MPI (Message Passing Interface) parallelization for large-scale HPC simulations. This enables distributed-memory parallel computing across multiple nodes in supercomputing clusters.

### Features

✅ **Domain Decomposition**
- 1D, 2D, and 3D Cartesian decomposition
- Automatic load balancing
- Ghost cell communication

✅ **Parallel Solvers**
- MPI-parallel diffusion solver (1D, 2D)
- MPI-parallel reaction-diffusion solver (planned)
- Support for explicit and implicit time-stepping

✅ **Performance Monitoring**
- Computation time tracking
- Communication overhead measurement
- Scaling efficiency analysis

✅ **HPC Integration**
- Compatible with Apptainer containers
- SLURM job script templates
- Optimized for distributed-memory architectures

---

## Quick Start

### 1. Build with MPI Support

```bash
# Enable MPI during CMake configuration
cmake -B build \
    -DENABLE_MPI=ON \
    -DBUILD_TESTING=ON \
    -DBUILD_EXAMPLES=ON

# Build
cmake --build build -j$(nproc)
```

### 2. Run MPI Example

```bash
# Run 1D diffusion example with 4 processes
mpirun -np 4 ./build/examples/mpi_diffusion_1d_example

# Run 2D diffusion example with 16 processes (4x4 grid)
mpirun -np 16 ./build/examples/mpi_diffusion_2d_example
```

### 3. Run MPI Tests

```bash
# Run tests with MPI
cd build
mpirun -np 4 ./tests/parallel/test_mpi_diffusion
```

---

## Usage Examples

### Example 1: 1D Diffusion

```cpp
#include "parallel/solver/MPIDiffusionSolver.h"
#include "parallel/mpi/MPIWrapper.h"

int main(int argc, char** argv) {
    // Initialize MPI
    koo::parallel::mpi::MPIEnvironment mpiEnv(&argc, &argv);
    auto comm = koo::parallel::mpi::MPIEnvironment::getWorldComm();

    // Problem parameters
    int globalNx = 1000;           // Global grid points
    double L = 1.0;                // Domain length (m)
    double D = 1.0e-9;             // Diffusion coefficient (m²/s)

    // Create MPI solver
    koo::parallel::solver::MPIDiffusionSolver1D solver(
        globalNx, L, D, comm);

    // Set initial condition (Gaussian pulse)
    auto ic = [L](double x) {
        double x0 = L / 2.0;
        double sigma = 0.1;
        return std::exp(-(x - x0) * (x - x0) / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(ic);

    // Set boundary conditions
    solver.setNeumannBC(0.0, 0.0);  // Zero flux

    // Solve
    double dt = 0.0001;             // Time step (s)
    int nSteps = 10000;             // Number of steps
    solver.solve(dt, nSteps, 1000); // Output every 1000 steps

    // Gather and save results
    std::vector<double> globalC, globalX;
    solver.gatherGlobalSolution(globalC, globalX);

    if (comm.isRoot()) {
        // Write output (only on root process)
        koo::io::VTKWriter::write1DStructuredGrid(
            "result.vtk", globalX, globalC, "concentration");
    }

    return 0;
}
```

### Example 2: 2D Diffusion

```cpp
#include "parallel/solver/MPIDiffusionSolver.h"
#include "parallel/mpi/MPIWrapper.h"

int main(int argc, char** argv) {
    koo::parallel::mpi::MPIEnvironment mpiEnv(&argc, &argv);
    auto comm = koo::parallel::mpi::MPIEnvironment::getWorldComm();

    // 2D problem
    int globalNx = 200, globalNy = 200;
    double Lx = 1.0, Ly = 1.0;
    double D = 1.0e-9;

    koo::parallel::solver::MPIDiffusionSolver2D solver(
        globalNx, globalNy, Lx, Ly, D, comm);

    // Gaussian initial condition in center
    auto ic = [Lx, Ly](double x, double y) {
        double x0 = Lx / 2.0, y0 = Ly / 2.0;
        double sigma = 0.1;
        double r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
        return std::exp(-r2 / (2.0 * sigma * sigma));
    };
    solver.setInitialCondition(ic);

    // Dirichlet BC (zero on boundaries)
    solver.setDirichletBC(0.0);

    // Solve
    double dt = 0.00005;
    solver.solve(dt, 5000, 500);

    return 0;
}
```

---

## Domain Decomposition

### 1D Decomposition

The 1D domain is split contiguously among processes:

```
Global domain: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
Processes: 4

Rank 0: [0, 1, 2]      + ghosts
Rank 1: [3, 4, 5]      + ghosts
Rank 2: [6, 7, 8]      + ghosts
Rank 3: [9]            + ghosts
```

Each process maintains:
- **Interior points**: Local domain points
- **Ghost cells**: Boundary values from neighbors

### 2D Decomposition

Uses Cartesian topology for 2D grids:

```
Process grid (4 processes, 2x2):

+-------+-------+
| P0    | P1    |
| (0,0) | (1,0) |
+-------+-------+
| P2    | P3    |
| (0,1) | (1,1) |
+-------+-------+
```

### 3D Decomposition

Extends to 3D Cartesian grids (implementation in progress).

---

## Ghost Cell Communication

Ghost cells enable communication between neighboring domains:

```cpp
// Ghost cell exchange happens automatically in stepExplicit()
solver.stepExplicit(dt);

// Manual ghost exchange (if needed)
// solver.exchangeGhostCells();  // Internal method
```

### Communication Pattern (1D)

```
Before exchange:
Rank 0: [?, C0, C1, C2, ?]
Rank 1: [?, C3, C4, C5, ?]

After exchange:
Rank 0: [X, C0, C1, C2, C3]  <- C3 from Rank 1
Rank 1: [C2, C3, C4, C5, X]  <- C2 from Rank 0
```

---

## Performance Analysis

### CFL Condition

For stability, ensure:

```cpp
double cfl = solver.checkCFL(dt);
// CFL should be < 0.5 for explicit schemes
```

CFL number: `CFL = D * dt / dx²` (1D) or `D * dt * (1/dx² + 1/dy²)` (2D)

### Performance Statistics

The solver automatically tracks:

```
========================================
MPI Solver Performance Statistics
========================================
Total time:          2.345678 s
Compute time:        2.100000 s (89.5%)
Communication time:  0.200000 s (8.5%)
I/O time:            0.045678 s (1.9%)
Time steps:          10000
Ghost exchanges:     10000
Avg step time:       0.000235 s
========================================
```

### Scaling Guidelines

**Strong Scaling** (fixed problem size, more processes):
- Target: >70% efficiency up to 8-16 processes
- Communication overhead increases with more processes

**Weak Scaling** (problem size grows with processes):
- Target: >90% efficiency
- Ideal for large-scale HPC

---

## HPC Cluster Usage

### SLURM Job Script

```bash
#!/bin/bash
#SBATCH --job-name=koolab_mpi
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=32
#SBATCH --time=01:00:00
#SBATCH --partition=compute

# Load MPI module
module load openmpi/4.1.4

# Run with MPI
mpirun -np 128 ./mpi_diffusion_2d_example
```

### PBS/Torque Script

```bash
#!/bin/bash
#PBS -N koolab_mpi
#PBS -l nodes=4:ppn=32
#PBS -l walltime=01:00:00
#PBS -q batch

cd $PBS_O_WORKDIR
module load openmpi

mpirun -np 128 ./mpi_diffusion_2d_example
```

### Apptainer Container

```bash
# Build container with MPI support
sudo apptainer build koolab_mpi.sif koolab.def

# Run with MPI
mpirun -np 4 apptainer run koolab_mpi.sif \
    /opt/koolab/bin/mpi_diffusion_1d_example
```

---

## API Reference

### MPIDiffusionSolver1D

```cpp
class MPIDiffusionSolver1D {
public:
    // Constructor
    MPIDiffusionSolver1D(int globalNx, double L, double diffusivity,
                         const mpi::MPIComm& comm);

    // Setup
    void setInitialCondition(std::function<double(double)> ic);
    void setDirichletBC(double left, double right);
    void setNeumannBC(double leftFlux, double rightFlux);

    // Solve
    void stepExplicit(double dt);
    void solve(double dt, int nSteps, int outputInterval = 0);

    // Access
    const std::vector<double>& getLocalConcentration() const;
    const std::vector<double>& getLocalCoordinates() const;
    void gatherGlobalSolution(std::vector<double>& globalC,
                             std::vector<double>& globalX);

    // Utilities
    double checkCFL(double dt) const;
    double getCurrentTime() const;
    const MPISolverStats& getStats() const;
};
```

### MPIDiffusionSolver2D

```cpp
class MPIDiffusionSolver2D {
public:
    // Constructor
    MPIDiffusionSolver2D(int globalNx, int globalNy,
                         double Lx, double Ly, double diffusivity,
                         const mpi::MPIComm& comm);

    // Setup
    void setInitialCondition(std::function<double(double, double)> ic);
    void setDirichletBC(double value = 0.0);

    // Solve
    void stepExplicit(double dt);
    void solve(double dt, int nSteps, int outputInterval = 0);

    // Access
    const std::vector<std::vector<double>>& getLocalConcentration() const;
    void gatherGlobalSolution(std::vector<std::vector<double>>& globalC);

    // Utilities
    double checkCFL(double dt) const;
    double getCurrentTime() const;
    const MPISolverStats& getStats() const;
};
```

---

## Troubleshooting

### Problem: MPI not found during build

**Solution**: Install MPI and ensure it's in PATH
```bash
# Ubuntu/Debian
sudo apt-get install libopenmpi-dev openmpi-bin

# RHEL/CentOS
sudo yum install openmpi openmpi-devel

# Load module on HPC
module load openmpi
```

### Problem: Segmentation fault with MPI

**Possible causes**:
1. Array bounds issues - check domain decomposition
2. Uninitialized ghost cells - ensure `exchangeGhostCells()` is called
3. MPI type mismatch - verify data types in send/recv

**Debugging**:
```bash
# Run with MPI debugger
mpirun -np 4 gdb ./mpi_diffusion_1d_example

# Enable MPI error checking
export MPICH_ERROR_CHECK=1
```

### Problem: Poor scaling performance

**Solutions**:
1. Reduce communication frequency (larger time steps if stable)
2. Increase local problem size (more points per process)
3. Use faster interconnect (InfiniBand vs Ethernet)
4. Check load balance across processes

---

## Future Enhancements

### Planned Features (Priority D1 continuation)

- [ ] **Reaction-Diffusion MPI Solver**
- [ ] **3D MPI Solver** (full implementation)
- [ ] **Implicit Time-Stepping** (for stiff problems)
- [ ] **Load Balancing** (dynamic redistribution)
- [ ] **Parallel I/O** (MPI-IO, parallel HDF5, parallel VTK)
- [ ] **Overlap Communication/Computation**
- [ ] **One-sided MPI** (RMA for reduced synchronization)

---

## References

- MPI Standard: https://www.mpi-forum.org/
- OpenMPI Documentation: https://www.open-mpi.org/doc/
- MPICH User Guide: https://www.mpich.org/documentation/guides/
- HPC Best Practices: https://hpc.llnl.gov/

---

## Examples Directory

```
examples/
├── mpi_diffusion_1d_example.cpp    # 1D parallel diffusion
├── mpi_diffusion_2d_example.cpp    # 2D parallel diffusion
└── (coming soon)
    ├── mpi_reaction_diffusion.cpp  # Parallel reaction-diffusion
    └── mpi_scaling_benchmark.cpp   # Scaling analysis
```

---

## Contact & Support

For MPI-related questions:
- GitHub Issues: https://github.com/squall321/KooChemicalSimulation/issues
- Tag issues with `mpi`, `parallel`, or `hpc`

---

**Next Steps**: See PRIORITY_D_PLAN.md for upcoming MPI features and PINN integration.
