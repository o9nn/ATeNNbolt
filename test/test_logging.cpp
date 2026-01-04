#include "bolt/test_framework.hpp"
#include "bolt/core/logging.hpp"
#include "bolt/core/error_handling.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <thread>
#include <chrono>

// ===== Basic Logging Tests =====

BOLT_TEST(Logging, BasicLogEntry) {
    bolt::LogEntry entry(bolt::LogLevel::INFO, bolt::LogCategory::CORE, "Test message");
    
    BOLT_ASSERT_TRUE(entry.level == bolt::LogLevel::INFO);
    BOLT_ASSERT_TRUE(entry.category == bolt::LogCategory::CORE);
    BOLT_ASSERT_TRUE(entry.message == "Test message");
    BOLT_ASSERT_TRUE(!entry.file.empty() == false); // file should be empty in this case
    BOLT_ASSERT_TRUE(entry.line == 0);
}

BOLT_TEST(Logging, LogEntryWithLocation) {
    bolt::LogEntry entry(bolt::LogLevel::ERROR, bolt::LogCategory::MEMORY, "Memory error", 
                        "test.cpp", 42, "testFunction");
    
    BOLT_ASSERT_TRUE(entry.level == bolt::LogLevel::ERROR);
    BOLT_ASSERT_TRUE(entry.category == bolt::LogCategory::MEMORY);
    BOLT_ASSERT_TRUE(entry.message == "Memory error");
    BOLT_ASSERT_TRUE(entry.file == "test.cpp");
    BOLT_ASSERT_TRUE(entry.line == 42);
    BOLT_ASSERT_TRUE(entry.function == "testFunction");
}

// ===== Formatter Tests =====

BOLT_TEST(Logging, SimpleFormatter) {
    bolt::SimpleFormatter formatter;
    bolt::LogEntry entry(bolt::LogLevel::WARN, bolt::LogCategory::EDITOR, "Warning message");
    
    std::string formatted = formatter.format(entry);
    
    BOLT_ASSERT_TRUE(formatted.find("WARN") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("EDITOR") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("Warning message") != std::string::npos);
}

BOLT_TEST(Logging, DetailedFormatter) {
    bolt::DetailedFormatter formatter(true, true, true);
    bolt::LogEntry entry(bolt::LogLevel::DEBUG, bolt::LogCategory::AI, "Debug message", 
                        "ai_model.cpp", 123, "processData");
    
    std::string formatted = formatter.format(entry);
    
    BOLT_ASSERT_TRUE(formatted.find("DEBUG") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("AI") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("Debug message") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("ai_model.cpp") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("123") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("processData") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("T:") != std::string::npos); // Thread ID
}

BOLT_TEST(Logging, JsonFormatter) {
    bolt::JsonFormatter formatter;
    bolt::LogEntry entry(bolt::LogLevel::INFO, bolt::LogCategory::NETWORK, "Network status", 
                        "network.cpp", 56, "checkConnection");
    
    std::string formatted = formatter.format(entry);
    
    BOLT_ASSERT_TRUE(formatted.find("\"level\":\"INFO\"") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("\"category\":\"NETWORK\"") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("\"message\":\"Network status\"") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("\"file\":\"network.cpp\"") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("\"line\":56") != std::string::npos);
    BOLT_ASSERT_TRUE(formatted.find("\"function\":\"checkConnection\"") != std::string::npos);
}

// ===== Sink Tests =====

BOLT_TEST(Logging, ConsoleSink) {
    bolt::ConsoleSink sink;
    bolt::LogEntry entry(bolt::LogLevel::INFO, bolt::LogCategory::CORE, "Console test");
    
    BOLT_ASSERT_TRUE(sink.isOpen());
    
    // This test mainly verifies that write doesn't crash
    sink.write(entry, "Test message");
    sink.flush();
}

BOLT_TEST(Logging, FileSink) {
    std::string testFile = "/tmp/bolt_test_log.txt";
    
    // Clean up any existing file
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    {
        bolt::FileSink sink(testFile);
        bolt::LogEntry entry(bolt::LogLevel::ERROR, bolt::LogCategory::TESTING, "File test");
        
        BOLT_ASSERT_TRUE(sink.isOpen());
        BOLT_ASSERT_TRUE(sink.getFilename() == testFile);
        
        sink.write(entry, "Test message for file");
        sink.flush();
    } // FileSink destructor should close file
    
    // Verify file was created and contains content
    BOLT_ASSERT_TRUE(std::filesystem::exists(testFile));
    
    std::ifstream file(testFile);
    std::string content;
    std::getline(file, content);
    BOLT_ASSERT_TRUE(content == "Test message for file");
    
    // Clean up
    std::filesystem::remove(testFile);
}

