/**
 * @file ConfigSchema.h
 * @brief Configuration schema definition and validation
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 36: Configuration Schema
 *
 * Defines expected configuration structure with types, defaults,
 * constraints, and documentation for auto-validation and generation.
 */

#ifndef KOO_CONFIG_SCHEMA_CONFIG_SCHEMA_H
#define KOO_CONFIG_SCHEMA_CONFIG_SCHEMA_H

#include "config/Config.h"
#include "config/validator/ConfigValidator.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <optional>

namespace koo {
namespace config {
namespace schema {

// ============================================================================
// Schema Field Definition
// ============================================================================

/**
 * @brief Schema field definition
 *
 * Defines expected properties for a configuration parameter
 */
struct SchemaField {
    std::string key;                          ///< Configuration key
    ConfigType type;                          ///< Expected type
    bool required;                            ///< Whether field is required
    std::optional<ConfigValue> defaultValue;  ///< Default value
    std::string description;                  ///< Field description
    std::string category;                     ///< Category (e.g., "solver", "mesh")
    std::vector<std::shared_ptr<validator::IValidationRule>> rules; ///< Validation rules

    SchemaField() : type(ConfigType::NONE), required(false) {}

    SchemaField(const std::string& k, ConfigType t, bool req = false,
               const std::string& desc = "", const std::string& cat = "")
        : key(k), type(t), required(req), description(desc), category(cat) {}

    /**
     * @brief Add a validation rule
     */
    void addRule(std::shared_ptr<validator::IValidationRule> rule) {
        rules.push_back(rule);
    }

    /**
     * @brief Set default value
     */
    template<typename T>
    void setDefault(const T& value) {
        defaultValue = ConfigValue(value);
    }
};

// ============================================================================
// Configuration Schema
// ============================================================================

/**
 * @brief Configuration schema
 *
 * Defines the structure and constraints of a configuration
 */
class ConfigSchema {
public:
    /**
     * @brief Constructor
     * @param name Schema name
     * @param version Schema version
     */
    ConfigSchema(const std::string& name = "", const std::string& version = "1.0")
        : name_(name), version_(version) {}

    /**
     * @brief Add a field to the schema
     */
    void addField(const SchemaField& field) {
        fields_[field.key] = field;
    }

    /**
     * @brief Define a required field
     */
    SchemaField& defineRequired(const std::string& key, ConfigType type,
                               const std::string& description = "",
                               const std::string& category = "") {
        SchemaField field(key, type, true, description, category);
        fields_[key] = field;
        return fields_[key];
    }

    /**
     * @brief Define an optional field
     */
    template<typename T>
    SchemaField& defineOptional(const std::string& key, ConfigType type,
                               const T& defaultValue,
                               const std::string& description = "",
                               const std::string& category = "") {
        SchemaField field(key, type, false, description, category);
        field.setDefault(defaultValue);
        fields_[key] = field;
        return fields_[key];
    }

    /**
     * @brief Validate a configuration against this schema
     *
     * @param config Configuration to validate
     * @return Validation report
     */
    validator::ValidationReport validate(std::shared_ptr<const Config> config) const {
        validator::ValidationReport report;

        // Check all schema fields
        for (const auto& pair : fields_) {
            const std::string& key = pair.first;
            const SchemaField& field = pair.second;

            // Check if field exists
            if (!config->has(key)) {
                if (field.required) {
                    report.addResult(validator::ValidationResult::failure(
                        "Required field missing: " + field.description, key));
                }
                continue; // Skip validation if field not present
            }

            // Check type
            ConfigType actualType = config->getType(key);
            if (actualType != field.type) {
                report.addResult(validator::ValidationResult::failure(
                    "Type mismatch: expected " + typeToString(field.type) +
                    ", got " + typeToString(actualType), key));
                continue;
            }

            // Apply validation rules
            auto entry = config->getEntry(key);
            if (entry.has_value()) {
                for (const auto& rule : field.rules) {
                    auto result = rule->validate(entry->value, key);
                    report.addResult(result);
                    if (!result.valid) break; // Stop on first failure
                }
            }
        }

        return report;
    }

