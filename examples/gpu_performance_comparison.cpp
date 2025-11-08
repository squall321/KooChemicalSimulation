/**
 * @file gpu_performance_comparison.cpp
 * @brief CPU vs GPU performance comparison
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * This example benchmarks:
 * - 2D diffusion solver on CPU
 * - 2D diffusion solver on GPU (simulated)
 * - Performance comparison across problem sizes
 * - Recommendation for optimal hardware selection
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <algorithm>

/**
 * @brief CPU-based 2D diffusion solver
 */
class CPUDiffusionSolver {
private:
    int nx_, ny_;
    double D_, dt_, dx_, dy_;
    std::vector<std::vector<double>> u_, u_new_;

public:
    CPUDiffusionSolver(int nx, int ny, double L, double D, double dt)
        : nx_(nx), ny_(ny), D_(D), dt_(dt)
    {
        dx_ = L / (nx - 1);
        dy_ = L / (ny - 1);

        u_.resize(nx_, std::vector<double>(ny_, 0.0));
        u_new_.resize(nx_, std::vector<double>(ny_, 0.0));

        // Initialize with hot spot
        int cx = nx_ / 2;
        int cy = ny_ / 2;
        int r = 5;
        for (int i = cx - r; i <= cx + r; ++i) {
            for (int j = cy - r; j <= cy + r; ++j) {
                if (i >= 0 && i < nx_ && j >= 0 && j < ny_) {
                    u_[i][j] = 1.0;
                }
            }
        }
    }

    void step() {
        double alpha_x = D_ * dt_ / (dx_ * dx_);
        double alpha_y = D_ * dt_ / (dy_ * dy_);

        // Update interior points
        for (int i = 1; i < nx_ - 1; ++i) {
            for (int j = 1; j < ny_ - 1; ++j) {
                u_new_[i][j] = u_[i][j]
                    + alpha_x * (u_[i+1][j] - 2.0*u_[i][j] + u_[i-1][j])
                    + alpha_y * (u_[i][j+1] - 2.0*u_[i][j] + u_[i][j-1]);
            }
        }

        // Boundary conditions (Neumann)
        for (int i = 0; i < nx_; ++i) {
            u_new_[i][0] = u_new_[i][1];
            u_new_[i][ny_-1] = u_new_[i][ny_-2];
        }
        for (int j = 0; j < ny_; ++j) {
            u_new_[0][j] = u_new_[1][j];
            u_new_[nx_-1][j] = u_new_[nx_-2][j];
        }

        std::swap(u_, u_new_);
    }

    void run(int steps) {
        for (int s = 0; s < steps; ++s) {
            step();
        }
    }

    double getSum() const {
        double sum = 0.0;
        for (int i = 0; i < nx_; ++i) {
            for (int j = 0; j < ny_; ++j) {
                sum += u_[i][j];
            }
        }
        return sum;
    }
};

/**
 * @brief Simulated GPU diffusion solver
 * In actual implementation, this would use CUDA/HIP kernels
 */
class GPUDiffusionSolverSimulated {
private:
    int nx_, ny_;
    CPUDiffusionSolver cpu_solver_;  // For simulation

public:
    GPUDiffusionSolverSimulated(int nx, int ny, double L, double D, double dt)
        : nx_(nx), ny_(ny), cpu_solver_(nx, ny, L, D, dt)
    {
        // In real GPU implementation:
        // 1. Allocate device memory
        // 2. Copy data to GPU
        // 3. Launch kernel
    }

    void run(int steps) {
        // Simulate GPU execution
        cpu_solver_.run(steps);
    }

    double getSum() const {
        return cpu_solver_.getSum();
    }

    /**
     * @brief Estimate GPU speedup based on problem size
     */
    static double estimateSpeedup(int nx, int ny) {
        int total_points = nx * ny;

        if (total_points < 10000) {
            // Small problems: overhead dominates
            return 0.5 + total_points / 20000.0;
        } else if (total_points < 100000) {
            // Medium problems: good speedup
            return 1.0 + (total_points - 10000) / 5000.0;
        } else if (total_points < 1000000) {
            // Large problems: excellent speedup
            return 20.0 + (total_points - 100000) / 20000.0;
        } else {
            // Very large problems: saturates around 50x
            return std::min(50.0, 40.0 + (total_points - 1000000) / 100000.0);
        }
    }
};

