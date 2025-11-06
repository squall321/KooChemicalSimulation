/**
 * @file ConfigEnvironment.h
 * @brief Environment variable integration for configuration
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 38: Environment Variable Integration
 *
 * Allows loading configuration from environment variables:
 * - KOO_SOLVER_TYPE=implicit
 * - KOO_TIME_DT=0.001
 * - Priority: env vars > config file > defaults
 */

#ifndef KOO_CONFIG_ENV_CONFIG_ENVIRONMENT_H
#define KOO_CONFIG_ENV_CONFIG_ENVIRONMENT_H

#include "config/Config.h"
#include <string>
#include <cstdlib>
#include <cctype>
#include <algorithm>
#include <sstream>

// Declare environment variable access
#ifndef _WIN32
extern "C" {
    extern char** environ;
}
#endif

namespace koo {
namespace config {
namespace env {

// ============================================================================
// Environment Variable Loader
// ============================================================================

/**
 * @brief Environment variable loader for configuration
 *
 * Converts environment variables to configuration keys:
 * - Environment: KOO_SOLVER_TYPE=implicit
 * - Config key: solver.type
 */
class EnvironmentLoader {
public:
    /**
     * @brief Constructor
     * @param prefix Environment variable prefix (default: "KOO_")
     */
    explicit EnvironmentLoader(const std::string& prefix = "KOO_")
        : prefix_(prefix) {}

    /**
     * @brief Load environment variables into configuration
     *
     * Scans all environment variables with the configured prefix
     * and adds them to the configuration
     *
     * @param config Configuration to populate
     * @param overwrite If true, overwrite existing values
     * @return Number of variables loaded
     */
    size_t loadEnvironment(std::shared_ptr<Config> config, bool overwrite = true) const {
        size_t count = 0;

        // Get environment variables (platform-specific)
#ifdef _WIN32
        // Windows: use _environ
        extern char** _environ;
        char** env = _environ;
#else
        // Unix/Linux: use environ
        char** env = ::environ;
#endif

        if (env == nullptr) return 0;

        for (int i = 0; env[i] != nullptr; ++i) {
            std::string envVar(env[i]);
            size_t eqPos = envVar.find('=');
            if (eqPos == std::string::npos) continue;

            std::string name = envVar.substr(0, eqPos);
            std::string value = envVar.substr(eqPos + 1);

            // Check if variable has our prefix
            if (name.find(prefix_) != 0) continue;

            // Convert to config key
            std::string key = envVarToConfigKey(name);

            // Check if we should set this value
            if (!overwrite && config->has(key)) continue;

            // Try to determine type and set value
            setConfigValue(config, key, value);
            count++;
        }

        return count;
    }

    /**
     * @brief Load a specific environment variable
     *
     * @param config Configuration to populate
     * @param envVarName Environment variable name (e.g., "KOO_SOLVER_TYPE")
     * @param configKey Configuration key (e.g., "solver.type")
     * @return true if variable was loaded
     */
    bool loadVariable(std::shared_ptr<Config> config,
                     const std::string& envVarName,
                     const std::string& configKey) const {
        const char* value = std::getenv(envVarName.c_str());
        if (value == nullptr) return false;

        setConfigValue(config, configKey, std::string(value));
        return true;
    }

    /**
     * @brief Load a variable using automatic key conversion
     *
     * @param config Configuration to populate
     * @param envVarName Environment variable name
     * @return true if variable was loaded
     */
    bool loadVariable(std::shared_ptr<Config> config,
                     const std::string& envVarName) const {
        std::string configKey = envVarToConfigKey(envVarName);
        return loadVariable(config, envVarName, configKey);
    }

    /**
     * @brief Get environment variable value
     *
     * @param envVarName Environment variable name
     * @return Value if found, nullopt otherwise
     */
    std::optional<std::string> getValue(const std::string& envVarName) const {
        const char* value = std::getenv(envVarName.c_str());
        if (value == nullptr) return std::nullopt;
        return std::string(value);
    }

    /**
     * @brief Check if environment variable exists
     */
    bool hasVariable(const std::string& envVarName) const {
        return std::getenv(envVarName.c_str()) != nullptr;
    }

    /**
     * @brief Convert config key to environment variable name
     *
     * Example: "solver.type" -> "KOO_SOLVER_TYPE"
     */
    std::string configKeyToEnvVar(const std::string& key) const {
        std::string envVar = prefix_;
        for (char c : key) {
            if (c == '.') {
                envVar += '_';
            } else {
                envVar += std::toupper(c);
            }
        }
        return envVar;
    }

    /**
     * @brief Convert environment variable name to config key
     *
     * Example: "KOO_SOLVER_TYPE" -> "solver.type"
     */
    std::string envVarToConfigKey(const std::string& envVar) const {
        if (envVar.find(prefix_) != 0) return envVar;

        std::string key = envVar.substr(prefix_.length());
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // Replace underscores with dots
        for (char& c : key) {
            if (c == '_') c = '.';
        }

        return key;
    }

