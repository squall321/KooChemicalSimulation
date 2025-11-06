/**
 * @file ConfigValidator.h
 * @brief Configuration validation framework
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-beta
 * @date 2025-11-06
 *
 * This file defines a validation framework for configuration values.
 * Supports range checking, enum validation, regex patterns, and custom
 * validation rules.
 */

#ifndef KOO_CONFIG_VALIDATOR_CONFIG_VALIDATOR_H
#define KOO_CONFIG_VALIDATOR_CONFIG_VALIDATOR_H

#include "config/Config.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <regex>
#include <limits>
#include <algorithm>

namespace koo {
namespace config {
namespace validator {

// ============================================================================
// Validation Result
// ============================================================================

/**
 * @brief Result of a validation check
 */
struct ValidationResult {
    bool valid;                      ///< Whether validation passed
    std::string message;             ///< Error/warning message
    std::string key;                 ///< Configuration key

    ValidationResult() : valid(true) {}

    ValidationResult(bool v, const std::string& msg = "", const std::string& k = "")
        : valid(v), message(msg), key(k) {}

    static ValidationResult success() {
        return ValidationResult(true);
    }

    static ValidationResult failure(const std::string& message, const std::string& key = "") {
        return ValidationResult(false, message, key);
    }
};

/**
 * @brief Collection of validation results
 */
class ValidationReport {
public:
    void addResult(const ValidationResult& result) {
        results_.push_back(result);
        if (!result.valid) {
            failureCount_++;
        }
    }

    bool isValid() const {
        return failureCount_ == 0;
    }

    size_t getFailureCount() const {
        return failureCount_;
    }

    size_t getTotalCount() const {
        return results_.size();
    }

    const std::vector<ValidationResult>& getResults() const {
        return results_;
    }

    std::vector<ValidationResult> getFailures() const {
        std::vector<ValidationResult> failures;
        for (const auto& result : results_) {
            if (!result.valid) {
                failures.push_back(result);
            }
        }
        return failures;
    }

    void clear() {
        results_.clear();
        failureCount_ = 0;
    }

private:
    std::vector<ValidationResult> results_;
    size_t failureCount_ = 0;
};

// ============================================================================
// Validation Rule Interface
// ============================================================================

/**
 * @brief Abstract validation rule
 */
class IValidationRule {
public:
    virtual ~IValidationRule() = default;

    /**
     * @brief Validate a configuration value
     *
     * @param value Value to validate
     * @param key Configuration key
     * @return Validation result
     */
    virtual ValidationResult validate(const ConfigValue& value,
                                     const std::string& key) const = 0;

    /**
     * @brief Get rule description
     */
    virtual std::string getDescription() const = 0;
};

// ============================================================================
// Concrete Validation Rules
// ============================================================================

/**
 * @brief Range validation for numeric values
 */
template<typename T>
class RangeRule : public IValidationRule {
public:
    RangeRule(T min, T max, bool includeMin = true, bool includeMax = true)
        : min_(min), max_(max), includeMin_(includeMin), includeMax_(includeMax) {}

    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        if (!std::holds_alternative<T>(value)) {
            return ValidationResult::failure("Type mismatch for range validation", key);
        }

        T val = std::get<T>(value);
        bool valid = true;

        if (includeMin_) {
            valid = valid && (val >= min_);
        } else {
            valid = valid && (val > min_);
        }

        if (includeMax_) {
            valid = valid && (val <= max_);
        } else {
            valid = valid && (val < max_);
        }

        if (!valid) {
            std::string msg = "Value out of range [";
            msg += includeMin_ ? "[" : "(";
            msg += std::to_string(min_) + ", " + std::to_string(max_);
            msg += includeMax_ ? "]" : ")";
            return ValidationResult::failure(msg, key);
        }

        return ValidationResult::success();
    }

