/**
 * @file Vector.h
 * @brief GPU vector operations with BLAS support
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 * @date 2025-11-06
 *
 * Phase 52: GPU Linear Algebra
 *
 * Provides GPU-accelerated vector operations using cuBLAS/rocBLAS.
 * Supports BLAS Level 1 operations with automatic CPU fallback.
 */

#ifndef KOO_GPU_LINALG_VECTOR_H
#define KOO_GPU_LINALG_VECTOR_H

#include "../Device.h"
#include "../Memory.h"
#include "../Stream.h"
#include <vector>
#include <cmath>
#include <algorithm>

// BLAS includes
#if defined(KOO_CUDA_ENABLED)
    #include <cublas_v2.h>
    #define KOO_BLAS_HANDLE cublasHandle_t
    #define KOO_BLAS_STATUS cublasStatus_t
    #define KOO_BLAS_SUCCESS CUBLAS_STATUS_SUCCESS
#elif defined(KOO_HIP_ENABLED)
    #include <rocblas/rocblas.h>
    #define KOO_BLAS_HANDLE rocblas_handle
    #define KOO_BLAS_STATUS rocblas_status
    #define KOO_BLAS_SUCCESS rocblas_status_success
    #define cublasCreate rocblas_create_handle
    #define cublasDestroy rocblas_destroy_handle
    #define cublasSetStream rocblas_set_stream
#endif

namespace koo {
namespace gpu {
namespace linalg {

/**
 * @class BLASError
 * @brief Exception for BLAS errors
 */
class BLASError : public std::runtime_error {
public:
    explicit BLASError(const std::string& message)
        : std::runtime_error("BLAS Error: " + message) {}
};

/**
 * @brief Check BLAS call and throw on error
 */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    #define CHECK_BLAS(call) \
        do { \
            KOO_BLAS_STATUS status = call; \
            if (status != KOO_BLAS_SUCCESS) { \
                throw koo::gpu::linalg::BLASError( \
                    std::string(#call) + " failed with status " + \
                    std::to_string(static_cast<int>(status))); \
            } \
        } while(0)
#else
    #define CHECK_BLAS(call) call
#endif

/**
 * @class BLASHandle
 * @brief RAII wrapper for BLAS handle
 *
 * Phase 52: BLAS Integration
 *
 * Manages cuBLAS/rocBLAS handle lifecycle.
 */
class BLASHandle {
public:
    /**
     * @brief Constructor - create BLAS handle
     */
    BLASHandle() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_BLAS(cublasCreate(&handle_));
#else
        handle_ = nullptr;
#endif
    }

    /**
     * @brief Destructor - destroy BLAS handle
     */
    ~BLASHandle() {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        if (handle_ != nullptr) {
            cublasDestroy(handle_);
        }
#endif
    }

    /**
     * @brief Set stream for BLAS operations
     */
    void setStream(const Stream& stream) {
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        CHECK_BLAS(cublasSetStream(handle_, stream.get()));
#endif
    }

    /**
     * @brief Get native handle
     */
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    KOO_BLAS_HANDLE get() const { return handle_; }
#else
    void* get() const { return nullptr; }
#endif

    /**
     * @brief Get global BLAS handle (singleton)
     */
    static BLASHandle& getGlobal() {
        static BLASHandle globalHandle;
        return globalHandle;
    }

    // Non-copyable
    BLASHandle(const BLASHandle&) = delete;
    BLASHandle& operator=(const BLASHandle&) = delete;

private:
#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
    KOO_BLAS_HANDLE handle_;
#else
    void* handle_;
#endif
};

/**
 * @class GPUVector
 * @brief GPU vector with BLAS operations
 *
 * Phase 52: GPU Linear Algebra
 *
 * Provides BLAS Level 1 operations on GPU vectors.
 * Automatically uses cuBLAS (CUDA) or rocBLAS (HIP).
 *
 * @tparam T Element type (float or double)
 */
template<typename T>
class GPUVector {
public:
    /**
     * @brief Constructor - allocate GPU vector
     * @param size Vector size
     */
    explicit GPUVector(size_t size = 0)
        : size_(size), data_(size) {
        if (size > 0) {
            data_.zero();
        }
    }

