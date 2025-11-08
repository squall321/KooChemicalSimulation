/**
 * @file AsyncMemory.h
 * @brief Asynchronous memory operations with streams
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 61: GPU Memory Optimization
 *
 * Features:
 * - Non-blocking memory transfers
 * - Stream-based async operations
 * - Pinned memory for faster transfers
 * - Memory transfer timing and profiling
 */

#pragma once

#include "../Device.h"
#include "../Stream.h"
#include <chrono>
#include <stdexcept>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace koo {
namespace gpu {
namespace memory {

/**
 * @brief Memory transfer direction
 */
enum class TransferDirection {
    HostToDevice,
    DeviceToHost,
    DeviceToDevice
};

/**
 * @brief Memory transfer statistics
 */
struct TransferStats {
    size_t bytes_transferred;
    double duration_ms;
    double bandwidth_gbps;

    TransferStats() : bytes_transferred(0), duration_ms(0.0), bandwidth_gbps(0.0) {}

    TransferStats(size_t bytes, double ms)
        : bytes_transferred(bytes), duration_ms(ms) {
        // Calculate bandwidth in GB/s
        if (ms > 0.0) {
            bandwidth_gbps = (bytes / 1e9) / (ms / 1000.0);
        } else {
            bandwidth_gbps = 0.0;
        }
    }
};

/**
 * @brief Pinned (page-locked) host memory for faster transfers
 */
template<typename T>
class PinnedMemory {
public:
    /**
     * @brief Construct empty pinned memory
     */
    PinnedMemory() : ptr_(nullptr), size_(0) {}

