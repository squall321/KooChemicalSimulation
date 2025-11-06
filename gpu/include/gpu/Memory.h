/**
 * @file Memory.h
 * @brief GPU memory management with RAII
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 51: GPU Abstraction Layer
 *
 * Provides RAII-based GPU memory allocation and data transfer.
 * Supports device memory, pinned host memory, and unified memory.
 */

#ifndef KOO_GPU_MEMORY_H
#define KOO_GPU_MEMORY_H

#include "Device.h"
#include <cstring>
#include <algorithm>
#include <type_traits>

namespace koo {
namespace gpu {

/**
 * @class DeviceMemory
 * @brief RAII wrapper for GPU device memory
 *
 * Phase 51: GPU Memory Management
 *
 * Automatically allocates and deallocates GPU memory.
 * Provides type-safe memory operations.
 *
 * @tparam T Element type
 */
template<typename T>
class DeviceMemory {
public:
    /**
     * @brief Constructor - allocate GPU memory
     * @param size Number of elements
     */
    explicit DeviceMemory(size_t size = 0)
        : size_(size), devicePtr_(nullptr) {
        if (size > 0) {
            allocate(size);
        }
    }

    /**
     * @brief Destructor - free GPU memory
     */
    ~DeviceMemory() {
        free();
    }

    // Move semantics
    DeviceMemory(DeviceMemory&& other) noexcept
        : size_(other.size_), devicePtr_(other.devicePtr_) {
        other.size_ = 0;
        other.devicePtr_ = nullptr;
    }

    DeviceMemory& operator=(DeviceMemory&& other) noexcept {
        if (this != &other) {
            free();
            size_ = other.size_;
            devicePtr_ = other.devicePtr_;
            other.size_ = 0;
            other.devicePtr_ = nullptr;
        }
        return *this;
    }

    // Non-copyable
    DeviceMemory(const DeviceMemory&) = delete;
    DeviceMemory& operator=(const DeviceMemory&) = delete;

    /**
     * @brief Allocate GPU memory
     * @param size Number of elements
     */
    void allocate(size_t size) {
        if (devicePtr_ != nullptr) {
            free();
        }

        size_ = size;
        if (size == 0) return;

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMalloc(&devicePtr_, size * sizeof(T)));
#else
        // CPU fallback
        devicePtr_ = reinterpret_cast<T*>(std::malloc(size * sizeof(T)));
        if (!devicePtr_) {
            throw GPUError("CPU memory allocation failed");
        }
#endif
    }

    /**
     * @brief Free GPU memory
     */
    void free() {
        if (devicePtr_ != nullptr) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
            cudaFree(devicePtr_);  // Don't check error in destructor path
#else
            std::free(devicePtr_);
#endif
            devicePtr_ = nullptr;
            size_ = 0;
        }
    }

    /**
     * @brief Copy data from host to device
     * @param hostPtr Host data pointer
     * @param count Number of elements (default: all)
     */
    void copyFromHost(const T* hostPtr, size_t count = 0) {
        if (count == 0) count = size_;
        if (count > size_) {
            throw GPUError("Copy count exceeds allocated size");
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpy(devicePtr_, hostPtr, count * sizeof(T),
                            cudaMemcpyHostToDevice));
#else
        std::memcpy(devicePtr_, hostPtr, count * sizeof(T));
#endif
    }

    /**
     * @brief Copy data from device to host
     * @param hostPtr Host data pointer
     * @param count Number of elements (default: all)
     */
    void copyToHost(T* hostPtr, size_t count = 0) const {
        if (count == 0) count = size_;
        if (count > size_) {
            throw GPUError("Copy count exceeds allocated size");
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpy(hostPtr, devicePtr_, count * sizeof(T),
                            cudaMemcpyDeviceToHost));
#else
        std::memcpy(hostPtr, devicePtr_, count * sizeof(T));
#endif
    }

    /**
     * @brief Copy to host vector
     * @return Host vector
     */
    std::vector<T> toHost() const {
        std::vector<T> result(size_);
        copyToHost(result.data());
        return result;
    }

