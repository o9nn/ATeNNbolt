#include "bolt/core/logging.hpp"
#include "bolt/core/error_handling.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

void test_basic_logging() {
    std::cout << "Testing basic logging..." << std::endl;
    
    // Test log entry creation
    bolt::LogEntry entry(bolt::LogLevel::INFO, bolt::LogCategory::CORE, "Test message");
    assert(entry.level == bolt::LogLevel::INFO);
    assert(entry.category == bolt::LogCategory::CORE);
    assert(entry.message == "Test message");
    
    // Test formatters
    bolt::SimpleFormatter simpleFormatter;
    std::string formatted = simpleFormatter.format(entry);
    assert(formatted.find("INFO") != std::string::npos);
    assert(formatted.find("CORE") != std::string::npos);
    assert(formatted.find("Test message") != std::string::npos);
    
    bolt::DetailedFormatter detailedFormatter;
    std::string detailed = detailedFormatter.format(entry);
    assert(detailed.find("INFO") != std::string::npos);
    assert(detailed.find("CORE") != std::string::npos);
    
    bolt::JsonFormatter jsonFormatter;
    std::string json = jsonFormatter.format(entry);
    assert(json.find("\"level\":\"INFO\"") != std::string::npos);
    assert(json.find("\"category\":\"CORE\"") != std::string::npos);
    
    std::cout << "✓ Basic logging tests passed" << std::endl;
}

void test_file_logging() {
    std::cout << "Testing file logging..." << std::endl;
    
    std::string testFile = "/tmp/test_bolt_logging.log";
    
    // Clean up any existing file
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    // Test file sink
    {
        bolt::FileSink sink(testFile);
        assert(sink.isOpen());
        
        bolt::LogEntry entry(bolt::LogLevel::INFO, bolt::LogCategory::TESTING, "File test");
        sink.write(entry, "Test message for file");
        sink.flush();
    }
    
    // Verify file was created and contains content
    assert(std::filesystem::exists(testFile));
    
    std::ifstream file(testFile);
    std::string content;
    std::getline(file, content);
    assert(content == "Test message for file");
    
    // Clean up
    std::filesystem::remove(testFile);
    
    std::cout << "✓ File logging tests passed" << std::endl;
}

void test_logger_functionality() {
    std::cout << "Testing logger functionality..." << std::endl;
    
    bolt::Logger logger(bolt::LogLevel::DEBUG, bolt::LogCategory::TESTING);
    
    assert(logger.getLevel() == bolt::LogLevel::DEBUG);
    assert(logger.getDefaultCategory() == bolt::LogCategory::TESTING);
    assert(logger.isEnabled());
    assert(logger.getSinkCount() == 0);
    
    // Add a file sink
    std::string testFile = "/tmp/test_logger.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    logger.addSink(std::make_unique<bolt::FileSink>(testFile));
    assert(logger.getSinkCount() == 1);
    
    // Test logging at different levels
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warn("Warning message");
    logger.error("Error message");
    
    logger.flush();
    
    // Verify messages were logged
    assert(std::filesystem::exists(testFile));
    
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        assert(line.find("message") != std::string::npos);
    }
    assert(lineCount == 4);
    
    // Clean up
    std::filesystem::remove(testFile);
    
    std::cout << "✓ Logger functionality tests passed" << std::endl;
}

void test_level_filtering() {
    std::cout << "Testing level filtering..." << std::endl;
    
    bolt::Logger logger(bolt::LogLevel::WARN);
    
    std::string testFile = "/tmp/test_filter.log";
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
    
    logger.flush();
    
    // Count lines in file
    std::ifstream file(testFile);
    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
    }
    assert(lineCount == 2); // Only WARN and ERROR
    
    // Clean up
    std::filesystem::remove(testFile);
    
    std::cout << "✓ Level filtering tests passed" << std::endl;
}

