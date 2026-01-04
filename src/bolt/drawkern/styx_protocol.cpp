#include "bolt/drawkern/styx_protocol.hpp"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <BaseTsd.h>
    #define close closesocket
    typedef SSIZE_T ssize_t;
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include <thread>

namespace bolt {
namespace drawkern {

// Helper functions for Styx protocol serialization
namespace {
    void write_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
        buffer.push_back(value & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
    }
    
    void write_uint32(std::vector<uint8_t>& buffer, uint32_t value) {
        buffer.push_back(value & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
        buffer.push_back((value >> 16) & 0xFF);
        buffer.push_back((value >> 24) & 0xFF);
    }
    
    void write_string(std::vector<uint8_t>& buffer, const std::string& str) {
        write_uint16(buffer, str.length());
        buffer.insert(buffer.end(), str.begin(), str.end());
    }
    
    uint16_t read_uint16(const std::vector<uint8_t>& buffer, size_t& offset) {
        uint16_t value = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        return value;
    }
    
    uint32_t read_uint32(const std::vector<uint8_t>& buffer, size_t& offset) {
        uint32_t value = buffer[offset] | (buffer[offset + 1] << 8) | 
                        (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
        offset += 4;
        return value;
    }
    
    [[maybe_unused]] std::string read_string(const std::vector<uint8_t>& buffer, size_t& offset) {
        uint16_t length = read_uint16(buffer, offset);
        std::string str(buffer.begin() + offset, buffer.begin() + offset + length);
        offset += length;
        return str;
    }
}

void StyxMessage::serialize(std::vector<uint8_t>& buffer) const {
    buffer.clear();
    write_uint32(buffer, size);
    buffer.push_back(static_cast<uint8_t>(type));
    write_uint16(buffer, tag);
    buffer.insert(buffer.end(), data.begin(), data.end());
}

StyxMessage StyxMessage::deserialize(const std::vector<uint8_t>& buffer) {
    StyxMessage msg;
    size_t offset = 0;
    
    msg.size = read_uint32(buffer, offset);
    msg.type = static_cast<StyxMessageType>(buffer[offset++]);
    msg.tag = read_uint16(buffer, offset);
    
    msg.data.assign(buffer.begin() + offset, buffer.end());
    return msg;
}

// StyxConnection implementation
StyxConnection::StyxConnection(const std::string& address) 
    : address_(address), socket_fd_(-1), connected_(false), next_tag_(1), next_fid_(1) {
}

StyxConnection::~StyxConnection() {
    disconnect();
}

bool StyxConnection::connect() {
    // Parse address (simplified - assumes localhost:port format)
    size_t colon_pos = address_.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string host = address_.substr(0, colon_pos);
    int port = std::stoi(address_.substr(colon_pos + 1));
    
#ifdef _WIN32
    socket_fd_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
#else
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (socket_fd_ < 0) {
        return false;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (host == "localhost" || host == "127.0.0.1") {
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        // For simplicity, only support localhost in this implementation
        close(socket_fd_);
        return false;
    }
    
    if (::connect(socket_fd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    return true;
}

void StyxConnection::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

bool StyxConnection::version(const std::string& version) {
    if (!connected_) return false;
    
    StyxMessage msg;
    msg.type = StyxMessageType::Tversion;
    msg.tag = next_tag_++;
    
    // Tversion message data: msize[4] version[s]
    write_uint32(msg.data, 8192);  // Max message size
    write_string(msg.data, version);
    msg.size = msg.data.size() + 7;  // Header size
    
    StyxMessage response = send_message(msg);
    return response.type == StyxMessageType::Rversion;
}

bool StyxConnection::attach(const std::string& aname, const std::string& uname) {
    if (!connected_) return false;
    
    StyxMessage msg;
    msg.type = StyxMessageType::Tattach;
    msg.tag = next_tag_++;
    
    // Tattach message data: fid[4] afid[4] uname[s] aname[s]
    write_uint32(msg.data, next_fid_++);  // fid
    write_uint32(msg.data, ~0U);          // afid (no auth)
    write_string(msg.data, uname);
    write_string(msg.data, aname);
    msg.size = msg.data.size() + 7;
    
    StyxMessage response = send_message(msg);
    return response.type == StyxMessageType::Rattach;
}

bool StyxConnection::send_glyph(const std::string& glyph_data) {
    if (!connected_) return false;
    
    StyxMessage msg;
    msg.type = StyxMessageType::Tdrawkern;
    msg.tag = next_tag_++;
    
    // Custom DrawKern message format
    write_string(msg.data, "glyph");
    write_string(msg.data, glyph_data);
    msg.size = msg.data.size() + 7;
    
    StyxMessage response = send_message(msg);
    return response.type == StyxMessageType::Rdrawkern;
}

bool StyxConnection::spawn_vm(const std::string& vm_spec) {
    if (!connected_) return false;
    
    StyxMessage msg;
    msg.type = StyxMessageType::Tvmspawn;
    msg.tag = next_tag_++;
    
    write_string(msg.data, vm_spec);
    msg.size = msg.data.size() + 7;
    
    StyxMessage response = send_message(msg);
    return response.type == StyxMessageType::Rvmspawn;
}

StyxMessage StyxConnection::send_message(const StyxMessage& message) {
    std::vector<uint8_t> buffer;
    message.serialize(buffer);
    
    if (!send_data(buffer)) {
        StyxMessage error;
        error.type = StyxMessageType::Terror;
        return error;
    }
    
    std::vector<uint8_t> response_data = receive_data();
    if (response_data.empty()) {
        StyxMessage error;
        error.type = StyxMessageType::Terror;
        return error;
    }
    
    return StyxMessage::deserialize(response_data);
}

bool StyxConnection::send_data(const std::vector<uint8_t>& data) {
    if (socket_fd_ < 0) return false;
    
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t result = send(socket_fd_, reinterpret_cast<const char*>(data.data() + sent), data.size() - sent, 0);
        if (result <= 0) return false;
        sent += result;
    }
    return true;
}

std::vector<uint8_t> StyxConnection::receive_data() {
    if (socket_fd_ < 0) return {};
    
    // First read the size field
    uint8_t size_buffer[4];
    if (recv(socket_fd_, reinterpret_cast<char*>(size_buffer), 4, MSG_WAITALL) != 4) {
        return {};
    }
    
    uint32_t message_size = size_buffer[0] | (size_buffer[1] << 8) | 
                           (size_buffer[2] << 16) | (size_buffer[3] << 24);
    
    if (message_size > 8192 || message_size < 7) {
        return {};
    }
    
    std::vector<uint8_t> buffer(message_size);
    memcpy(buffer.data(), size_buffer, 4);
    
    if (recv(socket_fd_, reinterpret_cast<char*>(buffer.data() + 4), message_size - 4, MSG_WAITALL) != (message_size - 4)) {
        return {};
    }
    
    return buffer;
}

// StyxServer implementation (minimal for demo)
StyxServer::StyxServer(const std::string& address) 
    : address_(address), listen_fd_(-1), running_(false) {
}

StyxServer::~StyxServer() {
    stop();
}

bool StyxServer::start() {
    // Parse address
    size_t colon_pos = address_.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    int port = std::stoi(address_.substr(colon_pos + 1));
    
#ifdef _WIN32
    listen_fd_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
#else
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (listen_fd_ < 0) {
        return false;
    }
    
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(listen_fd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(listen_fd_);
        return false;
    }
    
    if (listen(listen_fd_, 5) < 0) {
        close(listen_fd_);
        return false;
    }
    
    running_ = true;
    std::cout << "Styx server listening on " << address_ << std::endl;
    return true;
}

void StyxServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
}

void StyxServer::run() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
#ifdef _WIN32
        int client_fd = static_cast<int>(accept(listen_fd_, (sockaddr*)&client_addr, &client_len));
#else
        int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &client_len);
#endif
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }
        
