#ifndef OPTIMIZED_WEBSOCKET_SERVER_HPP
#define OPTIMIZED_WEBSOCKET_SERVER_HPP

#include "bolt/network/websocket_server.hpp"
#include "bolt/network/connection_pool.hpp"
#include "bolt/network/message_compression.hpp"
#include "bolt/network/network_buffer.hpp"
#include "bolt/network/network_metrics.hpp"
#include <chrono>
#include <thread>
#include <atomic>

namespace bolt {

/**
 * High-performance WebSocket connection with optimizations
 */
class OptimizedWebSocketConnection : public WebSocketConnection {
public:
    OptimizedWebSocketConnection(int socket, const std::string& endpoint);
    ~OptimizedWebSocketConnection();
    
    // Enhanced send operations
    void sendOptimized(const std::string& message, bool compress = true);
    void sendBinary(const std::vector<uint8_t>& data, bool compress = true);
    void sendPing(const std::string& payload = "");
    void sendPong(const std::string& payload = "");
    
    // Connection management
    void enableKeepAlive(std::chrono::seconds interval = std::chrono::seconds(30));
    void disableKeepAlive();
    bool isAlive() const;
    void setCompression(bool enabled) { compressionEnabled_ = enabled; }
    
    // Statistics
    size_t getBytesSent() const { return bytesSent_; }
    size_t getBytesReceived() const { return bytesReceived_; }
    std::chrono::steady_clock::time_point getLastActivity() const { return lastActivity_; }
    
    // Buffer management
    void setReceiveBufferSize(size_t size);
    void setSendBufferSize(size_t size);
    
private:
    std::string endpoint_;
    std::unique_ptr<NetworkBuffer> receiveBuffer_;
    std::unique_ptr<NetworkBuffer> sendBuffer_;
    std::unique_ptr<WebSocketCompression> compression_;
    
    // Keep-alive management
    std::atomic<bool> keepAliveEnabled_;
    std::chrono::seconds keepAliveInterval_;
    std::thread keepAliveThread_;
    std::chrono::steady_clock::time_point lastPongReceived_;
    
    // Statistics
    std::atomic<size_t> bytesSent_;
    std::atomic<size_t> bytesReceived_;
    std::chrono::steady_clock::time_point lastActivity_;
    bool compressionEnabled_;
    
    void startKeepAlive();
    void stopKeepAlive();
    void keepAliveWorker();
    void updateLastActivity();
    
    // Frame processing
    std::vector<uint8_t> createOptimizedFrame(const std::vector<uint8_t>& payload, 
                                             uint8_t opcode, bool compressed = false);
    bool processIncomingFrame(const std::vector<uint8_t>& frame);
};

/**
 * High-performance WebSocket server with all optimizations
 */
class OptimizedWebSocketServer {
public:
    static OptimizedWebSocketServer& getInstance() {
        static OptimizedWebSocketServer instance;
        return instance;
    }
    
    // Server configuration
    void setMaxConnections(size_t max) { maxConnections_ = max; }
    void setKeepAliveInterval(std::chrono::seconds interval) { keepAliveInterval_ = interval; }
    void setCompressionEnabled(bool enabled) { compressionEnabled_ = enabled; }
    void setMetricsEnabled(bool enabled) { metricsEnabled_ = enabled; }
    
    // Server lifecycle
    void start(int port = 8080, size_t threadPoolSize = 4);
    void stop();
    bool isRunning() const { return running_; }
    
    // Connection management
    void broadcast(const std::string& message, bool compress = true);
    void broadcastBinary(const std::vector<uint8_t>& data, bool compress = true);
    void broadcastToEndpoint(const std::string& endpoint, const std::string& message);
    size_t getConnectionCount() const;
    std::vector<std::string> getConnectedEndpoints() const;
    
    // Event handlers
    void onMessage(std::function<void(const std::string&, OptimizedWebSocketConnection*, bool)> callback) {
        messageCallback_ = callback;
    }
    
    void onConnect(std::function<void(OptimizedWebSocketConnection*)> callback) {
        connectCallback_ = callback;
    }
    
    void onDisconnect(std::function<void(OptimizedWebSocketConnection*)> callback) {
        disconnectCallback_ = callback;
    }
    