    /**
     * @brief Allocate pinned host memory
     */
    explicit PinnedMemory(size_t count) : ptr_(nullptr), size_(count) {
#ifdef KOO_USE_CUDA
        if (count > 0) {
            cudaError_t err = cudaMallocHost(&ptr_, count * sizeof(T));
            if (err != cudaSuccess) {
                throw std::runtime_error("Failed to allocate pinned memory: " +
                                       std::string(cudaGetErrorString(err)));
            }
        }
#else
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Destructor
     */
    ~PinnedMemory() {
        if (ptr_) {
#ifdef KOO_USE_CUDA
            cudaFreeHost(ptr_);
#endif
        }
    }

    // Move semantics
    PinnedMemory(PinnedMemory&& other) noexcept
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    PinnedMemory& operator=(PinnedMemory&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
#ifdef KOO_USE_CUDA
                cudaFreeHost(ptr_);
#endif
            }
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // No copy
    PinnedMemory(const PinnedMemory&) = delete;
    PinnedMemory& operator=(const PinnedMemory&) = delete;

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
     * @brief Get size
     */
    size_t size() const { return size_; }
    size_t size_bytes() const { return size_ * sizeof(T); }

    /**
     * @brief Check if valid
     */
    explicit operator bool() const { return ptr_ != nullptr; }

private:
    T* ptr_;
    size_t size_;
};

/**
 * @brief Asynchronous memory operations
 */
class AsyncMemoryOps {
public:
    /**
     * @brief Async copy from host to device
     */
    template<typename T>
    static void copyH2DAsync(const T* src, T* dst, size_t count,
                             const Stream& stream) {
#ifdef KOO_USE_CUDA
        cudaError_t err = cudaMemcpyAsync(dst, src, count * sizeof(T),
                                         cudaMemcpyHostToDevice, stream.get());
        if (err != cudaSuccess) {
            throw std::runtime_error("Async H2D copy failed: " +
                                   std::string(cudaGetErrorString(err)));
        }
#else
        (void)src; (void)dst; (void)count; (void)stream;
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Async copy from device to host
     */
    template<typename T>
    static void copyD2HAsync(const T* src, T* dst, size_t count,
                             const Stream& stream) {
#ifdef KOO_USE_CUDA
        cudaError_t err = cudaMemcpyAsync(dst, src, count * sizeof(T),
                                         cudaMemcpyDeviceToHost, stream.get());
        if (err != cudaSuccess) {
            throw std::runtime_error("Async D2H copy failed: " +
                                   std::string(cudaGetErrorString(err)));
        }
#else
        (void)src; (void)dst; (void)count; (void)stream;
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Async copy from device to device
     */
    template<typename T>
    static void copyD2DAsync(const T* src, T* dst, size_t count,
                             const Stream& stream) {
#ifdef KOO_USE_CUDA
        cudaError_t err = cudaMemcpyAsync(dst, src, count * sizeof(T),
                                         cudaMemcpyDeviceToDevice, stream.get());
        if (err != cudaSuccess) {
            throw std::runtime_error("Async D2D copy failed: " +
                                   std::string(cudaGetErrorString(err)));
        }
#else
        (void)src; (void)dst; (void)count; (void)stream;
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Async memset on device
     */
    static void memsetAsync(void* ptr, int value, size_t count,
                           const Stream& stream) {
#ifdef KOO_USE_CUDA
        cudaError_t err = cudaMemsetAsync(ptr, value, count, stream.get());
        if (err != cudaSuccess) {
            throw std::runtime_error("Async memset failed: " +
                                   std::string(cudaGetErrorString(err)));
        }
#else
        (void)ptr; (void)value; (void)count; (void)stream;
        throw std::runtime_error("CUDA not available");
#endif
    }

    /**
     * @brief Timed copy from host to device
     */
    template<typename T>
    static TransferStats copyH2DTimed(const T* src, T* dst, size_t count,
                                      const Stream& stream) {
        auto start = std::chrono::high_resolution_clock::now();

        copyH2DAsync(src, dst, count, stream);
        stream.synchronize();

        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        return TransferStats(count * sizeof(T), ms);
    }

    /**
     * @brief Timed copy from device to host
     */
    template<typename T>
    static TransferStats copyD2HTimed(const T* src, T* dst, size_t count,
                                      const Stream& stream) {
        auto start = std::chrono::high_resolution_clock::now();

        copyD2HAsync(src, dst, count, stream);
        stream.synchronize();

        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        return TransferStats(count * sizeof(T), ms);
    }

    /**
     * @brief Benchmark memory bandwidth
     */
    static TransferStats benchmarkBandwidth(size_t bytes,
                                           TransferDirection direction,
                                           const Device& device) {
#ifdef KOO_USE_CUDA
        device.setCurrent();
        Stream stream(device);

        // Allocate memory
        void* d_ptr = nullptr;
        void* h_ptr = nullptr;
        cudaMalloc(&d_ptr, bytes);
        cudaMallocHost(&h_ptr, bytes);

        TransferStats stats;

        auto start = std::chrono::high_resolution_clock::now();

        switch (direction) {
            case TransferDirection::HostToDevice:
                cudaMemcpyAsync(d_ptr, h_ptr, bytes, cudaMemcpyHostToDevice,
                              stream.get());
                break;
            case TransferDirection::DeviceToHost:
                cudaMemcpyAsync(h_ptr, d_ptr, bytes, cudaMemcpyDeviceToHost,
                              stream.get());
                break;
            case TransferDirection::DeviceToDevice:
                void* d_ptr2;
                cudaMalloc(&d_ptr2, bytes);
                cudaMemcpyAsync(d_ptr2, d_ptr, bytes, cudaMemcpyDeviceToDevice,
                              stream.get());
                cudaFree(d_ptr2);
                break;
        }

        stream.synchronize();
        auto end = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        stats = TransferStats(bytes, ms);

        // Cleanup
        cudaFree(d_ptr);
        cudaFreeHost(h_ptr);

        return stats;
#else
        (void)bytes; (void)direction; (void)device;
        return TransferStats();
#endif
    }
};

/**
 * @brief Double-buffered host memory for overlapping transfers
 */
template<typename T>
class DoubleBuffer {
public:
    DoubleBuffer(size_t count)
        : buffer0_(count), buffer1_(count), active_(0) {}

    /**
     * @brief Get active buffer for writing
     */
    T* getActive() {
        return (active_ == 0) ? buffer0_.get() : buffer1_.get();
    }

    /**
     * @brief Get inactive buffer for async transfer
     */
    T* getInactive() {
        return (active_ == 0) ? buffer1_.get() : buffer0_.get();
    }

    /**
     * @brief Swap buffers
     */
    void swap() {
        active_ = 1 - active_;
    }

    /**
     * @brief Get size
     */
    size_t size() const { return buffer0_.size(); }

private:
    PinnedMemory<T> buffer0_;
    PinnedMemory<T> buffer1_;
    int active_;
};

/**
 * @brief Helper functions
 */

/// Create pinned memory
template<typename T>
PinnedMemory<T> make_pinned(size_t count) {
    return PinnedMemory<T>(count);
}

/// Create double buffer
template<typename T>
DoubleBuffer<T> make_double_buffer(size_t count) {
    return DoubleBuffer<T>(count);
}

}  // namespace memory
}  // namespace gpu
}  // namespace koo
