/**
 * @file ConfigPreset.h
 * @brief Configuration presets and templates
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 37: Configuration Presets
 *
 * Provides pre-configured templates for common simulation scenarios:
 * - Fast prototyping
 * - High accuracy
 * - Large scale simulations
 * - Surface chemistry
 * - Reaction-diffusion
 */

#ifndef KOO_CONFIG_PRESET_CONFIG_PRESET_H
#define KOO_CONFIG_PRESET_CONFIG_PRESET_H

#include "config/Config.h"
#include <string>
#include <memory>
#include <map>
#include <functional>

namespace koo {
namespace config {
namespace preset {

// ============================================================================
// Preset Types
// ============================================================================

/**
 * @brief Configuration preset type
 */
enum class PresetType {
    FAST,              ///< Fast computation, lower accuracy
    BALANCED,          ///< Balanced speed and accuracy
    ACCURATE,          ///< High accuracy, slower
    PRODUCTION,        ///< Production run with output
    DEBUG,             ///< Debug mode with verbose output
    SURFACE_CHEMISTRY, ///< Surface chemistry simulation
    REACTION_DIFFUSION,///< Reaction-diffusion patterns
    TRANSPORT,         ///< Transport phenomena
    CUSTOM             ///< Custom preset
};

// ============================================================================
// Configuration Preset
// ============================================================================

/**
 * @brief Configuration preset
 *
 * Encapsulates a pre-configured set of parameters for common scenarios
 */
class ConfigPreset {
public:
    /**
     * @brief Default constructor
     */
    ConfigPreset()
        : name_(""), description_(""), type_(PresetType::CUSTOM) {}

    /**
     * @brief Constructor
     * @param name Preset name
     * @param description Description
     * @param type Preset type
     */
    ConfigPreset(const std::string& name,
                const std::string& description,
                PresetType type = PresetType::CUSTOM)
        : name_(name), description_(description), type_(type) {}

    /**
     * @brief Apply this preset to a configuration
     *
     * @param config Configuration to modify
     * @param overwrite If true, overwrite existing values
     */
    void apply(std::shared_ptr<Config> config, bool overwrite = true) const {
        for (const auto& pair : values_) {
            if (overwrite || !config->has(pair.first)) {
                setConfigValue(config, pair.first, pair.second);
            }
        }
    }

    /**
     * @brief Set a preset value
     */
    template<typename T>
    void set(const std::string& key, const T& value) {
        values_[key] = ConfigValue(value);
    }

    /**
     * @brief Get preset values
     */
    const std::map<std::string, ConfigValue>& getValues() const {
        return values_;
    }

