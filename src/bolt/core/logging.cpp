#include "bolt/core/logging.hpp"
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace bolt {

// Helper function to extract just the filename from a full path
std::string extractFilename(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    return (pos != std::string::npos) ? filepath.substr(pos + 1) : filepath;
}

// Helper function to format timestamp
std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Helper function to format thread ID
std::string formatThreadId(const std::thread::id& threadId) {
    std::stringstream ss;
    ss << threadId;
    return ss.str();
}

// SimpleFormatter implementation
std::string SimpleFormatter::format(const LogEntry& entry) {
    std::stringstream ss;
    ss << "[" << Logger::levelToString(entry.level) << "] "
       << "[" << Logger::categoryToString(entry.category) << "] "
       << entry.message;
    return ss.str();
}

// DetailedFormatter implementation
std::string DetailedFormatter::format(const LogEntry& entry) {
    std::stringstream ss;
    
    if (includeTimestamp_) {
        ss << "[" << formatTimestamp(entry.timestamp) << "] ";
    }
    
    ss << "[" << Logger::levelToString(entry.level) << "] ";
    ss << "[" << Logger::categoryToString(entry.category) << "] ";
    
    if (includeThreadId_) {
        ss << "[T:" << formatThreadId(entry.threadId) << "] ";
    }
    
    if (includeLocation_ && !entry.file.empty()) {
        ss << "[" << extractFilename(entry.file) << ":" << entry.line;
        if (!entry.function.empty()) {
            ss << " in " << entry.function << "()";
        }
        ss << "] ";
    }
    
    ss << entry.message;
    return ss.str();
}

// JsonFormatter implementation
std::string JsonFormatter::format(const LogEntry& entry) {
    std::stringstream ss;
    ss << "{"
       << "\"timestamp\":\"" << formatTimestamp(entry.timestamp) << "\","
       << "\"level\":\"" << Logger::levelToString(entry.level) << "\","
       << "\"category\":\"" << Logger::categoryToString(entry.category) << "\","
       << "\"thread\":\"" << formatThreadId(entry.threadId) << "\","
       << "\"message\":\"" << entry.message << "\"";
    
    if (!entry.file.empty()) {
        ss << ",\"file\":\"" << extractFilename(entry.file) << "\""
           << ",\"line\":" << entry.line;
    }
    
    if (!entry.function.empty()) {
        ss << ",\"function\":\"" << entry.function << "\"";
    }
    
    ss << "}";
    return ss.str();
}

// ConsoleSink implementation
void ConsoleSink::write(const LogEntry& entry, const std::string& formattedMessage) {
    std::ostream& stream = useStderr_ ? std::cerr : std::cout;
    
    // Use different colors based on log level (if terminal supports it)
    if (entry.level >= LogLevel::ERROR) {
        stream << "\033[31m" << formattedMessage << "\033[0m" << std::endl; // Red
    } else if (entry.level == LogLevel::WARN) {
        stream << "\033[33m" << formattedMessage << "\033[0m" << std::endl; // Yellow
    } else if (entry.level == LogLevel::DEBUG) {
        stream << "\033[36m" << formattedMessage << "\033[0m" << std::endl; // Cyan
    } else {
        stream << formattedMessage << std::endl; // Default color
    }
}

void ConsoleSink::flush() {
    std::cout.flush();
    std::cerr.flush();
}

// FileSink implementation
FileSink::FileSink(const std::string& filename) : filename_(filename) {
    // Create directory if it doesn't exist
    std::filesystem::path filepath(filename);
    if (filepath.has_parent_path()) {
        std::filesystem::create_directories(filepath.parent_path());
    }
    
    file_.open(filename, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

FileSink::~FileSink() {
    if (file_.is_open()) {
        file_.close();
    }
}

void FileSink::write(const LogEntry& entry, const std::string& formattedMessage) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_ << formattedMessage << std::endl;
    }
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

bool FileSink::isOpen() const {
    return file_.is_open();
}

// RotatingFileSink implementation
RotatingFileSink::RotatingFileSink(const std::string& baseFilename, size_t maxFileSize, size_t maxFiles)
    : baseFilename_(baseFilename), maxFileSize_(maxFileSize), maxFiles_(maxFiles), currentFileSize_(0) {
    
    // Create directory if it doesn't exist
    std::filesystem::path filepath(baseFilename);
    if (filepath.has_parent_path()) {
        std::filesystem::create_directories(filepath.parent_path());
    }
    
    currentFile_ = std::make_unique<std::ofstream>(baseFilename, std::ios::app);
    if (!currentFile_->is_open()) {
        throw std::runtime_error("Failed to open rotating log file: " + baseFilename);
    }
    
    // Get current file size
    std::filesystem::path path(baseFilename);
    if (std::filesystem::exists(path)) {
        currentFileSize_ = std::filesystem::file_size(path);
    }
}

RotatingFileSink::~RotatingFileSink() {
    if (currentFile_ && currentFile_->is_open()) {
        currentFile_->close();
    }
}

void RotatingFileSink::rotateFile() {
    if (currentFile_ && currentFile_->is_open()) {
        currentFile_->close();
    }
    
    // Move existing files
    for (int i = maxFiles_ - 1; i >= 1; --i) {
        std::string oldFile = getRotatedFilename(i - 1);
        std::string newFile = getRotatedFilename(i);
        
        if (std::filesystem::exists(oldFile)) {
            std::filesystem::rename(oldFile, newFile);
        }
    }
    
    // Move current file to .1
    if (std::filesystem::exists(baseFilename_)) {
        std::filesystem::rename(baseFilename_, getRotatedFilename(1));
    }
    
    // Open new current file
    currentFile_ = std::make_unique<std::ofstream>(baseFilename_, std::ios::app);
    currentFileSize_ = 0;
}

std::string RotatingFileSink::getRotatedFilename(size_t index) {
    return baseFilename_ + "." + std::to_string(index);
}

void RotatingFileSink::write(const LogEntry& entry, const std::string& formattedMessage) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!currentFile_ || !currentFile_->is_open()) {
        return;
    }
    
    // Check if we need to rotate
    size_t messageSize = formattedMessage.length() + 1; // +1 for newline
    if (currentFileSize_ + messageSize > maxFileSize_) {
        rotateFile();
    }
    
    if (currentFile_ && currentFile_->is_open()) {
        *currentFile_ << formattedMessage << std::endl;
        currentFileSize_ += messageSize;
    }
}

void RotatingFileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentFile_ && currentFile_->is_open()) {
        currentFile_->flush();
    }
}

bool RotatingFileSink::isOpen() const {
    return currentFile_ && currentFile_->is_open();
}

// CategoryFilter implementation
bool CategoryFilter::shouldLog(const LogEntry& entry) {
    auto it = categoryLevels_.find(entry.category);
    LogLevel requiredLevel = (it != categoryLevels_.end()) ? it->second : defaultLevel_;
    return entry.level >= requiredLevel;
}

// Logger implementation
Logger::Logger(LogLevel level, LogCategory defaultCategory)
    : level_(level), defaultCategory_(defaultCategory), enabled_(true),
      totalMessages_(0), droppedMessages_(0) {
    
    // Set default formatter
    formatter_ = std::make_unique<DetailedFormatter>();
}

Logger::~Logger() {
    flush();
}

bool Logger::shouldLog(const LogEntry& entry) const {
    // Check if logging is enabled
    if (!enabled_.read([](const bool& e) { return e; })) {
        return false;
    }
    
    // Check level
    if (entry.level < level_) {
        return false;
    }
    
    // Check filters
    for (const auto& filter : filters_) {
        if (!filter->shouldLog(entry)) {
            return false;
        }
    }
    
    return true;
}

void Logger::writeToSinks(const LogEntry& entry) {
    if (sinks_.empty()) {
        return;
    }
    
    std::string formattedMessage = formatter_->format(entry);
    
    for (const auto& sink : sinks_) {
        try {
            if (sink->isOpen()) {
                sink->write(entry, formattedMessage);
            }
        } catch (const std::exception& e) {
            // Avoid infinite recursion by writing directly to stderr
            std::cerr << "Log sink error: " << e.what() << std::endl;
            droppedMessages_++;
        }
    }
}

void Logger::log(LogLevel level, LogCategory category, const std::string& message,
                 const std::string& file, int line, const std::string& function) {
    LogEntry entry(level, category, message, file, line, function);
    
    totalMessages_++;
    
    if (!shouldLog(entry)) {
        droppedMessages_++;
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    writeToSinks(entry);
}

void Logger::addSink(std::unique_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(std::move(sink));
}

void Logger::clearSinks() {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

size_t Logger::getSinkCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sinks_.size();
}

void Logger::addFilter(std::unique_ptr<LogFilter> filter) {
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.push_back(std::move(filter));
}

void Logger::clearFilters() {
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.clear();
}

size_t Logger::getFilterCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return filters_.size();
}

void Logger::setFormatter(std::unique_ptr<LogFormatter> formatter) {
    std::lock_guard<std::mutex> lock(mutex_);
    formatter_ = std::move(formatter);
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& sink : sinks_) {
        try {
            sink->flush();
        } catch (const std::exception& e) {
            std::cerr << "Error flushing log sink: " << e.what() << std::endl;
        }
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        case LogLevel::OFF: return "OFF";
        default: return "UNKNOWN";
    }
}

