/**
 * @file Kernel.h
 * @brief GPU kernel launch utilities
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 51: GPU Abstraction Layer
 *
 * Provides utilities for kernel launch configuration and execution.
 * Simplifies grid/block size computation and kernel invocation.
 */

#ifndef KOO_GPU_KERNEL_H
#define KOO_GPU_KERNEL_H

#include "Device.h"
#include "Stream.h"
#include <cmath>

namespace koo {
namespace gpu {

/**
 * @struct dim3
 * @brief 3D dimensions for grid and block
 *
 * Compatible with CUDA/HIP dim3 but available in CPU mode.
 */
#if !defined(KOO_CUDA_ENABLED) && !defined(KOO_HIP_ENABLED)
struct dim3 {
    unsigned int x, y, z;

    dim3(unsigned int x_ = 1, unsigned int y_ = 1, unsigned int z_ = 1)
        : x(x_), y(y_), z(z_) {}

    size_t size() const { return static_cast<size_t>(x) * y * z; }
};
#endif

/**
 * @struct LaunchConfig
 * @brief Kernel launch configuration
 *
 * Phase 51: Kernel Configuration
 *
 * Encapsulates grid size, block size, and dynamic shared memory.
 */
struct LaunchConfig {
    dim3 gridSize;              ///< Grid dimensions (blocks)
    dim3 blockSize;             ///< Block dimensions (threads)
    size_t sharedMemBytes{0};   ///< Dynamic shared memory (bytes)
    Stream* stream{nullptr};    ///< Stream for async execution

    /**
     * @brief Constructor
     */
    LaunchConfig(dim3 grid, dim3 block,
                size_t sharedMem = 0,
                Stream* str = nullptr)
        : gridSize(grid), blockSize(block),
          sharedMemBytes(sharedMem), stream(str) {}

    /**
     * @brief Get total number of blocks
     */
    size_t numBlocks() const {
        return static_cast<size_t>(gridSize.x) * gridSize.y * gridSize.z;
    }

    /**
     * @brief Get total number of threads per block
     */
    size_t numThreadsPerBlock() const {
        return static_cast<size_t>(blockSize.x) * blockSize.y * blockSize.z;
    }

    /**
     * @brief Get total number of threads
     */
    size_t totalThreads() const {
        return numBlocks() * numThreadsPerBlock();
    }

    /**
     * @brief Print configuration
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "Grid: (" << gridSize.x << ", " << gridSize.y << ", " << gridSize.z << "), "
            << "Block: (" << blockSize.x << ", " << blockSize.y << ", " << blockSize.z << "), "
            << "Shared Mem: " << sharedMemBytes << " bytes, "
            << "Total Threads: " << totalThreads();
        return oss.str();
    }
};

/**
 * @class KernelLauncher
 * @brief Utilities for kernel launch configuration
 *
 * Phase 51: Kernel Launch Utilities
 *
 * Provides helper functions to compute optimal grid and block sizes.
 */
class KernelLauncher {
public:
    /**
     * @brief Compute 1D launch configuration
     * @param n Problem size
     * @param blockSize Threads per block (default: 256)
     * @return Launch configuration
     */
    static LaunchConfig make1DConfig(size_t n, int blockSize = 256) {
        unsigned int numBlocks = static_cast<unsigned int>((n + blockSize - 1) / blockSize);
        return LaunchConfig(
            dim3(numBlocks, 1, 1),
            dim3(blockSize, 1, 1)
        );
    }

    /**
     * @brief Compute 2D launch configuration
     * @param nx Size in x dimension
     * @param ny Size in y dimension
     * @param blockX Threads per block in x (default: 16)
     * @param blockY Threads per block in y (default: 16)
     * @return Launch configuration
     */
    static LaunchConfig make2DConfig(size_t nx, size_t ny,
                                     int blockX = 16, int blockY = 16) {
        unsigned int gridX = static_cast<unsigned int>((nx + blockX - 1) / blockX);
        unsigned int gridY = static_cast<unsigned int>((ny + blockY - 1) / blockY);

        return LaunchConfig(
            dim3(gridX, gridY, 1),
            dim3(blockX, blockY, 1)
        );
    }