BOLT_TEST(Logging, RotatingFileSink) {
    std::string baseFile = "/tmp/bolt_rotating_test.log";
    
    // Clean up any existing files
    for (int i = 0; i <= 5; ++i) {
        std::string file = baseFile + (i > 0 ? "." + std::to_string(i) : "");
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
        }
    }
    
    {
        // Create sink with small file size to trigger rotation
        bolt::RotatingFileSink sink(baseFile, 50, 3); // 50 bytes max, 3 files
        
        BOLT_ASSERT_TRUE(sink.isOpen());
        
        // Write several messages to trigger rotation
        for (int i = 0; i < 5; ++i) {
            bolt::LogEntry entry(bolt::LogLevel::INFO, bolt::LogCategory::TESTING, 
                               "Rotating test message " + std::to_string(i));
            std::string message = "This is a test message number " + std::to_string(i) + 
                                " that should be long enough to trigger rotation";
            sink.write(entry, message);
        }
        
        sink.flush();
    }
    
    // Verify that files were created
    BOLT_ASSERT_TRUE(std::filesystem::exists(baseFile));
    
    // Clean up
    for (int i = 0; i <= 5; ++i) {
        std::string file = baseFile + (i > 0 ? "." + std::to_string(i) : "");
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
        }
    }
}

// ===== Filter Tests =====

BOLT_TEST(Logging, LevelFilter) {
    bolt::LevelFilter filter(bolt::LogLevel::WARN);
    
    bolt::LogEntry debugEntry(bolt::LogLevel::DEBUG, bolt::LogCategory::CORE, "Debug");
    bolt::LogEntry infoEntry(bolt::LogLevel::INFO, bolt::LogCategory::CORE, "Info");
    bolt::LogEntry warnEntry(bolt::LogLevel::WARN, bolt::LogCategory::CORE, "Warn");
    bolt::LogEntry errorEntry(bolt::LogLevel::ERROR, bolt::LogCategory::CORE, "Error");
    
    BOLT_ASSERT_FALSE(filter.shouldLog(debugEntry));
    BOLT_ASSERT_FALSE(filter.shouldLog(infoEntry));
    BOLT_ASSERT_TRUE(filter.shouldLog(warnEntry));
    BOLT_ASSERT_TRUE(filter.shouldLog(errorEntry));
    
    // Test level change
    filter.setMinLevel(bolt::LogLevel::DEBUG);
    BOLT_ASSERT_TRUE(filter.shouldLog(debugEntry));
    BOLT_ASSERT_TRUE(filter.getMinLevel() == bolt::LogLevel::DEBUG);
}

BOLT_TEST(Logging, CategoryFilter) {
    bolt::CategoryFilter filter(bolt::LogLevel::INFO); // Default level
    
    // Set specific levels for categories
    filter.setCategoryLevel(bolt::LogCategory::MEMORY, bolt::LogLevel::DEBUG);
    filter.setCategoryLevel(bolt::LogCategory::EDITOR, bolt::LogLevel::ERROR);
    
    bolt::LogEntry memoryDebug(bolt::LogLevel::DEBUG, bolt::LogCategory::MEMORY, "Memory debug");
    bolt::LogEntry memoryInfo(bolt::LogLevel::INFO, bolt::LogCategory::MEMORY, "Memory info");
    bolt::LogEntry editorInfo(bolt::LogLevel::INFO, bolt::LogCategory::EDITOR, "Editor info");
    bolt::LogEntry editorError(bolt::LogLevel::ERROR, bolt::LogCategory::EDITOR, "Editor error");
    bolt::LogEntry coreInfo(bolt::LogLevel::INFO, bolt::LogCategory::CORE, "Core info");
    bolt::LogEntry coreDebug(bolt::LogLevel::DEBUG, bolt::LogCategory::CORE, "Core debug");
    
    BOLT_ASSERT_TRUE(filter.shouldLog(memoryDebug));   // DEBUG >= DEBUG for MEMORY
    BOLT_ASSERT_TRUE(filter.shouldLog(memoryInfo));    // INFO >= DEBUG for MEMORY
    BOLT_ASSERT_FALSE(filter.shouldLog(editorInfo));   // INFO < ERROR for EDITOR
    BOLT_ASSERT_TRUE(filter.shouldLog(editorError));   // ERROR >= ERROR for EDITOR
    BOLT_ASSERT_TRUE(filter.shouldLog(coreInfo));      // INFO >= INFO (default)
    BOLT_ASSERT_FALSE(filter.shouldLog(coreDebug));    // DEBUG < INFO (default)
}

