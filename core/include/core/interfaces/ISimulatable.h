/**
 * @file ISimulatable.h
 * @brief Interface for objects that can be simulated
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha2
 * @date 2025-11-06
 *
 * This file defines the ISimulatable interface, which represents any object
 * that can participate in a simulation. This is a core abstraction that enables
 * polymorphic treatment of various simulation components.
 */

#ifndef KOO_CORE_INTERFACES_ISIMULATABLE_H
#define KOO_CORE_INTERFACES_ISIMULATABLE_H

#include <memory>
#include <string>

namespace koo {
namespace core {

/**
 * @brief Interface for simulatable objects
 *
 * ISimulatable defines the contract for objects that can be simulated within
 * the chemical simulation framework. Any object that needs to evolve over time
 * or participate in a simulation should implement this interface.
 *
 * Key responsibilities:
 * - Initialization of simulation state
 * - Time advancement and state updates
 * - Validation of simulation state
 * - Finalization and cleanup
 *
 * Design Pattern: Interface Segregation Principle (ISP)
 *
 * @note This is a pure virtual interface - all methods must be implemented
 *       by concrete classes.
 *
 * Example usage:
 * @code
 * class MySimulation : public ISimulatable {
 * public:
 *     void initialize() override { }
 *     void step(double dt) override { }
 *     bool validate() const override { return true; }
 *     void finalize() override { }
 *     std::string getName() const override { return "MySimulation"; }
 * };
 * @endcode
 */
class ISimulatable {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~ISimulatable() = default;

    /**
     * @brief Initialize the simulatable object
     *
     * This method is called once before the simulation starts. It should
     * set up initial conditions, allocate resources, and prepare the object
     * for time stepping.
     *
     * @throws std::runtime_error if initialization fails
     */
    virtual void initialize() = 0;

    /**
     * @brief Advance the simulation by one time step
     *
     * This method advances the simulation state by a given time increment.
     * The implementation should update all relevant internal state variables.
     *
     * @param dt Time step size (must be positive)
     * @throws std::invalid_argument if dt <= 0
     * @throws std::runtime_error if step fails
     */
    virtual void step(double dt) = 0;

    /**
     * @brief Validate the current simulation state
     *
     * Checks whether the current state is physically valid and numerically
     * stable. This can be used for error checking during simulation.
     *
     * @return true if state is valid, false otherwise
     */
    virtual bool validate() const = 0;

    /**
     * @brief Finalize the simulation
     *
     * This method is called once after the simulation completes. It should
     * perform cleanup operations, release resources, and write final output.
     */
    virtual void finalize() = 0;

    /**
     * @brief Get the name of this simulatable object
     *
     * Returns a human-readable name that identifies this object.
     * Useful for logging and debugging.
     *
     * @return Object name as string
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Check if the simulation has converged
     *
     * For steady-state simulations, this method checks if the solution
     * has reached a converged state.
     *
     * @return true if converged, false otherwise
     */
    virtual bool hasConverged() const {
        // Default implementation: always false (transient simulation)
        return false;
    }

    /**
     * @brief Get current simulation time
     *
     * @return Current time value
     */
    virtual double getCurrentTime() const = 0;

    /**
     * @brief Reset the simulation to initial state
     *
     * Resets all internal state to the initial conditions. This allows
     * rerunning the simulation without recreating the object.
     */
    virtual void reset() = 0;

protected:
    /**
     * @brief Protected default constructor
     *
     * Prevents direct instantiation of interface. Only derived classes
     * can be constructed.
     */
    ISimulatable() = default;

    /**
     * @brief Protected copy constructor (prevent slicing)
     */
    ISimulatable(const ISimulatable&) = default;

    /**
     * @brief Protected copy assignment (prevent slicing)
     */
    ISimulatable& operator=(const ISimulatable&) = default;

    /**
     * @brief Protected move constructor
     */
    ISimulatable(ISimulatable&&) = default;

    /**
     * @brief Protected move assignment
     */
    ISimulatable& operator=(ISimulatable&&) = default;
};

/**
 * @brief Shared pointer type for ISimulatable
 *
 * Convenience type alias for managing ISimulatable objects with shared ownership.
 */
using ISimulatablePtr = std::shared_ptr<ISimulatable>;

/**
 * @brief Unique pointer type for ISimulatable
 *
 * Convenience type alias for managing ISimulatable objects with unique ownership.
 */
using ISimulatableUniquePtr = std::unique_ptr<ISimulatable>;

} // namespace core
} // namespace koo

#endif // KOO_CORE_INTERFACES_ISIMULATABLE_H
