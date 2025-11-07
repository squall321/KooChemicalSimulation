/**
 * @file AdaptiveTimestepper.h
 * @brief Adaptive timestepping with automatic error control
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * Phase 66: Advanced Simulation Features
 *
 * Features:
 * - Automatic timestep adjustment based on error estimates
 * - Multiple error control strategies (absolute, relative, mixed)
 * - Step rejection and retry with smaller timestep
 * - Stability monitoring and control
 * - PI/PID controllers for smooth timestep evolution
 */

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace koo {
namespace simulation {
namespace timestepping {

/**
 * @brief Error control strategy
 */
enum class ErrorControl {
    Absolute,      ///< Absolute error tolerance
    Relative,      ///< Relative error tolerance
    Mixed,         ///< Mixed absolute + relative
    Scaled         ///< Scaled error (typical for ODE solvers)
};

/**
 * @brief Timestep controller type
 */
enum class ControllerType {
    Elementary,    ///< Simple scaling based on error
    PI,           ///< PI controller (proportional-integral)
    PID           ///< PID controller (proportional-integral-derivative)
};

/**
 * @brief Timestep adjustment result
 */
struct StepResult {
    bool accepted;         ///< Whether step was accepted
    double new_dt;         ///< Suggested new timestep
    double error_estimate; ///< Error estimate for this step
    double safety_factor;  ///< Safety factor applied
    int num_rejections;    ///< Number of rejections so far

    StepResult()
        : accepted(false), new_dt(0.0), error_estimate(0.0),
          safety_factor(1.0), num_rejections(0) {}
};

/**
 * @brief Adaptive timestepper configuration
 */
struct TimestepperConfig {
    // Tolerances
    double abs_tol;           ///< Absolute tolerance
    double rel_tol;           ///< Relative tolerance
    ErrorControl error_mode;  ///< Error control strategy

    // Timestep bounds
    double dt_min;            ///< Minimum allowed timestep
    double dt_max;            ///< Maximum allowed timestep
    double dt_initial;        ///< Initial timestep

    // Control parameters
    ControllerType controller; ///< Controller type
    double safety_factor;      ///< Safety factor (typically 0.8-0.95)
    double min_scale;          ///< Minimum scaling factor
    double max_scale;          ///< Maximum scaling factor

    // PI/PID controller parameters
    double pi_beta1;           ///< PI: proportional parameter
    double pi_beta2;           ///< PI: integral parameter
    double pid_beta3;          ///< PID: derivative parameter

    // Step rejection
    int max_rejections;        ///< Maximum consecutive rejections
    bool reject_on_divergence; ///< Reject if solution diverges

    // Order of accuracy
    int order;                 ///< Method order for error estimation

    TimestepperConfig()
        : abs_tol(1e-6), rel_tol(1e-4), error_mode(ErrorControl::Mixed),
          dt_min(1e-10), dt_max(1.0), dt_initial(1e-4),
          controller(ControllerType::PI), safety_factor(0.9),
          min_scale(0.2), max_scale(5.0),
          pi_beta1(0.7), pi_beta2(0.4), pid_beta3(0.0),
          max_rejections(10), reject_on_divergence(true),
          order(2) {}
};

/**
 * @brief Adaptive timestepper with error control
 */
class AdaptiveTimestepper {
public:
    /**
     * @brief Construct with configuration
     */
    explicit AdaptiveTimestepper(const TimestepperConfig& config = TimestepperConfig())
        : config_(config), current_dt_(config.dt_initial),
          num_steps_(0), num_accepted_(0), num_rejected_(0),
          consecutive_rejections_(0),
          error_prev_(0.0), error_prev2_(0.0) {}

    /**
     * @brief Evaluate step and adjust timestep
     *
     * @param solution Current solution vector
     * @param solution_new New solution from integrator
     * @param error_estimate Error estimate from integrator
     * @return Step result with acceptance decision and new dt
     */
    StepResult evaluateStep(const std::vector<double>& solution,
                           const std::vector<double>& solution_new,
                           double error_estimate) {
        StepResult result;
        result.error_estimate = error_estimate;

        // Compute scaled error
        double scaled_error = computeScaledError(solution, solution_new, error_estimate);

        // Check for divergence
        if (config_.reject_on_divergence && isDiverging(solution_new)) {
            result.accepted = false;
            result.new_dt = current_dt_ * config_.min_scale;
            consecutive_rejections_++;
            num_rejected_++;
            return result;
        }

        // Accept/reject decision
        result.accepted = (scaled_error <= 1.0);

        if (result.accepted) {
            // Accept step - compute new timestep
            result.new_dt = computeNewTimestep(scaled_error);
            result.safety_factor = config_.safety_factor;

            num_accepted_++;
            consecutive_rejections_ = 0;
            num_steps_++;

            // Update error history for PID controller
            error_prev2_ = error_prev_;
            error_prev_ = scaled_error;
        } else {
            // Reject step - reduce timestep
            result.new_dt = current_dt_ * config_.min_scale;
            consecutive_rejections_++;
            num_rejected_++;

            // Check maximum rejections
            if (consecutive_rejections_ >= config_.max_rejections) {
                throw std::runtime_error(
                    "Maximum consecutive rejections reached. "
                    "Consider reducing tolerances or checking solver stability.");
            }
        }

        result.num_rejections = consecutive_rejections_;

        // Apply bounds
        result.new_dt = std::max(config_.dt_min,
                                std::min(config_.dt_max, result.new_dt));

        current_dt_ = result.new_dt;

        return result;
    }

    /**
     * @brief Get current timestep
     */
    double getCurrentTimestep() const { return current_dt_; }

    /**
     * @brief Set timestep manually
     */
    void setTimestep(double dt) {
        current_dt_ = std::max(config_.dt_min, std::min(config_.dt_max, dt));
    }

    /**
     * @brief Get configuration
     */
    const TimestepperConfig& getConfig() const { return config_; }

    /**
     * @brief Update configuration
     */
    void setConfig(const TimestepperConfig& config) {
        config_ = config;
        current_dt_ = std::max(config_.dt_min,
                              std::min(config_.dt_max, current_dt_));
    }

    /**
     * @brief Get statistics
     */
    struct Statistics {
        size_t num_steps;
        size_t num_accepted;
        size_t num_rejected;
        double acceptance_rate;
    };

    Statistics getStatistics() const {
        Statistics stats;
        stats.num_steps = num_steps_;
        stats.num_accepted = num_accepted_;
        stats.num_rejected = num_rejected_;
        stats.acceptance_rate = num_steps_ > 0 ?
            static_cast<double>(num_accepted_) / static_cast<double>(num_accepted_ + num_rejected_) : 0.0;
        return stats;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        num_steps_ = 0;
        num_accepted_ = 0;
        num_rejected_ = 0;
        consecutive_rejections_ = 0;
    }

    /**
     * @brief Reset to initial state
     */
    void reset() {
        current_dt_ = config_.dt_initial;
        resetStatistics();
        error_prev_ = 0.0;
        error_prev2_ = 0.0;
    }

private:
    TimestepperConfig config_;
    double current_dt_;
    size_t num_steps_;
    size_t num_accepted_;
    size_t num_rejected_;
    int consecutive_rejections_;

    // Error history for PID controller
    double error_prev_;
    double error_prev2_;

    /**
     * @brief Compute scaled error based on error control mode
     */
    double computeScaledError(const std::vector<double>& solution,
                             const std::vector<double>& solution_new,
                             double error_estimate) const {
        if (solution.size() != solution_new.size()) {
            throw std::invalid_argument("Solution vectors must have same size");
        }

        double scaled_error = 0.0;
        size_t n = solution.size();

        switch (config_.error_mode) {
            case ErrorControl::Absolute:
                // Simple absolute error
                scaled_error = error_estimate / config_.abs_tol;
                break;

            case ErrorControl::Relative:
                // Relative error based on solution magnitude
                {
                    double sol_norm = 0.0;
                    for (size_t i = 0; i < n; ++i) {
                        sol_norm += solution_new[i] * solution_new[i];
                    }
                    sol_norm = std::sqrt(sol_norm / static_cast<double>(n));
                    scaled_error = error_estimate / (config_.rel_tol * std::max(sol_norm, 1.0));
                }
                break;

            case ErrorControl::Mixed:
                // Mixed absolute + relative
                {
                    double max_scaled = 0.0;
                    for (size_t i = 0; i < n; ++i) {
                        double scale = config_.abs_tol +
                                     config_.rel_tol * std::abs(solution_new[i]);
                        double component_error = std::abs(solution_new[i] - solution[i]);
                        max_scaled = std::max(max_scaled, component_error / scale);
                    }
                    scaled_error = max_scaled;
                }
                break;

            case ErrorControl::Scaled:
                // Scaled error (typical for ODE solvers)
                {
                    double sum_sq = 0.0;
                    for (size_t i = 0; i < n; ++i) {
                        double scale = config_.abs_tol +
                                     config_.rel_tol * std::max(std::abs(solution[i]),
                                                               std::abs(solution_new[i]));
                        double err_i = std::abs(solution_new[i] - solution[i]) / scale;
                        sum_sq += err_i * err_i;
                    }
                    scaled_error = std::sqrt(sum_sq / static_cast<double>(n));
                }
                break;
        }

        return scaled_error;
    }

    /**
     * @brief Compute new timestep based on error and controller type
     */
    double computeNewTimestep(double scaled_error) const {
        double scale_factor = 1.0;

        // Prevent division by zero
        scaled_error = std::max(scaled_error, 1e-10);

        switch (config_.controller) {
            case ControllerType::Elementary:
                // Simple controller: dt_new = safety * dt * (1/error)^(1/order)
                scale_factor = config_.safety_factor *
                              std::pow(1.0 / scaled_error, 1.0 / (config_.order + 1));
                break;

            case ControllerType::PI:
                // PI controller: combines current and previous error
                {
                    double exp_factor = 1.0 / (config_.order + 1);
                    double error_ratio = (error_prev_ > 0.0) ?
                                        scaled_error / error_prev_ : 1.0;

                    scale_factor = config_.safety_factor *
                                  std::pow(1.0 / scaled_error, config_.pi_beta1 * exp_factor) *
                                  std::pow(error_ratio, -config_.pi_beta2 * exp_factor);
                }
                break;

            case ControllerType::PID:
                // PID controller: includes second derivative term
                {
                    double exp_factor = 1.0 / (config_.order + 1);
                    double error_ratio1 = (error_prev_ > 0.0) ?
                                         scaled_error / error_prev_ : 1.0;
                    double error_ratio2 = (error_prev2_ > 0.0 && error_prev_ > 0.0) ?
                                         error_prev_ / error_prev2_ : 1.0;

                    scale_factor = config_.safety_factor *
                                  std::pow(1.0 / scaled_error, config_.pi_beta1 * exp_factor) *
                                  std::pow(error_ratio1, -config_.pi_beta2 * exp_factor) *
                                  std::pow(error_ratio2, config_.pid_beta3 * exp_factor);
                }
                break;
        }

        // Apply bounds on scaling
        scale_factor = std::max(config_.min_scale,
                               std::min(config_.max_scale, scale_factor));

        return current_dt_ * scale_factor;
    }

    /**
     * @brief Check if solution is diverging
     */
    bool isDiverging(const std::vector<double>& solution) const {
        const double divergence_threshold = 1e10;

        for (double val : solution) {
            if (!std::isfinite(val) || std::abs(val) > divergence_threshold) {
                return true;
            }
        }

        return false;
    }
};

/**
 * @brief Predefined configurations for common use cases
 */
namespace Configs {

/// High accuracy configuration
inline TimestepperConfig HighAccuracy() {
    TimestepperConfig config;
    config.abs_tol = 1e-8;
    config.rel_tol = 1e-6;
    config.safety_factor = 0.85;
    config.controller = ControllerType::PI;
    return config;
}

/// Balanced configuration (default)
inline TimestepperConfig Balanced() {
    return TimestepperConfig();  // Use defaults
}

/// Fast configuration (lower accuracy)
inline TimestepperConfig Fast() {
    TimestepperConfig config;
    config.abs_tol = 1e-4;
    config.rel_tol = 1e-3;
    config.safety_factor = 0.95;
    config.max_scale = 10.0;
    return config;
}

/// Stiff problems configuration
inline TimestepperConfig Stiff() {
    TimestepperConfig config;
    config.abs_tol = 1e-6;
    config.rel_tol = 1e-4;
    config.safety_factor = 0.8;
    config.min_scale = 0.1;
    config.max_scale = 2.0;
    config.controller = ControllerType::PID;
    return config;
}

}  // namespace Configs

}  // namespace timestepping
}  // namespace simulation
}  // namespace koo
