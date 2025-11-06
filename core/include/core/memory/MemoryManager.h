/**
 * @file MemoryManager.h
 * @brief Memory management and tracking system
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha4
 * @date 2025-11-06
 *
 * This file defines a memory management system for tracking allocations,
 * monitoring memory usage, and detecting memory leaks.
 */

#ifndef KOO_CORE_MEMORY_MEMORY_MANAGER_H
#define KOO_CORE_MEMORY_MEMORY_MANAGER_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <iostream>
#include <chrono>
#include <vector>

namespace koo {
namespace core {
namespace memory {

/**
 * @brief Memory allocation information
 */
struct AllocationInfo {
    void* address;           ///< Memory address
    size_t size;             ///< Allocation size in bytes
    std::string file;        ///< Source file where allocated
    int line;                ///< Line number
    std::string function;    ///< Function name
    size_t timestamp;        ///< Allocation timestamp (in microseconds)
};

/**
 * @brief Memory statistics
 */
struct MemoryStats {
    size_t totalAllocated{0};      ///< Total bytes allocated
    size_t totalFreed{0};           ///< Total bytes freed
    size_t currentUsage{0};         ///< Current memory usage
    size_t peakUsage{0};            ///< Peak memory usage
    size_t allocationCount{0};      ///< Number of allocations
    size_t freeCount{0};            ///< Number of frees
    size_t activeAllocations{0};    ///< Currently active allocations

    /**
     * @brief Check if there are memory leaks
     */
    bool hasLeaks() const {
        return activeAllocations > 0;
    }

    /**
     * @brief Get memory leak amount
     */
    size_t leakAmount() const {
        return currentUsage;
    }
};

/**
 * @brief Memory manager for allocation tracking and leak detection
 *
 * This class provides centralized memory management with:
 * - Allocation tracking with source location
 * - Memory usage statistics
 * - Leak detection
 * - Thread-safe operations
 *
 * Design Pattern: Singleton
 *
 * Example usage:
 * @code
 * // Enable tracking
 * MemoryManager::getInstance().enableTracking();
 *
 * // Allocate memory
 * void* ptr = MemoryManager::getInstance().allocate(1024, __FILE__, __LINE__, __FUNCTION__);
 *
 * // Free memory
 * MemoryManager::getInstance().deallocate(ptr);
 *
 * // Get statistics
 * auto stats = MemoryManager::getInstance().getStats();
 * std::cout << "Current usage: " << stats.currentUsage << " bytes" << std::endl;
 * @endcode
 */
class MemoryManager {
public:
    /**
     * @brief Get singleton instance
     */
    static MemoryManager& getInstance() {
        static MemoryManager instance;
        return instance;
    }

    /**
     * @brief Enable memory tracking
     */
    void enableTracking() {
        std::lock_guard<std::mutex> lock(mutex_);
        trackingEnabled_ = true;
    }

    /**
     * @brief Disable memory tracking
     */
    void disableTracking() {
        std::lock_guard<std::mutex> lock(mutex_);
        trackingEnabled_ = false;
    }

    /**
     * @brief Check if tracking is enabled
     */
    bool isTrackingEnabled() const {
        return trackingEnabled_;
    }

    /**
     * @brief Allocate memory with tracking
     *
     * @param size Size in bytes
     * @param file Source file
     * @param line Line number
     * @param function Function name
     * @return Allocated memory pointer
     */
    void* allocate(size_t size, const char* file = "", int line = 0,
                   const char* function = "") {
        void* ptr = ::operator new(size);

        if (trackingEnabled_) {
            std::lock_guard<std::mutex> lock(mutex_);

            AllocationInfo info;
            info.address = ptr;
            info.size = size;
            info.file = file;
            info.line = line;
            info.function = function;
            info.timestamp = getCurrentTimestamp();

            allocations_[ptr] = info;

            // Update statistics
            stats_.totalAllocated += size;
            stats_.currentUsage += size;
            stats_.allocationCount++;
            stats_.activeAllocations++;

            if (stats_.currentUsage > stats_.peakUsage) {
                stats_.peakUsage = stats_.currentUsage;
            }
        }

        return ptr;
    }

    /**
     * @brief Deallocate memory
     *
     * @param ptr Pointer to deallocate
     */
    void deallocate(void* ptr) {
        if (ptr == nullptr) {
            return;
        }

        if (trackingEnabled_) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                size_t size = it->second.size;
                allocations_.erase(it);

                // Update statistics
                stats_.totalFreed += size;
                stats_.currentUsage -= size;
                stats_.freeCount++;
                stats_.activeAllocations--;
            }
        }