        std::cout << "Styx client connected" << std::endl;
        
        // Handle client in separate thread
        std::thread client_thread([this, client_fd]() {
            handle_client(client_fd);
        });
        client_thread.detach();
    }
}

void StyxServer::serve_file(const std::string& path, const std::string& content) {
    virtual_files_[path] = content;
}

void StyxServer::register_glyph_handler(std::function<void(const std::string&)> handler) {
    glyph_handler_ = handler;
}

void StyxServer::register_vm_handler(std::function<void(const std::string&)> handler) {
    vm_handler_ = handler;
}

void StyxServer::handle_client(int client_fd) {
    // Simplified client handling for demo
    std::cout << "Handling Styx client connection" << std::endl;
    
    // Echo back basic responses for now
    char buffer[1024];
    while (running_) {
        ssize_t bytes = recv(client_fd, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
        if (bytes <= 0) break;
        
        std::cout << "Received Styx message: " << bytes << " bytes" << std::endl;
        
        // Send a simple response
        const char* response = "Styx DrawKern server ready\n";
        send(client_fd, reinterpret_cast<const char*>(response), strlen(response), 0);
    }
    
    close(client_fd);
    std::cout << "Styx client disconnected" << std::endl;
}

// DrawKernNamespace implementation
DrawKernNamespace::DrawKernNamespace() {
}

DrawKernNamespace::~DrawKernNamespace() {
}

bool DrawKernNamespace::mount(const std::string& address, const std::string& local_path) {
    auto connection = std::make_unique<StyxConnection>(address);
    if (!connection->connect()) {
        return false;
    }
    
    if (!connection->version() || !connection->attach()) {
        return false;
    }
    
    mounted_services_[local_path] = std::move(connection);
    std::cout << "Mounted DrawKern namespace: " << address << " -> " << local_path << std::endl;
    return true;
}

bool DrawKernNamespace::unmount(const std::string& local_path) {
    auto it = mounted_services_.find(local_path);
    if (it != mounted_services_.end()) {
        mounted_services_.erase(it);
        std::cout << "Unmounted DrawKern namespace: " << local_path << std::endl;
        return true;
    }
    return false;
}

std::string DrawKernNamespace::read_glyph(const std::string& glyph_id) {
    // In a real implementation, this would read from the mounted namespace
    return "DrawKern glyph: " + glyph_id;
}

bool DrawKernNamespace::write_glyph(const std::string& glyph_id, const std::string& content) {
    std::cout << "Writing glyph " << glyph_id << ": " << content.substr(0, 50) << "..." << std::endl;
    return true;
}

bool DrawKernNamespace::spawn_vm_from_file(const std::string& vm_spec_path) {
    std::cout << "Spawning VM from spec: " << vm_spec_path << std::endl;
    return true;
}

std::string DrawKernNamespace::get_vm_status(const std::string& vm_id) {
    return "VM " + vm_id + " running";
}

} // namespace drawkern
} // namespace bolt