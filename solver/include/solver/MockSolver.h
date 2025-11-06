/**
 * @file MockSolver.h
 * @brief Mock PDE solver for testing
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha2
 * @date 2025-11-06
 *
 * Provides a simple mock solver implementation for testing and demonstration.
 */

#ifndef KOO_SOLVER_MOCK_SOLVER_H
#define KOO_SOLVER_MOCK_SOLVER_H

#include "pde/ISolver.h"
#include "pde/SolverTypes.h"
#include "pde/SolverOptions.h"
#include "pde/LinearSystem.h"
#include <chrono>

namespace koo {
namespace solver {

/**
 * @brief Mock solver implementation for testing
 *
 * This solver provides a simple implementation of the ISolver interface
 * without requiring any external solver libraries. Useful for testing
 * and demonstration purposes.
 */
class MockSolver : public pde::BaseSolver {
public:
    MockSolver()
        : pde::BaseSolver(),
          assembled_(false),
          bcsApplied_(false) {
        pdeType_ = pde::PDEType::ELLIPTIC;
    }

    ~MockSolver() override = default;

    // === ISolver Interface Implementation ===

    pde::SolverBackend getBackend() const override {
        return pde::SolverBackend::CUSTOM;
    }

    std::string getName() const override {
        return "MockSolver";
    }

    std::string getVersion() const override {
        return "1.0.0-mock";
    }

    bool initialize(std::shared_ptr<mesh::MeshManager> meshManager) override {
        if (!meshManager || !meshManager->isLoaded()) {
            lastError_ = "Invalid or unloaded mesh";
            return false;
        }

        meshManager_ = meshManager;

        // Get mesh size
        auto mesh = meshManager->getMesh();
        size_t numNodes = mesh->getNumNodes();

        if (numNodes == 0) {
            lastError_ = "Mesh has no nodes";
            return false;
        }

        // Initialize linear system
        linearSystem_.resize(numNodes);
        solution_.resize(numNodes);
        solution_.zero();

        initialized_ = true;
        status_ = pde::SolverStatus::INITIALIZED;

        return true;
    }

    bool assemble() override {
        if (!initialized_) {
            lastError_ = "Solver not initialized";
            return false;
        }

        // Build a simple Laplacian matrix for demonstration
        // This is a mock implementation - real solvers would do proper assembly

        size_t n = linearSystem_.size();
        auto& A = linearSystem_.getMatrix();
        auto& b = linearSystem_.getRHS();

        // Clear previous assembly
        A = pde::SparseMatrix(n, n);
        A.reserve(n * 5);  // Estimate 5 non-zeros per row

        // Simple 1D Laplacian: -u''(x) = f(x)
        // Discretized as: (-u_{i-1} + 2*u_i - u_{i+1}) / h^2 = f_i

        double h = 1.0 / (n + 1);  // Grid spacing
        double h2 = h * h;

        for (size_t i = 0; i < n; ++i) {
            // Diagonal entry
            A.addEntry(i, i, 2.0 / h2);

            // Off-diagonal entries
            if (i > 0) {
                A.addEntry(i, i-1, -1.0 / h2);
            }
            if (i < n - 1) {
                A.addEntry(i, i+1, -1.0 / h2);
            }

            // RHS: f(x) = 1 (constant forcing)
            b[i] = 1.0;
        }

        A.finalize();
        assembled_ = true;

        return true;
    }

    bool applyBoundaryConditions() override {
        if (!assembled_) {
            lastError_ = "System not assembled";
            return false;
        }

        // Apply BCs from mesh manager
        auto bcManager = meshManager_->getBCManager();
        auto dirichletBCs = bcManager->getDirichletBCs();

        for (const auto& bc : dirichletBCs) {
            if (!bc->isEnabled()) {
                continue;
            }

            // For mock solver, apply BC at boundary nodes
            // In real implementation, would map physical tags to DOFs
            int tag = bc->getPhysicalTag();

            // Mock: Apply BC at first and last nodes
            if (tag == 0 || dirichletBCs.size() == 1) {
                linearSystem_.applyDirichletBC(0, bc->evaluate(0, 0, 0, 0));
                linearSystem_.applyDirichletBC(linearSystem_.size() - 1,
                                              bc->evaluate(1, 0, 0, 0));
            }
        }

        bcsApplied_ = true;
        return true;
    }