    /**
     * @brief Compute 3D launch configuration
     * @param nx Size in x dimension
     * @param ny Size in y dimension
     * @param nz Size in z dimension
     * @param blockX Threads per block in x (default: 8)
     * @param blockY Threads per block in y (default: 8)
     * @param blockZ Threads per block in z (default: 8)
     * @return Launch configuration
     */
    static LaunchConfig make3DConfig(size_t nx, size_t ny, size_t nz,
                                     int blockX = 8, int blockY = 8, int blockZ = 8) {
        unsigned int gridX = static_cast<unsigned int>((nx + blockX - 1) / blockX);
        unsigned int gridY = static_cast<unsigned int>((ny + blockY - 1) / blockY);
        unsigned int gridZ = static_cast<unsigned int>((nz + blockZ - 1) / blockZ);

        return LaunchConfig(
            dim3(gridX, gridY, gridZ),
            dim3(blockX, blockY, blockZ)
        );
    }

    /**
     * @brief Get optimal 1D block size for device
     * @param device GPU device
     * @return Optimal block size (typically 128, 256, or 512)
     */
    static int getOptimal1DBlockSize(const Device& device) {
        auto props = device.getProperties();

        // For modern GPUs, 256 is usually optimal
        // For older GPUs with fewer resources, use 128
        if (props.major >= 7) {
            return 256;  // Volta, Turing, Ampere, Ada
        } else if (props.major >= 6) {
            return 256;  // Pascal
        } else {
            return 128;  // Older architectures
        }
    }

    /**
     * @brief Get optimal 2D block size for device
     * @return {blockX, blockY}
     */
    static std::pair<int, int> getOptimal2DBlockSize(const Device& device) {
        auto props = device.getProperties();

        // 16x16 = 256 threads is usually optimal for 2D
        if (props.major >= 6) {
            return {16, 16};
        } else {
            return {16, 8};  // 128 threads for older GPUs
        }
    }

    /**
     * @brief Compute occupancy for given configuration
     * @param blockSize Block size
     * @param sharedMemPerBlock Shared memory per block
     * @param device GPU device
     * @return Estimated occupancy (0.0 to 1.0)
     */
    static double computeOccupancy(int blockSize,
                                  size_t sharedMemPerBlock,
                                  const Device& device) {
        auto props = device.getProperties();

        int maxThreadsPerBlock = props.maxThreadsPerBlock;
        int multiProcessorCount = props.multiProcessorCount;
        size_t sharedMemPerSM = props.sharedMemPerBlock;

        // Simple occupancy estimate
        if (blockSize > maxThreadsPerBlock) {
            return 0.0;  // Invalid configuration
        }

        // Estimate based on thread limit
        double threadOccupancy = static_cast<double>(blockSize) / maxThreadsPerBlock;

        // Estimate based on shared memory limit
        double memOccupancy = 1.0;
        if (sharedMemPerBlock > 0) {
            int blocksPerSM = static_cast<int>(sharedMemPerSM / sharedMemPerBlock);
            if (blocksPerSM == 0) return 0.0;  // Too much shared memory
            memOccupancy = std::min(1.0, static_cast<double>(blocksPerSM) / 16.0);
        }

        return std::min(threadOccupancy, memOccupancy);
    }

    /**
     * @brief Get maximum threads per block for device
     */
    static int getMaxThreadsPerBlock(const Device& device) {
        return device.getProperties().maxThreadsPerBlock;
    }

    /**
     * @brief Get warp/wavefront size for device
     */
    static int getWarpSize(const Device& device) {
        return device.getProperties().warpSize;
    }

    /**
     * @brief Round up to nearest multiple of warp size
     */
    static int roundUpToWarpSize(int value, const Device& device) {
        int warpSize = getWarpSize(device);
        return ((value + warpSize - 1) / warpSize) * warpSize;
    }

