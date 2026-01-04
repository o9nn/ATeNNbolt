#ifndef NETWORK_BUFFER_HPP
#define NETWORK_BUFFER_HPP

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <cstring>

namespace bolt {

/**
 * High-performance buffer for network I/O operations
 */
class NetworkBuffer {
public:
    explicit NetworkBuffer(size_t initialSize = 8192);
    ~NetworkBuffer() = default;

    // Buffer operations
    void reserve(size_t size);
    void resize(size_t size);
    size_t size() const { return size_; }
    size_t capacity() const { return data_.capacity(); }
    bool empty() const { return size_ == 0; }
    void clear();
    
    // Data access
    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }
    uint8_t* begin() { return data_.data(); }
    uint8_t* end() { return data_.data() + size_; }
    const uint8_t* begin() const { return data_.data(); }
    const uint8_t* end() const { return data_.data() + size_; }
    
    // Write operations
    void append(const void* data, size_t length);
    void append(const std::string& str);
    void append(const std::vector<uint8_t>& vec);
    void append(uint8_t byte);
    
    // Read operations
    std::vector<uint8_t> consume(size_t length);
    std::string consumeString(size_t length);
    void consume(void* dest, size_t length);
    void discard(size_t length);
    
    // Efficient operations
    void compact(); // Remove consumed data from front
    void ensureSpace(size_t needed);
    
    // Zero-copy operations
    uint8_t* prepareWrite(size_t length);
    void commitWrite(size_t length);
    
private:
    std::vector<uint8_t> data_;
    size_t size_;
    size_t readPos_;
};

/**
 * Memory pool for network buffers to reduce allocations
 */
class NetworkBufferPool {
public:
    static NetworkBufferPool& getInstance() {
        static NetworkBufferPool instance;
        return instance;
    }
    
    std::unique_ptr<NetworkBuffer> getBuffer(size_t minSize = 8192);
    void returnBuffer(std::unique_ptr<NetworkBuffer> buffer);
    
    // Pool statistics
    size_t getPoolSize() const;
    size_t getActiveBuffers() const { return activeBuffers_; }
    void setMaxPoolSize(size_t maxSize) { maxPoolSize_ = maxSize; }
    
private:
    NetworkBufferPool() : maxPoolSize_(100), activeBuffers_(0) {}
    
    mutable std::mutex poolMutex_;
    std::queue<std::unique_ptr<NetworkBuffer>> pool_;
    size_t maxPoolSize_;
    std::atomic<size_t> activeBuffers_;
};

/**
 * Ring buffer for high-throughput streaming data
 */
class RingBuffer {
public:
    explicit RingBuffer(size_t size);
    ~RingBuffer() = default;
    
    // Write operations (producer)
    size_t write(const void* data, size_t length);
    size_t writeAvailable() const;
    bool canWrite(size_t length) const;
    
    // Read operations (consumer)
    size_t read(void* data, size_t length);
    size_t readAvailable() const;
    bool canRead(size_t length) const;
    
    // Buffer state
    bool empty() const;
    bool full() const;
    void clear();
    size_t size() const { return size_; }
    
    // Advanced operations
    size_t peek(void* data, size_t length) const;
    void skip(size_t length);
    
private:
    std::vector<uint8_t> buffer_;
    size_t size_;
    std::atomic<size_t> writePos_;
    std::atomic<size_t> readPos_;
    
    size_t nextPos(size_t pos) const;
};

/**
 * Scatter-gather I/O vector for efficient network operations
 */
struct IOVector {
    void* data;
    size_t length;
    
    IOVector(void* d, size_t len) : data(d), length(len) {}
};

/**
 * Efficient scatter-gather buffer for network I/O
 */
class ScatterGatherBuffer {
public:
    ScatterGatherBuffer() = default;
    
    // Add data segments
    void addSegment(void* data, size_t length);
    void addSegment(const std::string& str);
    void addSegment(const std::vector<uint8_t>& vec);
    
    // Access segments
    const std::vector<IOVector>& getSegments() const { return segments_; }
    size_t getSegmentCount() const { return segments_.size(); }
    size_t getTotalSize() const;
    
    // Convert to contiguous buffer if needed
    std::vector<uint8_t> flatten() const;
    
    // Clear all segments
    void clear();
    
private:
    std::vector<IOVector> segments_;
};

/**
 * Zero-copy buffer for high-performance network operations
 */
class ZeroCopyBuffer {
public:
    ZeroCopyBuffer() = default;
    
    // Add data by reference (no copying)
    void addReference(const void* data, size_t length);
    void addReference(const std::string& str);
    void addReference(const std::vector<uint8_t>& vec);
    
    // Access data
    size_t getTotalSize() const;
    void copyTo(void* dest) const;
    std::vector<uint8_t> copyToVector() const;
    
    // Iterate through segments
    template<typename Func>
    void forEachSegment(Func&& func) const {
        for (const auto& segment : segments_) {
            func(segment.data, segment.length);
        }
    }
    
    void clear();
    
private:
    struct Segment {
        const void* data;
        size_t length;
        
        Segment(const void* d, size_t len) : data(d), length(len) {}
    };
    
    std::vector<Segment> segments_;
};

} // namespace bolt

#endif