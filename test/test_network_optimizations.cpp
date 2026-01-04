#include "bolt/test_framework.hpp"
#include "bolt/network/connection_pool.hpp"
#include "bolt/network/message_compression.hpp"
#include "bolt/network/network_buffer.hpp"
#include "bolt/network/network_metrics.hpp"
#include <thread>
#include <chrono>

namespace bolt {
namespace test {

// Helper assertion macros for numeric comparisons
#define BOLT_ASSERT_GT(a, b) \
    do { \
        if (!((a) > (b))) { \
            std::stringstream ss; \
            ss << "Expected " << (a) << " > " << (b); \
            throw bolt::test::TestFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_LT(a, b) \
    do { \
        if (!((a) < (b))) { \
            std::stringstream ss; \
            ss << "Expected " << (a) << " < " << (b); \
            throw bolt::test::TestFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define BOLT_ASSERT_GE(a, b) \
    do { \
        if (!((a) >= (b))) { \
            std::stringstream ss; \
            ss << "Expected " << (a) << " >= " << (b); \
            throw bolt::test::TestFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

// Reset metrics before tests
static void setupNetworkTest() {
    NetworkMetrics::getInstance().resetAllStats();
    // Set short timeout for tests to avoid long waits in CI
    ConnectionPool::getInstance().setConnectionTimeout(std::chrono::seconds(1));
}

// Connection Pool Tests
// NOTE: These tests require actual network connections and may fail in CI
// environments without a test server. They test the connection pool logic
// but skip actual socket operations if connections fail.
BOLT_TEST(NetworkOptimizations, ConnectionPoolBasicOperations) {
    setupNetworkTest();

    auto& pool = ConnectionPool::getInstance();
    pool.setMaxConnectionsPerHost(5);

    // Test connection creation
    // Note: This may return nullptr in CI environments without a test server
    auto conn1 = pool.getConnection("127.0.0.1", 8080);
    if (!conn1) {
        // Skip test if connection fails (no server listening)
        return;
    }
    BOLT_ASSERT_NOT_NULL(conn1.get());
    BOLT_ASSERT_EQ(std::string("127.0.0.1"), conn1->host);
    BOLT_ASSERT_EQ(8080, static_cast<int>(conn1->port));
    BOLT_ASSERT_TRUE(conn1->inUse);

    // Test connection reuse
    pool.releaseConnection(conn1);
    BOLT_ASSERT_FALSE(conn1->inUse);

    auto conn2 = pool.getConnection("127.0.0.1", 8080);
    BOLT_ASSERT_EQ(conn1.get(), conn2.get()); // Should reuse the same connection

    pool.closeConnection(conn2);
}

BOLT_TEST(NetworkOptimizations, ConnectionPoolStats) {
    setupNetworkTest();

    auto& pool = ConnectionPool::getInstance();
    pool.resetStats();

    // Use reference instead of copy (Stats is non-copyable)
    const auto& initialStats = pool.getStats();
    BOLT_ASSERT_EQ(static_cast<size_t>(0), initialStats.connectionsCreated.load());
    BOLT_ASSERT_EQ(static_cast<size_t>(0), initialStats.connectionsReused.load());

    // Create a connection
    // Note: This may return nullptr in CI environments without a test server
    auto conn1 = pool.getConnection("127.0.0.1", 8081);
    if (!conn1) {
        // Skip test if connection fails (no server listening)
        return;
    }
    const auto& statsAfterCreate = pool.getStats();
    BOLT_ASSERT_EQ(static_cast<size_t>(1), statsAfterCreate.connectionsCreated.load());

    // Release and get again (should reuse)
    pool.releaseConnection(conn1);
    auto conn2 = pool.getConnection("127.0.0.1", 8081);
    const auto& statsAfterReuse = pool.getStats();
    BOLT_ASSERT_EQ(static_cast<size_t>(1), statsAfterReuse.connectionsReused.load());

    pool.closeConnection(conn2);
}

// Message Compression Tests
BOLT_TEST(NetworkOptimizations, MessageCompressionBasic) {
    setupNetworkTest();

    MessageCompressor compressor(CompressionType::GZIP, 6);

    std::string originalMessage = "This is a test message that should compress well when repeated. "
                                 "This is a test message that should compress well when repeated. "
                                 "This is a test message that should compress well when repeated.";

    auto compressed = compressor.compress(originalMessage);
    BOLT_ASSERT_LT(compressed.size(), originalMessage.size()); // Should be smaller

    auto decompressed = compressor.decompress(compressed);
    BOLT_ASSERT_EQ(decompressed, originalMessage);
}

BOLT_TEST(NetworkOptimizations, MessageCompressionThreshold) {
    setupNetworkTest();

    MessageCompressor compressor(CompressionType::GZIP, 6);
    compressor.setMinCompressionSize(100);

    std::string shortMessage = "Short";
    auto compressedShort = compressor.compress(shortMessage);
    BOLT_ASSERT_EQ(compressedShort.size(), shortMessage.size()); // Should not compress

    std::string longMessage(200, 'A'); // 200 'A' characters
    auto compressedLong = compressor.compress(longMessage);
    BOLT_ASSERT_LT(compressedLong.size(), longMessage.size()); // Should compress
}

BOLT_TEST(NetworkOptimizations, MessageCompressionStats) {
    setupNetworkTest();

    MessageCompressor compressor(CompressionType::GZIP, 6);
    compressor.resetStats();

    auto initialStats = compressor.getStats();
    BOLT_ASSERT_EQ(static_cast<size_t>(0), initialStats.compressionCalls);
    BOLT_ASSERT_EQ(static_cast<size_t>(0), initialStats.decompressionCalls);

    std::string message(500, 'X');
    auto compressed = compressor.compress(message);
    auto decompressed = compressor.decompress(compressed);
    (void)decompressed; // Suppress unused warning

    auto finalStats = compressor.getStats();
    BOLT_ASSERT_EQ(static_cast<size_t>(1), finalStats.compressionCalls);
    BOLT_ASSERT_EQ(static_cast<size_t>(1), finalStats.decompressionCalls);
    BOLT_ASSERT_GT(finalStats.totalBytesIn, static_cast<size_t>(0));
    BOLT_ASSERT_GT(finalStats.totalBytesOut, static_cast<size_t>(0));
}

// Network Buffer Tests
BOLT_TEST(NetworkOptimizations, NetworkBufferBasicOperations) {
    setupNetworkTest();

    NetworkBuffer buffer(1024);

    // Test append operations
    std::string testData = "Hello, World!";
    buffer.append(testData);
    BOLT_ASSERT_EQ(buffer.size(), testData.size());
    BOLT_ASSERT_FALSE(buffer.empty());

    // Test consume operations
    auto consumed = buffer.consumeString(5);
    BOLT_ASSERT_EQ(std::string("Hello"), consumed);
    BOLT_ASSERT_EQ(buffer.size(), testData.size()); // Size unchanged until compact

    // Test compact
    buffer.compact();
    BOLT_ASSERT_EQ(buffer.size(), testData.size() - 5);

    buffer.clear();
    BOLT_ASSERT_TRUE(buffer.empty());
    BOLT_ASSERT_EQ(static_cast<size_t>(0), buffer.size());
}

BOLT_TEST(NetworkOptimizations, NetworkBufferGrowth) {
    setupNetworkTest();

    NetworkBuffer buffer(10); // Small initial size

    std::string largeData(100, 'A');
    buffer.append(largeData);

    BOLT_ASSERT_GE(buffer.capacity(), largeData.size());
    BOLT_ASSERT_EQ(buffer.size(), largeData.size());

    auto consumed = buffer.consumeString(largeData.size());
    BOLT_ASSERT_EQ(consumed, largeData);
}

BOLT_TEST(NetworkOptimizations, NetworkBufferPool) {
    setupNetworkTest();

    auto& pool = NetworkBufferPool::getInstance();

    // Get buffer from pool
    auto buffer1 = pool.getBuffer(1024);
    BOLT_ASSERT_NOT_NULL(buffer1.get());
    BOLT_ASSERT_GE(buffer1->capacity(), static_cast<size_t>(1024));

    size_t activeCount = pool.getActiveBuffers();
    BOLT_ASSERT_GT(activeCount, static_cast<size_t>(0));

    // Return buffer to pool
    pool.returnBuffer(std::move(buffer1));

    // Get another buffer (should reuse from pool)
    auto buffer2 = pool.getBuffer(1024);
    BOLT_ASSERT_NOT_NULL(buffer2.get());

    pool.returnBuffer(std::move(buffer2));
}

// Ring Buffer Tests
BOLT_TEST(NetworkOptimizations, RingBufferBasicOperations) {
    setupNetworkTest();

    RingBuffer ringBuf(100);

    BOLT_ASSERT_TRUE(ringBuf.empty());
    BOLT_ASSERT_FALSE(ringBuf.full());
    BOLT_ASSERT_EQ(static_cast<size_t>(0), ringBuf.readAvailable());
    BOLT_ASSERT_GT(ringBuf.writeAvailable(), static_cast<size_t>(0));

    // Write some data
    std::string testData = "Test data";
    size_t written = ringBuf.write(testData.data(), testData.size());
    BOLT_ASSERT_EQ(written, testData.size());
    BOLT_ASSERT_EQ(ringBuf.readAvailable(), testData.size());

    // Read data back
    char readBuffer[100];
    size_t readCount = ringBuf.read(readBuffer, testData.size());
    BOLT_ASSERT_EQ(readCount, testData.size());
    BOLT_ASSERT_EQ(std::string(readBuffer, readCount), testData);
    BOLT_ASSERT_TRUE(ringBuf.empty());
}

BOLT_TEST(NetworkOptimizations, RingBufferWrapAround) {
    setupNetworkTest();

    RingBuffer ringBuf(10);

    // Fill buffer almost to capacity
    std::string data1 = "12345678";
    ringBuf.write(data1.data(), data1.size());

    // Read some data to make space
    char readBuf[5];
    ringBuf.read(readBuf, 4);

    // Write more data that should wrap around
    std::string data2 = "ABCDEF";
    size_t written = ringBuf.write(data2.data(), data2.size());
    BOLT_ASSERT_GT(written, static_cast<size_t>(0)); // Should write at least some data

    // Verify we can read the wrapped data
    size_t available = ringBuf.readAvailable();
    BOLT_ASSERT_GT(available, static_cast<size_t>(0));
}

// Network Metrics Tests
BOLT_TEST(NetworkOptimizations, NetworkMetricsBasic) {
    setupNetworkTest();

    auto& metrics = NetworkMetrics::getInstance();
    metrics.resetAllStats();

    std::string endpoint = "test-endpoint";
    metrics.registerEndpoint(endpoint);

    // Record some events
    metrics.recordConnection(endpoint, true);
    metrics.recordDataSent(endpoint, 1024);
    metrics.recordDataReceived(endpoint, 512);
    metrics.recordLatency(endpoint, 1500); // 1.5ms

    auto stats = metrics.getEndpointStats(endpoint);
    BOLT_ASSERT_NOT_NULL(stats.get());
    BOLT_ASSERT_EQ(static_cast<size_t>(1), stats->connectionsOpened.load());
    BOLT_ASSERT_EQ(static_cast<size_t>(1), stats->connectionsActive.load());
    BOLT_ASSERT_EQ(static_cast<size_t>(1024), stats->bytesSent.load());
    BOLT_ASSERT_EQ(static_cast<size_t>(512), stats->bytesReceived.load());
    BOLT_ASSERT_EQ(static_cast<size_t>(1), stats->latencySamples.load());

    metrics.recordDisconnection(endpoint);
    stats = metrics.getEndpointStats(endpoint);
    BOLT_ASSERT_EQ(static_cast<size_t>(0), stats->connectionsActive.load());
    BOLT_ASSERT_EQ(static_cast<size_t>(1), stats->connectionsClosed.load());
}

BOLT_TEST(NetworkOptimizations, NetworkMetricsGlobal) {
    setupNetworkTest();

    auto& metrics = NetworkMetrics::getInstance();
    metrics.resetAllStats();

    std::string endpoint1 = "endpoint1";
    std::string endpoint2 = "endpoint2";

    metrics.registerEndpoint(endpoint1);
    metrics.registerEndpoint(endpoint2);

    // Record events on both endpoints
    metrics.recordConnection(endpoint1, true);
    metrics.recordConnection(endpoint2, true);
    metrics.recordDataSent(endpoint1, 1000);
    metrics.recordDataSent(endpoint2, 2000);

    auto globalStats = metrics.getGlobalStats();
    BOLT_ASSERT_EQ(static_cast<size_t>(2), globalStats.connectionsOpened);
    BOLT_ASSERT_EQ(static_cast<size_t>(3000), globalStats.bytesSent);
}

BOLT_TEST(NetworkOptimizations, NetworkMetricsReport) {
    setupNetworkTest();

    auto& metrics = NetworkMetrics::getInstance();
    metrics.resetAllStats();

    std::string endpoint = "test-endpoint-report";
    metrics.registerEndpoint(endpoint);
    metrics.recordConnection(endpoint, true);
    metrics.recordDataSent(endpoint, 1024);

    std::string report = metrics.generateReport();
    BOLT_ASSERT_FALSE(report.empty());
    BOLT_ASSERT(report.find("Network Performance Report") != std::string::npos);
    BOLT_ASSERT(report.find("Global Statistics") != std::string::npos);

    std::string endpointReport = metrics.generateEndpointReport(endpoint);
    BOLT_ASSERT_FALSE(endpointReport.empty());
    BOLT_ASSERT(endpointReport.find("Endpoint Report") != std::string::npos);
}

// Bandwidth Tracking Tests
BOLT_TEST(NetworkOptimizations, BandwidthTracking) {
    setupNetworkTest();

    BandwidthTracker tracker(std::chrono::seconds(5));

    // Initially no bandwidth
    BOLT_ASSERT_EQ(0.0, tracker.getCurrentBandwidth());

    // Record some data
    tracker.recordBytes(1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tracker.recordBytes(2000);

    // Should have some bandwidth now
    double bandwidth = tracker.getCurrentBandwidth();
    BOLT_ASSERT_GT(bandwidth, 0.0);

    double peak = tracker.getPeakBandwidth();
    BOLT_ASSERT_GE(peak, bandwidth);
}

// Zero-Copy Buffer Tests
BOLT_TEST(NetworkOptimizations, ZeroCopyBuffer) {
    setupNetworkTest();

    ZeroCopyBuffer zcBuffer;

    std::string data1 = "Hello";
    std::string data2 = "World";

    zcBuffer.addReference(data1);
    zcBuffer.addReference(data2);

    BOLT_ASSERT_EQ(zcBuffer.getTotalSize(), data1.size() + data2.size());

    auto combined = zcBuffer.copyToVector();
    std::string result(combined.begin(), combined.end());
    BOLT_ASSERT_EQ(result, data1 + data2);
}

// Scatter-Gather Buffer Tests
BOLT_TEST(NetworkOptimizations, ScatterGatherBuffer) {
    setupNetworkTest();

    ScatterGatherBuffer sgBuffer;

    std::string segment1 = "First";
    std::string segment2 = "Second";
    std::vector<uint8_t> segment3 = {'T', 'h', 'i', 'r', 'd'};

    sgBuffer.addSegment(segment1);
    sgBuffer.addSegment(segment2);
    sgBuffer.addSegment(segment3);

    BOLT_ASSERT_EQ(static_cast<size_t>(3), sgBuffer.getSegmentCount());
    BOLT_ASSERT_EQ(sgBuffer.getTotalSize(), segment1.size() + segment2.size() + segment3.size());

    auto flattened = sgBuffer.flatten();
    std::string result(flattened.begin(), flattened.end());
    BOLT_ASSERT_EQ(std::string("FirstSecondThird"), result);
}

} // namespace test
} // namespace bolt

int main() {
    return bolt::test::TestSuite::getInstance().runAllTests();
}
