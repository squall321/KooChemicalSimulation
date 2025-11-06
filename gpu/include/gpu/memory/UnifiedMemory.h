/**
 * @file UnifiedMemory.h
 * @brief Unified memory (managed memory) support for seamless CPU-GPU data sharing
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 61: GPU Memory Optimization
 *
 * Features:
 * - Automatic CPU-GPU data migration
 * - Simplified memory management (no explicit copies)
 * - Prefetching and memory advice hints
 * - RAII wrapper for safe memory handling
 */

#pragma once

#include "../Device.h"
#include <memory>
#include <vector>
#include <stdexcept>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace koo {
namespace gpu {
namespace memory {

/**
 * @brief Memory advice flags for unified memory
 */
enum class MemoryAdvice {
    SetReadMostly,           ///< Data will be mostly read, not written
    UnsetReadMostly,         ///< Unset read-mostly hint
    SetPreferredLocation,    ///< Set preferred location (CPU or GPU)
    SetAccessedBy,           ///< Mark as accessed by specific device
    SetCoarseGrain          ///< Use coarse-grain allocation on AMD GPUs
};

/**
 * @brief Unified memory allocation flags
 */
enum class UnifiedFlags {
    Default = 0,             ///< Default unified memory
    AttachHost = 1,          ///< Attach to host (CPU-preferred)
    AttachGlobal = 2         ///< Attach to all GPUs
};

/**
 * @brief RAII wrapper for unified memory
 *
 * Provides automatic allocation/deallocation of CUDA unified memory
 * with type safety and convenient access patterns.
 */
template<typename T>
class UnifiedPtr {
public:
    /**
     * @brief Construct empty unified pointer
     */
    UnifiedPtr() : ptr_(nullptr), size_(0), owns_memory_(false) {}

    /**
     * @brief Allocate unified memory
     * @param count Number of elements
     * @param flags Allocation flags
     */
    explicit UnifiedPtr(size_t count, UnifiedFlags flags = UnifiedFlags::Default)
        : ptr_(nullptr), size_(count), owns_memory_(true) {
#ifdef KOO_USE_CUDA
        if (count > 0) {
            cudaError_t err = cudaMallocManaged(&ptr_, count * sizeof(T),
                                                static_cast<unsigned int>(flags));
            if (err != cudaSuccess) {
                throw std::runtime_error("Failed to allocate unified memory: " +
                                       std::string(cudaGetErrorString(err)));
            }
        }
#else
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Construct from existing unified memory (non-owning)
     */
    UnifiedPtr(T* ptr, size_t count)
        : ptr_(ptr), size_(count), owns_memory_(false) {}

    /**
     * @brief Destructor
     */
    ~UnifiedPtr() {
        if (owns_memory_ && ptr_) {
#ifdef KOO_USE_CUDA
            cudaFree(ptr_);
#endif
        }
    }

    // Move semantics
    UnifiedPtr(UnifiedPtr&& other) noexcept
        : ptr_(other.ptr_), size_(other.size_), owns_memory_(other.owns_memory_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
        other.owns_memory_ = false;
    }

    UnifiedPtr& operator=(UnifiedPtr&& other) noexcept {
        if (this != &other) {
            if (owns_memory_ && ptr_) {
#ifdef KOO_USE_CUDA
                cudaFree(ptr_);
#endif
            }
            ptr_ = other.ptr_;
            size_ = other.size_;
            owns_memory_ = other.owns_memory_;
            other.ptr_ = nullptr;
            other.size_ = 0;
            other.owns_memory_ = false;
        }
        return *this;
    }

    // No copy
    UnifiedPtr(const UnifiedPtr&) = delete;
    UnifiedPtr& operator=(const UnifiedPtr&) = delete;

    /**
     * @brief Get raw pointer
     */
    T* get() { return ptr_; }
    const T* get() const { return ptr_; }

    /**
     * @brief Array access
     */
    T& operator[](size_t idx) { return ptr_[idx]; }
    const T& operator[](size_t idx) const { return ptr_[idx]; }

    /**
     * @brief Get size (number of elements)
     */
    size_t size() const { return size_; }

    /**
     * @brief Get size in bytes
     */
    size_t size_bytes() const { return size_ * sizeof(T); }

    /**
     * @brief Check if valid
     */
    explicit operator bool() const { return ptr_ != nullptr; }

    /**
     * @brief Prefetch data to device
     * @param device Target device
     */
    void prefetchToDevice(const Device& device) {
#ifdef KOO_USE_CUDA
        if (ptr_ && size_ > 0) {
            cudaError_t err = cudaMemPrefetchAsync(ptr_, size_bytes(),
                                                   device.getId());
            if (err != cudaSuccess) {
                throw std::runtime_error("Prefetch failed: " +
                                       std::string(cudaGetErrorString(err)));
            }
        }
#else
        (void)device;
#endif
    }

    /**
     * @brief Prefetch data to CPU
     */
    void prefetchToHost() {
#ifdef KOO_USE_CUDA
        if (ptr_ && size_ > 0) {
            cudaError_t err = cudaMemPrefetchAsync(ptr_, size_bytes(),
                                                   cudaCpuDeviceId);
            if (err != cudaSuccess) {
                throw std::runtime_error("Prefetch to host failed: " +
                                       std::string(cudaGetErrorString(err)));
            }
        }
#endif
    }

    /**
     * @brief Apply memory advice hint
     * @param advice Memory advice type
     * @param device Target device (for location/access hints)
     */
    void setMemoryAdvice(MemoryAdvice advice, const Device* device = nullptr) {
#ifdef KOO_USE_CUDA
        if (!ptr_ || size_ == 0) return;

        cudaMemoryAdvise cuda_advice;
        int device_id = device ? device->getId() : cudaCpuDeviceId;

        switch (advice) {
            case MemoryAdvice::SetReadMostly:
                cuda_advice = cudaMemAdviseSetReadMostly;
                break;
            case MemoryAdvice::UnsetReadMostly:
                cuda_advice = cudaMemAdviseUnsetReadMostly;
                break;
            case MemoryAdvice::SetPreferredLocation:
                cuda_advice = cudaMemAdviseSetPreferredLocation;
                break;
            case MemoryAdvice::SetAccessedBy:
                cuda_advice = cudaMemAdviseSetAccessedBy;
                break;
            default:
                return;
        }

        cudaError_t err = cudaMemAdvise(ptr_, size_bytes(), cuda_advice, device_id);
        if (err != cudaSuccess) {
            throw std::runtime_error("Memory advice failed: " +
                                   std::string(cudaGetErrorString(err)));
        }
#else
        (void)advice;
        (void)device;
#endif
    }

    /**
     * @brief Set preferred location to device
     */
    void setPreferredLocationDevice(const Device& device) {
        setMemoryAdvice(MemoryAdvice::SetPreferredLocation, &device);
    }

    /**
     * @brief Set preferred location to host
     */
    void setPreferredLocationHost() {
        setMemoryAdvice(MemoryAdvice::SetPreferredLocation, nullptr);
    }

    /**
     * @brief Mark as read-mostly (optimization hint)
     */
    void setReadMostly() {
        setMemoryAdvice(MemoryAdvice::SetReadMostly);
    }

    /**
     * @brief Mark as accessed by device
     */
    void setAccessedBy(const Device& device) {
        setMemoryAdvice(MemoryAdvice::SetAccessedBy, &device);
    }

    /**
     * @brief Fill with value
     */
    void fill(const T& value) {
        if (ptr_ && size_ > 0) {
            for (size_t i = 0; i < size_; ++i) {
                ptr_[i] = value;
            }
        }
    }

    /**
     * @brief Copy from host array
     */
    void copyFrom(const T* src, size_t count) {
        if (!ptr_ || !src || count == 0) return;
        size_t copy_count = std::min(count, size_);
        std::copy(src, src + copy_count, ptr_);
    }

    /**
     * @brief Copy to host array
     */
    void copyTo(T* dst, size_t count) const {
        if (!ptr_ || !dst || count == 0) return;
        size_t copy_count = std::min(count, size_);
        std::copy(ptr_, ptr_ + copy_count, dst);
    }

private:
    T* ptr_;
    size_t size_;
    bool owns_memory_;
};

/**
 * @brief Unified memory vector (STL-like interface)
 */
template<typename T>
class UnifiedVector {
public:
    UnifiedVector() = default;

