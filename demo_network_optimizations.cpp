#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

// Include network optimization headers
#include "bolt/network/connection_pool.hpp"
#include "bolt/network/message_compression.hpp"
#include "bolt/network/network_buffer.hpp"
#include "bolt/network/network_metrics.hpp"

using namespace bolt;

void demonstrateConnectionPool() {
    std::cout << "\n=== Connection Pool Demo ===\n";
    
    auto& pool = ConnectionPool::getInstance();
    pool.setMaxConnectionsPerHost(3);
    pool.setConnectionTimeout(std::chrono::seconds(5));
    
    std::cout << "Creating connections to localhost:8080...\n";
    
    // Note: These would normally connect to a real server
    // For demo purposes, we'll show the pool management
    std::vector<std::shared_ptr<PooledConnection>> connections;
    
    for (int i = 0; i < 5; i++) {
        auto conn = pool.getConnection("127.0.0.1", 8080);
        if (conn) {
            std::cout << "Connection " << i + 1 << " created/reused\n";
            connections.push_back(conn);
        }
    }
    
    // Release connections back to pool
    for (auto& conn : connections) {
        pool.releaseConnection(conn);
    }
    
    // Show statistics
    const auto& stats = pool.getStats();
    std::cout << "Pool Statistics:\n";
    std::cout << "  Connections Created: " << stats.connectionsCreated << "\n";
    std::cout << "  Connections Reused: " << stats.connectionsReused << "\n";
    std::cout << "  Active Connections: " << pool.getActiveConnections() << "\n";
    std::cout << "  Total Connections: " << pool.getTotalConnections() << "\n";
}

void demonstrateMessageCompression() {
    std::cout << "\n=== Message Compression Demo ===\n";
    
    MessageCompressor compressor(CompressionType::GZIP, 6);
    
    // Create a repetitive message that compresses well
    std::string original = "This is a test message that will be repeated many times. ";
    for (int i = 0; i < 50; i++) {
        original += "This is a test message that will be repeated many times. ";
    }
    
    std::cout << "Original message size: " << original.size() << " bytes\n";
    
    // Compress the message
    auto compressed = compressor.compress(original);
    std::cout << "Compressed size: " << compressed.size() << " bytes\n";
    
    double compressionRatio = static_cast<double>(compressed.size()) / original.size();
    std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) 
              << compressionRatio * 100 << "%\n";
    
    // Decompress and verify
    auto decompressed = compressor.decompress(compressed);
    bool isEqual = (decompressed == original);
    std::cout << "Decompression successful: " << (isEqual ? "Yes" : "No") << "\n";
    
    // Show compression statistics
    auto stats = compressor.getStats();
    std::cout << "Compression Statistics:\n";
    std::cout << "  Compression calls: " << stats.compressionCalls << "\n";
    std::cout << "  Decompression calls: " << stats.decompressionCalls << "\n";
    std::cout << "  Average compression ratio: " << std::fixed << std::setprecision(2) 
              << stats.averageCompressionRatio() * 100 << "%\n";
}

void demonstrateNetworkBuffers() {
    std::cout << "\n=== Network Buffer Demo ===\n";
    
    // Regular NetworkBuffer
    NetworkBuffer buffer(1024);
    
    std::string data1 = "Hello, ";
    std::string data2 = "World!";
    
    buffer.append(data1);
    buffer.append(data2);
    
    std::cout << "Buffer size after appends: " << buffer.size() << "\n";
    std::cout << "Buffer capacity: " << buffer.capacity() << "\n";
    
    auto consumed = buffer.consumeString(5);
    std::cout << "Consumed: '" << consumed << "'\n";
    
    buffer.compact();
    std::cout << "Buffer size after compact: " << buffer.size() << "\n";
    
    // Ring Buffer Demo
    std::cout << "\nRing Buffer Demo:\n";
    RingBuffer ringBuf(20);
    
    std::string testData = "ABCDEFGHIJ";
    size_t written = ringBuf.write(testData.data(), testData.size());
    std::cout << "Written to ring buffer: " << written << " bytes\n";
    std::cout << "Available to read: " << ringBuf.readAvailable() << " bytes\n";
    
    char readBuffer[15];
    size_t read = ringBuf.read(readBuffer, 5);
    readBuffer[read] = '\0';
    std::cout << "Read from ring buffer: '" << readBuffer << "'\n";
    
    // Buffer Pool Demo
    std::cout << "\nBuffer Pool Demo:\n";
    auto& pool = NetworkBufferPool::getInstance();
    
    auto pooledBuffer = pool.getBuffer(2048);
    std::cout << "Got buffer from pool, capacity: " << pooledBuffer->capacity() << "\n";
    std::cout << "Active buffers: " << pool.getActiveBuffers() << "\n";
    
    pool.returnBuffer(std::move(pooledBuffer));
    std::cout << "Returned buffer to pool\n";
    std::cout << "Pool size: " << pool.getPoolSize() << "\n";
}

