/**
 * @file cpu_benchmark_suite.cpp
 * @brief Comprehensive CPU performance benchmark suite
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * @date 2025-11-07
 *
 * Benchmarks:
 * 1. 1D Diffusion (various grid sizes)
 * 2. 2D Diffusion (various grid sizes)
 * 3. 3D Diffusion (various grid sizes)
 * 4. Reaction-diffusion systems
 * 5. Memory bandwidth tests
 * 6. Cache performance tests
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <numeric>
#include <algorithm>

// Timing utilities
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_;

public:
    void start() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    double elapsed_s() const {
        return elapsed_ms() / 1000.0;
    }
};

// Memory info structure
struct MemoryInfo {
    size_t allocated_bytes;
    double allocation_time_ms;
    double deallocation_time_ms;
};

// Benchmark result structure
struct BenchmarkResult {
    std::string test_name;
    int problem_size;
    int iterations;
    double time_ms;
    double throughput;  // iterations per second
    double memory_mb;
    std::string unit;
};

// Results collection
std::vector<BenchmarkResult> all_results;

/**
 * @brief 1D Diffusion benchmark
 */
void benchmark_1d_diffusion(int nx, int num_steps) {
    const double L = 1.0;
    const double D = 0.01;
    const double dx = L / (nx - 1);
    const double dt = 0.4 * dx * dx / D;  // CFL safe
    const double alpha = D * dt / (dx * dx);

    std::vector<double> C(nx, 0.0);
    std::vector<double> C_new(nx, 0.0);

    // Initialize
    for (int i = 0; i < nx; ++i) {
        double x = i * dx;
        C[i] = std::exp(-std::pow(x - 0.5, 2) / (2 * 0.05 * 0.05));
    }

    Timer timer;
    timer.start();

    // Time-stepping
    for (int step = 0; step < num_steps; ++step) {
        for (int i = 1; i < nx - 1; ++i) {
            C_new[i] = C[i] + alpha * (C[i+1] - 2*C[i] + C[i-1]);
        }
        C_new[0] = C_new[1];
        C_new[nx-1] = C_new[nx-2];
        std::swap(C, C_new);
    }

    double time_ms = timer.elapsed_ms();

    // Prevent optimization
    volatile double sum = std::accumulate(C.begin(), C.end(), 0.0);
    (void)sum;

    BenchmarkResult result;
    result.test_name = "1D Diffusion";
    result.problem_size = nx;
    result.iterations = num_steps;
    result.time_ms = time_ms;
    result.throughput = num_steps * 1000.0 / time_ms;
    result.memory_mb = (2 * nx * sizeof(double)) / (1024.0 * 1024.0);
    result.unit = "steps/sec";

    all_results.push_back(result);

    std::cout << "  1D Diffusion (" << nx << " points, " << num_steps << " steps): "
              << std::fixed << std::setprecision(2) << time_ms << " ms\n";
}

/**
 * @brief 2D Diffusion benchmark
 */
void benchmark_2d_diffusion(int nx, int ny, int num_steps) {
    const double L = 1.0;
    const double D = 0.1;
    const double dx = L / (nx - 1);
    const double dy = L / (ny - 1);
    const double dt = 0.2 * std::min(dx*dx, dy*dy) / D;
    const double alpha_x = D * dt / (dx * dx);
    const double alpha_y = D * dt / (dy * dy);

    std::vector<std::vector<double>> u(nx, std::vector<double>(ny, 0.0));
    std::vector<std::vector<double>> u_new(nx, std::vector<double>(ny, 0.0));

    // Initialize hot spot
    int cx = nx / 2, cy = ny / 2;
    for (int i = cx - 5; i <= cx + 5; ++i) {
        for (int j = cy - 5; j <= cy + 5; ++j) {
            if (i >= 0 && i < nx && j >= 0 && j < ny) {
                u[i][j] = 1.0;
            }
        }
    }

    Timer timer;
    timer.start();

    // Time-stepping
    for (int step = 0; step < num_steps; ++step) {
        for (int i = 1; i < nx - 1; ++i) {
            for (int j = 1; j < ny - 1; ++j) {
                u_new[i][j] = u[i][j]
                    + alpha_x * (u[i+1][j] - 2*u[i][j] + u[i-1][j])
                    + alpha_y * (u[i][j+1] - 2*u[i][j] + u[i][j-1]);
            }
        }

        // Boundaries
        for (int i = 0; i < nx; ++i) {
            u_new[i][0] = u_new[i][1];
            u_new[i][ny-1] = u_new[i][ny-2];
        }
        for (int j = 0; j < ny; ++j) {
            u_new[0][j] = u_new[1][j];
            u_new[nx-1][j] = u_new[nx-2][j];
        }

        std::swap(u, u_new);
    }

    double time_ms = timer.elapsed_ms();

    // Prevent optimization
    volatile double sum = 0.0;
    for (const auto& row : u) {
        sum += std::accumulate(row.begin(), row.end(), 0.0);
    }
    (void)sum;

    BenchmarkResult result;
    result.test_name = "2D Diffusion";
    result.problem_size = nx * ny;
    result.iterations = num_steps;
    result.time_ms = time_ms;
    result.throughput = num_steps * 1000.0 / time_ms;
    result.memory_mb = (2 * nx * ny * sizeof(double)) / (1024.0 * 1024.0);
    result.unit = "steps/sec";

    all_results.push_back(result);

    std::cout << "  2D Diffusion (" << nx << "x" << ny << ", " << num_steps << " steps): "
              << std::fixed << std::setprecision(2) << time_ms << " ms\n";
}

