# Conan Package Manager Setup

KooChemicalSimulation uses Conan 2.0 for dependency management.

## Quick Start

### 1. Install Conan

```bash
pip install conan>=2.0
```

### 2. Configure Conan Profile

```bash
conan profile detect --force
```

### 3. Install Dependencies

```bash
conan install . --output-folder=build --build=missing
```

### 4. Build with Conan

```bash
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Available Options

The package supports various build options:

| Option | Default | Description |
|--------|---------|-------------|
| `shared` | False | Build shared libraries |
| `with_mpi` | False | Enable MPI support |
| `with_openmp` | True | Enable OpenMP support |
| `with_python` | True | Build Python bindings |
| `with_gpu` | False | Enable GPU support (CUDA/HIP) |
| `build_tests` | False | Build unit tests |
| `build_examples` | False | Build examples |

## Custom Build Options

```bash
# Build with MPI support
conan install . -o with_mpi=True --output-folder=build --build=missing

# Build with GPU support
conan install . -o with_gpu=True --output-folder=build --build=missing

# Build with tests
conan install . -o build_tests=True --output-folder=build --build=missing
```

## Creating a Conan Package

```bash
# Create package locally
conan create . --build=missing

# Export to local cache
conan export-pkg .
```

## Using as a Dependency

Add to your `conanfile.txt` or `conanfile.py`:

```python
[requires]
koolab/6.0.0-alpha5

[generators]
CMakeDeps
CMakeToolchain
```

In your `CMakeLists.txt`:

```cmake
find_package(KooLab REQUIRED)
target_link_libraries(your_target KooLab::KooLab)
```

## Troubleshooting

### Missing Dependencies

If dependencies cannot be found:

```bash
# Add remotes
conan remote add conancenter https://center.conan.io
conan remote list
```

### Build Errors

```bash
# Clean and rebuild
rm -rf build
conan install . --output-folder=build --build=missing
```

### Compiler Issues

```bash
# Update profile
conan profile detect --force
conan profile show default
```

## Dependencies

Core dependencies (automatically installed):
- Eigen 3.4.0
- fmt 10.1.1
- spdlog 1.12.0
- nlohmann_json 3.11.2
- yaml-cpp 0.8.0

Optional dependencies:
- pybind11 2.11.1 (if `with_python=True`)
- gtest 1.14.0 (if `build_tests=True`)

## More Information

See [CONAN_GUIDE.md](CONAN_GUIDE.md) for detailed instructions.
