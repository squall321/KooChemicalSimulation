/**
 * @file ConfigUtil.h
 * @brief Configuration utilities: diff, merge, and documentation
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * Phase 39-40: Configuration Diff & Documentation
 *
 * Provides utilities for:
 * - Comparing configurations (diff)
 * - Smart merging with conflict resolution
 * - Generating documentation from schemas
 * - Exporting configuration reports
 */

#ifndef KOO_CONFIG_UTIL_CONFIG_UTIL_H
#define KOO_CONFIG_UTIL_CONFIG_UTIL_H

#include "config/Config.h"
#include "config/schema/ConfigSchema.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace koo {
namespace config {
namespace util {

// ============================================================================
// Configuration Difference
// ============================================================================

/**
 * @brief Type of configuration difference
 */
enum class DiffType {
    ADDED,      ///< Key exists in B but not in A
    REMOVED,    ///< Key exists in A but not in B
    MODIFIED,   ///< Key exists in both but values differ
    UNCHANGED   ///< Key exists in both with same value
};

/**
 * @brief Configuration difference entry
 */
struct ConfigDiff {
    DiffType type;
    std::string key;
    std::optional<ConfigValue> valueA;  ///< Value in config A
    std::optional<ConfigValue> valueB;  ///< Value in config B

    ConfigDiff(DiffType t, const std::string& k)
        : type(t), key(k) {}

    ConfigDiff(DiffType t, const std::string& k,
              const ConfigValue& a, const ConfigValue& b)
        : type(t), key(k), valueA(a), valueB(b) {}

    /**
     * @brief Get type name
     */
    std::string getTypeName() const {
        switch (type) {
            case DiffType::ADDED: return "ADDED";
            case DiffType::REMOVED: return "REMOVED";
            case DiffType::MODIFIED: return "MODIFIED";
            case DiffType::UNCHANGED: return "UNCHANGED";
            default: return "UNKNOWN";
        }
    }
};

/**
 * @brief Configuration diff result
 */
class ConfigDiffResult {
public:
    /**
     * @brief Add a diff entry
     */
    void addDiff(const ConfigDiff& diff) {
        diffs_.push_back(diff);
    }

    /**
     * @brief Get all diffs
     */
    const std::vector<ConfigDiff>& getDiffs() const {
        return diffs_;
    }

    /**
     * @brief Get diffs of a specific type
     */
    std::vector<ConfigDiff> getDiffsByType(DiffType type) const {
        std::vector<ConfigDiff> result;
        for (const auto& diff : diffs_) {
            if (diff.type == type) {
                result.push_back(diff);
            }
        }
        return result;
    }

    /**
     * @brief Check if configurations are identical
     */
    bool isIdentical() const {
        return getAddedCount() == 0 && getRemovedCount() == 0 && getModifiedCount() == 0;
    }

    /**
     * @brief Get count of added keys
     */
    size_t getAddedCount() const {
        return getDiffsByType(DiffType::ADDED).size();
    }

    /**
     * @brief Get count of removed keys
     */
    size_t getRemovedCount() const {
        return getDiffsByType(DiffType::REMOVED).size();
    }

    /**
     * @brief Get count of modified keys
     */
    size_t getModifiedCount() const {
        return getDiffsByType(DiffType::MODIFIED).size();
    }

    /**
     * @brief Get total number of differences
     */
    size_t getTotalDiffs() const {
        return getAddedCount() + getRemovedCount() + getModifiedCount();
    }

    /**
     * @brief Generate diff summary string
     */
    std::string summary() const {
        std::ostringstream oss;
        oss << "Configuration Diff Summary:\n";
        oss << "  Added:    " << getAddedCount() << "\n";
        oss << "  Removed:  " << getRemovedCount() << "\n";
        oss << "  Modified: " << getModifiedCount() << "\n";
        oss << "  Total:    " << getTotalDiffs() << "\n";
        return oss.str();
    }