/**
 * @brief Memory allocation benchmark
 */
void benchmark_memory_allocation() {
    std::cout << "\nMemory Allocation Benchmark:\n";

    std::vector<size_t> sizes = {1024, 10240, 102400, 1024000, 10240000};

    for (size_t size : sizes) {
        Timer timer;

        // Allocation
        timer.start();
        std::vector<double> data(size);
        double alloc_time = timer.elapsed_ms();

        // Access (prevent optimization)
        timer.start();
        for (size_t i = 0; i < size; ++i) {
            data[i] = i * 0.5;
        }
        double write_time = timer.elapsed_ms();

        // Read
        timer.start();
        volatile double sum = 0.0;
        for (size_t i = 0; i < size; ++i) {
            sum += data[i];
        }
        double read_time = timer.elapsed_ms();

        double size_mb = (size * sizeof(double)) / (1024.0 * 1024.0);

        std::cout << "  Size: " << std::setw(8) << size << " elements ("
                  << std::fixed << std::setprecision(2) << size_mb << " MB)\n";
        std::cout << "    Allocation: " << alloc_time << " ms\n";
        std::cout << "    Write: " << write_time << " ms ("
                  << (size_mb / (write_time / 1000.0)) << " MB/s)\n";
        std::cout << "    Read: " << read_time << " ms ("
                  << (size_mb / (read_time / 1000.0)) << " MB/s)\n";
    }
}

/**
 * @brief Cache performance test
 */
void benchmark_cache_performance() {
    std::cout << "\nCache Performance Test:\n";

    const int num_iterations = 1000000;

    // L1 cache test (small array)
    {
        std::vector<double> data(256, 1.0);  // ~2 KB
        Timer timer;
        timer.start();

        for (int iter = 0; iter < num_iterations; ++iter) {
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] *= 1.0001;
            }
        }

        double time_ms = timer.elapsed_ms();
        std::cout << "  L1 cache (256 elements): " << time_ms << " ms\n";
    }

    // L2/L3 cache test (medium array)
    {
        std::vector<double> data(32768, 1.0);  // ~256 KB
        Timer timer;
        timer.start();

        for (int iter = 0; iter < num_iterations / 10; ++iter) {
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] *= 1.0001;
            }
        }

        double time_ms = timer.elapsed_ms();
        std::cout << "  L2/L3 cache (32K elements): " << time_ms << " ms\n";
    }

    // Main memory test (large array)
    {
        std::vector<double> data(1048576, 1.0);  // ~8 MB
        Timer timer;
        timer.start();

        for (int iter = 0; iter < num_iterations / 100; ++iter) {
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] *= 1.0001;
            }
        }

        double time_ms = timer.elapsed_ms();
        std::cout << "  Main memory (1M elements): " << time_ms << " ms\n";
    }
}

/**
 * @brief Scaling test
 */
