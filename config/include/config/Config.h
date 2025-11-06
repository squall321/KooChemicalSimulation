/**
 * @file Config.h
 * @brief Configuration management system
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * This file defines the core configuration management system for storing
 * and accessing simulation parameters. Supports hierarchical configuration,
 * type-safe access, and default value management.
 */

#ifndef KOO_CONFIG_CONFIG_H
#define KOO_CONFIG_CONFIG_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <mutex>
#include <stdexcept>

namespace koo {
namespace config {

// ============================================================================
// Configuration Value Types
// ============================================================================

/**
 * @brief Variant type for configuration values
 *
 * Supports: bool, int, double, string, and arrays of these types
 */
using ConfigValue = std::variant<
    bool,
    int,
    double,
    std::string,
    std::vector<bool>,
    std::vector<int>,
    std::vector<double>,
    std::vector<std::string>
>;

/**
 * @brief Configuration value type enum
 */
enum class ConfigType {
    BOOL,
    INT,
    DOUBLE,
    STRING,
    BOOL_ARRAY,
    INT_ARRAY,
    DOUBLE_ARRAY,
    STRING_ARRAY,
    NONE
};

// ============================================================================
// Configuration Entry
// ============================================================================

/**
 * @brief Configuration entry with metadata
 */
struct ConfigEntry {
    ConfigValue value;              ///< The actual value
    std::string description;         ///< Description of this parameter
    bool hasDefault;                 ///< Whether default value is set
    ConfigValue defaultValue;        ///< Default value
    bool isRequired;                 ///< Whether this parameter is required

    ConfigEntry()
        : value(false), hasDefault(false), defaultValue(false), isRequired(false) {}

    explicit ConfigEntry(ConfigValue val, const std::string& desc = "",
                        bool required = false)
        : value(val), description(desc), hasDefault(false),
          defaultValue(false), isRequired(required) {}

    ConfigEntry(ConfigValue val, ConfigValue defVal, const std::string& desc = "",
                bool required = false)
        : value(val), description(desc), hasDefault(true),
          defaultValue(defVal), isRequired(required) {}
};

// ============================================================================
// Configuration Class
// ============================================================================

/**
 * @brief Main configuration class
 *
 * Provides hierarchical configuration storage with type-safe access,
 * default value management, and validation support.
 *
 * Thread-safe for concurrent access.
 *
 * Example usage:
 * @code
 * Config config;
 * config.set("solver.tolerance", 1e-6);
 * config.set("solver.max_iterations", 1000);
 * config.set("mesh.filename", "mesh.msh");
 *
 * double tol = config.get<double>("solver.tolerance");
 * int maxIter = config.getOrDefault<int>("solver.max_iterations", 500);
 * @endcode
 */
class Config {
public:
    /**
     * @brief Constructor
     */
    Config() = default;

    /**
     * @brief Destructor
     */
    ~Config() = default;

    // Delete copy and move (use shared_ptr to share configs)
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    /**
     * @brief Set a configuration value
     *
     * @param key Hierarchical key (e.g., "solver.tolerance")
     * @param value Value to set
     */
    template<typename T>
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_[key] = ConfigEntry(ConfigValue(value));
    }

    /**
     * @brief Set a configuration value with description
     *
     * @param key Hierarchical key
     * @param value Value to set
     * @param description Description of the parameter
     * @param required Whether this parameter is required
     */
    template<typename T>
    void set(const std::string& key, const T& value,
             const std::string& description, bool required = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_[key] = ConfigEntry(ConfigValue(value), description, required);
    }

    /**
     * @brief Set a configuration value with default
     *
     * @param key Hierarchical key
     * @param value Current value
     * @param defaultValue Default value
     * @param description Description of the parameter
     * @param required Whether this parameter is required
     */
    template<typename T>
    void setWithDefault(const std::string& key, const T& value, const T& defaultValue,
                       const std::string& description = "", bool required = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_[key] = ConfigEntry(ConfigValue(value), ConfigValue(defaultValue),
                                    description, required);
    }

