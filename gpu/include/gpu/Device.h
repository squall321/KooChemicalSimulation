/**
 * @file Device.h
 * @brief GPU device management and abstraction
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 51: GPU Abstraction Layer
 *
 * Provides unified API for CUDA and HIP, with CPU fallback.
 * Supports NVIDIA GPUs (CUDA 11.0+) and AMD GPUs (ROCm 5.0+).
 */

#ifndef KOO_GPU_DEVICE_H
#define KOO_GPU_DEVICE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <sstream>

// Detect GPU backend
#if defined(__CUDACC__) || defined(__NVCC__)
    #define KOO_CUDA_ENABLED
    #include <cuda_runtime.h>
    #define KOO_GPU_RUNTIME "CUDA"
#elif defined(__HIP_PLATFORM_AMD__) || defined(__HIP__)
    #define KOO_HIP_ENABLED
    #include <hip/hip_runtime.h>
    #define KOO_GPU_RUNTIME "HIP"
    // HIP uses hip prefix instead of cuda prefix
    #define cudaError_t hipError_t
    #define cudaSuccess hipSuccess
    #define cudaGetDeviceCount hipGetDeviceCount
    #define cudaGetDeviceProperties hipGetDeviceProperties
    #define cudaDeviceProp hipDeviceProp_t
    #define cudaSetDevice hipSetDevice
    #define cudaGetDevice hipGetDevice
    #define cudaDeviceSynchronize hipDeviceSynchronize
    #define cudaGetLastError hipGetLastError
    #define cudaGetErrorString hipGetErrorString
#else
    #define KOO_CPU_FALLBACK
    #define KOO_GPU_RUNTIME "CPU"
#endif

namespace koo {
namespace gpu {

/**
 * @class GPUError
 * @brief Exception for GPU errors
 */
class GPUError : public std::runtime_error {
public:
    explicit GPUError(const std::string& message)
        : std::runtime_error("GPU Error: " + message) {}
};

/**
 * @brief Check GPU API call and throw on error
 */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    #define CHECK_GPU(call) \
        do { \
            cudaError_t err = call; \
            if (err != cudaSuccess) { \
                std::ostringstream oss; \
                oss << cudaGetErrorString(err) \
                    << " at " << __FILE__ << ":" << __LINE__; \
                throw koo::gpu::GPUError(oss.str()); \
            } \
        } while(0)
#else
    #define CHECK_GPU(call) call
#endif

/**
 * @struct DeviceProperties
 * @brief GPU device properties
 */
struct DeviceProperties {
    std::string name;              ///< Device name
    int major{0};                  ///< Compute capability major
    int minor{0};                  ///< Compute capability minor
    size_t totalMemory{0};         ///< Total global memory (bytes)
    size_t sharedMemPerBlock{0};   ///< Shared memory per block (bytes)
    int maxThreadsPerBlock{0};     ///< Max threads per block
    int multiProcessorCount{0};    ///< Number of SMs/CUs
    int warpSize{32};              ///< Warp/wavefront size
    bool managedMemory{false};     ///< Supports unified memory
    bool concurrentKernels{false}; ///< Concurrent kernel execution

    /**
     * @brief Get human-readable string
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "GPU Device: " << name << "\n"
            << "  Compute Capability: " << major << "." << minor << "\n"
            << "  Total Memory: " << (totalMemory / (1024*1024*1024.0)) << " GB\n"
            << "  Multiprocessors: " << multiProcessorCount << "\n"
            << "  Max Threads/Block: " << maxThreadsPerBlock << "\n"
            << "  Warp Size: " << warpSize << "\n"
            << "  Managed Memory: " << (managedMemory ? "Yes" : "No") << "\n"
            << "  Concurrent Kernels: " << (concurrentKernels ? "Yes" : "No");
        return oss.str();
    }
};

/**
 * @class Device
 * @brief GPU device management
 *
 * Phase 51: GPU Device Abstraction
 *
 * Manages GPU device selection, queries, and synchronization.
 * Provides unified interface for CUDA and HIP.
 */
class Device {
public:
    /**
     * @brief Get number of available GPU devices
     * @return Number of GPUs (0 if none or CPU-only mode)
     */
    static int getDeviceCount() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        int count = 0;
        CHECK_GPU(cudaGetDeviceCount(&count));
        return count;
#else
        return 0;  // CPU-only mode
#endif
    }

    /**
     * @brief Get device instance
     * @param deviceId Device ID (0-based)
     * @return Device instance
     */
    static Device getDevice(int deviceId = 0) {
        return Device(deviceId);
    }