    /**
     * @brief Check if block size is valid
     */
    static bool isValidBlockSize(int blockSize, const Device& device) {
        int warpSize = getWarpSize(device);
        int maxThreads = getMaxThreadsPerBlock(device);

        // Must be multiple of warp size and <= max threads
        return (blockSize % warpSize == 0) && (blockSize <= maxThreads) && (blockSize > 0);
    }
};

/**
 * @brief Macro for getting thread ID in 1D kernel
 */
#define KOO_GET_THREAD_ID_1D() \
    (blockIdx.x * blockDim.x + threadIdx.x)

/**
 * @brief Macro for getting thread ID in 2D kernel
 */
#define KOO_GET_THREAD_ID_2D(i, j) \
    do { \
        i = blockIdx.x * blockDim.x + threadIdx.x; \
        j = blockIdx.y * blockDim.y + threadIdx.y; \
    } while(0)

/**
 * @brief Macro for getting thread ID in 3D kernel
 */
#define KOO_GET_THREAD_ID_3D(i, j, k) \
    do { \
        i = blockIdx.x * blockDim.x + threadIdx.x; \
        j = blockIdx.y * blockDim.y + threadIdx.y; \
        k = blockIdx.z * blockDim.z + threadIdx.z; \
    } while(0)

/**
 * @brief Macro for bounds check in 1D kernel
 */
#define KOO_KERNEL_BOUNDS_CHECK_1D(i, n) \
    if (i >= n) return

/**
 * @brief Macro for bounds check in 2D kernel
 */
#define KOO_KERNEL_BOUNDS_CHECK_2D(i, j, nx, ny) \
    if (i >= nx || j >= ny) return

/**
 * @brief Macro for bounds check in 3D kernel
 */
#define KOO_KERNEL_BOUNDS_CHECK_3D(i, j, k, nx, ny, nz) \
    if (i >= nx || j >= ny || k >= nz) return

/**
 * @class KernelTimer
 * @brief Measure kernel execution time
 *
 * Example:
 * KernelTimer timer;
 * timer.start();
 * myKernel<<<grid, block>>>();
 * float ms = timer.stop();
 */
class KernelTimer {
public:
    /**
     * @brief Start timing
     */
    void start(Stream* stream = nullptr) {
        stream_ = stream;
        if (stream_) {
            startEvent_.record(*stream_);
        } else {
            startEvent_.record();
        }
    }

    /**
     * @brief Stop timing and return elapsed time (ms)
     */
    float stop() {
        if (stream_) {
            stopEvent_.record(*stream_);
        } else {
            stopEvent_.record();
        }
        stopEvent_.synchronize();
        return Event::elapsedTime(startEvent_, stopEvent_);
    }

private:
    Event startEvent_;
    Event stopEvent_;
    Stream* stream_{nullptr};
};

/**
 * @brief Get number of CUDA cores per SM (approximate)
 */
inline int getCoresPerSM(int major, int minor) {
    // Based on compute capability
    switch (major) {
        case 2: return 32;   // Fermi
        case 3: return 192;  // Kepler
        case 5: return 128;  // Maxwell
        case 6:              // Pascal
            return (minor == 0) ? 64 : 128;
        case 7:              // Volta/Turing
            return (minor == 0) ? 64 : 64;
        case 8:              // Ampere/Ada
            return (minor == 0) ? 64 : 128;
        case 9:              // Hopper
            return 128;
        default:
            return 64;  // Default estimate
    }
}

/**
 * @brief Estimate peak GFLOPS for device
 */
inline double estimatePeakGFLOPS(const Device& device) {
    auto props = device.getProperties();
    int smCount = props.multiProcessorCount;
    auto [major, minor] = device.getComputeCapability();

    int coresPerSM = getCoresPerSM(major, minor);

    // Rough clock speed estimate (this should be queried from device)
    // Modern GPUs: ~1.0-2.0 GHz
    double clockGHz = 1.5;  // Conservative estimate

    // GFLOPS = cores * clock * 2 (multiply-add)
    double totalCores = smCount * coresPerSM;
    return totalCores * clockGHz * 2.0;
}

} // namespace gpu
} // namespace koo

#endif // KOO_GPU_KERNEL_H
