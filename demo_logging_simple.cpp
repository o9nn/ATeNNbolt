#include "bolt/core/logging.hpp"
#include "bolt/core/error_handling.hpp"
#include <iostream>
#include <thread>
#include <chrono>

void demonstrateBasicLogging() {
    std::cout << "\n=== Basic Logging Demo ===" << std::endl;
    
    // Configure console logging with detailed format
    bolt::LogManager::configureConsoleLogging(bolt::LogLevel::DEBUG, true);
    
    // Test different log levels
    BOLT_TRACE("This is a trace message");
    BOLT_DEBUG("This is a debug message");
    BOLT_INFO("This is an info message");
    BOLT_WARN("This is a warning message");
    BOLT_ERROR("This is an error message");
    BOLT_FATAL("This is a fatal message");
}

void demonstrateCategoryLogging() {
    std::cout << "\n=== Category-Based Logging Demo ===" << std::endl;
    
    // Test category-specific logging
    BOLT_LOG_CORE(bolt::LogLevel::INFO, "Core system initialization");
    BOLT_LOG_MEMORY(bolt::LogLevel::DEBUG, "Memory allocation: 1024 bytes");
    BOLT_LOG_EDITOR(bolt::LogLevel::INFO, "File opened: example.cpp");
    BOLT_LOG_NETWORK(bolt::LogLevel::ERROR, "Connection timeout");
    BOLT_LOG_PLUGIN(bolt::LogLevel::INFO, "Plugin loaded: syntax-highlighter");
}

void demonstrateFileLogging() {
    std::cout << "\n=== File Logging Demo ===" << std::endl;
    
    // Configure file logging
    std::string logFile = "/tmp/bolt_demo.log";
    bolt::LogManager::configureFileLogging(logFile, bolt::LogLevel::TRACE, true);
    
    std::cout << "Logging to file: " << logFile << std::endl;
    
    BOLT_INFO("File logging is now active");
    BOLT_DEBUG("This message includes file and line information");
    BOLT_WARN("This is written to the log file");
    
    std::cout << "Check " << logFile << " for logged messages" << std::endl;
}

void demonstratePerformanceMetrics() {
    std::cout << "\n=== Performance Metrics Demo ===" << std::endl;
    
    bolt::LogManager::configureConsoleLogging(bolt::LogLevel::WARN, false); // Simple format
    bolt::Logger& logger = bolt::LogManager::getInstance();
    
    logger.resetMetrics();
    
    std::cout << "Generating 1000 log messages..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        if (i % 4 == 0) {
            logger.debug("Debug message " + std::to_string(i)); // Will be filtered
        } else if (i % 4 == 1) {
            logger.info("Info message " + std::to_string(i)); // Will be filtered
        } else if (i % 4 == 2) {
            logger.warn("Warning message " + std::to_string(i)); // Will be logged
        } else {
            logger.error("Error message " + std::to_string(i)); // Will be logged
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Performance metrics:" << std::endl;
    std::cout << "- Total messages: " << logger.getTotalMessages() << std::endl;
    std::cout << "- Dropped messages: " << logger.getDroppedMessages() << std::endl;
    std::cout << "- Logged messages: " << (logger.getTotalMessages() - logger.getDroppedMessages()) << std::endl;
    std::cout << "- Time taken: " << duration.count() << " microseconds" << std::endl;
    std::cout << "- Average per message: " << (duration.count() / 1000.0) << " microseconds" << std::endl;
}

void demonstrateErrorIntegration() {
    std::cout << "\n=== Error Handling Integration Demo ===" << std::endl;
    
    bolt::LogManager::configureConsoleLogging(bolt::LogLevel::DEBUG, true);
    
    try {
        BOLT_INFO("Attempting risky operation...");
        
        // Simulate different types of errors
        BOLT_WARN("Warning: Operation may fail");
        
        throw bolt::MemoryException(bolt::ErrorCode::MEMORY_ALLOCATION_FAILED, 
                                   "Failed to allocate 1GB of memory");
        
    } catch (const bolt::MemoryException& e) {
        BOLT_ERROR("Memory exception caught: " + std::string(e.what()));
        BOLT_DEBUG("Error code: " + std::to_string(static_cast<int>(e.getErrorCode())));
    } catch (const bolt::BoltException& e) {
        BOLT_ERROR("General Bolt exception: " + std::string(e.what()));
    } catch (const std::exception& e) {
        BOLT_FATAL("Unexpected exception: " + std::string(e.what()));
    }
    
    BOLT_INFO("Error handling demonstration completed");
}

int main() {
    std::cout << "Bolt Comprehensive Logging System Demo" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    try {
        demonstrateBasicLogging();
        demonstrateCategoryLogging();
        demonstrateFileLogging();
        demonstratePerformanceMetrics();
        demonstrateErrorIntegration();
        
        std::cout << "\n=== Demo Complete ===" << std::endl;
        std::cout << "All logging features demonstrated successfully!" << std::endl;
        std::cout << "Check the generated log files in /tmp/ for examples of different formats." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}