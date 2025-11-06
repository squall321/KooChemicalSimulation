/**
 * @file StabilityMonitor.h
 * @brief Stability monitoring and control for simulations
 * @author KooChemicalSimulation Development Team
 * @version 6.0.0-alpha4
 * Phase 66: Advanced Simulation Features
 *
 * Features:
 * - CFL condition monitoring
 * - von Neumann stability analysis
 * - Solution boundedness checks
 * - Oscillation detection
 * - Early warning system for instability
 */

#pragma once

#include <vector>
#include <cmath>
#include <deque>
#include <algorithm>
#include <limits>

namespace koo {
namespace simulation {
namespace stability {

/**
 * @brief Stability status
 */
enum class StabilityStatus {
    Stable,           ///< System is stable
    Warning,          ///< Potential instability detected
    Unstable,         ///< System is unstable
    Critical          ///< Critical instability (immediate action required)
};

/**
 * @brief Stability metrics
 */
struct StabilityMetrics {
    double cfl_number;           ///< Current CFL number
    double max_cfl_allowed;      ///< Maximum allowed CFL
    double solution_growth_rate; ///< Solution growth rate
    double oscillation_index;    ///< Oscillation indicator
    bool has_nan;                ///< Whether NaN detected
    bool has_inf;                ///< Whether Inf detected
    bool out_of_bounds;          ///< Whether solution is out of physical bounds

    StabilityStatus status;      ///< Overall stability status

    StabilityMetrics()
        : cfl_number(0.0), max_cfl_allowed(1.0),
          solution_growth_rate(0.0), oscillation_index(0.0),
          has_nan(false), has_inf(false), out_of_bounds(false),
          status(StabilityStatus::Stable) {}
};

/**
 * @brief Physical bounds for solution variables
 */
struct PhysicalBounds {
    double min_value;    ///< Minimum physical value
    double max_value;    ///< Maximum physical value

    PhysicalBounds(double min = -std::numeric_limits<double>::infinity(),
                  double max = std::numeric_limits<double>::infinity())
        : min_value(min), max_value(max) {}

    bool isValid(double value) const {
        return std::isfinite(value) && value >= min_value && value <= max_value;
    }
};

/**
 * @brief Stability monitor configuration
 */
struct MonitorConfig {
    // CFL monitoring
    double max_cfl;              ///< Maximum allowed CFL number
    bool enable_cfl_check;       ///< Enable CFL checking

    // Growth rate monitoring
    double max_growth_rate;      ///< Maximum allowed growth rate per step
    bool enable_growth_check;    ///< Enable growth rate checking

    // Oscillation detection
    int oscillation_window;      ///< Window size for oscillation detection
    double oscillation_threshold;///< Threshold for oscillation index
    bool enable_oscillation_check; ///< Enable oscillation checking

    // Physical bounds
    bool enable_bounds_check;    ///< Enable physical bounds checking

    MonitorConfig()
        : max_cfl(1.0), enable_cfl_check(true),
          max_growth_rate(2.0), enable_growth_check(true),
          oscillation_window(10), oscillation_threshold(0.5),
          enable_oscillation_check(true),
          enable_bounds_check(true) {}
};

/**
 * @brief Stability monitor
 */
class StabilityMonitor {
public:
    /**
     * @brief Construct with configuration
     */
    explicit StabilityMonitor(const MonitorConfig& config = MonitorConfig())
        : config_(config) {}

    /**
     * @brief Set physical bounds for variable
     */
    void setBounds(const std::string& var_name, const PhysicalBounds& bounds) {
        bounds_[var_name] = bounds;
    }

    /**
     * @brief Check stability of current solution
     *
     * @param solution Current solution vector
     * @param solution_prev Previous solution vector
     * @param dt Timestep size
     * @param dx Spatial grid spacing
     * @param max_velocity Maximum velocity in domain
     * @return Stability metrics
     */
    StabilityMetrics checkStability(const std::vector<double>& solution,
                                   const std::vector<double>& solution_prev,
                                   double dt,
                                   double dx = 1.0,
                                   double max_velocity = 0.0) {
        StabilityMetrics metrics;

        // Check for NaN/Inf
        metrics.has_nan = hasNaN(solution);
        metrics.has_inf = hasInf(solution);

        if (metrics.has_nan || metrics.has_inf) {
            metrics.status = StabilityStatus::Critical;
            return metrics;
        }

        // Check physical bounds
        if (config_.enable_bounds_check) {
            metrics.out_of_bounds = !checkBounds(solution);
            if (metrics.out_of_bounds) {
                metrics.status = StabilityStatus::Unstable;
            }
        }

        // Compute CFL number
        if (config_.enable_cfl_check && dx > 0.0 && dt > 0.0) {
            metrics.cfl_number = computeCFL(dt, dx, max_velocity);
            metrics.max_cfl_allowed = config_.max_cfl;

            if (metrics.cfl_number > config_.max_cfl) {
                metrics.status = StabilityStatus::Unstable;
            } else if (metrics.cfl_number > 0.8 * config_.max_cfl) {
                metrics.status = StabilityStatus::Warning;
            }
        }

        // Compute growth rate
        if (config_.enable_growth_check && !solution_prev.empty()) {
            metrics.solution_growth_rate = computeGrowthRate(solution, solution_prev);

            if (metrics.solution_growth_rate > config_.max_growth_rate) {
                metrics.status = StabilityStatus::Unstable;
            } else if (metrics.solution_growth_rate > 0.8 * config_.max_growth_rate) {
                if (metrics.status == StabilityStatus::Stable) {
                    metrics.status = StabilityStatus::Warning;
                }
            }
        }

        // Detect oscillations
        if (config_.enable_oscillation_check) {
            solution_history_.push_back(solution);
            if (solution_history_.size() > static_cast<size_t>(config_.oscillation_window)) {
                solution_history_.pop_front();
            }

            metrics.oscillation_index = detectOscillations();

            if (metrics.oscillation_index > config_.oscillation_threshold) {
                if (metrics.status == StabilityStatus::Stable) {
                    metrics.status = StabilityStatus::Warning;
                }
            }
        }

        return metrics;
    }

