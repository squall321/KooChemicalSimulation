/**
 * @file bindings.cpp
 * @brief Main pybind11 module for KooChemicalSimulation
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 56: Core Python Interface
 *
 * Main entry point for Python bindings using pybind11.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

// Forward declarations for submodule binding functions
void bind_core(py::module& m);
void bind_mesh(py::module& m);
void bind_chemistry(py::module& m);
void bind_gpu(py::module& m);

/**
 * @brief Main Python module definition
 *
 * Creates the koolab._core module and binds all submodules.
 */
PYBIND11_MODULE(_core, m) {
    m.doc() = R"pbdoc(
        KooChemicalSimulation Python Interface
        =======================================

        High-performance chemical simulation framework with Python bindings.

        .. currentmodule:: koolab

        .. autosummary::
           :toctree: _generate

        Submodules
        ----------
        - core: Core types and utilities
        - mesh: Mesh management and operations
        - chemistry: Chemical species and reactions
        - gpu: GPU acceleration

        Example
        -------
        >>> import koolab as koo
        >>> print(koo.__version__)
        6.0.0-alpha2
    )pbdoc";

    // Module version
    m.attr("__version__") = "6.0.0-alpha2";

    // Runtime information
    m.def("get_runtime_info", []() {
        py::dict info;
        info["version"] = "6.0.0-alpha2";
        info["build_type"] = "Release";

#ifdef KOO_CUDA_ENABLED
        info["cuda_enabled"] = true;
#else
        info["cuda_enabled"] = false;
#endif

#ifdef KOO_HIP_ENABLED
        info["hip_enabled"] = true;
#else
        info["hip_enabled"] = false;
#endif

#ifdef KOO_USE_MPI
        info["mpi_enabled"] = true;
#else
        info["mpi_enabled"] = false;
#endif

#ifdef _OPENMP
        info["openmp_enabled"] = true;
#else
        info["openmp_enabled"] = false;
#endif

        return info;
    }, R"pbdoc(
        Get runtime configuration information.

        Returns
        -------
        dict
            Dictionary containing build configuration
    )pbdoc");

    // Create submodules
    auto core_module = m.def_submodule("core", "Core types and utilities");
    auto mesh_module = m.def_submodule("mesh", "Mesh management");
    auto chemistry_module = m.def_submodule("chemistry", "Chemical species and reactions");
    auto gpu_module = m.def_submodule("gpu", "GPU acceleration");

    // Bind submodules
    bind_core(core_module);
    bind_mesh(mesh_module);
    bind_chemistry(chemistry_module);
    bind_gpu(gpu_module);
}
