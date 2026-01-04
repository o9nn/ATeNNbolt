
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <string>
#include <sstream>
#include <unordered_map>
#include "network_commands.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

namespace bolt {

class HTTPResponse {
public:
    HTTPResponse() : status_code_(200), status_message_("OK") {}
    
    void setStatus(int code, const std::string& message) {
        status_code_ = code;
        status_message_ = message;
    }
    
    void setHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }
    
    void setBody(const std::string& body) {
        body_ = body;
    }
    
    std::string toString() const {
        std::stringstream ss;
        ss << "HTTP/1.1 " << status_code_ << " " << status_message_ << "\r\n";
        for (const auto& [key, value] : headers_) {
            ss << key << ": " << value << "\r\n";
        }
        ss << "Content-Length: " << body_.length() << "\r\n";
        ss << "\r\n";
        ss << body_;
        return ss.str();
    }

private:
    int status_code_;
    std::string status_message_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

class HTTPServerCommand : public NetworkCommand {
public:
    HTTPServerCommand() : sockfd_(-1), port_(8080) {}
    
    bool connect(const std::string& host, int port) override {
        port_ = port;
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) return false;
        
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port_);
        
        if (bind(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            return false;
        }
        
        if (listen(sockfd_, 5) < 0) {
            return false;
        }
        
        return true;
    }
    
    void disconnect() override {
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }
    
    void execute() override {
        if (connect("0.0.0.0", port_)) {
            std::cout << "HTTP Server listening on port " << port_ << std::endl;
            
            while (true) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(sockfd_, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd < 0) {
                    continue;
                }
                
                handleClient(client_fd);
                close(client_fd);
            }
        }
    }
    
    std::string getDescription() const override {
        return "HTTP Server Command";
    }

private:
    void handleClient(int client_fd) {
        char buffer[1024] = {0};
        read(client_fd, buffer, 1024);
        
        HTTPResponse response;
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>Bolt C++ HTTP Server</h1></body></html>");
        
        std::string response_str = response.toString();
        write(client_fd, response_str.c_str(), response_str.length());
    }
    
    int sockfd_;
    int port_;
};

} // namespace bolt

#endif
