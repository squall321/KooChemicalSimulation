/**
 * @file IntegrationTest.h
 * @brief Integration testing framework
 * @author KooChemicalSimulation Development Team
 * @version 5.0.0
 * @date 2025-11-06
 *
 * Phase 46-47: Testing & Validation Framework
 *
 * Comprehensive integration testing with:
 * - Multi-module integration tests
 * - Performance benchmarks
 * - Validation against analytical solutions
 * - Regression testing
 */

#ifndef KOO_TESTING_INTEGRATION_TEST_H
#define KOO_TESTING_INTEGRATION_TEST_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace koo {
namespace testing {

// ============================================================================
// Test Result
// ============================================================================

/**
 * @brief Test result
 */
struct TestResult {
    std::string name;
    bool passed;
    double duration;  ///< Execution time in seconds
    std::string message;

    TestResult(const std::string& n, bool p, double d, const std::string& m = "")
        : name(n), passed(p), duration(d), message(m) {}
};

// ============================================================================
// Test Suite
// ============================================================================

/**
 * @brief Test suite for integration tests
 */
class TestSuite {
public:
    /**
     * @brief Add a test
     */
    void addTest(const std::string& name, std::function<bool()> test) {
        tests_.push_back({name, test});
    }

    /**
     * @brief Run all tests
     */
    std::vector<TestResult> runAll() {
        std::vector<TestResult> results;

        std::cout << "Running " << tests_.size() << " integration tests..." << std::endl;
        std::cout << "========================================" << std::endl;

        for (const auto& test : tests_) {
            std::cout << "Running: " << test.first << "... " << std::flush;

            auto start = std::chrono::high_resolution_clock::now();
            bool passed = false;
            std::string message;

            try {
                passed = test.second();
            } catch (const std::exception& e) {
                passed = false;
                message = std::string("Exception: ") + e.what();
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;

            results.emplace_back(test.first, passed, duration.count(), message);

            if (passed) {
                std::cout << "PASS (" << std::fixed << std::setprecision(3)
                         << duration.count() << "s)" << std::endl;
            } else {
                std::cout << "FAIL";
                if (!message.empty()) {
                    std::cout << " - " << message;
                }
                std::cout << std::endl;
            }
        }

        std::cout << "========================================" << std::endl;
        return results;
    }

    /**
     * @brief Get summary
     */
    void printSummary(const std::vector<TestResult>& results) {
        int passed = 0;
        int failed = 0;
        double totalTime = 0.0;

        for (const auto& result : results) {
            if (result.passed) {
                passed++;
            } else {
                failed++;
            }
            totalTime += result.duration;
        }

        std::cout << "\nTest Summary:" << std::endl;
        std::cout << "  Total:  " << results.size() << std::endl;
        std::cout << "  Passed: " << passed << std::endl;
        std::cout << "  Failed: " << failed << std::endl;
        std::cout << "  Time:   " << std::fixed << std::setprecision(3)
                  << totalTime << "s" << std::endl;

        if (failed == 0) {
            std::cout << "\n✓ All tests passed!" << std::endl;
        } else {
            std::cout << "\n✗ Some tests failed!" << std::endl;
        }
    }

private:
    std::vector<std::pair<std::string, std::function<bool()>>> tests_;
};

// ============================================================================
// Benchmark Framework
// ============================================================================

/**
 * @brief Benchmark result
 */
struct BenchmarkResult {
    std::string name;
    double minTime;
    double maxTime;
    double avgTime;
    double stdDev;
    size_t iterations;
};

/**
 * @brief Benchmark framework
 */
class Benchmark {
public:
    /**
     * @brief Run benchmark
     */
    static BenchmarkResult run(const std::string& name,
                               std::function<void()> func,
                               size_t iterations = 100) {
        std::vector<double> times;
        times.reserve(iterations);

        // Warmup
        func();

        // Benchmark
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> duration = end - start;
            times.push_back(duration.count());
        }

        // Compute statistics
        double minTime = *std::min_element(times.begin(), times.end());
        double maxTime = *std::max_element(times.begin(), times.end());
        double avgTime = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

        double variance = 0.0;
        for (double t : times) {
            variance += (t - avgTime) * (t - avgTime);
        }
        variance /= times.size();
        double stdDev = std::sqrt(variance);

        return {name, minTime, maxTime, avgTime, stdDev, iterations};
    }

    /**
     * @brief Print benchmark results
     */
    static void printResults(const std::vector<BenchmarkResult>& results) {
        std::cout << "\nBenchmark Results:" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::left << std::setw(30) << "Name"
                  << std::right << std::setw(12) << "Min (s)"
                  << std::setw(12) << "Max (s)"
                  << std::setw(12) << "Avg (s)"
                  << std::setw(12) << "StdDev" << std::endl;
        std::cout << std::string(78, '-') << std::endl;

        for (const auto& result : results) {
            std::cout << std::left << std::setw(30) << result.name
                      << std::right << std::fixed << std::setprecision(6)
                      << std::setw(12) << result.minTime
                      << std::setw(12) << result.maxTime
                      << std::setw(12) << result.avgTime
                      << std::setw(12) << result.stdDev << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }
};

// ============================================================================
// Validation Utilities
// ============================================================================

/**
 * @brief Validation utilities
 */
class Validator {
public:
    /**
     * @brief Check if value is close to expected
     */
    static bool isClose(double value, double expected, double tolerance = 1e-6) {
        return std::abs(value - expected) < tolerance;
    }

    /**
     * @brief Check if vector is close to expected
     */
    static bool isClose(const std::vector<double>& values,
                       const std::vector<double>& expected,
                       double tolerance = 1e-6) {
        if (values.size() != expected.size()) return false;

        for (size_t i = 0; i < values.size(); ++i) {
            if (!isClose(values[i], expected[i], tolerance)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Compute relative error
     */
    static double relativeError(double value, double expected) {
        if (std::abs(expected) < 1e-15) return std::abs(value);
        return std::abs(value - expected) / std::abs(expected);
    }

    /**
     * @brief Compute L2 error
     */
    static double l2Error(const std::vector<double>& values,
                         const std::vector<double>& expected) {
        if (values.size() != expected.size()) return -1.0;

        double error = 0.0;
        for (size_t i = 0; i < values.size(); ++i) {
            double diff = values[i] - expected[i];
            error += diff * diff;
        }
        return std::sqrt(error / values.size());
    }
};

} // namespace testing
} // namespace koo

#endif // KOO_TESTING_INTEGRATION_TEST_H
