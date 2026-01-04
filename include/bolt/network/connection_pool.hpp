#ifndef CONNECTION_POOL_HPP
#define CONNECTION_POOL_HPP

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace bolt {

/**
 * Connection pool entry for efficient connection reuse
 */
struct PooledConnection {
    int socket;
    std::string host;
    int port;
    std::chrono::steady_clock::time_point lastUsed;
    std::chrono::steady_clock::time_point created;
    bool inUse;
    size_t useCount;
    
    PooledConnection(int s, const std::string& h, int p) 
        : socket(s), host(h), port(p), 
          lastUsed(std::chrono::steady_clock::now()),
          created(std::chrono::steady_clock::now()),
          inUse(false), useCount(0) {}
};

/**
 * High-performance connection pool with keep-alive support
 */
class ConnectionPool {
public:
    static ConnectionPool& getInstance() {
        static ConnectionPool instance;
        return instance;
    }

    // Configuration
    void setMaxConnectionsPerHost(size_t max) { maxConnectionsPerHost_ = max; }
    void setConnectionTimeout(std::chrono::seconds timeout) { connectionTimeout_ = timeout; }
    void setKeepAliveTimeout(std::chrono::seconds timeout) { keepAliveTimeout_ = timeout; }
    void setMaxIdleTime(std::chrono::seconds timeout) { maxIdleTime_ = timeout; }

    // Connection management
    std::shared_ptr<PooledConnection> getConnection(const std::string& host, int port);
    void releaseConnection(std::shared_ptr<PooledConnection> conn);
    void closeConnection(std::shared_ptr<PooledConnection> conn);
    
    // Pool maintenance
    void cleanupExpiredConnections();
    size_t getActiveConnections() const;
    size_t getTotalConnections() const;
    
    // Statistics
    struct Stats {
        std::atomic<size_t> connectionsCreated{0};
        std::atomic<size_t> connectionsReused{0};
        std::atomic<size_t> connectionsClosed{0};
        std::atomic<size_t> timeouts{0};
        std::atomic<size_t> errors{0};
        
        // Disable copy constructor and assignment operator
        Stats() = default;
        Stats(const Stats&) = delete;
        Stats& operator=(const Stats&) = delete;
        
        // Allow move construction and assignment
        Stats(Stats&&) = default;
        Stats& operator=(Stats&&) = default;
    };
    
    const Stats& getStats() const { return stats_; }
    void resetStats();

private:
    ConnectionPool() : maxConnectionsPerHost_(10), 
                      connectionTimeout_(std::chrono::seconds(30)),
                      keepAliveTimeout_(std::chrono::seconds(60)),
                      maxIdleTime_(std::chrono::seconds(300)) {}
    
    ~ConnectionPool();
    
    std::shared_ptr<PooledConnection> createConnection(const std::string& host, int port);
    bool isConnectionValid(const std::shared_ptr<PooledConnection>& conn);
    std::string getConnectionKey(const std::string& host, int port);
    
    mutable std::mutex poolMutex_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<PooledConnection>>> pool_;
    
    size_t maxConnectionsPerHost_;
    std::chrono::seconds connectionTimeout_;
    std::chrono::seconds keepAliveTimeout_;
    std::chrono::seconds maxIdleTime_;
    
    Stats stats_;
};

} // namespace bolt

#endif