    /**
     * @brief Copy data from another device memory
     * @param other Source device memory
     * @param count Number of elements (default: all)
     */
    void copyFromDevice(const DeviceMemory<T>& other, size_t count = 0) {
        if (count == 0) count = std::min(size_, other.size_);
        if (count > size_ || count > other.size_) {
            throw GPUError("Copy count exceeds allocated size");
        }

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpy(devicePtr_, other.devicePtr_, count * sizeof(T),
                            cudaMemcpyDeviceToDevice));
#else
        std::memcpy(devicePtr_, other.devicePtr_, count * sizeof(T));
#endif
    }

    /**
     * @brief Fill memory with a value
     * @param value Value to fill
     */
    void fill(const T& value) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        // For simple types, use cudaMemset if possible
        if (std::is_trivial<T>::value && sizeof(T) == 1) {
            CHECK_GPU(cudaMemset(devicePtr_, static_cast<int>(value), size_));
        } else {
            // For complex types, copy from host
            std::vector<T> temp(size_, value);
            copyFromHost(temp.data());
        }
#else
        std::fill_n(devicePtr_, size_, value);
#endif
    }

    /**
     * @brief Zero-fill memory
     */
    void zero() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemset(devicePtr_, 0, size_ * sizeof(T)));
#else
        std::memset(devicePtr_, 0, size_ * sizeof(T));
#endif
    }

    /**
     * @brief Resize memory (reallocates)
     * @param newSize New size
     */
    void resize(size_t newSize) {
        if (newSize != size_) {
            free();
            allocate(newSize);
        }
    }

    /**
     * @brief Get device pointer
     */
    T* data() { return devicePtr_; }
    const T* data() const { return devicePtr_; }

    /**
     * @brief Get size (number of elements)
     */
    size_t size() const { return size_; }

    /**
     * @brief Get size in bytes
     */
    size_t sizeBytes() const { return size_ * sizeof(T); }

    /**
     * @brief Check if memory is allocated
     */
    bool empty() const { return size_ == 0 || devicePtr_ == nullptr; }

private:
    size_t size_;
    T* devicePtr_;
};

/**
 * @class PinnedMemory
 * @brief Pinned (page-locked) host memory for fast transfers
 *
 * Phase 51: GPU Memory Management
 *
 * Pinned memory enables faster CPU-GPU transfers and
 * asynchronous operations.
 */
template<typename T>
class PinnedMemory {
public:
    /**
     * @brief Constructor - allocate pinned memory
     * @param size Number of elements
     */
    explicit PinnedMemory(size_t size = 0)
        : size_(size), hostPtr_(nullptr) {
        if (size > 0) {
            allocate(size);
        }
    }

    /**
     * @brief Destructor - free pinned memory
     */
    ~PinnedMemory() {
        free();
    }

    // Move semantics
    PinnedMemory(PinnedMemory&& other) noexcept
        : size_(other.size_), hostPtr_(other.hostPtr_) {
        other.size_ = 0;
        other.hostPtr_ = nullptr;
    }

    PinnedMemory& operator=(PinnedMemory&& other) noexcept {
        if (this != &other) {
            free();
            size_ = other.size_;
            hostPtr_ = other.hostPtr_;
            other.size_ = 0;
            other.hostPtr_ = nullptr;
        }
        return *this;
    }

    // Non-copyable
    PinnedMemory(const PinnedMemory&) = delete;
    PinnedMemory& operator=(const PinnedMemory&) = delete;

    /**
     * @brief Allocate pinned memory
     */
    void allocate(size_t size) {
        if (hostPtr_ != nullptr) {
            free();
        }

        size_ = size;
        if (size == 0) return;

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMallocHost(&hostPtr_, size * sizeof(T)));
#else
        hostPtr_ = reinterpret_cast<T*>(std::malloc(size * sizeof(T)));
        if (!hostPtr_) {
            throw GPUError("Pinned memory allocation failed");
        }
#endif
    }

    /**
     * @brief Free pinned memory
     */
    void free() {
        if (hostPtr_ != nullptr) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
            cudaFreeHost(hostPtr_);
#else
            std::free(hostPtr_);
#endif
            hostPtr_ = nullptr;
            size_ = 0;
        }
    }

    /**
     * @brief Get host pointer
     */
    T* data() { return hostPtr_; }
    const T* data() const { return hostPtr_; }

    /**
     * @brief Array access
     */
    T& operator[](size_t i) { return hostPtr_[i]; }
    const T& operator[](size_t i) const { return hostPtr_[i]; }

    /**
     * @brief Get size
     */
    size_t size() const { return size_; }

    /**
     * @brief Check if allocated
     */
    bool empty() const { return size_ == 0 || hostPtr_ == nullptr; }

