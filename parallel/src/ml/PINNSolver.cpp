/**
 * @file PINNSolver.cpp
 * @brief Implementation of C++ PINN solver interface
 */

#include "parallel/ml/PINNSolver.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>

namespace koo {
namespace parallel {
namespace ml {

// ============================================================================
// PINNSolver Implementation
// ============================================================================

PINNSolver::PINNSolver(const std::string& device)
    : device_(device == "cuda" && torch::cuda::is_available()
              ? torch::kCUDA
              : torch::kCPU),
      modelLoaded_(false) {

    if (device == "cuda" && !torch::cuda::is_available()) {
        std::cerr << "Warning: CUDA requested but not available. Using CPU.\n";
    }
}

void PINNSolver::loadModel(const std::string& model_path) {
    try {
        // Load the TorchScript model
        model_ = torch::jit::load(model_path);
        model_.to(device_);
        model_.eval();  // Set to evaluation mode

        modelLoaded_ = true;

        std::cout << "Successfully loaded PINN model from: " << model_path << "\n";
        std::cout << "Device: " << (device_.is_cuda() ? "CUDA" : "CPU") << "\n";

    } catch (const c10::Error& e) {
        throw std::runtime_error("Failed to load PINN model: " + std::string(e.what()));
    }
}

void PINNSolver::toGPU() {
    if (!torch::cuda::is_available()) {
        std::cerr << "Warning: CUDA not available. Staying on CPU.\n";
        return;
    }

    device_ = torch::kCUDA;
    if (modelLoaded_) {
        model_.to(device_);
    }
}

void PINNSolver::toCPU() {
    device_ = torch::kCPU;
    if (modelLoaded_) {
        model_.to(device_);
    }
}

std::vector<double> PINNSolver::predict(const std::vector<double>& x, double t) {
    if (!modelLoaded_) {
        throw std::runtime_error("Model not loaded. Call loadModel() first.");
    }

    // Create input tensor [1 x (dim + 1)]
    std::vector<double> input = x;
    input.push_back(t);

    torch::Tensor input_tensor = vectorToTensor(input).unsqueeze(0);
    input_tensor = input_tensor.to(device_);

    // Forward pass
    torch::NoGradGuard no_grad;  // Disable gradient computation
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input_tensor);

    torch::Tensor output = model_.forward(inputs).toTensor();
    output = output.to(torch::kCPU);

    return tensorToVector(output.squeeze(0));
}

std::vector<std::vector<double>> PINNSolver::predictBatch(
    const std::vector<std::vector<double>>& x_batch,
    const std::vector<double>& t_batch) {

    if (!modelLoaded_) {
        throw std::runtime_error("Model not loaded. Call loadModel() first.");
    }

    if (x_batch.size() != t_batch.size()) {
        throw std::invalid_argument("x_batch and t_batch must have same size");
    }

    int n = x_batch.size();
    int spatial_dim = x_batch[0].size();

    // Build input tensor [N x (spatial_dim + 1)]
    std::vector<float> input_data;
    input_data.reserve(n * (spatial_dim + 1));

    for (int i = 0; i < n; ++i) {
        for (double val : x_batch[i]) {
            input_data.push_back(static_cast<float>(val));
        }
        input_data.push_back(static_cast<float>(t_batch[i]));
    }

    torch::Tensor input_tensor = torch::from_blob(
        input_data.data(),
        {n, spatial_dim + 1},
        torch::kFloat32).clone();
    input_tensor = input_tensor.to(device_);

    // Forward pass
    torch::NoGradGuard no_grad;
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input_tensor);

    torch::Tensor output = model_.forward(inputs).toTensor();
    output = output.to(torch::kCPU);

    // Convert to vector
    auto output_acc = output.accessor<float, 2>();
    int output_dim = output.size(1);

    std::vector<std::vector<double>> result(n, std::vector<double>(output_dim));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < output_dim; ++j) {
            result[i][j] = static_cast<double>(output_acc[i][j]);
        }
    }

    return result;
}

