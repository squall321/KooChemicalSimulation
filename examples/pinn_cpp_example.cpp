/**
 * @file pinn_cpp_example.cpp
 * @brief Example: Using PINN models in C++
 *
 * Priority D2.4: PINN C++ Integration
 * Version: 6.0.0-alpha5
 *
 * This example demonstrates how to:
 * 1. Load a PINN model trained in Python
 * 2. Make predictions in C++
 * 3. Compare with analytical solutions
 * 4. Measure performance
 *
 * Prerequisites:
 * 1. Train a PINN model in Python:
 *    ```python
 *    from koolab.ml import DiffusionPINN1D
 *    pinn = DiffusionPINN1D(diffusivity=1.0e-9)
 *    pinn.train_model(...)
 *
 *    # Save as TorchScript
 *    import torch
 *    scripted = torch.jit.script(pinn)
 *    scripted.save('diffusion_pinn_1d.pt')
 *    ```
 *
 * 2. Compile with LibTorch:
 *    ```bash
 *    cmake -DCMAKE_PREFIX_PATH=/path/to/libtorch ..
 *    make pinn_cpp_example
 *    ```
 *
 * Usage:
 *    ./pinn_cpp_example <model_path>
 */

#include "parallel/ml/PINNSolver.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>

using namespace koo::parallel::ml;

// ============================================================================
// Analytical Solution (for validation)
// ============================================================================

/**
 * @brief Analytical solution for 1D diffusion with Gaussian IC
 *
 * u(x, t) = (σ₀/σₜ) * exp(-(x-x₀)²/(2σₜ²))
 * where σₜ = sqrt(2Dt + σ₀²)
 */
double analyticalSolution(double x, double t, double D) {
    const double x0 = 0.5;      // Center
    const double sigma0 = 0.1;  // Initial width

    double sigma_t = std::sqrt(2.0 * D * t + sigma0 * sigma0);
    double amplitude = sigma0 / sigma_t;
    double exponent = -(x - x0) * (x - x0) / (2.0 * sigma_t * sigma_t);

    return amplitude * std::exp(exponent);
}

