from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
import os

class KooChemicalSimulationConan(ConanFile):
    name = "koolab"
    version = "6.0.0-alpha5"
    license = "MIT"
    author = "KooLab Development Team"
    url = "https://github.com/squall321/KooChemicalSimulation"
    description = "High-performance chemical simulation framework with GPU acceleration"
    topics = ("chemistry", "simulation", "gpu", "fem", "pde", "scientific-computing")

    # Package settings
    settings = "os", "compiler", "build_type", "arch"

    # Package options
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_mpi": [True, False],
        "with_openmp": [True, False],
        "with_python": [True, False],
        "with_gpu": [True, False],
        "build_tests": [True, False],
        "build_examples": [True, False],
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "with_mpi": False,
        "with_openmp": True,
        "with_python": True,
        "with_gpu": False,
        "build_tests": False,
        "build_examples": False,
    }

    # Sources are located in the same place as this recipe
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "tests/*",
        "examples/*",
        "benchmarks/*",
        "python/*",
        "LICENSE",
        "README.md",
    )

    def config_options(self):
        """Remove options that don't apply to certain platforms"""
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        """Configure options based on dependencies"""
        if self.options.shared:
            # Shared libraries don't need fPIC
            self.options.rm_safe("fPIC")

    def requirements(self):
        """Declare package dependencies"""
        # Core dependencies
        self.requires("eigen/3.4.0")
        self.requires("fmt/10.1.1")
        self.requires("spdlog/1.12.0")
        self.requires("nlohmann_json/3.11.2")
        self.requires("yaml-cpp/0.8.0")

        # Optional dependencies
        if self.options.with_python:
            self.requires("pybind11/2.11.1")

        if self.options.build_tests:
            self.requires("gtest/1.14.0")

    def build_requirements(self):
        """Build-time dependencies"""
        self.tool_requires("cmake/[>=3.20]")

    def layout(self):
        """Define project layout"""
        cmake_layout(self)

    def generate(self):
        """Generate build files"""
        # Generate CMake toolchain
        tc = CMakeToolchain(self)

        # Pass options to CMake
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["BUILD_TESTING"] = self.options.build_tests
        tc.variables["BUILD_EXAMPLES"] = self.options.build_examples
        tc.variables["ENABLE_MPI"] = self.options.with_mpi
        tc.variables["ENABLE_OPENMP"] = self.options.with_openmp
        tc.variables["BUILD_PYTHON_BINDINGS"] = self.options.with_python

        # GPU support (if CUDA available)
        if self.options.with_gpu:
            tc.variables["BUILD_GPU"] = True

        tc.generate()

        # Generate CMake dependencies
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        """Build the project"""
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        # Run tests if enabled
        if self.options.build_tests:
            cmake.test()

    def package(self):
        """Package the project files"""
        cmake = CMake(self)
        cmake.install()

        # Copy license
        copy(self, "LICENSE",
             src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))

        # Copy documentation
        copy(self, "*.md",
             src=self.source_folder,
             dst=os.path.join(self.package_folder, "docs"))

    def package_info(self):
        """Provide package information to consumers"""
        # Library names
        self.cpp_info.libs = ["koolab_core", "koolab_mesh", "koolab_chemistry",
                               "koolab_physics", "koolab_utils", "koolab_config"]

        # Include directories
        self.cpp_info.includedirs = ["include"]

        # C++ standard
        self.cpp_info.set_property("cmake_target_name", "KooLab::KooLab")

        # Define preprocessor macros
        if self.options.with_gpu:
            self.cpp_info.defines.append("KOOLAB_GPU_ENABLED")

        if self.options.with_python:
            self.cpp_info.defines.append("KOOLAB_PYTHON_ENABLED")

        # System libraries
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.extend(["pthread", "m", "dl"])

        # OpenMP linking
        if self.options.with_openmp:
            if self.settings.compiler in ["gcc", "clang"]:
                self.cpp_info.sharedlinkflags.append("-fopenmp")
                self.cpp_info.exelinkflags.append("-fopenmp")
