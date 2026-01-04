#ifndef MESSAGE_COMPRESSION_HPP
#define MESSAGE_COMPRESSION_HPP

#include <string>
#include <vector>
#include <memory>

#ifdef BOLT_HAVE_ZLIB
    #include <zlib.h>
#endif

namespace bolt {

/**
 * Compression algorithm types supported
 */
enum class CompressionType {
    NONE,
    GZIP,
    DEFLATE,
    LZ4,  // Future implementation
    ZSTD  // Future implementation
};

/**
 * Compression statistics for monitoring
 */
struct CompressionStats {
    size_t totalBytesIn = 0;
    size_t totalBytesOut = 0;
    size_t compressionCalls = 0;
    size_t decompressionCalls = 0;
    double averageCompressionRatio() const {
        return totalBytesIn > 0 ? static_cast<double>(totalBytesOut) / totalBytesIn : 1.0;
    }
};

/**
 * High-performance message compression for network protocols
 */
class MessageCompressor {
public:
    MessageCompressor(CompressionType type = CompressionType::GZIP, int level = 6);
    ~MessageCompressor();

    // Compression operations
    std::vector<uint8_t> compress(const std::string& data);
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    std::string decompress(const std::vector<uint8_t>& compressedData);
    std::vector<uint8_t> decompressToBytes(const std::vector<uint8_t>& compressedData);
    
    // Configuration
    void setCompressionLevel(int level);
    void setCompressionType(CompressionType type);
    void setMinCompressionSize(size_t minSize) { minCompressionSize_ = minSize; }
    
    // Statistics
    const CompressionStats& getStats() const { return stats_; }
    void resetStats();
    
    // Utility methods
    bool shouldCompress(const std::string& data) const;
    bool shouldCompress(const std::vector<uint8_t>& data) const;
    static bool isCompressed(const std::vector<uint8_t>& data);

private:
    void initializeZlib();
    void cleanupZlib();
    
    CompressionType type_;
    int compressionLevel_;
    size_t minCompressionSize_;
    
#ifdef BOLT_HAVE_ZLIB
    // Zlib streams for reuse
    z_stream compressStream_;
    z_stream decompressStream_;
#endif
    bool zlibInitialized_;
    
    CompressionStats stats_;
    
    // Internal compression methods
    std::vector<uint8_t> compressGzip(const uint8_t* data, size_t size);
    std::vector<uint8_t> compressDeflate(const uint8_t* data, size_t size);
    std::vector<uint8_t> decompressGzip(const uint8_t* data, size_t size);
    std::vector<uint8_t> decompressDeflate(const uint8_t* data, size_t size);
};

/**
 * WebSocket extension for per-message-deflate compression
 */
class WebSocketCompression {
public:
    WebSocketCompression();
    
    // WebSocket specific compression methods
    std::vector<uint8_t> compressMessage(const std::string& message, bool& compressed);
    std::string decompressMessage(const std::vector<uint8_t>& data, bool isCompressed);
    
    // Extension negotiation
    std::string getExtensionOffer() const;
    bool parseExtensionResponse(const std::string& response);
    
    // Configuration
    void setServerMaxWindowBits(int bits) { serverMaxWindowBits_ = bits; }
    void setClientMaxWindowBits(int bits) { clientMaxWindowBits_ = bits; }
    void setServerMaxNoContextTakeover(bool enabled) { serverMaxNoContextTakeover_ = enabled; }
    void setClientMaxNoContextTakeover(bool enabled) { clientMaxNoContextTakeover_ = enabled; }

private:
    MessageCompressor compressor_;
    
    // WebSocket compression parameters
    int serverMaxWindowBits_;
    int clientMaxWindowBits_;
    bool serverMaxNoContextTakeover_;
    bool clientMaxNoContextTakeover_;
    
    // Compression thresholds
    size_t minCompressionSize_;
    double minCompressionRatio_;
};

} // namespace bolt

#endif