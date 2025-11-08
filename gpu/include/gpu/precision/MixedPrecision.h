/**
 * @file MixedPrecision.h
 * @brief Mixed precision computation support (FP16/FP32/FP64)
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 63: Mixed Precision
 *
 * Features:
 * - Half precision (FP16) for faster computation
 * - Automatic mixed precision (AMP) support
 * - Loss scaling for numerical stability
 * - Conversion utilities between precisions
 * - Precision policy management
 */

#pragma once

#include "../Device.h"
#include <cuda_fp16.h>
#include <stdexcept>
#include <cmath>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace koo {
namespace gpu {
namespace precision {

/**
 * @brief Precision types
 */
enum class PrecisionType {
    FP64,    ///< Double precision (64-bit)
    FP32,    ///< Single precision (32-bit)
    FP16     ///< Half precision (16-bit)
};

/**
 * @brief Check if device supports FP16
 */
inline bool supportsFP16(const Device& device) {
#ifdef KOO_USE_CUDA
    auto props = device.getProperties();
    // Pascal (6.0) and later support FP16
    return props.major >= 6;
#else
    (void)device;
    return false;
#endif
}

/**
 * @brief Check if device has Tensor Cores (optimized FP16)
 */
inline bool supportsTensorCores(const Device& device) {
#ifdef KOO_USE_CUDA
    auto props = device.getProperties();
    // Volta (7.0) and later have Tensor Cores
    return props.major >= 7;
#else
    (void)device;
    return false;
#endif
}

/**
 * @brief Conversion functions between precisions
 */

/// Convert FP32 to FP16
#ifdef KOO_USE_CUDA
__host__ __device__
#endif
inline __half float_to_half(float val) {
#ifdef __CUDA_ARCH__
    return __float2half(val);
#else
#ifdef KOO_USE_CUDA
    return __float2half(val);
#else
    // Simplified CPU version
    return __half{};
#endif
#endif
}

/// Convert FP16 to FP32
#ifdef KOO_USE_CUDA
__host__ __device__
#endif
inline float half_to_float(__half val) {
#ifdef __CUDA_ARCH__
    return __half2float(val);
#else
#ifdef KOO_USE_CUDA
    return __half2float(val);
#else
    return 0.0f;
#endif
#endif
}

/**
 * @brief Mixed precision policy
 *
 * Defines which operations use which precision
 */
class MixedPrecisionPolicy {
public:
    MixedPrecisionPolicy()
        : compute_precision_(PrecisionType::FP32),
          storage_precision_(PrecisionType::FP32),
          use_amp_(false),
          loss_scale_(1.0f) {}

    /**
     * @brief Set compute precision
     */
    void setComputePrecision(PrecisionType prec) {
        compute_precision_ = prec;
    }

    /**
     * @brief Set storage precision
     */
    void setStoragePrecision(PrecisionType prec) {
        storage_precision_ = prec;
    }

    /**
     * @brief Enable automatic mixed precision
     */
    void enableAMP(bool enable = true) {
        use_amp_ = enable;
    }

    /**
     * @brief Set loss scale for numerical stability
     */
    void setLossScale(float scale) {
        loss_scale_ = scale;
    }

    /**
     * @brief Get compute precision
     */
    PrecisionType getComputePrecision() const {
        return compute_precision_;
    }

    /**
     * @brief Get storage precision
     */
    PrecisionType getStoragePrecision() const {
        return storage_precision_;
    }

    /**
     * @brief Check if AMP is enabled
     */
    bool isAMPEnabled() const {
        return use_amp_;
    }

    /**
     * @brief Get loss scale
     */
    float getLossScale() const {
        return loss_scale_;
    }

    /**
     * @brief Create FP16 compute, FP32 storage policy (common for training)
     */
    static MixedPrecisionPolicy createAMPPolicy() {
        MixedPrecisionPolicy policy;
        policy.setComputePrecision(PrecisionType::FP16);
        policy.setStoragePrecision(PrecisionType::FP32);
        policy.enableAMP(true);
        policy.setLossScale(1024.0f);  // Common initial scale
        return policy;
    }

    /**
     * @brief Create full FP32 policy (baseline)
     */
    static MixedPrecisionPolicy createFP32Policy() {
        MixedPrecisionPolicy policy;
        policy.setComputePrecision(PrecisionType::FP32);
        policy.setStoragePrecision(PrecisionType::FP32);
        policy.enableAMP(false);
        return policy;
    }