    /**
     * @brief Apply defaults to a configuration
     *
     * Sets default values for fields not present in the config
     *
     * @param config Configuration to modify
     * @return Number of defaults applied
     */
    size_t applyDefaults(std::shared_ptr<Config> config) const {
        size_t count = 0;
        for (const auto& pair : fields_) {
            const std::string& key = pair.first;
            const SchemaField& field = pair.second;

            if (!config->has(key) && field.defaultValue.has_value()) {
                const ConfigValue& defVal = field.defaultValue.value();
                setConfigValue(config, key, defVal);
                count++;
            }
        }
        return count;
    }

    /**
     * @brief Create a config validator from this schema
     */
    validator::ConfigValidator createValidator() const {
        validator::ConfigValidator validator;
        for (const auto& pair : fields_) {
            for (const auto& rule : pair.second.rules) {
                validator.addRule(pair.first, rule);
            }
        }
        return validator;
    }

    /**
     * @brief Get all fields
     */
    const std::map<std::string, SchemaField>& getFields() const {
        return fields_;
    }

    /**
     * @brief Get field by key
     */
    std::optional<SchemaField> getField(const std::string& key) const {
        auto it = fields_.find(key);
        if (it == fields_.end()) return std::nullopt;
        return it->second;
    }

    /**
     * @brief Get all fields in a category
     */
    std::vector<SchemaField> getFieldsByCategory(const std::string& category) const {
        std::vector<SchemaField> result;
        for (const auto& pair : fields_) {
            if (pair.second.category == category) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    /**
     * @brief Get all categories
     */
    std::vector<std::string> getCategories() const {
        std::vector<std::string> categories;
        for (const auto& pair : fields_) {
            if (!pair.second.category.empty()) {
                if (std::find(categories.begin(), categories.end(), pair.second.category)
                    == categories.end()) {
                    categories.push_back(pair.second.category);
                }
            }
        }
        return categories;
    }

    /**
     * @brief Get required fields
     */
    std::vector<std::string> getRequiredFields() const {
        std::vector<std::string> required;
        for (const auto& pair : fields_) {
            if (pair.second.required) {
                required.push_back(pair.first);
            }
        }
        return required;
    }

    /**
     * @brief Generate documentation string
     */
    std::string generateDocumentation() const {
        std::string doc;
        doc += "Configuration Schema: " + name_ + " (v" + version_ + ")\n";
        doc += "======================================\n\n";

        auto categories = getCategories();
        if (categories.empty()) {
            // No categories, just list all fields
            for (const auto& pair : fields_) {
                doc += formatFieldDoc(pair.second);
            }
        } else {
            // Group by category
            for (const auto& category : categories) {
                doc += "## " + category + "\n\n";
                auto fields = getFieldsByCategory(category);
                for (const auto& field : fields) {
                    doc += formatFieldDoc(field);
                }
                doc += "\n";
            }

            // Uncategorized fields
            std::vector<SchemaField> uncategorized;
            for (const auto& pair : fields_) {
                if (pair.second.category.empty()) {
                    uncategorized.push_back(pair.second);
                }
            }
            if (!uncategorized.empty()) {
                doc += "## Other\n\n";
                for (const auto& field : uncategorized) {
                    doc += formatFieldDoc(field);
                }
            }
        }

        return doc;
    }

    std::string getName() const { return name_; }
    std::string getVersion() const { return version_; }
    void setName(const std::string& name) { name_ = name; }
    void setVersion(const std::string& version) { version_ = version; }

private:
    /**
     * @brief Convert ConfigType to string
     */
    static std::string typeToString(ConfigType type) {
        switch (type) {
            case ConfigType::BOOL: return "bool";
            case ConfigType::INT: return "int";
            case ConfigType::DOUBLE: return "double";
            case ConfigType::STRING: return "string";
            case ConfigType::BOOL_ARRAY: return "bool[]";
            case ConfigType::INT_ARRAY: return "int[]";
            case ConfigType::DOUBLE_ARRAY: return "double[]";
            case ConfigType::STRING_ARRAY: return "string[]";
            default: return "unknown";
        }
    }

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

    /**
     * @brief Format field documentation
     */
    std::string formatFieldDoc(const SchemaField& field) const {
        std::string doc;
        doc += "- **" + field.key + "** (" + typeToString(field.type) + ")";
        if (field.required) {
            doc += " *[required]*";
        }
        doc += "\n";

        if (!field.description.empty()) {
            doc += "  - " + field.description + "\n";
        }

        if (field.defaultValue.has_value()) {
            doc += "  - Default: " + valueToString(field.defaultValue.value()) + "\n";
        }

        if (!field.rules.empty()) {
            doc += "  - Constraints: ";
            for (size_t i = 0; i < field.rules.size(); ++i) {
                if (i > 0) doc += "; ";
                doc += field.rules[i]->getDescription();
            }
            doc += "\n";
        }

        return doc;
    }

    /**
     * @brief Convert ConfigValue to string for documentation
     */
    static std::string valueToString(const ConfigValue& value) {
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? "true" : "false";
        } else if (std::holds_alternative<int>(value)) {
            return std::to_string(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            return std::to_string(std::get<double>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            return "\"" + std::get<std::string>(value) + "\"";
        }
        return "[array]";
    }

    std::string name_;                           ///< Schema name
    std::string version_;                        ///< Schema version
    std::map<std::string, SchemaField> fields_;  ///< Schema fields
};

// ============================================================================
// Common Schema Builders
// ============================================================================

/**
 * @brief Build a schema for solver configuration
 */
inline ConfigSchema buildSolverSchema() {
    ConfigSchema schema("Solver", "1.0");

    // Required fields
    schema.defineRequired("solver.type", ConfigType::STRING,
                         "Solver type (explicit, implicit, petsc, etc.)", "solver")
          .addRule(validator::makeStringEnum({"explicit", "implicit", "petsc", "ngsolve"}));

    // Optional with defaults
    schema.defineOptional("solver.tolerance", ConfigType::DOUBLE, 1.0e-6,
                         "Convergence tolerance", "solver")
          .addRule(validator::makePositive());

    schema.defineOptional("solver.max_iterations", ConfigType::INT, 1000,
                         "Maximum number of iterations", "solver")
          .addRule(validator::makeIntRange(1, 1000000));

    schema.defineOptional("solver.preconditioner", ConfigType::STRING, std::string("jacobi"),
                         "Preconditioner type", "solver")
          .addRule(validator::makeStringEnum({"none", "jacobi", "ilu", "multigrid"}));

    return schema;
}

/**
 * @brief Build a schema for mesh configuration
 */
inline ConfigSchema buildMeshSchema() {
    ConfigSchema schema("Mesh", "1.0");

    schema.defineRequired("mesh.filename", ConfigType::STRING,
                         "Mesh file path", "mesh")
          .addRule(validator::makeNonEmpty());

    schema.defineOptional("mesh.dimension", ConfigType::INT, 2,
                         "Spatial dimension", "mesh")
          .addRule(validator::makeIntRange(1, 3));

    schema.defineOptional("mesh.refine_level", ConfigType::INT, 0,
                         "Refinement level", "mesh")
          .addRule(validator::makeIntRange(0, 10));

    return schema;
}

/**
 * @brief Build a schema for time stepping configuration
 */
inline ConfigSchema buildTimeSteppingSchema() {
    ConfigSchema schema("Time Stepping", "1.0");

    schema.defineRequired("time.dt", ConfigType::DOUBLE,
                         "Time step size", "time")
          .addRule(validator::makePositive());

    schema.defineOptional("time.t_final", ConfigType::DOUBLE, 1.0,
                         "Final simulation time", "time")
          .addRule(validator::makePositive());

    schema.defineOptional("time.output_interval", ConfigType::INT, 10,
                         "Output interval (in steps)", "time")
          .addRule(validator::makeIntRange(1, 1000000));

    schema.defineOptional("time.cfl", ConfigType::DOUBLE, 0.5,
                         "CFL number for stability", "time")
          .addRule(validator::makeDoubleRange(0.0, 1.0));

    return schema;
}

} // namespace schema
} // namespace config
} // namespace koo

#endif // KOO_CONFIG_SCHEMA_CONFIG_SCHEMA_H