    std::string getDescription() const override {
        std::string desc = "Value must be in range ";
        desc += includeMin_ ? "[" : "(";
        desc += std::to_string(min_) + ", " + std::to_string(max_);
        desc += includeMax_ ? "]" : ")";
        return desc;
    }

private:
    T min_;
    T max_;
    bool includeMin_;
    bool includeMax_;
};

/**
 * @brief Enum validation (value must be in a set)
 */
template<typename T>
class EnumRule : public IValidationRule {
public:
    explicit EnumRule(const std::vector<T>& allowedValues)
        : allowedValues_(allowedValues) {}

    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        if (!std::holds_alternative<T>(value)) {
            return ValidationResult::failure("Type mismatch for enum validation", key);
        }

        T val = std::get<T>(value);
        bool found = std::find(allowedValues_.begin(), allowedValues_.end(), val)
                     != allowedValues_.end();

        if (!found) {
            return ValidationResult::failure("Value not in allowed set", key);
        }

        return ValidationResult::success();
    }

    std::string getDescription() const override {
        return "Value must be one of the allowed values";
    }

private:
    std::vector<T> allowedValues_;
};

/**
 * @brief Regex validation for strings
 */
class RegexRule : public IValidationRule {
public:
    explicit RegexRule(const std::string& pattern)
        : pattern_(pattern), regex_(pattern) {}

    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        if (!std::holds_alternative<std::string>(value)) {
            return ValidationResult::failure("Type mismatch for regex validation", key);
        }

        std::string val = std::get<std::string>(value);
        if (!std::regex_match(val, regex_)) {
            return ValidationResult::failure("Value does not match pattern: " + pattern_, key);
        }

        return ValidationResult::success();
    }

    std::string getDescription() const override {
        return "Value must match pattern: " + pattern_;
    }

private:
    std::string pattern_;
    std::regex regex_;
};

/**
 * @brief Non-empty string validation
 */
class NonEmptyRule : public IValidationRule {
public:
    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        if (std::holds_alternative<std::string>(value)) {
            if (std::get<std::string>(value).empty()) {
                return ValidationResult::failure("String must not be empty", key);
            }
        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            if (std::get<std::vector<std::string>>(value).empty()) {
                return ValidationResult::failure("Array must not be empty", key);
            }
        }

        return ValidationResult::success();
    }

    std::string getDescription() const override {
        return "Value must not be empty";
    }
};

/**
 * @brief Custom validation function
 */
template<typename T>
class CustomRule : public IValidationRule {
public:
    using ValidatorFunc = std::function<bool(const T&)>;

    CustomRule(ValidatorFunc func, const std::string& description)
        : func_(func), description_(description) {}

    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        if (!std::holds_alternative<T>(value)) {
            return ValidationResult::failure("Type mismatch for custom validation", key);
        }

        T val = std::get<T>(value);
        if (!func_(val)) {
            return ValidationResult::failure(description_, key);
        }

        return ValidationResult::success();
    }

    std::string getDescription() const override {
        return description_;
    }

private:
    ValidatorFunc func_;
    std::string description_;
};

// ============================================================================
// Composite Validation Rules
// ============================================================================

/**
 * @brief AND composition of rules (all must pass)
 */
class AndRule : public IValidationRule {
public:
    void addRule(std::shared_ptr<IValidationRule> rule) {
        rules_.push_back(rule);
    }

    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        for (const auto& rule : rules_) {
            auto result = rule->validate(value, key);
            if (!result.valid) {
                return result;
            }
        }
        return ValidationResult::success();
    }

    std::string getDescription() const override {
        std::string desc = "All of: ";
        for (size_t i = 0; i < rules_.size(); ++i) {
            if (i > 0) desc += ", ";
            desc += rules_[i]->getDescription();
        }
        return desc;
    }

private:
    std::vector<std::shared_ptr<IValidationRule>> rules_;
};

/**
 * @brief OR composition of rules (at least one must pass)
 */
class OrRule : public IValidationRule {
public:
    void addRule(std::shared_ptr<IValidationRule> rule) {
        rules_.push_back(rule);
    }

    ValidationResult validate(const ConfigValue& value,
                             const std::string& key) const override {
        for (const auto& rule : rules_) {
            auto result = rule->validate(value, key);
            if (result.valid) {
                return ValidationResult::success();
            }
        }
        return ValidationResult::failure("None of the OR conditions satisfied", key);
    }

