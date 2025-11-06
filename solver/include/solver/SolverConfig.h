/**
 * @file SolverConfig.h
 * @brief High-level solver configuration system
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha4
 * @date 2025-11-06
 *
 * Provides unified configuration for solver creation, options, and strategies.
 */

#ifndef KOO_SOLVER_SOLVER_CONFIG_H
#define KOO_SOLVER_SOLVER_CONFIG_H

#include "SolverFactory.h"
#include "SolverStrategy.h"
#include "pde/SolverOptions.h"
#include "pde/SolverTypes.h"

#include <memory>
#include <string>
#include <map>

namespace koo {
namespace solver {

/**
 * @brief Strategy type enumeration
 */
enum class StrategyType {
    BASIC,          ///< Basic solving strategy
    TIME_STEPPING,  ///< Time-dependent strategy
    ADAPTIVE        ///< Adaptive strategy with retries
};

/**
 * @brief Convert strategy type to string
 */
inline std::string toString(StrategyType type) {
    switch (type) {
        case StrategyType::BASIC:         return "Basic";
        case StrategyType::TIME_STEPPING: return "TimeStepping";
        case StrategyType::ADAPTIVE:      return "Adaptive";
        default:                          return "Unknown";
    }
}

/**
 * @brief Complete solver configuration
 *
 * Combines solver selection, options, and strategy into single configuration
 */
class SolverConfig {
public:
    SolverConfig()
        : backend_(pde::SolverBackend::CUSTOM),
          solverName_("MockSolver"),
          strategyType_(StrategyType::BASIC),
          startTime_(0.0),
          endTime_(1.0),
          timeStep_(0.01),
          maxRetries_(3) {}

    // === Solver Selection ===

    /**
     * @brief Set solver backend
     */
    void setBackend(pde::SolverBackend backend) {
        backend_ = backend;
        solverName_.clear();  // Clear name when using backend
    }

    /**
     * @brief Get solver backend
     */
    pde::SolverBackend getBackend() const {
        return backend_;
    }

    /**
     * @brief Set solver by name
     */
    void setSolverName(const std::string& name) {
        solverName_ = name;
    }

    /**
     * @brief Get solver name
     */
    const std::string& getSolverName() const {
        return solverName_;
    }

    // === Solver Options ===

    /**
     * @brief Set solver options
     */
    void setOptions(const pde::SolverOptions& options) {
        options_ = options;
    }

    /**
     * @brief Get solver options
     */
    const pde::SolverOptions& getOptions() const {
        return options_;
    }

    /**
     * @brief Get mutable solver options
     */
    pde::SolverOptions& getOptions() {
        return options_;
    }

    // === Strategy Selection ===

    /**
     * @brief Set strategy type
     */
    void setStrategy(StrategyType type) {
        strategyType_ = type;
    }

    /**
     * @brief Get strategy type
     */
    StrategyType getStrategy() const {
        return strategyType_;
    }

    // === Time-Stepping Parameters ===

    /**
     * @brief Set time range for time-stepping
     */
    void setTimeRange(double start, double end, double step) {
        startTime_ = start;
        endTime_ = end;
        timeStep_ = step;
    }

    /**
     * @brief Get start time
     */
    double getStartTime() const { return startTime_; }

    /**
     * @brief Get end time
     */
    double getEndTime() const { return endTime_; }

    /**
     * @brief Get time step
     */
    double getTimeStep() const { return timeStep_; }

    // === Adaptive Parameters ===

    /**
     * @brief Set maximum retries for adaptive strategy
     */
    void setMaxRetries(int retries) {
        maxRetries_ = retries;
    }

    /**
     * @brief Get maximum retries
     */
    int getMaxRetries() const {
        return maxRetries_;
    }

    // === Factory Methods ===

    /**
     * @brief Create solver from configuration
     */
    std::shared_ptr<pde::ISolver> createSolver() const {
        auto& factory = SolverFactory::getInstance();

        std::shared_ptr<pde::ISolver> solver;

        if (!solverName_.empty()) {
            solver = factory.createSolver(solverName_);
        } else {
            solver = factory.createSolver(backend_);
        }

        if (solver) {
            solver->setOptions(options_);
        }

        return solver;
    }

    /**
     * @brief Create strategy from configuration
     */
    std::shared_ptr<ISolverStrategy> createStrategy() const {
        switch (strategyType_) {
            case StrategyType::BASIC:
                return StrategyManager::createBasicStrategy();

            case StrategyType::TIME_STEPPING:
                return StrategyManager::createTimeSteppingStrategy(
                    startTime_, endTime_, timeStep_
                );

            case StrategyType::ADAPTIVE: {
                auto strategy = StrategyManager::createAdaptiveStrategy();
                strategy->setMaxRetries(maxRetries_);
                return strategy;
            }

            default:
                return StrategyManager::createBasicStrategy();
        }
    }

