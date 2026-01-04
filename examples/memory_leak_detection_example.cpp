/**
 * @file examples/memory_leak_detection_example.cpp
 * @brief Comprehensive example demonstrating memory leak detection tools
 * 
 * This example shows how to use:
 * 1. Built-in MemoryLeakDetector
 * 2. MemoryManager
 * 3. Integration with sanitizers
 * 4. Best practices for leak-free code
 */

#include "bolt/core/memory_leak_detector.hpp"
#include "bolt/core/memory_manager.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <cstring>

// RAII wrapper for tracked memory
template<typename T>
class TrackedBuffer {
public:
    TrackedBuffer(size_t count, const std::string& category = "general") 
        : size_(count * sizeof(T)), category_(category) {
        ptr_ = static_cast<T*>(malloc(size_));
        if (ptr_) {
            TRACK_ALLOCATION(ptr_, size_, category_);
            std::cout << "Allocated " << size_ << " bytes in category '" 
                      << category_ << "'\n";
        }
    }
    
    ~TrackedBuffer() {
        if (ptr_) {
            UNTRACK_ALLOCATION(ptr_);
            free(ptr_);
            std::cout << "Freed " << size_ << " bytes from category '" 
                      << category_ << "'\n";
        }
    }
    
    // Disable copy
    TrackedBuffer(const TrackedBuffer&) = delete;
    TrackedBuffer& operator=(const TrackedBuffer&) = delete;
    
    // Enable move
    TrackedBuffer(TrackedBuffer&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_), category_(std::move(other.category_)) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    T* get() { return ptr_; }
    const T* get() const { return ptr_; }
    size_t size() const { return size_; }
    
private:
    T* ptr_ = nullptr;
    size_t size_ = 0;
    std::string category_;
};

// Example 1: Basic leak detection
void example_basic_leak_detection() {
    std::cout << "\n=== Example 1: Basic Leak Detection ===\n";
    
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Allocate some memory
    void* ptr1 = malloc(1024);
    TRACK_ALLOCATION(ptr1, 1024, "buffers");
    
    void* ptr2 = malloc(2048);
    TRACK_ALLOCATION(ptr2, 2048, "network");
    
    // Check for leaks before cleanup
    std::cout << "\nBefore cleanup:\n";
    std::cout << detector.generateReport() << "\n";
    
    // Properly cleanup
    UNTRACK_ALLOCATION(ptr1);
    free(ptr1);
    
    UNTRACK_ALLOCATION(ptr2);
    free(ptr2);
    
    // Check again
    std::cout << "\nAfter cleanup:\n";
    std::cout << detector.generateReport() << "\n";
}

// Example 2: RAII-based leak prevention
void example_raii_leak_prevention() {
    std::cout << "\n=== Example 2: RAII-based Leak Prevention ===\n";
    
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    {
        // Allocations are automatically cleaned up
        TrackedBuffer<char> buffer1(1024, "string_buffers");
        TrackedBuffer<int> buffer2(256, "int_arrays");
        
        // Use the buffers
        strcpy(buffer1.get(), "Hello, World!");
        std::cout << "Buffer1 contains: " << buffer1.get() << "\n";
        
        // Buffers automatically freed when going out of scope
    }
    
    std::cout << "\nAfter scope exit:\n";
    std::cout << detector.generateReport() << "\n";
}

// Example 3: Exception safety with leak detection
void example_exception_safety() {
    std::cout << "\n=== Example 3: Exception Safety ===\n";
    
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    try {
        TrackedBuffer<char> buffer(4096, "exception_test");
        
        // Simulate an error
        throw std::runtime_error("Simulated error");
        
        // This won't execute, but buffer is still cleaned up
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << "\n";
    }
    
    std::cout << "\nAfter exception:\n";
    std::cout << detector.generateReport() << "\n";
}

// Example 4: Integration with MemoryManager
void example_memory_manager_integration() {
    std::cout << "\n=== Example 4: Memory Manager Integration ===\n";
    
    auto& manager = bolt::MemoryManager::getInstance();
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Set memory limit
    manager.setMaxAllowedUsage(10 * 1024 * 1024); // 10 MB
    
    std::vector<void*> allocations;
    
    try {
        // Allocate using MemoryManager
        for (int i = 0; i < 5; ++i) {
            void* ptr = manager.allocate(1024 * 1024); // 1 MB each
            allocations.push_back(ptr);
            
            // Also track with leak detector
            TRACK_ALLOCATION(ptr, 1024 * 1024, "memory_manager");
        }
        
        std::cout << "Current usage: " << manager.getCurrentUsage() << " bytes\n";
        std::cout << "Peak usage: " << manager.getPeakUsage() << " bytes\n";
        
        // Cleanup
        for (void* ptr : allocations) {
            UNTRACK_ALLOCATION(ptr);
            manager.deallocate(ptr);
        }
        
        std::cout << "\nAfter cleanup:\n";
        std::cout << "Current usage: " << manager.getCurrentUsage() << " bytes\n";
        std::cout << detector.generateReport() << "\n";
        
    } catch (const bolt::MemoryException& e) {
        std::cerr << "Memory error: " << e.what() << "\n";
        
        // Emergency cleanup
        for (void* ptr : allocations) {
            if (ptr) {
                UNTRACK_ALLOCATION(ptr);
                manager.deallocate(ptr);
            }
        }
    }
}

