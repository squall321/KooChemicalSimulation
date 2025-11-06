/**
 * @file KineticsIntegrator.h
 * @brief Time integration for chemical kinetics
 * @author KooChemicalSimulation Development Team
 * @version 0.4.0-alpha3
 * @date 2025-11-06
 *
 * Provides ODE solvers for chemical kinetics systems.
 */

#ifndef KOO_CHEMISTRY_KINETICS_INTEGRATOR_H
#define KOO_CHEMISTRY_KINETICS_INTEGRATOR_H

#include "ChemicalSystem.h"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace koo {
namespace chemistry {

/**
 * @brief Integration method for ODE solver
 */
enum class IntegrationMethod {
    EXPLICIT_EULER,  ///< Forward Euler (1st order)
    RK4,            ///< Runge-Kutta 4th order
    RK2             ///< Runge-Kutta 2nd order (midpoint)
};

/**
 * @brief Time integration for chemical kinetics
 *
 * Solves the ODE system:
 * d[C_i]/dt = sum_j(nu_ij * ROP_j)
 *
 * where C_i are species concentrations and ROP_j are reaction rates of progress.
 */
class KineticsIntegrator {
public:
    /**
     * @brief Constructor with integration method
     */
    KineticsIntegrator(IntegrationMethod method = IntegrationMethod::RK4)
        : method_(method), dt_(1.0e-6), time_(0.0) {}

    /**
     * @brief Set integration method
     */
    void setMethod(IntegrationMethod method) {
        method_ = method;
    }

    /**
     * @brief Get integration method
     */
    IntegrationMethod getMethod() const {
        return method_;
    }

    /**
     * @brief Set time step
     * @param dt Time step (s)
     */
    void setTimeStep(double dt) {
        if (dt <= 0.0) {
            throw std::invalid_argument("Time step must be positive");
        }
        dt_ = dt;
    }

    /**
     * @brief Get time step
     */
    double getTimeStep() const {
        return dt_;
    }

    /**
     * @brief Get current time
     */
    double getTime() const {
        return time_;
    }

    /**
     * @brief Reset time to zero
     */
    void resetTime() {
        time_ = 0.0;
    }

    /**
     * @brief Advance system by one time step
     * @param system Chemical system to integrate
     */
    void advance(ChemicalSystem& system) {
        switch (method_) {
            case IntegrationMethod::EXPLICIT_EULER:
                advanceEuler(system);
                break;
            case IntegrationMethod::RK2:
                advanceRK2(system);
                break;
            case IntegrationMethod::RK4:
                advanceRK4(system);
                break;
        }
        time_ += dt_;
    }

    /**
     * @brief Integrate system from t0 to tf
     * @param system Chemical system to integrate
     * @param t0 Initial time (s)
     * @param tf Final time (s)
     * @param saveInterval Interval for saving states (0 = only save final state)
     * @return Vector of saved states with times
     */
    std::vector<std::pair<double, std::vector<double>>> integrate(
        ChemicalSystem& system, double t0, double tf, double saveInterval = 0.0) {

        std::vector<std::pair<double, std::vector<double>>> trajectory;

        time_ = t0;

        // Save initial state
        trajectory.push_back({time_, system.getState()});

        double nextSaveTime = (saveInterval > 0.0) ? (t0 + saveInterval) : tf;

        while (time_ < tf) {
            // Adjust last time step to hit tf exactly
            if (time_ + dt_ > tf) {
                dt_ = tf - time_;
            }

            advance(system);

            // Save state if needed
            if (saveInterval > 0.0 && time_ >= nextSaveTime) {
                trajectory.push_back({time_, system.getState()});
                nextSaveTime += saveInterval;
            }
        }

        // Save final state if not already saved
        if (trajectory.back().first < time_) {
            trajectory.push_back({time_, system.getState()});
        }

        return trajectory;
    }

    /**
     * @brief Get integration method name
     */
    std::string getMethodName() const {
        switch (method_) {
            case IntegrationMethod::EXPLICIT_EULER: return "Explicit Euler";
            case IntegrationMethod::RK2: return "RK2 (Midpoint)";
            case IntegrationMethod::RK4: return "RK4";
            default: return "Unknown";
        }
    }

private:
    /**
     * @brief Explicit Euler integration step
     */
    void advanceEuler(ChemicalSystem& system) {
        auto state = system.getState();
        auto rates = system.getProductionRates();

        // y(t+dt) = y(t) + dt * f(y,t)
        for (size_t i = 0; i < state.size(); ++i) {
            state[i] += dt_ * rates[i];
            // Ensure non-negative concentrations
            if (state[i] < 0.0) state[i] = 0.0;
        }

        system.setState(state);
    }

    /**
     * @brief RK2 (midpoint) integration step
     */
    void advanceRK2(ChemicalSystem& system) {
        auto y0 = system.getState();

        // k1 = f(t, y)
        auto k1 = system.getProductionRates();

        // Calculate midpoint: y_mid = y + 0.5*dt*k1
        std::vector<double> y_mid = y0;
        for (size_t i = 0; i < y0.size(); ++i) {
            y_mid[i] += 0.5 * dt_ * k1[i];
            if (y_mid[i] < 0.0) y_mid[i] = 0.0;
        }
        system.setState(y_mid);

        // k2 = f(t + 0.5*dt, y_mid)
        auto k2 = system.getProductionRates();

        // y(t+dt) = y(t) + dt * k2
        std::vector<double> y_new = y0;
        for (size_t i = 0; i < y0.size(); ++i) {
            y_new[i] += dt_ * k2[i];
            if (y_new[i] < 0.0) y_new[i] = 0.0;
        }

        system.setState(y_new);
    }

    /**
     * @brief RK4 integration step
     */
    void advanceRK4(ChemicalSystem& system) {
        auto y0 = system.getState();

        // k1 = f(t, y)
        auto k1 = system.getProductionRates();

        // k2 = f(t + 0.5*dt, y + 0.5*dt*k1)
        std::vector<double> y_temp = y0;
        for (size_t i = 0; i < y0.size(); ++i) {
            y_temp[i] += 0.5 * dt_ * k1[i];
            if (y_temp[i] < 0.0) y_temp[i] = 0.0;
        }
        system.setState(y_temp);
        auto k2 = system.getProductionRates();

        // k3 = f(t + 0.5*dt, y + 0.5*dt*k2)
        y_temp = y0;
        for (size_t i = 0; i < y0.size(); ++i) {
            y_temp[i] += 0.5 * dt_ * k2[i];
            if (y_temp[i] < 0.0) y_temp[i] = 0.0;
        }
        system.setState(y_temp);
        auto k3 = system.getProductionRates();

        // k4 = f(t + dt, y + dt*k3)
        y_temp = y0;
        for (size_t i = 0; i < y0.size(); ++i) {
            y_temp[i] += dt_ * k3[i];
            if (y_temp[i] < 0.0) y_temp[i] = 0.0;
        }
        system.setState(y_temp);
        auto k4 = system.getProductionRates();

        // y(t+dt) = y(t) + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
        std::vector<double> y_new = y0;
        for (size_t i = 0; i < y0.size(); ++i) {
            y_new[i] += (dt_ / 6.0) * (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]);
            if (y_new[i] < 0.0) y_new[i] = 0.0;
        }

        system.setState(y_new);
    }

    IntegrationMethod method_;  ///< Integration method
    double dt_;                 ///< Time step (s)
    double time_;               ///< Current time (s)
};

} // namespace chemistry
} // namespace koo

#endif // KOO_CHEMISTRY_KINETICS_INTEGRATOR_H
