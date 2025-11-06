/**
 * @file Stream.h
 * @brief GPU stream management for asynchronous execution
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 51: GPU Abstraction Layer
 *
 * Provides RAII-based GPU stream management for asynchronous
 * kernel execution and memory transfers.
 */

#ifndef KOO_GPU_STREAM_H
#define KOO_GPU_STREAM_H

#include "Device.h"
#include "Memory.h"

namespace koo {
namespace gpu {

/**
 * @class Stream
 * @brief RAII wrapper for GPU streams
 *
 * Phase 51: GPU Stream Management
 *
 * Enables asynchronous kernel execution and memory transfers.
 * Multiple streams can execute concurrently on the GPU.
 */
class Stream {
public:
    /**
     * @brief Constructor - create stream
     * @param flags Stream creation flags (0 = default)
     */
    explicit Stream(unsigned int flags = 0) : stream_(nullptr) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaStreamCreate(&stream_));
#endif
    }

    /**
     * @brief Destructor - destroy stream
     */
    ~Stream() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        if (stream_ != nullptr) {
            cudaStreamDestroy(stream_);  // Don't check error in destructor
        }
#endif
    }

    // Move semantics
    Stream(Stream&& other) noexcept : stream_(other.stream_) {
        other.stream_ = nullptr;
    }

    Stream& operator=(Stream&& other) noexcept {
        if (this != &other) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
            if (stream_ != nullptr) {
                cudaStreamDestroy(stream_);
            }
#endif
            stream_ = other.stream_;
            other.stream_ = nullptr;
        }
        return *this;
    }

    // Non-copyable
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    /**
     * @brief Synchronize stream (wait for all operations)
     */
    void synchronize() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaStreamSynchronize(stream_));
#endif
    }

    /**
     * @brief Check if all operations in stream are complete
     * @return True if complete, false if still running
     */
    bool query() const {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        cudaError_t err = cudaStreamQuery(stream_);
        if (err == cudaSuccess) {
            return true;  // Stream is idle
        } else if (err == cudaErrorNotReady) {
            return false;  // Stream is busy
        } else {
            CHECK_GPU(err);  // Throw on other errors
            return false;
        }
#else
        return true;  // CPU mode - always complete
#endif
    }

    /**
     * @brief Wait for an event on this stream
     */
    void waitEvent(void* event) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaStreamWaitEvent(stream_,
                                     reinterpret_cast<cudaEvent_t>(event), 0));
#endif
    }

    /**
     * @brief Get native stream handle
     */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    cudaStream_t get() const { return stream_; }
#else
    void* get() const { return nullptr; }
#endif

    /**
     * @brief Async memory copy (host to device)
     */
    template<typename T>
    void copyFromHostAsync(DeviceMemory<T>& deviceMem,
                          const T* hostPtr,
                          size_t count) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpyAsync(deviceMem.data(), hostPtr,
                                 count * sizeof(T),
                                 cudaMemcpyHostToDevice,
                                 stream_));
#else
        deviceMem.copyFromHost(hostPtr, count);
#endif
    }

    /**
     * @brief Async memory copy (device to host)
     */
    template<typename T>
    void copyToHostAsync(const DeviceMemory<T>& deviceMem,
                        T* hostPtr,
                        size_t count) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpyAsync(hostPtr, deviceMem.data(),
                                 count * sizeof(T),
                                 cudaMemcpyDeviceToHost,
                                 stream_));
#else
        deviceMem.copyToHost(hostPtr, count);
#endif
    }

    /**
     * @brief Async memory copy (device to device)
     */
    template<typename T>
    void copyDeviceAsync(DeviceMemory<T>& dst,
                        const DeviceMemory<T>& src,
                        size_t count) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemcpyAsync(dst.data(), src.data(),
                                 count * sizeof(T),
                                 cudaMemcpyDeviceToDevice,
                                 stream_));
#else
        dst.copyFromDevice(src, count);
#endif
    }

    /**
     * @brief Async memset
     */
    template<typename T>
    void memsetAsync(DeviceMemory<T>& deviceMem, int value, size_t count) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaMemsetAsync(deviceMem.data(), value,
                                 count * sizeof(T), stream_));
#else
        deviceMem.fill(static_cast<T>(value));
#endif
    }

private:
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    cudaStream_t stream_;
#else
    void* stream_;
#endif
};

/**
 * @class Event
 * @brief GPU event for timing and synchronization
 *
 * Phase 51: GPU Event Management
 *
 * Events can be used to:
 * - Time GPU operations
 * - Synchronize between streams
 * - Create dependencies between operations
 */
