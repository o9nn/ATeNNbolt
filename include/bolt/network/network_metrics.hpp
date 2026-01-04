#ifndef NETWORK_METRICS_HPP
#define NETWORK_METRICS_HPP

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

namespace bolt {

/**
 * Network performance metrics and statistics
 */
struct NetworkStats {
    // Connection metrics
    std::atomic<uint64_t> connectionsOpened{0};
    std::atomic<uint64_t> connectionsClosed{0};
    std::atomic<uint64_t> connectionsActive{0};
    std::atomic<uint64_t> connectionsFailed{0};
    std::atomic<uint64_t> connectionsTimedOut{0};
    
    // Data transfer metrics
    std::atomic<uint64_t> bytesReceived{0};
    std::atomic<uint64_t> bytesSent{0};
    std::atomic<uint64_t> messagesReceived{0};
    std::atomic<uint64_t> messagesSent{0};
    
    // Performance metrics
    std::atomic<uint64_t> totalLatency{0};  // microseconds
    std::atomic<uint64_t> latencySamples{0};
    std::atomic<uint64_t> maxLatency{0};    // microseconds
    std::atomic<uint64_t> minLatency{UINT64_MAX};
    
    // Error metrics
    std::atomic<uint64_t> sendErrors{0};
    std::atomic<uint64_t> receiveErrors{0};
    std::atomic<uint64_t> protocolErrors{0};
    std::atomic<uint64_t> compressionErrors{0};
    
    // WebSocket specific
    std::atomic<uint64_t> framesReceived{0};
    std::atomic<uint64_t> framesSent{0};
    std::atomic<uint64_t> pingSent{0};
    std::atomic<uint64_t> pongReceived{0};
    
    // HTTP specific
    std::atomic<uint64_t> requestsProcessed{0};
    std::atomic<uint64_t> responsesOk{0};
    std::atomic<uint64_t> responseErrors{0};
    
    // Disable copy constructor and assignment
    NetworkStats() = default;
    NetworkStats(const NetworkStats&) = delete;
    NetworkStats& operator=(const NetworkStats&) = delete;
    
    // Enable move operations
    NetworkStats(NetworkStats&& other) noexcept {
        *this = std::move(other);
    }
    
    NetworkStats& operator=(NetworkStats&& other) noexcept {
        if (this != &other) {
            connectionsOpened.store(other.connectionsOpened.load());
            connectionsClosed.store(other.connectionsClosed.load());
            connectionsActive.store(other.connectionsActive.load());
            connectionsFailed.store(other.connectionsFailed.load());
            connectionsTimedOut.store(other.connectionsTimedOut.load());
            
            bytesReceived.store(other.bytesReceived.load());
            bytesSent.store(other.bytesSent.load());
            messagesReceived.store(other.messagesReceived.load());
            messagesSent.store(other.messagesSent.load());
            
            totalLatency.store(other.totalLatency.load());
            latencySamples.store(other.latencySamples.load());
            maxLatency.store(other.maxLatency.load());
            minLatency.store(other.minLatency.load());
            
            sendErrors.store(other.sendErrors.load());
            receiveErrors.store(other.receiveErrors.load());
            protocolErrors.store(other.protocolErrors.load());
            compressionErrors.store(other.compressionErrors.load());
            
            framesReceived.store(other.framesReceived.load());
            framesSent.store(other.framesSent.load());
            pingSent.store(other.pingSent.load());
            pongReceived.store(other.pongReceived.load());
            
            requestsProcessed.store(other.requestsProcessed.load());
            responsesOk.store(other.responsesOk.load());
            responseErrors.store(other.responseErrors.load());
        }
        return *this;
    }
    
    double getAverageLatency() const {
        uint64_t samples = latencySamples.load();
        return samples > 0 ? static_cast<double>(totalLatency.load()) / samples : 0.0;
    }
    
    void reset() {
        connectionsOpened = 0;
        connectionsClosed = 0;
        connectionsActive = 0;
        connectionsFailed = 0;
        connectionsTimedOut = 0;
        
        bytesReceived = 0;
        bytesSent = 0;
        messagesReceived = 0;
        messagesSent = 0;
        
        totalLatency = 0;
        latencySamples = 0;
        maxLatency = 0;
        minLatency = UINT64_MAX;
        
        sendErrors = 0;
        receiveErrors = 0;
        protocolErrors = 0;
        compressionErrors = 0;
        
        framesReceived = 0;
        framesSent = 0;
        pingSent = 0;
        pongReceived = 0;
        
        requestsProcessed = 0;
        responsesOk = 0;
        responseErrors = 0;
    }
    
    // Create a copy for returning by value
    NetworkStats copy() const {
        NetworkStats result;
        result.connectionsOpened = connectionsOpened.load();
        result.connectionsClosed = connectionsClosed.load();
        result.connectionsActive = connectionsActive.load();
        result.connectionsFailed = connectionsFailed.load();
        result.connectionsTimedOut = connectionsTimedOut.load();
        
        result.bytesReceived = bytesReceived.load();
        result.bytesSent = bytesSent.load();
        result.messagesReceived = messagesReceived.load();
        result.messagesSent = messagesSent.load();
        
        result.totalLatency = totalLatency.load();
        result.latencySamples = latencySamples.load();
        result.maxLatency = maxLatency.load();
        result.minLatency = minLatency.load();
        
        result.sendErrors = sendErrors.load();
        result.receiveErrors = receiveErrors.load();
        result.protocolErrors = protocolErrors.load();
        result.compressionErrors = compressionErrors.load();
        
        result.framesReceived = framesReceived.load();
        result.framesSent = framesSent.load();
        result.pingSent = pingSent.load();
        result.pongReceived = pongReceived.load();
        
        result.requestsProcessed = requestsProcessed.load();
        result.responsesOk = responsesOk.load();
        result.responseErrors = responseErrors.load();
        
        return result;
    }
};

/**
 * Latency measurement helper
 */
class LatencyMeasurer {
public:
    LatencyMeasurer() : startTime_(std::chrono::high_resolution_clock::now()) {}
    
