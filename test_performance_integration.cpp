#include "bolt/core/performance_profiler.hpp"
#include "bolt/core/logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

// Mock AI components to demonstrate profiler integration
class MockAIModel {
private:
    std::string modelName_;
    size_t modelSize_;
    
public:
    MockAIModel(const std::string& name, size_t size) 
        : modelName_(name), modelSize_(size) {}
    
    void load() {
        BOLT_PROFILE_AI("model_load:" + modelName_);
        
        // Simulate model loading time based on size
        int loadTime = modelSize_ / 1000000; // 1ms per MB
        std::this_thread::sleep_for(std::chrono::milliseconds(loadTime));
        
        // Simulate memory allocation
        std::vector<float> modelData(modelSize_ / sizeof(float));
        modelData.clear(); // Immediate cleanup for demo
    }
    
    std::string inference(const std::string& input) {
        BOLT_PROFILE_AI("inference:" + modelName_);
        
        // Simulate inference time
        std::this_thread::sleep_for(std::chrono::milliseconds(20 + input.length() / 10));
        
        return "AI processed: " + input;
    }
    
    void unload() {
        BOLT_PROFILE_AI("model_unload:" + modelName_);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
};

// Mock editor operations
class MockEditor {
public:
    void loadFile(const std::string& filename) {
        BOLT_PROFILE_EDITOR("file_load:" + filename);
        
        // Simulate file loading
        int fileSize = filename.length() * 1000; // Mock file size
        int loadTime = fileSize / 100000; // Time based on size
        std::this_thread::sleep_for(std::chrono::milliseconds(loadTime));
    }
    
    void applySyntaxHighlighting(const std::string& content) {
        BOLT_PROFILE_EDITOR("syntax_highlighting");
        
        // Simulate syntax highlighting processing
        int processTime = content.length() / 1000; // Time based on content
        std::this_thread::sleep_for(std::chrono::milliseconds(processTime + 5));
    }
    
    void saveFile(const std::string& filename, const std::string& content) {
        BOLT_PROFILE_EDITOR("file_save:" + filename);
        
        // Simulate file saving
        int saveTime = content.length() / 50000; // Time based on content
        std::this_thread::sleep_for(std::chrono::milliseconds(saveTime + 2));
    }
};

// Mock memory manager
class MockMemoryManager {
public:
    void* allocate(size_t size, const std::string& purpose) {
        BOLT_PROFILE_MEMORY("allocate:" + purpose);
        
        // Simulate allocation time
        std::this_thread::sleep_for(std::chrono::microseconds(size / 1000));
        
        return malloc(size);
    }
    
