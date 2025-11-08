# Conan Package Manager Guide

This guide explains how to use Conan to install and manage KooChemicalSimulation and its dependencies.

---

## 📦 What is Conan?

[Conan](https://conan.io/) is a C/C++ package manager that simplifies dependency management. With Conan, you can:
- Install KooLab with one command
- Automatically handle all dependencies (Eigen, fmt, spdlog, etc.)
- Build for different configurations (Debug/Release, shared/static)
- Create reproducible builds

---

## 🚀 Quick Start

### Install Conan

```bash
# Using pip (recommended)
pip install conan

# Or using Homebrew (macOS)
brew install conan

# Verify installation
conan --version
```

### Install KooLab (once published)

```bash
# Install latest version
conan install koolab/6.0.0-alpha5@

# Or create a conanfile.txt
cat > conanfile.txt << EOF
[requires]
koolab/6.0.0-alpha5

[generators]
CMakeDeps
CMakeToolchain

[options]
koolab:with_python=True
koolab:with_openmp=True
EOF

# Install dependencies
conan install . --build=missing
```

---

## 🏗️ Building KooLab from Source with Conan

### Method 1: Local Build

```bash
# Clone repository
git clone https://github.com/squall321/KooChemicalSimulation.git
cd KooChemicalSimulation

# Create Conan package locally
conan create . --build=missing

# Test the package
conan create . --build=missing -o with_python=True -o build_tests=True
```

### Method 2: Editable Mode (for development)

```bash
# Clone repository
git clone https://github.com/squall321/KooChemicalSimulation.git
cd KooChemicalSimulation

# Install dependencies
conan install . --build=missing -of build

# Build with CMake
cmake -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build

# Make package editable
conan editable add . koolab/6.0.0-alpha5@
```

---

## ⚙️ Package Options

Customize KooLab build with these options:

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `shared` | True/False | False | Build shared libraries |
| `fPIC` | True/False | True | Position-independent code (Linux/macOS) |
| `with_mpi` | True/False | False | Enable MPI support |
| `with_openmp` | True/False | True | Enable OpenMP parallelization |
| `with_python` | True/False | True | Build Python bindings |
| `with_gpu` | True/False | False | Enable GPU support (requires CUDA) |
| `build_tests` | True/False | False | Build test suite |
| `build_examples` | True/False | False | Build example programs |

### Examples

```bash
# Python bindings + OpenMP
conan create . -o with_python=True -o with_openmp=True

# Shared library with GPU support
conan create . -o shared=True -o with_gpu=True

# Full build with tests and examples
conan create . -o build_tests=True -o build_examples=True

# Minimal build (no Python, no tests)
conan create . -o with_python=False -o build_tests=False
```

---

## 🔧 Using KooLab in Your Project

### CMake Project

**Step 1**: Create `conanfile.txt`

```ini
[requires]
koolab/6.0.0-alpha5

[generators]
CMakeDeps
CMakeToolchain

[options]
koolab:with_python=True
```

**Step 2**: Install dependencies

```bash
conan install . --build=missing -of build
```

**Step 3**: Update your `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject CXX)

# Find KooLab
find_package(KooLab REQUIRED)

# Create executable
add_executable(my_simulation main.cpp)

# Link against KooLab
target_link_libraries(my_simulation PRIVATE KooLab::KooLab)

# Set C++ standard
target_compile_features(my_simulation PRIVATE cxx_std_17)
```

**Step 4**: Build

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build
```

### Conanfile.py (Advanced)

For more control, use `conanfile.py`:

```python
from conan import ConanFile

class MyProjectConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("koolab/6.0.0-alpha5")

    def configure(self):
        # Force specific options
        self.options["koolab"].with_python = True
        self.options["koolab"].with_openmp = True
```

---

## 📊 Profile Configuration

Create custom profiles for different build configurations:

### Debug Profile

```bash
# Create debug profile
cat > ~/.conan2/profiles/debug << EOF
[settings]
os=Linux
arch=x86_64
compiler=gcc
compiler.version=11
compiler.libcxx=libstdc++11
build_type=Debug

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
EOF

# Use it
conan create . --profile=debug
```

### Release Profile

```bash
# Create release profile
cat > ~/.conan2/profiles/release << EOF
[settings]
os=Linux
arch=x86_64
compiler=gcc
compiler.version=11
compiler.libcxx=libstdc++11
build_type=Release

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
EOF

# Use it
conan create . --profile=release
```

---

## 🐍 Python Integration

When `with_python=True`:

```bash
# Install with Python support
conan install . -o with_python=True

# Python package will be available
python3 -c "import _core as koo; print('KooLab imported successfully')"
```

---

## 🧪 Testing the Package

The `test_package` ensures the package works correctly:

```bash
# Automatic test during creation
conan create . --build=missing

# Manual test
cd test_package
conan test . koolab/6.0.0-alpha5@
```

**Expected output:**
```
KooLab Chemical Simulation Library
Version: 6.0.0-alpha5

✓ Python bindings enabled
○ GPU support disabled

Test package compiled and linked successfully!
KooLab is ready to use.
```

---

## 🔍 Troubleshooting

### Problem: "Package not found"

**Solution**: Build it locally first:

```bash
conan create . --build=missing
```

### Problem: Dependency conflicts

**Solution**: Update all dependencies:

```bash
conan install . --build=missing --update
```

### Problem: Compiler mismatch

**Solution**: Detect default profile:

```bash
conan profile detect --force
conan create . --build=missing
```

### Problem: GPU support not working

**Solution**: Ensure CUDA is installed and use GPU option:

```bash
conan create . -o with_gpu=True --build=missing
```

---

## 📚 Conan Commands Cheat Sheet

```bash
# Create package from source
conan create .

# Install dependencies only
conan install .

# List installed packages
conan list "*"

# Remove package
conan remove koolab/6.0.0-alpha5

# Show package info
conan inspect koolab/6.0.0-alpha5

# Upload to remote (Conan Center, Artifactory, etc.)
conan upload koolab/6.0.0-alpha5 -r=conan-center

# Search packages
conan search koolab

# Create from specific commit
conan create . koolab/6.0.0-alpha5@user/channel
```

---

## 🌐 Publishing to Conan Center

To make KooLab available globally:

1. **Fork conan-center-index**:
   ```bash
   git clone https://github.com/conan-io/conan-center-index.git
   cd conan-center-index
   ```

2. **Add recipe**:
   ```bash
   mkdir -p recipes/koolab/all
   cp /path/to/conanfile.py recipes/koolab/all/
   ```

3. **Test locally**:
   ```bash
   conan create recipes/koolab/all/conanfile.py --version=6.0.0-alpha5
   ```

4. **Submit PR**:
   - Follow [contributing guidelines](https://github.com/conan-io/conan-center-index/blob/master/docs/how_to_add_packages.md)
   - Wait for CI to pass
   - Address reviewer feedback

---

## 🔗 Integration with Other Tools

### vcpkg → Conan Migration

```bash
# Old (vcpkg)
vcpkg install eigen3 fmt spdlog

# New (Conan)
conan install . --build=missing
```

### CMake FetchContent → Conan

```cmake
# Old (FetchContent)
include(FetchContent)
FetchContent_Declare(koolab GIT_REPOSITORY ...)
FetchContent_MakeAvailable(koolab)

# New (Conan)
find_package(KooLab REQUIRED)
target_link_libraries(myapp PRIVATE KooLab::KooLab)
```

---

## 📖 Additional Resources

- **Conan Documentation**: https://docs.conan.io/
- **Conan Center**: https://conan.io/center/
- **KooLab Repository**: https://github.com/squall321/KooChemicalSimulation
- **Issue Tracker**: https://github.com/squall321/KooChemicalSimulation/issues

---

## ✅ Quick Reference

```bash
# Install KooLab
conan create . --build=missing

# With options
conan create . -o with_python=True -o with_gpu=True

# Install dependencies for development
conan install . --build=missing -of build

# Build with CMake
cmake -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build

# Test package
conan test test_package koolab/6.0.0-alpha5@

# Clean Conan cache
conan remove "*" --confirm
```

---

**Happy packaging!** 📦
