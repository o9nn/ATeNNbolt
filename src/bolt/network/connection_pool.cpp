#include "bolt/network/connection_pool.hpp"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#ifndef _WIN32
#include <fcntl.h>
#endif
#include <errno.h>
#include <iostream>
#include <algorithm>

namespace bolt {

ConnectionPool::~ConnectionPool() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    for (auto& [key, connections] : pool_) {
        for (auto& conn : connections) {
            if (conn->socket >= 0) {
                close(conn->socket);
            }
        }
    }
}

std::shared_ptr<PooledConnection> ConnectionPool::getConnection(const std::string& host, int port) {
    std::string key = getConnectionKey(host, port);
    
    {
        std::lock_guard<std::mutex> lock(poolMutex_);
        auto it = pool_.find(key);
        if (it != pool_.end()) {
            auto& connections = it->second;
            
            // Find an available connection
            for (auto& conn : connections) {
                if (!conn->inUse && isConnectionValid(conn)) {
                    conn->inUse = true;
                    conn->lastUsed = std::chrono::steady_clock::now();
                    conn->useCount++;
                    stats_.connectionsReused++;
                    return conn;
                }
            }
        }
    }
    
    // Create new connection if none available
    auto newConn = createConnection(host, port);
    if (newConn) {
        std::lock_guard<std::mutex> lock(poolMutex_);
        auto& connections = pool_[key];
        
        // Remove expired connections to make room
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [this](const std::shared_ptr<PooledConnection>& conn) {
                    return !isConnectionValid(conn);
                }),
            connections.end()
        );
        
        // Only add if we haven't exceeded the limit
        if (connections.size() < maxConnectionsPerHost_) {
            connections.push_back(newConn);
        }
        
        stats_.connectionsCreated++;
    }
    
    return newConn;
}

void ConnectionPool::releaseConnection(std::shared_ptr<PooledConnection> conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(poolMutex_);
    conn->inUse = false;
    conn->lastUsed = std::chrono::steady_clock::now();
}

void ConnectionPool::closeConnection(std::shared_ptr<PooledConnection> conn) {
    if (!conn) return;
    
    if (conn->socket >= 0) {
        close(conn->socket);
        conn->socket = -1;
        stats_.connectionsClosed++;
    }
    
    std::string key = getConnectionKey(conn->host, conn->port);
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    auto it = pool_.find(key);
    if (it != pool_.end()) {
        auto& connections = it->second;
        connections.erase(
            std::remove(connections.begin(), connections.end(), conn),
            connections.end()
        );
        
        if (connections.empty()) {
            pool_.erase(it);
        }
    }
}

std::shared_ptr<PooledConnection> ConnectionPool::createConnection(const std::string& host, int port) {
#ifdef _WIN32
    int sockfd = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
#else
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (sockfd < 0) {
        stats_.errors++;
        return nullptr;
    }
    
    // Set socket to non-blocking for timeout handling
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
    
    int result = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    if (result < 0 && errno != EINPROGRESS) {
        close(sockfd);
        stats_.errors++;
        return nullptr;
    }
    
    // Handle connection timeout
    if (errno == EINPROGRESS) {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sockfd, &writeSet);
        
        struct timeval timeout;
        timeout.tv_sec = connectionTimeout_.count();
        timeout.tv_usec = 0;
        
        result = select(sockfd + 1, nullptr, &writeSet, nullptr, &timeout);
        if (result <= 0) {
            close(sockfd);
            stats_.timeouts++;
            return nullptr;
        }
        
        // Check if connection was successful
        int error;
        socklen_t len = sizeof(error);
#ifdef _WIN32
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
#else
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
#endif
        if (error != 0) {
            close(sockfd);
            stats_.errors++;
            return nullptr;
        }
    }
    
    // Set socket back to blocking mode
#ifdef _WIN32
    u_long blockingMode = 0;
    ioctlsocket(sockfd, FIONBIO, &blockingMode);
#else
    fcntl(sockfd, F_SETFL, flags);
#endif
    
    // Enable keep-alive
    int keepAlive = 1;
#ifdef _WIN32
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive));
#else
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
#endif
    
    auto conn = std::make_shared<PooledConnection>(sockfd, host, port);
    conn->inUse = true;
    return conn;
}

bool ConnectionPool::isConnectionValid(const std::shared_ptr<PooledConnection>& conn) {
    if (!conn || conn->socket < 0) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - conn->lastUsed);
    
    if (idleTime > maxIdleTime_) {
        return false;
    }
    
    // Test socket validity with non-blocking send
    char testByte = 0;
#ifdef _WIN32
    int result = send(conn->socket, &testByte, 0, 0);
    return result >= 0 || WSAGetLastError() == WSAEWOULDBLOCK;
#else
    int result = send(conn->socket, &testByte, 0, MSG_DONTWAIT | MSG_NOSIGNAL);
    return result >= 0 || errno == EAGAIN;
#endif
}

std::string ConnectionPool::getConnectionKey(const std::string& host, int port) {
    return host + ":" + std::to_string(port);
}

void ConnectionPool::cleanupExpiredConnections() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    for (auto it = pool_.begin(); it != pool_.end();) {
        auto& connections = it->second;
        
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [this](const std::shared_ptr<PooledConnection>& conn) {
                    if (!isConnectionValid(conn)) {
                        if (conn->socket >= 0) {
                            close(conn->socket);
                            stats_.connectionsClosed++;
                        }
                        return true;
                    }
                    return false;
                }),
            connections.end()
        );
        
        if (connections.empty()) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t ConnectionPool::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(poolMutex_);
    size_t active = 0;
    for (const auto& [key, connections] : pool_) {
        for (const auto& conn : connections) {
            if (conn->inUse) {
                active++;
            }
        }
    }
    return active;
}

size_t ConnectionPool::getTotalConnections() const {
    std::lock_guard<std::mutex> lock(poolMutex_);
    size_t total = 0;
    for (const auto& [key, connections] : pool_) {
        total += connections.size();
    }
    return total;
}

void ConnectionPool::resetStats() {
    stats_.connectionsCreated = 0;
    stats_.connectionsReused = 0;
    stats_.connectionsClosed = 0;
    stats_.timeouts = 0;
    stats_.errors = 0;
}

} // namespace bolt