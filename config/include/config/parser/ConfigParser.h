/**
 * @file ConfigParser.h
 * @brief Configuration file parser for JSON and YAML formats
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * This file defines configuration parsers for loading configuration
 * from JSON and YAML files. Uses nlohmann-json and yaml-cpp if available,
 * with fallback implementations.
 */

#ifndef KOO_CONFIG_PARSER_CONFIG_PARSER_H
#define KOO_CONFIG_PARSER_CONFIG_PARSER_H

#include "config/Config.h"
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Optional dependencies
#ifdef USE_JSON
#include <nlohmann/json.hpp>
#endif

#ifdef USE_YAML
#include <yaml-cpp/yaml.h>
#endif

namespace koo {
namespace config {
namespace parser {

// ============================================================================
// Parser Format Enum
// ============================================================================

/**
 * @brief Configuration file format
 */
enum class ConfigFormat {
    JSON,           ///< JSON format
    YAML,           ///< YAML format
    AUTO_DETECT     ///< Auto-detect from file extension
};

// ============================================================================
// Parser Exceptions
// ============================================================================

/**
 * @brief Parser exception
 */
class ParserException : public std::runtime_error {
public:
    explicit ParserException(const std::string& message)
        : std::runtime_error(message) {}
};

// ============================================================================
// Config Parser Interface
// ============================================================================

/**
 * @brief Abstract configuration parser interface
 */
class IConfigParser {
public:
    virtual ~IConfigParser() = default;

    /**
     * @brief Parse configuration from file
     *
     * @param filename Path to configuration file
     * @param config Configuration object to populate
     */
    virtual void parseFile(const std::string& filename,
                          std::shared_ptr<Config> config) = 0;

    /**
     * @brief Parse configuration from string
     *
     * @param content Configuration content
     * @param config Configuration object to populate
     */
    virtual void parseString(const std::string& content,
                            std::shared_ptr<Config> config) = 0;

    /**
     * @brief Write configuration to file
     *
     * @param filename Path to output file
     * @param config Configuration object to write
     */
    virtual void writeFile(const std::string& filename,
                          std::shared_ptr<const Config> config) = 0;
};

// ============================================================================
// JSON Parser
// ============================================================================

#ifdef USE_JSON

/**
 * @brief JSON configuration parser using nlohmann-json
 */
class JsonParser : public IConfigParser {
public:
    void parseFile(const std::string& filename,
                   std::shared_ptr<Config> config) override {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw ParserException("Cannot open file: " + filename);
        }

        nlohmann::json j;
        try {
            file >> j;
        } catch (const nlohmann::json::parse_error& e) {
            throw ParserException("JSON parse error: " + std::string(e.what()));
        }

        parseJsonObject(j, "", config);
    }

    void parseString(const std::string& content,
                     std::shared_ptr<Config> config) override {
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(content);
        } catch (const nlohmann::json::parse_error& e) {
            throw ParserException("JSON parse error: " + std::string(e.what()));
        }

        parseJsonObject(j, "", config);
    }

    void writeFile(const std::string& filename,
                   std::shared_ptr<const Config> config) override {
        nlohmann::json j = nlohmann::json::object();

        // Convert Config to JSON
        auto keys = config->getKeys();
        for (const auto& key : keys) {
            auto entry = config->getEntry(key);
            if (!entry.has_value()) continue;

            setJsonValue(j, key, entry->value);
        }

        std::ofstream file(filename);
        if (!file.is_open()) {
            throw ParserException("Cannot open file for writing: " + filename);
        }

        file << j.dump(2); // Pretty print with 2-space indent
    }

private:
    /**
     * @brief Parse JSON object recursively
     */
    void parseJsonObject(const nlohmann::json& j, const std::string& prefix,
                        std::shared_ptr<Config> config) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();

