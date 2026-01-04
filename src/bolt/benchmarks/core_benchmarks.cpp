#include "../../include/bolt/core/benchmark_suite.hpp"
#include "../../include/bolt/core/performance_profiler.hpp"
#include "../../include/bolt/core/memory_manager.hpp"
#include "../../include/bolt/core/memory_pool.hpp"
#include "../../include/bolt/core/logging.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <fstream>
#include <unordered_map>

using namespace bolt;

// Memory Management Benchmarks
BOLT_BENCHMARK_CONFIG(memory_allocation_basic, "CORE", 
    "Basic memory allocation and deallocation performance", 100) {
    
    std::vector<void*> allocations;
    const size_t numAllocations = 1000;
    const size_t allocationSize = 1024;
    
    // Allocate
    for (size_t i = 0; i < numAllocations; ++i) {
        void* ptr = malloc(allocationSize);
        if (ptr) {
            allocations.push_back(ptr);
        }
    }
    
    // Deallocate
    for (void* ptr : allocations) {
        if (ptr) {
            free(ptr);
        }
    }
}

BOLT_BENCHMARK_CONFIG(memory_pool_performance, "CORE",
    "Memory pool allocation vs standard allocation", 50) {
    
    const size_t numAllocations = 500;
    const size_t blockSize = 512;
    
    // Test memory pool
    MemoryPool pool(blockSize, numAllocations);
    std::vector<void*> poolAllocations;
    
    for (size_t i = 0; i < numAllocations; ++i) {
        void* ptr = pool.allocate();
        if (ptr) {
            poolAllocations.push_back(ptr);
        }
    }
    
    for (void* ptr : poolAllocations) {
        if (ptr) {
            pool.deallocate(ptr);
        }
    }
}

// Performance Profiler Benchmarks
BOLT_BENCHMARK_CONFIG(profiler_overhead, "CORE",
    "Measure performance profiler overhead", 1000) {
    
    auto& profiler = PerformanceProfiler::getInstance();
    
    // Simple operation with profiling
    for (int i = 0; i < 100; ++i) {
        auto metric = profiler.startMetric("test_operation", "BENCHMARK");
        
        // Simulate some work
        volatile int sum = 0;
        for (int j = 0; j < 1000; ++j) {
            sum += j;
        }
        
        profiler.endMetric(metric);
    }
}

// Threading Benchmarks
BOLT_BENCHMARK_CONFIG(thread_creation_overhead, "CORE",
    "Thread creation and destruction overhead", 20) {
    
    const int numThreads = 10;
    std::vector<std::thread> threads;
    
    // Create threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([]() {
            // Simple work
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }
    
    // Join threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

BOLT_BENCHMARK_CONFIG(mutex_contention, "CORE",
    "Mutex contention performance with multiple threads", 10) {
    
    const int numThreads = 4;
    const int operationsPerThread = 1000;
    std::mutex testMutex;
    int sharedCounter = 0;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                std::lock_guard<std::mutex> lock(testMutex);
                sharedCounter++;
            }
        });
    }
    
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// String Processing Benchmarks
BOLT_BENCHMARK_CONFIG(string_operations, "CORE",
    "String creation, manipulation, and destruction", 100) {
    
    const int numStrings = 1000;
    std::vector<std::string> strings;
    
    // String creation
    for (int i = 0; i < numStrings; ++i) {
        strings.emplace_back("Test string " + std::to_string(i) + " with some content");
    }
    
    // String manipulation
    for (auto& str : strings) {
        str += " appended";
        str = str.substr(0, str.length() / 2);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    }
    
    // String searching
    int found = 0;
    for (const auto& str : strings) {
        if (str.find("TEST") != std::string::npos) {
            found++;
        }
    }
}

// Container Benchmarks
BOLT_BENCHMARK_CONFIG(vector_operations, "CORE",
    "Vector push_back, insert, and erase operations", 50) {
    
    std::vector<int> vec;
    const int numElements = 10000;
    
    // Push back
    for (int i = 0; i < numElements; ++i) {
        vec.push_back(i);
    }
    
    // Random access
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, numElements - 1);
    
    int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += vec[dis(gen)];
    }
    
    // Erase elements
    while (!vec.empty() && vec.size() > numElements / 2) {
        vec.erase(vec.begin() + vec.size() / 2);
    }
}

BOLT_BENCHMARK_CONFIG(map_operations, "CORE",
    "Map insertion, lookup, and deletion operations", 30) {
    
    std::unordered_map<std::string, int> map;
    const int numElements = 5000;
    
    // Insertion
    for (int i = 0; i < numElements; ++i) {
        map["key_" + std::to_string(i)] = i;
    }
    
    // Lookup
    int sum = 0;
    for (int i = 0; i < numElements; ++i) {
        auto it = map.find("key_" + std::to_string(i));
        if (it != map.end()) {
            sum += it->second;
        }
    }
    
    // Deletion
    for (int i = 0; i < numElements / 2; ++i) {
        map.erase("key_" + std::to_string(i));
    }
}

// File I/O Benchmarks
BOLT_BENCHMARK_CONFIG(file_io_operations, "CORE",
    "File creation, writing, reading, and deletion", 10) {
    
    const std::string filename = "/tmp/benchmark_test_file.txt";
    const std::string content = "This is test content for file I/O benchmark. ";
    const int numLines = 1000;
    
    // Write file
    {
        std::ofstream file(filename);
        for (int i = 0; i < numLines; ++i) {
            file << content << "Line " << i << std::endl;
        }
    }
    
    // Read file
    {
        std::ifstream file(filename);
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            lineCount++;
        }
    }
    
    // Clean up
    std::remove(filename.c_str());
}

// Logging Performance Benchmark
BOLT_BENCHMARK_CONFIG(logging_performance, "CORE",
    "Logging system performance with different levels", 50) {
    
    const int numLogs = 1000;
    
    // Configure logging for benchmark
    LogManager::configureConsoleLogging(LogLevel::WARN, false);
    
    // Test different log levels
    for (int i = 0; i < numLogs; ++i) {
        BOLT_LOG_DEBUG("Debug message {}", i);
        BOLT_LOG_INFO("Info message {}", i);
        BOLT_LOG_WARNING("Warning message {}", i);
        if (i % 100 == 0) {
            BOLT_LOG_ERROR("Error message {}", i);
        }
    }
}