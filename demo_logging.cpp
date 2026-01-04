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
    BOLT_LOG_AI(bolt::LogLevel::WARN, "Model loading took longer than expected");
    BOLT_LOG_GUI(bolt::LogLevel::INFO, "Window resized to 1920x1080");
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
    
    // Demonstrate JSON logging
    auto logger = std::make_unique<bolt::Logger>(bolt::LogLevel::DEBUG);
    logger->addSink(std::make_unique<bolt::FileSink>("/tmp/bolt_demo.json"));
    logger->setFormatter(std::make_unique<bolt::JsonFormatter>());
    
    logger->info("JSON formatted message");
    logger->error("Error in JSON format", __FILE__, __LINE__, __func__);
    
    bolt::LogManager::setGlobalLogger(std::move(logger));
    
    std::cout << "Check " << logFile << " and /tmp/bolt_demo.json for logged messages" << std::endl;
}

void demonstrateRotatingLogs() {
    std::cout << "\n=== Rotating File Logging Demo ===" << std::endl;
    
    // Configure rotating file logging (small files for demo)
    bolt::LogManager::configureRotatingFileLogging("/tmp/bolt_rotating.log", 100, 3);
    
    std::cout << "Generating messages to trigger log rotation..." << std::endl;
    
    for (int i = 0; i < 20; ++i) {
        BOLT_INFO("Message number " + std::to_string(i) + " - this is a longer message to fill up the log file quickly");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "Check /tmp/bolt_rotating.log* files for rotation" << std::endl;
}

void demonstrateFiltering() {
    std::cout << "\n=== Log Filtering Demo ===" << std::endl;
    
    // Create a custom logger with filters
    auto logger = std::make_unique<bolt::Logger>(bolt::LogLevel::TRACE); // Allow all levels
    logger->addSink(std::make_unique<bolt::ConsoleSink>());
    
    // Add category filter
    auto categoryFilter = std::make_unique<bolt::CategoryFilter>(bolt::LogLevel::INFO); // Default level
    categoryFilter->setCategoryLevel(bolt::LogCategory::MEMORY, bolt::LogLevel::DEBUG); // Allow debug for memory
    categoryFilter->setCategoryLevel(bolt::LogCategory::EDITOR, bolt::LogLevel::ERROR); // Only errors for editor
    
    logger->addFilter(std::move(categoryFilter));
    bolt::LogManager::setGlobalLogger(std::move(logger));
    
    std::cout << "Demonstrating category-based filtering:" << std::endl;
    std::cout << "- MEMORY: DEBUG level and above" << std::endl;
    std::cout << "- EDITOR: ERROR level and above" << std::endl;
    std::cout << "- Others: INFO level and above" << std::endl;
    
    BOLT_LOG_MEMORY(bolt::LogLevel::DEBUG, "Memory debug message (should show)");
    BOLT_LOG_MEMORY(bolt::LogLevel::TRACE, "Memory trace message (should NOT show)");
    
    BOLT_LOG_EDITOR(bolt::LogLevel::INFO, "Editor info message (should NOT show)");
    BOLT_LOG_EDITOR(bolt::LogLevel::ERROR, "Editor error message (should show)");
    
    BOLT_LOG_CORE(bolt::LogLevel::INFO, "Core info message (should show)");
    BOLT_LOG_CORE(bolt::LogLevel::DEBUG, "Core debug message (should NOT show)");
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

void demonstrateScopedLogging() {
    std::cout << "\n=== Scoped Logging Demo ===" << std::endl;
    
    bolt::LogManager::configureConsoleLogging(bolt::LogLevel::TRACE, true);
    
    {
        BOLT_SCOPED_LOG_CORE();
        
        BOLT_INFO("Inside scoped function");
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        {
            BOLT_SCOPED_LOG(bolt::LogCategory::MEMORY);
            BOLT_DEBUG("Nested scope for memory operations");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        BOLT_INFO("Continuing work in outer scope");
    }
    
    BOLT_INFO("Outside of scoped function");
}

void demonstrateThreadSafety() {
    std::cout << "\n=== Thread Safety Demo ===" << std::endl;
    
    bolt::LogManager::configureFileLogging("/tmp/bolt_threaded.log", bolt::LogLevel::INFO, true);
    
    const int numThreads = 4;
    const int messagesPerThread = 5;
    std::vector<std::thread> threads;
    
    std::cout << "Launching " << numThreads << " threads, each logging " << messagesPerThread << " messages..." << std::endl;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                BOLT_INFO("Thread " + std::to_string(t) + " - Message " + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "All threads completed. Check /tmp/bolt_threaded.log for interleaved messages" << std::endl;
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
        demonstrateRotatingLogs();
        demonstrateFiltering();
        demonstratePerformanceMetrics();
        demonstrateScopedLogging();
        demonstrateThreadSafety();
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