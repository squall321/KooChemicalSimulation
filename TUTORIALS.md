# KooChemicalSimulation Tutorials

This document provides step-by-step tutorials for learning KooChemicalSimulation.

## Table of Contents

1. [Tutorial 1: Your First Diffusion Simulation](#tutorial-1-your-first-diffusion-simulation)
2. [Tutorial 2: Reaction-Diffusion Systems](#tutorial-2-reaction-diffusion-systems)
3. [Tutorial 3: Working with Meshes](#tutorial-3-working-with-meshes)
4. [Tutorial 4: Multi-Physics Coupling](#tutorial-4-multi-physics-coupling)
5. [Tutorial 5: GPU Acceleration](#tutorial-5-gpu-acceleration)
6. [Tutorial 6: Python Integration](#tutorial-6-python-integration)
7. [Tutorial 7: Visualization and Output](#tutorial-7-visualization-and-output)

---

## Tutorial 1: Your First Diffusion Simulation

**Goal**: Implement a simple 1D diffusion equation solver.

### Theory

The 1D diffusion equation is:

```
∂C/∂t = D ∂²C/∂x²
```

where:
- `C` = concentration
- `D` = diffusion coefficient
- `t` = time
- `x` = spatial coordinate

### Step 1: Setup

Create a new file `tutorial1_diffusion.cpp`:

```cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

int main() {
    // Domain parameters
    const int nx = 100;           // Number of grid points
    const double L = 1.0;         // Domain length
    const double dx = L / (nx - 1); // Grid spacing

    // Physical parameters
    const double D = 0.01;        // Diffusion coefficient
    const double dt = 0.001;      // Time step
    const int num_steps = 1000;   // Number of time steps

    // Compute CFL number
    const double alpha = D * dt / (dx * dx);
    std::cout << "CFL number (alpha): " << alpha << std::endl;

    if (alpha > 0.5) {
        std::cerr << "WARNING: CFL condition violated!" << std::endl;
        std::cerr << "Reduce dt or increase dx for stability." << std::endl;
        return 1;
    }

    return 0;
}
```

### Step 2: Initialize Data

Add initialization code:

```cpp
    // Initialize arrays
    std::vector<double> x(nx);
    std::vector<double> C(nx);
    std::vector<double> C_new(nx);

    // Setup grid
    for (int i = 0; i < nx; ++i) {
        x[i] = i * dx;
    }

    // Initial condition: Gaussian pulse
    const double x0 = 0.5;        // Center position
    const double sigma = 0.05;    // Width

    for (int i = 0; i < nx; ++i) {
        double dist = x[i] - x0;
        C[i] = std::exp(-dist * dist / (2.0 * sigma * sigma));
    }

    std::cout << "Initial mass: " << computeMass(C, dx) << std::endl;
```

Add helper function before `main()`:

```cpp
double computeMass(const std::vector<double>& C, double dx) {
    double mass = 0.0;
    for (size_t i = 0; i < C.size(); ++i) {
        mass += C[i] * dx;
    }
    return mass;
}
```

### Step 3: Time-Stepping Loop

```cpp
    // Time-stepping with explicit Euler
    for (int step = 0; step < num_steps; ++step) {
        // Update interior points
        for (int i = 1; i < nx - 1; ++i) {
            C_new[i] = C[i] + alpha * (C[i+1] - 2.0*C[i] + C[i-1]);
        }

        // Boundary conditions (Neumann: zero flux)
        C_new[0] = C_new[1];
        C_new[nx-1] = C_new[nx-2];

        // Update C
        C = C_new;

        // Print progress
        if ((step + 1) % 100 == 0) {
            std::cout << "Step " << (step + 1) << "/" << num_steps
                      << ", time = " << (step + 1) * dt << std::endl;
        }
    }

    std::cout << "Final mass: " << computeMass(C, dx) << std::endl;
```

### Step 4: Output Results

```cpp
    // Write results to file
    std::ofstream outfile("diffusion_result.dat");
    outfile << "# x C\n";
    for (int i = 0; i < nx; ++i) {
        outfile << x[i] << " " << C[i] << "\n";
    }
    outfile.close();

    std::cout << "Results written to diffusion_result.dat" << std::endl;
```

### Step 5: Compile and Run

```bash
g++ -std=c++17 -O2 tutorial1_diffusion.cpp -o tutorial1
./tutorial1
```

### Step 6: Visualize with Python

Create `plot_diffusion.py`:

```python
import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt('diffusion_result.dat')
x = data[:, 0]
C = data[:, 1]

plt.figure(figsize=(10, 6))
plt.plot(x, C, 'b-', linewidth=2)
plt.xlabel('Position x')
plt.ylabel('Concentration C')
plt.title('1D Diffusion Result')
plt.grid(True, alpha=0.3)
plt.savefig('diffusion_result.png', dpi=150)
plt.show()
```

Run:
```bash
python3 plot_diffusion.py
```

### Expected Results

- Initial Gaussian pulse spreads out over time
- Total mass is conserved (within numerical error)
- Peak concentration decreases as distribution widens

### Exercise

1. Try different values of `D` (e.g., 0.001, 0.05, 0.1)
2. Change initial condition to a square pulse
3. Implement Dirichlet boundary conditions (fixed values)
4. Add multiple pulses and observe interaction

---

## Tutorial 2: Reaction-Diffusion Systems

**Goal**: Simulate a reaction-diffusion system with two species.

### Theory

The Brusselator model:

```
∂u/∂t = D_u ∇²u + a - (b+1)u + u²v
∂v/∂t = D_v ∇²v + bu - u²v
```

### Implementation

```cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

class Brusselator2D {
private:
    int nx, ny;
    double dx, dy, dt;
    double Du, Dv, a, b;
    std::vector<std::vector<double>> u, v;
    std::vector<std::vector<double>> u_new, v_new;

public:
    Brusselator2D(int nx_, int ny_, double L, double dt_,
                  double Du_, double Dv_, double a_, double b_)
        : nx(nx_), ny(ny_), dt(dt_), Du(Du_), Dv(Dv_), a(a_), b(b_)
    {
        dx = L / (nx - 1);
        dy = L / (ny - 1);

        // Initialize arrays
        u.resize(nx, std::vector<double>(ny));
        v.resize(nx, std::vector<double>(ny));
        u_new.resize(nx, std::vector<double>(ny));
        v_new.resize(nx, std::vector<double>(ny));

        // Initial conditions: uniform + random perturbation
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                u[i][j] = a + 0.1 * (rand() / (double)RAND_MAX - 0.5);
                v[i][j] = b/a + 0.1 * (rand() / (double)RAND_MAX - 0.5);
            }
        }
    }

    double laplacian_u(int i, int j) {
        double lap = (u[i+1][j] + u[i-1][j] - 2*u[i][j]) / (dx*dx)
                   + (u[i][j+1] + u[i][j-1] - 2*u[i][j]) / (dy*dy);
        return lap;
    }

    double laplacian_v(int i, int j) {
        double lap = (v[i+1][j] + v[i-1][j] - 2*v[i][j]) / (dx*dx)
                   + (v[i][j+1] + v[i][j-1] - 2*v[i][j]) / (dy*dy);
        return lap;
    }

    void step() {
        // Update interior points
        for (int i = 1; i < nx-1; ++i) {
            for (int j = 1; j < ny-1; ++j) {
                double uuv = u[i][j] * u[i][j] * v[i][j];

                double du = Du * laplacian_u(i, j)
                          + a - (b+1)*u[i][j] + uuv;
                double dv = Dv * laplacian_v(i, j)
                          + b*u[i][j] - uuv;

                u_new[i][j] = u[i][j] + dt * du;
                v_new[i][j] = v[i][j] + dt * dv;
            }
        }

        // Periodic boundary conditions
        for (int i = 0; i < nx; ++i) {
            u_new[i][0] = u_new[i][ny-2];
            u_new[i][ny-1] = u_new[i][1];
            v_new[i][0] = v_new[i][ny-2];
            v_new[i][ny-1] = v_new[i][1];
        }
        for (int j = 0; j < ny; ++j) {
            u_new[0][j] = u_new[nx-2][j];
            u_new[nx-1][j] = u_new[1][j];
            v_new[0][j] = v_new[nx-2][j];
            v_new[nx-1][j] = v_new[1][j];
        }

        u = u_new;
        v = v_new;
    }

    void save(const std::string& filename) {
        std::ofstream file(filename);
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                file << u[i][j] << " ";
            }
            file << "\n";
        }
        file.close();
    }
};

int main() {
    // Parameters
    const int nx = 128, ny = 128;
    const double L = 2.5;
    const double dt = 0.01;
    const double Du = 0.5, Dv = 1.0;
    const double a = 1.0, b = 3.0;
    const int num_steps = 5000;

    Brusselator2D sim(nx, ny, L, dt, Du, Dv, a, b);

    std::cout << "Running Brusselator simulation..." << std::endl;

    for (int step = 0; step < num_steps; ++step) {
        sim.step();

        if ((step + 1) % 500 == 0) {
            std::cout << "Step " << (step + 1) << "/" << num_steps << std::endl;
            sim.save("brusselator_" + std::to_string(step + 1) + ".dat");
        }
    }

    std::cout << "Simulation complete!" << std::endl;
    return 0;
}
```

### Visualization

```python
import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt('brusselator_5000.dat')

plt.figure(figsize=(10, 10))
plt.imshow(data, cmap='RdYlBu_r', interpolation='bilinear')
plt.colorbar(label='Concentration u')
plt.title('Brusselator Pattern Formation')
plt.axis('off')
plt.savefig('brusselator_pattern.png', dpi=200, bbox_inches='tight')
plt.show()
```

---

## Tutorial 3: Working with Meshes

**Goal**: Create and manipulate structured and unstructured meshes.

### Using KooLab Mesh API

```cpp
#include "mesh/core/MeshData.h"
#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "utils/logger/Logger.h"
#include <iostream>

using namespace koo;

int main() {
    // Initialize logger
    utils::logger::Logger::initialize("MeshTutorial");
    utils::logger::Logger::setLevel(utils::logger::LogLevel::INFO);

    // Create mesh
    mesh::core::MeshData mesh;

    // Add nodes for a 3x3 grid
    int node_id = 0;
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            double x = i * 0.33;
            double y = j * 0.33;
            mesh.addNode(mesh::core::Node(node_id++, x, y, 0.0));
        }
    }

    utils::logger::Logger::info("Added " + std::to_string(mesh.getNumNodes()) + " nodes");

    // Add quadrilateral elements
    int elem_id = 0;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < 3; ++i) {
            size_t n0 = j * 4 + i;
            size_t n1 = n0 + 1;
            size_t n2 = n0 + 5;
            size_t n3 = n0 + 4;

            std::vector<size_t> nodeIds = {n0, n1, n2, n3};
            mesh.addElement(mesh::core::Element(elem_id++,
                           core::ElementType::QUADRILATERAL,
                           nodeIds));
        }
    }

    utils::logger::Logger::info("Added " + std::to_string(mesh.getNumElements()) + " elements");

    // Access and print mesh data
    std::cout << "\nFirst 5 nodes:\n";
    for (size_t i = 0; i < 5; ++i) {
        const auto* node = mesh.getNode(i);
        if (node) {
            std::cout << "Node " << node->getId()
                      << ": (" << node->getX()
                      << ", " << node->getY() << ")\n";
        }
    }

    std::cout << "\nFirst element:\n";
    const auto* elem = mesh.getElement(0);
    if (elem) {
        std::cout << "Element " << elem->getId()
                  << " has " << elem->getNumNodes() << " nodes: ";
        auto nodeIds = elem->getNodeIds();
        for (size_t nid : nodeIds) {
            std::cout << nid << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
```

Compile:
```bash
cd build
cmake --build . --target mesh_tutorial
./mesh_tutorial
```

---

## Tutorial 4: Multi-Physics Coupling

**Goal**: Couple thermal and chemical equations.

### Theory

Heat equation:
```
ρc_p ∂T/∂t = k∇²T + Q_reaction
```

Chemical equation with temperature dependence:
```
∂C/∂t = D∇²C - k(T)C
where k(T) = A exp(-E_a/RT)
```

### Implementation Sketch

```cpp
class ThermalChemicalCoupling {
private:
    std::vector<double> T;  // Temperature
    std::vector<double> C;  // Concentration
    double k_thermal;       // Thermal conductivity
    double rho_cp;          // Heat capacity
    double D_chemical;      // Diffusion coefficient
    double A;               // Pre-exponential factor
    double E_a;             // Activation energy
    double R = 8.314;       // Gas constant

public:
    void step(double dt) {
        // 1. Update chemistry with current temperature
        for (size_t i = 0; i < C.size(); ++i) {
            double k_reaction = A * exp(-E_a / (R * T[i]));
            double dC = D_chemical * laplacian_C(i) - k_reaction * C[i];
            C[i] += dt * dC;
        }

        // 2. Compute heat release from reaction
        std::vector<double> Q_reaction(C.size());
        for (size_t i = 0; i < C.size(); ++i) {
            double k_reaction = A * exp(-E_a / (R * T[i]));
            Q_reaction[i] = -deltaH * k_reaction * C[i];  // Exothermic
        }

        // 3. Update temperature with heat release
        for (size_t i = 0; i < T.size(); ++i) {
            double dT = (k_thermal / rho_cp) * laplacian_T(i)
                      + Q_reaction[i] / rho_cp;
            T[i] += dt * dT;
        }
    }
};
```

---

## Tutorial 5: GPU Acceleration

**Goal**: Run simulations on GPU for 10-100x speedup.

### Prerequisites

- CUDA Toolkit 11.0+ or ROCm 4.0+
- KooLab built with `-DUSE_GPU=ON`

### Example (Conceptual)

```cpp
#include "gpu/Device.h"
#include "gpu/kernels/DiffusionKernel.h"

int main() {
    using namespace koo::gpu;

    // Initialize GPU
    Device device(0);  // Use GPU 0
    std::cout << device.getProperties().toString() << std::endl;

    // Allocate GPU memory
    const size_t N = 1024 * 1024;
    auto u_gpu = device.allocate<double>(N);
    auto u_new_gpu = device.allocate<double>(N);

    // Copy data to GPU
    std::vector<double> u_cpu(N, 1.0);
    device.copyToDevice(u_cpu.data(), u_gpu, N);

    // Run kernel
    DiffusionKernel kernel(device);
    kernel.setParameters(D, dt, dx);

    for (int step = 0; step < 1000; ++step) {
        kernel.execute(u_gpu, u_new_gpu, N);
        std::swap(u_gpu, u_new_gpu);
    }

    // Copy result back
    device.copyToHost(u_gpu, u_cpu.data(), N);

    return 0;
}
```

---

## Tutorial 6: Python Integration

**Goal**: Use KooLab from Python with full interoperability.

### Basic Usage

```python
import sys
sys.path.insert(0, 'build')
import _core as koo
import numpy as np
import matplotlib.pyplot as plt

# Initialize
koo.Logger.initialize("PythonTutorial")
koo.Logger.set_level(koo.LogLevel.INFO)

# Create mesh
mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 20, 20)
print(f"Mesh: {mesh}")

# Extract node coordinates for visualization
coords = []
for i in range(mesh.get_num_nodes()):
    node = mesh.get_node(i)
    if node:
        coords.append([node.x, node.y])

coords = np.array(coords)

# Plot
plt.figure(figsize=(8, 8))
plt.scatter(coords[:, 0], coords[:, 1], s=20)
plt.xlabel('X')
plt.ylabel('Y')
plt.title('2D Mesh')
plt.axis('equal')
plt.grid(True)
plt.show()
```

### NumPy Integration

```python
# Simulate diffusion with NumPy
def simulate_diffusion(nx=100, steps=1000):
    D = 0.01
    dt = 0.001
    dx = 1.0 / (nx - 1)
    alpha = D * dt / dx**2

    # Initial condition
    x = np.linspace(0, 1, nx)
    C = np.exp(-((x - 0.5)**2) / (2 * 0.05**2))

    # Time stepping
    for _ in range(steps):
        C[1:-1] += alpha * (C[2:] - 2*C[1:-1] + C[:-2])

    return x, C

x, C = simulate_diffusion()

plt.plot(x, C, linewidth=2)
plt.xlabel('Position')
plt.ylabel('Concentration')
plt.title('1D Diffusion Result')
plt.grid(True)
plt.show()
```

---

## Tutorial 7: Visualization and Output

**Goal**: Generate publication-quality visualizations.

### VTK Output

```cpp
#include "io/VTKWriter.h"

void writeVTK(const MeshData& mesh, const std::vector<double>& data,
              const std::string& filename) {
    io::VTKWriter writer;
    writer.setMesh(mesh);
    writer.addScalarField("concentration", data);
    writer.write(filename);
}
```

### Matplotlib Visualization

```python
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Load time series data
def load_snapshot(step):
    return np.loadtxt(f'snapshot_{step:04d}.dat')

# Create animation
fig, ax = plt.subplots(figsize=(10, 8))

def animate(frame):
    ax.clear()
    data = load_snapshot(frame * 10)
    im = ax.imshow(data, cmap='hot', vmin=0, vmax=1)
    ax.set_title(f'Time step: {frame * 10}')
    return [im]

anim = FuncAnimation(fig, animate, frames=100, interval=50)
anim.save('simulation.mp4', writer='ffmpeg', fps=20)
plt.show()
```

---

## Next Steps

1. ✅ Complete all tutorials above
2. 📚 Read [API_REFERENCE.md](API_REFERENCE.md) for detailed API documentation
3. 🚀 Explore example programs in `examples/` directory
4. 💻 Try Jupyter notebooks in `notebooks/` directory
5. 🔬 Build your own simulation!

## Need Help?

- Check [GETTING_STARTED.md](GETTING_STARTED.md) for installation issues
- See [README.md](README.md) for project overview
- Open an issue on GitHub for bugs or questions

Happy simulating! 🎉
