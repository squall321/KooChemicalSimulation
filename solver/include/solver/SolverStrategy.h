/**
 * @file SolverStrategy.h
 * @brief Strategy pattern for PDE solver execution
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha4
 * @date 2025-11-06
 *
 * Provides strategy pattern for customizing solver behavior including
 * preprocessing, solving, and postprocessing steps.
 */

#ifndef KOO_SOLVER_SOLVER_STRATEGY_H
#define KOO_SOLVER_SOLVER_STRATEGY_H

#include "pde/ISolver.h"
#include "pde/SolverTypes.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace koo {
namespace solver {

/**
 * @brief Hook function types for solver workflow
 */
using PreprocessHook = std::function<bool(pde::ISolver&)>;
using PostprocessHook = std::function<bool(pde::ISolver&, const pde::ConvergenceInfo&)>;
using IterationHook = std::function<bool(pde::ISolver&, int iteration, double residual)>;

/**
 * @brief Strategy for executing PDE solvers
 *
 * Defines the workflow for solving a PDE problem with customizable
 * preprocessing, solving, and postprocessing steps.
 */
class ISolverStrategy {
public:
    virtual ~ISolverStrategy() = default;

    /**
     * @brief Execute the solving strategy
     * @param solver Solver to execute
     * @return Convergence information
     */
    virtual pde::ConvergenceInfo execute(std::shared_ptr<pde::ISolver> solver) = 0;

    /**
     * @brief Get strategy name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get strategy description
     */
    virtual std::string getDescription() const = 0;
};

/**
 * @brief Basic solving strategy
 *
 * Implements standard workflow:
 * 1. Preprocessing hooks
 * 2. Assembly
 * 3. Apply boundary conditions
 * 4. Solve
 * 5. Postprocessing hooks
 */
class BasicStrategy : public ISolverStrategy {
public:
    BasicStrategy() = default;
    ~BasicStrategy() override = default;

    pde::ConvergenceInfo execute(std::shared_ptr<pde::ISolver> solver) override {
        if (!solver) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "Null solver";
            return info;
        }

        // Run preprocessing hooks
        if (!runPreprocessHooks(solver)) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "Preprocessing failed";
            return info;
        }

        // Assembly
        if (!solver->assemble()) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "Assembly failed: " + solver->getLastError();
            return info;
        }

        // Apply boundary conditions
        if (!solver->applyBoundaryConditions()) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "BC application failed: " + solver->getLastError();
            return info;
        }

        // Solve
        pde::ConvergenceInfo info = solver->solve();

        // Run postprocessing hooks
        if (!runPostprocessHooks(solver, info)) {
            info.message += " (postprocessing failed)";
        }

        return info;
    }

    std::string getName() const override {
        return "BasicStrategy";
    }

    std::string getDescription() const override {
        return "Standard workflow: preprocess -> assemble -> apply BCs -> solve -> postprocess";
    }

    /**
     * @brief Add preprocessing hook
     */
    void addPreprocessHook(PreprocessHook hook) {
        preprocessHooks_.push_back(hook);
    }

    /**
     * @brief Add postprocessing hook
     */
    void addPostprocessHook(PostprocessHook hook) {
        postprocessHooks_.push_back(hook);
    }

    /**
     * @brief Clear all hooks
     */
    void clearHooks() {
        preprocessHooks_.clear();
        postprocessHooks_.clear();
    }

    /**
     * @brief Get number of preprocessing hooks
     */
    size_t getNumPreprocessHooks() const {
        return preprocessHooks_.size();
    }

    /**
     * @brief Get number of postprocessing hooks
     */
    size_t getNumPostprocessHooks() const {
        return postprocessHooks_.size();
    }

protected:
    bool runPreprocessHooks(std::shared_ptr<pde::ISolver> solver) {
        for (const auto& hook : preprocessHooks_) {
            if (!hook(*solver)) {
                return false;
            }
        }
        return true;
    }

    bool runPostprocessHooks(std::shared_ptr<pde::ISolver> solver,
                             const pde::ConvergenceInfo& info) {
        for (const auto& hook : postprocessHooks_) {
            if (!hook(*solver, info)) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<PreprocessHook> preprocessHooks_;
    std::vector<PostprocessHook> postprocessHooks_;
};

/**
 * @brief Time-stepping strategy
 *
 * Implements time-dependent solving with multiple time steps
 */
class TimeSteppingStrategy : public BasicStrategy {
public:
    TimeSteppingStrategy(double startTime, double endTime, double timeStep)
        : startTime_(startTime),
          endTime_(endTime),
          timeStep_(timeStep),
          currentTime_(startTime) {}

    ~TimeSteppingStrategy() override = default;

    pde::ConvergenceInfo execute(std::shared_ptr<pde::ISolver> solver) override {
        if (!solver) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "Null solver";
            return info;
        }

        pde::ConvergenceInfo finalInfo;
        currentTime_ = startTime_;
        int timeStep = 0;

        while (currentTime_ < endTime_) {
            timeStep++;

            // Run preprocessing for this time step
            if (!runPreprocessHooks(solver)) {
                finalInfo.status = pde::SolverStatus::ERROR;
                finalInfo.message = "Preprocessing failed at time step " + std::to_string(timeStep);
                return finalInfo;
            }

            // Assemble for current time
            if (!solver->assemble()) {
                finalInfo.status = pde::SolverStatus::ERROR;
                finalInfo.message = "Assembly failed at time step " + std::to_string(timeStep);
                return finalInfo;
            }

            // Apply boundary conditions
            if (!solver->applyBoundaryConditions()) {
                finalInfo.status = pde::SolverStatus::ERROR;
                finalInfo.message = "BC application failed at time step " + std::to_string(timeStep);
                return finalInfo;
            }

            // Solve for this time step
            pde::ConvergenceInfo stepInfo = solver->solve();

            if (stepInfo.status == pde::SolverStatus::ERROR) {
                finalInfo = stepInfo;
                finalInfo.message += " at time step " + std::to_string(timeStep);
                return finalInfo;
            }

            // Run postprocessing
            if (!runPostprocessHooks(solver, stepInfo)) {
                stepInfo.message += " (postprocessing failed)";
            }

            // Accumulate information
            finalInfo.iterations += stepInfo.iterations;
            finalInfo.timeElapsed += stepInfo.timeElapsed;
            finalInfo = stepInfo;  // Keep last step info

            currentTime_ += timeStep_;
        }

        finalInfo.message = "Completed " + std::to_string(timeStep) + " time steps";
        return finalInfo;
    }

    std::string getName() const override {
        return "TimeSteppingStrategy";
    }

    std::string getDescription() const override {
        return "Time-dependent solving with multiple time steps from " +
               std::to_string(startTime_) + " to " + std::to_string(endTime_) +
               " with dt=" + std::to_string(timeStep_);
    }

    /**
     * @brief Get current time
     */
    double getCurrentTime() const {
        return currentTime_;
    }

    /**
     * @brief Get time step size
     */
    double getTimeStep() const {
        return timeStep_;
    }

    /**
     * @brief Set time step size
     */
    void setTimeStep(double dt) {
        timeStep_ = dt;
    }

private:
    double startTime_;
    double endTime_;
    double timeStep_;
    double currentTime_;
};

