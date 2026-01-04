#ifndef BOLT_LOGGING_HPP
#define BOLT_LOGGING_HPP

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <fstream>
#include <chrono>
#include <iostream>
#include <sstream>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <thread>
#include "thread_safety.hpp"

namespace bolt {

// Log levels in order of severity
enum class LogLevel {
    TRACE = 0,   // Detailed debugging information
    DEBUG = 1,   // General debugging information
    INFO = 2,    // Informational messages
    WARN = 3,    // Warning messages
    ERROR = 4,   // Error messages
    FATAL = 5,   // Fatal error messages
    OFF = 6      // Disable logging
};

// Log categories for different components
enum class LogCategory {
    CORE,         // Core system components
    MEMORY,       // Memory management
    EDITOR,       // Editor functionality
    AI,           // AI/ML components
    GUI,          // User interface
    NETWORK,      // Network operations
    PLUGIN,       // Plugin system
    COLLABORATION,// Collaborative editing
    DEBUGGER,     // Debugger functionality
    TESTING,      // Test framework
    UNKNOWN       // Unknown or miscellaneous
};

// Forward declarations
class LogFormatter;
class LogSink;

// Log entry structure
struct LogEntry {
    LogLevel level;
    LogCategory category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
    
    LogEntry(LogLevel lvl, LogCategory cat, const std::string& msg, 
             const std::string& f = "", int l = 0, const std::string& func = "")
        : level(lvl), category(cat), message(msg), file(f), line(l), 
          function(func), timestamp(std::chrono::system_clock::now()),
          threadId(std::this_thread::get_id()) {}
};

// Abstract base class for log formatters
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(const LogEntry& entry) = 0;
};

// Simple text formatter
class SimpleFormatter : public LogFormatter {
public:
    std::string format(const LogEntry& entry) override;
};

// Detailed formatter with timestamp, thread ID, etc.
class DetailedFormatter : public LogFormatter {
private:
    bool includeTimestamp_;
    bool includeThreadId_;
    bool includeLocation_;
    
public:
    DetailedFormatter(bool timestamp = true, bool threadId = true, bool location = false)
        : includeTimestamp_(timestamp), includeThreadId_(threadId), includeLocation_(location) {}
    
    std::string format(const LogEntry& entry) override;
    
    void setIncludeTimestamp(bool include) { includeTimestamp_ = include; }
    void setIncludeThreadId(bool include) { includeThreadId_ = include; }
    void setIncludeLocation(bool include) { includeLocation_ = include; }
};

// JSON formatter for structured logging
class JsonFormatter : public LogFormatter {
public:
    std::string format(const LogEntry& entry) override;
};

// Abstract base class for log output destinations
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogEntry& entry, const std::string& formattedMessage) = 0;
    virtual void flush() = 0;
    virtual bool isOpen() const = 0;
};

// Console output sink
class ConsoleSink : public LogSink {
private:
    bool useStderr_;
    
public:
    explicit ConsoleSink(bool useStderr = false) : useStderr_(useStderr) {}
    
    void write(const LogEntry& entry, const std::string& formattedMessage) override;
    void flush() override;
    bool isOpen() const override { return true; }
};

// File output sink
class FileSink : public LogSink {
private:
    std::ofstream file_;
    std::string filename_;
    std::mutex mutex_;
    
public:
    explicit FileSink(const std::string& filename);
    ~FileSink();
    
    void write(const LogEntry& entry, const std::string& formattedMessage) override;
    void flush() override;
    bool isOpen() const override;
    
    const std::string& getFilename() const { return filename_; }
};

// Rotating file sink (creates new files when size limit is reached)
class RotatingFileSink : public LogSink {
private:
    std::string baseFilename_;
    size_t maxFileSize_;
    size_t maxFiles_;
    std::unique_ptr<std::ofstream> currentFile_;
    size_t currentFileSize_;
    std::mutex mutex_;
    
