/**
 * @file MultiGPU.h
 * @brief Multi-GPU management and domain decomposition
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 55: GPU Domain Decomposition
 *
 * Provides multi-GPU support with automatic domain partitioning,
 * load balancing, and GPU resource management.
 */

#ifndef KOO_GPU_PARALLEL_MULTIGPU_H
#define KOO_GPU_PARALLEL_MULTIGPU_H

#include "../Device.h"
#include "../Memory.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace koo {
namespace gpu {
namespace parallel {

/**
 * @struct DomainPartition
 * @brief Describes a spatial domain partition for one GPU
 */
struct DomainPartition {
    int gpuId{-1};              ///< GPU device ID
    size_t startIndex{0};       ///< Start index in global array
    size_t endIndex{0};         ///< End index in global array (exclusive)
    size_t localSize{0};        ///< Number of elements in this partition
    size_t halos[2]{0, 0};      ///< Halo sizes [left, right]

    /**
     * @brief Get total size including halos
     */
    size_t totalSize() const {
        return localSize + halos[0] + halos[1];
    }

    /**
     * @brief Check if this partition is valid
     */
    bool isValid() const {
        return gpuId >= 0 && localSize > 0 && endIndex > startIndex;
    }
};

/**
 * @struct GPUWorkload
 * @brief GPU workload and utilization metrics
 */
struct GPUWorkload {
    int gpuId{-1};
    size_t assignedWork{0};     ///< Assigned work units
    double utilization{0.0};    ///< Utilization fraction [0, 1]
    double computeTime{0.0};    ///< Last compute time (seconds)
    double memoryUsed{0.0};     ///< Memory used (bytes)

    /**
     * @brief Calculate load factor (higher = more loaded)
     */
    double loadFactor() const {
        return utilization * (1.0 + computeTime);
    }
};

/**
 * @class MultiGPUManager
 * @brief Manages multiple GPUs for parallel computation
 *
 * Features:
 * - Automatic GPU discovery and initialization
 * - Domain decomposition across GPUs
 * - Load balancing based on GPU capabilities
 * - GPU utilization monitoring
 *
 * Example:
 * @code
 *   MultiGPUManager mgr;
 *   mgr.initialize();
 *
 *   auto partitions = mgr.partition1D(1000000, 2);  // 2 halo cells
 *   for (const auto& part : partitions) {
 *       // Distribute work to GPU part.gpuId
 *   }
 * @endcode
 */
class MultiGPUManager {
public:
    /**
     * @brief Default constructor
     */
    MultiGPUManager() = default;

    /**
     * @brief Destructor
     */
    ~MultiGPUManager() {
        finalize();
    }

    // Non-copyable
    MultiGPUManager(const MultiGPUManager&) = delete;
    MultiGPUManager& operator=(const MultiGPUManager&) = delete;

    /**
     * @brief Initialize multi-GPU system
     * @param deviceIds GPU device IDs to use (empty = use all)
     * @throws GPUError if initialization fails
     */
    void initialize(const std::vector<int>& deviceIds = {}) {
        if (initialized_) {
            return;
        }

        int deviceCount = Device::getDeviceCount();
        if (deviceCount == 0) {
            throw GPUError("No GPU devices available");
        }

        // Select devices
        if (deviceIds.empty()) {
            // Use all available devices
            for (int i = 0; i < deviceCount; ++i) {
                deviceIds_.push_back(i);
            }
        } else {
            // Use specified devices
            for (int id : deviceIds) {
                if (id < 0 || id >= deviceCount) {
                    throw GPUError("Invalid device ID: " + std::to_string(id));
                }
                deviceIds_.push_back(id);
            }
        }

        // Initialize devices
        devices_.clear();
        workloads_.clear();

        for (int id : deviceIds_) {
            devices_.push_back(Device::getDevice(id));

            GPUWorkload workload;
            workload.gpuId = id;
            workload.assignedWork = 0;
            workload.utilization = 0.0;
            workloads_.push_back(workload);
        }

        initialized_ = true;
    }

    /**
     * @brief Finalize and cleanup
     */
    void finalize() {
        if (!initialized_) {
            return;
        }

        // Synchronize all devices
        for (auto& device : devices_) {
            device.synchronize();
        }

        devices_.clear();
        deviceIds_.clear();
        workloads_.clear();
        initialized_ = false;
    }