/**
 * @brief Performance benchmark result
 */
struct BenchmarkResult {
    int nx, ny;
    int steps;
    double cpu_time_ms;
    double gpu_time_ms;
    double speedup;
    double cpu_throughput;  // updates/second
    double gpu_throughput;
    std::string recommendation;
};

/**
 * @brief Run benchmark for a given problem size
 */
BenchmarkResult runBenchmark(int nx, int ny, int steps) {
    BenchmarkResult result;
    result.nx = nx;
    result.ny = ny;
    result.steps = steps;

    const double L = 1.0;
    const double D = 0.1;
    const double dt = 0.001;

    std::cout << "\nBenchmarking " << nx << "x" << ny << " grid ("
              << (nx * ny) << " points) for " << steps << " steps...\n";

    // CPU benchmark
    {
        CPUDiffusionSolver solver(nx, ny, L, D, dt);
        auto start = std::chrono::high_resolution_clock::now();
        solver.run(steps);
        auto end = std::chrono::high_resolution_clock::now();

        result.cpu_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        result.cpu_throughput = (steps * 1000.0) / result.cpu_time_ms;

        std::cout << "  CPU time: " << std::fixed << std::setprecision(2)
                  << result.cpu_time_ms << " ms\n";
    }

    // GPU benchmark (simulated)
    {
        GPUDiffusionSolverSimulated solver(nx, ny, L, D, dt);

        // Simulate GPU speedup
        double speedup = GPUDiffusionSolverSimulated::estimateSpeedup(nx, ny);
        result.gpu_time_ms = result.cpu_time_ms / speedup;
        result.gpu_throughput = (steps * 1000.0) / result.gpu_time_ms;
        result.speedup = speedup;

        std::cout << "  GPU time: " << result.gpu_time_ms << " ms (simulated)\n";
        std::cout << "  Speedup: " << std::setprecision(1) << speedup << "x\n";
    }

    // Recommendation
    if (result.speedup < 1.5) {
        result.recommendation = "Use CPU (GPU overhead too high)";
    } else if (result.speedup < 5.0) {
        result.recommendation = "CPU acceptable, GPU provides moderate speedup";
    } else if (result.speedup < 20.0) {
        result.recommendation = "GPU recommended (good speedup)";
    } else {
        result.recommendation = "GPU highly recommended (excellent speedup)";
    }

    return result;
}

/**
 * @brief Print results table
 */
void printResultsTable(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "                    Performance Comparison Summary                      \n";
    std::cout << "========================================================================\n\n";

    std::cout << std::left;
    std::cout << std::setw(12) << "Grid Size"
              << std::setw(12) << "Points"
              << std::setw(12) << "CPU (ms)"
              << std::setw(12) << "GPU (ms)"
              << std::setw(10) << "Speedup"
              << "Recommendation\n";
    std::cout << "------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::cout << std::setw(12) << (std::to_string(r.nx) + "x" + std::to_string(r.ny))
                  << std::setw(12) << (r.nx * r.ny)
                  << std::setw(12) << std::fixed << std::setprecision(2) << r.cpu_time_ms
                  << std::setw(12) << r.gpu_time_ms
                  << std::setw(10) << std::setprecision(1) << r.speedup << "x  "
                  << r.recommendation << "\n";
    }

    std::cout << "========================================================================\n\n";
}

/**
 * @brief Analyze scaling behavior
 */
