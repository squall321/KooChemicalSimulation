/**
 * @file AutoTuner.h
 * @brief Automatic GPU kernel parameter tuning
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * Phase 69: Performance Auto-tuning
 *
 * Features:
 * - Automatic block size optimization
 * - Grid configuration tuning
 * - Occupancy optimization
 * - Performance benchmarking
 * - Parameter search strategies
 */

#pragma once

#include "../Device.h"
#include "../Stream.h"
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>

#ifdef KOO_USE_CUDA
#include <cuda_runtime.h>
#include <cuda_occupancy.h>
#endif

namespace koo {
namespace gpu {
namespace tuning {

/**
 * @brief Kernel configuration parameters
 */
struct KernelConfig {
    int block_size_x;      ///< Block size in X dimension
    int block_size_y;      ///< Block size in Y dimension
    int block_size_z;      ///< Block size in Z dimension
    size_t shared_mem;     ///< Shared memory per block (bytes)

    KernelConfig()
        : block_size_x(256), block_size_y(1), block_size_z(1),
          shared_mem(0) {}

    KernelConfig(int bx, int by = 1, int bz = 1, size_t smem = 0)
        : block_size_x(bx), block_size_y(by), block_size_z(bz),
          shared_mem(smem) {}

    int totalThreads() const {
        return block_size_x * block_size_y * block_size_z;
    }

    dim3 blockDim() const {
        return dim3(block_size_x, block_size_y, block_size_z);
    }
};

/**
 * @brief Kernel performance metrics
 */
struct PerformanceMetrics {
    double execution_time_ms;     ///< Execution time (ms)
    double throughput;            ///< Throughput (elements/second)
    double bandwidth_gb_s;        ///< Memory bandwidth (GB/s)
    double occupancy;             ///< Theoretical occupancy
    int num_blocks;               ///< Number of blocks
    int num_threads;              ///< Total number of threads

    PerformanceMetrics()
        : execution_time_ms(0.0), throughput(0.0),
          bandwidth_gb_s(0.0), occupancy(0.0),
          num_blocks(0), num_threads(0) {}
};

/**
 * @brief Search strategy for parameter tuning
 */
enum class SearchStrategy {
    Exhaustive,      ///< Try all combinations
    GridSearch,      ///< Regular grid sampling
    RandomSearch,    ///< Random sampling
    BayesianOpt,     ///< Bayesian optimization (future)
    Adaptive         ///< Adaptive sampling based on results
};

/**
 * @brief Auto-tuner for GPU kernels
 */
class AutoTuner {
public:
    /**
     * @brief Construct auto-tuner
     */
    explicit AutoTuner(const Device& device)
        : device_(device), best_config_(), best_performance_() {}

    /**
     * @brief Add candidate configuration
     */
    void addCandidate(const KernelConfig& config) {
        candidates_.push_back(config);
    }

    /**
     * @brief Generate common block sizes to test
     */
    void generateCommonBlockSizes() {
        candidates_.clear();

        // Common 1D block sizes
        std::vector<int> sizes_1d = {32, 64, 128, 256, 512, 1024};
        for (int size : sizes_1d) {
            candidates_.emplace_back(size, 1, 1);
        }

        // Common 2D block sizes
        std::vector<std::pair<int, int>> sizes_2d = {
            {16, 16}, {32, 8}, {8, 32}, {32, 16}, {16, 32}
        };
        for (const auto& [x, y] : sizes_2d) {
            candidates_.emplace_back(x, y, 1);
        }

        // Common 3D block sizes
        std::vector<std::tuple<int, int, int>> sizes_3d = {
            {8, 8, 8}, {16, 8, 4}, {8, 16, 4}
        };
        for (const auto& [x, y, z] : sizes_3d) {
            candidates_.emplace_back(x, y, z);
        }
    }

    /**
     * @brief Benchmark a specific configuration
     *
     * @param config Configuration to test
     * @param benchmark_func Benchmark function that returns execution time
     * @param problem_size Problem size (for throughput calculation)
     * @param data_bytes Bytes transferred (for bandwidth calculation)
     * @return Performance metrics
     */
    template<typename BenchmarkFunc>
    PerformanceMetrics benchmark(const KernelConfig& config,
                                BenchmarkFunc benchmark_func,
                                size_t problem_size,
                                size_t data_bytes) {
        PerformanceMetrics metrics;

        // Run benchmark
        try {
            metrics.execution_time_ms = benchmark_func(config);

            // Compute throughput
            if (metrics.execution_time_ms > 0.0) {
                double time_s = metrics.execution_time_ms / 1000.0;
                metrics.throughput = problem_size / time_s;

                // Compute bandwidth
                metrics.bandwidth_gb_s = (data_bytes / 1e9) / time_s;
            }

            // Compute grid dimensions
            metrics.num_threads = config.totalThreads();

#ifdef KOO_USE_CUDA
            // Compute occupancy (requires CUDA)
            metrics.occupancy = computeOccupancy(config);
#endif

        } catch (const std::exception& e) {
            // Benchmark failed - set very high time
            metrics.execution_time_ms = std::numeric_limits<double>::max();
        }

        return metrics;
    }