    explicit UnifiedVector(size_t count, const T& value = T())
        : data_(count) {
        data_.fill(value);
    }

    // Size operations
    size_t size() const { return data_.size(); }
    bool empty() const { return size() == 0; }

    // Element access
    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }

    T& at(size_t idx) {
        if (idx >= size()) {
            throw std::out_of_range("UnifiedVector index out of range");
        }
        return data_[idx];
    }

    const T& at(size_t idx) const {
        if (idx >= size()) {
            throw std::out_of_range("UnifiedVector index out of range");
        }
        return data_[idx];
    }

    // Raw pointer access
    T* data() { return data_.get(); }
    const T* data() const { return data_.get(); }

    // Memory management
    void prefetchToDevice(const Device& device) {
        data_.prefetchToDevice(device);
    }

    void prefetchToHost() {
        data_.prefetchToHost();
    }

    void setPreferredLocationDevice(const Device& device) {
        data_.setPreferredLocationDevice(device);
    }

    void setPreferredLocationHost() {
        data_.setPreferredLocationHost();
    }

    void setReadMostly() {
        data_.setReadMostly();
    }

    void setAccessedBy(const Device& device) {
        data_.setAccessedBy(device);
    }

    // Fill with value
    void fill(const T& value) {
        data_.fill(value);
    }

    // STL compatibility
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    pointer begin() { return data_.get(); }
    const_pointer begin() const { return data_.get(); }
    pointer end() { return data_.get() + size(); }
    const_pointer end() const { return data_.get() + size(); }

private:
    UnifiedPtr<T> data_;
};

/**
 * @brief Helper functions for unified memory
 */

/// Create unified pointer
template<typename T>
UnifiedPtr<T> make_unified(size_t count, UnifiedFlags flags = UnifiedFlags::Default) {
    return UnifiedPtr<T>(count, flags);
}

/// Create unified vector
template<typename T>
UnifiedVector<T> make_unified_vector(size_t count, const T& value = T()) {
    return UnifiedVector<T>(count, value);
}

/// Check if unified memory is supported
inline bool isUnifiedMemorySupported() {
#ifdef KOO_USE_CUDA
    int device_count;
    cudaGetDeviceCount(&device_count);

    for (int i = 0; i < device_count; ++i) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        if (prop.managedMemory) {
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}

/// Get concurrent access supported
inline bool isConcurrentAccessSupported() {
#ifdef KOO_USE_CUDA
    int device_count;
    cudaGetDeviceCount(&device_count);

    for (int i = 0; i < device_count; ++i) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        if (prop.concurrentManagedAccess) {
            return true;
        }
    }
    return false;
#else
    return false;
#endif
}

}  // namespace memory
}  // namespace gpu
}  // namespace koo
