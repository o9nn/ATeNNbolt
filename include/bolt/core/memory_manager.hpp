
#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <cstddef>
#include <mutex>
#include "error_handling.hpp"

namespace bolt {

class MemoryManager {
private:
    size_t totalAllocated_ = 0;
    size_t peakUsage_ = 0;
    size_t maxAllowedUsage_ = SIZE_MAX;
    std::unordered_map<void*, size_t> allocations_;
    mutable std::mutex mutex_; // Thread safety for memory operations

public:
    static MemoryManager& getInstance() {
        static MemoryManager instance;
        return instance;
    }

    // Set maximum allowed memory usage (for memory bounds checking)
    void setMaxAllowedUsage(size_t maxUsage) {
        std::lock_guard<std::mutex> lock(mutex_);
        ErrorHandler::validateParameter(maxUsage > 0, "Max allowed usage must be greater than 0");
        maxAllowedUsage_ = maxUsage;
    }

    void* allocate(size_t size) {
        ErrorHandler::validateMemorySize(size);
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if allocation would exceed maximum allowed usage
        if (totalAllocated_ + size > maxAllowedUsage_) {
            throw MemoryException(ErrorCode::MEMORY_ALLOCATION_FAILED,
                "Allocation would exceed maximum allowed usage. Requested: " + 
                std::to_string(size) + ", Current: " + std::to_string(totalAllocated_) + 
                ", Max: " + std::to_string(maxAllowedUsage_));
        }
        
        void* ptr = std::malloc(size);
        if (!ptr) {
            throw MemoryException(ErrorCode::MEMORY_ALLOCATION_FAILED,
                "Failed to allocate " + std::to_string(size) + " bytes");
        }
        
        allocations_[ptr] = size;
        totalAllocated_ += size;
        if (totalAllocated_ > peakUsage_) {
            peakUsage_ = totalAllocated_;
        }
        
        return ptr;
    }

    void deallocate(void* ptr) {
        if (!ptr) return; // Allow null pointer deallocation (like standard free)
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) {
            throw MemoryException(ErrorCode::MEMORY_CORRUPTION,
                "Attempting to deallocate untracked or already freed pointer");
        }
        
        totalAllocated_ -= it->second;
        allocations_.erase(it);
        std::free(ptr);
    }

    size_t getCurrentUsage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return totalAllocated_;
    }

    size_t getPeakUsage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return peakUsage_;
    }

    size_t getTotalAllocated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return totalAllocated_;
    }

    size_t getAllocationCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return allocations_.size();
    }

    // Check for memory leaks
    bool hasMemoryLeaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !allocations_.empty();
    }

    // Get detailed leak information
    std::vector<std::pair<void*, size_t>> getLeakedAllocations() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<void*, size_t>> leaks;
        for (const auto& allocation : allocations_) {
            leaks.emplace_back(allocation.first, allocation.second);
        }
        return leaks;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!allocations_.empty()) {
            throw MemoryException(ErrorCode::MEMORY_CORRUPTION,
                "Cannot reset memory manager with " + std::to_string(allocations_.size()) + 
                " active allocations. This would cause memory leaks.");
        }
        
        totalAllocated_ = 0;
        peakUsage_ = 0;
    }

    // Safe reset that frees all allocations (use with caution)
    void forceReset() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Free all tracked allocations
        for (const auto& allocation : allocations_) {
            std::free(allocation.first);
        }
        
        allocations_.clear();
        totalAllocated_ = 0;
        peakUsage_ = 0;
    }
};

} // namespace bolt

#endif