    void onError(std::function<void(OptimizedWebSocketConnection*, const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Performance monitoring
    NetworkStats getServerStats() const;
    std::string generatePerformanceReport() const;
    void resetMetrics();
    
    // Advanced features
    void enableRateLimiting(size_t messagesPerSecond, size_t bytesPerSecond);
    void disableRateLimiting();
    void setMaxMessageSize(size_t maxSize) { maxMessageSize_ = maxSize; }
    void enableConnectionPooling(bool enabled) { connectionPoolingEnabled_ = enabled; }

private:
    OptimizedWebSocketServer() : maxConnections_(1000), 
                                keepAliveInterval_(std::chrono::seconds(30)),
                                compressionEnabled_(true),
                                metricsEnabled_(true),
                                running_(false),
                                maxMessageSize_(64 * 1024 * 1024), // 64MB
                                connectionPoolingEnabled_(true),
                                rateLimitEnabled_(false) {}
    
    ~OptimizedWebSocketServer() { stop(); }
    
    // Server configuration
    size_t maxConnections_;
    std::chrono::seconds keepAliveInterval_;
    bool compressionEnabled_;
    bool metricsEnabled_;
    
    // Server state
    std::atomic<bool> running_;
    int serverSocket_;
    std::vector<std::thread> threadPool_;
    
    // Connection management
    mutable std::mutex connectionsMutex_;
    std::unordered_map<std::string, std::vector<OptimizedWebSocketConnection*>> connections_;
    std::unique_ptr<NetworkBufferPool> bufferPool_;
    
    // Event callbacks
    std::function<void(const std::string&, OptimizedWebSocketConnection*, bool)> messageCallback_;
    std::function<void(OptimizedWebSocketConnection*)> connectCallback_;
    std::function<void(OptimizedWebSocketConnection*)> disconnectCallback_;
    std::function<void(OptimizedWebSocketConnection*, const std::string&)> errorCallback_;
    
    // Performance features
    size_t maxMessageSize_;
    bool connectionPoolingEnabled_;
    
    // Rate limiting
    bool rateLimitEnabled_;
    size_t maxMessagesPerSecond_;
    size_t maxBytesPerSecond_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastMessageTime_;
    std::unordered_map<std::string, size_t> messageCount_;
    std::unordered_map<std::string, size_t> byteCount_;
    
    // Worker methods
    void acceptConnections();
    void handleClient(int clientSocket);
    void workerThread();
    
    // Connection management
    void addConnection(const std::string& endpoint, OptimizedWebSocketConnection* conn);
    void removeConnection(const std::string& endpoint, OptimizedWebSocketConnection* conn);
    
    // Rate limiting
    bool checkRateLimit(const std::string& endpoint, size_t messageSize);
    void updateRateLimitCounters(const std::string& endpoint, size_t messageSize);
    void cleanupRateLimitCounters();
    
    // Utility methods
    std::string extractEndpointFromRequest(const std::string& request);
    bool performOptimizedHandshake(int socket, std::string& endpoint);
};

/**
 * WebSocket frame parser optimized for high throughput
 */
class OptimizedFrameParser {
public:
    struct Frame {
        bool fin;
        uint8_t opcode;
        bool masked;
        uint64_t payloadLength;
        uint8_t mask[4];
        std::vector<uint8_t> payload;
        bool compressed;
        
        bool isControlFrame() const {
            return opcode >= 0x08;
        }
        
        bool isDataFrame() const {
            return opcode == 0x01 || opcode == 0x02;
        }
        
        bool isPing() const {
            return opcode == 0x09;
        }
        
        bool isPong() const {
            return opcode == 0x0A;
        }
        
        bool isClose() const {
            return opcode == 0x08;
        }
    };
    
    // Parse frames from buffer
    std::vector<Frame> parseFrames(NetworkBuffer& buffer);
    
    // Create frames
    static std::vector<uint8_t> createFrame(const std::vector<uint8_t>& payload, 
                                           uint8_t opcode, bool compressed = false);
    static std::vector<uint8_t> createPingFrame(const std::string& payload = "");
    static std::vector<uint8_t> createPongFrame(const std::string& payload = "");
    static std::vector<uint8_t> createCloseFrame(uint16_t code = 1000, 
                                                const std::string& reason = "");

private:
    // Internal parsing state
    enum class ParseState {
        HEADER,
        EXTENDED_LENGTH,
        MASK,
        PAYLOAD
    };
    
    ParseState state_ = ParseState::HEADER;
    Frame currentFrame_;
    size_t bytesNeeded_ = 2; // Start with basic header
    size_t bytesRead_ = 0;
    
    bool parseHeader(NetworkBuffer& buffer);
    bool parseExtendedLength(NetworkBuffer& buffer);
    bool parseMask(NetworkBuffer& buffer);
    bool parsePayload(NetworkBuffer& buffer);
    void resetParser();
};

} // namespace bolt

#endif