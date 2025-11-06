"""
KooChemicalSimulation Python Interface
=======================================

High-performance chemical simulation framework with Python bindings.

Modules
-------
- core: Core types and utilities
- mesh: Mesh management
- chemistry: Chemical species and reactions
- gpu: GPU acceleration

Example
-------
>>> import koolab as koo
>>> print(koo.__version__)
6.0.0-alpha2

>>> # Check GPU availability
>>> print(koo.gpu.device_count())
0

>>> # Create a simple mesh
>>> mesh = koo.mesh.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
>>> print(mesh)
<MeshData nodes=121 elements=100>

>>> # Create chemical species
>>> h2 = koo.Species("H2")
>>> o2 = koo.Species("O2")
>>> h2o = koo.Species("H2O")

>>> # Create reaction
>>> rxn = koo.Reaction()
>>> rxn.add_reactant("H2", 2.0)
>>> rxn.add_reactant("O2", 1.0)
>>> rxn.add_product("H2O", 2.0)
>>> print(rxn)
<Reaction 2.0 H2 + O2 => 2.0 H2O>

Authors: KooChemicalSimulation Development Team
Version: 6.0.0-alpha2
Phase: 56 - Core Python Interface
"""

__version__ = "6.0.0-alpha2"
__author__ = "KooChemicalSimulation Development Team"

# Import C++ extension module
try:
    from ._core import *
    from ._core import core, mesh, chemistry, gpu

    # Import version info
    __version__ = _core.__version__

    # Convenience imports - bring common classes to top level
    from ._core.core import Vector, Logger, LogLevel
    from ._core.mesh import Node, Element, MeshData, ElementType, create_rectangular_mesh
    from ._core.chemistry import Species, Reaction, ArrheniusRate, parse_reaction
    from ._core.gpu import Device, MultiGPUManager, DeviceProperties

    _cpp_available = True

except ImportError as e:
    import warnings
    warnings.warn(f"Could not import C++ extension: {e}. "
                  "Only pure Python functionality will be available.")
    _cpp_available = False

    # Define minimal fallback
    core = None
    mesh = None
    chemistry = None
    gpu = None


def get_info():
    """
    Get package information.

    Returns
    -------
    dict
        Package information including version, build configuration, etc.
    """
    info = {
        "version": __version__,
        "author": __author__,
        "cpp_available": _cpp_available,
    }

    if _cpp_available:
        try:
            runtime_info = get_runtime_info()
            info.update(runtime_info)
        except:
            pass

    return info


def print_info():
    """Print package information."""
    print("=" * 50)
    print("KooChemicalSimulation Python Interface")
    print("=" * 50)

    info = get_info()
    for key, value in info.items():
        print(f"{key:20s}: {value}")

    if _cpp_available and gpu is not None:
        print("\nGPU Information:")
        print(f"  Device count: {gpu.device_count()}")
        print(f"  Runtime: {Device.get_runtime()}")

    print("=" * 50)


# Module-level convenience functions
def hello():
    """Print hello message."""
    print(f"KooChemicalSimulation v{__version__}")
    print("High-performance chemical simulation framework")
    if _cpp_available:
        print("C++ extension loaded successfully ✓")
    else:
        print("C++ extension not available ✗")


__all__ = [
    # Version
    "__version__",
    "__author__",

    # Modules
    "core",
    "mesh",
    "chemistry",
    "gpu",

    # Core classes
    "Vector",
    "Logger",
    "LogLevel",

    # Mesh classes
    "Node",
    "Element",
    "MeshData",
    "ElementType",
    "create_rectangular_mesh",

    # Chemistry classes
    "Species",
    "Reaction",
    "ArrheniusRate",
    "parse_reaction",

    # GPU classes
    "Device",
    "MultiGPUManager",
    "DeviceProperties",

    # Utility functions
    "get_info",
    "print_info",
    "hello",
]
