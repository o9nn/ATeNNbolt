#include "bolt/network/network_metrics.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace bolt {

// BandwidthTracker Implementation

BandwidthTracker::BandwidthTracker(std::chrono::seconds windowSize) 
    : windowSize_(windowSize), peakBandwidth_(0.0) {
}

void BandwidthTracker::recordBytes(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    dataPoints_.push_back({now, bytes});
    
    cleanup();
    
    // Update peak bandwidth
    double currentBandwidth = getCurrentBandwidth();
    if (currentBandwidth > peakBandwidth_) {
        peakBandwidth_ = currentBandwidth;
    }
}

double BandwidthTracker::getCurrentBandwidth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (dataPoints_.size() < 2) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - windowSize_;
    
    uint64_t totalBytes = 0;
    size_t validPoints = 0;
    
    for (const auto& point : dataPoints_) {
        if (point.timestamp >= cutoff) {
            totalBytes += point.bytes;
            validPoints++;
        }
    }
    
    if (validPoints < 2) {
        return 0.0;
    }
    
    auto timeSpan = std::chrono::duration_cast<std::chrono::microseconds>(windowSize_);
    double seconds = timeSpan.count() / 1000000.0;
    
    return static_cast<double>(totalBytes) / seconds;
}

double BandwidthTracker::getPeakBandwidth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return peakBandwidth_;
}

void BandwidthTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    dataPoints_.clear();
    peakBandwidth_ = 0.0;
}

void BandwidthTracker::cleanup() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - windowSize_;
    
    dataPoints_.erase(
        std::remove_if(dataPoints_.begin(), dataPoints_.end(),
            [cutoff](const DataPoint& point) {
                return point.timestamp < cutoff;
            }),
        dataPoints_.end()
    );
}

// NetworkMetrics Implementation

void NetworkMetrics::registerEndpoint(const std::string& name) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    if (endpointStats_.find(name) == endpointStats_.end()) {
        endpointStats_.emplace(name, std::make_unique<NetworkStats>());
        
        if (!globalBandwidthIn_) {
            globalBandwidthIn_ = std::make_unique<BandwidthTracker>();
            globalBandwidthOut_ = std::make_unique<BandwidthTracker>();
        }
        
        endpointBandwidthIn_[name] = std::make_unique<BandwidthTracker>();
        endpointBandwidthOut_[name] = std::make_unique<BandwidthTracker>();
    }
}

void NetworkMetrics::unregisterEndpoint(const std::string& name) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    endpointStats_.erase(name);
    endpointBandwidthIn_.erase(name);
    endpointBandwidthOut_.erase(name);
}

void NetworkMetrics::recordConnection(const std::string& endpoint, bool success) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    if (success) {
        globalStats_.connectionsOpened++;
        globalStats_.connectionsActive++;
        
        stats.connectionsOpened++;
        stats.connectionsActive++;
    } else {
        globalStats_.connectionsFailed++;
        
        stats.connectionsFailed++;
    }
}

void NetworkMetrics::recordDisconnection(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    globalStats_.connectionsClosed++;
    if (globalStats_.connectionsActive > 0) {
        globalStats_.connectionsActive--;
    }
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    stats.connectionsClosed++;
    if (stats.connectionsActive > 0) {
        stats.connectionsActive--;
    }
}

void NetworkMetrics::recordDataSent(const std::string& endpoint, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    globalStats_.bytesSent += bytes;
    globalStats_.messagesSent++;
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    stats.bytesSent += bytes;
    stats.messagesSent++;
    
    if (globalBandwidthOut_) {
        globalBandwidthOut_->recordBytes(bytes);
    }
    
    auto& tracker = getOrCreateBandwidthTracker(endpointBandwidthOut_, endpoint);
    tracker.recordBytes(bytes);
}

