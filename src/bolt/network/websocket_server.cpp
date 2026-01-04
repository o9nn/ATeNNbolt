
#include "bolt/network/websocket_server.hpp"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
#include <iostream>
#include <cstring>

// Only include OpenSSL if available
#ifdef BOLT_HAVE_OPENSSL
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#endif

#include <algorithm>
#include <sstream>

namespace bolt {

std::string WebSocketConnection::generateAcceptKey(const std::string& clientKey) {
#ifdef BOLT_HAVE_OPENSSL
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = clientKey + magic;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);
    
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, hash, SHA_DIGEST_LENGTH);
    BIO_flush(b64);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    std::string encoded(bptr->data, bptr->length);
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '\n'), encoded.end());
    
    BIO_free_all(b64);
    return encoded;
#else
    // Fallback implementation without OpenSSL
    // This is not secure and should only be used for testing
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = clientKey + magic;
    
    // Simple base64-like encoding as fallback (not secure)
    std::string encoded;
    for (char c : combined) {
        encoded += std::to_string(static_cast<int>(c));
    }
    return encoded;
#endif
}

bool WebSocketConnection::performHandshake() {
    char buffer[1024];
    int bytes = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return false;
    
    buffer[bytes] = '\0';
    std::string request(buffer);
    
    size_t keyStart = request.find("Sec-WebSocket-Key: ") + 19;
    size_t keyEnd = request.find("\r\n", keyStart);
    std::string clientKey = request.substr(keyStart, keyEnd - keyStart);
    
    std::string acceptKey = generateAcceptKey(clientKey);
    
    std::stringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";
             
    std::string responseStr = response.str();
    return ::send(socket_, responseStr.c_str(), responseStr.length(), 0) > 0;
}

std::vector<uint8_t> WebSocketConnection::createFrame(const std::string& payload, bool binary) {
    std::vector<uint8_t> frame;
    uint8_t firstByte = binary ? 0x82 : 0x81;
    frame.push_back(firstByte);
    
    if (payload.length() <= 125) {
        frame.push_back(payload.length());
    } else if (payload.length() <= 65535) {
        frame.push_back(126);
        frame.push_back((payload.length() >> 8) & 0xFF);
        frame.push_back(payload.length() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((payload.length() >> (i * 8)) & 0xFF);
        }
    }
    
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

void WebSocketConnection::send(const std::string& message, bool binary) {
    auto frame = createFrame(message, binary);
    // Cast to const char* for Windows compatibility (send expects const char*)
    ::send(socket_, reinterpret_cast<const char*>(frame.data()), static_cast<int>(frame.size()), 0);
}

void WebSocketConnection::close() {
    if (socket_ >= 0) {
        ::close(socket_);
        socket_ = -1;
    }
}

void WebSocketServer::start(int port) {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (serverSocket_ == INVALID_SOCKET) {
#else
    if (serverSocket_ < 0) {
#endif
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket_, 5) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    running_ = true;
    serverThread_ = std::thread([this]() {
        while (running_) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
#ifdef _WIN32
            SOCKET clientSocketHandle = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocketHandle == INVALID_SOCKET) {
                continue;
            }
            // Convert SOCKET to int for our internal API
            int clientSocket = static_cast<int>(clientSocketHandle);
#else
            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket < 0) {
                continue;
            }
#endif
            auto conn = new WebSocketConnection(clientSocket);
            if (conn->performHandshake()) {
                {
                    std::lock_guard<std::mutex> lock(connectionsMutex_);
                    connections_.insert(conn);
                }
                if (connectCallback_) {
                    connectCallback_(conn);
                }
                std::thread(&WebSocketServer::handleClient, this, clientSocket).detach();
            } else {
                delete conn;
            }
        }
    });
}

std::vector<uint8_t> WebSocketServer::parseFrame(const std::vector<uint8_t>& buffer, bool& isBinary) {
    if (buffer.size() < 2) return {};
    
    uint8_t firstByte = buffer[0];
    uint8_t secondByte = buffer[1];
    
    bool fin = (firstByte & 0x80) != 0;
    (void)fin; // Suppress unused variable warning
    uint8_t opcode = firstByte & 0x0F;
    bool masked = (secondByte & 0x80) != 0;
    uint64_t payloadLen = secondByte & 0x7F;
    
    isBinary = (opcode == 0x02);
    
    size_t headerLen = 2;
    if (payloadLen == 126) {
        headerLen += 2;
        payloadLen = (buffer[2] << 8) | buffer[3];
    } else if (payloadLen == 127) {
        headerLen += 8;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | buffer[2 + i];
        }
    }
    
    if (masked) {
        uint8_t mask[4];
        std::copy(buffer.begin() + headerLen, buffer.begin() + headerLen + 4, mask);
        headerLen += 4;
        
        std::vector<uint8_t> payload(payloadLen);
        for (size_t i = 0; i < payloadLen; ++i) {
            payload[i] = buffer[headerLen + i] ^ mask[i % 4];
        }
        return payload;
    } else {
        return std::vector<uint8_t>(buffer.begin() + headerLen, buffer.begin() + headerLen + payloadLen);
    }
}

void WebSocketServer::handleClient(int clientSocket) {
    std::vector<uint8_t> buffer(1024);
    WebSocketConnection* conn = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        for (auto c : connections_) {
            if (c->getSocket() == clientSocket) {
                conn = c;
                break;
            }
        }
    }

    while (running_ && conn) {
        // Cast to char* for cross-platform compatibility (recv expects char*)
        int bytesRead = recv(clientSocket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
        if (bytesRead <= 0) break;
        
        bool isBinary;
        auto payload = parseFrame(std::vector<uint8_t>(buffer.begin(), buffer.begin() + bytesRead), isBinary);
        
        if (!payload.empty() && messageCallback_) {
            std::string message(payload.begin(), payload.end());
            messageCallback_(message, conn, isBinary);
        }
    }

    if (disconnectCallback_) {
        disconnectCallback_(conn);
    }

    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        connections_.erase(conn);
    }
    delete conn;
}

void WebSocketServer::broadcast(const std::string& message, bool binary) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto conn : connections_) {
        conn->send(message, binary);
    }
}

void WebSocketServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
    
    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto conn : connections_) {
        conn->close();
        delete conn;
    }
    connections_.clear();
}

} // namespace bolt