// ===== Logger Tests =====

BOLT_TEST(Logging, BasicLogger) {
    bolt::Logger logger(bolt::LogLevel::DEBUG, bolt::LogCategory::TESTING);
    
    BOLT_ASSERT_TRUE(logger.getLevel() == bolt::LogLevel::DEBUG);
    BOLT_ASSERT_TRUE(logger.getDefaultCategory() == bolt::LogCategory::TESTING);
    BOLT_ASSERT_TRUE(logger.isEnabled());
    BOLT_ASSERT_TRUE(logger.getSinkCount() == 0);
    BOLT_ASSERT_TRUE(logger.getFilterCount() == 0);
}

BOLT_TEST(Logging, LoggerWithSinks) {
    bolt::Logger logger;
    
    // Add test file sink
    std::string testFile = "/tmp/bolt_logger_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    logger.addSink(std::make_unique<bolt::FileSink>(testFile));
    BOLT_ASSERT_TRUE(logger.getSinkCount() == 1);
    
    // Test logging
    logger.info("Test info message");
    logger.warn("Test warning message");
    logger.error("Test error message");
    
    logger.flush();
    
    // Verify file contains messages
    BOLT_ASSERT_TRUE(std::filesystem::exists(testFile));
    
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        BOLT_ASSERT_TRUE(line.find("Test") != std::string::npos);
    }
    BOLT_ASSERT_TRUE(lineCount == 3);
    
    // Clean up
    std::filesystem::remove(testFile);
}

BOLT_TEST(Logging, LoggerLevelFiltering) {
    bolt::Logger logger(bolt::LogLevel::WARN);
    
    std::string testFile = "/tmp/bolt_level_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    logger.addSink(std::make_unique<bolt::FileSink>(testFile));
    
    // These should be filtered out
    logger.trace("Trace message");
    logger.debug("Debug message");
    logger.info("Info message");
    
    // These should be logged
    logger.warn("Warning message");
    logger.error("Error message");
    logger.fatal("Fatal message");
    
    logger.flush();
    
    // Count lines in file
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
    }
    BOLT_ASSERT_TRUE(lineCount == 3); // Only WARN, ERROR, FATAL
    
    // Clean up
    std::filesystem::remove(testFile);
}

BOLT_TEST(Logging, LoggerWithFilters) {
    bolt::Logger logger(bolt::LogLevel::TRACE); // Allow all levels
    
    std::string testFile = "/tmp/bolt_filter_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    logger.addSink(std::make_unique<bolt::FileSink>(testFile));
    logger.addFilter(std::make_unique<bolt::LevelFilter>(bolt::LogLevel::ERROR));
    
    BOLT_ASSERT_TRUE(logger.getFilterCount() == 1);
    
    // Only ERROR and FATAL should pass through filter
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warn("Warning message");
    logger.error("Error message");
    logger.fatal("Fatal message");
    
    logger.flush();
    
    // Count lines in file
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
    }
    BOLT_ASSERT_TRUE(lineCount == 2); // Only ERROR, FATAL
    
    // Clean up
    std::filesystem::remove(testFile);
}

