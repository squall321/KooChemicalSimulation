/**
 * @file Exception.h
 * @brief Exception handling framework
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha3
 * @date 2025-11-06
 *
 * This file defines a hierarchical exception system for the framework.
 * All framework exceptions derive from KooException, which extends std::exception.
 */

#ifndef KOO_UTILS_ERROR_EXCEPTION_H
#define KOO_UTILS_ERROR_EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>
#include <source_location>

namespace koo {
namespace utils {
namespace error {

/**
 * @brief Base exception class for all framework exceptions
 *
 * This class extends std::exception and provides additional context
 * information such as file location, function name, and custom messages.
 *
 * Example usage:
 * @code
 * if (value < 0) {
 *     throw InvalidArgumentException("Value must be non-negative");
 * }
 * @endcode
 */
class KooException : public std::exception {
public:
    /**
     * @brief Constructor with message
     */
    explicit KooException(const std::string& message)
        : message_(message), fullMessage_(message) {}

    /**
     * @brief Constructor with message and location info
     */
    KooException(const std::string& message,
                 const char* file,
                 int line,
                 const char* function)
        : message_(message) {
        std::ostringstream oss;
        oss << "[" << function << " at " << file << ":" << line << "] "
            << message;
        fullMessage_ = oss.str();
    }

    /**
     * @brief Virtual destructor
     */
    virtual ~KooException() = default;

    /**
     * @brief Get the error message
     */
    virtual const char* what() const noexcept override {
        return fullMessage_.c_str();
    }

    /**
     * @brief Get the short message (without location)
     */
    const std::string& message() const noexcept {
        return message_;
    }

    /**
     * @brief Get exception type name
     */
    virtual const char* type() const noexcept {
        return "KooException";
    }

protected:
    std::string message_;      ///< Short error message
    std::string fullMessage_;  ///< Full error message with location
};

// ============================================================================
// Specific Exception Types
// ============================================================================

/**
 * @brief Exception for invalid function arguments
 */
class InvalidArgumentException : public KooException {
public:
    explicit InvalidArgumentException(const std::string& message)
        : KooException(message) {}

    InvalidArgumentException(const std::string& message,
                            const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "InvalidArgumentException";
    }
};

/**
 * @brief Exception for index out of range errors
 */
class OutOfRangeException : public KooException {
public:
    explicit OutOfRangeException(const std::string& message)
        : KooException(message) {}

    OutOfRangeException(const std::string& message,
                       const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "OutOfRangeException";
    }
};

/**
 * @brief Exception for runtime errors
 */
class RuntimeException : public KooException {
public:
    explicit RuntimeException(const std::string& message)
        : KooException(message) {}

    RuntimeException(const std::string& message,
                    const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "RuntimeException";
    }
};

/**
 * @brief Exception for logic errors
 */
class LogicException : public KooException {
public:
    explicit LogicException(const std::string& message)
        : KooException(message) {}

    LogicException(const std::string& message,
                  const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "LogicException";
    }
};

/**
 * @brief Exception for not implemented features
 */
class NotImplementedException : public KooException {
public:
    explicit NotImplementedException(const std::string& message = "Feature not implemented")
        : KooException(message) {}

    NotImplementedException(const std::string& message,
                           const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "NotImplementedException";
    }
};

/**
 * @brief Exception for file I/O errors
 */
class IOException : public KooException {
public:
    explicit IOException(const std::string& message)
        : KooException(message) {}

    IOException(const std::string& message,
               const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "IOException";
    }
};

/**
 * @brief Exception for convergence failures
 */
class ConvergenceException : public KooException {
public:
    explicit ConvergenceException(const std::string& message)
        : KooException(message) {}

    ConvergenceException(const std::string& message,
                        const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "ConvergenceException";
    }
};

/**
 * @brief Exception for mesh-related errors
 */
class MeshException : public KooException {
public:
    explicit MeshException(const std::string& message)
        : KooException(message) {}

    MeshException(const std::string& message,
                 const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "MeshException";
    }
};

/**
 * @brief Exception for solver-related errors
 */
class SolverException : public KooException {
public:
    explicit SolverException(const std::string& message)
        : KooException(message) {}

    SolverException(const std::string& message,
                   const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "SolverException";
    }
};

/**
 * @brief Exception for chemistry-related errors
 */
class ChemistryException : public KooException {
public:
    explicit ChemistryException(const std::string& message)
        : KooException(message) {}

    ChemistryException(const std::string& message,
                      const char* file, int line, const char* function)
        : KooException(message, file, line, function) {}

    const char* type() const noexcept override {
        return "ChemistryException";
    }
};

// ============================================================================
// Convenience Macros
// ============================================================================

/**
 * @brief Throw exception with automatic location information
 *
 * Example: THROW_EXCEPTION(InvalidArgumentException, "Value must be positive");
 */
#define THROW_EXCEPTION(ExceptionType, message) \
    throw ExceptionType(message, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Assert condition and throw exception if false
 *
 * Example: KOO_ASSERT(x > 0, "x must be positive");
 */
#define KOO_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            THROW_EXCEPTION(LogicException, \
                std::string("Assertion failed: ") + #condition + ". " + message); \
        } \
    } while (false)

/**
 * @brief Runtime assertion
 */
#define KOO_RUNTIME_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            THROW_EXCEPTION(RuntimeException, \
                std::string("Runtime assertion failed: ") + #condition + ". " + message); \
        } \
    } while (false)

/**
 * @brief Check for null pointer
 */
#define KOO_CHECK_NOT_NULL(ptr, name) \
    do { \
        if ((ptr) == nullptr) { \
            THROW_EXCEPTION(InvalidArgumentException, \
                std::string(name) + " must not be null"); \
        } \
    } while (false)

/**
 * @brief Check for valid range
 */
#define KOO_CHECK_RANGE(value, min, max, name) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            THROW_EXCEPTION(OutOfRangeException, \
                std::string(name) + " must be in range [" + \
                std::to_string(min) + ", " + std::to_string(max) + "]"); \
        } \
    } while (false)

/**
 * @brief Mark code as not implemented
 */
#define KOO_NOT_IMPLEMENTED() \
    THROW_EXCEPTION(NotImplementedException, \
        "This feature is not yet implemented")

} // namespace error
} // namespace utils
} // namespace koo

#endif // KOO_UTILS_ERROR_EXCEPTION_H