        ::operator delete(ptr);
    }

    /**
     * @brief Get memory statistics
     */
    MemoryStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /**
     * @brief Reset statistics
     */
    void resetStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_ = MemoryStats();
    }

    /**
     * @brief Get all active allocations
     */
    std::vector<AllocationInfo> getActiveAllocations() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AllocationInfo> result;
        result.reserve(allocations_.size());
        for (const auto& pair : allocations_) {
            result.push_back(pair.second);
        }
        return result;
    }

    /**
     * @brief Print memory report
     */
    void printReport() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::cout << "\n========================================\n";
        std::cout << "  Memory Manager Report\n";
        std::cout << "========================================\n";
        std::cout << "Total allocated:      " << formatBytes(stats_.totalAllocated) << "\n";
        std::cout << "Total freed:          " << formatBytes(stats_.totalFreed) << "\n";
        std::cout << "Current usage:        " << formatBytes(stats_.currentUsage) << "\n";
        std::cout << "Peak usage:           " << formatBytes(stats_.peakUsage) << "\n";
        std::cout << "Allocation count:     " << stats_.allocationCount << "\n";
        std::cout << "Free count:           " << stats_.freeCount << "\n";
        std::cout << "Active allocations:   " << stats_.activeAllocations << "\n";

        if (stats_.hasLeaks()) {
            std::cout << "\nWARNING: Memory leaks detected!\n";
            std::cout << "Leaked memory:        " << formatBytes(stats_.leakAmount()) << "\n";
            std::cout << "Leaked allocations:   " << stats_.activeAllocations << "\n";

            std::cout << "\nLeak details:\n";
            for (const auto& pair : allocations_) {
                const auto& info = pair.second;
                std::cout << "  " << formatBytes(info.size) << " at " << info.address
                         << " [" << info.file << ":" << info.line << " in "
                         << info.function << "]\n";
            }
        } else {
            std::cout << "\nNo memory leaks detected.\n";
        }
        std::cout << "========================================\n\n";
    }

    /**
     * @brief Check for memory leaks
     */
    bool hasLeaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_.hasLeaks();
    }

private:
    /**
     * @brief Private constructor (Singleton)
     */
    MemoryManager() : trackingEnabled_(false) {}

    /**
     * @brief Destructor - prints final report
     */
    ~MemoryManager() {
        if (trackingEnabled_ && stats_.hasLeaks()) {
            printReport();
        }
    }

    // Delete copy and move
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    /**
     * @brief Get current timestamp in microseconds
     */
    static size_t getCurrentTimestamp() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

    /**
     * @brief Format bytes for display
     */
    static std::string formatBytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit < 4) {
            size /= 1024.0;
            unit++;
        }

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
        return std::string(buffer);
    }

    mutable std::mutex mutex_;                                    ///< Thread safety
    bool trackingEnabled_;                                        ///< Tracking enabled flag
    std::unordered_map<void*, AllocationInfo> allocations_;      ///< Active allocations
    MemoryStats stats_;                                           ///< Memory statistics
};

// ============================================================================
// Convenience Macros
// ============================================================================

/**
 * @brief Allocate memory with tracking
 *
 * Example: void* ptr = KOO_ALLOCATE(1024);
 */
#define KOO_ALLOCATE(size) \
    ::koo::core::memory::MemoryManager::getInstance().allocate( \
        size, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Deallocate memory
 *
 * Example: KOO_DEALLOCATE(ptr);
 */
#define KOO_DEALLOCATE(ptr) \
    ::koo::core::memory::MemoryManager::getInstance().deallocate(ptr)

/**
 * @brief Allocate typed memory
 *
 * Example: int* arr = KOO_NEW_ARRAY(int, 100);
 */
#define KOO_NEW_ARRAY(T, count) \
    static_cast<T*>(KOO_ALLOCATE(sizeof(T) * (count)))

/**
 * @brief Deallocate typed memory
 *
 * Example: KOO_DELETE_ARRAY(arr);
 */
#define KOO_DELETE_ARRAY(ptr) \
    KOO_DEALLOCATE(ptr)

} // namespace memory
} // namespace core
} // namespace koo

#endif // KOO_CORE_MEMORY_MEMORY_MANAGER_H