BOLT_TEST(Logging, LoggerCategoryLogging) {
    bolt::Logger logger;
    
    std::string testFile = "/tmp/bolt_category_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    logger.addSink(std::make_unique<bolt::FileSink>(testFile));
    
    // Test category-specific logging methods
    logger.logCore(bolt::LogLevel::INFO, "Core message");
    logger.logMemory(bolt::LogLevel::WARN, "Memory message");
    logger.logEditor(bolt::LogLevel::ERROR, "Editor message");
    logger.logAI(bolt::LogLevel::DEBUG, "AI message");
    
    logger.flush();
    
    // Verify messages contain correct categories
    std::ifstream file(testFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    BOLT_ASSERT_TRUE(content.find("CORE") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("MEMORY") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("EDITOR") != std::string::npos);
    // AI message might be filtered out due to DEBUG level
    
    // Clean up
    std::filesystem::remove(testFile);
}

BOLT_TEST(Logging, LoggerMetrics) {
    bolt::Logger logger(bolt::LogLevel::WARN); // High threshold to test dropped messages
    
    logger.resetMetrics();
    BOLT_ASSERT_TRUE(logger.getTotalMessages() == 0);
    BOLT_ASSERT_TRUE(logger.getDroppedMessages() == 0);
    
    // Log messages at different levels
    logger.debug("Debug 1");   // Should be dropped
    logger.info("Info 1");     // Should be dropped
    logger.warn("Warning 1");  // Should be logged
    logger.error("Error 1");   // Should be logged
    logger.debug("Debug 2");   // Should be dropped
    
    BOLT_ASSERT_TRUE(logger.getTotalMessages() == 5);
    BOLT_ASSERT_TRUE(logger.getDroppedMessages() == 3);
}

BOLT_TEST(Logging, LoggerEnableDisable) {
    bolt::Logger logger;
    
    std::string testFile = "/tmp/bolt_enable_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    logger.addSink(std::make_unique<bolt::FileSink>(testFile));
    
    // Log with enabled logger
    logger.info("Message 1");
    
    // Disable and try to log
    logger.setEnabled(false);
    BOLT_ASSERT_FALSE(logger.isEnabled());
    logger.info("Message 2"); // Should be dropped
    
    // Re-enable and log
    logger.setEnabled(true);
    BOLT_ASSERT_TRUE(logger.isEnabled());
    logger.info("Message 3");
    
    logger.flush();
    
    // Verify only 2 messages were logged
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
    }
    BOLT_ASSERT_TRUE(lineCount == 2);
    
    // Clean up
    std::filesystem::remove(testFile);
}

// ===== Utility Function Tests =====

BOLT_TEST(Logging, LevelStringConversion) {
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::TRACE) == "TRACE");
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::DEBUG) == "DEBUG");
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::INFO) == "INFO");
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::WARN) == "WARN");
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::ERROR) == "ERROR");
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::FATAL) == "FATAL");
    BOLT_ASSERT_TRUE(bolt::Logger::levelToString(bolt::LogLevel::OFF) == "OFF");
    
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("TRACE") == bolt::LogLevel::TRACE);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("debug") == bolt::LogLevel::DEBUG);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("Info") == bolt::LogLevel::INFO);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("WARN") == bolt::LogLevel::WARN);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("WARNING") == bolt::LogLevel::WARN);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("ERROR") == bolt::LogLevel::ERROR);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("FATAL") == bolt::LogLevel::FATAL);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("OFF") == bolt::LogLevel::OFF);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToLevel("UNKNOWN") == bolt::LogLevel::INFO); // Default
}

BOLT_TEST(Logging, CategoryStringConversion) {
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::CORE) == "CORE");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::MEMORY) == "MEMORY");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::EDITOR) == "EDITOR");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::AI) == "AI");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::GUI) == "GUI");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::NETWORK) == "NETWORK");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::PLUGIN) == "PLUGIN");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::COLLABORATION) == "COLLABORATION");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::DEBUGGER) == "DEBUGGER");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::TESTING) == "TESTING");
    BOLT_ASSERT_TRUE(bolt::Logger::categoryToString(bolt::LogCategory::UNKNOWN) == "UNKNOWN");
    
    BOLT_ASSERT_TRUE(bolt::Logger::stringToCategory("CORE") == bolt::LogCategory::CORE);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToCategory("memory") == bolt::LogCategory::MEMORY);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToCategory("Editor") == bolt::LogCategory::EDITOR);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToCategory("AI") == bolt::LogCategory::AI);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToCategory("gui") == bolt::LogCategory::GUI);
    BOLT_ASSERT_TRUE(bolt::Logger::stringToCategory("INVALID") == bolt::LogCategory::UNKNOWN);
}

// ===== LogManager Tests =====

BOLT_TEST(Logging, LogManagerSingleton) {
    // Reset to clean state
    bolt::LogManager::reset();
    
    // Get instance should create default logger
    bolt::Logger& logger1 = bolt::LogManager::getInstance();
    bolt::Logger& logger2 = bolt::LogManager::getInstance();
    
    // Should be same instance
    BOLT_ASSERT_TRUE(&logger1 == &logger2);
}

BOLT_TEST(Logging, LogManagerConfiguration) {
    std::string testFile = "/tmp/bolt_manager_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    // Configure file logging
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::DEBUG, true);
    
    // Use global logger
    bolt::Logger& logger = bolt::LogManager::getInstance();
    logger.info("Manager test message");
    logger.flush();
    
    // Verify file was created
    BOLT_ASSERT_TRUE(std::filesystem::exists(testFile));
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
}