private:
    size_t size_;
    T* hostPtr_;
};

/**
 * @class ManagedMemory
 * @brief Unified (managed) memory accessible from CPU and GPU
 *
 * Phase 51: GPU Memory Management
 *
 * Managed memory is automatically migrated between CPU and GPU.
 * Simplifies programming but may have performance overhead.
 */
template<typename T>
class ManagedMemory {
public:
    /**
     * @brief Constructor - allocate managed memory
     * @param size Number of elements
     */
    explicit ManagedMemory(size_t size = 0)
        : size_(size), managedPtr_(nullptr) {
        if (size > 0) {
            allocate(size);
        }
    }

    /**
     * @brief Destructor - free managed memory
     */
    ~ManagedMemory() {
        free();
    }

    // Move semantics
    ManagedMemory(ManagedMemory&& other) noexcept
        : size_(other.size_), managedPtr_(other.managedPtr_) {
        other.size_ = 0;
        other.managedPtr_ = nullptr;
    }

    ManagedMemory& operator=(ManagedMemory&& other) noexcept {
        if (this != &other) {
            free();
            size_ = other.size_;
            managedPtr_ = other.managedPtr_;
            other.size_ = 0;
            other.managedPtr_ = nullptr;
        }
        return *this;
    }

    // Non-copyable
    ManagedMemory(const ManagedMemory&) = delete;
    ManagedMemory& operator=(const ManagedMemory&) = delete;

    /**
     * @brief Allocate managed memory
     */
    void allocate(size_t size) {
        if (managedPtr_ != nullptr) {
            free();
        }

        size_ = size;
        if (size == 0) return;

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMallocManaged(&managedPtr_, size * sizeof(T)));
#else
        managedPtr_ = reinterpret_cast<T*>(std::malloc(size * sizeof(T)));
        if (!managedPtr_) {
            throw GPUError("Managed memory allocation failed");
        }
#endif
    }

    /**
     * @brief Free managed memory
     */
    void free() {
        if (managedPtr_ != nullptr) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
            cudaFree(managedPtr_);
#else
            std::free(managedPtr_);
#endif
            managedPtr_ = nullptr;
            size_ = 0;
        }
    }

    /**
     * @brief Prefetch to device (hint for performance)
     */
    void prefetchToDevice(int deviceId = 0) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemPrefetchAsync(managedPtr_, size_ * sizeof(T),
                                      deviceId, nullptr));
#endif
    }

    /**
     * @brief Prefetch to host (hint for performance)
     */
    void prefetchToHost() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemPrefetchAsync(managedPtr_, size_ * sizeof(T),
                                      cudaCpuDeviceId, nullptr));
#endif
    }

    /**
     * @brief Get pointer (accessible from CPU and GPU)
     */
    T* data() { return managedPtr_; }
    const T* data() const { return managedPtr_; }

    /**
     * @brief Array access
     */
    T& operator[](size_t i) { return managedPtr_[i]; }
    const T& operator[](size_t i) const { return managedPtr_[i]; }

    /**
     * @brief Get size
     */
    size_t size() const { return size_; }

    /**
     * @brief Check if allocated
     */
    bool empty() const { return size_ == 0 || managedPtr_ == nullptr; }

private:
    size_t size_;
    T* managedPtr_;
};

/**
 * @brief Helper function to copy vector to GPU
 */
template<typename T>
DeviceMemory<T> makeDeviceMemory(const std::vector<T>& hostData) {
    DeviceMemory<T> deviceMem(hostData.size());
    deviceMem.copyFromHost(hostData.data());
    return deviceMem;
}

/**
 * @brief Helper function to copy GPU to vector
 */
template<typename T>
std::vector<T> toVector(const DeviceMemory<T>& deviceMem) {
    std::vector<T> hostData(deviceMem.size());
    deviceMem.copyToHost(hostData.data());
    return hostData;
}

} // namespace gpu
} // namespace koo

#endif // KOO_GPU_MEMORY_H
