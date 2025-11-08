/**
 * @file PINNSolver.h
 * @brief C++ interface for Physics-Informed Neural Networks
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha5
 * @date 2025-11-08
 *
 * Priority D2: PINN Integration - C++ Interface
 *
 * This header provides C++ wrappers for loading and using PINN models
 * trained in Python. Uses LibTorch (PyTorch C++ API) for inference.
 */

#ifndef KOO_PARALLEL_ML_PINN_SOLVER_H
#define KOO_PARALLEL_ML_PINN_SOLVER_H

#include <torch/torch.h>
#include <torch/script.h>
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace koo {
namespace parallel {
namespace ml {

// ============================================================================
// PINN Solver Base Class
// ============================================================================

/**
 * @brief Base class for PINN solvers in C++
 *
 * Loads PyTorch models trained in Python and provides inference capabilities.
 * Supports both CPU and GPU execution.
 */
class PINNSolver {
public:
    /**
     * @brief Constructor
     * @param device Device type ("cpu" or "cuda")
     */
    explicit PINNSolver(const std::string& device = "cpu");

    /**
     * @brief Virtual destructor
     */
    virtual ~PINNSolver() = default;

    /**
     * @brief Load trained PINN model from file
     * @param model_path Path to TorchScript model (.pt file)
     *
     * Model should be saved in Python using:
     *   model_scripted = torch.jit.script(model)
     *   model_scripted.save('model.pt')
     */
    void loadModel(const std::string& model_path);

    /**
     * @brief Check if model is loaded
     */
    bool isLoaded() const { return modelLoaded_; }

    /**
     * @brief Get device type
     */
    torch::Device getDevice() const { return device_; }

    /**
     * @brief Move model to GPU if available
     */
    void toGPU();

    /**
     * @brief Move model to CPU
     */
    void toCPU();

    /**
     * @brief Predict at single point
     * @param x Spatial coordinate(s)
     * @param t Time coordinate
     * @return Predicted value(s)
     */
    std::vector<double> predict(const std::vector<double>& x, double t);

    /**
     * @brief Predict at multiple points (batch)
     * @param x_batch Spatial coordinates [N x dim]
     * @param t_batch Time coordinates [N]
     * @return Predicted values [N x output_dim]
     */
    std::vector<std::vector<double>> predictBatch(
        const std::vector<std::vector<double>>& x_batch,
        const std::vector<double>& t_batch);

    /**
     * @brief Raw tensor prediction (advanced)
     * @param inputs Input tensor [N x (spatial_dim + 1)]
     * @return Output tensor [N x output_dim]
     */
    torch::Tensor forward(torch::Tensor inputs);

protected:
    torch::jit::script::Module model_;      ///< Loaded TorchScript model
    torch::Device device_;                   ///< Computation device
    bool modelLoaded_;                       ///< Model load status

    // Helper functions
    torch::Tensor vectorToTensor(const std::vector<double>& vec);
    std::vector<double> tensorToVector(torch::Tensor tensor);
};

// ============================================================================
// Diffusion PINN Solver (1D)
// ============================================================================

/**
 * @brief 1D Diffusion PINN solver
 *
 * PDE: ∂u/∂t = D ∂²u/∂x²
 */
class DiffusionPINN1D : public PINNSolver {
public:
    explicit DiffusionPINN1D(const std::string& device = "cpu")
        : PINNSolver(device) {}

    /**
     * @brief Predict solution u(x, t)
     * @param x Spatial coordinate [m]
     * @param t Time [s]
     * @return Concentration u
     */
    double predict(double x, double t);

    /**
     * @brief Predict on spatial grid at time t
     * @param x_grid Spatial grid points
     * @param t Time
     * @return Concentrations at grid points
     */
    std::vector<double> predictGrid(const std::vector<double>& x_grid, double t);
};

// ============================================================================
// Diffusion PINN Solver (2D)
// ============================================================================

/**
 * @brief 2D Diffusion PINN solver
 *
 * PDE: ∂u/∂t = D (∂²u/∂x² + ∂²u/∂y²)
 */
class DiffusionPINN2D : public PINNSolver {
public:
    explicit DiffusionPINN2D(const std::string& device = "cpu")
        : PINNSolver(device) {}

    /**
     * @brief Predict solution u(x, y, t)
     * @param x X coordinate [m]
     * @param y Y coordinate [m]
     * @param t Time [s]
     * @return Concentration u
     */
    double predict(double x, double y, double t);

    /**
     * @brief Predict on 2D grid at time t
     * @param x_grid X coordinates
     * @param y_grid Y coordinates
     * @param t Time
     * @return Concentrations [ny x nx]
     */
    std::vector<std::vector<double>> predictGrid(
        const std::vector<double>& x_grid,
        const std::vector<double>& y_grid,
        double t);
};

// ============================================================================
// Reaction-Diffusion PINN Solver
// ============================================================================

/**
 * @brief Reaction-Diffusion PINN solver
 *
 * PDEs:
 *   ∂u/∂t = D_u ∂²u/∂x² + f(u,v)
 *   ∂v/∂t = D_v ∂²v/∂x² + g(u,v)
 */
class ReactionDiffusionPINN : public PINNSolver {
public:
    explicit ReactionDiffusionPINN(const std::string& device = "cpu")
        : PINNSolver(device) {}

    /**
     * @brief Predict both species (u, v) at (x, t)
     * @param x Spatial coordinate
     * @param t Time
     * @return {u, v}
     */
    std::vector<double> predict(double x, double t);

    /**
     * @brief Predict on grid
     * @param x_grid Spatial grid
     * @param t Time
     * @return {u_grid, v_grid}
     */
    std::pair<std::vector<double>, std::vector<double>> predictGrid(
        const std::vector<double>& x_grid, double t);
};

// ============================================================================
// Hybrid Solver: Traditional PDE + PINN
// ============================================================================

/**
 * @brief Hybrid solver combining traditional PDE solver with PINN
 *
 * Strategy:
 * 1. Use PINN for coarse initial approximation (fast)
 * 2. Refine with traditional PDE solver (accurate)
 * 3. Or: PINN for macroscale, PDE for microscale
 */
class HybridSolver {
public:
    using PDESolver = std::function<std::vector<double>(const std::vector<double>&, double)>;

    /**
     * @brief Constructor
     * @param pinn_solver PINN solver
     * @param pde_solver Traditional PDE solver
     */
    HybridSolver(std::shared_ptr<PINNSolver> pinn_solver, PDESolver pde_solver);

    /**
     * @brief Solve using two-stage approach
     * @param x_grid Spatial grid
     * @param t_final Final time
     * @param coarse_dt Time step for PINN coarse solve
     * @param fine_dt Time step for PDE fine solve
     * @return Final solution
     */
    std::vector<double> solve(
        const std::vector<double>& x_grid,
        double t_final,
        double coarse_dt,
        double fine_dt);

    /**
     * @brief Stage 1: Coarse approximation with PINN
     * @param x_grid Spatial grid
     * @param t_target Target time
     * @return PINN approximation
     */
    std::vector<double> coarseApproximation(
        const std::vector<double>& x_grid,
        double t_target);

    /**
     * @brief Stage 2: Fine refinement with PDE solver
     * @param initial_condition From PINN
     * @param x_grid Spatial grid
     * @param t_start Start time
     * @param t_end End time
     * @param dt Time step
     * @return Refined solution
     */
    std::vector<double> fineRefinement(
        const std::vector<double>& initial_condition,
        const std::vector<double>& x_grid,
        double t_start,
        double t_end,
        double dt);

    /**
     * @brief Multiscale solve: PINN for macro, PDE for micro
     */
    std::vector<double> multiscaleSolve(
        const std::vector<double>& x_grid,
        double t_final);

private:
    std::shared_ptr<PINNSolver> pinnSolver_;
    PDESolver pdeSolver_;
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Save model predictions to file
 */
void savePredictions(
    const std::string& filename,
    const std::vector<double>& x,
    const std::vector<double>& u,
    double t);

/**
 * @brief Compare PINN vs analytical solution
 * @return L2 relative error
 */
double computeL2Error(
    const std::vector<double>& u_pinn,
    const std::vector<double>& u_exact);

} // namespace ml
} // namespace parallel
} // namespace koo

#endif // KOO_PARALLEL_ML_PINN_SOLVER_H