    pde::ConvergenceInfo solve() override {
        auto startTime = std::chrono::high_resolution_clock::now();

        pde::ConvergenceInfo info;
        info.status = pde::SolverStatus::SOLVING;

        if (!assembled_) {
            lastError_ = "System not assembled";
            info.status = pde::SolverStatus::ERROR;
            info.message = lastError_;
            lastConvergenceInfo_ = info;
            status_ = pde::SolverStatus::ERROR;
            return info;
        }

        // Simple iterative solver (Jacobi method)
        const size_t maxIter = options_.getMaxIterations();
        const double tol = options_.getTolerance();

        auto& A = linearSystem_.getMatrix();
        auto& b = linearSystem_.getRHS();
        auto& x = linearSystem_.getSolution();

        pde::SolutionVector xOld = x;

        for (size_t iter = 0; iter < maxIter; ++iter) {
            // Jacobi iteration: x_new[i] = (b[i] - sum(A[i,j]*x[j], j!=i)) / A[i,i]
            for (size_t i = 0; i < linearSystem_.size(); ++i) {
                double sum = 0.0;
                double diag = 0.0;

                // Get row i from sparse matrix
                const auto& rowPtr = A.getRowPtr();
                const auto& colIdx = A.getColIndices();
                const auto& values = A.getValues();

                for (size_t j = rowPtr[i]; j < rowPtr[i + 1]; ++j) {
                    if (colIdx[j] == i) {
                        diag = values[j];
                    } else {
                        sum += values[j] * xOld[colIdx[j]];
                    }
                }

                if (std::abs(diag) > 1e-14) {
                    x[i] = (b[i] - sum) / diag;
                } else {
                    x[i] = xOld[i];
                }
            }

            // Check convergence
            double error = 0.0;
            for (size_t i = 0; i < linearSystem_.size(); ++i) {
                double diff = x[i] - xOld[i];
                error += diff * diff;
            }
            error = std::sqrt(error);

            info.iterations = iter + 1;
            info.residual = error;
            info.relativeError = error / (x.norm() + 1e-14);

            if (error < tol) {
                info.status = pde::SolverStatus::CONVERGED;
                break;
            }

            xOld = x;
        }

        if (info.status != pde::SolverStatus::CONVERGED) {
            if (info.iterations >= maxIter) {
                info.status = pde::SolverStatus::MAX_ITERATIONS;
                info.message = "Maximum iterations reached";
            }
        } else {
            info.message = "Converged successfully";
        }

        // Copy solution
        solution_ = x;

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        info.timeElapsed = elapsed.count();
        info.solutionNorm = solution_.norm();

        lastConvergenceInfo_ = info;
        status_ = info.status;

        return info;
    }

    double getSolutionAt(double x, double y, double z) const override {
        // Simple interpolation for 1D case
        if (solution_.size() == 0) {
            return 0.0;
        }

        // Map x coordinate to node index
        size_t idx = static_cast<size_t>(x * (solution_.size() - 1));
        if (idx >= solution_.size()) {
            idx = solution_.size() - 1;
        }

        return solution_[idx];
    }

    bool exportSolution(const std::string& filename,
                       const std::string& format) const override {
        // Mock export - just write solution values to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << "# MockSolver Solution Export\n";
        file << "# Format: " << format << "\n";
        file << "# Size: " << solution_.size() << "\n";
        file << "# x value\n";

        for (size_t i = 0; i < solution_.size(); ++i) {
            double x = static_cast<double>(i) / (solution_.size() - 1);
            file << x << " " << solution_[i] << "\n";
        }

        file.close();
        return true;
    }

    // === Additional Methods ===

    /**
     * @brief Check if system is assembled
     */
    bool isAssembled() const {
        return assembled_;
    }

    /**
     * @brief Check if BCs are applied
     */
    bool areBCsApplied() const {
        return bcsApplied_;
    }

    /**
     * @brief Get number of DOFs
     */
    size_t getNumDOFs() const {
        return linearSystem_.size();
    }

private:
    bool assembled_;
    bool bcsApplied_;
};

} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_MOCK_SOLVER_H