    void rotateFile();
    std::string getRotatedFilename(size_t index);
    
public:
    RotatingFileSink(const std::string& baseFilename, size_t maxFileSize, size_t maxFiles);
    ~RotatingFileSink();
    
    void write(const LogEntry& entry, const std::string& formattedMessage) override;
    void flush() override;
    bool isOpen() const override;
};

// Log filter interface
class LogFilter {
public:
    virtual ~LogFilter() = default;
    virtual bool shouldLog(const LogEntry& entry) = 0;
};

// Level-based filter
class LevelFilter : public LogFilter {
private:
    LogLevel minLevel_;
    
public:
    explicit LevelFilter(LogLevel minLevel) : minLevel_(minLevel) {}
    
    bool shouldLog(const LogEntry& entry) override {
        return entry.level >= minLevel_;
    }
    
    void setMinLevel(LogLevel level) { minLevel_ = level; }
    LogLevel getMinLevel() const { return minLevel_; }
};

// Category-based filter
class CategoryFilter : public LogFilter {
private:
    std::unordered_map<LogCategory, LogLevel> categoryLevels_;
    LogLevel defaultLevel_;
    
public:
    explicit CategoryFilter(LogLevel defaultLevel = LogLevel::INFO)
        : defaultLevel_(defaultLevel) {}
    
    bool shouldLog(const LogEntry& entry) override;
    
    void setCategoryLevel(LogCategory category, LogLevel level) {
        categoryLevels_[category] = level;
    }
    
    void setDefaultLevel(LogLevel level) { defaultLevel_ = level; }
};

// Main logger class
class Logger {
private:
    LogLevel level_;
    LogCategory defaultCategory_;
    std::vector<std::unique_ptr<LogSink>> sinks_;
    std::vector<std::unique_ptr<LogFilter>> filters_;
    std::unique_ptr<LogFormatter> formatter_;
    ThreadSafe<bool> enabled_;
    mutable std::mutex mutex_;
    
    // Performance metrics
    std::atomic<size_t> totalMessages_;
    std::atomic<size_t> droppedMessages_;
    
    bool shouldLog(const LogEntry& entry) const;
    void writeToSinks(const LogEntry& entry);
    
public:
    Logger(LogLevel level = LogLevel::INFO, LogCategory defaultCategory = LogCategory::CORE);
    ~Logger();
    
    // Core logging methods
    void log(LogLevel level, LogCategory category, const std::string& message,
             const std::string& file = "", int line = 0, const std::string& function = "");
    
    void log(LogLevel level, const std::string& message,
             const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, defaultCategory_, message, file, line, function);
    }
    
    // Convenience methods for different log levels
    void trace(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::TRACE, message, file, line, function);
    }
    
    void debug(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::DEBUG, message, file, line, function);
    }
    
    void info(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::INFO, message, file, line, function);
    }
    
    void warn(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::WARN, message, file, line, function);
    }
    
    void error(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::ERROR, message, file, line, function);
    }
    
    void fatal(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::FATAL, message, file, line, function);
    }
    
    // Category-specific logging
    void logCore(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::CORE, message, file, line, function);
    }
    
    void logMemory(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::MEMORY, message, file, line, function);
    }
    
    void logEditor(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::EDITOR, message, file, line, function);
    }
    
    void logAI(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::AI, message, file, line, function);
    }
    
    void logGUI(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::GUI, message, file, line, function);
    }
    
    void logNetwork(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::NETWORK, message, file, line, function);
    }
    
    void logPlugin(LogLevel level, const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(level, LogCategory::PLUGIN, message, file, line, function);
    }
    
    // Configuration methods
    void setLevel(LogLevel level) { level_ = level; }
    LogLevel getLevel() const { return level_; }
    
    void setDefaultCategory(LogCategory category) { defaultCategory_ = category; }
    LogCategory getDefaultCategory() const { return defaultCategory_; }
    
    void setEnabled(bool enabled) { enabled_.write([enabled](bool& e) { e = enabled; }); }
    bool isEnabled() const { return enabled_.read([](const bool& e) { return e; }); }
    
    // Sink management
    void addSink(std::unique_ptr<LogSink> sink);
    void clearSinks();
    size_t getSinkCount() const;
    
    // Filter management
    void addFilter(std::unique_ptr<LogFilter> filter);
    void clearFilters();
    size_t getFilterCount() const;
    
    // Formatter management
    void setFormatter(std::unique_ptr<LogFormatter> formatter);
    
    // Performance metrics
    size_t getTotalMessages() const { return totalMessages_.load(); }
    size_t getDroppedMessages() const { return droppedMessages_.load(); }
    void resetMetrics() { totalMessages_.store(0); droppedMessages_.store(0); }
    
    // Utility methods
    void flush();
    static std::string levelToString(LogLevel level);
    static std::string categoryToString(LogCategory category);
    static LogLevel stringToLevel(const std::string& levelStr);
    static LogCategory stringToCategory(const std::string& categoryStr);
};