    /**
     * @brief Generate detailed diff report
     */
    std::string report() const {
        std::ostringstream oss;
        oss << summary() << "\n";

        if (getTotalDiffs() == 0) {
            oss << "Configurations are identical.\n";
            return oss.str();
        }

        oss << "Details:\n";
        oss << "========\n\n";

        // Added keys
        auto added = getDiffsByType(DiffType::ADDED);
        if (!added.empty()) {
            oss << "Added Keys (" << added.size() << "):\n";
            for (const auto& diff : added) {
                oss << "  + " << diff.key << " = " << valueToString(diff.valueB.value()) << "\n";
            }
            oss << "\n";
        }

        // Removed keys
        auto removed = getDiffsByType(DiffType::REMOVED);
        if (!removed.empty()) {
            oss << "Removed Keys (" << removed.size() << "):\n";
            for (const auto& diff : removed) {
                oss << "  - " << diff.key << " = " << valueToString(diff.valueA.value()) << "\n";
            }
            oss << "\n";
        }

        // Modified keys
        auto modified = getDiffsByType(DiffType::MODIFIED);
        if (!modified.empty()) {
            oss << "Modified Keys (" << modified.size() << "):\n";
            for (const auto& diff : modified) {
                oss << "  ~ " << diff.key << ":\n";
                oss << "      " << valueToString(diff.valueA.value()) << " -> "
                    << valueToString(diff.valueB.value()) << "\n";
            }
        }

        return oss.str();
    }

private:
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

    std::vector<ConfigDiff> diffs_;
};

// ============================================================================
// Configuration Comparator
// ============================================================================

/**
 * @brief Compare configurations
 */
class ConfigComparator {
public:
    /**
     * @brief Compare two configurations
     *
     * @param configA First configuration
     * @param configB Second configuration
     * @param includeUnchanged Include unchanged keys in result
     * @return Diff result
     */
    static ConfigDiffResult compare(std::shared_ptr<const Config> configA,
                                   std::shared_ptr<const Config> configB,
                                   bool includeUnchanged = false) {
        ConfigDiffResult result;

        auto keysA = configA->getKeys();
        auto keysB = configB->getKeys();

        // Check for removed and modified keys
        for (const auto& key : keysA) {
            if (!configB->has(key)) {
                // Removed
                auto entry = configA->getEntry(key);
                ConfigDiff diff(DiffType::REMOVED, key);
                diff.valueA = entry->value;
                result.addDiff(diff);
            } else {
                // Check if modified
                auto entryA = configA->getEntry(key);
                auto entryB = configB->getEntry(key);

                if (!valuesEqual(entryA->value, entryB->value)) {
                    ConfigDiff diff(DiffType::MODIFIED, key, entryA->value, entryB->value);
                    result.addDiff(diff);
                } else if (includeUnchanged) {
                    ConfigDiff diff(DiffType::UNCHANGED, key, entryA->value, entryB->value);
                    result.addDiff(diff);
                }
            }
        }

        // Check for added keys
        for (const auto& key : keysB) {
            if (!configA->has(key)) {
                // Added
                auto entry = configB->getEntry(key);
                ConfigDiff diff(DiffType::ADDED, key);
                diff.valueB = entry->value;
                result.addDiff(diff);
            }
        }

        return result;
    }

private:
    /**
     * @brief Check if two ConfigValues are equal
     */
    static bool valuesEqual(const ConfigValue& a, const ConfigValue& b) {
        if (a.index() != b.index()) return false;

        if (std::holds_alternative<bool>(a)) {
            return std::get<bool>(a) == std::get<bool>(b);
        } else if (std::holds_alternative<int>(a)) {
            return std::get<int>(a) == std::get<int>(b);
        } else if (std::holds_alternative<double>(a)) {
            return std::abs(std::get<double>(a) - std::get<double>(b)) < 1.0e-15;
        } else if (std::holds_alternative<std::string>(a)) {
            return std::get<std::string>(a) == std::get<std::string>(b);
        } else if (std::holds_alternative<std::vector<bool>>(a)) {
            return std::get<std::vector<bool>>(a) == std::get<std::vector<bool>>(b);
        } else if (std::holds_alternative<std::vector<int>>(a)) {
            return std::get<std::vector<int>>(a) == std::get<std::vector<int>>(b);
        } else if (std::holds_alternative<std::vector<double>>(a)) {
            const auto& va = std::get<std::vector<double>>(a);
            const auto& vb = std::get<std::vector<double>>(b);
            if (va.size() != vb.size()) return false;
            for (size_t i = 0; i < va.size(); ++i) {
                if (std::abs(va[i] - vb[i]) > 1.0e-15) return false;
            }
            return true;
        } else if (std::holds_alternative<std::vector<std::string>>(a)) {
            return std::get<std::vector<std::string>>(a) == std::get<std::vector<std::string>>(b);
        }

        return false;
    }
};

// ============================================================================
// Configuration Documentation Generator
// ============================================================================

/**
 * @brief Generate documentation from configuration and schema
 */
class ConfigDocGenerator {
public:
    /**
     * @brief Generate documentation for a configuration
     *
     * @param config Configuration
     * @param schema Optional schema for additional metadata
     * @return Documentation string
     */
    static std::string generate(std::shared_ptr<const Config> config,
                                const schema::ConfigSchema* schema = nullptr) {
        std::ostringstream oss;

        // Header
        std::time_t now = std::time(nullptr);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        oss << "Configuration Documentation\n";
        oss << "==========================\n";
        oss << "Generated: " << timeStr << "\n\n";

        if (schema) {
            oss << "Schema: " << schema->getName() << " (v" << schema->getVersion() << ")\n\n";
        }

        // Configuration summary
        oss << "Total Parameters: " << config->size() << "\n\n";

        // Group by category if schema available
        if (schema) {
            generateCategorizedDoc(oss, config, schema);
        } else {
            generateFlatDoc(oss, config);
        }

        return oss.str();
    }

private:
    /**
     * @brief Generate categorized documentation
     */
    static void generateCategorizedDoc(std::ostringstream& oss,
                                      std::shared_ptr<const Config> config,
                                      const schema::ConfigSchema* schema) {
        auto categories = schema->getCategories();

        for (const auto& category : categories) {
            oss << "## " << category << "\n\n";
            auto fields = schema->getFieldsByCategory(category);

            for (const auto& field : fields) {
                if (config->has(field.key)) {
                    formatParameter(oss, field.key, config, &field);
                }
            }
            oss << "\n";
        }

        // Uncategorized
        auto keys = config->getKeys();
        std::vector<std::string> uncategorized;
        for (const auto& key : keys) {
            auto field = schema->getField(key);
            if (!field.has_value() || field->category.empty()) {
                uncategorized.push_back(key);
            }
        }

        if (!uncategorized.empty()) {
            oss << "## Other\n\n";
            for (const auto& key : uncategorized) {
                formatParameter(oss, key, config, nullptr);
            }
        }
    }

