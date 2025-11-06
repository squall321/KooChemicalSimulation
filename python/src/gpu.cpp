/**
 * @file gpu.cpp
 * @brief Python bindings for GPU acceleration
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 56: Core Python Interface
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// GPU headers
#include "gpu/Device.h"
#include "gpu/parallel/MultiGPU.h"

namespace py = pybind11;

void bind_gpu(py::module& m) {
    m.doc() = "GPU acceleration";

    // ============================================
    // Device bindings
    // ============================================
    py::class_<koo::gpu::DeviceProperties>(m, "DeviceProperties")
        .def_readonly("name", &koo::gpu::DeviceProperties::name,
                     "Device name")
        .def_readonly("major", &koo::gpu::DeviceProperties::major,
                     "Compute capability major")
        .def_readonly("minor", &koo::gpu::DeviceProperties::minor,
                     "Compute capability minor")
        .def_readonly("total_memory", &koo::gpu::DeviceProperties::totalMemory,
                     "Total global memory (bytes)")
        .def_readonly("multiprocessor_count", &koo::gpu::DeviceProperties::multiProcessorCount,
                     "Number of multiprocessors")
        .def_readonly("max_threads_per_block", &koo::gpu::DeviceProperties::maxThreadsPerBlock,
                     "Max threads per block")

        .def("__repr__", [](const koo::gpu::DeviceProperties& props) {
            std::ostringstream oss;
            oss << "<DeviceProperties"
                << " name='" << props.name << "'"
                << " compute=" << props.major << "." << props.minor
                << " memory=" << (props.totalMemory / (1024.0*1024.0*1024.0)) << "GB"
                << ">";
            return oss.str();
        });

    py::class_<koo::gpu::Device>(m, "Device")
        .def_static("get_device_count", &koo::gpu::Device::getDeviceCount,
                   "Get number of available GPU devices")

        .def_static("is_gpu_available", &koo::gpu::Device::isGPUAvailable,
                   "Check if GPU is available")

        .def_static("get_runtime", &koo::gpu::Device::getRuntime,
                   "Get GPU runtime (CUDA/HIP/CPU)")

        .def_static("get_device", &koo::gpu::Device::getDevice,
                   "Get device by ID", py::arg("device_id") = 0,
                   py::return_value_policy::move)

        .def("get_id", &koo::gpu::Device::getId,
             "Get device ID")

        .def("get_name", &koo::gpu::Device::getName,
             "Get device name")

        .def("get_properties", &koo::gpu::Device::getProperties,
             "Get device properties")

        .def("set_current", &koo::gpu::Device::setCurrent,
             "Set this device as current")

        .def("synchronize", &koo::gpu::Device::synchronize,
             "Synchronize device")

        .def("__repr__", [](const koo::gpu::Device& dev) {
            return "<Device id=" + std::to_string(dev.getId()) +
                   " name='" + dev.getName() + "'>";
        });

    // ============================================
    // MultiGPU bindings
    // ============================================
    py::class_<koo::gpu::parallel::DomainPartition>(m, "DomainPartition")
        .def_readonly("gpu_id", &koo::gpu::parallel::DomainPartition::gpuId,
                     "GPU device ID")
        .def_readonly("start_index", &koo::gpu::parallel::DomainPartition::startIndex,
                     "Start index in global array")
        .def_readonly("end_index", &koo::gpu::parallel::DomainPartition::endIndex,
                     "End index in global array")
        .def_readonly("local_size", &koo::gpu::parallel::DomainPartition::localSize,
                     "Number of elements in partition")

        .def("total_size", &koo::gpu::parallel::DomainPartition::totalSize,
             "Get total size including halos")

        .def("is_valid", &koo::gpu::parallel::DomainPartition::isValid,
             "Check if partition is valid")

        .def("__repr__", [](const koo::gpu::parallel::DomainPartition& part) {
            std::ostringstream oss;
            oss << "<DomainPartition gpu=" << part.gpuId
                << " range=[" << part.startIndex << ", " << part.endIndex << ")"
                << " size=" << part.localSize << ">";
            return oss.str();
        });

    py::class_<koo::gpu::parallel::MultiGPUManager>(m, "MultiGPUManager")
        .def(py::init<>(), "Default constructor")

        .def("initialize", py::overload_cast<const std::vector<int>&>(
                &koo::gpu::parallel::MultiGPUManager::initialize),
             "Initialize multi-GPU system",
             py::arg("device_ids") = std::vector<int>{})

        .def("finalize", &koo::gpu::parallel::MultiGPUManager::finalize,
             "Finalize and cleanup")

        .def("get_num_gpus", &koo::gpu::parallel::MultiGPUManager::getNumGPUs,
             "Get number of managed GPUs")

        .def("is_initialized", &koo::gpu::parallel::MultiGPUManager::isInitialized,
             "Check if initialized")

        .def("synchronize_all", &koo::gpu::parallel::MultiGPUManager::synchronizeAll,
             "Synchronize all GPUs")

        .def("partition_1d", &koo::gpu::parallel::MultiGPUManager::partition1D,
             "Partition 1D domain across GPUs",
             py::arg("global_size"), py::arg("halo_size") = 0)

        .def("partition_2d", &koo::gpu::parallel::MultiGPUManager::partition2D,
             "Partition 2D domain across GPUs",
             py::arg("nx"), py::arg("ny"), py::arg("halo_size") = 0)

        .def("is_load_balanced", &koo::gpu::parallel::MultiGPUManager::isLoadBalanced,
             "Check if load is balanced",
             py::arg("threshold") = 0.2)

        .def("enable_peer_access", &koo::gpu::parallel::MultiGPUManager::enablePeerAccess,
             "Enable peer-to-peer access between GPUs")

        .def("print_info", &koo::gpu::parallel::MultiGPUManager::printInfo,
             "Print multi-GPU configuration")

        .def("__repr__", [](const koo::gpu::parallel::MultiGPUManager& mgr) {
            return "<MultiGPUManager num_gpus=" +
                   std::to_string(mgr.getNumGPUs()) +
                   " initialized=" + (mgr.isInitialized() ? "True" : "False") + ">";
        })

        .def("__enter__", [](koo::gpu::parallel::MultiGPUManager& mgr) -> koo::gpu::parallel::MultiGPUManager& {
            if (!mgr.isInitialized()) {
                mgr.initialize();
            }
            return mgr;
        }, py::return_value_policy::reference)

        .def("__exit__", [](koo::gpu::parallel::MultiGPUManager& mgr, py::object, py::object, py::object) {
            mgr.finalize();
        });

    // ============================================
    // Utility functions
    // ============================================
    m.def("device_count", &koo::gpu::Device::getDeviceCount,
          "Get number of available GPU devices");

    m.def("is_cuda_available", []() {
#ifdef KOO_CUDA_ENABLED
        return true;
#else
        return false;
#endif
    }, "Check if CUDA is available");

    m.def("is_hip_available", []() {
#ifdef KOO_HIP_ENABLED
        return true;
#else
        return false;
#endif
    }, "Check if HIP is available");

    m.def("get_gpu_info", []() {
        py::dict info;
        info["device_count"] = koo::gpu::Device::getDeviceCount();
        info["runtime"] = koo::gpu::Device::getRuntime();
        info["gpu_available"] = koo::gpu::Device::isGPUAvailable();

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

        return info;
    }, "Get GPU information");
}