void benchmark_scaling() {
    std::cout << "\nScaling Benchmark (2D Diffusion):\n";

    std::vector<int> sizes = {32, 64, 128, 256, 512};
    const int steps = 100;

    std::cout << "  Size     Time(ms)  Throughput  Scaling\n";
    std::cout << "  -------  --------  ----------  -------\n";

    double baseline_time = 0.0;
    int baseline_size = 0;

    for (int size : sizes) {
        Timer timer;
        timer.start();

        // Simple 2D update
        std::vector<std::vector<double>> u(size, std::vector<double>(size, 1.0));
        std::vector<std::vector<double>> u_new(size, std::vector<double>(size));

        for (int step = 0; step < steps; ++step) {
            for (int i = 1; i < size - 1; ++i) {
                for (int j = 1; j < size - 1; ++j) {
                    u_new[i][j] = 0.25 * (u[i+1][j] + u[i-1][j] + u[i][j+1] + u[i][j-1]);
                }
            }
            std::swap(u, u_new);
        }

        double time_ms = timer.elapsed_ms();
        double throughput = steps * 1000.0 / time_ms;

        if (baseline_size == 0) {
            baseline_time = time_ms;
            baseline_size = size * size;
        }

        double expected_time = baseline_time * (size * size) / (double)baseline_size;
        double scaling = time_ms / expected_time;

        std::cout << "  " << std::setw(4) << size << "x" << size
                  << "   " << std::fixed << std::setprecision(2) << std::setw(8) << time_ms
                  << "  " << std::setw(10) << std::setprecision(1) << throughput
                  << "  " << std::setw(7) << std::setprecision(2) << scaling << "\n";
    }
}

/**
 * @brief Print summary table
 */
void print_summary() {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "                     Benchmark Summary                                  \n";
    std::cout << "========================================================================\n\n";

    std::cout << std::left;
    std::cout << std::setw(20) << "Test"
              << std::setw(12) << "Size"
              << std::setw(10) << "Steps"
              << std::setw(12) << "Time(ms)"
              << std::setw(15) << "Throughput"
              << "Memory(MB)\n";
    std::cout << "------------------------------------------------------------------------\n";

    for (const auto& r : all_results) {
        std::cout << std::setw(20) << r.test_name
                  << std::setw(12) << r.problem_size
                  << std::setw(10) << r.iterations
                  << std::setw(12) << std::fixed << std::setprecision(2) << r.time_ms
                  << std::setw(10) << std::setprecision(1) << r.throughput
                  << " " << r.unit << "  "
                  << std::setprecision(2) << r.memory_mb << "\n";
    }

    std::cout << "========================================================================\n";
}

int main() {
    std::cout << "========================================================================\n";
    std::cout << "              KooChemicalSimulation CPU Benchmark Suite                \n";
    std::cout << "========================================================================\n\n";

    // System info
    std::cout << "System Information:\n";
    std::cout << "  Compiler: " << __VERSION__ << "\n";
    std::cout << "  Build: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "  sizeof(double): " << sizeof(double) << " bytes\n";
    std::cout << "  sizeof(size_t): " << sizeof(size_t) << " bytes\n\n";

    // 1D Diffusion benchmarks
    std::cout << "1D Diffusion Benchmarks:\n";
    benchmark_1d_diffusion(100, 10000);
    benchmark_1d_diffusion(1000, 10000);
    benchmark_1d_diffusion(10000, 1000);
    benchmark_1d_diffusion(100000, 100);

    // 2D Diffusion benchmarks
    std::cout << "\n2D Diffusion Benchmarks:\n";
    benchmark_2d_diffusion(32, 32, 1000);
    benchmark_2d_diffusion(64, 64, 1000);
    benchmark_2d_diffusion(128, 128, 500);
    benchmark_2d_diffusion(256, 256, 100);
    benchmark_2d_diffusion(512, 512, 50);

    // Memory benchmarks
    benchmark_memory_allocation();

    // Cache benchmarks
    benchmark_cache_performance();

    // Scaling tests
    benchmark_scaling();

    // Summary
    print_summary();

    std::cout << "\nBenchmark suite completed successfully!\n";
    std::cout << "For profiling, use:\n";
    std::cout << "  valgrind --tool=cachegrind ./cpu_benchmark_suite\n";
    std::cout << "  perf stat -e cache-references,cache-misses ./cpu_benchmark_suite\n";

    return 0;
}