    /**
     * @brief Create full FP16 policy (maximum speed, lower accuracy)
     */
    static MixedPrecisionPolicy createFP16Policy() {
        MixedPrecisionPolicy policy;
        policy.setComputePrecision(PrecisionType::FP16);
        policy.setStoragePrecision(PrecisionType::FP16);
        policy.enableAMP(false);
        return policy;
    }

private:
    PrecisionType compute_precision_;
    PrecisionType storage_precision_;
    bool use_amp_;
    float loss_scale_;
};

/**
 * @brief Loss scaler for automatic mixed precision
 *
 * Dynamically adjusts loss scale to prevent underflow/overflow
 */
class LossScaler {
public:
    /**
     * @brief Construct with initial scale
     */
    explicit LossScaler(float initial_scale = 1024.0f,
                       float growth_factor = 2.0f,
                       float backoff_factor = 0.5f,
                       int growth_interval = 2000)
        : scale_(initial_scale),
          growth_factor_(growth_factor),
          backoff_factor_(backoff_factor),
          growth_interval_(growth_interval),
          growth_counter_(0),
          num_overflows_(0) {}

    /**
     * @brief Get current scale
     */
    float getScale() const { return scale_; }

    /**
     * @brief Scale loss value
     */
    float scaleLoss(float loss) const {
        return loss * scale_;
    }

    /**
     * @brief Unscale gradient
     */
    float unscaleGradient(float grad) const {
        return grad / scale_;
    }

    /**
     * @brief Update scale based on overflow detection
     * @param has_overflow Whether overflow/inf/nan was detected
     */
    void update(bool has_overflow) {
        if (has_overflow) {
            // Decrease scale on overflow
            scale_ *= backoff_factor_;
            growth_counter_ = 0;
            num_overflows_++;
        } else {
            // Increase scale periodically if no overflow
            growth_counter_++;
            if (growth_counter_ >= growth_interval_) {
                scale_ *= growth_factor_;
                growth_counter_ = 0;
            }
        }

        // Clamp scale to reasonable range
        scale_ = std::max(1.0f, std::min(scale_, 65536.0f));
    }

    /**
     * @brief Reset scaler
     */
    void reset(float initial_scale = 1024.0f) {
        scale_ = initial_scale;
        growth_counter_ = 0;
        num_overflows_ = 0;
    }

    /**
     * @brief Get number of overflows encountered
     */
    int getNumOverflows() const { return num_overflows_; }

private:
    float scale_;
    float growth_factor_;
    float backoff_factor_;
    int growth_interval_;
    int growth_counter_;
    int num_overflows_;
};

/**
 * @brief Mixed precision tensor operations
 */
class MixedPrecisionOps {
public:
    /**
     * @brief Convert FP32 array to FP16 on device
     */
    template<typename T>
    static void convertFP32ToFP16(const float* src, __half* dst, size_t count,
                                  cudaStream_t stream = 0);

    /**
     * @brief Convert FP16 array to FP32 on device
     */
    template<typename T>
    static void convertFP16ToFP32(const __half* src, float* dst, size_t count,
                                  cudaStream_t stream = 0);

    /**
     * @brief Scale array by constant (for loss scaling)
     */
    static void scaleArray(float* data, size_t count, float scale,
                          cudaStream_t stream = 0);

    /**
     * @brief Check for overflow/inf/nan in array
     */
    static bool hasInfOrNan(const float* data, size_t count,
                           cudaStream_t stream = 0);
};

/**
 * @brief CUDA kernels for mixed precision operations
 */
#ifdef __CUDACC__

/// Convert FP32 to FP16
__global__
void convertFP32ToFP16Kernel(const float* __restrict__ src,
                             __half* __restrict__ dst,
                             size_t count) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < count) {
        dst[idx] = __float2half(src[idx]);
    }
}

/// Convert FP16 to FP32
__global__
void convertFP16ToFP32Kernel(const __half* __restrict__ src,
                             float* __restrict__ dst,
                             size_t count) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < count) {
        dst[idx] = __half2float(src[idx]);
    }
}

/// Scale array
__global__
void scaleArrayKernel(float* __restrict__ data, size_t count, float scale) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < count) {
        data[idx] *= scale;
    }
}

/// Check for inf/nan
__global__
void checkInfNanKernel(const float* __restrict__ data, size_t count,
                       int* has_inf_nan) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < count) {
        float val = data[idx];
        if (isnan(val) || isinf(val)) {
            atomicOr(has_inf_nan, 1);
        }
    }
}

#endif  // __CUDACC__

/**
 * @brief Example usage:
 *
 * // Setup mixed precision
 * auto policy = MixedPrecisionPolicy::createAMPPolicy();
 * LossScaler scaler(1024.0f);
 *
 * // Training loop
 * for (int iter = 0; iter < max_iters; ++iter) {
 *     // Forward pass in FP16
 *     float loss = compute_loss();
 *
 *     // Scale loss
 *     float scaled_loss = scaler.scaleLoss(loss);
 *
 *     // Backward pass
 *     compute_gradients(scaled_loss);
 *
 *     // Unscale gradients
 *     unscale_gradients(scaler.getScale());
 *
 *     // Check for overflow
 *     bool overflow = check_overflow();
 *     scaler.update(overflow);
 *
 *     if (!overflow) {
 *         // Update weights in FP32
 *         update_weights();
 *     }
 * }
 */

}  // namespace precision
}  // namespace gpu
}  // namespace koo
