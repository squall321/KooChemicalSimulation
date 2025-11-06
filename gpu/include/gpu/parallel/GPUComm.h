/**
 * @file GPUComm.h
 * @brief GPU-to-GPU communication and data transfer
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha2
 * @date 2025-11-06
 *
 * Phase 55: GPU Domain Decomposition
 *
 * Provides GPU-to-GPU communication primitives including:
 * - Direct GPU-to-GPU transfers (peer access)
 * - Halo exchange for domain decomposition
 * - Asynchronous communication with streams
 */

#ifndef KOO_GPU_PARALLEL_GPUCOMM_H
#define KOO_GPU_PARALLEL_GPUCOMM_H

#include "../Device.h"
#include "../Memory.h"
#include "../Stream.h"
#include "MultiGPU.h"
#include <vector>
#include <memory>
#include <chrono>

namespace koo {
namespace gpu {
namespace parallel {

/**
 * @enum TransferMode
 * @brief GPU communication transfer modes
 */
enum class TransferMode {
    PeerDirect,     ///< Direct GPU-to-GPU (GPU-Direct)
    ViaCPU,         ///< Through CPU memory
    Automatic       ///< Automatically select best method
};

/**
 * @struct TransferStats
 * @brief Statistics for GPU transfers
 */
struct TransferStats {
    size_t bytesTransferred{0};
    double timeElapsed{0.0};  // seconds
    TransferMode mode{TransferMode::Automatic};

    double bandwidth() const {
        if (timeElapsed > 0) {
            return bytesTransferred / timeElapsed / 1e9;  // GB/s
        }
        return 0.0;
    }
};

/**
 * @class GPUCommunicator
 * @brief Handles GPU-to-GPU communication
 *
 * Features:
 * - Direct peer-to-peer transfers when available
 * - Fallback to CPU-mediated transfers
 * - Asynchronous transfers with streams
 * - Halo exchange for domain decomposition
 *
 * Example:
 * @code
 *   GPUCommunicator comm(mgr);
 *   comm.transfer(srcGPU, srcData, dstGPU, dstData, count);
 * @endcode
 */
class GPUCommunicator {
public:
    /**
     * @brief Constructor
     * @param manager Multi-GPU manager
     */
    explicit GPUCommunicator(MultiGPUManager& manager)
        : manager_(manager) {
        initialize();
    }

    /**
     * @brief Destructor
     */
    ~GPUCommunicator() = default;

    // Non-copyable
    GPUCommunicator(const GPUCommunicator&) = delete;
    GPUCommunicator& operator=(const GPUCommunicator&) = delete;

    /**
     * @brief Check if peer access is available between two GPUs
     * @param srcGPU Source GPU index
     * @param dstGPU Destination GPU index
     * @return True if peer access is available
     */
    bool canAccessPeer(int srcGPU, int dstGPU) const {
        if (srcGPU < 0 || srcGPU >= manager_.getNumGPUs() ||
            dstGPU < 0 || dstGPU >= manager_.getNumGPUs()) {
            return false;
        }

        if (srcGPU == dstGPU) {
            return true;
        }

#if defined(KOO_CUDA_ENABLED)
        int srcDeviceId = manager_.getDeviceId(srcGPU);
        int dstDeviceId = manager_.getDeviceId(dstGPU);

        int canAccess = 0;
        cudaDeviceCanAccessPeer(&canAccess, srcDeviceId, dstDeviceId);
        return canAccess != 0;
#elif defined(KOO_HIP_ENABLED)
        int srcDeviceId = manager_.getDeviceId(srcGPU);
        int dstDeviceId = manager_.getDeviceId(dstGPU);

        int canAccess = 0;
        hipDeviceCanAccessPeer(&canAccess, srcDeviceId, dstDeviceId);
        return canAccess != 0;
#else
        return false;
#endif
    }