            if (it->is_object()) {
                // Recursively parse nested object
                parseJsonObject(*it, key, config);
            } else if (it->is_boolean()) {
                config->set(key, it->get<bool>());
            } else if (it->is_number_integer()) {
                config->set(key, it->get<int>());
            } else if (it->is_number_float()) {
                config->set(key, it->get<double>());
            } else if (it->is_string()) {
                config->set(key, it->get<std::string>());
            } else if (it->is_array()) {
                parseJsonArray(*it, key, config);
            }
        }
    }

    /**
     * @brief Parse JSON array
     */
    void parseJsonArray(const nlohmann::json& j, const std::string& key,
                       std::shared_ptr<Config> config) {
        if (j.empty()) return;

        // Determine array type from first element
        if (j[0].is_boolean()) {
            std::vector<bool> arr;
            for (const auto& elem : j) {
                arr.push_back(elem.get<bool>());
            }
            config->set(key, arr);
        } else if (j[0].is_number_integer()) {
            std::vector<int> arr;
            for (const auto& elem : j) {
                arr.push_back(elem.get<int>());
            }
            config->set(key, arr);
        } else if (j[0].is_number_float()) {
            std::vector<double> arr;
            for (const auto& elem : j) {
                arr.push_back(elem.get<double>());
            }
            config->set(key, arr);
        } else if (j[0].is_string()) {
            std::vector<std::string> arr;
            for (const auto& elem : j) {
                arr.push_back(elem.get<std::string>());
            }
            config->set(key, arr);
        }
    }

    /**
     * @brief Set JSON value from ConfigValue
     */
    void setJsonValue(nlohmann::json& j, const std::string& key,
                     const ConfigValue& value) {
        // Split key into parts
        std::vector<std::string> parts = splitKey(key);

        // Navigate/create nested structure
        nlohmann::json* current = &j;
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current->contains(parts[i])) {
                (*current)[parts[i]] = nlohmann::json::object();
            }
            current = &(*current)[parts[i]];
        }

        // Set value based on type
        const std::string& lastKey = parts.back();
        if (std::holds_alternative<bool>(value)) {
            (*current)[lastKey] = std::get<bool>(value);
        } else if (std::holds_alternative<int>(value)) {
            (*current)[lastKey] = std::get<int>(value);
        } else if (std::holds_alternative<double>(value)) {
            (*current)[lastKey] = std::get<double>(value);
        } else if (std::holds_alternative<std::string>(value)) {
            (*current)[lastKey] = std::get<std::string>(value);
        } else if (std::holds_alternative<std::vector<bool>>(value)) {
            (*current)[lastKey] = std::get<std::vector<bool>>(value);
        } else if (std::holds_alternative<std::vector<int>>(value)) {
            (*current)[lastKey] = std::get<std::vector<int>>(value);
        } else if (std::holds_alternative<std::vector<double>>(value)) {
            (*current)[lastKey] = std::get<std::vector<double>>(value);
        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            (*current)[lastKey] = std::get<std::vector<std::string>>(value);
        }
    }

    /**
     * @brief Split key by dots
     */
    std::vector<std::string> splitKey(const std::string& key) const {
        std::vector<std::string> parts;
        std::stringstream ss(key);
        std::string part;
        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }
        return parts;
    }
};

#endif // USE_JSON

// ============================================================================
// YAML Parser
// ============================================================================

#ifdef USE_YAML

/**
 * @brief YAML configuration parser using yaml-cpp
 */
class YamlParser : public IConfigParser {
public:
    void parseFile(const std::string& filename,
                   std::shared_ptr<Config> config) override {
        try {
            YAML::Node root = YAML::LoadFile(filename);
            parseYamlNode(root, "", config);
        } catch (const YAML::Exception& e) {
            throw ParserException("YAML parse error: " + std::string(e.what()));
        }
    }

    void parseString(const std::string& content,
                     std::shared_ptr<Config> config) override {
        try {
            YAML::Node root = YAML::Load(content);
            parseYamlNode(root, "", config);
        } catch (const YAML::Exception& e) {
            throw ParserException("YAML parse error: " + std::string(e.what()));
        }
    }

