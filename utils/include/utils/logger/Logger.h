/**
 * @file Logger.h
 * @brief Logging system for the framework
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha3
 * @date 2025-11-06
 *
 * This file defines a flexible logging system with multiple severity levels
 * and output destinations. Can be integrated with spdlog when available.
 */

#ifndef KOO_UTILS_LOGGER_LOGGER_H
#define KOO_UTILS_LOGGER_LOGGER_H

#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>

// Use spdlog if available (Phase 1 made it optional)
#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#endif

namespace koo {
namespace utils {
namespace logger {

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    TRACE,    ///< Detailed trace information
    DEBUG,    ///< Debug information
    INFO,     ///< General information
    WARNING,  ///< Warning messages
    ERROR,    ///< Error messages
    CRITICAL, ///< Critical errors
    OFF       ///< Disable logging
};

/**
 * @brief Convert log level to string
 */
inline const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:    return "TRACE";
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        case LogLevel::OFF:      return "OFF";
        default:                 return "UNKNOWN";
    }
}

#ifndef USE_SPDLOG

/**
 * @brief Simple logger implementation (fallback when spdlog is not available)
 *
 * This is a basic logger that outputs to console and/or file. For production
 * use, spdlog integration is recommended for better performance and features.
 */
class SimpleLogger {
public:
    /**
     * @brief Get singleton instance
     */
    static SimpleLogger& getInstance() {
        static SimpleLogger instance;
        return instance;
    }

    /**
     * @brief Set log level
     */
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    /**
     * @brief Get current log level
     */
    LogLevel getLevel() const {
        return level_;
    }

    /**
     * @brief Enable file logging
     */
    void enableFileLogging(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        logFile_.open(filename, std::ios::out | std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    /**
     * @brief Disable file logging
     */
    void disableFileLogging() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    /**
     * @brief Log a message
     */
    void log(LogLevel level, const std::string& message) {
        if (level < level_ || level == LogLevel::OFF) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
            << "[" << levelToString(level) << "] "
            << message;

        std::string formattedMessage = oss.str();

        // Output to console
        if (level >= LogLevel::ERROR) {
            std::cerr << formattedMessage << std::endl;
        } else {
            std::cout << formattedMessage << std::endl;
        }

        // Output to file if enabled
        if (logFile_.is_open()) {
            logFile_ << formattedMessage << std::endl;
            logFile_.flush();
        }
    }

private:
    SimpleLogger() : level_(LogLevel::INFO) {}
    ~SimpleLogger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    // Delete copy and move
    SimpleLogger(const SimpleLogger&) = delete;
    SimpleLogger& operator=(const SimpleLogger&) = delete;
    SimpleLogger(SimpleLogger&&) = delete;
    SimpleLogger& operator=(SimpleLogger&&) = delete;

    LogLevel level_;
    std::ofstream logFile_;
    std::mutex mutex_;
};

#endif // !USE_SPDLOG

/**
 * @brief Logger class - unified interface for logging
 *
 * This class provides a unified logging interface that works with or without
 * spdlog. When spdlog is available, it uses spdlog for better performance
 * and features. Otherwise, it falls back to the simple logger.
 *
 * Example usage:
 * @code
 * Logger::info("Starting simulation");
 * Logger::debug("Iteration {}: residual = {}", i, residual);
 * Logger::error("Failed to converge after {} iterations", maxIter);
 * @endcode
 */
class Logger {
public:
    /**
     * @brief Initialize logger with name
     */
    static void initialize(const std::string& name = "KooChemicalSimulation") {
#ifdef USE_SPDLOG
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        logger_ = std::make_shared<spdlog::logger>(name, console_sink);
        logger_->set_level(spdlog::level::info);
        spdlog::set_default_logger(logger_);
#else
        (void)name; // Suppress unused parameter warning
#endif
    }

    /**
     * @brief Set log level
     */
    static void setLevel(LogLevel level) {
#ifdef USE_SPDLOG
        spdlog::level::level_enum spdlogLevel;
        switch (level) {
            case LogLevel::TRACE:    spdlogLevel = spdlog::level::trace; break;
            case LogLevel::DEBUG:    spdlogLevel = spdlog::level::debug; break;
            case LogLevel::INFO:     spdlogLevel = spdlog::level::info; break;
            case LogLevel::WARNING:  spdlogLevel = spdlog::level::warn; break;
            case LogLevel::ERROR:    spdlogLevel = spdlog::level::err; break;
            case LogLevel::CRITICAL: spdlogLevel = spdlog::level::critical; break;
            case LogLevel::OFF:      spdlogLevel = spdlog::level::off; break;
            default:                 spdlogLevel = spdlog::level::info; break;
        }
        if (logger_) {
            logger_->set_level(spdlogLevel);
        }
#else
        SimpleLogger::getInstance().setLevel(level);
#endif
    }

    /**
     * @brief Enable file logging
     */
    static void enableFileLogging(const std::string& filename) {
#ifdef USE_SPDLOG
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
        if (logger_) {
            logger_->sinks().push_back(file_sink);
        }
#else
        SimpleLogger::getInstance().enableFileLogging(filename);
#endif
    }

    /**
     * @brief Log trace message
     */
    template<typename... Args>
    static void trace(const std::string& fmt, Args&&... args) {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->trace(fmt, std::forward<Args>(args)...);
        }
#else
        log(LogLevel::TRACE, format(fmt, std::forward<Args>(args)...));
#endif
    }

    /**
     * @brief Log debug message
     */
    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->debug(fmt, std::forward<Args>(args)...);
        }
#else
        log(LogLevel::DEBUG, format(fmt, std::forward<Args>(args)...));
#endif
    }

    /**
     * @brief Log info message
     */
    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->info(fmt, std::forward<Args>(args)...);
        }