    /**
     * @brief Get a configuration value
     *
     * @param key Hierarchical key
     * @return The value
     * @throws std::out_of_range if key not found
     * @throws std::bad_variant_access if type mismatch
     */
    template<typename T>
    T get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            throw std::out_of_range("Configuration key not found: " + key);
        }
        return std::get<T>(it->second.value);
    }

    /**
     * @brief Get a configuration value or return default
     *
     * @param key Hierarchical key
     * @param defaultValue Value to return if key not found
     * @return The value or default
     */
    template<typename T>
    T getOrDefault(const std::string& key, const T& defaultValue) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return defaultValue;
        }
        try {
            return std::get<T>(it->second.value);
        } catch (const std::bad_variant_access&) {
            return defaultValue;
        }
    }

    /**
     * @brief Check if a key exists
     *
     * @param key Hierarchical key
     * @return true if key exists
     */
    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.find(key) != entries_.end();
    }

    /**
     * @brief Remove a configuration entry
     *
     * @param key Hierarchical key
     * @return true if entry was removed
     */
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.erase(key) > 0;
    }

    /**
     * @brief Clear all configuration entries
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
    }

    /**
     * @brief Get all keys
     *
     * @return Vector of all configuration keys
     */
    std::vector<std::string> getKeys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        keys.reserve(entries_.size());
        for (const auto& pair : entries_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    /**
     * @brief Get keys with a specific prefix
     *
     * @param prefix Key prefix (e.g., "solver." to get all solver keys)
     * @return Vector of matching keys
     */
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        for (const auto& pair : entries_) {
            if (pair.first.find(prefix) == 0) {
                keys.push_back(pair.first);
            }
        }
        return keys;
    }

    /**
     * @brief Get configuration entry (with metadata)
     *
     * @param key Hierarchical key
     * @return Optional containing the entry if found
     */
    std::optional<ConfigEntry> getEntry(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    /**
     * @brief Get the type of a configuration value
     *
     * @param key Hierarchical key
     * @return The type of the value, or NONE if key not found
     */
    ConfigType getType(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return ConfigType::NONE;
        }
        return getValueType(it->second.value);
    }

    /**
     * @brief Get number of configuration entries
     *
     * @return Number of entries
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    /**
     * @brief Check if configuration is empty
     *
     * @return true if no entries
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.empty();
    }

    /**
     * @brief Merge another configuration into this one
     *
     * @param other Configuration to merge
     * @param overwrite If true, overwrite existing keys
     */
    void merge(const Config& other, bool overwrite = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::lock_guard<std::mutex> otherLock(other.mutex_);

        for (const auto& pair : other.entries_) {
            if (overwrite || entries_.find(pair.first) == entries_.end()) {
                entries_[pair.first] = pair.second;
            }
        }
    }

    /**
     * @brief Create a sub-configuration with a key prefix
     *
     * @param prefix Key prefix
     * @return New Config containing only keys with the prefix
     */
    std::shared_ptr<Config> getSubConfig(const std::string& prefix) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto subConfig = std::make_shared<Config>();

        size_t prefixLen = prefix.length();
        for (const auto& pair : entries_) {
            if (pair.first.find(prefix) == 0) {
                // Remove prefix from key
                std::string subKey = pair.first.substr(prefixLen);
                if (!subKey.empty() && subKey[0] == '.') {
                    subKey = subKey.substr(1); // Remove leading dot
                }
                subConfig->entries_[subKey] = pair.second;
            }
        }

        return subConfig;
    }

    /**
     * @brief Reset entry to its default value
     *
     * @param key Hierarchical key
     * @return true if reset successful
     */
    bool resetToDefault(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end() || !it->second.hasDefault) {
            return false;
        }
        it->second.value = it->second.defaultValue;
        return true;
    }

    /**
     * @brief Reset all entries to their default values
     *
     * @return Number of entries reset
     */
    size_t resetAllToDefaults() {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (auto& pair : entries_) {
            if (pair.second.hasDefault) {
                pair.second.value = pair.second.defaultValue;
                count++;
            }
        }
        return count;
    }

    /**
     * @brief Validate that all required keys are set
     *
     * @return Vector of missing required keys (empty if all present)
     */
    std::vector<std::string> validateRequired() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> missing;
        for (const auto& pair : entries_) {
            if (pair.second.isRequired) {
                missing.push_back(pair.first);
            }
        }
        return missing;
    }

private:
    /**
     * @brief Get the type of a ConfigValue
     */
    static ConfigType getValueType(const ConfigValue& value) {
        if (std::holds_alternative<bool>(value)) return ConfigType::BOOL;
        if (std::holds_alternative<int>(value)) return ConfigType::INT;
        if (std::holds_alternative<double>(value)) return ConfigType::DOUBLE;
        if (std::holds_alternative<std::string>(value)) return ConfigType::STRING;
        if (std::holds_alternative<std::vector<bool>>(value)) return ConfigType::BOOL_ARRAY;
        if (std::holds_alternative<std::vector<int>>(value)) return ConfigType::INT_ARRAY;
        if (std::holds_alternative<std::vector<double>>(value)) return ConfigType::DOUBLE_ARRAY;
        if (std::holds_alternative<std::vector<std::string>>(value)) return ConfigType::STRING_ARRAY;
        return ConfigType::NONE;
    }

    std::map<std::string, ConfigEntry> entries_;  ///< Configuration entries
    mutable std::mutex mutex_;                     ///< Mutex for thread safety
};

} // namespace config
} // namespace koo

#endif // KOO_CONFIG_CONFIG_H
