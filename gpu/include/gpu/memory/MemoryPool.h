/**
 * @file MemoryPool.h
 * @brief GPU memory pool allocator for efficient memory management
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 61: GPU Memory Optimization
 *
 * Features:
 * - Fast allocation/deallocation through memory pooling
 * - Reduces cudaMalloc/cudaFree overhead
 * - Configurable chunk sizes and growth strategy
 * - Memory usage tracking and statistics
 * - Thread-safe operations
 */

#pragma once

#include "../Device.h"
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <cmath>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#endif

namespace koo {
namespace gpu {
namespace memory {

/**
 * @brief Memory pool statistics
 */
struct PoolStats {
    size_t total_allocated;      ///< Total bytes allocated from GPU
    size_t total_freed;           ///< Total bytes freed
    size_t current_usage;         ///< Current bytes in use
    size_t peak_usage;            ///< Peak memory usage
    size_t num_allocations;       ///< Number of allocation requests
    size_t num_deallocations;     ///< Number of deallocation requests
    size_t num_pool_hits;         ///< Allocations served from pool
    size_t num_pool_misses;       ///< Allocations requiring new memory

    PoolStats() : total_allocated(0), total_freed(0), current_usage(0),
                  peak_usage(0), num_allocations(0), num_deallocations(0),
                  num_pool_hits(0), num_pool_misses(0) {}

    double hit_rate() const {
        return num_allocations > 0 ?
               static_cast<double>(num_pool_hits) / num_allocations : 0.0;
    }
};

/**
 * @brief Memory block in the pool
 */
struct MemoryBlock {
    void* ptr;                    ///< GPU memory pointer
    size_t size;                  ///< Block size in bytes
    bool in_use;                  ///< Whether block is currently allocated
    int device_id;                ///< GPU device ID

    MemoryBlock(void* p, size_t s, int dev)
        : ptr(p), size(s), in_use(false), device_id(dev) {}
};

/**
 * @brief GPU memory pool allocator
 *
 * Manages a pool of pre-allocated GPU memory blocks to reduce
 * allocation overhead. Uses best-fit strategy for allocation.
 */
class MemoryPool {
public:
    /**
     * @brief Pool configuration
     */
    struct Config {
        size_t initial_size;      ///< Initial pool size (bytes)
        size_t max_size;          ///< Maximum pool size (bytes, 0=unlimited)
        size_t chunk_size;        ///< Allocation chunk size (bytes)
        bool allow_growth;        ///< Allow pool to grow beyond initial size
        double growth_factor;     ///< Growth factor when expanding pool

        Config() : initial_size(256 * 1024 * 1024),    // 256 MB
                   max_size(0),                         // Unlimited
                   chunk_size(1024 * 1024),             // 1 MB chunks
                   allow_growth(true),
                   growth_factor(1.5) {}
    };

    /**
     * @brief Construct memory pool for specific device
     * @param device GPU device
     * @param config Pool configuration
     */
    explicit MemoryPool(const Device& device, const Config& config = Config())
        : device_(device), config_(config) {
#ifdef KOO_USE_CUDA
        device_.setCurrent();

        // Pre-allocate initial pool if requested
        if (config_.initial_size > 0) {
            allocateNewBlock(config_.initial_size);
        }
#endif
    }

    /**
     * @brief Destructor - frees all pooled memory
     */
    ~MemoryPool() {
#ifdef KOO_USE_CUDA
        std::lock_guard<std::mutex> lock(mutex_);
        device_.setCurrent();

        for (auto& block : blocks_) {
            if (block.ptr) {
                cudaFree(block.ptr);
            }
        }
        blocks_.clear();
#endif
    }

    // Non-copyable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    /**
     * @brief Allocate memory from pool
     * @param size Requested size in bytes
     * @return Pointer to allocated memory (nullptr on failure)
     */
    void* allocate(size_t size) {
        if (size == 0) return nullptr;

        std::lock_guard<std::mutex> lock(mutex_);
        stats_.num_allocations++;

        // Round up to chunk size for better reuse
        size_t aligned_size = alignSize(size);

        // Try to find suitable block in pool (best-fit strategy)
        MemoryBlock* best_block = nullptr;
        size_t best_size_diff = SIZE_MAX;

        for (auto& block : blocks_) {
            if (!block.in_use && block.size >= aligned_size) {
                size_t size_diff = block.size - aligned_size;
                if (size_diff < best_size_diff) {
                    best_block = &block;
                    best_size_diff = size_diff;

                    // Perfect fit
                    if (size_diff == 0) break;
                }
            }
        }

        // Reuse existing block
        if (best_block) {
            best_block->in_use = true;
            stats_.num_pool_hits++;
            stats_.current_usage += best_block->size;
            stats_.peak_usage = std::max(stats_.peak_usage, stats_.current_usage);
            return best_block->ptr;
        }

        // Need to allocate new block
        stats_.num_pool_misses++;

        // Check if growth is allowed
        if (!config_.allow_growth && stats_.total_allocated > 0) {
            return nullptr;  // Pool exhausted
        }

        // Check max size limit
        if (config_.max_size > 0 &&
            stats_.total_allocated + aligned_size > config_.max_size) {
            return nullptr;  // Would exceed max size
        }

        // Allocate new block
        void* ptr = allocateNewBlock(aligned_size);
        if (ptr) {
            // Mark as in use
            for (auto& block : blocks_) {
                if (block.ptr == ptr) {
                    block.in_use = true;
                    stats_.current_usage += block.size;
                    stats_.peak_usage = std::max(stats_.peak_usage, stats_.current_usage);
                    break;
                }
            }
        }

        return ptr;
    }

