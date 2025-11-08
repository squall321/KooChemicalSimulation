/**
 * @file Profiler.h
 * @brief GPU profiling API for performance analysis
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha3
 * Phase 62: GPU Profiling & Debugging
 *
 * Features:
 * - CUDA profiler control (start/stop)
 * - Event-based timing measurements
 * - Kernel launch statistics
 * - Memory bandwidth profiling
 * - Profile data export
 */

#pragma once

#include "../Device.h"
#include "../Stream.h"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <fstream>
#include <iomanip>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#include <cuda_profiler_api.h>
#endif

namespace koo {
namespace gpu {
namespace profiling {

/**
 * @brief Kernel execution statistics
 */
struct KernelStats {
    std::string name;
    size_t call_count;
    double total_time_ms;
    double min_time_ms;
    double max_time_ms;
    double avg_time_ms;

    KernelStats()
        : call_count(0), total_time_ms(0.0),
          min_time_ms(1e9), max_time_ms(0.0), avg_time_ms(0.0) {}

    void addTiming(double time_ms) {
        call_count++;
        total_time_ms += time_ms;
        min_time_ms = std::min(min_time_ms, time_ms);
        max_time_ms = std::max(max_time_ms, time_ms);
        avg_time_ms = total_time_ms / call_count;
    }
};

/**
 * @brief CUDA event pair for timing
 */
class EventTimer {
public:
    EventTimer() {
#ifdef KOO_USE_CUDA
        cudaEventCreate(&start_);
        cudaEventCreate(&stop_);
#endif
    }

    ~EventTimer() {
#ifdef KOO_USE_CUDA
        cudaEventDestroy(start_);
        cudaEventDestroy(stop_);
#endif
    }

    // Non-copyable
    EventTimer(const EventTimer&) = delete;
    EventTimer& operator=(const EventTimer&) = delete;

    /**
     * @brief Record start event
     */
    void start(const Stream& stream) {
#ifdef KOO_USE_CUDA
        cudaEventRecord(start_, stream.get());
#else
        (void)stream;
#endif
    }

    /**
     * @brief Record stop event
     */
    void stop(const Stream& stream) {
#ifdef KOO_USE_CUDA
        cudaEventRecord(stop_, stream.get());
#else
        (void)stream;
#endif
    }

    /**
     * @brief Get elapsed time in milliseconds
     */
    float elapsed() {
#ifdef KOO_USE_CUDA
        cudaEventSynchronize(stop_);
        float ms = 0.0f;
        cudaEventElapsedTime(&ms, start_, stop_);
        return ms;
#else
        return 0.0f;
#endif
    }

private:
#ifdef KOO_USE_CUDA
    cudaEvent_t start_;
    cudaEvent_t stop_;
#endif
};

/**
 * @brief GPU profiler for performance monitoring
 */
class Profiler {
public:
    /**
     * @brief Get global profiler instance
     */
    static Profiler& getInstance() {
        static Profiler instance;
        return instance;
    }

    /**
     * @brief Start CUDA profiler
     */
    void start() {
#ifdef KOO_USE_CUDA
        cudaProfilerStart();
        enabled_ = true;
#endif
    }

    /**
     * @brief Stop CUDA profiler
     */
    void stop() {
#ifdef KOO_USE_CUDA
        cudaProfilerStop();
        enabled_ = false;
#endif
    }

    /**
     * @brief Check if profiling is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Begin timing a kernel
     * @param name Kernel name
     * @param stream Stream to use for timing
     * @return Timer ID
     */
    size_t beginKernel(const std::string& name, const Stream& stream) {
        if (!enabled_) return 0;

        size_t id = timers_.size();
        timers_.emplace_back();
        timer_names_.push_back(name);
        timers_.back().start(stream);

        return id;
    }

