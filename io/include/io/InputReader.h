#ifndef KOO_INPUT_READER_H
#define KOO_INPUT_READER_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace koo {
namespace io {

/**
 * @class InputReader
 * @brief Reads input files in simple key-value format
 *
 * Phase 31: Input System
 *
 * Format:
 * # Comments start with #
 * key = value
 * [section]
 * nested_key = value
 */
class InputReader {
public:
    /**
     * @brief Constructor
     */
    InputReader() = default;

    /**
     * @brief Read file
     */
    bool readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            lastError_ = "Cannot open file: " + filename;
            return false;
        }

        std::string line;
        std::string currentSection = "";
        int lineNumber = 0;

        while (std::getline(file, line)) {
            lineNumber++;

            // Trim whitespace
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            // Check for section
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            // Parse key-value pair
            size_t pos = line.find('=');
            if (pos == std::string::npos) {
                lastError_ = "Invalid line " + std::to_string(lineNumber) + ": " + line;
                continue;
            }

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            // Store with section prefix
            std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
            data_[fullKey] = value;
        }

        file.close();
        return true;
    }

    /**
     * @brief Get string value
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Get integer value
     */
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = data_.find(key);
        if (it == data_.end()) return defaultValue;
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }

    /**
     * @brief Get double value
     */
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = data_.find(key);
        if (it == data_.end()) return defaultValue;
        try {
            return std::stod(it->second);
        } catch (...) {
            return defaultValue;
        }
    }

    /**
     * @brief Get boolean value
     */
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = data_.find(key);
        if (it == data_.end()) return defaultValue;

        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        return (value == "true" || value == "yes" || value == "1");
    }

    /**
     * @brief Check if key exists
     */
    bool hasKey(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    /**
     * @brief Get all keys
     */
    std::vector<std::string> getKeys() const {
        std::vector<std::string> keys;
        for (const auto& pair : data_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    /**
     * @brief Get last error
     */
    std::string getLastError() const { return lastError_; }

private:
    std::map<std::string, std::string> data_;
    std::string lastError_;

    /**
     * @brief Trim whitespace
     */
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }
};

} // namespace io
} // namespace koo

#endif // KOO_INPUT_READER_H