    std::string getDescription() const override {
        std::string desc = "Any of: ";
        for (size_t i = 0; i < rules_.size(); ++i) {
            if (i > 0) desc += ", ";
            desc += rules_[i]->getDescription();
        }
        return desc;
    }

private:
    std::vector<std::shared_ptr<IValidationRule>> rules_;
};

// ============================================================================
// Config Validator
// ============================================================================

/**
 * @brief Configuration validator
 *
 * Manages validation rules for configuration keys and performs validation.
 */
class ConfigValidator {
public:
    /**
     * @brief Add a validation rule for a key
     *
     * @param key Configuration key
     * @param rule Validation rule
     */
    void addRule(const std::string& key, std::shared_ptr<IValidationRule> rule) {
        rules_[key].push_back(rule);
    }

    /**
     * @brief Validate a single key
     *
     * @param config Configuration to validate
     * @param key Key to validate
     * @return Validation result
     */
    ValidationResult validateKey(std::shared_ptr<const Config> config,
                                 const std::string& key) const {
        auto it = rules_.find(key);
        if (it == rules_.end()) {
            return ValidationResult::success(); // No rules = valid
        }

        auto entry = config->getEntry(key);
        if (!entry.has_value()) {
            return ValidationResult::failure("Key not found in configuration", key);
        }

        for (const auto& rule : it->second) {
            auto result = rule->validate(entry->value, key);
            if (!result.valid) {
                return result;
            }
        }

        return ValidationResult::success();
    }

    /**
     * @brief Validate entire configuration
     *
     * @param config Configuration to validate
     * @return Validation report
     */
    ValidationReport validate(std::shared_ptr<const Config> config) const {
        ValidationReport report;

        // Validate all keys with rules
        for (const auto& pair : rules_) {
            auto result = validateKey(config, pair.first);
            report.addResult(result);
        }

        // Check for required keys
        auto missing = config->validateRequired();
        for (const auto& key : missing) {
            report.addResult(ValidationResult::failure("Required key missing", key));
        }

        return report;
    }

    /**
     * @brief Remove all rules for a key
     *
     * @param key Configuration key
     */
    void removeRules(const std::string& key) {
        rules_.erase(key);
    }

    /**
     * @brief Clear all validation rules
     */
    void clear() {
        rules_.clear();
    }

    /**
     * @brief Get number of keys with validation rules
     */
    size_t getRuleCount() const {
        return rules_.size();
    }

    /**
     * @brief Check if a key has validation rules
     */
    bool hasRules(const std::string& key) const {
        return rules_.find(key) != rules_.end();
    }

private:
    std::map<std::string, std::vector<std::shared_ptr<IValidationRule>>> rules_;
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Create a range rule for integers
 */
inline std::shared_ptr<IValidationRule> makeIntRange(int min, int max) {
    return std::make_shared<RangeRule<int>>(min, max);
}

/**
 * @brief Create a range rule for doubles
 */
inline std::shared_ptr<IValidationRule> makeDoubleRange(double min, double max) {
    return std::make_shared<RangeRule<double>>(min, max);
}

/**
 * @brief Create a positive number rule
 */
inline std::shared_ptr<IValidationRule> makePositive() {
    return std::make_shared<RangeRule<double>>(0.0, std::numeric_limits<double>::max(), false, true);
}

/**
 * @brief Create a non-negative number rule
 */
inline std::shared_ptr<IValidationRule> makeNonNegative() {
    return std::make_shared<RangeRule<double>>(0.0, std::numeric_limits<double>::max());
}

/**
 * @brief Create an enum rule for strings
 */
inline std::shared_ptr<IValidationRule> makeStringEnum(const std::vector<std::string>& values) {
    return std::make_shared<EnumRule<std::string>>(values);
}

/**
 * @brief Create a regex rule
 */
inline std::shared_ptr<IValidationRule> makeRegex(const std::string& pattern) {
    return std::make_shared<RegexRule>(pattern);
}

/**
 * @brief Create a non-empty rule
 */
inline std::shared_ptr<IValidationRule> makeNonEmpty() {
    return std::make_shared<NonEmptyRule>();
}

} // namespace validator
} // namespace config
} // namespace koo

#endif // KOO_CONFIG_VALIDATOR_CONFIG_VALIDATOR_H
