#ifndef KOO_LOGGER_H
#define KOO_LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <memory>

namespace koo {
namespace io {

/**
 * @enum LogLevel
 * @brief Logging levels
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @class Logger
 * @brief Simple logging system
 *
 * Phase 34: Logging System
 */
class Logger {
public:
    /**
     * @brief Get singleton instance
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief Set log file
     */
    void setLogFile(const std::string& filename) {
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(filename, std::ios::app);
        logToFile_ = logFile_.is_open();
    }

    /**
     * @brief Set minimum log level
     */
    void setLevel(LogLevel level) {
        minLevel_ = level;
    }

    /**
     * @brief Enable/disable console output
     */
    void setConsoleOutput(bool enable) {
        logToConsole_ = enable;
    }

    /**
     * @brief Log message
     */
    void log(LogLevel level, const std::string& message) {
        if (level < minLevel_) return;

        std::string levelStr = levelToString(level);
        std::string timestamp = getTimestamp();

        std::stringstream ss;
        ss << "[" << timestamp << "] [" << levelStr << "] " << message;
        std::string fullMessage = ss.str();

        if (logToConsole_) {
            if (level >= LogLevel::ERROR) {
                std::cerr << fullMessage << std::endl;
            } else {
                std::cout << fullMessage << std::endl;
            }
        }

        if (logToFile_ && logFile_.is_open()) {
            logFile_ << fullMessage << std::endl;
            logFile_.flush();
        }
    }

    /**
     * @brief Convenience methods
     */
    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warning(const std::string& message) { log(LogLevel::WARNING, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }
    void critical(const std::string& message) { log(LogLevel::CRITICAL, message); }

    /**
     * @brief Close log file
     */
    void close() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logToFile_ = false;
    }

    ~Logger() { close(); }

private:
    Logger() : minLevel_(LogLevel::INFO), logToConsole_(true), logToFile_(false) {}

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Convert level to string
     */
    static std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARNING";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    /**
     * @brief Get current timestamp
     */
    static std::string getTimestamp() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    LogLevel minLevel_;
    bool logToConsole_;
    bool logToFile_;
    std::ofstream logFile_;
};

/**
 * @class ProgressMonitor
 * @brief Monitor simulation progress
 *
 * Phase 35: Progress Monitoring
 */
class ProgressMonitor {
public:
    /**
     * @brief Constructor
     */
    explicit ProgressMonitor(int totalSteps, int reportInterval = 10)
        : totalSteps_(totalSteps),
          currentStep_(0),
          reportInterval_(reportInterval),
          startTime_(std::time(nullptr)) {}

    /**
     * @brief Update progress
     */
    void update(int step) {
        currentStep_ = step;

        if (step % reportInterval_ == 0 || step == totalSteps_) {
            double percent = 100.0 * step / totalSteps_;
            auto elapsed = std::time(nullptr) - startTime_;
            double rate = elapsed > 0 ? static_cast<double>(step) / elapsed : 0.0;
            int remaining = rate > 0 ? (totalSteps_ - step) / rate : 0;

            std::stringstream ss;
            ss << "Progress: " << step << "/" << totalSteps_
               << " (" << std::fixed << std::setprecision(1) << percent << "%)"
               << " - Elapsed: " << elapsed << "s"
               << " - Remaining: ~" << remaining << "s";

            Logger::getInstance().info(ss.str());
        }
    }

    /**
     * @brief Mark as complete
     */
    void complete() {
        currentStep_ = totalSteps_;
        auto elapsed = std::time(nullptr) - startTime_;

        std::stringstream ss;
        ss << "Completed " << totalSteps_ << " steps in " << elapsed << " seconds";
        Logger::getInstance().info(ss.str());
    }

private:
    int totalSteps_;
    int currentStep_;
    int reportInterval_;
    std::time_t startTime_;
};

} // namespace io
} // namespace koo

#endif // KOO_LOGGER_H