    std::string getPrefix() const { return prefix_; }
    void setPrefix(const std::string& prefix) { prefix_ = prefix; }

private:
    /**
     * @brief Set config value from string, attempting type inference
     */
    void setConfigValue(std::shared_ptr<Config> config,
                       const std::string& key,
                       const std::string& valueStr) const {
        // Try to infer type
        if (valueStr == "true" || valueStr == "True" || valueStr == "TRUE") {
            config->set(key, true);
        } else if (valueStr == "false" || valueStr == "False" || valueStr == "FALSE") {
            config->set(key, false);
        } else if (isInteger(valueStr)) {
            config->set(key, std::stoi(valueStr));
        } else if (isFloat(valueStr)) {
            config->set(key, std::stod(valueStr));
        } else if (isArray(valueStr)) {
            parseArray(config, key, valueStr);
        } else {
            // Default to string
            config->set(key, valueStr);
        }
    }

    /**
     * @brief Check if string represents an integer
     */
    bool isInteger(const std::string& str) const {
        if (str.empty()) return false;
        size_t start = (str[0] == '-' || str[0] == '+') ? 1 : 0;
        if (start >= str.length()) return false;
        for (size_t i = start; i < str.length(); ++i) {
            if (!std::isdigit(str[i])) return false;
        }
        return true;
    }

    /**
     * @brief Check if string represents a float
     */
    bool isFloat(const std::string& str) const {
        if (str.empty()) return false;
        char* end;
        std::strtod(str.c_str(), &end);
        return *end == '\0' && str.find('.') != std::string::npos;
    }

    /**
     * @brief Check if string is an array (comma-separated)
     */
    bool isArray(const std::string& str) const {
        return str.find(',') != std::string::npos;
    }

    /**
     * @brief Parse array from comma-separated string
     */
    void parseArray(std::shared_ptr<Config> config,
                   const std::string& key,
                   const std::string& str) const {
        std::vector<std::string> parts;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            parts.push_back(item);
        }

        if (parts.empty()) return;

        // Determine array type from first element
        if (parts[0] == "true" || parts[0] == "false") {
            std::vector<bool> arr;
            for (const auto& p : parts) {
                arr.push_back(p == "true");
            }
            config->set(key, arr);
        } else if (isInteger(parts[0])) {
            std::vector<int> arr;
            for (const auto& p : parts) {
                arr.push_back(std::stoi(p));
            }
            config->set(key, arr);
        } else if (isFloat(parts[0])) {
            std::vector<double> arr;
            for (const auto& p : parts) {
                arr.push_back(std::stod(p));
            }
            config->set(key, arr);
        } else {
            config->set(key, parts);
        }
    }

    std::string prefix_;
};

// ============================================================================
// Environment Configuration Builder
// ============================================================================

/**
 * @brief Build configuration with environment variable support
 *
 * Priority order (highest to lowest):
 * 1. Environment variables
 * 2. Config file values
 * 3. Schema defaults
 */
class EnvironmentConfigBuilder {
public:
    /**
     * @brief Constructor
     * @param prefix Environment variable prefix
     */
    explicit EnvironmentConfigBuilder(const std::string& prefix = "KOO_")
        : loader_(prefix) {}

    /**
     * @brief Build configuration from multiple sources
     *
     * @param schemaDefaults Configuration with schema defaults
     * @param fileConfig Configuration from file (optional)
     * @return Combined configuration
     */
    std::shared_ptr<Config> build(std::shared_ptr<Config> schemaDefaults,
                                  std::shared_ptr<Config> fileConfig = nullptr) const {
        auto config = std::make_shared<Config>();

        // 1. Apply schema defaults
        if (schemaDefaults) {
            config->merge(*schemaDefaults, true);
        }

        // 2. Apply file configuration
        if (fileConfig) {
            config->merge(*fileConfig, true);
        }

        // 3. Apply environment variables (highest priority)
        loader_.loadEnvironment(config, true);

        return config;
    }

    /**
     * @brief Build with explicit priority control
     */
    std::shared_ptr<Config> buildWithPriority(
        std::shared_ptr<Config> defaults,
        std::shared_ptr<Config> fileConfig,
        bool envOverridesFile = true,
        bool fileOverridesDefaults = true) const {

        auto config = std::make_shared<Config>();

        // Apply in priority order
        config->merge(*defaults, true);

        if (fileConfig) {
            config->merge(*fileConfig, fileOverridesDefaults);
        }

        if (envOverridesFile) {
            loader_.loadEnvironment(config, true);
        }

        return config;
    }

    EnvironmentLoader& getLoader() { return loader_; }
    const EnvironmentLoader& getLoader() const { return loader_; }

private:
    EnvironmentLoader loader_;
};

} // namespace env
} // namespace config
} // namespace koo

#endif // KOO_CONFIG_ENV_CONFIG_ENVIRONMENT_H