    void writeFile(const std::string& filename,
                   std::shared_ptr<const Config> config) override {
        YAML::Emitter out;
        out << YAML::BeginMap;

        // Convert Config to YAML
        auto keys = config->getKeys();
        for (const auto& key : keys) {
            auto entry = config->getEntry(key);
            if (!entry.has_value()) continue;

            // For now, write flat structure (TODO: hierarchical)
            out << YAML::Key << key;
            emitYamlValue(out, entry->value);
        }

        out << YAML::EndMap;

        std::ofstream file(filename);
        if (!file.is_open()) {
            throw ParserException("Cannot open file for writing: " + filename);
        }

        file << out.c_str();
    }

private:
    /**
     * @brief Parse YAML node recursively
     */
    void parseYamlNode(const YAML::Node& node, const std::string& prefix,
                      std::shared_ptr<Config> config) {
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                std::string key = prefix.empty() ? it->first.as<std::string>()
                                                 : prefix + "." + it->first.as<std::string>();
                parseYamlNode(it->second, key, config);
            }
        } else if (node.IsScalar()) {
            // Try to determine type
            if (node.Tag() == "tag:yaml.org,2002:bool" ||
                node.as<std::string>() == "true" ||
                node.as<std::string>() == "false") {
                config->set(prefix, node.as<bool>());
            } else if (node.Tag() == "tag:yaml.org,2002:int" ||
                      isInteger(node.as<std::string>())) {
                config->set(prefix, node.as<int>());
            } else if (node.Tag() == "tag:yaml.org,2002:float" ||
                      isFloat(node.as<std::string>())) {
                config->set(prefix, node.as<double>());
            } else {
                config->set(prefix, node.as<std::string>());
            }
        } else if (node.IsSequence()) {
            parseYamlSequence(node, prefix, config);
        }
    }

    /**
     * @brief Parse YAML sequence (array)
     */
    void parseYamlSequence(const YAML::Node& node, const std::string& key,
                          std::shared_ptr<Config> config) {
        if (node.size() == 0) return;

        // Determine array type from first element
        const YAML::Node& first = node[0];
        if (first.IsScalar()) {
            std::string val = first.as<std::string>();
            if (val == "true" || val == "false") {
                std::vector<bool> arr;
                for (const auto& elem : node) {
                    arr.push_back(elem.as<bool>());
                }
                config->set(key, arr);
            } else if (isInteger(val)) {
                std::vector<int> arr;
                for (const auto& elem : node) {
                    arr.push_back(elem.as<int>());
                }
                config->set(key, arr);
            } else if (isFloat(val)) {
                std::vector<double> arr;
                for (const auto& elem : node) {
                    arr.push_back(elem.as<double>());
                }
                config->set(key, arr);
            } else {
                std::vector<std::string> arr;
                for (const auto& elem : node) {
                    arr.push_back(elem.as<std::string>());
                }
                config->set(key, arr);
            }
        }
    }

    /**
     * @brief Emit YAML value from ConfigValue
     */
    void emitYamlValue(YAML::Emitter& out, const ConfigValue& value) {
        out << YAML::Value;
        if (std::holds_alternative<bool>(value)) {
            out << std::get<bool>(value);
        } else if (std::holds_alternative<int>(value)) {
            out << std::get<int>(value);
        } else if (std::holds_alternative<double>(value)) {
            out << std::get<double>(value);
        } else if (std::holds_alternative<std::string>(value)) {
            out << std::get<std::string>(value);
        } else if (std::holds_alternative<std::vector<bool>>(value)) {
            out << std::get<std::vector<bool>>(value);
        } else if (std::holds_alternative<std::vector<int>>(value)) {
            out << std::get<std::vector<int>>(value);
        } else if (std::holds_alternative<std::vector<double>>(value)) {
            out << std::get<std::vector<double>>(value);
        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            out << std::get<std::vector<std::string>>(value);
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
        bool hasDot = false;
        bool hasExp = false;
        size_t start = (str[0] == '-' || str[0] == '+') ? 1 : 0;
        if (start >= str.length()) return false;

        for (size_t i = start; i < str.length(); ++i) {
            if (str[i] == '.') {
                if (hasDot || hasExp) return false;
                hasDot = true;
            } else if (str[i] == 'e' || str[i] == 'E') {
                if (hasExp) return false;
                hasExp = true;
                if (i + 1 < str.length() && (str[i+1] == '+' || str[i+1] == '-')) {
                    i++; // Skip sign after exponent
                }
            } else if (!std::isdigit(str[i])) {
                return false;
            }
        }
        return hasDot || hasExp;
    }
};

#endif // USE_YAML

// ============================================================================
// Config Parser Factory
// ============================================================================

/**
 * @brief Configuration parser factory
 */
class ConfigParserFactory {
public:
    /**
     * @brief Create a parser for the specified format
     *
     * @param format File format
     * @return Parser instance
     * @throws ParserException if format not supported
     */
    static std::unique_ptr<IConfigParser> create([[maybe_unused]] ConfigFormat format) {
#ifdef USE_JSON
        if (format == ConfigFormat::JSON) {
            return std::make_unique<JsonParser>();
        }
#endif

#ifdef USE_YAML
        if (format == ConfigFormat::YAML) {
            return std::make_unique<YamlParser>();
        }
#endif

        throw ParserException("Unsupported configuration format (library not available)");
    }

    /**
     * @brief Detect format from file extension
     *
     * @param filename File name
     * @return Detected format
     */
    static ConfigFormat detectFormat(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            throw ParserException("Cannot detect format: no file extension");
        }

        std::string ext = filename.substr(dotPos + 1);
        if (ext == "json") {
            return ConfigFormat::JSON;
        } else if (ext == "yaml" || ext == "yml") {
            return ConfigFormat::YAML;
        } else {
            throw ParserException("Unknown file extension: " + ext);
        }
    }

    /**
     * @brief Load configuration from file
     *
     * @param filename File path
     * @param format Format (or AUTO_DETECT)
     * @return Loaded configuration
     */
    static std::shared_ptr<Config> loadFile(const std::string& filename,
                                            ConfigFormat format = ConfigFormat::AUTO_DETECT) {
        if (format == ConfigFormat::AUTO_DETECT) {
            format = detectFormat(filename);
        }

        auto parser = create(format);
        auto config = std::make_shared<Config>();
        parser->parseFile(filename, config);
        return config;
    }

    /**
     * @brief Save configuration to file
     *
     * @param filename File path
     * @param config Configuration to save
     * @param format Format (or AUTO_DETECT)
     */
    static void saveFile(const std::string& filename,
                        std::shared_ptr<const Config> config,
                        ConfigFormat format = ConfigFormat::AUTO_DETECT) {
        if (format == ConfigFormat::AUTO_DETECT) {
            format = detectFormat(filename);
        }

        auto parser = create(format);
        parser->writeFile(filename, config);
    }
};

} // namespace parser
} // namespace config
} // namespace koo

#endif // KOO_CONFIG_PARSER_CONFIG_PARSER_H