torch::Tensor PINNSolver::forward(torch::Tensor inputs) {
    if (!modelLoaded_) {
        throw std::runtime_error("Model not loaded. Call loadModel() first.");
    }

    inputs = inputs.to(device_);

    torch::NoGradGuard no_grad;
    std::vector<torch::jit::IValue> model_inputs;
    model_inputs.push_back(inputs);

    return model_.forward(model_inputs).toTensor();
}

torch::Tensor PINNSolver::vectorToTensor(const std::vector<double>& vec) {
    std::vector<float> vec_float(vec.begin(), vec.end());
    return torch::from_blob(vec_float.data(), {static_cast<long>(vec.size())}, torch::kFloat32).clone();
}

std::vector<double> PINNSolver::tensorToVector(torch::Tensor tensor) {
    tensor = tensor.to(torch::kCPU).contiguous();
    auto accessor = tensor.accessor<float, 1>();

    std::vector<double> result(accessor.size(0));
    for (int i = 0; i < accessor.size(0); ++i) {
        result[i] = static_cast<double>(accessor[i]);
    }
    return result;
}

// ============================================================================
// DiffusionPINN1D Implementation
// ============================================================================

double DiffusionPINN1D::predict(double x, double t) {
    std::vector<double> result = PINNSolver::predict({x}, t);
    return result[0];
}

std::vector<double> DiffusionPINN1D::predictGrid(
    const std::vector<double>& x_grid, double t) {

    int n = x_grid.size();
    std::vector<std::vector<double>> x_batch(n);
    std::vector<double> t_batch(n, t);

    for (int i = 0; i < n; ++i) {
        x_batch[i] = {x_grid[i]};
    }

    auto result_batch = PINNSolver::predictBatch(x_batch, t_batch);

    std::vector<double> result(n);
    for (int i = 0; i < n; ++i) {
        result[i] = result_batch[i][0];
    }

    return result;
}

// ============================================================================
// DiffusionPINN2D Implementation
// ============================================================================

double DiffusionPINN2D::predict(double x, double y, double t) {
    std::vector<double> result = PINNSolver::predict({x, y}, t);
    return result[0];
}

std::vector<std::vector<double>> DiffusionPINN2D::predictGrid(
    const std::vector<double>& x_grid,
    const std::vector<double>& y_grid,
    double t) {

    int nx = x_grid.size();
    int ny = y_grid.size();
    int n = nx * ny;

    std::vector<std::vector<double>> xy_batch(n);
    std::vector<double> t_batch(n, t);

    int idx = 0;
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            xy_batch[idx++] = {x_grid[i], y_grid[j]};
        }
    }

    auto result_batch = PINNSolver::predictBatch(xy_batch, t_batch);

    std::vector<std::vector<double>> result(ny, std::vector<double>(nx));
    idx = 0;
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            result[j][i] = result_batch[idx++][0];
        }
    }

    return result;
}

// ============================================================================
// ReactionDiffusionPINN Implementation
// ============================================================================

std::vector<double> ReactionDiffusionPINN::predict(double x, double t) {
    return PINNSolver::predict({x}, t);
}

std::pair<std::vector<double>, std::vector<double>>
ReactionDiffusionPINN::predictGrid(const std::vector<double>& x_grid, double t) {

    int n = x_grid.size();
    std::vector<std::vector<double>> x_batch(n);
    std::vector<double> t_batch(n, t);

    for (int i = 0; i < n; ++i) {
        x_batch[i] = {x_grid[i]};
    }

    auto result_batch = PINNSolver::predictBatch(x_batch, t_batch);

    std::vector<double> u_grid(n), v_grid(n);
    for (int i = 0; i < n; ++i) {
        u_grid[i] = result_batch[i][0];
        v_grid[i] = result_batch[i][1];
    }

    return {u_grid, v_grid};
}

// ============================================================================
// HybridSolver Implementation
// ============================================================================