void demonstrateNetworkMetrics() {
    std::cout << "\n=== Network Metrics Demo ===\n";
    
    auto& metrics = NetworkMetrics::getInstance();
    metrics.resetAllStats();
    
    // Register some endpoints
    std::string endpoint1 = "websocket-server";
    std::string endpoint2 = "http-api";
    
    metrics.registerEndpoint(endpoint1);
    metrics.registerEndpoint(endpoint2);
    
    // Simulate some network activity
    std::cout << "Simulating network activity...\n";
    
    // WebSocket activity
    metrics.recordConnection(endpoint1, true);
    metrics.recordDataSent(endpoint1, 1024);
    metrics.recordDataReceived(endpoint1, 512);
    metrics.recordLatency(endpoint1, 1500); // 1.5ms
    metrics.recordWebSocketFrame(endpoint1, true, false);
    
    // HTTP API activity
    metrics.recordConnection(endpoint2, true);
    metrics.recordDataSent(endpoint2, 2048);
    metrics.recordDataReceived(endpoint2, 4096);
    metrics.recordLatency(endpoint2, 2500); // 2.5ms
    metrics.recordHttpRequest(endpoint2, 200);
    
    // Some errors
    metrics.recordError(endpoint1, "timeout");
    metrics.recordError(endpoint2, "protocol");
    
    // Show global statistics
    auto globalStats = metrics.getGlobalStats();
    std::cout << "\nGlobal Statistics:\n";
    std::cout << "  Connections opened: " << globalStats.connectionsOpened << "\n";
    std::cout << "  Bytes sent: " << globalStats.bytesSent << "\n";
    std::cout << "  Bytes received: " << globalStats.bytesReceived << "\n";
    std::cout << "  Average latency: " << std::fixed << std::setprecision(2) 
              << globalStats.getAverageLatency() << " μs\n";
    
    // Show endpoint-specific statistics
    auto wsStats = metrics.getEndpointStats(endpoint1);
    if (wsStats) {
        std::cout << "\nWebSocket endpoint stats:\n";
        std::cout << "  Frames sent: " << wsStats->framesSent << "\n";
        std::cout << "  Connection timeouts: " << wsStats->connectionsTimedOut << "\n";
    }
    
    auto httpStats = metrics.getEndpointStats(endpoint2);
    if (httpStats) {
        std::cout << "\nHTTP endpoint stats:\n";
        std::cout << "  Requests processed: " << httpStats->requestsProcessed << "\n";
        std::cout << "  Successful responses: " << httpStats->responsesOk << "\n";
        std::cout << "  Protocol errors: " << httpStats->protocolErrors << "\n";
    }
    
    // Generate and display a full report
    std::cout << "\n" << metrics.generateReport() << "\n";
}

void demonstrateBandwidthTracking() {
    std::cout << "\n=== Bandwidth Tracking Demo ===\n";
    
    BandwidthTracker tracker(std::chrono::seconds(5));
    
    std::cout << "Recording data transfer over time...\n";
    
    // Simulate data transfer
    for (int i = 0; i < 10; i++) {
        tracker.recordBytes(1000 + i * 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (i % 3 == 0) {
            double bandwidth = tracker.getCurrentBandwidth();
            std::cout << "Current bandwidth: " << std::fixed << std::setprecision(2) 
                      << bandwidth / 1024.0 << " KB/s\n";
        }
    }
    
    double peakBandwidth = tracker.getPeakBandwidth();
    std::cout << "Peak bandwidth: " << std::fixed << std::setprecision(2) 
              << peakBandwidth / 1024.0 << " KB/s\n";
}

void demonstrateZeroCopyOperations() {
    std::cout << "\n=== Zero-Copy Operations Demo ===\n";
    
    // Zero-copy buffer
    ZeroCopyBuffer zcBuffer;
    
    std::string segment1 = "Header: ";
    std::string segment2 = "Content data goes here";
    std::string segment3 = " | Footer";
    
    zcBuffer.addReference(segment1);
    zcBuffer.addReference(segment2);
    zcBuffer.addReference(segment3);
    
    std::cout << "Zero-copy buffer total size: " << zcBuffer.getTotalSize() << " bytes\n";
    
    // Iterate through segments without copying
    std::cout << "Segments: ";
    zcBuffer.forEachSegment([](const void* data, size_t length) {
        std::cout << "'" << std::string(static_cast<const char*>(data), length) << "' ";
    });
    std::cout << "\n";
    
    // Scatter-gather buffer
    ScatterGatherBuffer sgBuffer;
    sgBuffer.addSegment("Part1");
    sgBuffer.addSegment("Part2");
    sgBuffer.addSegment("Part3");
    
    std::cout << "Scatter-gather buffer:\n";
    std::cout << "  Segments: " << sgBuffer.getSegmentCount() << "\n";
    std::cout << "  Total size: " << sgBuffer.getTotalSize() << " bytes\n";
    
    auto flattened = sgBuffer.flatten();
    std::string result(flattened.begin(), flattened.end());
    std::cout << "  Flattened: '" << result << "'\n";
}

int main() {
    std::cout << "Bolt C++ Network Protocol Optimizations Demo\n";
    std::cout << "============================================\n";
    
    try {
        demonstrateConnectionPool();
        demonstrateMessageCompression();
        demonstrateNetworkBuffers();
        demonstrateNetworkMetrics();
        demonstrateBandwidthTracking();
        demonstrateZeroCopyOperations();
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "All network optimizations demonstrated successfully!\n";
        
        std::cout << "\nKey Optimizations Implemented:\n";
        std::cout << "✓ Connection pooling with keep-alive support\n";
        std::cout << "✓ Message compression (GZIP/DEFLATE)\n";
        std::cout << "✓ High-performance buffer management\n";
        std::cout << "✓ Comprehensive network metrics\n";
        std::cout << "✓ Bandwidth tracking and monitoring\n";
        std::cout << "✓ Zero-copy and scatter-gather operations\n";
        std::cout << "✓ Memory pooling for reduced allocations\n";
        std::cout << "✓ Thread-safe implementations\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Demo error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}