void analyzeScaling(const std::vector<BenchmarkResult>& results) {
    std::cout << "Scaling Analysis:\n";
    std::cout << "-----------------\n\n";

    if (results.size() < 2) return;

    // CPU scaling
    double cpu_scaling = results.back().cpu_time_ms / results.front().cpu_time_ms;
    double size_ratio = (double)(results.back().nx * results.back().ny) /
                       (double)(results.front().nx * results.front().ny);

    std::cout << "CPU Performance:\n";
    std::cout << "  Size increased by: " << std::fixed << std::setprecision(1)
              << size_ratio << "x\n";
    std::cout << "  Time increased by: " << cpu_scaling << "x\n";
    std::cout << "  Scaling factor: " << (cpu_scaling / size_ratio) << " (ideal = 1.0)\n\n";

    // GPU scaling
    double gpu_scaling = results.back().gpu_time_ms / results.front().gpu_time_ms;

    std::cout << "GPU Performance:\n";
    std::cout << "  Size increased by: " << size_ratio << "x\n";
    std::cout << "  Time increased by: " << gpu_scaling << "x\n";
    std::cout << "  Scaling factor: " << (gpu_scaling / size_ratio) << " (ideal = 1.0)\n\n";

    // Speedup trend
    std::cout << "Speedup Trend:\n";
    std::cout << "  Small problems (" << results.front().nx << "x" << results.front().ny
              << "): " << results.front().speedup << "x\n";
    std::cout << "  Large problems (" << results.back().nx << "x" << results.back().ny
              << "): " << results.back().speedup << "x\n";
    std::cout << "  Conclusion: Speedup " << (results.back().speedup > results.front().speedup ? "improves" : "degrades")
              << " with problem size\n\n";
}

/**
 * @brief Provide recommendations
 */
void provideRecommendations(const std::vector<BenchmarkResult>& results) {
    std::cout << "========================================================================\n";
    std::cout << "                         Recommendations                                \n";
    std::cout << "========================================================================\n\n";

    // Find crossover point
    size_t crossover_idx = 0;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].speedup > 2.0) {
            crossover_idx = i;
            break;
        }
    }

    if (crossover_idx > 0) {
        std::cout << "1. Problem Size Recommendations:\n";
        std::cout << "   - Use CPU for problems smaller than ~"
                  << results[crossover_idx].nx << "x" << results[crossover_idx].ny
                  << " (" << (results[crossover_idx].nx * results[crossover_idx].ny)
                  << " points)\n";
        std::cout << "   - Use GPU for problems larger than this threshold\n\n";
    }

    std::cout << "2. Performance Characteristics:\n";
    std::cout << "   - GPU has initialization overhead (~1-10 ms)\n";
    std::cout << "   - GPU excels at large, parallel computations\n";
    std::cout << "   - CPU is better for small problems and complex logic\n\n";

    std::cout << "3. Optimization Tips:\n";
    std::cout << "   - For GPU: Maximize occupancy, use shared memory\n";
    std::cout << "   - For CPU: Enable vectorization (AVX/SSE), use OpenMP\n";
    std::cout << "   - For both: Minimize memory transfers, use in-place operations\n\n";

    std::cout << "4. Real-world Considerations:\n";
    std::cout << "   - GPU requires CUDA/HIP installation\n";
    std::cout << "   - CPU-only code is more portable\n";
    std::cout << "   - Consider hybrid approaches for mixed workloads\n\n";

    std::cout << "========================================================================\n";
}

int main() {
    std::cout << "========================================================================\n";
    std::cout << "         CPU vs GPU Performance Comparison for 2D Diffusion            \n";
    std::cout << "========================================================================\n";

    // Problem sizes to test
    std::vector<std::pair<int, int>> problem_sizes = {
        {32, 32},      // Small
        {64, 64},      // Medium-small
        {128, 128},    // Medium
        {256, 256},    // Medium-large
        {512, 512},    // Large
        {1024, 1024}   // Very large
    };

    const int steps = 100;

    std::vector<BenchmarkResult> results;

    // Run benchmarks
    for (const auto& size : problem_sizes) {
        results.push_back(runBenchmark(size.first, size.second, steps));
    }

    // Print results
    printResultsTable(results);
    analyzeScaling(results);
    provideRecommendations(results);

    std::cout << "\nNote: GPU times are simulated based on typical speedup factors.\n";
    std::cout << "Actual performance depends on hardware, kernel optimization, and problem characteristics.\n";

    return 0;
}
