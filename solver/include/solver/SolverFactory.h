/**
 * @file SolverFactory.h
 * @brief Factory for creating PDE solvers
 * @author KooChemicalSimulation Development Team
 * @version 0.3.0-alpha3
 * @date 2025-11-06
 *
 * Provides a factory pattern for creating different types of PDE solvers.
 * Supports registration of solver creators and configuration-based instantiation.
 */

#ifndef KOO_SOLVER_SOLVER_FACTORY_H
#define KOO_SOLVER_SOLVER_FACTORY_H

#include "pde/ISolver.h"
#include "pde/SolverTypes.h"
#include "MockSolver.h"

#include <memory>
#include <map>
#include <functional>
#include <stdexcept>
#include <string>

namespace koo {
namespace solver {

/**
 * @brief Creator function type for solvers
 */
using SolverCreator = std::function<std::shared_ptr<pde::ISolver>()>;

/**
 * @brief Factory for creating PDE solvers
 *
 * Implements the factory pattern for solver creation. Supports:
 * - Backend-based creation (CUSTOM, NGSOLVE, MFEM, etc.)
 * - Name-based creation for custom solvers
 * - Registration of new solver types
 * - Configuration-based instantiation
 *
 * Usage:
 * @code
 *   auto factory = SolverFactory::getInstance();
 *   auto solver = factory.createSolver(SolverBackend::CUSTOM);
 *   // or
 *   auto solver = factory.createSolver("MockSolver");
 * @endcode
 */
class SolverFactory {
public:
    /**
     * @brief Get singleton instance
     */
    static SolverFactory& getInstance() {
        static SolverFactory instance;
        return instance;
    }

    /**
     * @brief Register a solver creator
     * @param backend Backend type
     * @param creator Creator function
     * @throws std::runtime_error if backend already registered
     */
    void registerSolver(pde::SolverBackend backend, SolverCreator creator) {
        if (backendCreators_.find(backend) != backendCreators_.end()) {
            throw std::runtime_error(
                "Solver backend " + pde::toString(backend) + " already registered"
            );
        }
        backendCreators_[backend] = creator;
    }

    /**
     * @brief Register a solver creator by name
     * @param name Solver name
     * @param creator Creator function
     * @throws std::runtime_error if name already registered
     */
    void registerSolver(const std::string& name, SolverCreator creator) {
        if (nameCreators_.find(name) != nameCreators_.end()) {
            throw std::runtime_error(
                "Solver '" + name + "' already registered"
            );
        }
        nameCreators_[name] = creator;
    }