// ===== Macro Tests =====

BOLT_TEST(Logging, LoggingMacros) {
    std::string testFile = "/tmp/bolt_macro_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::TRACE, true);
    
    // Test convenience macros
    BOLT_TRACE("Trace message");
    BOLT_DEBUG("Debug message");
    BOLT_INFO("Info message");
    BOLT_WARN("Warning message");
    BOLT_ERROR("Error message");
    BOLT_FATAL("Fatal message");
    
    bolt::LogManager::getInstance().flush();
    
    // Verify all messages were logged
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        // Verify file/line information is included
        BOLT_ASSERT_TRUE(line.find("test_logging.cpp") != std::string::npos);
    }
    BOLT_ASSERT_TRUE(lineCount == 6);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
}

BOLT_TEST(Logging, CategoryMacros) {
    std::string testFile = "/tmp/bolt_category_macro_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::DEBUG, true);
    
    // Test category-specific macros
    BOLT_LOG_CORE(bolt::LogLevel::INFO, "Core message");
    BOLT_LOG_MEMORY(bolt::LogLevel::WARN, "Memory message");
    BOLT_LOG_EDITOR(bolt::LogLevel::ERROR, "Editor message");
    BOLT_LOG_AI(bolt::LogLevel::DEBUG, "AI message");
    
    bolt::LogManager::getInstance().flush();
    
    // Verify messages contain correct categories
    std::ifstream file(testFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    BOLT_ASSERT_TRUE(content.find("CORE") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("MEMORY") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("EDITOR") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("AI") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
}

// ===== Thread Safety Tests =====

BOLT_TEST(Logging, ThreadSafety) {
    std::string testFile = "/tmp/bolt_thread_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::INFO, true);
    
    const int numThreads = 4;
    const int messagesPerThread = 10;
    std::vector<std::thread> threads;
    
    // Launch multiple threads that log simultaneously
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                std::string message = "Thread " + std::to_string(t) + " message " + std::to_string(i);
                BOLT_INFO(message);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    bolt::LogManager::getInstance().flush();
    
    // Verify all messages were logged
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
    }
    BOLT_ASSERT_TRUE(lineCount == numThreads * messagesPerThread);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
}

// ===== Scoped Logger Tests =====

BOLT_TEST(Logging, ScopedLogger) {
    std::string testFile = "/tmp/bolt_scoped_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::TRACE, true);
    
    {
        BOLT_SCOPED_LOG_CORE();
        
        // Do some work
        BOLT_INFO("Inside scoped function");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    bolt::LogManager::getInstance().flush();
    
    // Verify entry and exit messages
    std::ifstream file(testFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    BOLT_ASSERT_TRUE(content.find("Entering") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("Exiting") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("Inside scoped function") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
}

// ===== Performance Tests =====

BOLT_TEST(Logging, PerformanceMetrics) {
    bolt::Logger logger(bolt::LogLevel::INFO);
    
    logger.resetMetrics();
    
    const int numMessages = 1000;
    
    // Log many messages
    for (int i = 0; i < numMessages; ++i) {
        if (i % 2 == 0) {
            logger.info("Info message " + std::to_string(i));
        } else {
            logger.debug("Debug message " + std::to_string(i)); // Should be dropped
        }
    }
    
    BOLT_ASSERT_TRUE(logger.getTotalMessages() == numMessages);
    BOLT_ASSERT_TRUE(logger.getDroppedMessages() == numMessages / 2); // Half are DEBUG
}

// ===== Integration Tests =====

BOLT_TEST(Logging, ErrorHandlingIntegration) {
    std::string testFile = "/tmp/bolt_error_integration_test.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::DEBUG, true);
    
    try {
        // Simulate an error scenario
        BOLT_ERROR("About to throw exception");
        throw bolt::BoltException(bolt::ErrorCode::INVALID_PARAMETER, "Test exception");
    } catch (const bolt::BoltException& e) {
        BOLT_ERROR("Caught exception: " + std::string(e.what()));
        BOLT_DEBUG("Exception code: " + std::to_string(static_cast<int>(e.getErrorCode())));
    }
    
    bolt::LogManager::getInstance().flush();
    
    // Verify error messages were logged
    std::ifstream file(testFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    BOLT_ASSERT_TRUE(content.find("About to throw exception") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("Caught exception") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("Test exception") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
}