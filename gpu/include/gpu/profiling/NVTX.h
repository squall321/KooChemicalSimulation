/**
 * @file NVTX.h
 * @brief NVIDIA Tools Extension (NVTX) markers for visual profiling
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 62: GPU Profiling & Debugging
 *
 * Features:
 * - Range markers for code sections
 * - Color-coded visualization in Nsight Systems/Compute
 * - Custom messages and categories
 * - Nested ranges for hierarchical profiling
 * - Domain-based organization
 */

#pragma once

#include <string>
#include <cstdint>

// Check if NVTX is available
#ifdef KOO_USE_CUDA
#include <nvtx3/nvToolsExt.h>
#define KOO_HAS_NVTX
#endif

namespace koo {
namespace gpu {
namespace profiling {

/**
 * @brief NVTX color palette
 */
enum class NVTXColor : uint32_t {
    Red     = 0xFFFF0000,
    Green   = 0xFF00FF00,
    Blue    = 0xFF0000FF,
    Yellow  = 0xFFFFFF00,
    Cyan    = 0xFF00FFFF,
    Magenta = 0xFFFF00FF,
    Orange  = 0xFFFF8800,
    Purple  = 0xFF8800FF,
    Pink    = 0xFFFF0088,
    Lime    = 0xFF88FF00,
    Gray    = 0xFF888888,
    White   = 0xFFFFFFFF
};

/**
 * @brief NVTX range marker
 *
 * Use to mark code sections for visualization in NVIDIA profiling tools.
 * Automatically ends range when object goes out of scope (RAII).
 */
class NVTXRange {
public:
    /**
     * @brief Start a named range with optional color
     * @param message Range name/message
     * @param color Color for visualization
     */
    explicit NVTXRange(const std::string& message,
                      NVTXColor color = NVTXColor::Green) {
#ifdef KOO_HAS_NVTX
        nvtxEventAttributes_t attr = {0};
        attr.version = NVTX_VERSION;
        attr.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.colorType = NVTX_COLOR_ARGB;
        attr.color = static_cast<uint32_t>(color);
        attr.messageType = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = message.c_str();

        nvtxRangePushEx(&attr);
#else
        (void)message;
        (void)color;
#endif
    }

    /**
     * @brief Start a range with category
     * @param message Range name
     * @param category Category ID for grouping
     * @param color Color for visualization
     */
    NVTXRange(const std::string& message, uint32_t category,
             NVTXColor color = NVTXColor::Blue) {
#ifdef KOO_HAS_NVTX
        nvtxEventAttributes_t attr = {0};
        attr.version = NVTX_VERSION;
        attr.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.colorType = NVTX_COLOR_ARGB;
        attr.color = static_cast<uint32_t>(color);
        attr.messageType = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = message.c_str();
        attr.category = category;

        nvtxRangePushEx(&attr);
#else
        (void)message;
        (void)category;
        (void)color;
#endif
    }

    /**
     * @brief Destructor - automatically ends range
     */
    ~NVTXRange() {
#ifdef KOO_HAS_NVTX
        nvtxRangePop();
#endif
    }

    // Non-copyable and non-movable
    NVTXRange(const NVTXRange&) = delete;
    NVTXRange& operator=(const NVTXRange&) = delete;
    NVTXRange(NVTXRange&&) = delete;
    NVTXRange& operator=(NVTXRange&&) = delete;
};

/**
 * @brief NVTX marker for single point in time
 */
class NVTXMark {
public:
    /**
     * @brief Place a marker at current point
     * @param message Marker message
     * @param color Marker color
     */
    static void mark(const std::string& message,
                    NVTXColor color = NVTXColor::Yellow) {
#ifdef KOO_HAS_NVTX
        nvtxEventAttributes_t attr = {0};
        attr.version = NVTX_VERSION;
        attr.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.colorType = NVTX_COLOR_ARGB;
        attr.color = static_cast<uint32_t>(color);
        attr.messageType = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = message.c_str();

        nvtxMarkEx(&attr);
#else
        (void)message;
        (void)color;
#endif
    }
};

/**
 * @brief NVTX domain for organizing markers
 */
class NVTXDomain {
public:
    /**
     * @brief Create named domain
     */
    explicit NVTXDomain(const std::string& name) {
#ifdef KOO_HAS_NVTX
        handle_ = nvtxDomainCreateA(name.c_str());
#else
        (void)name;
#endif
    }

    /**
     * @brief Destructor
     */
    ~NVTXDomain() {
#ifdef KOO_HAS_NVTX
        if (handle_) {
            nvtxDomainDestroy(handle_);
        }
#endif
    }

    // Non-copyable
    NVTXDomain(const NVTXDomain&) = delete;
    NVTXDomain& operator=(const NVTXDomain&) = delete;