    /**
     * @brief Compute recommended timestep for stability
     *
     * @param dx Spatial grid spacing
     * @param diffusivity Diffusion coefficient
     * @param max_velocity Maximum velocity
     * @param safety_factor Safety factor (typically 0.5-0.9)
     * @return Recommended timestep
     */
    double computeStableTimestep(double dx,
                                double diffusivity,
                                double max_velocity = 0.0,
                                double safety_factor = 0.9) const {
        double dt_diffusion = std::numeric_limits<double>::max();
        double dt_advection = std::numeric_limits<double>::max();

        // Diffusion stability (explicit): dt <= dx^2 / (2*D)
        if (diffusivity > 0.0) {
            dt_diffusion = (dx * dx) / (2.0 * diffusivity);
        }

        // Advection stability (CFL): dt <= dx / |v|
        if (max_velocity > 0.0) {
            dt_advection = dx / max_velocity;
        }

        // Take minimum and apply safety factor
        double dt_stable = std::min(dt_diffusion, dt_advection) * safety_factor;

        return dt_stable;
    }

    /**
     * @brief Get configuration
     */
    const MonitorConfig& getConfig() const { return config_; }

    /**
     * @brief Update configuration
     */
    void setConfig(const MonitorConfig& config) { config_ = config; }

    /**
     * @brief Reset monitor state
     */
    void reset() {
        solution_history_.clear();
    }

private:
    MonitorConfig config_;
    std::map<std::string, PhysicalBounds> bounds_;
    std::deque<std::vector<double>> solution_history_;

    /**
     * @brief Check for NaN values
     */
    bool hasNaN(const std::vector<double>& solution) const {
        return std::any_of(solution.begin(), solution.end(),
                          [](double val) { return std::isnan(val); });
    }

    /**
     * @brief Check for Inf values
     */
    bool hasInf(const std::vector<double>& solution) const {
        return std::any_of(solution.begin(), solution.end(),
                          [](double val) { return std::isinf(val); });
    }

    /**
     * @brief Check if solution is within physical bounds
     */
    bool checkBounds(const std::vector<double>& solution) const {
        if (bounds_.empty()) return true;

        // Check default bounds if set
        auto default_bounds = bounds_.find("default");
        if (default_bounds != bounds_.end()) {
            for (double val : solution) {
                if (!default_bounds->second.isValid(val)) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief Compute CFL number
     */
    double computeCFL(double dt, double dx, double max_velocity) const {
        if (dx <= 0.0 || dt <= 0.0) return 0.0;
        return (max_velocity * dt) / dx;
    }

    /**
     * @brief Compute solution growth rate
     */
    double computeGrowthRate(const std::vector<double>& solution,
                            const std::vector<double>& solution_prev) const {
        if (solution.size() != solution_prev.size()) return 0.0;

        double norm_current = 0.0;
        double norm_prev = 0.0;

        for (size_t i = 0; i < solution.size(); ++i) {
            norm_current += solution[i] * solution[i];
            norm_prev += solution_prev[i] * solution_prev[i];
        }

        norm_current = std::sqrt(norm_current);
        norm_prev = std::sqrt(norm_prev);

        if (norm_prev < 1e-12) return 0.0;

        return norm_current / norm_prev;
    }

    /**
     * @brief Detect oscillations in solution history
     */
    double detectOscillations() const {
        if (solution_history_.size() < 3) return 0.0;

        // Count sign changes in derivative
        size_t n_vars = solution_history_[0].size();
        int total_sign_changes = 0;
        int max_possible_changes = (solution_history_.size() - 2) * n_vars;

        for (size_t i = 0; i < n_vars; ++i) {
            int sign_changes = 0;
            for (size_t t = 1; t < solution_history_.size() - 1; ++t) {
                double deriv_prev = solution_history_[t][i] - solution_history_[t-1][i];
                double deriv_next = solution_history_[t+1][i] - solution_history_[t][i];

                if (deriv_prev * deriv_next < 0) {
                    sign_changes++;
                }
            }
            total_sign_changes += sign_changes;
        }

        // Oscillation index: fraction of sign changes
        return static_cast<double>(total_sign_changes) / max_possible_changes;
    }
};

/**
 * @brief Predefined monitor configurations
 */
namespace MonitorConfigs {

/// Strict monitoring (conservative)
inline MonitorConfig Strict() {
    MonitorConfig config;
    config.max_cfl = 0.5;
    config.max_growth_rate = 1.5;
    config.oscillation_threshold = 0.3;
    return config;
}

/// Standard monitoring (default)
inline MonitorConfig Standard() {
    return MonitorConfig();
}

/// Relaxed monitoring (permissive)
inline MonitorConfig Relaxed() {
    MonitorConfig config;
    config.max_cfl = 2.0;
    config.max_growth_rate = 5.0;
    config.oscillation_threshold = 0.7;
    return config;
}

}  // namespace MonitorConfigs

}  // namespace stability
}  // namespace simulation
}  // namespace koo