    /**
     * @brief Transfer data between GPUs
     * @param srcGPU Source GPU index
     * @param srcPtr Source device pointer
     * @param dstGPU Destination GPU index
     * @param dstPtr Destination device pointer
     * @param count Number of elements
     * @param mode Transfer mode
     * @return Transfer statistics
     */
    template<typename T>
    TransferStats transfer(
        int srcGPU, const T* srcPtr,
        int dstGPU, T* dstPtr,
        size_t count,
        TransferMode mode = TransferMode::Automatic
    ) {
        if (srcGPU == dstGPU) {
            throw GPUError("Source and destination GPU are the same");
        }

        size_t bytes = count * sizeof(T);
        TransferStats stats;
        stats.bytesTransferred = bytes;

        auto start = std::chrono::high_resolution_clock::now();

        // Determine transfer mode
        TransferMode actualMode = mode;
        if (mode == TransferMode::Automatic) {
            actualMode = canAccessPeer(srcGPU, dstGPU) ?
                         TransferMode::PeerDirect : TransferMode::ViaCPU;
        }

        stats.mode = actualMode;

        if (actualMode == TransferMode::PeerDirect) {
            // Direct GPU-to-GPU transfer
            transferPeer(srcGPU, srcPtr, dstGPU, dstPtr, bytes);
        } else {
            // Transfer via CPU
            transferViaCPU(srcGPU, srcPtr, dstGPU, dstPtr, bytes);
        }

        auto end = std::chrono::high_resolution_clock::now();
        stats.timeElapsed = std::chrono::duration<double>(end - start).count();

        return stats;
    }

    /**
     * @brief Asynchronous transfer between GPUs
     * @param srcGPU Source GPU index
     * @param srcPtr Source device pointer
     * @param dstGPU Destination GPU index
     * @param dstPtr Destination device pointer
     * @param count Number of elements
     * @param stream Stream for asynchronous transfer
     */
    template<typename T>
    void transferAsync(
        int srcGPU, const T* srcPtr,
        int dstGPU, T* dstPtr,
        size_t count,
        Stream& stream
    ) {
        if (srcGPU == dstGPU) {
            throw GPUError("Source and destination GPU are the same");
        }

        size_t bytes = count * sizeof(T);

        if (canAccessPeer(srcGPU, dstGPU)) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
            // Set destination device
            int dstDeviceId = manager_.getDeviceId(dstGPU);
            CHECK_GPU(cudaSetDevice(dstDeviceId));

            // Async peer copy
            CHECK_GPU(cudaMemcpyPeerAsync(
                dstPtr, dstDeviceId,
                srcPtr, manager_.getDeviceId(srcGPU),
                bytes, stream.get()
            ));
#endif
        } else {
            // For CPU fallback, we use synchronous transfer
            transferViaCPU(srcGPU, srcPtr, dstGPU, dstPtr, bytes);
        }
    }

    /**
     * @brief Exchange halo regions between neighboring GPUs
     * @param partitions Domain partitions
     * @param data Data pointers for each GPU
     * @param haloSize Number of halo elements
     */
    template<typename T>
    void exchangeHalos(
        const std::vector<DomainPartition>& partitions,
        const std::vector<T*>& data,
        size_t haloSize
    ) {
        int numGPUs = static_cast<int>(partitions.size());

        if (static_cast<int>(data.size()) != numGPUs) {
            throw GPUError("Data size does not match number of partitions");
        }

        // Exchange halos between neighboring GPUs
        for (int i = 0; i < numGPUs - 1; ++i) {
            const auto& leftPart = partitions[i];
            const auto& rightPart = partitions[i + 1];

            // Send right boundary of GPU i to left halo of GPU i+1
            T* srcPtr = data[i] + leftPart.localSize - haloSize;
            T* dstPtr = data[i + 1];  // Left halo of i+1

            transfer(i, srcPtr, i + 1, dstPtr, haloSize);

            // Send left boundary of GPU i+1 to right halo of GPU i
            srcPtr = data[i + 1] + haloSize;  // First real element of i+1
            dstPtr = data[i] + leftPart.localSize;  // Right halo of i

            transfer(i + 1, srcPtr, i, dstPtr, haloSize);
        }
    }

    /**
     * @brief Broadcast data from one GPU to all others
     * @param srcGPU Source GPU index
     * @param srcPtr Source data pointer
     * @param dstPtrs Destination pointers for each GPU
     * @param count Number of elements
     */
    template<typename T>
    void broadcast(
        int srcGPU,
        const T* srcPtr,
        const std::vector<T*>& dstPtrs,
        size_t count
    ) {
        int numGPUs = manager_.getNumGPUs();

        if (static_cast<int>(dstPtrs.size()) != numGPUs) {
            throw GPUError("Destination array size mismatch");
        }

        for (int i = 0; i < numGPUs; ++i) {
            if (i == srcGPU) {
                // Copy to self if needed
                if (srcPtr != dstPtrs[i]) {
                    auto& device = manager_.getDevice(srcGPU);
                    device.setCurrent();
                    size_t bytes = count * sizeof(T);
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
                    CHECK_GPU(cudaMemcpy(dstPtrs[i], srcPtr, bytes, cudaMemcpyDeviceToDevice));
#else
                    std::memcpy(dstPtrs[i], srcPtr, bytes);
#endif
                }
            } else {
                transfer(srcGPU, srcPtr, i, dstPtrs[i], count);
            }
        }
    }