    // === Preset Configurations ===

    /**
     * @brief Create configuration for fast solving
     */
    static SolverConfig createFastConfig() {
        SolverConfig config;
        config.setBackend(pde::SolverBackend::CUSTOM);
        config.setOptions(pde::presets::getFast());
        config.setStrategy(StrategyType::BASIC);
        return config;
    }

    /**
     * @brief Create configuration for accurate solving
     */
    static SolverConfig createAccurateConfig() {
        SolverConfig config;
        config.setBackend(pde::SolverBackend::CUSTOM);
        config.setOptions(pde::presets::getAccurate());
        config.setStrategy(StrategyType::BASIC);
        return config;
    }

    /**
     * @brief Create configuration for time-dependent problems
     */
    static SolverConfig createTimeDependentConfig(double tStart, double tEnd, double dt) {
        SolverConfig config;
        config.setBackend(pde::SolverBackend::CUSTOM);
        config.setOptions(pde::presets::getTimeDependent(dt, tEnd));
        config.setStrategy(StrategyType::TIME_STEPPING);
        config.setTimeRange(tStart, tEnd, dt);
        return config;
    }

    /**
     * @brief Create configuration for robust solving
     */
    static SolverConfig createRobustConfig() {
        SolverConfig config;
        config.setBackend(pde::SolverBackend::CUSTOM);
        config.setOptions(pde::presets::getAccurate());
        config.setStrategy(StrategyType::ADAPTIVE);
        config.setMaxRetries(5);
        return config;
    }

    // === Information ===

    /**
     * @brief Get configuration summary
     */
    std::string getSummary() const {
        std::string summary = "Solver Configuration:\n";

        if (!solverName_.empty()) {
            summary += "  Solver: " + solverName_ + "\n";
        } else {
            summary += "  Backend: " + pde::toString(backend_) + "\n";
        }

        summary += "  Strategy: " + toString(strategyType_) + "\n";

        summary += "  Options:\n";
        summary += "    Solver: " + pde::toString(options_.getLinearSolver()) + "\n";
        summary += "    Max Iterations: " + std::to_string(options_.getMaxIterations()) + "\n";
        summary += "    Tolerance: " + std::to_string(options_.getTolerance()) + "\n";

        if (strategyType_ == StrategyType::TIME_STEPPING) {
            summary += "  Time Range: [" + std::to_string(startTime_) + ", " +
                      std::to_string(endTime_) + "] dt=" + std::to_string(timeStep_) + "\n";
        }

        if (strategyType_ == StrategyType::ADAPTIVE) {
            summary += "  Max Retries: " + std::to_string(maxRetries_) + "\n";
        }

        return summary;
    }

private:
    pde::SolverBackend backend_;
    std::string solverName_;
    pde::SolverOptions options_;
    StrategyType strategyType_;

    // Time-stepping parameters
    double startTime_;
    double endTime_;
    double timeStep_;

    // Adaptive parameters
    int maxRetries_;
};

/**
 * @brief Solver builder for fluent configuration
 */
class SolverBuilder {
public:
    SolverBuilder() = default;

    /**
     * @brief Set solver backend
     */
    SolverBuilder& withBackend(pde::SolverBackend backend) {
        config_.setBackend(backend);
        return *this;
    }

    /**
     * @brief Set solver name
     */
    SolverBuilder& withName(const std::string& name) {
        config_.setSolverName(name);
        return *this;
    }

    /**
     * @brief Set solver options
     */
    SolverBuilder& withOptions(const pde::SolverOptions& options) {
        config_.setOptions(options);
        return *this;
    }

    /**
     * @brief Set strategy
     */
    SolverBuilder& withStrategy(StrategyType strategy) {
        config_.setStrategy(strategy);
        return *this;
    }

    /**
     * @brief Set time range
     */
    SolverBuilder& withTimeRange(double start, double end, double step) {
        config_.setTimeRange(start, end, step);
        return *this;
    }

    /**
     * @brief Set max retries
     */
    SolverBuilder& withMaxRetries(int retries) {
        config_.setMaxRetries(retries);
        return *this;
    }

    /**
     * @brief Build solver configuration
     */
    SolverConfig build() const {
        return config_;
    }

    /**
     * @brief Build and create solver
     */
    std::shared_ptr<pde::ISolver> buildSolver() const {
        return config_.createSolver();
    }

    /**
     * @brief Build and create strategy
     */
    std::shared_ptr<ISolverStrategy> buildStrategy() const {
        return config_.createStrategy();
    }

private:
    SolverConfig config_;
};

} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_SOLVER_CONFIG_H