    /**
     * @brief Start range in this domain
     */
    void pushRange(const std::string& message,
                   NVTXColor color = NVTXColor::Green) {
#ifdef KOO_HAS_NVTX
        nvtxEventAttributes_t attr = {0};
        attr.version = NVTX_VERSION;
        attr.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.colorType = NVTX_COLOR_ARGB;
        attr.color = static_cast<uint32_t>(color);
        attr.messageType = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = message.c_str();

        nvtxDomainRangePushEx(handle_, &attr);
#else
        (void)message;
        (void)color;
#endif
    }

    /**
     * @brief End range in this domain
     */
    void popRange() {
#ifdef KOO_HAS_NVTX
        nvtxDomainRangePop(handle_);
#endif
    }

    /**
     * @brief Place marker in this domain
     */
    void mark(const std::string& message,
              NVTXColor color = NVTXColor::Yellow) {
#ifdef KOO_HAS_NVTX
        nvtxEventAttributes_t attr = {0};
        attr.version = NVTX_VERSION;
        attr.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.colorType = NVTX_COLOR_ARGB;
        attr.color = static_cast<uint32_t>(color);
        attr.messageType = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = message.c_str();

        nvtxDomainMarkEx(handle_, &attr);
#else
        (void)message;
        (void)color;
#endif
    }

private:
#ifdef KOO_HAS_NVTX
    nvtxDomainHandle_t handle_;
#else
    void* handle_ = nullptr;
#endif
};

/**
 * @brief RAII helper for domain ranges
 */
class ScopedDomainRange {
public:
    ScopedDomainRange(NVTXDomain& domain, const std::string& message,
                     NVTXColor color = NVTXColor::Green)
        : domain_(domain) {
        domain_.pushRange(message, color);
    }

    ~ScopedDomainRange() {
        domain_.popRange();
    }

private:
    NVTXDomain& domain_;
};

/**
 * @brief Predefined categories for common operations
 */
namespace Categories {
    constexpr uint32_t Memory    = 1;   ///< Memory operations
    constexpr uint32_t Compute   = 2;   ///< Computation kernels
    constexpr uint32_t IO        = 3;   ///< Input/Output
    constexpr uint32_t Sync      = 4;   ///< Synchronization
    constexpr uint32_t Comm      = 5;   ///< Communication (MPI/GPU)
    constexpr uint32_t Init      = 6;   ///< Initialization
    constexpr uint32_t Analysis  = 7;   ///< Analysis/post-processing
    constexpr uint32_t Custom    = 100; ///< User-defined start
}

/**
 * @brief Convenience macros for NVTX
 */

// Simple range with default color
#define KOO_NVTX_RANGE(msg) \
    koo::gpu::profiling::NVTXRange _nvtx_range_##__LINE__(msg)

// Range with custom color
#define KOO_NVTX_RANGE_COLOR(msg, color) \
    koo::gpu::profiling::NVTXRange _nvtx_range_##__LINE__(msg, color)

// Range with category and color
#define KOO_NVTX_RANGE_CAT(msg, cat, color) \
    koo::gpu::profiling::NVTXRange _nvtx_range_##__LINE__(msg, cat, color)

// Single marker
#define KOO_NVTX_MARK(msg) \
    koo::gpu::profiling::NVTXMark::mark(msg)

// Marker with color
#define KOO_NVTX_MARK_COLOR(msg, color) \
    koo::gpu::profiling::NVTXMark::mark(msg, color)

// Pre-colored convenience macros
#define KOO_NVTX_MEMORY(msg) \
    KOO_NVTX_RANGE_CAT(msg, koo::gpu::profiling::Categories::Memory, \
                       koo::gpu::profiling::NVTXColor::Cyan)

#define KOO_NVTX_COMPUTE(msg) \
    KOO_NVTX_RANGE_CAT(msg, koo::gpu::profiling::Categories::Compute, \
                       koo::gpu::profiling::NVTXColor::Green)

#define KOO_NVTX_IO(msg) \
    KOO_NVTX_RANGE_CAT(msg, koo::gpu::profiling::Categories::IO, \
                       koo::gpu::profiling::NVTXColor::Orange)

#define KOO_NVTX_SYNC(msg) \
    KOO_NVTX_RANGE_CAT(msg, koo::gpu::profiling::Categories::Sync, \
                       koo::gpu::profiling::NVTXColor::Red)

#define KOO_NVTX_COMM(msg) \
    KOO_NVTX_RANGE_CAT(msg, koo::gpu::profiling::Categories::Comm, \
                       koo::gpu::profiling::NVTXColor::Purple)

/**
 * @brief Example usage:
 *
 * void myFunction() {
 *     KOO_NVTX_RANGE("myFunction");
 *
 *     {
 *         KOO_NVTX_MEMORY("Allocate buffers");
 *         // Memory allocation code
 *     }
 *
 *     {
 *         KOO_NVTX_COMPUTE("Run kernel");
 *         // Kernel launch
 *     }
 *
 *     KOO_NVTX_MARK("Checkpoint reached");
 * }
 */

}  // namespace profiling
}  // namespace gpu
}  // namespace koo