void NetworkMetrics::recordDataReceived(const std::string& endpoint, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    globalStats_.bytesReceived += bytes;
    globalStats_.messagesReceived++;
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    stats.bytesReceived += bytes;
    stats.messagesReceived++;
    
    if (globalBandwidthIn_) {
        globalBandwidthIn_->recordBytes(bytes);
    }
    
    auto& tracker = getOrCreateBandwidthTracker(endpointBandwidthIn_, endpoint);
    tracker.recordBytes(bytes);
}

void NetworkMetrics::recordLatency(const std::string& endpoint, uint64_t microseconds) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    globalStats_.totalLatency += microseconds;
    globalStats_.latencySamples++;
    
    uint64_t currentMax = globalStats_.maxLatency.load();
    while (microseconds > currentMax && 
           !globalStats_.maxLatency.compare_exchange_weak(currentMax, microseconds));
    
    uint64_t currentMin = globalStats_.minLatency.load();
    while (microseconds < currentMin && 
           !globalStats_.minLatency.compare_exchange_weak(currentMin, microseconds));
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    stats.totalLatency += microseconds;
    stats.latencySamples++;
    
    currentMax = stats.maxLatency.load();
    while (microseconds > currentMax && 
           !stats.maxLatency.compare_exchange_weak(currentMax, microseconds));
    
    currentMin = stats.minLatency.load();
    while (microseconds < currentMin && 
           !stats.minLatency.compare_exchange_weak(currentMin, microseconds));
}

void NetworkMetrics::recordError(const std::string& endpoint, const std::string& errorType) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    
    if (errorType == "send") {
        globalStats_.sendErrors++;
        stats.sendErrors++;
    } else if (errorType == "receive") {
        globalStats_.receiveErrors++;
        stats.receiveErrors++;
    } else if (errorType == "protocol") {
        globalStats_.protocolErrors++;
        stats.protocolErrors++;
    } else if (errorType == "compression") {
        globalStats_.compressionErrors++;
        stats.compressionErrors++;
    } else if (errorType == "timeout") {
        globalStats_.connectionsTimedOut++;
        stats.connectionsTimedOut++;
    }
}

void NetworkMetrics::recordWebSocketFrame(const std::string& endpoint, bool sent, bool received) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto& stats = getOrCreateEndpointStats(endpoint);
    
    if (sent) {
        globalStats_.framesSent++;
        stats.framesSent++;
    }
    
    if (received) {
        globalStats_.framesReceived++;
        stats.framesReceived++;
    }
}

void NetworkMetrics::recordHttpRequest(const std::string& endpoint, int statusCode) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    globalStats_.requestsProcessed++;
    auto& stats = getOrCreateEndpointStats(endpoint);
    stats.requestsProcessed++;
    
    if (statusCode >= 200 && statusCode < 300) {
        globalStats_.responsesOk++;
        stats.responsesOk++;
    } else {
        globalStats_.responseErrors++;
        stats.responseErrors++;
    }
}

NetworkStats NetworkMetrics::getGlobalStats() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return globalStats_.copy();
}

std::shared_ptr<NetworkStats> NetworkMetrics::getEndpointStats(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = endpointStats_.find(endpoint);
    if (it != endpointStats_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<std::string> NetworkMetrics::getEndpoints() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    std::vector<std::string> endpoints;
    endpoints.reserve(endpointStats_.size());
    
    for (const auto& [name, stats] : endpointStats_) {
        endpoints.push_back(name);
    }
    
    return endpoints;
}

double NetworkMetrics::getGlobalBandwidthIn() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return globalBandwidthIn_ ? globalBandwidthIn_->getCurrentBandwidth() : 0.0;
}

double NetworkMetrics::getGlobalBandwidthOut() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return globalBandwidthOut_ ? globalBandwidthOut_->getCurrentBandwidth() : 0.0;
}

double NetworkMetrics::getEndpointBandwidthIn(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = endpointBandwidthIn_.find(endpoint);
    return it != endpointBandwidthIn_.end() ? it->second->getCurrentBandwidth() : 0.0;
}