std::string Logger::categoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::CORE: return "CORE";
        case LogCategory::MEMORY: return "MEMORY";
        case LogCategory::EDITOR: return "EDITOR";
        case LogCategory::AI: return "AI";
        case LogCategory::GUI: return "GUI";
        case LogCategory::NETWORK: return "NETWORK";
        case LogCategory::PLUGIN: return "PLUGIN";
        case LogCategory::COLLABORATION: return "COLLABORATION";
        case LogCategory::DEBUGGER: return "DEBUGGER";
        case LogCategory::TESTING: return "TESTING";
        case LogCategory::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::stringToLevel(const std::string& levelStr) {
    std::string upper = levelStr;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "TRACE") return LogLevel::TRACE;
    if (upper == "DEBUG") return LogLevel::DEBUG;
    if (upper == "INFO") return LogLevel::INFO;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::WARN;
    if (upper == "ERROR") return LogLevel::ERROR;
    if (upper == "FATAL") return LogLevel::FATAL;
    if (upper == "OFF") return LogLevel::OFF;
    
    return LogLevel::INFO; // Default
}

LogCategory Logger::stringToCategory(const std::string& categoryStr) {
    std::string upper = categoryStr;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "CORE") return LogCategory::CORE;
    if (upper == "MEMORY") return LogCategory::MEMORY;
    if (upper == "EDITOR") return LogCategory::EDITOR;
    if (upper == "AI") return LogCategory::AI;
    if (upper == "GUI") return LogCategory::GUI;
    if (upper == "NETWORK") return LogCategory::NETWORK;
    if (upper == "PLUGIN") return LogCategory::PLUGIN;
    if (upper == "COLLABORATION") return LogCategory::COLLABORATION;
    if (upper == "DEBUGGER") return LogCategory::DEBUGGER;
    if (upper == "TESTING") return LogCategory::TESTING;
    
    return LogCategory::UNKNOWN;
}

// LogManager implementation
std::unique_ptr<Logger> LogManager::globalLogger_;
std::mutex LogManager::mutex_;

Logger& LogManager::getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!globalLogger_) {
        globalLogger_ = std::make_unique<Logger>();
        // Set up default console logging
        globalLogger_->addSink(std::make_unique<ConsoleSink>());
    }
    return *globalLogger_;
}

void LogManager::setGlobalLogger(std::unique_ptr<Logger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalLogger_ = std::move(logger);
}

void LogManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    globalLogger_.reset();
}

void LogManager::configureConsoleLogging(LogLevel level, bool useDetailedFormat) {
    auto logger = std::make_unique<Logger>(level);
    logger->addSink(std::make_unique<ConsoleSink>());
    
    if (useDetailedFormat) {
        logger->setFormatter(std::make_unique<DetailedFormatter>(true, true, false));
    } else {
        logger->setFormatter(std::make_unique<SimpleFormatter>());
    }
    
    setGlobalLogger(std::move(logger));
}

void LogManager::configureFileLogging(const std::string& filename, LogLevel level, bool useDetailedFormat) {
    auto logger = std::make_unique<Logger>(level);
    logger->addSink(std::make_unique<FileSink>(filename));
    
    if (useDetailedFormat) {
        logger->setFormatter(std::make_unique<DetailedFormatter>(true, true, true));
    } else {
        logger->setFormatter(std::make_unique<SimpleFormatter>());
    }
    
    setGlobalLogger(std::move(logger));
}

void LogManager::configureRotatingFileLogging(const std::string& baseFilename, size_t maxFileSize, size_t maxFiles, LogLevel level) {
    auto logger = std::make_unique<Logger>(level);
    logger->addSink(std::make_unique<RotatingFileSink>(baseFilename, maxFileSize, maxFiles));
    logger->setFormatter(std::make_unique<DetailedFormatter>(true, true, true));
    
    setGlobalLogger(std::move(logger));
}

void LogManager::configureDualLogging(const std::string& filename, LogLevel consoleLevel, LogLevel fileLevel) {
    auto logger = std::make_unique<Logger>(std::min(consoleLevel, fileLevel));
    
    // Add console sink with level filter
    logger->addSink(std::make_unique<ConsoleSink>());
    logger->addFilter(std::make_unique<LevelFilter>(consoleLevel));
    
    // Add file sink
    logger->addSink(std::make_unique<FileSink>(filename));
    
    // Use detailed formatter for file logging
    logger->setFormatter(std::make_unique<DetailedFormatter>(true, true, true));
    
    setGlobalLogger(std::move(logger));
}

// ScopedLogger implementation
ScopedLogger::ScopedLogger(const std::string& function, LogCategory category)
    : function_(function), category_(category), startTime_(std::chrono::steady_clock::now()) {
    
    LogManager::getInstance().log(LogLevel::TRACE, category_, "Entering " + function_);
}

ScopedLogger::~ScopedLogger() {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);
    
    std::stringstream ss;
    ss << "Exiting " << function_ << " (took " << duration.count() << "μs)";
    
    LogManager::getInstance().log(LogLevel::TRACE, category_, ss.str());
}

} // namespace bolt