    /**
     * @brief Generate flat documentation
     */
    static void generateFlatDoc(std::ostringstream& oss,
                               std::shared_ptr<const Config> config) {
        auto keys = config->getKeys();
        for (const auto& key : keys) {
            formatParameter(oss, key, config, nullptr);
        }
    }

    /**
     * @brief Format a single parameter
     */
    static void formatParameter(std::ostringstream& oss,
                               const std::string& key,
                               std::shared_ptr<const Config> config,
                               const schema::SchemaField* field) {
        auto entry = config->getEntry(key);
        if (!entry.has_value()) return;

        oss << "**" << key << "**\n";
        oss << "  - Value: " << valueToString(entry->value) << "\n";
        oss << "  - Type: " << typeToString(config->getType(key)) << "\n";

        if (field) {
            if (!field->description.empty()) {
                oss << "  - Description: " << field->description << "\n";
            }
            if (field->required) {
                oss << "  - Required: Yes\n";
            }
        }

        oss << "\n";
    }

    static std::string valueToString(const ConfigValue& value) {
        if (std::holds_alternative<bool>(value)) {
            return std::get<bool>(value) ? "true" : "false";
        } else if (std::holds_alternative<int>(value)) {
            return std::to_string(std::get<int>(value));
        } else if (std::holds_alternative<double>(value)) {
            return std::to_string(std::get<double>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }
        return "[array]";
    }

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
};

} // namespace util
} // namespace config
} // namespace koo

#endif // KOO_CONFIG_UTIL_CONFIG_UTIL_H