void test_global_logger() {
    std::cout << "Testing global logger..." << std::endl;
    
    // Reset global logger
    bolt::LogManager::reset();
    
    // Configure console logging
    bolt::LogManager::configureConsoleLogging(bolt::LogLevel::INFO, false);
    
    // Get global logger instance
    bolt::Logger& logger = bolt::LogManager::getInstance();
    
    // Test basic logging through global instance
    logger.info("Global logger test");
    
    // Test macros
    std::string testFile = "/tmp/test_global.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::DEBUG, true);
    
    BOLT_INFO("Macro test message");
    BOLT_WARN("Warning via macro");
    BOLT_ERROR("Error via macro");
    
    bolt::LogManager::getInstance().flush();
    
    // Verify messages were logged
    assert(std::filesystem::exists(testFile));
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
    
    std::cout << "✓ Global logger tests passed" << std::endl;
}

void test_category_logging() {
    std::cout << "Testing category logging..." << std::endl;
    
    std::string testFile = "/tmp/test_categories.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::DEBUG, true);
    
    // Test category-specific logging
    BOLT_LOG_CORE(bolt::LogLevel::INFO, "Core message");
    BOLT_LOG_MEMORY(bolt::LogLevel::WARN, "Memory message");
    BOLT_LOG_EDITOR(bolt::LogLevel::ERROR, "Editor message");
    
    bolt::LogManager::getInstance().flush();
    
    // Verify categories are in file
    std::ifstream file(testFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    assert(content.find("CORE") != std::string::npos);
    assert(content.find("MEMORY") != std::string::npos);
    assert(content.find("EDITOR") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
    
    std::cout << "✓ Category logging tests passed" << std::endl;
}

void test_string_conversion() {
    std::cout << "Testing string conversions..." << std::endl;
    
    // Test level to string
    assert(bolt::Logger::levelToString(bolt::LogLevel::DEBUG) == "DEBUG");
    assert(bolt::Logger::levelToString(bolt::LogLevel::INFO) == "INFO");
    assert(bolt::Logger::levelToString(bolt::LogLevel::WARN) == "WARN");
    assert(bolt::Logger::levelToString(bolt::LogLevel::ERROR) == "ERROR");
    
    // Test string to level
    assert(bolt::Logger::stringToLevel("DEBUG") == bolt::LogLevel::DEBUG);
    assert(bolt::Logger::stringToLevel("info") == bolt::LogLevel::INFO);
    assert(bolt::Logger::stringToLevel("WARN") == bolt::LogLevel::WARN);
    assert(bolt::Logger::stringToLevel("error") == bolt::LogLevel::ERROR);
    
    // Test category to string
    assert(bolt::Logger::categoryToString(bolt::LogCategory::CORE) == "CORE");
    assert(bolt::Logger::categoryToString(bolt::LogCategory::MEMORY) == "MEMORY");
    assert(bolt::Logger::categoryToString(bolt::LogCategory::EDITOR) == "EDITOR");
    
    // Test string to category
    assert(bolt::Logger::stringToCategory("CORE") == bolt::LogCategory::CORE);
    assert(bolt::Logger::stringToCategory("memory") == bolt::LogCategory::MEMORY);
    assert(bolt::Logger::stringToCategory("Editor") == bolt::LogCategory::EDITOR);
    
    std::cout << "✓ String conversion tests passed" << std::endl;
}

void test_error_integration() {
    std::cout << "Testing error handling integration..." << std::endl;
    
    std::string testFile = "/tmp/test_error_integration.log";
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    bolt::LogManager::configureFileLogging(testFile, bolt::LogLevel::DEBUG, true);
    
    try {
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
    
    assert(content.find("About to throw exception") != std::string::npos);
    assert(content.find("Caught exception") != std::string::npos);
    assert(content.find("Test exception") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(testFile);
    bolt::LogManager::reset();
    
    std::cout << "✓ Error integration tests passed" << std::endl;
}

int main() {
    std::cout << "Running Bolt Logging System Tests" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        test_basic_logging();
        test_file_logging();
        test_logger_functionality();
        test_level_filtering();
        test_global_logger();
        test_category_logging();
        test_string_conversion();
        test_error_integration();
        
        std::cout << "\n🎉 All logging tests passed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}