// Global logger instance manager
class LogManager {
private:
    static std::unique_ptr<Logger> globalLogger_;
    static std::mutex mutex_;
    
public:
    static Logger& getInstance();
    static void setGlobalLogger(std::unique_ptr<Logger> logger);
    static void reset();
    
    // Quick configuration methods
    static void configureConsoleLogging(LogLevel level = LogLevel::INFO, bool useDetailedFormat = true);
    static void configureFileLogging(const std::string& filename, LogLevel level = LogLevel::DEBUG, bool useDetailedFormat = true);
    static void configureRotatingFileLogging(const std::string& baseFilename, size_t maxFileSize, size_t maxFiles, LogLevel level = LogLevel::DEBUG);
    static void configureDualLogging(const std::string& filename, LogLevel consoleLevel = LogLevel::INFO, LogLevel fileLevel = LogLevel::DEBUG);
};

// Utility macros for easier logging with file/line information
#define BOLT_LOG(level, message) \
    bolt::LogManager::getInstance().log(level, message, __FILE__, __LINE__, __func__)

#define BOLT_LOG_CATEGORY(level, category, message) \
    bolt::LogManager::getInstance().log(level, category, message, __FILE__, __LINE__, __func__)

#define BOLT_TRACE(message) BOLT_LOG(bolt::LogLevel::TRACE, message)
#define BOLT_DEBUG(message) BOLT_LOG(bolt::LogLevel::DEBUG, message)
#define BOLT_INFO(message) BOLT_LOG(bolt::LogLevel::INFO, message)
#define BOLT_WARN(message) BOLT_LOG(bolt::LogLevel::WARN, message)
#define BOLT_ERROR(message) BOLT_LOG(bolt::LogLevel::ERROR, message)
#define BOLT_FATAL(message) BOLT_LOG(bolt::LogLevel::FATAL, message)

// Category-specific macros
#define BOLT_LOG_CORE(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::CORE, message)
#define BOLT_LOG_MEMORY(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::MEMORY, message)
#define BOLT_LOG_EDITOR(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::EDITOR, message)
#define BOLT_LOG_AI(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::AI, message)
#define BOLT_LOG_GUI(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::GUI, message)
#define BOLT_LOG_NETWORK(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::NETWORK, message)
#define BOLT_LOG_PLUGIN(level, message) BOLT_LOG_CATEGORY(level, bolt::LogCategory::PLUGIN, message)

// Scoped logging for tracking function entry/exit
class ScopedLogger {
private:
    std::string function_;
    LogCategory category_;
    std::chrono::steady_clock::time_point startTime_;
    
public:
    ScopedLogger(const std::string& function, LogCategory category = LogCategory::CORE);
    ~ScopedLogger();
};

#define BOLT_SCOPED_LOG(category) bolt::ScopedLogger _scopedLogger(__func__, category)
#define BOLT_SCOPED_LOG_CORE() BOLT_SCOPED_LOG(bolt::LogCategory::CORE)

} // namespace bolt

#endif // BOLT_LOGGING_HPP