HybridSolver::HybridSolver(
    std::shared_ptr<PINNSolver> pinn_solver,
    PDESolver pde_solver)
    : pinnSolver_(pinn_solver), pdeSolver_(pde_solver) {}

std::vector<double> HybridSolver::solve(
    const std::vector<double>& x_grid,
    double t_final,
    double coarse_dt,
    double fine_dt) {

    std::cout << "\n=== Hybrid Solver: PINN + PDE ===\n";
    std::cout << "Stage 1: Coarse approximation with PINN...\n";

    // Stage 1: Fast coarse approximation with PINN
    auto coarse_solution = coarseApproximation(x_grid, t_final);

    std::cout << "Stage 2: Fine refinement with PDE solver...\n";

    // Stage 2: Refine with traditional PDE solver
    // (Start from t=0, use PINN as guide or initial condition)
    auto final_solution = fineRefinement(
        coarse_solution, x_grid, 0.0, t_final, fine_dt);

    std::cout << "Hybrid solve complete!\n";

    return final_solution;
}

std::vector<double> HybridSolver::coarseApproximation(
    const std::vector<double>& x_grid,
    double t_target) {

    // Use PINN for quick prediction
    // Assuming pinnSolver_ is DiffusionPINN1D or similar
    int n = x_grid.size();
    std::vector<std::vector<double>> x_batch(n);
    std::vector<double> t_batch(n, t_target);

    for (int i = 0; i < n; ++i) {
        x_batch[i] = {x_grid[i]};
    }

    auto result_batch = pinnSolver_->predictBatch(x_batch, t_batch);

    std::vector<double> result(n);
    for (int i = 0; i < n; ++i) {
        result[i] = result_batch[i][0];
    }

    return result;
}

std::vector<double> HybridSolver::fineRefinement(
    const std::vector<double>& initial_condition,
    const std::vector<double>& x_grid,
    double t_start,
    double t_end,
    double dt) {

    // Use traditional PDE solver for accurate refinement
    // This is a placeholder - actual implementation depends on PDE solver interface

    std::vector<double> solution = initial_condition;
    double t = t_start;
    int steps = static_cast<int>((t_end - t_start) / dt);

    for (int step = 0; step < steps; ++step) {
        solution = pdeSolver_(solution, dt);
        t += dt;
    }

    return solution;
}

std::vector<double> HybridSolver::multiscaleSolve(
    const std::vector<double>& x_grid,
    double t_final) {

    // Multiscale: Use PINN for macroscale behavior
    // and PDE for microscale corrections

    // Get macroscale solution from PINN
    auto macro_solution = coarseApproximation(x_grid, t_final);

    // Apply microscale corrections with PDE solver
    // (This is simplified - real implementation would be more complex)
    auto final_solution = fineRefinement(
        macro_solution, x_grid, t_final - 1.0, t_final, 0.01);

    return final_solution;
}

// ============================================================================
// Utility Functions
// ============================================================================

void savePredictions(
    const std::string& filename,
    const std::vector<double>& x,
    const std::vector<double>& u,
    double t) {

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    file << "# PINN predictions at t = " << t << "\n";
    file << "# x, u\n";

    for (size_t i = 0; i < x.size(); ++i) {
        file << x[i] << ", " << u[i] << "\n";
    }

    file.close();
    std::cout << "Saved predictions to: " << filename << "\n";
}

double computeL2Error(
    const std::vector<double>& u_pinn,
    const std::vector<double>& u_exact) {

    if (u_pinn.size() != u_exact.size()) {
        throw std::invalid_argument("Vectors must have same size");
    }

    double error_sq = 0.0;
    double norm_sq = 0.0;

    for (size_t i = 0; i < u_pinn.size(); ++i) {
        double diff = u_pinn[i] - u_exact[i];
        error_sq += diff * diff;
        norm_sq += u_exact[i] * u_exact[i];
    }

    return std::sqrt(error_sq / norm_sq);
}

} // namespace ml
} // namespace parallel
} // namespace koo