double NetworkMetrics::getEndpointBandwidthOut(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = endpointBandwidthOut_.find(endpoint);
    return it != endpointBandwidthOut_.end() ? it->second->getCurrentBandwidth() : 0.0;
}

void NetworkMetrics::resetGlobalStats() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    globalStats_.reset();
    
    if (globalBandwidthIn_) {
        globalBandwidthIn_->reset();
    }
    if (globalBandwidthOut_) {
        globalBandwidthOut_->reset();
    }
}

void NetworkMetrics::resetEndpointStats(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = endpointStats_.find(endpoint);
    if (it != endpointStats_.end()) {
        it->second->reset();
    }
    
    auto inIt = endpointBandwidthIn_.find(endpoint);
    if (inIt != endpointBandwidthIn_.end()) {
        inIt->second->reset();
    }
    
    auto outIt = endpointBandwidthOut_.find(endpoint);
    if (outIt != endpointBandwidthOut_.end()) {
        outIt->second->reset();
    }
}

void NetworkMetrics::resetAllStats() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    globalStats_.reset();
    
    for (auto& [name, stats] : endpointStats_) {
        stats->reset();
    }
    
    if (globalBandwidthIn_) {
        globalBandwidthIn_->reset();
    }
    if (globalBandwidthOut_) {
        globalBandwidthOut_->reset();
    }
    
    for (auto& [name, tracker] : endpointBandwidthIn_) {
        tracker->reset();
    }
    for (auto& [name, tracker] : endpointBandwidthOut_) {
        tracker->reset();
    }
}

void NetworkMetrics::setBandwidthWindow(std::chrono::seconds window) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    // Recreate trackers with new window size
    if (globalBandwidthIn_) {
        globalBandwidthIn_ = std::make_unique<BandwidthTracker>(window);
        globalBandwidthOut_ = std::make_unique<BandwidthTracker>(window);
    }
    
    for (auto& [name, tracker] : endpointBandwidthIn_) {
        tracker = std::make_unique<BandwidthTracker>(window);
    }
    for (auto& [name, tracker] : endpointBandwidthOut_) {
        tracker = std::make_unique<BandwidthTracker>(window);
    }
}

std::string NetworkMetrics::generateReport() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    std::ostringstream report;
    
    report << "=== Network Performance Report ===\n\n";
    
    // Global statistics
    report << "Global Statistics:\n";
    report << "  Connections: " << globalStats_.connectionsOpened.load() << " opened, "
           << globalStats_.connectionsClosed.load() << " closed, "
           << globalStats_.connectionsActive.load() << " active, "
           << globalStats_.connectionsFailed.load() << " failed\n";
    
    report << "  Data Transfer: " << globalStats_.bytesReceived.load() << " bytes received, "
           << globalStats_.bytesSent.load() << " bytes sent\n";
    
    report << "  Messages: " << globalStats_.messagesReceived.load() << " received, "
           << globalStats_.messagesSent.load() << " sent\n";
    
    if (globalStats_.latencySamples.load() > 0) {
        report << "  Latency: avg=" << std::fixed << std::setprecision(2) 
               << globalStats_.getAverageLatency() << "μs, "
               << "min=" << globalStats_.minLatency.load() << "μs, "
               << "max=" << globalStats_.maxLatency.load() << "μs\n";
    }
    
    report << "  Bandwidth: in=" << std::fixed << std::setprecision(2) 
           << getGlobalBandwidthIn() / 1024.0 << " KB/s, "
           << "out=" << getGlobalBandwidthOut() / 1024.0 << " KB/s\n";
    
    report << "  Errors: send=" << globalStats_.sendErrors.load()
           << ", receive=" << globalStats_.receiveErrors.load()
           << ", protocol=" << globalStats_.protocolErrors.load() << "\n\n";
    
    // Endpoint statistics
    if (!endpointStats_.empty()) {
        report << "Endpoint Statistics:\n";
        for (const auto& [name, stats] : endpointStats_) {
            report << "  " << name << ":\n";
            report << "    Connections: " << stats->connectionsOpened.load() << " opened, "
                   << stats->connectionsActive.load() << " active\n";
            report << "    Data: " << stats->bytesReceived.load() << " bytes in, "
                   << stats->bytesSent.load() << " bytes out\n";
            
            if (stats->latencySamples.load() > 0) {
                report << "    Latency: " << std::fixed << std::setprecision(2) 
                       << stats->getAverageLatency() << "μs avg\n";
            }
            
            report << "    Bandwidth: in=" << std::fixed << std::setprecision(2) 
                   << getEndpointBandwidthIn(name) / 1024.0 << " KB/s, "
                   << "out=" << getEndpointBandwidthOut(name) / 1024.0 << " KB/s\n";
        }
    }
    
    return report.str();
}