    std::string getName() const { return name_; }
    std::string getDescription() const { return description_; }
    PresetType getType() const { return type_; }

private:
    /**
     * @brief Set config value from ConfigValue variant
     */
    static void setConfigValue(std::shared_ptr<Config> config,
                               const std::string& key,
                               const ConfigValue& value) {
        if (std::holds_alternative<bool>(value)) {
            config->set(key, std::get<bool>(value));
        } else if (std::holds_alternative<int>(value)) {
            config->set(key, std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            config->set(key, std::get<double>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            config->set(key, std::get<std::string>(value));
        } else if (std::holds_alternative<std::vector<bool>>(value)) {
            config->set(key, std::get<std::vector<bool>>(value));
        } else if (std::holds_alternative<std::vector<int>>(value)) {
            config->set(key, std::get<std::vector<int>>(value));
        } else if (std::holds_alternative<std::vector<double>>(value)) {
            config->set(key, std::get<std::vector<double>>(value));
        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            config->set(key, std::get<std::vector<std::string>>(value));
        }
    }

    std::string name_;
    std::string description_;
    PresetType type_;
    std::map<std::string, ConfigValue> values_;
};

// ============================================================================
// Preset Factory
// ============================================================================

/**
 * @brief Configuration preset factory
 *
 * Creates and manages standard presets
 */
class PresetFactory {
public:
    /**
     * @brief Create a fast computation preset
     */
    static ConfigPreset createFast() {
        ConfigPreset preset("Fast", "Fast computation, lower accuracy", PresetType::FAST);

        // Solver settings
        preset.set("solver.type", std::string("explicit"));
        preset.set("solver.tolerance", 1.0e-3);
        preset.set("solver.max_iterations", 100);

        // Time stepping
        preset.set("time.dt", 0.01);
        preset.set("time.cfl", 0.9);
        preset.set("time.output_interval", 100);

        // Output
        preset.set("output.format", std::string("csv"));
        preset.set("output.verbose", false);

        return preset;
    }

    /**
     * @brief Create a balanced preset
     */
    static ConfigPreset createBalanced() {
        ConfigPreset preset("Balanced", "Balanced speed and accuracy", PresetType::BALANCED);

        preset.set("solver.type", std::string("implicit"));
        preset.set("solver.tolerance", 1.0e-6);
        preset.set("solver.max_iterations", 500);
        preset.set("solver.preconditioner", std::string("jacobi"));

        preset.set("time.dt", 0.001);
        preset.set("time.cfl", 0.5);
        preset.set("time.output_interval", 50);

        preset.set("output.format", std::string("vtk"));
        preset.set("output.verbose", false);

        return preset;
    }

    /**
     * @brief Create a high accuracy preset
     */
    static ConfigPreset createAccurate() {
        ConfigPreset preset("Accurate", "High accuracy, slower", PresetType::ACCURATE);

        preset.set("solver.type", std::string("implicit"));
        preset.set("solver.tolerance", 1.0e-9);
        preset.set("solver.max_iterations", 2000);
        preset.set("solver.preconditioner", std::string("ilu"));

        preset.set("time.dt", 0.0001);
        preset.set("time.cfl", 0.25);
        preset.set("time.output_interval", 10);

        preset.set("mesh.refine_level", 2);

        preset.set("output.format", std::string("vtk"));
        preset.set("output.precision", 12);
        preset.set("output.verbose", false);

        return preset;
    }

    /**
     * @brief Create a production run preset
     */
    static ConfigPreset createProduction() {
        ConfigPreset preset("Production", "Production run with output", PresetType::PRODUCTION);

        preset.set("solver.type", std::string("implicit"));
        preset.set("solver.tolerance", 1.0e-6);
        preset.set("solver.max_iterations", 1000);
        preset.set("solver.preconditioner", std::string("ilu"));

        preset.set("time.dt", 0.001);
        preset.set("time.output_interval", 100);

        preset.set("output.format", std::string("vtk"));
        preset.set("output.checkpoint_interval", 1000);
        preset.set("output.save_fields", true);
        preset.set("output.save_statistics", true);
        preset.set("output.verbose", true);

        preset.set("logging.level", std::string("INFO"));
        preset.set("logging.file", std::string("simulation.log"));

        return preset;
    }

    /**
     * @brief Create a debug preset
     */
    static ConfigPreset createDebug() {
        ConfigPreset preset("Debug", "Debug mode with verbose output", PresetType::DEBUG);

        preset.set("solver.type", std::string("explicit"));
        preset.set("solver.tolerance", 1.0e-3);
        preset.set("solver.max_iterations", 50);

        preset.set("time.dt", 0.01);
        preset.set("time.output_interval", 1);

        preset.set("output.verbose", true);
        preset.set("output.save_every_step", true);

        preset.set("logging.level", std::string("DEBUG"));
        preset.set("logging.console", true);

        return preset;
    }

    /**
     * @brief Create a surface chemistry preset
     */
    static ConfigPreset createSurfaceChemistry() {
        ConfigPreset preset("Surface Chemistry", "Surface chemistry simulation",
                          PresetType::SURFACE_CHEMISTRY);

        preset.set("solver.type", std::string("implicit"));
        preset.set("solver.tolerance", 1.0e-8);

        preset.set("time.dt", 1.0e-6);  // Microsecond scale
        preset.set("time.output_interval", 1000);

        preset.set("surface.mechanism", std::string("langmuir_hinshelwood"));
        preset.set("surface.site_density", 1.0e19);  // sites/m^2
        preset.set("surface.temperature", 300.0);     // K

        preset.set("output.format", std::string("csv"));
        preset.set("output.save_coverage", true);
        preset.set("output.save_rates", true);

        return preset;
    }

    /**
     * @brief Create a reaction-diffusion preset
     */
    static ConfigPreset createReactionDiffusion() {
        ConfigPreset preset("Reaction-Diffusion", "Pattern formation simulation",
                          PresetType::REACTION_DIFFUSION);

        preset.set("solver.type", std::string("implicit"));
        preset.set("solver.tolerance", 1.0e-6);

        preset.set("time.dt", 0.01);
        preset.set("time.t_final", 1000.0);
        preset.set("time.output_interval", 100);

        preset.set("coupling.model", std::string("gray_scott"));
        preset.set("coupling.diffusion_u", 2.0e-5);
        preset.set("coupling.diffusion_v", 1.0e-5);
        preset.set("coupling.feed_rate", 0.055);
        preset.set("coupling.kill_rate", 0.062);

        preset.set("mesh.dimension", 2);
        preset.set("mesh.periodic", true);

        preset.set("output.format", std::string("vtk"));
        preset.set("output.save_pattern_metrics", true);

        return preset;
    }

    /**
     * @brief Create a transport preset
     */
    static ConfigPreset createTransport() {
        ConfigPreset preset("Transport", "Transport phenomena simulation",
                          PresetType::TRANSPORT);

        preset.set("solver.type", std::string("implicit"));
        preset.set("solver.tolerance", 1.0e-6);

        preset.set("time.dt", 0.001);
        preset.set("time.cfl", 0.5);

        preset.set("transport.diffusion_coefficient", 1.0e-9);
        preset.set("transport.velocity_field", std::string("uniform"));
        preset.set("transport.boundary_conditions", std::string("dirichlet"));

        preset.set("output.format", std::string("vtk"));
        preset.set("output.save_fluxes", true);

        return preset;
    }

    /**
     * @brief Create a preset by type
     */
    static ConfigPreset create(PresetType type) {
        switch (type) {
            case PresetType::FAST:
                return createFast();
            case PresetType::BALANCED:
                return createBalanced();
            case PresetType::ACCURATE:
                return createAccurate();
            case PresetType::PRODUCTION:
                return createProduction();
            case PresetType::DEBUG:
                return createDebug();
            case PresetType::SURFACE_CHEMISTRY:
                return createSurfaceChemistry();
            case PresetType::REACTION_DIFFUSION:
                return createReactionDiffusion();
            case PresetType::TRANSPORT:
                return createTransport();
            default:
                return ConfigPreset("Custom", "Custom preset", PresetType::CUSTOM);
        }
    }

    /**
     * @brief Get all available preset types
     */
    static std::vector<PresetType> getAvailableTypes() {
        return {
            PresetType::FAST,
            PresetType::BALANCED,
            PresetType::ACCURATE,
            PresetType::PRODUCTION,
            PresetType::DEBUG,
            PresetType::SURFACE_CHEMISTRY,
            PresetType::REACTION_DIFFUSION,
            PresetType::TRANSPORT
        };
    }

    /**
     * @brief Get preset type name
     */
    static std::string getTypeName(PresetType type) {
        switch (type) {
            case PresetType::FAST: return "Fast";
            case PresetType::BALANCED: return "Balanced";
            case PresetType::ACCURATE: return "Accurate";
            case PresetType::PRODUCTION: return "Production";
            case PresetType::DEBUG: return "Debug";
            case PresetType::SURFACE_CHEMISTRY: return "Surface Chemistry";
            case PresetType::REACTION_DIFFUSION: return "Reaction-Diffusion";
            case PresetType::TRANSPORT: return "Transport";
            case PresetType::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
};

// ============================================================================
// Preset Manager
// ============================================================================

/**
 * @brief Manages custom presets
 */
class PresetManager {
public:
    /**
     * @brief Register a custom preset
     */
    void registerPreset(const std::string& name, const ConfigPreset& preset) {
        presets_[name] = preset;
    }

    /**
     * @brief Get a preset by name
     */
    std::optional<ConfigPreset> getPreset(const std::string& name) const {
        auto it = presets_.find(name);
        if (it == presets_.end()) return std::nullopt;
        return it->second;
    }

    /**
     * @brief Check if a preset exists
     */
    bool hasPreset(const std::string& name) const {
        return presets_.find(name) != presets_.end();
    }

    /**
     * @brief Get all preset names
     */
    std::vector<std::string> getPresetNames() const {
        std::vector<std::string> names;
        names.reserve(presets_.size());
        for (const auto& pair : presets_) {
            names.push_back(pair.first);
        }
        return names;
    }

    /**
     * @brief Remove a preset
     */
    bool removePreset(const std::string& name) {
        return presets_.erase(name) > 0;
    }

    /**
     * @brief Clear all presets
     */
    void clear() {
        presets_.clear();
    }

    /**
     * @brief Get singleton instance
     */
    static PresetManager& getInstance() {
        static PresetManager instance;
        return instance;
    }

private:
    PresetManager() = default;
    std::map<std::string, ConfigPreset> presets_;
};

} // namespace preset
} // namespace config
} // namespace koo

#endif // KOO_CONFIG_PRESET_CONFIG_PRESET_H