    uint64_t getMicroseconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - startTime_).count();
    }
    
    void recordLatency(NetworkStats& stats) {
        uint64_t latency = getMicroseconds();
        stats.totalLatency += latency;
        stats.latencySamples++;
        
        // Update min/max atomically
        uint64_t currentMax = stats.maxLatency.load();
        while (latency > currentMax && !stats.maxLatency.compare_exchange_weak(currentMax, latency));
        
        uint64_t currentMin = stats.minLatency.load();
        while (latency < currentMin && !stats.minLatency.compare_exchange_weak(currentMin, latency));
    }
    
private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

/**
 * Bandwidth tracker for monitoring data rates
 */
class BandwidthTracker {
public:
    BandwidthTracker(std::chrono::seconds windowSize = std::chrono::seconds(60));
    
    void recordBytes(uint64_t bytes);
    double getCurrentBandwidth() const; // bytes per second
    double getPeakBandwidth() const;
    void reset();
    
private:
    struct DataPoint {
        std::chrono::steady_clock::time_point timestamp;
        uint64_t bytes;
    };
    
    mutable std::mutex mutex_;
    std::vector<DataPoint> dataPoints_;
    std::chrono::seconds windowSize_;
    double peakBandwidth_;
    
    void cleanup();
};

/**
 * Network metrics collector and aggregator
 */
class NetworkMetrics {
public:
    static NetworkMetrics& getInstance() {
        static NetworkMetrics instance;
        return instance;
    }
    
    // Metrics registration
    void registerEndpoint(const std::string& name);
    void unregisterEndpoint(const std::string& name);
    
    // Metrics recording
    void recordConnection(const std::string& endpoint, bool success);
    void recordDisconnection(const std::string& endpoint);
    void recordDataSent(const std::string& endpoint, uint64_t bytes);
    void recordDataReceived(const std::string& endpoint, uint64_t bytes);
    void recordLatency(const std::string& endpoint, uint64_t microseconds);
    void recordError(const std::string& endpoint, const std::string& errorType);
    void recordWebSocketFrame(const std::string& endpoint, bool sent, bool received);
    void recordHttpRequest(const std::string& endpoint, int statusCode);
    
    // Metrics retrieval
    NetworkStats getGlobalStats() const;
    std::shared_ptr<NetworkStats> getEndpointStats(const std::string& endpoint) const;
    std::vector<std::string> getEndpoints() const;
    
    // Bandwidth monitoring
    double getGlobalBandwidthIn() const;
    double getGlobalBandwidthOut() const;
    double getEndpointBandwidthIn(const std::string& endpoint) const;
    double getEndpointBandwidthOut(const std::string& endpoint) const;
    
    // Reset functions
    void resetGlobalStats();
    void resetEndpointStats(const std::string& endpoint);
    void resetAllStats();
    
    // Configuration
    void setBandwidthWindow(std::chrono::seconds window);
    void enableDetailedMetrics(bool enable) { detailedMetrics_ = enable; }
    
    // Report generation
    std::string generateReport() const;
    std::string generateEndpointReport(const std::string& endpoint) const;

private:
    NetworkMetrics() : detailedMetrics_(true) {}
    
    mutable std::mutex metricsMutex_;
    NetworkStats globalStats_;
    std::unordered_map<std::string, std::shared_ptr<NetworkStats>> endpointStats_;
    
    // Bandwidth trackers
    std::unique_ptr<BandwidthTracker> globalBandwidthIn_;
    std::unique_ptr<BandwidthTracker> globalBandwidthOut_;
    std::unordered_map<std::string, std::unique_ptr<BandwidthTracker>> endpointBandwidthIn_;
    std::unordered_map<std::string, std::unique_ptr<BandwidthTracker>> endpointBandwidthOut_;
    
    bool detailedMetrics_;
    
    NetworkStats& getOrCreateEndpointStats(const std::string& endpoint);
    BandwidthTracker& getOrCreateBandwidthTracker(
        std::unordered_map<std::string, std::unique_ptr<BandwidthTracker>>& trackers,
        const std::string& endpoint
    );
};

/**
 * RAII helper for automatic metric recording
 */
class MetricRecorder {
public:
    explicit MetricRecorder(const std::string& endpoint) 
        : endpoint_(endpoint), latencyMeasurer_() {
        NetworkMetrics::getInstance().recordConnection(endpoint_, true);
    }
    
    ~MetricRecorder() {
        auto endpointStats = NetworkMetrics::getInstance().getEndpointStats(endpoint_);
        if (endpointStats) {
            latencyMeasurer_.recordLatency(*endpointStats);
        }
        NetworkMetrics::getInstance().recordDisconnection(endpoint_);
    }
    
    void recordDataSent(uint64_t bytes) {
        NetworkMetrics::getInstance().recordDataSent(endpoint_, bytes);
    }
    
    void recordDataReceived(uint64_t bytes) {
        NetworkMetrics::getInstance().recordDataReceived(endpoint_, bytes);
    }
    
    void recordError(const std::string& errorType) {
        NetworkMetrics::getInstance().recordError(endpoint_, errorType);
    }
    
private:
    std::string endpoint_;
    LatencyMeasurer latencyMeasurer_;
};

} // namespace bolt

#endif