#else
        log(LogLevel::INFO, format(fmt, std::forward<Args>(args)...));
#endif
    }

    /**
     * @brief Log warning message
     */
    template<typename... Args>
    static void warning(const std::string& fmt, Args&&... args) {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->warn(fmt, std::forward<Args>(args)...);
        }
#else
        log(LogLevel::WARNING, format(fmt, std::forward<Args>(args)...));
#endif
    }

    /**
     * @brief Log error message
     */
    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->error(fmt, std::forward<Args>(args)...);
        }
#else
        log(LogLevel::ERROR, format(fmt, std::forward<Args>(args)...));
#endif
    }

    /**
     * @brief Log critical message
     */
    template<typename... Args>
    static void critical(const std::string& fmt, Args&&... args) {
#ifdef USE_SPDLOG
        if (logger_) {
            logger_->critical(fmt, std::forward<Args>(args)...);
        }
#else
        log(LogLevel::CRITICAL, format(fmt, std::forward<Args>(args)...));
#endif
    }

private:
#ifdef USE_SPDLOG
    static std::shared_ptr<spdlog::logger> logger_;
#else
    /**
     * @brief Simple string formatting (fallback)
     */
    template<typename... Args>
    static std::string format(const std::string& fmt, Args&&... /*args*/) {
        // Simple fallback - just return the format string
        // In production, would use fmt library or similar
        return fmt;
    }

    /**
     * @brief Log message using simple logger
     */
    static void log(LogLevel level, const std::string& message) {
        SimpleLogger::getInstance().log(level, message);
    }
#endif
};

#ifdef USE_SPDLOG
inline std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
#endif

// ============================================================================
// Convenience Macros
// ============================================================================

#define LOG_TRACE(...)    ::koo::utils::logger::Logger::trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::koo::utils::logger::Logger::debug(__VA_ARGS__)
#define LOG_INFO(...)     ::koo::utils::logger::Logger::info(__VA_ARGS__)
#define LOG_WARNING(...)  ::koo::utils::logger::Logger::warning(__VA_ARGS__)
#define LOG_ERROR(...)    ::koo::utils::logger::Logger::error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::koo::utils::logger::Logger::critical(__VA_ARGS__)

} // namespace logger
} // namespace utils
} // namespace koo

#endif // KOO_UTILS_LOGGER_LOGGER_H
