"""
KooChemicalSimulation Python Package Setup
Phase 56: Core Python Interface
"""

import os
import sys
import subprocess
from pathlib import Path

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext

__version__ = "6.0.0-alpha2"

class CMakeExtension(Extension):
    """Custom extension for CMake-based builds"""
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """Custom build command using CMake"""

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        # CMake configuration arguments
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            "-DENABLE_PYTHON=ON",
            "-DCMAKE_BUILD_TYPE=Release",
        ]

        # Build arguments
        build_args = ["--config", "Release"]

        # Parallel build
        if hasattr(self, "parallel") and self.parallel:
            build_args += ["-j", str(self.parallel)]
        else:
            build_args += ["-j4"]

        # Create build directory
        build_temp = Path(self.build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)

        # Run CMake configure
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args,
            cwd=self.build_temp
        )

        # Run CMake build
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args,
            cwd=self.build_temp
        )


# Read long description from README
long_description = """
KooChemicalSimulation - High-Performance Chemical Simulation Framework

A modern C++ framework for chemical kinetics, reaction-diffusion systems,
surface chemistry, and GPU-accelerated simulations with Python bindings.

Features:
- Chemical kinetics and reaction networks
- PDE solvers for diffusion and transport
- Surface chemistry and catalysis
- GPU acceleration (CUDA/HIP)
- MPI + OpenMP parallelization
- Python API for rapid prototyping
"""

setup(
    name="koolab",
    version=__version__,
    author="KooChemicalSimulation Development Team",
    author_email="",
    description="High-performance chemical simulation framework",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/squall321/KooChemicalSimulation",
    packages=find_packages(),
    package_dir={"": "."},
    ext_modules=[CMakeExtension("koolab._core")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    python_requires=">=3.7",
    install_requires=[
        "numpy>=1.18.0",
    ],
    extras_require={
        "viz": ["matplotlib>=3.3.0"],
        "jupyter": ["jupyter>=1.0.0", "ipywidgets>=7.6.0"],
        "dev": ["pytest>=6.0.0", "pytest-cov>=2.10.0"],
    },
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Science/Research",
        "Topic :: Scientific/Engineering :: Chemistry",
        "Topic :: Scientific/Engineering :: Physics",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: C++",
    ],
    keywords="chemistry simulation kinetics pde gpu cuda parallel",
    project_urls={
        "Documentation": "https://github.com/squall321/KooChemicalSimulation",
        "Source": "https://github.com/squall321/KooChemicalSimulation",
        "Tracker": "https://github.com/squall321/KooChemicalSimulation/issues",
    },
)
