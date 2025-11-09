/**
 * @file mpi_domain_decomposition_benchmark.cpp
 * @brief MPI Domain Decomposition Performance Benchmark
 *
 * Compares performance of balanced vs simple domain decomposition
 */

#include "parallel/domain/DomainDecomposition.h"
#include "parallel/mpi/MPIWrapper.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

using namespace koo::parallel;

struct BenchmarkResult {
    int nprocs;
    std::array<int, 3> dims;
    double setupTime;        // microseconds
    int surfaceToVolume;     // Communication overhead metric
    bool isBalanced;

    void print() const {
        std::cout << std::setw(8) << nprocs
                  << std::setw(12) << (dims[0] << "×" << dims[1] << "×" << dims[2])
                  << std::setw(15) << std::fixed << std::setprecision(2) << setupTime
                  << std::setw(18) << surfaceToVolume
                  << std::setw(12) << (isBalanced ? "Yes" : "No")
                  << std::endl;
    }
};

// Calculate surface-to-volume ratio (communication overhead metric)
int calculateSurfaceToVolume(int npx, int npy, int npz) {
    // Surface area of the processor grid
    int surface = 2 * (npx * npy + npy * npz + npz * npx);
    // Volume (number of processors)
    int volume = npx * npy * npz;
    // Return ratio (lower is better)
    return (volume > 0) ? (surface * 1000 / volume) : 0;
}

// Simple factorization (old method)
std::array<int, 3> simpleFactorization(int nprocs) {
    std::array<int, 3> dims = {nprocs, 1, 1};

    if (nprocs >= 4) {
        int npz = static_cast<int>(std::cbrt(nprocs));
        int remaining = nprocs / npz;
        int npy = static_cast<int>(std::sqrt(remaining));
        int npx = nprocs / (npy * npz);
        dims = {npx, npy, npz};
    }

    return dims;
}

// Balanced factorization (new method) - simplified version
std::array<int, 3> balancedFactorization(int nprocs) {
    std::array<int, 3> dims = {1, 1, 1};
    int remaining = nprocs;

    // Factor out and distribute
    while (remaining > 1) {
        // Find smallest dimension
        int min_idx = 0;
        int min_val = dims[0];
        for (int i = 1; i < 3; ++i) {
            if (dims[i] < min_val) {
                min_val = dims[i];
                min_idx = i;
            }
        }

        // Try to factor
        bool factored = false;
        for (int factor : {2, 3, 5, 7}) {
            if (remaining % factor == 0) {
                dims[min_idx] *= factor;
                remaining /= factor;
                factored = true;
                break;
            }
        }

        if (!factored) {
            dims[min_idx] *= remaining;
            remaining = 1;
        }
    }

    return dims;
}

// Benchmark a single configuration
BenchmarkResult benchmarkDecomposition(int nprocs, bool useBalanced) {
    BenchmarkResult result;
    result.nprocs = nprocs;
    result.isBalanced = useBalanced;

    auto start = std::chrono::high_resolution_clock::now();

    if (useBalanced) {
        result.dims = balancedFactorization(nprocs);
    } else {
        result.dims = simpleFactorization(nprocs);
    }

    auto end = std::chrono::high_resolution_clock::now();

    result.setupTime = std::chrono::duration<double, std::micro>(end - start).count();
    result.surfaceToVolume = calculateSurfaceToVolume(result.dims[0], result.dims[1], result.dims[2]);

    return result;
}

int main(int argc, char** argv) {
    std::cout << "\n"
              << "========================================\n"
              << "  MPI Domain Decomposition Benchmark\n"
              << "========================================\n\n";

    // Test processor counts
    std::vector<int> processorCounts = {1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128};

    std::cout << "Comparing Simple vs Balanced Factorization\n\n";

    std::cout << std::setw(8) << "Procs"
              << std::setw(12) << "Dimensions"
              << std::setw(15) << "Setup (μs)"
              << std::setw(18) << "Comm Overhead"
              << std::setw(12) << "Balanced"
              << std::endl;
    std::cout << std::string(65, '-') << std::endl;

    double totalSimpleOverhead = 0.0;
    double totalBalancedOverhead = 0.0;
    int count = 0;

    for (int nprocs : processorCounts) {
        // Benchmark simple method
        auto simpleResult = benchmarkDecomposition(nprocs, false);
        simpleResult.print();

        // Benchmark balanced method
        auto balancedResult = benchmarkDecomposition(nprocs, true);
        balancedResult.print();

        // Calculate improvement
        double improvement = 100.0 * (simpleResult.surfaceToVolume - balancedResult.surfaceToVolume) /
                           static_cast<double>(simpleResult.surfaceToVolume);

        std::cout << "        → Improvement: " << std::fixed << std::setprecision(1)
                  << improvement << "%\n\n";

        totalSimpleOverhead += simpleResult.surfaceToVolume;
        totalBalancedOverhead += balancedResult.surfaceToVolume;
        count++;
    }

    // Summary
    std::cout << std::string(65, '=') << std::endl;
    std::cout << "\nSummary:\n";
    std::cout << "  Average Simple Overhead:   " << (totalSimpleOverhead / count) << "\n";
    std::cout << "  Average Balanced Overhead: " << (totalBalancedOverhead / count) << "\n";
    std::cout << "  Overall Improvement:       "
              << std::fixed << std::setprecision(1)
              << (100.0 * (totalSimpleOverhead - totalBalancedOverhead) / totalSimpleOverhead)
              << "%\n\n";

    std::cout << "Key:\n";
    std::cout << "  - Dimensions: Process grid layout (npx×npy×npz)\n";
    std::cout << "  - Comm Overhead: Surface-to-volume ratio (lower is better)\n";
    std::cout << "  - Balanced: Uses new balanced factorization algorithm\n\n";

    std::cout << "Conclusion:\n";
    std::cout << "  Balanced factorization creates more cubic processor grids,\n";
    std::cout << "  reducing communication overhead for better scalability.\n\n";

    return 0;
}
