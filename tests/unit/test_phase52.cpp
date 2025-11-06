/**
 * @file test_phase52.cpp
 * @brief Unit tests for Phase 52: GPU Linear Algebra
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha1
 */

#include "gpu/linalg/Vector.h"
#include "gpu/linalg/Matrix.h"
#include "gpu/linalg/Solvers.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

using namespace koo::gpu::linalg;

// Test counter
int tests_passed = 0;
int tests_total = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        tests_total++; \
        std::cout << "Test " << tests_total << ": " << #name << " ... "; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << "PASSED" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << "FAILED: " << e.what() << std::endl; \
        } \
    } \
    void test_##name()

// ============================================
// Phase 52 Tests
// ============================================

TEST(gpu_vector_creation) {
    GPUVectorD vec(100);
    assert(vec.size() == 100);
    assert(!vec.empty());
}

TEST(gpu_vector_from_host) {
    std::vector<double> hostData = {1.0, 2.0, 3.0, 4.0, 5.0};
    GPUVectorD vec(hostData);

    auto result = vec.toHost();
    assert(result.size() == 5);
    for (size_t i = 0; i < 5; ++i) {
        assert(std::abs(result[i] - hostData[i]) < 1e-10);
    }
}

TEST(gpu_vector_zero_fill) {
    GPUVectorD vec(50);
    vec.zero();

    auto result = vec.toHost();
    for (double val : result) {
        assert(val == 0.0);
    }
}

TEST(gpu_vector_scale) {
    std::vector<double> hostData = {1.0, 2.0, 3.0, 4.0};
    GPUVectorD vec(hostData);

    vec.scale(2.0);

    auto result = vec.toHost();
    for (size_t i = 0; i < 4; ++i) {
        assert(std::abs(result[i] - hostData[i] * 2.0) < 1e-10);
    }
}

TEST(gpu_vector_axpy) {
    std::vector<double> x_data = {1.0, 2.0, 3.0};
    std::vector<double> y_data = {4.0, 5.0, 6.0};

    GPUVectorD x(x_data);
    GPUVectorD y(y_data);

    y.axpy(2.0, x);  // y = 2*x + y

    auto result = y.toHost();
    std::vector<double> expected = {6.0, 9.0, 12.0};
    for (size_t i = 0; i < 3; ++i) {
        assert(std::abs(result[i] - expected[i]) < 1e-10);
    }
}

TEST(gpu_vector_dot_product) {
    std::vector<double> x_data = {1.0, 2.0, 3.0};
    std::vector<double> y_data = {4.0, 5.0, 6.0};

    GPUVectorD x(x_data);
    GPUVectorD y(y_data);

    double dot = x.dot(y);
    double expected = 1*4 + 2*5 + 3*6;  // 32

    assert(std::abs(dot - expected) < 1e-10);
}

TEST(gpu_vector_norm2) {
    std::vector<double> data = {3.0, 4.0};  // 3-4-5 triangle
    GPUVectorD vec(data);

    double norm = vec.norm2();
    double expected = 5.0;

    assert(std::abs(norm - expected) < 1e-10);
}

TEST(gpu_vector_norm1) {
    std::vector<double> data = {-1.0, 2.0, -3.0};
    GPUVectorD vec(data);

    double norm = vec.norm1();
    double expected = 6.0;

    assert(std::abs(norm - expected) < 1e-10);
}

TEST(gpu_sparse_matrix_creation) {
    // 3x3 identity matrix
    std::vector<int> rows = {0, 1, 2};
    std::vector<int> cols = {0, 1, 2};
    std::vector<double> vals = {1.0, 1.0, 1.0};

    GPUSparseMatrixD A(3, 3, rows, cols, vals);

    assert(A.numRows() == 3);
    assert(A.numCols() == 3);
    assert(A.nnz() == 3);
}

TEST(gpu_sparse_matrix_spmv) {
    // 3x3 diagonal matrix with values [2, 3, 4]
    std::vector<int> rows = {0, 1, 2};
    std::vector<int> cols = {0, 1, 2};
    std::vector<double> vals = {2.0, 3.0, 4.0};

    GPUSparseMatrixD A(3, 3, rows, cols, vals);

    std::vector<double> x_data = {1.0, 1.0, 1.0};
    GPUVectorD x(x_data);
    GPUVectorD y(3);

    A.spmv(x, y);

    auto result = y.toHost();
    std::vector<double> expected = {2.0, 3.0, 4.0};
    for (size_t i = 0; i < 3; ++i) {
        assert(std::abs(result[i] - expected[i]) < 1e-10);
    }
}

TEST(cg_solver_simple) {
    // Solve: A*x = b where A is 2x2 identity, b = [1, 2]
    std::vector<int> rows = {0, 1};
    std::vector<int> cols = {0, 1};
    std::vector<double> vals = {1.0, 1.0};

    GPUSparseMatrixD A(2, 2, rows, cols, vals);

    std::vector<double> b_data = {1.0, 2.0};
    GPUVectorD b(b_data);
    GPUVectorD x(2);

    ConjugateGradient<double> cg(A, 100, 1e-6);
    auto stats = cg.solve(b, x);

    assert(stats.converged);

    auto solution = x.toHost();
    assert(std::abs(solution[0] - 1.0) < 1e-5);
    assert(std::abs(solution[1] - 2.0) < 1e-5);
}

TEST(bicgstab_solver_simple) {
    // Solve: A*x = b where A is 2x2 diagonal [3, 4], b = [6, 8]
    std::vector<int> rows = {0, 1};
    std::vector<int> cols = {0, 1};
    std::vector<double> vals = {3.0, 4.0};

    GPUSparseMatrixD A(2, 2, rows, cols, vals);

    std::vector<double> b_data = {6.0, 8.0};
    GPUVectorD b(b_data);
    GPUVectorD x(2);

    BiCGStab<double> bicg(A, 100, 1e-6);
    auto stats = bicg.solve(b, x);

    assert(stats.converged);

    auto solution = x.toHost();
    assert(std::abs(solution[0] - 2.0) < 1e-5);
    assert(std::abs(solution[1] - 2.0) < 1e-5);
}

// ============================================
// Main
// ============================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Phase 52: GPU Linear Algebra Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_gpu_vector_creation();
    run_test_gpu_vector_from_host();
    run_test_gpu_vector_zero_fill();
    run_test_gpu_vector_scale();
    run_test_gpu_vector_axpy();
    run_test_gpu_vector_dot_product();
    run_test_gpu_vector_norm2();
    run_test_gpu_vector_norm1();
    run_test_gpu_sparse_matrix_creation();
    run_test_gpu_sparse_matrix_spmv();
    run_test_cg_solver_simple();
    run_test_bicgstab_solver_simple();

    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << " / " << tests_total << std::endl;
    std::cout << "========================================" << std::endl;

    if (tests_passed == tests_total) {
        std::cout << "✓ All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some tests FAILED" << std::endl;
        return 1;
    }
}