    /**
     * @brief Unregister a solver backend
     * @param backend Backend type
     * @return true if unregistered, false if not found
     */
    bool unregisterSolver(pde::SolverBackend backend) {
        auto it = backendCreators_.find(backend);
        if (it != backendCreators_.end()) {
            backendCreators_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Unregister a solver by name
     * @param name Solver name
     * @return true if unregistered, false if not found
     */
    bool unregisterSolver(const std::string& name) {
        auto it = nameCreators_.find(name);
        if (it != nameCreators_.end()) {
            nameCreators_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Create a solver by backend type
     * @param backend Backend type
     * @return Solver instance
     * @throws std::runtime_error if backend not registered or creation fails
     */
    std::shared_ptr<pde::ISolver> createSolver(pde::SolverBackend backend) const {
        auto it = backendCreators_.find(backend);
        if (it == backendCreators_.end()) {
            throw std::runtime_error(
                "Solver backend " + pde::toString(backend) + " not registered"
            );
        }

        try {
            auto solver = it->second();
            if (!solver) {
                throw std::runtime_error("Creator returned null solver");
            }
            return solver;
        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Failed to create solver for backend " + pde::toString(backend) +
                ": " + e.what()
            );
        }
    }

    /**
     * @brief Create a solver by name
     * @param name Solver name
     * @return Solver instance
     * @throws std::runtime_error if name not registered or creation fails
     */
    std::shared_ptr<pde::ISolver> createSolver(const std::string& name) const {
        auto it = nameCreators_.find(name);
        if (it == nameCreators_.end()) {
            throw std::runtime_error(
                "Solver '" + name + "' not registered"
            );
        }

        try {
            auto solver = it->second();
            if (!solver) {
                throw std::runtime_error("Creator returned null solver");
            }
            return solver;
        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Failed to create solver '" + name + "': " + e.what()
            );
        }
    }

    /**
     * @brief Check if a backend is registered
     * @param backend Backend type
     * @return true if registered
     */
    bool isRegistered(pde::SolverBackend backend) const {
        return backendCreators_.find(backend) != backendCreators_.end();
    }

    /**
     * @brief Check if a solver name is registered
     * @param name Solver name
     * @return true if registered
     */
    bool isRegistered(const std::string& name) const {
        return nameCreators_.find(name) != nameCreators_.end();
    }

    /**
     * @brief Get list of registered backends
     * @return Vector of registered backends
     */
    std::vector<pde::SolverBackend> getRegisteredBackends() const {
        std::vector<pde::SolverBackend> backends;
        backends.reserve(backendCreators_.size());
        for (const auto& pair : backendCreators_) {
            backends.push_back(pair.first);
        }
        return backends;
    }

    /**
     * @brief Get list of registered solver names
     * @return Vector of registered names
     */
    std::vector<std::string> getRegisteredNames() const {
        std::vector<std::string> names;
        names.reserve(nameCreators_.size());
        for (const auto& pair : nameCreators_) {
            names.push_back(pair.first);
        }
        return names;
    }

    /**
     * @brief Get number of registered solvers
     * @return Total number of registered solvers (backends + names)
     */
    size_t getNumRegistered() const {
        return backendCreators_.size() + nameCreators_.size();
    }

    /**
     * @brief Clear all registrations
     */
    void clear() {
        backendCreators_.clear();
        nameCreators_.clear();
    }

    /**
     * @brief Get factory information string
     */
    std::string getInfo() const {
        std::string info = "SolverFactory Information:\n";
        info += "  Registered Backends: " + std::to_string(backendCreators_.size()) + "\n";

        for (const auto& pair : backendCreators_) {
            info += "    - " + pde::toString(pair.first) + "\n";
        }

        info += "  Registered Names: " + std::to_string(nameCreators_.size()) + "\n";
        for (const auto& pair : nameCreators_) {
            info += "    - " + pair.first + "\n";
        }

        return info;
    }

    // Delete copy constructor and assignment operator (singleton)
    SolverFactory(const SolverFactory&) = delete;
    SolverFactory& operator=(const SolverFactory&) = delete;

private:
    /**
     * @brief Private constructor (singleton)
     */
    SolverFactory() {
        // Register built-in solvers
        registerBuiltInSolvers();
    }

    /**
     * @brief Register built-in solvers
     */
    void registerBuiltInSolvers() {
        // Register MockSolver
        backendCreators_[pde::SolverBackend::CUSTOM] = []() {
            return std::make_shared<MockSolver>();
        };

        nameCreators_["MockSolver"] = []() {
            return std::make_shared<MockSolver>();
        };

        nameCreators_["Mock"] = []() {
            return std::make_shared<MockSolver>();
        };
    }

    std::map<pde::SolverBackend, SolverCreator> backendCreators_;
    std::map<std::string, SolverCreator> nameCreators_;
};

/**
 * @brief Helper function to create a solver by backend
 */
inline std::shared_ptr<pde::ISolver> createSolver(pde::SolverBackend backend) {
    return SolverFactory::getInstance().createSolver(backend);
}

/**
 * @brief Helper function to create a solver by name
 */
inline std::shared_ptr<pde::ISolver> createSolver(const std::string& name) {
    return SolverFactory::getInstance().createSolver(name);
}

/**
 * @brief Solver registration helper (RAII)
 *
 * Automatically registers a solver on construction and unregisters on destruction.
 * Useful for plugin-style solver registration.
 */
class SolverRegistration {
public:
    /**
     * @brief Constructor - register by backend
     */
    SolverRegistration(pde::SolverBackend backend, SolverCreator creator)
        : backend_(backend), useName_(false) {
        SolverFactory::getInstance().registerSolver(backend, creator);
    }

    /**
     * @brief Constructor - register by name
     */
    SolverRegistration(const std::string& name, SolverCreator creator)
        : name_(name), backend_(pde::SolverBackend::CUSTOM), useName_(true) {
        SolverFactory::getInstance().registerSolver(name, creator);
    }

    /**
     * @brief Destructor - unregister
     */
    ~SolverRegistration() {
        if (useName_) {
            SolverFactory::getInstance().unregisterSolver(name_);
        } else {
            SolverFactory::getInstance().unregisterSolver(backend_);
        }
    }

    // Delete copy and move
    SolverRegistration(const SolverRegistration&) = delete;
    SolverRegistration& operator=(const SolverRegistration&) = delete;
    SolverRegistration(SolverRegistration&&) = delete;
    SolverRegistration& operator=(SolverRegistration&&) = delete;

private:
    std::string name_;
    pde::SolverBackend backend_;
    bool useName_;
};

} // namespace solver
} // namespace koo

#endif // KOO_SOLVER_SOLVER_FACTORY_H