    /**
     * @brief Constructor
     * @param deviceId Device ID
     */
    explicit Device(int deviceId = 0) : deviceId_(deviceId) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        int count = getDeviceCount();
        if (deviceId < 0 || deviceId >= count) {
            throw GPUError("Invalid device ID: " + std::to_string(deviceId));
        }
        CHECK_GPU(cudaSetDevice(deviceId));

        // Query properties
        cudaDeviceProp prop;
        CHECK_GPU(cudaGetDeviceProperties(&prop, deviceId));

        properties_.name = prop.name;
        properties_.major = prop.major;
        properties_.minor = prop.minor;
        properties_.totalMemory = prop.totalGlobalMem;
        properties_.sharedMemPerBlock = prop.sharedMemPerBlock;
        properties_.maxThreadsPerBlock = prop.maxThreadsPerBlock;
        properties_.multiProcessorCount = prop.multiProcessorCount;
        properties_.warpSize = prop.warpSize;
        properties_.managedMemory = (prop.managedMemory != 0);
        properties_.concurrentKernels = (prop.concurrentKernels != 0);
#else
        // CPU fallback
        properties_.name = "CPU (No GPU)";
        properties_.major = 0;
        properties_.minor = 0;
#endif
    }

    /**
     * @brief Get device ID
     */
    int getId() const { return deviceId_; }

    /**
     * @brief Get device name
     */
    std::string getName() const { return properties_.name; }

    /**
     * @brief Get total device memory (bytes)
     */
    size_t getTotalMemory() const { return properties_.totalMemory; }

    /**
     * @brief Get compute capability
     * @return {major, minor}
     */
    std::pair<int, int> getComputeCapability() const {
        return {properties_.major, properties_.minor};
    }

    /**
     * @brief Get device properties
     */
    const DeviceProperties& getProperties() const {
        return properties_;
    }

    /**
     * @brief Set this device as current
     */
    void setCurrent() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaSetDevice(deviceId_));
#endif
    }

    /**
     * @brief Get current device ID
     */
    static int getCurrentDeviceId() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        int deviceId;
        CHECK_GPU(cudaGetDevice(&deviceId));
        return deviceId;
#else
        return -1;  // No GPU
#endif
    }

    /**
     * @brief Synchronize device (wait for all kernels to complete)
     */
    void synchronize() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_GPU(cudaDeviceSynchronize());
#endif
    }

    /**
     * @brief Check if GPU is available
     */
    static bool isGPUAvailable() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        return getDeviceCount() > 0;
#else
        return false;
#endif
    }

    /**
     * @brief Get GPU runtime (CUDA, HIP, or CPU)
     */
    static std::string getRuntime() {
        return KOO_GPU_RUNTIME;
    }

    /**
     * @brief Check if this is a CUDA device
     */
    static bool isCUDA() {
#ifdef KOO_CUDA_ENABLED
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Check if this is a HIP device
     */
    static bool isHIP() {
#ifdef KOO_HIP_ENABLED
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Check if running on CPU (no GPU)
     */
    static bool isCPU() {
#ifdef KOO_CPU_FALLBACK
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Get available memory on device (bytes)
     * @return {free, total}
     */
    std::pair<size_t, size_t> getMemoryInfo() const {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        size_t free, total;
        CHECK_GPU(cudaMemGetInfo(&free, &total));
        return {free, total};
#else
        return {0, 0};
#endif
    }

    /**
     * @brief Print device information
     */
    void printInfo() const {
        std::cout << properties_.toString() << std::endl;
    }

    /**
     * @brief Get all available devices
     */
    static std::vector<Device> getAllDevices() {
        std::vector<Device> devices;
        int count = getDeviceCount();
        for (int i = 0; i < count; ++i) {
            devices.emplace_back(i);
        }
        return devices;
    }

private:
    int deviceId_;
    DeviceProperties properties_;
};

/**
 * @class DeviceGuard
 * @brief RAII guard for device switching
 *
 * Automatically restores previous device on destruction.
 */
class DeviceGuard {
public:
    /**
     * @brief Constructor - set new device
     * @param deviceId Device to switch to
     */
    explicit DeviceGuard(int deviceId) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        previousDevice_ = Device::getCurrentDeviceId();
        if (deviceId != previousDevice_) {
            CHECK_GPU(cudaSetDevice(deviceId));
        }
#endif
    }

    /**
     * @brief Destructor - restore previous device
     */
    ~DeviceGuard() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        cudaSetDevice(previousDevice_);  // Don't throw in destructor
#endif
    }

    // Non-copyable
    DeviceGuard(const DeviceGuard&) = delete;
    DeviceGuard& operator=(const DeviceGuard&) = delete;

private:
    int previousDevice_{0};
};

} // namespace gpu
} // namespace koo

#endif // KOO_GPU_DEVICE_H
