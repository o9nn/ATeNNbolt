#include "bolt/network/network_buffer.hpp"
#include <algorithm>
#include <stdexcept>

namespace bolt {

// NetworkBuffer Implementation

NetworkBuffer::NetworkBuffer(size_t initialSize) : size_(0), readPos_(0) {
    data_.reserve(initialSize);
}

void NetworkBuffer::reserve(size_t size) {
    data_.reserve(size);
}

void NetworkBuffer::resize(size_t size) {
    data_.resize(size);
    size_ = size;
    readPos_ = std::min(readPos_, size_);
}

void NetworkBuffer::clear() {
    size_ = 0;
    readPos_ = 0;
}

void NetworkBuffer::append(const void* data, size_t length) {
    if (length == 0) return;
    
    ensureSpace(length);
    std::memcpy(data_.data() + size_, data, length);
    size_ += length;
}

void NetworkBuffer::append(const std::string& str) {
    append(str.data(), str.size());
}

void NetworkBuffer::append(const std::vector<uint8_t>& vec) {
    append(vec.data(), vec.size());
}

void NetworkBuffer::append(uint8_t byte) {
    ensureSpace(1);
    data_[size_++] = byte;
}

std::vector<uint8_t> NetworkBuffer::consume(size_t length) {
    size_t availableData = size_ - readPos_;
    size_t toConsume = std::min(length, availableData);
    
    std::vector<uint8_t> result(data_.begin() + readPos_, data_.begin() + readPos_ + toConsume);
    readPos_ += toConsume;
    
    return result;
}

std::string NetworkBuffer::consumeString(size_t length) {
    size_t availableData = size_ - readPos_;
    size_t toConsume = std::min(length, availableData);
    
    std::string result(data_.begin() + readPos_, data_.begin() + readPos_ + toConsume);
    readPos_ += toConsume;
    
    return result;
}

void NetworkBuffer::consume(void* dest, size_t length) {
    size_t availableData = size_ - readPos_;
    size_t toConsume = std::min(length, availableData);
    
    std::memcpy(dest, data_.data() + readPos_, toConsume);
    readPos_ += toConsume;
}

void NetworkBuffer::discard(size_t length) {
    size_t availableData = size_ - readPos_;
    size_t toDiscard = std::min(length, availableData);
    readPos_ += toDiscard;
}

void NetworkBuffer::compact() {
    if (readPos_ == 0) return;
    
    size_t remainingData = size_ - readPos_;
    if (remainingData > 0) {
        std::memmove(data_.data(), data_.data() + readPos_, remainingData);
    }
    
    size_ = remainingData;
    readPos_ = 0;
}

void NetworkBuffer::ensureSpace(size_t needed) {
    size_t requiredCapacity = size_ + needed;
    if (data_.capacity() < requiredCapacity) {
        // Grow by 1.5x or required size, whichever is larger
        size_t newCapacity = std::max(requiredCapacity, data_.capacity() * 3 / 2);
        data_.reserve(newCapacity);
    }
    data_.resize(std::max(data_.size(), requiredCapacity));
}

uint8_t* NetworkBuffer::prepareWrite(size_t length) {
    ensureSpace(length);
    return data_.data() + size_;
}

void NetworkBuffer::commitWrite(size_t length) {
    size_ += length;
}

// NetworkBufferPool Implementation

std::unique_ptr<NetworkBuffer> NetworkBufferPool::getBuffer(size_t minSize) {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    if (!pool_.empty()) {
        auto buffer = std::move(pool_.front());
        pool_.pop();
        
        buffer->clear();
        if (buffer->capacity() < minSize) {
            buffer->reserve(minSize);
        }
        
        activeBuffers_++;
        return buffer;
    }
    
    activeBuffers_++;
    return std::make_unique<NetworkBuffer>(minSize);
}

void NetworkBufferPool::returnBuffer(std::unique_ptr<NetworkBuffer> buffer) {
    if (!buffer) return;
    
    activeBuffers_--;
    
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    if (pool_.size() < maxPoolSize_) {
        buffer->clear();
        pool_.push(std::move(buffer));
    }
    // If pool is full, buffer is automatically destroyed
}

size_t NetworkBufferPool::getPoolSize() const {
    std::lock_guard<std::mutex> lock(poolMutex_);
    return pool_.size();
}

// RingBuffer Implementation

RingBuffer::RingBuffer(size_t size) : buffer_(size), size_(size), writePos_(0), readPos_(0) {
}

size_t RingBuffer::write(const void* data, size_t length) {
    const uint8_t* src = static_cast<const uint8_t*>(data);
    size_t written = 0;
    size_t currentWrite = writePos_.load();
    
    while (written < length && writeAvailable() > 0) {
        size_t currentRead = readPos_.load();
        size_t available = (currentRead > currentWrite) ? 
                          (currentRead - currentWrite - 1) : 
                          (size_ - currentWrite + currentRead - 1);
        
        if (available == 0) break;
        
        size_t toWrite = std::min(length - written, available);
        size_t endSpace = size_ - currentWrite;
        
        if (toWrite <= endSpace) {
            // Can write in one chunk
            std::memcpy(&buffer_[currentWrite], src + written, toWrite);
            currentWrite = (currentWrite + toWrite) % size_;
            written += toWrite;
        } else {
            // Split write
            std::memcpy(&buffer_[currentWrite], src + written, endSpace);
            size_t remaining = toWrite - endSpace;
            std::memcpy(&buffer_[0], src + written + endSpace, remaining);
            currentWrite = remaining;
            written += toWrite;
        }
    }
    
    writePos_.store(currentWrite);
    return written;
}

size_t RingBuffer::writeAvailable() const {
    size_t currentWrite = writePos_.load();
    size_t currentRead = readPos_.load();
    
    if (currentRead > currentWrite) {
        return currentRead - currentWrite - 1;
    } else {
        return size_ - currentWrite + currentRead - 1;
    }
}

bool RingBuffer::canWrite(size_t length) const {
    return writeAvailable() >= length;
}

size_t RingBuffer::read(void* data, size_t length) {
    uint8_t* dest = static_cast<uint8_t*>(data);
    size_t read = 0;
    size_t currentRead = readPos_.load();
    
    while (read < length && readAvailable() > 0) {
        size_t currentWrite = writePos_.load();
        size_t available = (currentWrite >= currentRead) ? 
                          (currentWrite - currentRead) : 
                          (size_ - currentRead + currentWrite);
        
        if (available == 0) break;
        
        size_t toRead = std::min(length - read, available);
        size_t endSpace = size_ - currentRead;
        
        if (toRead <= endSpace) {
            // Can read in one chunk
            std::memcpy(dest + read, &buffer_[currentRead], toRead);
            currentRead = (currentRead + toRead) % size_;
            read += toRead;
        } else {
            // Split read
            std::memcpy(dest + read, &buffer_[currentRead], endSpace);
            size_t remaining = toRead - endSpace;
            std::memcpy(dest + read + endSpace, &buffer_[0], remaining);
            currentRead = remaining;
            read += toRead;
        }
    }
    
    readPos_.store(currentRead);
    return read;
}

size_t RingBuffer::readAvailable() const {
    size_t currentWrite = writePos_.load();
    size_t currentRead = readPos_.load();
    
    if (currentWrite >= currentRead) {
        return currentWrite - currentRead;
    } else {
        return size_ - currentRead + currentWrite;
    }
}

bool RingBuffer::canRead(size_t length) const {
    return readAvailable() >= length;
}

bool RingBuffer::empty() const {
    return readAvailable() == 0;
}

bool RingBuffer::full() const {
    return writeAvailable() == 0;
}

void RingBuffer::clear() {
    writePos_.store(0);
    readPos_.store(0);
}

size_t RingBuffer::peek(void* data, size_t length) const {
    uint8_t* dest = static_cast<uint8_t*>(data);
    size_t currentRead = readPos_.load();
    size_t currentWrite = writePos_.load();
    
    size_t available = (currentWrite >= currentRead) ? 
                      (currentWrite - currentRead) : 
                      (size_ - currentRead + currentWrite);
    
    size_t toPeek = std::min(length, available);
    size_t endSpace = size_ - currentRead;
    
    if (toPeek <= endSpace) {
        std::memcpy(dest, &buffer_[currentRead], toPeek);
    } else {
        std::memcpy(dest, &buffer_[currentRead], endSpace);
        std::memcpy(dest + endSpace, &buffer_[0], toPeek - endSpace);
    }
    
    return toPeek;
}

void RingBuffer::skip(size_t length) {
    size_t available = readAvailable();
    size_t toSkip = std::min(length, available);
    size_t newRead = (readPos_.load() + toSkip) % size_;
    readPos_.store(newRead);
}

// ScatterGatherBuffer Implementation

void ScatterGatherBuffer::addSegment(void* data, size_t length) {
    if (length > 0) {
        segments_.emplace_back(data, length);
    }
}

void ScatterGatherBuffer::addSegment(const std::string& str) {
    addSegment(const_cast<char*>(str.data()), str.size());
}

void ScatterGatherBuffer::addSegment(const std::vector<uint8_t>& vec) {
    addSegment(const_cast<uint8_t*>(vec.data()), vec.size());
}

size_t ScatterGatherBuffer::getTotalSize() const {
    size_t total = 0;
    for (const auto& segment : segments_) {
        total += segment.length;
    }
    return total;
}

std::vector<uint8_t> ScatterGatherBuffer::flatten() const {
    std::vector<uint8_t> result;
    result.reserve(getTotalSize());
    
    for (const auto& segment : segments_) {
        const uint8_t* data = static_cast<const uint8_t*>(segment.data);
        result.insert(result.end(), data, data + segment.length);
    }
    
    return result;
}

void ScatterGatherBuffer::clear() {
    segments_.clear();
}

// ZeroCopyBuffer Implementation

void ZeroCopyBuffer::addReference(const void* data, size_t length) {
    if (length > 0) {
        segments_.emplace_back(data, length);
    }
}

void ZeroCopyBuffer::addReference(const std::string& str) {
    addReference(str.data(), str.size());
}

void ZeroCopyBuffer::addReference(const std::vector<uint8_t>& vec) {
    addReference(vec.data(), vec.size());
}

size_t ZeroCopyBuffer::getTotalSize() const {
    size_t total = 0;
    for (const auto& segment : segments_) {
        total += segment.length;
    }
    return total;
}

void ZeroCopyBuffer::copyTo(void* dest) const {
    uint8_t* output = static_cast<uint8_t*>(dest);
    size_t offset = 0;
    
    for (const auto& segment : segments_) {
        std::memcpy(output + offset, segment.data, segment.length);
        offset += segment.length;
    }
}

std::vector<uint8_t> ZeroCopyBuffer::copyToVector() const {
    std::vector<uint8_t> result;
    result.reserve(getTotalSize());
    
    for (const auto& segment : segments_) {
        const uint8_t* data = static_cast<const uint8_t*>(segment.data);
        result.insert(result.end(), data, data + segment.length);
    }
    
    return result;
}

void ZeroCopyBuffer::clear() {
    segments_.clear();
}

} // namespace bolt