    /**
     * @brief End timing a kernel
     * @param id Timer ID from beginKernel
     * @param stream Stream used for timing
     */
    void endKernel(size_t id, const Stream& stream) {
        if (!enabled_ || id >= timers_.size()) return;

        timers_[id].stop(stream);
        float ms = timers_[id].elapsed();

        const std::string& name = timer_names_[id];
        kernel_stats_[name].addTiming(ms);
    }

    /**
     * @brief Get kernel statistics
     */
    const std::map<std::string, KernelStats>& getKernelStats() const {
        return kernel_stats_;
    }

    /**
     * @brief Get statistics for specific kernel
     */
    KernelStats getKernelStats(const std::string& name) const {
        auto it = kernel_stats_.find(name);
        if (it != kernel_stats_.end()) {
            return it->second;
        }
        return KernelStats();
    }

    /**
     * @brief Reset all statistics
     */
    void reset() {
        kernel_stats_.clear();
        timers_.clear();
        timer_names_.clear();
    }

    /**
     * @brief Print summary of all kernel statistics
     */
    void printSummary(std::ostream& os = std::cout) const {
        os << "\n========== GPU Profiling Summary ==========\n";
        os << std::left << std::setw(30) << "Kernel Name"
           << std::right << std::setw(10) << "Calls"
           << std::setw(12) << "Total (ms)"
           << std::setw(12) << "Avg (ms)"
           << std::setw(12) << "Min (ms)"
           << std::setw(12) << "Max (ms)" << "\n";
        os << std::string(88, '-') << "\n";

        for (const auto& pair : kernel_stats_) {
            const auto& stats = pair.second;
            os << std::left << std::setw(30) << pair.first
               << std::right << std::setw(10) << stats.call_count
               << std::setw(12) << std::fixed << std::setprecision(3) << stats.total_time_ms
               << std::setw(12) << stats.avg_time_ms
               << std::setw(12) << stats.min_time_ms
               << std::setw(12) << stats.max_time_ms << "\n";
        }

        os << "==========================================\n";
    }

    /**
     * @brief Export profiling data to CSV
     */
    void exportCSV(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) return;

        // Header
        file << "Kernel,Calls,Total_ms,Avg_ms,Min_ms,Max_ms\n";

        // Data
        for (const auto& pair : kernel_stats_) {
            const auto& stats = pair.second;
            file << pair.first << ","
                 << stats.call_count << ","
                 << stats.total_time_ms << ","
                 << stats.avg_time_ms << ","
                 << stats.min_time_ms << ","
                 << stats.max_time_ms << "\n";
        }
    }

private:
    Profiler() : enabled_(false) {}
    ~Profiler() = default;

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    bool enabled_;
    std::vector<EventTimer> timers_;
    std::vector<std::string> timer_names_;
    std::map<std::string, KernelStats> kernel_stats_;
};

/**
 * @brief RAII helper for kernel profiling
 */
class ScopedKernelProfile {
public:
    ScopedKernelProfile(const std::string& name, const Stream& stream)
        : stream_(stream), id_(0) {
        id_ = Profiler::getInstance().beginKernel(name, stream);
    }

    ~ScopedKernelProfile() {
        Profiler::getInstance().endKernel(id_, stream_);
    }

private:
    const Stream& stream_;
    size_t id_;
};

/**
 * @brief Macros for convenient profiling
 */
#define KOO_PROFILE_KERNEL(name, stream) \
    koo::gpu::profiling::ScopedKernelProfile _profile_##__LINE__(name, stream)

#define KOO_PROFILER_START() \
    koo::gpu::profiling::Profiler::getInstance().start()

#define KOO_PROFILER_STOP() \
    koo::gpu::profiling::Profiler::getInstance().stop()

#define KOO_PROFILER_RESET() \
    koo::gpu::profiling::Profiler::getInstance().reset()

#define KOO_PROFILER_PRINT() \
    koo::gpu::profiling::Profiler::getInstance().printSummary()

}  // namespace profiling
}  // namespace gpu
}  // namespace koo