/**
 * @brief Adaptive strategy with error estimation
 *
 * Implements adaptive solving with convergence monitoring and retry logic
 */
class AdaptiveStrategy : public BasicStrategy {
public:
    AdaptiveStrategy()
        : maxRetries_(3),
          toleranceRelaxation_(10.0),
          retryCount_(0) {}

    ~AdaptiveStrategy() override = default;

    pde::ConvergenceInfo execute(std::shared_ptr<pde::ISolver> solver) override {
        if (!solver) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "Null solver";
            return info;
        }

        retryCount_ = 0;
        pde::ConvergenceInfo info;

        for (int attempt = 0; attempt <= maxRetries_; ++attempt) {
            // Adjust solver options for retry
            if (attempt > 0) {
                adjustSolverOptions(solver, attempt);
            }

            // Run preprocessing
            if (!runPreprocessHooks(solver)) {
                info.status = pde::SolverStatus::ERROR;
                info.message = "Preprocessing failed";
                return info;
            }

            // Assembly
            if (!solver->assemble()) {
                info.status = pde::SolverStatus::ERROR;
                info.message = "Assembly failed";
                return info;
            }

            // Apply BCs
            if (!solver->applyBoundaryConditions()) {
                info.status = pde::SolverStatus::ERROR;
                info.message = "BC application failed";
                return info;
            }

            // Solve
            info = solver->solve();

            // Check convergence
            if (info.converged()) {
                // Run postprocessing
                runPostprocessHooks(solver, info);

                if (attempt > 0) {
                    info.message += " (converged after " + std::to_string(attempt) + " retries)";
                }
                return info;
            }

            retryCount_++;
        }

        info.message = "Failed to converge after " + std::to_string(maxRetries_) + " retries";
        return info;
    }

    std::string getName() const override {
        return "AdaptiveStrategy";
    }

    std::string getDescription() const override {
        return "Adaptive solving with retry logic (max " + std::to_string(maxRetries_) + " retries)";
    }

    /**
     * @brief Set maximum number of retries
     */
    void setMaxRetries(int maxRetries) {
        maxRetries_ = maxRetries;
    }

    /**
     * @brief Get retry count from last execution
     */
    int getRetryCount() const {
        return retryCount_;
    }

private:
    void adjustSolverOptions(std::shared_ptr<pde::ISolver> solver, int attempt) {
        auto options = solver->getOptions();

        // Relax tolerance
        double newTol = options.getTolerance() * toleranceRelaxation_;
        options.setTolerance(newTol);

        // Increase max iterations
        int newMaxIter = options.getMaxIterations() * 2;
        options.setMaxIterations(newMaxIter);

        solver->setOptions(options);
    }

    int maxRetries_;
    double toleranceRelaxation_;
    int retryCount_;
};

/**
 * @brief Strategy manager for executing solvers with different strategies
 */
class StrategyManager {
public:
    StrategyManager() = default;

    /**
     * @brief Execute solver with given strategy
     */
    pde::ConvergenceInfo execute(std::shared_ptr<pde::ISolver> solver,
                                  std::shared_ptr<ISolverStrategy> strategy) {
        if (!solver || !strategy) {
            pde::ConvergenceInfo info;
            info.status = pde::SolverStatus::ERROR;
            info.message = "Null solver or strategy";
            return info;
        }

        return strategy->execute(solver);
    }

    /**
     * @brief Create basic strategy
     */
    static std::shared_ptr<BasicStrategy> createBasicStrategy() {
        return std::make_shared<BasicStrategy>();
    }

    /**
     * @brief Create time-stepping strategy
     */
    static std::shared_ptr<TimeSteppingStrategy> createTimeSteppingStrategy(
        double startTime, double endTime, double timeStep) {
        return std::make_shared<TimeSteppingStrategy>(startTime, endTime, timeStep);
    }

    /**
     * @brief Create adaptive strategy
     */
    static std::shared_ptr<AdaptiveStrategy> createAdaptiveStrategy() {
        return std::make_shared<AdaptiveStrategy>();
    }
};

} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_SOLVER_STRATEGY_H