std::string NetworkMetrics::generateEndpointReport(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = endpointStats_.find(endpoint);
    if (it == endpointStats_.end()) {
        return "Endpoint not found: " + endpoint;
    }
    
    const auto& stats = *it->second;
    std::ostringstream report;
    
    report << "=== Endpoint Report: " << endpoint << " ===\n\n";
    
    report << "Connections:\n";
    report << "  Opened: " << stats.connectionsOpened.load() << "\n";
    report << "  Closed: " << stats.connectionsClosed.load() << "\n";
    report << "  Active: " << stats.connectionsActive.load() << "\n";
    report << "  Failed: " << stats.connectionsFailed.load() << "\n";
    report << "  Timed Out: " << stats.connectionsTimedOut.load() << "\n\n";
    
    report << "Data Transfer:\n";
    report << "  Bytes Received: " << stats.bytesReceived.load() << "\n";
    report << "  Bytes Sent: " << stats.bytesSent.load() << "\n";
    report << "  Messages Received: " << stats.messagesReceived.load() << "\n";
    report << "  Messages Sent: " << stats.messagesSent.load() << "\n\n";
    
    if (stats.latencySamples.load() > 0) {
        report << "Latency:\n";
        report << "  Average: " << std::fixed << std::setprecision(2) 
               << stats.getAverageLatency() << " μs\n";
        report << "  Minimum: " << stats.minLatency.load() << " μs\n";
        report << "  Maximum: " << stats.maxLatency.load() << " μs\n";
        report << "  Samples: " << stats.latencySamples.load() << "\n\n";
    }
    
    report << "Bandwidth:\n";
    report << "  Inbound: " << std::fixed << std::setprecision(2) 
           << getEndpointBandwidthIn(endpoint) / 1024.0 << " KB/s\n";
    report << "  Outbound: " << getEndpointBandwidthOut(endpoint) / 1024.0 << " KB/s\n\n";
    
    report << "Errors:\n";
    report << "  Send Errors: " << stats.sendErrors.load() << "\n";
    report << "  Receive Errors: " << stats.receiveErrors.load() << "\n";
    report << "  Protocol Errors: " << stats.protocolErrors.load() << "\n";
    report << "  Compression Errors: " << stats.compressionErrors.load() << "\n\n";
    
    return report.str();
}

NetworkStats& NetworkMetrics::getOrCreateEndpointStats(const std::string& endpoint) {
    auto it = endpointStats_.find(endpoint);
    if (it == endpointStats_.end()) {
        endpointStats_[endpoint] = std::make_shared<NetworkStats>();
        it = endpointStats_.find(endpoint);
    }
    return *it->second;
}

BandwidthTracker& NetworkMetrics::getOrCreateBandwidthTracker(
    std::unordered_map<std::string, std::unique_ptr<BandwidthTracker>>& trackers,
    const std::string& endpoint) {
    
    auto it = trackers.find(endpoint);
    if (it == trackers.end()) {
        trackers[endpoint] = std::make_unique<BandwidthTracker>();
        it = trackers.find(endpoint);
    }
    return *it->second;
}

} // namespace bolt