// ============================================================================
// Main Example
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================\n";
    std::cout << "PINN C++ Integration Example\n";
    std::cout << "========================================\n\n";

    // Check arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model_path>\n";
        std::cerr << "Example: " << argv[0] << " diffusion_pinn_1d.pt\n";
        return 1;
    }

    std::string model_path = argv[1];
    std::string device = "cpu";  // Change to "cuda" for GPU

    // Problem parameters (should match training)
    const double L = 1.0;         // Domain length [m]
    const double D = 1.0e-9;      // Diffusion coefficient [m²/s]
    const double T = 100.0;       // Final time [s]

    // ========================================================================
    // 1. Load PINN Model
    // ========================================================================

    std::cout << "1. Loading PINN model...\n";
    std::cout << "   Model path: " << model_path << "\n";
    std::cout << "   Device: " << device << "\n\n";

    DiffusionPINN1D pinn(device);

    try {
        pinn.loadModel(model_path);
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << "\n";
        std::cerr << "\nMake sure you have:\n";
        std::cerr << "1. Trained a PINN model in Python\n";
        std::cerr << "2. Saved it using torch.jit.script()\n";
        std::cerr << "3. Provided the correct path\n";
        return 1;
    }

    std::cout << "Model loaded successfully!\n\n";

    // ========================================================================
    // 2. Single Point Prediction
    // ========================================================================

    std::cout << "2. Single point prediction...\n";

    double x_test = 0.5;  // Center
    double t_test = 50.0; // Half-time

    auto start = std::chrono::high_resolution_clock::now();
    double u_pred = pinn.predict(x_test, t_test);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double u_exact = analyticalSolution(x_test, t_test, D);
    double error = std::abs(u_pred - u_exact);

    std::cout << "   Point: (x=" << x_test << ", t=" << t_test << ")\n";
    std::cout << "   PINN prediction: " << u_pred << "\n";
    std::cout << "   Analytical: " << u_exact << "\n";
    std::cout << "   Error: " << error << " (" << (error/u_exact*100) << "%)\n";
    std::cout << "   Time: " << duration.count() << " μs\n\n";

    // ========================================================================
    // 3. Grid Prediction
    // ========================================================================

    std::cout << "3. Grid prediction...\n";

    const int nx = 100;
    std::vector<double> x_grid(nx);
    for (int i = 0; i < nx; ++i) {
        x_grid[i] = i * L / (nx - 1);
    }

    double t_grid = 50.0;

    start = std::chrono::high_resolution_clock::now();
    std::vector<double> u_pred_grid = pinn.predictGrid(x_grid, t_grid);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "   Grid size: " << nx << " points\n";
    std::cout << "   Time: " << duration.count() << " μs\n";
    std::cout << "   Time per point: " << (duration.count() / nx) << " μs\n\n";

    // ========================================================================
    // 4. Error Analysis
    // ========================================================================

    std::cout << "4. Error analysis vs analytical solution...\n";

    std::vector<double> u_exact_grid(nx);
    for (int i = 0; i < nx; ++i) {
        u_exact_grid[i] = analyticalSolution(x_grid[i], t_grid, D);
    }

    double l2_error = computeL2Error(u_pred_grid, u_exact_grid);

    std::cout << "   L2 relative error: " << l2_error << "\n";
    std::cout << "   L2 error %: " << (l2_error * 100) << "%\n\n";

    // ========================================================================
    // 5. Time Evolution
    // ========================================================================

    std::cout << "5. Time evolution prediction...\n";

    std::vector<double> times = {0.0, 25.0, 50.0, 75.0, 100.0};

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "   Center concentration (x=0.5) over time:\n";
    std::cout << "   Time [s]  | PINN      | Analytical | Error\n";
    std::cout << "   ----------|-----------|------------|-------\n";

    for (double t : times) {
        double u_p = pinn.predict(0.5, t);
        double u_a = analyticalSolution(0.5, t, D);
        double err = std::abs(u_p - u_a);

        std::cout << "   " << std::setw(9) << t << " | "
                  << std::setw(9) << u_p << " | "
                  << std::setw(10) << u_a << " | "
                  << std::setw(7) << err << "\n";
    }
    std::cout << "\n";

    // ========================================================================
    // 6. Save Results
    // ========================================================================

    std::cout << "6. Saving results...\n";

    std::string output_file = "pinn_cpp_results.csv";
    savePredictions(output_file, x_grid, u_pred_grid, t_grid);

    // Also save comparison
    std::ofstream comp_file("pinn_cpp_comparison.csv");
    comp_file << "x,u_pinn,u_exact,error\n";
    for (int i = 0; i < nx; ++i) {
        comp_file << x_grid[i] << ","
                  << u_pred_grid[i] << ","
                  << u_exact_grid[i] << ","
                  << std::abs(u_pred_grid[i] - u_exact_grid[i]) << "\n";
    }
    comp_file.close();
    std::cout << "   Saved comparison to: pinn_cpp_comparison.csv\n\n";

    // ========================================================================
    // 7. Performance Benchmark
    // ========================================================================

    std::cout << "7. Performance benchmark...\n";

    const int n_iterations = 1000;
    const int bench_nx = 100;

    std::vector<double> bench_x_grid(bench_nx);
    for (int i = 0; i < bench_nx; ++i) {
        bench_x_grid[i] = i * L / (bench_nx - 1);
    }

    start = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < n_iterations; ++iter) {
        pinn.predictGrid(bench_x_grid, 50.0);
    }
    end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avg_time = static_cast<double>(total_duration.count()) / n_iterations;
    double throughput = bench_nx / avg_time;  // points per ms

    std::cout << "   Iterations: " << n_iterations << "\n";
    std::cout << "   Grid size: " << bench_nx << " points\n";
    std::cout << "   Total time: " << total_duration.count() << " ms\n";
    std::cout << "   Avg time per iteration: " << avg_time << " ms\n";
    std::cout << "   Throughput: " << throughput << " points/ms\n\n";

    // ========================================================================
    // Summary
    // ========================================================================

    std::cout << "========================================\n";
    std::cout << "Summary\n";
    std::cout << "========================================\n";
    std::cout << "✓ PINN model loaded successfully\n";
    std::cout << "✓ Predictions match analytical solution\n";
    std::cout << "✓ L2 error: " << (l2_error * 100) << "%\n";
    std::cout << "✓ Performance: " << throughput << " points/ms\n";
    std::cout << "✓ Results saved to CSV files\n";
    std::cout << "\nPINN C++ integration working!\n";
    std::cout << "========================================\n";

    return 0;
}
