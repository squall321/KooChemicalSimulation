# Getting Started with KooChemicalSimulation

Welcome to KooChemicalSimulation! This guide will help you get started with the framework.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Building the Project](#building-the-project)
4. [Running Your First Simulation](#running-your-first-simulation)
5. [Python Interface](#python-interface)
6. [Common Issues](#common-issues)
7. [Next Steps](#next-steps)

---

## Prerequisites

### Required

- **C++ Compiler**: GCC 9+ or Clang 10+ (C++17 support required)
- **CMake**: 3.15 or higher
- **Python**: 3.8+ (for Python bindings)

### Optional

- **Eigen3**: For advanced linear algebra (highly recommended)
- **GTest**: For running unit tests
- **pybind11**: For Python bindings (v3.0+)
- **CUDA Toolkit**: For GPU acceleration (11.0+)
- **ROCm**: For AMD GPU support (4.0+)
- **MPI**: For parallel computing
- **spdlog**: For advanced logging
- **VTK**: For visualization output

### System Requirements

- **Minimum RAM**: 4 GB
- **Recommended RAM**: 16 GB+ for large simulations
- **Disk Space**: ~500 MB for source + build

---

## Installation

### Ubuntu/Debian

```bash
# Install required dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    g++ \
    python3 \
    python3-pip

# Install optional dependencies
sudo apt-get install -y \
    libeigen3-dev \
    libgtest-dev \
    libspdlog-dev \
    python3-numpy \
    python3-matplotlib

# Install pybind11
pip3 install pybind11
```

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake eigen spdlog pybind11
brew install python@3.11

# Install Python packages
pip3 install numpy matplotlib jupyter
```

### Windows (WSL2 Recommended)

We recommend using Windows Subsystem for Linux 2 (WSL2) with Ubuntu. Follow the Ubuntu instructions above after setting up WSL2.

---

## Building the Project

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/KooChemicalSimulation.git
cd KooChemicalSimulation
```

### 2. Create Build Directory

```bash
mkdir build
cd build
```

### 3. Configure with CMake

**Basic Build** (CPU only):
```bash
cmake ..
```

**With all features**:
```bash
cmake .. \
    -DUSE_EIGEN=ON \
    -DUSE_SPDLOG=ON \
    -DBUILD_TESTING=ON \
    -DBUILD_PYTHON=ON
```

**With GPU support** (requires CUDA):
```bash
cmake .. \
    -DUSE_GPU=ON \
    -DUSE_CUDA=ON \
    -DCMAKE_CUDA_ARCHITECTURES=75  # Adjust for your GPU
```

### 4. Build

```bash
# Build all targets
cmake --build . -j$(nproc)

# Or build specific target
cmake --build . --target diffusion_example -j$(nproc)
```

### 5. Verify Build

```bash
# Run a simple example
./examples/diffusion_example

# Run tests (if GTest is available)
ctest --output-on-failure
```

---

## Running Your First Simulation

### Example 1: Simple 1D Diffusion (C++)

Create a file `my_first_sim.cpp`:

```cpp
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    // Parameters
    const int nx = 100;
    const double L = 1.0;
    const double D = 0.01;
    const double dt = 0.001;
    const double dx = L / (nx - 1);
    const double alpha = D * dt / (dx * dx);

    // Check CFL condition
    if (alpha > 0.5) {
        std::cerr << "Warning: CFL condition violated! alpha = " << alpha << std::endl;
    }

    // Initialize concentration
    std::vector<double> C(nx, 0.0);
    std::vector<double> x(nx);

    for (int i = 0; i < nx; ++i) {
        x[i] = i * dx;
        C[i] = std::exp(-std::pow(x[i] - 0.5, 2) / (2 * 0.05 * 0.05));
    }

    // Time stepping
    const int num_steps = 1000;
    for (int step = 0; step < num_steps; ++step) {
        std::vector<double> C_new = C;

        for (int i = 1; i < nx - 1; ++i) {
            C_new[i] = C[i] + alpha * (C[i+1] - 2*C[i] + C[i-1]);
        }

        C = C_new;
    }

    // Output results
    std::cout << "Final concentration profile:\n";
    for (int i = 0; i < nx; i += 10) {
        std::cout << "x = " << x[i] << ", C = " << C[i] << std::endl;
    }

    return 0;
}
```

Compile and run:

```bash
g++ -std=c++17 my_first_sim.cpp -o my_first_sim
./my_first_sim
```

### Example 2: Using KooLab Framework

```cpp
#include "core/types/CommonTypes.h"
#include "mesh/core/MeshData.h"
#include "mesh/core/Node.h"
#include "utils/logger/Logger.h"

int main() {
    using namespace koo;

    // Initialize logger
    utils::logger::Logger::initialize("MySimulation");
    utils::logger::Logger::setLevel(utils::logger::LogLevel::INFO);

    utils::logger::Logger::info("Starting simulation");

    // Create a simple 2D mesh
    mesh::core::MeshData mesh;

    // Add nodes
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            double x = i * 0.1;
            double y = j * 0.1;
            mesh.addNode(mesh::core::Node(i * 10 + j, x, y, 0.0));
        }
    }

    utils::logger::Logger::info("Created mesh with " +
                                std::to_string(mesh.getNumNodes()) + " nodes");

    // Your simulation code here...

    utils::logger::Logger::info("Simulation complete");
    return 0;
}
```

Compile with KooLab:

```bash
cd build
cmake --build . --target my_simulation
./my_simulation
```

---

## Python Interface

### Installation

```bash
cd build
cmake --build . --target _core

# Add to Python path
export PYTHONPATH=$PYTHONPATH:$(pwd)
```

### Example: Basic Usage

```python
import sys
sys.path.insert(0, 'build')  # Adjust path to your build directory

import _core as koo
import numpy as np

# Initialize logger
koo.Logger.initialize("PythonSim")
koo.Logger.set_level(koo.LogLevel.INFO)
koo.Logger.info("Starting Python simulation")

# Create a 2D mesh
mesh = koo.create_rectangular_mesh(
    x0=0.0, y0=0.0,
    x1=1.0, y1=1.0,
    nx=10, ny=10
)

print(f"Created mesh: {mesh}")
print(f"Number of nodes: {mesh.get_num_nodes()}")
print(f"Number of elements: {mesh.get_num_elements()}")

# Access nodes
node = mesh.get_node(0)
if node:
    print(f"First node: id={node.id}, x={node.x}, y={node.y}")
```

### Jupyter Notebooks

See the `notebooks/` directory for interactive tutorials:

```bash
cd notebooks
jupyter notebook 01_basic_usage.ipynb
```

---

## Common Issues

### Issue 1: CMake cannot find Eigen3

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install libeigen3-dev

# Or specify path manually
cmake .. -DEigen3_DIR=/path/to/eigen3/share/eigen3/cmake
```

### Issue 2: Python module `_core` not found

**Solution**:
```bash
# Make sure you built the Python module
cd build
cmake --build . --target _core

# Add build directory to PYTHONPATH
export PYTHONPATH=$PYTHONPATH:$(pwd)

# Or install system-wide
sudo cmake --install .
```

### Issue 3: Linker errors about missing symbols

**Solution**:
```bash
# Clean and rebuild
rm -rf build
mkdir build
cd build
cmake .. && cmake --build . -j$(nproc)
```

### Issue 4: Tests fail to compile

**Solution**:
```bash
# Install GTest
sudo apt-get install libgtest-dev

# Or disable testing
cmake .. -DBUILD_TESTING=OFF
```

### Issue 5: Warning about CFL condition

This means your time step `dt` is too large for stability. Either:
- Reduce `dt`
- Increase spatial resolution (`dx`)
- Use implicit time-stepping

**Example**:
```cpp
// If alpha = D * dt / dx^2 > 0.5, reduce dt:
double dt = 0.4 * dx * dx / D;  // CFL-safe timestep
```

---

## Next Steps

### 🎓 Learn More

1. **Tutorials**: See [TUTORIALS.md](TUTORIALS.md) for step-by-step guides
2. **API Reference**: Check [API_REFERENCE.md](API_REFERENCE.md) for detailed API docs
3. **Examples**: Explore `examples/` directory for complete examples
4. **Jupyter Notebooks**: Run interactive tutorials in `notebooks/`

### 🚀 Advanced Topics

- **GPU Acceleration**: See Phase 61-65 documentation
- **MPI Parallelization**: See Phase 41-45 documentation
- **Multi-Physics Coupling**: See Phase 66-70 documentation

### 📊 Benchmark Your System

```bash
cd build

# Run diffusion example with timing
time ./examples/diffusion_example

# Run full simulation example
./examples/full_simulation_example
```

### 🤝 Get Help

- **Documentation**: Check `docs/` directory
- **Issues**: Report bugs on GitHub Issues
- **Discussions**: Ask questions in GitHub Discussions

---

## Quick Reference

### Common CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `USE_EIGEN` | `ON` | Use Eigen for linear algebra |
| `USE_SPDLOG` | `OFF` | Use spdlog for logging |
| `USE_GPU` | `OFF` | Enable GPU support |
| `USE_CUDA` | `OFF` | Use CUDA for GPU |
| `USE_HIP` | `OFF` | Use HIP for AMD GPUs |
| `BUILD_TESTING` | `ON` | Build unit tests |
| `BUILD_PYTHON` | `ON` | Build Python bindings |
| `BUILD_EXAMPLES` | `ON` | Build example programs |

### Build Targets

| Target | Description |
|--------|-------------|
| `all` | Build everything |
| `_core` | Python bindings |
| `diffusion_example` | Simple diffusion example |
| `full_simulation_example` | Complete simulation |
| `test_phase66_70_simple` | Phase 66-70 tests |

---

## Success!

You're now ready to use KooChemicalSimulation! 🎉

Continue with [TUTORIALS.md](TUTORIALS.md) for hands-on learning.