    /**
     * @brief Gather data from all GPUs to one GPU
     * @param dstGPU Destination GPU index
     * @param dstPtr Destination pointer (must be large enough)
     * @param srcPtrs Source pointers for each GPU
     * @param counts Number of elements from each GPU
     */
    template<typename T>
    void gather(
        int dstGPU,
        T* dstPtr,
        const std::vector<const T*>& srcPtrs,
        const std::vector<size_t>& counts
    ) {
        int numGPUs = manager_.getNumGPUs();

        if (static_cast<int>(srcPtrs.size()) != numGPUs ||
            static_cast<int>(counts.size()) != numGPUs) {
            throw GPUError("Source arrays size mismatch");
        }

        size_t offset = 0;
        for (int i = 0; i < numGPUs; ++i) {
            if (i == dstGPU) {
                // Copy within same GPU if needed
                if (srcPtrs[i] != dstPtr + offset) {
                    auto& device = manager_.getDevice(dstGPU);
                    device.setCurrent();
                    size_t bytes = counts[i] * sizeof(T);
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
                    CHECK_GPU(cudaMemcpy(dstPtr + offset, srcPtrs[i], bytes,
                                        cudaMemcpyDeviceToDevice));
#else
                    std::memcpy(dstPtr + offset, srcPtrs[i], bytes);
#endif
                }
            } else {
                transfer(i, srcPtrs[i], dstGPU, dstPtr + offset, counts[i]);
            }
            offset += counts[i];
        }
    }

    /**
     * @brief Print communication statistics
     */
    void printStats() const {
        std::cout << "=== GPU Communication Stats ===" << std::endl;
        std::cout << "Number of GPUs: " << manager_.getNumGPUs() << std::endl;

        int numGPUs = manager_.getNumGPUs();
        std::cout << "\nPeer Access Matrix:" << std::endl;
        std::cout << "     ";
        for (int j = 0; j < numGPUs; ++j) {
            std::cout << "GPU" << j << " ";
        }
        std::cout << std::endl;

        for (int i = 0; i < numGPUs; ++i) {
            std::cout << "GPU" << i << " ";
            for (int j = 0; j < numGPUs; ++j) {
                if (i == j) {
                    std::cout << "  -  ";
                } else {
                    std::cout << (canAccessPeer(i, j) ? " Yes " : " No  ");
                }
            }
            std::cout << std::endl;
        }
    }

private:
    /**
     * @brief Initialize communication system
     */
    void initialize() {
        // Enable peer access
        manager_.enablePeerAccess();
    }

    /**
     * @brief Direct peer-to-peer transfer
     */
    template<typename T>
    void transferPeer(int srcGPU, const T* srcPtr, int dstGPU, T* dstPtr, size_t bytes) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        int srcDeviceId = manager_.getDeviceId(srcGPU);
        int dstDeviceId = manager_.getDeviceId(dstGPU);

        CHECK_GPU(cudaMemcpyPeer(dstPtr, dstDeviceId, srcPtr, srcDeviceId, bytes));
#else
        throw GPUError("Peer transfer not supported on CPU");
#endif
    }

    /**
     * @brief Transfer via CPU memory
     */
    template<typename T>
    void transferViaCPU(int srcGPU, const T* srcPtr, int dstGPU, T* dstPtr, size_t bytes) {
        // Allocate temporary CPU buffer
        std::vector<char> cpuBuffer(bytes);

        // Copy from source GPU to CPU
        auto& srcDevice = manager_.getDevice(srcGPU);
        srcDevice.setCurrent();

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpy(cpuBuffer.data(), srcPtr, bytes, cudaMemcpyDeviceToHost));
#else
        std::memcpy(cpuBuffer.data(), srcPtr, bytes);
#endif

        // Copy from CPU to destination GPU
        auto& dstDevice = manager_.getDevice(dstGPU);
        dstDevice.setCurrent();

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpy(dstPtr, cpuBuffer.data(), bytes, cudaMemcpyHostToDevice));
#else
        std::memcpy(dstPtr, cpuBuffer.data(), bytes);
#endif
    }

    MultiGPUManager& manager_;
};

} // namespace parallel
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_PARALLEL_GPUCOMM_H