    /**
     * @brief Get number of managed GPUs
     */
    int getNumGPUs() const {
        return static_cast<int>(devices_.size());
    }

    /**
     * @brief Get device by index
     * @param index Index in managed devices
     */
    Device& getDevice(int index) {
        if (index < 0 || index >= getNumGPUs()) {
            throw GPUError("Device index out of range");
        }
        return devices_[index];
    }

    /**
     * @brief Get device ID by index
     */
    int getDeviceId(int index) const {
        if (index < 0 || index >= getNumGPUs()) {
            throw GPUError("Device index out of range");
        }
        return deviceIds_[index];
    }

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const {
        return initialized_;
    }

    /**
     * @brief Synchronize all GPUs
     */
    void synchronizeAll() {
        for (auto& device : devices_) {
            device.synchronize();
        }
    }

    /**
     * @brief Partition 1D domain across GPUs
     * @param globalSize Total domain size
     * @param haloSize Halo size for boundary exchange
     * @return Vector of domain partitions (one per GPU)
     */
    std::vector<DomainPartition> partition1D(size_t globalSize, size_t haloSize = 0) {
        if (!initialized_) {
            throw GPUError("MultiGPUManager not initialized");
        }

        if (globalSize == 0) {
            throw GPUError("Cannot partition zero-size domain");
        }

        int numGPUs = getNumGPUs();
        std::vector<DomainPartition> partitions(numGPUs);

        // Get GPU memory capacities for weighted partitioning
        std::vector<size_t> memoryCapacity(numGPUs);
        size_t totalMemory = 0;

        for (int i = 0; i < numGPUs; ++i) {
            auto props = devices_[i].getProperties();
            memoryCapacity[i] = props.totalMemory;
            totalMemory += props.totalMemory;
        }

        // Calculate partition sizes based on memory capacity
        size_t currentStart = 0;

        for (int i = 0; i < numGPUs; ++i) {
            DomainPartition& part = partitions[i];
            part.gpuId = deviceIds_[i];
            part.startIndex = currentStart;

            // Weighted by memory capacity
            if (i == numGPUs - 1) {
                // Last GPU gets remaining
                part.endIndex = globalSize;
            } else {
                double fraction = static_cast<double>(memoryCapacity[i]) / totalMemory;
                size_t partitionSize = static_cast<size_t>(globalSize * fraction);
                part.endIndex = currentStart + partitionSize;
            }

            part.localSize = part.endIndex - part.startIndex;

            // Set halo sizes
            part.halos[0] = (i > 0) ? haloSize : 0;              // Left halo
            part.halos[1] = (i < numGPUs - 1) ? haloSize : 0;    // Right halo

            currentStart = part.endIndex;

            // Update workload
            workloads_[i].assignedWork = part.localSize;
        }

        return partitions;
    }

    /**
     * @brief Partition 2D domain across GPUs
     * @param nx Grid size in x-direction
     * @param ny Grid size in y-direction
     * @param haloSize Halo size for boundary exchange
     * @return Vector of 2D domain partitions
     */
    std::vector<DomainPartition> partition2D(size_t nx, size_t ny, size_t haloSize = 0) {
        // For simplicity, partition along y-direction
        size_t globalSize = nx * ny;
        auto partitions1D = partition1D(ny, haloSize);

        // Convert to 2D partitions
        std::vector<DomainPartition> partitions2D;
        for (const auto& part1D : partitions1D) {
            DomainPartition part2D = part1D;
            part2D.startIndex *= nx;
            part2D.endIndex *= nx;
            part2D.localSize *= nx;
            partitions2D.push_back(part2D);
        }

        return partitions2D;
    }

    /**
     * @brief Get GPU workload information
     * @param index GPU index
     */
    const GPUWorkload& getWorkload(int index) const {
        if (index < 0 || index >= getNumGPUs()) {
            throw GPUError("Device index out of range");
        }
        return workloads_[index];
    }

    /**
     * @brief Update GPU utilization
     * @param index GPU index
     * @param utilization Utilization fraction [0, 1]
     */
    void updateUtilization(int index, double utilization) {
        if (index < 0 || index >= getNumGPUs()) {
            throw GPUError("Device index out of range");
        }
        workloads_[index].utilization = std::max(0.0, std::min(1.0, utilization));
    }