    /**
     * @brief Constructor from host vector
     */
    explicit GPUVector(const std::vector<T>& hostData)
        : size_(hostData.size()), data_(hostData.size()) {
        data_.copyFromHost(hostData.data());
    }

    /**
     * @brief Get size
     */
    size_t size() const { return size_; }

    /**
     * @brief Check if empty
     */
    bool empty() const { return size_ == 0; }

    /**
     * @brief Get device memory
     */
    DeviceMemory<T>& data() { return data_; }
    const DeviceMemory<T>& data() const { return data_; }

    /**
     * @brief Get raw pointer
     */
    T* ptr() { return data_.data(); }
    const T* ptr() const { return data_.data(); }

    /**
     * @brief Copy to host vector
     */
    std::vector<T> toHost() const {
        std::vector<T> result(size_);
        data_.copyToHost(result.data());
        return result;
    }

    /**
     * @brief Copy from host vector
     */
    void fromHost(const std::vector<T>& hostData) {
        if (hostData.size() != size_) {
            throw std::runtime_error("Size mismatch in fromHost");
        }
        data_.copyFromHost(hostData.data());
    }

    /**
     * @brief Zero-fill vector
     */
    void zero() {
        data_.zero();
    }

    /**
     * @brief Fill with constant value
     */
    void fill(T value) {
        data_.fill(value);
    }

    /**
     * @brief Resize vector
     */
    void resize(size_t newSize) {
        size_ = newSize;
        data_.resize(newSize);
    }

    // ============================================
    // BLAS Level 1 Operations
    // ============================================

    /**
     * @brief Scale vector: y = alpha * x
     * @param alpha Scalar
     */
    void scale(T alpha) {
#if defined(KOO_CUDA_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(cublasSscal(handle.get(), n, &alpha, ptr(), 1));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(cublasDscal(handle.get(), n, &alpha, ptr(), 1));
        }
#elif defined(KOO_HIP_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(rocblas_sscal(handle.get(), n, &alpha, ptr(), 1));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(rocblas_dscal(handle.get(), n, &alpha, ptr(), 1));
        }
#else
        // CPU fallback
        auto hostData = toHost();
        for (auto& val : hostData) val *= alpha;
        fromHost(hostData);
#endif
    }

    /**
     * @brief AXPY: y = alpha * x + y
     * @param alpha Scalar
     * @param x Input vector
     */
    void axpy(T alpha, const GPUVector<T>& x) {
        if (x.size() != size_) {
            throw std::runtime_error("Vector size mismatch in axpy");
        }

#if defined(KOO_CUDA_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(cublasSaxpy(handle.get(), n, &alpha, x.ptr(), 1, ptr(), 1));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(cublasDaxpy(handle.get(), n, &alpha, x.ptr(), 1, ptr(), 1));
        }
#elif defined(KOO_HIP_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(rocblas_saxpy(handle.get(), n, &alpha, x.ptr(), 1, ptr(), 1));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(rocblas_daxpy(handle.get(), n, &alpha, x.ptr(), 1, ptr(), 1));
        }
#else
        // CPU fallback
        auto hostX = x.toHost();
        auto hostY = toHost();
        for (size_t i = 0; i < size_; ++i) {
            hostY[i] += alpha * hostX[i];
        }
        fromHost(hostY);
#endif
    }

    /**
     * @brief Dot product: result = x^T * y
     * @param other Other vector
     * @return Dot product
     */
    T dot(const GPUVector<T>& other) const {
        if (other.size() != size_) {
            throw std::runtime_error("Vector size mismatch in dot");
        }

        T result = 0;

#if defined(KOO_CUDA_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(cublasSdot(handle.get(), n, ptr(), 1, other.ptr(), 1, &result));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(cublasDdot(handle.get(), n, ptr(), 1, other.ptr(), 1, &result));
        }
