# KooChemicalSimulation

**Version**: 0.1.0-alpha1 (Phase 1)

A comprehensive open-source chemical simulation solution built on finite element method (FEM) solvers for modeling chemical reactions, diffusion processes, and surface chemistry phenomena.

## Overview

KooChemicalSimulation is designed to provide a robust, scalable, and user-friendly platform for simulating complex chemical systems including:

- **PDE-based chemical reactions**: Multi-species reaction systems
- **Diffusion equations**: Fick's law and multi-component diffusion
- **Surface chemistry**: Corrosion, chemical migration, and surface reactions
- **HPC support**: Parallel processing with MPI
- **Flexible I/O**: VTK-based output for visualization in ParaView

## Project Status

🚧 **Currently in Phase 1 of 50-phase development plan**

This phase establishes the project foundation:
- ✅ Directory structure
- ✅ Build system (CMake)
- ✅ Dependency management (vcpkg)
- ✅ Git configuration

See [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for the complete roadmap.

## Features (Planned)

### Current Version (v0.1.0)
- Project infrastructure setup
- Build system configuration

### Future Versions
- **v0.2.0**: gmsh mesh integration
- **v0.3.0**: PDE solver integration (NGSolve/MFEM)
- **v0.4.0**: Chemical reaction system
- **v0.5.0**: Diffusion solver
- **v1.0.0**: Surface chemistry (corrosion, migration)
- **v2.0.0**: Complete I/O system with VTK output
- **v3.0.0**: Configuration management
- **v4.0.0**: HPC and MPI support
- **v5.0.0**: Production release

## Architecture

```
KooChemicalSimulation/
├── core/          # Core abstractions and interfaces
├── mesh/          # Mesh management (gmsh integration)
├── solver/        # PDE solvers (NGSolve/MFEM)
├── chemistry/     # Chemical species and reactions
├── physics/       # Physical models (diffusion, transport, surface)
├── io/            # Input/output (VTK, HDF5)
├── config/        # Configuration management
├── parallel/      # HPC support (MPI)
├── utils/         # Utilities (logging, math, error handling)
└── apps/          # Applications (CLI, examples)
```

## Requirements

### Build Requirements
- **CMake** ≥ 3.20
- **C++17** compatible compiler (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 2019)
- **vcpkg** (recommended) or manual dependency installation

### Dependencies

#### Required
- [Eigen3](https://eigen.tuxfamily.org/) ≥ 3.4.0 - Linear algebra
- [fmt](https://fmt.dev/) ≥ 10.0.0 - Formatting library
- [spdlog](https://github.com/gabime/spdlog) ≥ 1.12.0 - Logging
- [nlohmann-json](https://github.com/nlohmann/json) ≥ 3.11.0 - JSON parsing
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) ≥ 0.8.0 - YAML parsing

#### Optional (for later phases)
- [VTK](https://vtk.org/) ≥ 9.2.0 - Visualization output
- [HDF5](https://www.hdfgroup.org/solutions/hdf5/) ≥ 1.14.0 - Checkpoint/restart
- [Google Test](https://github.com/google/googletest) ≥ 1.14.0 - Testing
- MPI implementation (OpenMPI or MPICH) - Parallel computing
- [gmsh](https://gmsh.info/) ≥ 4.10 - Mesh generation
- [NGSolve](https://ngsolve.org/) - FEM solver (optional)
- [MFEM](https://mfem.org/) - FEM solver (optional)

## Building from Source

### Option 1: Using vcpkg (Recommended)

```bash
# 1. Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # On Windows: bootstrap-vcpkg.bat
export VCPKG_ROOT=$(pwd)  # Add to your shell profile

# 2. Clone the repository
git clone https://github.com/squall321/KooChemicalSimulation.git
cd KooChemicalSimulation

# 3. Create build directory
mkdir build && cd build

# 4. Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# 5. Build
cmake --build . -j$(nproc)

# 6. Run tests (when available)
ctest --output-on-failure
```

### Option 2: Manual Dependency Management

```bash
# Install dependencies manually (example for Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
    cmake \
    g++ \
    libeigen3-dev \
    libfmt-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libyaml-cpp-dev

# Clone and build
git clone https://github.com/squall321/KooChemicalSimulation.git
cd KooChemicalSimulation
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## Build Options

Configure build options with `-D<OPTION>=ON/OFF`:

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | ON | Build shared libraries |
| `BUILD_TESTING` | ON | Build test suite |
| `BUILD_EXAMPLES` | ON | Build example applications |
| `BUILD_DOCUMENTATION` | OFF | Build Doxygen documentation |
| `ENABLE_MPI` | OFF | Enable MPI for parallel computing |
| `ENABLE_OPENMP` | ON | Enable OpenMP |
| `ENABLE_NGSOLVE` | OFF | Enable NGSolve solver (Phase 12+) |
| `ENABLE_MFEM` | OFF | Enable MFEM solver (Phase 13+) |
| `ENABLE_COVERAGE` | OFF | Enable code coverage |
| `ENABLE_SANITIZERS` | OFF | Enable address/UB sanitizers |

### Example: Debug Build with Sanitizers

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON \
    -DBUILD_TESTING=ON
```

## Usage (Planned for Future Versions)

```yaml
# example_simulation.yaml (v3.0.0+)
simulation:
  name: "Corrosion Simulation"
  type: "surface_chemistry"

mesh:
  file: "geometry.msh"

chemistry:
  species: [Fe2+, O2, OH-]
  reactions:
    - equation: "Fe -> Fe2+ + 2e-"
      rate: 1e-6

physics:
  diffusion:
    species: [Fe2+, O2, OH-]
  surface_reaction:
    type: "butler_volmer"

output:
  format: "vtk"
  path: "results/"
```

```bash
# Run simulation (future)
koo-sim run example_simulation.yaml
```

## Development

### Phase 1 Checklist (Current)

- [x] Project structure
- [x] CMake build system
- [x] vcpkg dependency management
- [x] Git configuration
- [ ] Verify build succeeds

### Contributing

This project is currently in early development. Contributions will be welcomed once the core architecture is established (v1.0.0+).

### Code Style

- C++17 standard
- Follow SOLID principles
- Use modern C++ features
- Document all public APIs with Doxygen comments

## Documentation

Full documentation will be available in future releases:
- **API Reference**: Auto-generated with Doxygen (Phase 47)
- **User Manual**: Comprehensive guide (Phase 47)
- **Tutorials**: Step-by-step examples (Phase 47-48)
- **Theory Documentation**: Mathematical background (Phase 47)

## Roadmap

See [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md) for the detailed 50-phase development plan.

**Estimated Timeline**: 18 months to v5.0.0 release

## License

[To be determined - consider MIT, Apache 2.0, or GPL]

## Contact

- **Repository**: https://github.com/squall321/KooChemicalSimulation
- **Issues**: https://github.com/squall321/KooChemicalSimulation/issues

## Acknowledgments

Built with:
- [Eigen](https://eigen.tuxfamily.org/) - Linear algebra
- [gmsh](https://gmsh.info/) - Mesh generation
- [NGSolve](https://ngsolve.org/) / [MFEM](https://mfem.org/) - FEM solvers
- [VTK](https://vtk.org/) - Visualization

---

**Status**: Phase 1 - Infrastructure Setup ✅
**Next**: Phase 2 - Core Abstractions
