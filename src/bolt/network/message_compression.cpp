#include "bolt/network/message_compression.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace bolt {

MessageCompressor::MessageCompressor(CompressionType type, int level)
    : type_(type), compressionLevel_(level), minCompressionSize_(256), zlibInitialized_(false) {
    initializeZlib();
}

MessageCompressor::~MessageCompressor() {
    cleanupZlib();
}

void MessageCompressor::initializeZlib() {
#ifdef BOLT_HAVE_ZLIB
    if (type_ == CompressionType::NONE) {
        return;
    }
    
    memset(&compressStream_, 0, sizeof(compressStream_));
    memset(&decompressStream_, 0, sizeof(decompressStream_));
    
    // Initialize compression stream
    int windowBits = (type_ == CompressionType::GZIP) ? 15 + 16 : 15;
    if (deflateInit2(&compressStream_, compressionLevel_, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        std::cerr << "Failed to initialize compression stream" << std::endl;
        return;
    }
    
    // Initialize decompression stream
    if (inflateInit2(&decompressStream_, windowBits) != Z_OK) {
        std::cerr << "Failed to initialize decompression stream" << std::endl;
        deflateEnd(&compressStream_);
        return;
    }
    
    zlibInitialized_ = true;
#else
    std::cerr << "Warning: zlib support not available, compression disabled" << std::endl;
#endif
}

void MessageCompressor::cleanupZlib() {
#ifdef BOLT_HAVE_ZLIB
    if (zlibInitialized_) {
        deflateEnd(&compressStream_);
        inflateEnd(&decompressStream_);
        zlibInitialized_ = false;
    }
#endif
}

std::vector<uint8_t> MessageCompressor::compress(const std::string& data) {
    return compress(std::vector<uint8_t>(data.begin(), data.end()));
}

std::vector<uint8_t> MessageCompressor::compress(const std::vector<uint8_t>& data) {
    if (type_ == CompressionType::NONE || !shouldCompress(data)) {
        return data;
    }
    
    stats_.totalBytesIn += data.size();
    stats_.compressionCalls++;
    
    std::vector<uint8_t> result;
    
    switch (type_) {
        case CompressionType::GZIP:
            result = compressGzip(data.data(), data.size());
            break;
        case CompressionType::DEFLATE:
            result = compressDeflate(data.data(), data.size());
            break;
        default:
            result = data;
            break;
    }
    
    stats_.totalBytesOut += result.size();
    return result;
}

std::vector<uint8_t> MessageCompressor::compressGzip(const uint8_t* data, size_t size) {
#ifdef BOLT_HAVE_ZLIB
    if (!zlibInitialized_) {
        return std::vector<uint8_t>(data, data + size);
    }
    
    // Reset compression stream
    deflateReset(&compressStream_);
    
    std::vector<uint8_t> compressed;
    compressed.reserve(size); // Initial guess
    
    compressStream_.next_in = const_cast<uint8_t*>(data);
    compressStream_.avail_in = size;
    
    int flush = Z_FINISH;
    do {
        uint8_t buffer[8192];
        compressStream_.next_out = buffer;
        compressStream_.avail_out = sizeof(buffer);
        
        int result = deflate(&compressStream_, flush);
        if (result != Z_OK && result != Z_STREAM_END) {
            // Compression failed, return original data
            return std::vector<uint8_t>(data, data + size);
        }
        
        size_t bytesCompressed = sizeof(buffer) - compressStream_.avail_out;
        compressed.insert(compressed.end(), buffer, buffer + bytesCompressed);
        
    } while (compressStream_.avail_out == 0);
    
    return compressed;
#else
    // Return original data when zlib is not available
    return std::vector<uint8_t>(data, data + size);
#endif
}

std::vector<uint8_t> MessageCompressor::compressDeflate(const uint8_t* data, size_t size) {
    // Similar to GZIP but without headers
    return compressGzip(data, size);
}

std::string MessageCompressor::decompress(const std::vector<uint8_t>& compressedData) {
    auto decompressed = decompressToBytes(compressedData);
    return std::string(decompressed.begin(), decompressed.end());
}

std::vector<uint8_t> MessageCompressor::decompressToBytes(const std::vector<uint8_t>& compressedData) {
    if (type_ == CompressionType::NONE || !isCompressed(compressedData)) {
        return compressedData;
    }
    
    stats_.decompressionCalls++;
    
    switch (type_) {
        case CompressionType::GZIP:
            return decompressGzip(compressedData.data(), compressedData.size());
        case CompressionType::DEFLATE:
            return decompressDeflate(compressedData.data(), compressedData.size());
        default:
            return compressedData;
    }
}

std::vector<uint8_t> MessageCompressor::decompressGzip(const uint8_t* data, size_t size) {
#ifdef BOLT_HAVE_ZLIB
    if (!zlibInitialized_) {
        return std::vector<uint8_t>(data, data + size);
    }
    
    // Reset decompression stream
    inflateReset(&decompressStream_);
    
    std::vector<uint8_t> decompressed;
    decompressed.reserve(size * 4); // Guess expansion ratio
    
    decompressStream_.next_in = const_cast<uint8_t*>(data);
    decompressStream_.avail_in = size;
    
    do {
        uint8_t buffer[8192];
        decompressStream_.next_out = buffer;
        decompressStream_.avail_out = sizeof(buffer);
        
        int result = inflate(&decompressStream_, Z_NO_FLUSH);
        if (result != Z_OK && result != Z_STREAM_END) {
            // Decompression failed, return original data
            return std::vector<uint8_t>(data, data + size);
        }
        
        size_t bytesDecompressed = sizeof(buffer) - decompressStream_.avail_out;
        decompressed.insert(decompressed.end(), buffer, buffer + bytesDecompressed);
        
    } while (decompressStream_.avail_out == 0);
    
    return decompressed;
#else
    // Return original data when zlib is not available
    return std::vector<uint8_t>(data, data + size);
#endif
}

std::vector<uint8_t> MessageCompressor::decompressDeflate(const uint8_t* data, size_t size) {
    return decompressGzip(data, size);
}

void MessageCompressor::setCompressionLevel(int level) {
    compressionLevel_ = std::max(1, std::min(9, level));
    if (zlibInitialized_) {
        cleanupZlib();
        initializeZlib();
    }
}

void MessageCompressor::setCompressionType(CompressionType type) {
    if (type_ != type) {
        type_ = type;
        if (zlibInitialized_) {
            cleanupZlib();
            initializeZlib();
        }
    }
}

bool MessageCompressor::shouldCompress(const std::string& data) const {
    return data.size() >= minCompressionSize_;
}

bool MessageCompressor::shouldCompress(const std::vector<uint8_t>& data) const {
    return data.size() >= minCompressionSize_;
}

bool MessageCompressor::isCompressed(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return false;
    
    // Check for GZIP magic number
    if (data[0] == 0x1f && data[1] == 0x8b) {
        return true;
    }
    
    // Check for DEFLATE (basic heuristic)
    uint8_t cmf = data[0];
    uint8_t flg = data[1];
    if ((cmf & 0x0f) == 8 && (cmf >> 4) <= 7 && ((cmf * 256 + flg) % 31) == 0) {
        return true;
    }
    
    return false;
}

void MessageCompressor::resetStats() {
    stats_ = CompressionStats{};
}

// WebSocket Compression Implementation

WebSocketCompression::WebSocketCompression()
    : compressor_(CompressionType::DEFLATE, 6),
      serverMaxWindowBits_(15),
      clientMaxWindowBits_(15),
      serverMaxNoContextTakeover_(false),
      clientMaxNoContextTakeover_(false),
      minCompressionSize_(256),
      minCompressionRatio_(0.8) {
}

std::vector<uint8_t> WebSocketCompression::compressMessage(const std::string& message, bool& compressed) {
    compressed = false;
    
    if (message.size() < minCompressionSize_) {
        return std::vector<uint8_t>(message.begin(), message.end());
    }
    
    auto compressedData = compressor_.compress(message);
    double ratio = static_cast<double>(compressedData.size()) / message.size();
    
    if (ratio < minCompressionRatio_) {
        compressed = true;
        return compressedData;
    }
    
    return std::vector<uint8_t>(message.begin(), message.end());
}

std::string WebSocketCompression::decompressMessage(const std::vector<uint8_t>& data, bool isCompressed) {
    if (!isCompressed) {
        return std::string(data.begin(), data.end());
    }
    
    return compressor_.decompress(data);
}

std::string WebSocketCompression::getExtensionOffer() const {
    std::string offer = "permessage-deflate";
    
    if (serverMaxWindowBits_ != 15) {
        offer += "; server_max_window_bits=" + std::to_string(serverMaxWindowBits_);
    }
    
    if (clientMaxWindowBits_ != 15) {
        offer += "; client_max_window_bits=" + std::to_string(clientMaxWindowBits_);
    }
    
    if (serverMaxNoContextTakeover_) {
        offer += "; server_no_context_takeover";
    }
    
    if (clientMaxNoContextTakeover_) {
        offer += "; client_no_context_takeover";
    }
    
    return offer;
}

bool WebSocketCompression::parseExtensionResponse(const std::string& response) {
    // Simple parser for permessage-deflate parameters
    if (response.find("permessage-deflate") == std::string::npos) {
        return false;
    }
    
    // Parse parameters (simplified implementation)
    if (response.find("server_no_context_takeover") != std::string::npos) {
        serverMaxNoContextTakeover_ = true;
    }
    
    if (response.find("client_no_context_takeover") != std::string::npos) {
        clientMaxNoContextTakeover_ = true;
    }
    
    return true;
}

} // namespace bolt