#elif defined(KOO_HIP_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(rocblas_sdot(handle.get(), n, ptr(), 1, other.ptr(), 1, &result));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(rocblas_ddot(handle.get(), n, ptr(), 1, other.ptr(), 1, &result));
        }
#else
        // CPU fallback
        auto hostX = toHost();
        auto hostY = other.toHost();
        for (size_t i = 0; i < size_; ++i) {
            result += hostX[i] * hostY[i];
        }
#endif

        return result;
    }

    /**
     * @brief L2 norm: ||x||_2
     * @return L2 norm
     */
    T norm2() const {
        T result = 0;

#if defined(KOO_CUDA_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(cublasSnrm2(handle.get(), n, ptr(), 1, &result));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(cublasDnrm2(handle.get(), n, ptr(), 1, &result));
        }
#elif defined(KOO_HIP_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(rocblas_snrm2(handle.get(), n, ptr(), 1, &result));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(rocblas_dnrm2(handle.get(), n, ptr(), 1, &result));
        }
#else
        // CPU fallback
        auto hostData = toHost();
        for (const auto& val : hostData) {
            result += val * val;
        }
        result = std::sqrt(result);
#endif

        return result;
    }

    /**
     * @brief L1 norm: ||x||_1
     * @return L1 norm
     */
    T norm1() const {
        T result = 0;

#if defined(KOO_CUDA_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(cublasSasum(handle.get(), n, ptr(), 1, &result));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(cublasDasum(handle.get(), n, ptr(), 1, &result));
        }
#elif defined(KOO_HIP_ENABLED)
        auto& handle = BLASHandle::getGlobal();
        int n = static_cast<int>(size_);
        if constexpr (std::is_same_v<T, float>) {
            CHECK_BLAS(rocblas_sasum(handle.get(), n, ptr(), 1, &result));
        } else if constexpr (std::is_same_v<T, double>) {
            CHECK_BLAS(rocblas_dasum(handle.get(), n, ptr(), 1, &result));
        }
#else
        // CPU fallback
        auto hostData = toHost();
        for (const auto& val : hostData) {
            result += std::abs(val);
        }
#endif

        return result;
    }

    /**
     * @brief Infinity norm: ||x||_inf
     * @return Infinity norm
     */
    T normInf() const {
        T result = 0;

#if defined(KOO_CUDA_ENABLED) || defined(KOO_HIP_ENABLED)
        // cuBLAS/rocBLAS don't have direct inf-norm, use CPU
        auto hostData = toHost();
        for (const auto& val : hostData) {
            result = std::max(result, std::abs(val));
        }
#else
        // CPU fallback
        auto hostData = toHost();
        for (const auto& val : hostData) {
            result = std::max(result, std::abs(val));
        }
#endif

        return result;
    }

    /**
     * @brief Copy from another GPU vector
     */
    void copy(const GPUVector<T>& other) {
        if (other.size() != size_) {
            resize(other.size());
        }
        data_.copyFromDevice(other.data_);
    }

    /**
     * @brief Element-wise addition: z = x + y
     */
    static GPUVector<T> add(const GPUVector<T>& x, const GPUVector<T>& y) {
        if (x.size() != y.size()) {
            throw std::runtime_error("Vector size mismatch in add");
        }

        GPUVector<T> result(x.size());
        result.copy(y);
        result.axpy(1.0, x);
        return result;
    }

    /**
     * @brief Element-wise subtraction: z = x - y
     */
    static GPUVector<T> subtract(const GPUVector<T>& x, const GPUVector<T>& y) {
        if (x.size() != y.size()) {
            throw std::runtime_error("Vector size mismatch in subtract");
        }

        GPUVector<T> result(x.size());
        result.copy(x);
        result.axpy(-1.0, y);
        return result;
    }

private:
    size_t size_;
    DeviceMemory<T> data_;
};

// Type aliases
using GPUVectorF = GPUVector<float>;
using GPUVectorD = GPUVector<double>;

} // namespace linalg
} // namespace gpu
} // namespace koo

#endif // KOO_GPU_LINALG_VECTOR_H