    /**
     * @brief Free memory back to pool
     * @param ptr Pointer to free
     */
    void deallocate(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex_);
        stats_.num_deallocations++;

        // Find block and mark as free
        for (auto& block : blocks_) {
            if (block.ptr == ptr) {
                if (block.in_use) {
                    block.in_use = false;
                    stats_.current_usage -= block.size;
                }
                return;
            }
        }
    }

    /**
     * @brief Release all unused memory back to GPU
     * @return Bytes released
     */
    size_t trim() {
        std::lock_guard<std::mutex> lock(mutex_);

#ifdef KOO_USE_CUDA
        device_.setCurrent();

        size_t released = 0;
        auto it = blocks_.begin();
        while (it != blocks_.end()) {
            if (!it->in_use) {
                cudaFree(it->ptr);
                released += it->size;
                stats_.total_freed += it->size;
                it = blocks_.erase(it);
            } else {
                ++it;
            }
        }

        return released;
#else
        return 0;
#endif
    }

    /**
     * @brief Get pool statistics
     */
    PoolStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /**
     * @brief Reset statistics
     */
    void resetStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = PoolStats();
    }

    /**
     * @brief Get associated device
     */
    const Device& getDevice() const { return device_; }

    /**
     * @brief Get pool configuration
     */
    const Config& getConfig() const { return config_; }

private:
    Device device_;
    Config config_;
    std::vector<MemoryBlock> blocks_;
    PoolStats stats_;
    mutable std::mutex mutex_;

    /**
     * @brief Align size to chunk boundary
     */
    size_t alignSize(size_t size) const {
        if (config_.chunk_size == 0) return size;
        return ((size + config_.chunk_size - 1) / config_.chunk_size) * config_.chunk_size;
    }

    /**
     * @brief Allocate new memory block from GPU
     */
    void* allocateNewBlock(size_t size) {
#ifdef KOO_USE_CUDA
        device_.setCurrent();

        // Determine allocation size (may grow by factor)
        size_t alloc_size = size;
        if (blocks_.empty() && config_.initial_size > size) {
            alloc_size = config_.initial_size;
        } else if (config_.allow_growth && config_.growth_factor > 1.0) {
            alloc_size = static_cast<size_t>(size * config_.growth_factor);
        }

        // Allocate from GPU
        void* ptr = nullptr;
        cudaError_t err = cudaMalloc(&ptr, alloc_size);

        if (err != cudaSuccess) {
            // Try exact size on failure
            if (alloc_size > size) {
                alloc_size = size;
                err = cudaMalloc(&ptr, alloc_size);
            }

            if (err != cudaSuccess) {
                return nullptr;
            }
        }

        // Add to pool
        blocks_.emplace_back(ptr, alloc_size, device_.getId());
        stats_.total_allocated += alloc_size;

        return ptr;
#else
        (void)size;
        return nullptr;
#endif
    }
};

/**
 * @brief Global memory pool manager
 *
 * Manages one memory pool per GPU device
 */
class GlobalPoolManager {
public:
    static GlobalPoolManager& getInstance() {
        static GlobalPoolManager instance;
        return instance;
    }

    /**
     * @brief Get or create pool for device
     */
    std::shared_ptr<MemoryPool> getPool(const Device& device) {
        std::lock_guard<std::mutex> lock(mutex_);

        int device_id = device.getId();
        auto it = pools_.find(device_id);

        if (it == pools_.end()) {
            auto pool = std::make_shared<MemoryPool>(device);
            pools_[device_id] = pool;
            return pool;
        }

        return it->second;
    }

    /**
     * @brief Get pool for device (if exists)
     */
    std::shared_ptr<MemoryPool> getPoolIfExists(int device_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pools_.find(device_id);
        return (it != pools_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Release all pools
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pools_.clear();
    }

    /**
     * @brief Get total statistics across all pools
     */
    PoolStats getTotalStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        PoolStats total;
        for (const auto& pair : pools_) {
            PoolStats s = pair.second->getStats();
            total.total_allocated += s.total_allocated;
            total.total_freed += s.total_freed;
            total.current_usage += s.current_usage;
            total.peak_usage += s.peak_usage;
            total.num_allocations += s.num_allocations;
            total.num_deallocations += s.num_deallocations;
            total.num_pool_hits += s.num_pool_hits;
            total.num_pool_misses += s.num_pool_misses;
        }

        return total;
    }

private:
    GlobalPoolManager() = default;
    ~GlobalPoolManager() = default;

    GlobalPoolManager(const GlobalPoolManager&) = delete;
    GlobalPoolManager& operator=(const GlobalPoolManager&) = delete;

    std::map<int, std::shared_ptr<MemoryPool>> pools_;
    mutable std::mutex mutex_;
};

}  // namespace memory
}  // namespace gpu
}  // namespace koo