    /**
     * @brief Get least loaded GPU index
     */
    int getLeastLoadedGPU() const {
        if (workloads_.empty()) {
            throw GPUError("No GPUs available");
        }

        int minIndex = 0;
        double minLoad = workloads_[0].loadFactor();

        for (size_t i = 1; i < workloads_.size(); ++i) {
            double load = workloads_[i].loadFactor();
            if (load < minLoad) {
                minLoad = load;
                minIndex = static_cast<int>(i);
            }
        }

        return minIndex;
    }

    /**
     * @brief Check if load is balanced
     * @param threshold Imbalance threshold (default 0.2 = 20%)
     */
    bool isLoadBalanced(double threshold = 0.2) const {
        if (workloads_.size() <= 1) {
            return true;
        }

        // Calculate load statistics
        std::vector<double> loads;
        for (const auto& wl : workloads_) {
            loads.push_back(wl.loadFactor());
        }

        double minLoad = *std::min_element(loads.begin(), loads.end());
        double maxLoad = *std::max_element(loads.begin(), loads.end());

        if (maxLoad < 1e-10) {
            return true;  // No load
        }

        double imbalance = (maxLoad - minLoad) / maxLoad;
        return imbalance <= threshold;
    }

    /**
     * @brief Enable peer-to-peer access between GPUs
     */
    void enablePeerAccess() {
#if defined(KOO_CUDA_ENABLED)
        int numGPUs = getNumGPUs();

        for (int i = 0; i < numGPUs; ++i) {
            cudaSetDevice(deviceIds_[i]);

            for (int j = 0; j < numGPUs; ++j) {
                if (i == j) continue;

                int canAccess = 0;
                cudaDeviceCanAccessPeer(&canAccess, deviceIds_[i], deviceIds_[j]);

                if (canAccess) {
                    cudaError_t err = cudaDeviceEnablePeerAccess(deviceIds_[j], 0);
                    if (err == cudaSuccess || err == cudaErrorPeerAccessAlreadyEnabled) {
                        // Success or already enabled
                        cudaGetLastError();  // Clear error
                    }
                }
            }
        }
#elif defined(KOO_HIP_ENABLED)
        // HIP peer access
        int numGPUs = getNumGPUs();

        for (int i = 0; i < numGPUs; ++i) {
            hipSetDevice(deviceIds_[i]);

            for (int j = 0; j < numGPUs; ++j) {
                if (i == j) continue;

                int canAccess = 0;
                hipDeviceCanAccessPeer(&canAccess, deviceIds_[i], deviceIds_[j]);

                if (canAccess) {
                    hipError_t err = hipDeviceEnablePeerAccess(deviceIds_[j], 0);
                    if (err == hipSuccess || err == hipErrorPeerAccessAlreadyEnabled) {
                        // Success or already enabled
                        hipGetLastError();  // Clear error
                    }
                }
            }
        }
#endif
    }

    /**
     * @brief Print multi-GPU configuration
     */
    void printInfo() const {
        std::cout << "=== Multi-GPU Configuration ===" << std::endl;
        std::cout << "Number of GPUs: " << getNumGPUs() << std::endl;
        std::cout << "Runtime: " << Device::getRuntime() << std::endl;

        for (int i = 0; i < getNumGPUs(); ++i) {
            auto props = devices_[i].getProperties();
            const auto& wl = workloads_[i];

            std::cout << "\nGPU " << i << " (Device " << deviceIds_[i] << "):" << std::endl;
            std::cout << "  Name: " << props.name << std::endl;
            std::cout << "  Memory: " << (props.totalMemory / (1024.0*1024.0*1024.0))
                      << " GB" << std::endl;
            std::cout << "  Assigned work: " << wl.assignedWork << " units" << std::endl;
            std::cout << "  Utilization: " << (wl.utilization * 100.0) << "%" << std::endl;
        }

        std::cout << "\nLoad balanced: " << (isLoadBalanced() ? "Yes" : "No") << std::endl;
    }

private:
    bool initialized_{false};
    std::vector<int> deviceIds_;
    std::vector<Device> devices_;
    std::vector<GPUWorkload> workloads_;
};

} // namespace parallel
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_PARALLEL_MULTIGPU_H
