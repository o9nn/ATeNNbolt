
#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "network_commands.hpp"

namespace bolt {

class WebSocketConnection {
public:
    WebSocketConnection(int socket) : socket_(socket) {}
    void send(const std::string& message, bool binary = false);
    void close();
    bool performHandshake();
    int getSocket() const { return socket_; }
    
private:
    int socket_;
    std::string generateAcceptKey(const std::string& clientKey);
    std::vector<uint8_t> createFrame(const std::string& payload, bool binary);
};

class WebSocketServer {
public:
    static WebSocketServer& getInstance() {
        static WebSocketServer instance;
        return instance;
    }

    void start(int port = 8080);
    void stop();
    void broadcast(const std::string& message, bool binary = false);
    
    void onMessage(std::function<void(const std::string&, WebSocketConnection*, bool)> callback) {
        messageCallback_ = callback;
    }

    void onConnect(std::function<void(WebSocketConnection*)> callback) {
        connectCallback_ = callback;
    }

    void onDisconnect(std::function<void(WebSocketConnection*)> callback) {
        disconnectCallback_ = callback;
    }

private:
    WebSocketServer() = default;
    void handleClient(int clientSocket);
    std::vector<uint8_t> parseFrame(const std::vector<uint8_t>& buffer, bool& isBinary);
    
    bool running_ = false;
    int serverSocket_ = -1;
    std::thread serverThread_;
    std::mutex connectionsMutex_;
    std::unordered_set<WebSocketConnection*> connections_;
    std::function<void(const std::string&, WebSocketConnection*, bool)> messageCallback_;
    std::function<void(WebSocketConnection*)> connectCallback_;
    std::function<void(WebSocketConnection*)> disconnectCallback_;
};

} // namespace bolt

#endif