    /**
     * @brief Find optimal configuration
     *
     * @param benchmark_func Benchmark function
     * @param problem_size Problem size
     * @param data_bytes Data transfer size
     * @param strategy Search strategy
     * @param num_iterations Number of benchmark iterations
     * @return Best configuration
     */
    template<typename BenchmarkFunc>
    KernelConfig findOptimal(BenchmarkFunc benchmark_func,
                            size_t problem_size,
                            size_t data_bytes,
                            SearchStrategy strategy = SearchStrategy::Exhaustive,
                            int num_iterations = 10) {
        if (candidates_.empty()) {
            generateCommonBlockSizes();
        }

        double best_time = std::numeric_limits<double>::max();
        KernelConfig best_config;
        PerformanceMetrics best_perf;

        // Test each candidate
        for (const auto& config : candidates_) {
            // Run multiple iterations and take minimum time
            double min_time = std::numeric_limits<double>::max();
            PerformanceMetrics metrics;

            for (int iter = 0; iter < num_iterations; ++iter) {
                auto perf = benchmark(config, benchmark_func,
                                    problem_size, data_bytes);

                if (perf.execution_time_ms < min_time) {
                    min_time = perf.execution_time_ms;
                    metrics = perf;
                }
            }

            // Update best if this is better
            if (min_time < best_time) {
                best_time = min_time;
                best_config = config;
                best_perf = metrics;
            }

            // Store result
            results_[config] = metrics;
        }

        best_config_ = best_config;
        best_performance_ = best_perf;

        return best_config;
    }

    /**
     * @brief Get best configuration found
     */
    const KernelConfig& getBestConfig() const {
        return best_config_;
    }

    /**
     * @brief Get best performance metrics
     */
    const PerformanceMetrics& getBestPerformance() const {
        return best_performance_;
    }

    /**
     * @brief Get all results
     */
    const std::map<KernelConfig, PerformanceMetrics>& getResults() const {
        return results_;
    }

    /**
     * @brief Print tuning results
     */
    void printResults() const {
        std::cout << "\n========== Auto-Tuning Results ==========\n";
        std::cout << "Block Size (x,y,z) | Time (ms) | Throughput | Bandwidth (GB/s)\n";
        std::cout << "-----------------------------------------------------------\n";

        for (const auto& [config, perf] : results_) {
            std::cout << "(" << config.block_size_x << ","
                     << config.block_size_y << ","
                     << config.block_size_z << ")";
            std::cout << " | " << perf.execution_time_ms;
            std::cout << " | " << perf.throughput;
            std::cout << " | " << perf.bandwidth_gb_s << "\n";
        }

        std::cout << "\nBest configuration: ("
                 << best_config_.block_size_x << ","
                 << best_config_.block_size_y << ","
                 << best_config_.block_size_z << ")\n";
        std::cout << "Best time: " << best_performance_.execution_time_ms << " ms\n";
        std::cout << "=========================================\n";
    }

private:
    Device device_;
    std::vector<KernelConfig> candidates_;
    std::map<KernelConfig, PerformanceMetrics> results_;
    KernelConfig best_config_;
    PerformanceMetrics best_performance_;

    /**
     * @brief Compute theoretical occupancy
     */
    double computeOccupancy(const KernelConfig& config) const {
#ifdef KOO_USE_CUDA
        auto props = device_.getProperties();

        int threads_per_block = config.totalThreads();
        int max_threads_per_sm = props.maxThreadsPerMultiProcessor;
        int max_blocks_per_sm = props.maxBlocksPerMultiProcessor;

        // Simple occupancy estimate
        int blocks_per_sm = std::min(
            max_blocks_per_sm,
            max_threads_per_sm / threads_per_block
        );

        int active_warps = (threads_per_block * blocks_per_sm + 31) / 32;
        int max_warps_per_sm = max_threads_per_sm / 32;

        return static_cast<double>(active_warps) / max_warps_per_sm;
#else
        (void)config;
        return 0.0;
#endif
    }
};

/**
 * @brief Comparison operators for KernelConfig (for std::map)
 */
inline bool operator<(const KernelConfig& a, const KernelConfig& b) {
    if (a.block_size_x != b.block_size_x) return a.block_size_x < b.block_size_x;
    if (a.block_size_y != b.block_size_y) return a.block_size_y < b.block_size_y;
    if (a.block_size_z != b.block_size_z) return a.block_size_z < b.block_size_z;
    return a.shared_mem < b.shared_mem;
}

/**
 * @brief Parameter optimizer for general parameters
 */
template<typename T>
class ParameterOptimizer {
public:
    /**
     * @brief Objective function type
     */
    using ObjectiveFunc = std::function<double(const std::vector<T>&)>;

    /**
     * @brief Construct optimizer
     */
    ParameterOptimizer() = default;

    /**
     * @brief Grid search optimization
     *
     * @param objective Objective function to minimize
     * @param param_ranges Vector of (min, max, num_samples) for each parameter
     * @return Optimal parameters
     */
    std::vector<T> gridSearch(ObjectiveFunc objective,
                             const std::vector<std::tuple<T, T, int>>& param_ranges) {
        std::vector<T> best_params;
        double best_value = std::numeric_limits<double>::max();

        // Generate grid
        std::function<void(size_t, std::vector<T>&)> search;
        search = [&](size_t dim, std::vector<T>& current) {
            if (dim == param_ranges.size()) {
                // Evaluate
                double value = objective(current);
                if (value < best_value) {
                    best_value = value;
                    best_params = current;
                }
                return;
            }

            auto [min_val, max_val, num_samples] = param_ranges[dim];
            T step = (max_val - min_val) / (num_samples - 1);

            for (int i = 0; i < num_samples; ++i) {
                T value = min_val + i * step;
                current[dim] = value;
                search(dim + 1, current);
            }
        };

        std::vector<T> current(param_ranges.size());
        search(0, current);

        return best_params;
    }

    /**
     * @brief Get best objective value
     */
    double getBestValue() const { return best_value_; }

private:
    double best_value_ = std::numeric_limits<double>::max();
};

}  // namespace tuning
}  // namespace gpu
}  // namespace koo