    void deallocate(void* ptr, const std::string& purpose) {
        BOLT_PROFILE_MEMORY("deallocate:" + purpose);
        
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        free(ptr);
    }
};

// Integration test scenarios
void testAIWorkflow() {
    std::cout << "=== AI Workflow Integration Test ===" << std::endl;
    
    BOLT_PROFILE_SESSION("ai_workflow");
    
    // Multiple AI models working together
    std::vector<std::unique_ptr<MockAIModel>> models;
    models.emplace_back(std::make_unique<MockAIModel>("text_classifier", 10000000)); // 10MB
    models.emplace_back(std::make_unique<MockAIModel>("sentiment_analyzer", 5000000)); // 5MB
    models.emplace_back(std::make_unique<MockAIModel>("summarizer", 15000000)); // 15MB
    
    // Load all models
    {
        BOLT_PROFILE_SCOPE("batch_model_loading");
        for (auto& model : models) {
            model->load();
        }
    }
    
    // Process multiple inputs through pipeline
    std::vector<std::string> inputs = {
        "This is a test document for AI processing.",
        "Another longer document with more content to analyze and understand.",
        "Short text.",
        "Medium length document with some technical content and various terms."
    };
    
    std::vector<std::string> results;
    {
        BOLT_PROFILE_SCOPE("ai_pipeline_processing");
        for (const auto& input : inputs) {
            std::string result = input;
            for (auto& model : models) {
                result = model->inference(result);
            }
            results.push_back(result);
        }
    }
    
    // Cleanup
    {
        BOLT_PROFILE_SCOPE("model_cleanup");
        for (auto& model : models) {
            model->unload();
        }
    }
    
    auto session = bolt::PerformanceProfiler::getInstance().getCurrentSession();
    if (session) {
        std::cout << "AI Workflow completed: " << session->getMetricsCount() 
                  << " operations in " << session->getTotalDurationMs() << " ms" << std::endl;
    }
}

void testEditorIntegration() {
    std::cout << "\\n=== Editor Integration Test ===" << std::endl;
    
    BOLT_PROFILE_SESSION("editor_session");
    
    MockEditor editor;
    
    // Simulate opening multiple files
    std::vector<std::string> files = {
        "main.cpp", "header.hpp", "config.json", "readme.md", "large_file.txt"
    };
    
    {
        BOLT_PROFILE_SCOPE("multi_file_loading");
        for (const auto& file : files) {
            editor.loadFile(file);
            
            // Simulate file content based on extension
            std::string content;
            if (file.find(".cpp") != std::string::npos || file.find(".hpp") != std::string::npos) {
                content = std::string(5000, 'c'); // 5KB of C++ code
                editor.applySyntaxHighlighting(content);
            } else if (file.find(".json") != std::string::npos) {
                content = std::string(1000, 'j'); // 1KB of JSON
                editor.applySyntaxHighlighting(content);
            } else {
                content = std::string(2000, 't'); // 2KB of text
            }
            
            // Save modified files
            if (file != "readme.md") { // Don't modify readme
                editor.saveFile(file, content + "\\n// Modified");
            }
        }
    }
    
    auto session = bolt::PerformanceProfiler::getInstance().getCurrentSession();
    if (session) {
        std::cout << "Editor session completed: " << session->getMetricsCount() 
                  << " operations" << std::endl;
    }
}

void testMemoryIntegration() {
    std::cout << "\\n=== Memory Management Integration Test ===" << std::endl;
    
    BOLT_PROFILE_SESSION("memory_management");
    
    MockMemoryManager memMgr;
    std::vector<void*> allocations;
    
    {
        BOLT_PROFILE_SCOPE("bulk_allocations");
        
        // Allocate various sized blocks
        std::vector<std::pair<size_t, std::string>> allocs = {
            {1024 * 1024, "texture_buffer"},      // 1MB
            {512 * 1024, "audio_buffer"},         // 512KB
            {256 * 1024, "code_cache"},           // 256KB
            {128 * 1024, "ui_elements"},          // 128KB
            {64 * 1024, "small_buffers"}          // 64KB
        };
        
        for (const auto& [size, purpose] : allocs) {
            void* ptr = memMgr.allocate(size, purpose);
            allocations.push_back(ptr);
        }
    }
    
    {
        BOLT_PROFILE_SCOPE("memory_usage_simulation");
        // Simulate some work with allocated memory
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    {
        BOLT_PROFILE_SCOPE("bulk_deallocations");
        for (size_t i = 0; i < allocations.size(); ++i) {
            std::string purpose = "cleanup_" + std::to_string(i);
            memMgr.deallocate(allocations[i], purpose);
        }
    }
    
    auto session = bolt::PerformanceProfiler::getInstance().getCurrentSession();
    if (session) {
        std::cout << "Memory management completed: " << session->getMetricsCount() 
                  << " operations" << std::endl;
    }
}

void testConcurrentIntegration() {
    std::cout << "\\n=== Concurrent Operations Integration Test ===" << std::endl;
    
    const int numThreads = 3;
    std::vector<std::thread> workers;
    
    // Thread 1: AI processing
    workers.emplace_back([]() {
        BOLT_PROFILE_SESSION("concurrent_ai");
        
        MockAIModel model("concurrent_model", 5000000);
        model.load();
        
        for (int i = 0; i < 5; ++i) {
            std::string input = "Concurrent input " + std::to_string(i);
            model.inference(input);
        }
        
        model.unload();
    });
    
    // Thread 2: Editor operations
    workers.emplace_back([]() {
        BOLT_PROFILE_SESSION("concurrent_editor");
        
        MockEditor editor;
        
        for (int i = 0; i < 3; ++i) {
            std::string filename = "concurrent_file_" + std::to_string(i) + ".cpp";
            editor.loadFile(filename);
            editor.applySyntaxHighlighting("some code content here");
            editor.saveFile(filename, "modified content");
        }
    });
    
    // Thread 3: Memory operations
    workers.emplace_back([]() {
        BOLT_PROFILE_SESSION("concurrent_memory");
        
        MockMemoryManager memMgr;
        
        for (int i = 0; i < 10; ++i) {
            size_t size = (i + 1) * 1024; // 1KB to 10KB
            std::string purpose = "concurrent_alloc_" + std::to_string(i);
            void* ptr = memMgr.allocate(size, purpose);
            
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            
            memMgr.deallocate(ptr, purpose);
        }
    });
    
    // Wait for all threads
    for (auto& worker : workers) {
        worker.join();
    }
    
    std::cout << "Concurrent operations completed successfully" << std::endl;
}

void analyzePerformanceResults() {
    std::cout << "\\n=== Performance Analysis ===" << std::endl;
    
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    
    // Print overall summary
    profiler.printSummary();
    
    // Analyze by category
    auto categories = profiler.getCategories();
    std::cout << "\\nPerformance by Category:" << std::endl;
    
    for (const auto& category : categories) {
        auto metrics = profiler.getMetricsByCategory(category);
        if (!metrics.empty()) {
            double totalTime = 0.0;
            double minTime = metrics[0]->getDurationMs();
            double maxTime = metrics[0]->getDurationMs();
            
            for (const auto& metric : metrics) {
                double duration = metric->getDurationMs();
                totalTime += duration;
                minTime = std::min(minTime, duration);
                maxTime = std::max(maxTime, duration);
            }
            
            double avgTime = totalTime / metrics.size();
            
            std::cout << "  " << category << ": " 
                      << metrics.size() << " ops, "
                      << "avg " << avgTime << "ms, "
                      << "min " << minTime << "ms, "
                      << "max " << maxTime << "ms, "
                      << "total " << totalTime << "ms" << std::endl;
        }
    }
    
    // Export data for further analysis
    std::cout << "\\nExporting performance data..." << std::endl;
    profiler.exportToJson("/tmp/integration_test_performance.json");
    profiler.exportToCsv("/tmp/integration_test_performance.csv");
    
    std::cout << "Performance data exported to:" << std::endl;
    std::cout << "  JSON: /tmp/integration_test_performance.json" << std::endl;
    std::cout << "  CSV:  /tmp/integration_test_performance.csv" << std::endl;
}

int main() {
    std::cout << "Bolt Performance Profiler Integration Tests" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    try {
        // Initialize logging
        bolt::LogManager::configureConsoleLogging(bolt::LogLevel::WARN, false);
        
        // Initialize profiler
        auto& profiler = bolt::PerformanceProfiler::getInstance();
        profiler.reset();
        profiler.enable();
        profiler.enableSystemMonitoring();
        
        // Run integration tests
        testAIWorkflow();
        testEditorIntegration();
        testMemoryIntegration();
        testConcurrentIntegration();
        
        // Analyze results
        analyzePerformanceResults();
        
        std::cout << "\\n=== Integration Tests Complete ===" << std::endl;
        std::cout << "All integration tests completed successfully!" << std::endl;
        std::cout << "Total metrics collected: " << profiler.getTotalMetricsCount() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Integration tests failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}