class Event {
public:
    /**
     * @brief Constructor - create event
     * @param flags Event creation flags
     */
    explicit Event(unsigned int flags = 0) : event_(nullptr) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaEventCreate(&event_));
#endif
    }

    /**
     * @brief Destructor - destroy event
     */
    ~Event() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        if (event_ != nullptr) {
            cudaEventDestroy(event_);
        }
#endif
    }

    // Move semantics
    Event(Event&& other) noexcept : event_(other.event_) {
        other.event_ = nullptr;
    }

    Event& operator=(Event&& other) noexcept {
        if (this != &other) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
            if (event_ != nullptr) {
                cudaEventDestroy(event_);
            }
#endif
            event_ = other.event_;
            other.event_ = nullptr;
        }
        return *this;
    }

    // Non-copyable
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    /**
     * @brief Record event in stream
     */
    void record(const Stream& stream) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaEventRecord(event_, stream.get()));
#endif
    }

    /**
     * @brief Record event in default stream
     */
    void record() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaEventRecord(event_, nullptr));
#endif
    }

    /**
     * @brief Synchronize on event (wait for completion)
     */
    void synchronize() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaEventSynchronize(event_));
#endif
    }

    /**
     * @brief Check if event has occurred
     */
    bool query() const {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        cudaError_t err = cudaEventQuery(event_);
        if (err == cudaSuccess) {
            return true;
        } else if (err == cudaErrorNotReady) {
            return false;
        } else {
            CHECK_GPU(err);
            return false;
        }
#else
        return true;
#endif
    }

    /**
     * @brief Compute elapsed time between two events (ms)
     */
    static float elapsedTime(const Event& start, const Event& end) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        float ms = 0.0f;
        CHECK_GPU(cudaEventElapsedTime(&ms, start.event_, end.event_));
        return ms;
#else
        return 0.0f;
#endif
    }

    /**
     * @brief Get native event handle
     */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    cudaEvent_t get() const { return event_; }
#else
    void* get() const { return nullptr; }
#endif

private:
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    cudaEvent_t event_;
#else
    void* event_;
#endif
};

/**
 * @class StreamPool
 * @brief Pool of reusable streams
 *
 * Phase 51: Stream Management
 *
 * Manages a pool of streams for efficient reuse.
 */
class StreamPool {
public:
    /**
     * @brief Constructor
     * @param poolSize Number of streams in pool
     */
    explicit StreamPool(size_t poolSize = 4) {
        streams_.reserve(poolSize);
        for (size_t i = 0; i < poolSize; ++i) {
            streams_.emplace_back();
        }
    }

    /**
     * @brief Get stream by index
     */
    Stream& get(size_t index) {
        if (index >= streams_.size()) {
            throw GPUError("Stream index out of range");
        }
        return streams_[index];
    }

    /**
     * @brief Get number of streams
     */
    size_t size() const { return streams_.size(); }

    /**
     * @brief Synchronize all streams
     */
    void synchronizeAll() {
        for (auto& stream : streams_) {
            stream.synchronize();
        }
    }

    /**
     * @brief Get next stream (round-robin)
     */
    Stream& getNext() {
        size_t index = nextIndex_;
        nextIndex_ = (nextIndex_ + 1) % streams_.size();
        return streams_[index];
    }

private:
    std::vector<Stream> streams_;
    size_t nextIndex_{0};
};

/**
 * @class ScopedTimer
 * @brief RAII timer using GPU events
 *
 * Example:
 * {
 *     ScopedTimer timer("Kernel execution");
 *     // ... GPU operations ...
 * } // Prints elapsed time
 */
class ScopedTimer {
public:
    /**
     * @brief Constructor - start timer
     */
    explicit ScopedTimer(const std::string& name = "GPU Operation")
        : name_(name) {
        startEvent_.record();
    }

    /**
     * @brief Destructor - stop timer and print
     */
    ~ScopedTimer() {
        stopEvent_.record();
        stopEvent_.synchronize();
        float ms = Event::elapsedTime(startEvent_, stopEvent_);
        std::cout << name_ << ": " << ms << " ms" << std::endl;
    }

    /**
     * @brief Get elapsed time so far (ms)
     */
    float elapsed() {
        stopEvent_.record();
        stopEvent_.synchronize();
        return Event::elapsedTime(startEvent_, stopEvent_);
    }

private:
    std::string name_;
    Event startEvent_;
    Event stopEvent_;
};

} // namespace gpu
} // namespace koo

#endif // KOO_GPU_STREAM_H