// Example 5: Categorization for better leak analysis
void example_categorization() {
    std::cout << "\n=== Example 5: Leak Categorization ===\n";
    
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Allocate in different categories
    void* net_buf1 = malloc(1024);
    TRACK_ALLOCATION(net_buf1, 1024, "network");
    
    void* net_buf2 = malloc(2048);
    TRACK_ALLOCATION(net_buf2, 2048, "network");
    
    void* img_buf = malloc(1024 * 1024);
    TRACK_ALLOCATION(img_buf, 1024 * 1024, "images");
    
    void* str_buf = malloc(512);
    TRACK_ALLOCATION(str_buf, 512, "strings");
    
    // Get statistics
    auto stats = detector.getStats();
    
    std::cout << "\nMemory usage by category:\n";
    for (const auto& [category, bytes] : stats.leaksByCategory) {
        std::cout << "  " << category << ": " << bytes << " bytes\n";
    }
    
    // Simulate forgetting to free one allocation
    UNTRACK_ALLOCATION(net_buf1);
    free(net_buf1);
    
    UNTRACK_ALLOCATION(net_buf2);
    free(net_buf2);
    
    UNTRACK_ALLOCATION(str_buf);
    free(str_buf);
    
    // img_buf is "leaked" - not freed
    std::cout << "\nLeak report (img_buf not freed):\n";
    std::cout << detector.generateReport() << "\n";
    
    // Cleanup the leak
    UNTRACK_ALLOCATION(img_buf);
    free(img_buf);
}

// Example 6: Sanitizer integration
void example_sanitizer_integration() {
    std::cout << "\n=== Example 6: Sanitizer Integration ===\n";
    
#ifdef ASAN_ENABLED
    std::cout << "AddressSanitizer is ENABLED\n";
    std::cout << "Run with: ASAN_OPTIONS=detect_leaks=1 ./program\n";
#endif
    
#ifdef LSAN_ENABLED
    std::cout << "LeakSanitizer is ENABLED\n";
    std::cout << "Run with: LSAN_OPTIONS=verbosity=1:log_threads=1 ./program\n";
#endif
    
#ifdef UBSAN_ENABLED
    std::cout << "UndefinedBehaviorSanitizer is ENABLED\n";
#endif
    
#ifdef TSAN_ENABLED
    std::cout << "ThreadSanitizer is ENABLED\n";
#endif
    
#ifdef MSAN_ENABLED
    std::cout << "MemorySanitizer is ENABLED\n";
#endif
    
#if !defined(ASAN_ENABLED) && !defined(LSAN_ENABLED) && \
    !defined(UBSAN_ENABLED) && !defined(TSAN_ENABLED) && \
    !defined(MSAN_ENABLED)
    std::cout << "No sanitizers enabled. Build with:\n";
    std::cout << "  cmake -DENABLE_SANITIZER_ADDRESS=ON ..\n";
    std::cout << "  cmake -DENABLE_SANITIZER_LEAK=ON ..\n";
#endif
}

// Example 7: Demonstrating a real leak (for testing detection)
void example_intentional_leak() {
    std::cout << "\n=== Example 7: Intentional Leak (for testing) ===\n";
    
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Intentionally leak memory (temporarily for demonstration)
    void* leaked = malloc(4096);
    TRACK_ALLOCATION(leaked, 4096, "intentional_leak");
    
    std::cout << "Intentionally leaked 4096 bytes (temporarily)\n";
    std::cout << detector.generateReport() << "\n";
    
    std::cout << "\nNote: In production code, ALWAYS free allocated memory!\n";
    std::cout << "Cleaning up the intentional leak for proper shutdown...\n";
    
    // Cleanup for proper program termination
    // In a real leak scenario, this line would be missing
    UNTRACK_ALLOCATION(leaked);
    free(leaked);
    
    std::cout << "Leak cleaned up successfully.\n";
}

int main() {
    std::cout << "======================================\n";
    std::cout << "Memory Leak Detection Examples\n";
    std::cout << "======================================\n";
    
    // Enable leak detection
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.setEnabled(true);
    
    // Run examples
    example_basic_leak_detection();
    example_raii_leak_prevention();
    example_exception_safety();
    example_memory_manager_integration();
    example_categorization();
    example_sanitizer_integration();
    example_intentional_leak();
    
    std::cout << "\n======================================\n";
    std::cout << "Examples complete!\n";
    std::cout << "======================================\n";
    
    return 0;
}
