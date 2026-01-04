#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <functional>

namespace bolt {
namespace drawkern {

// Styx Protocol Implementation - Plan 9 network file system protocol
// This provides real network transparency for DrawKern operations

enum class StyxMessageType : uint8_t {
    // Core Styx messages
    Tversion = 100, Rversion = 101,
    Tauth    = 102, Rauth    = 103,
    Tattach  = 104, Rattach  = 105,
    Terror   = 106, Rerror   = 107,
    Tflush   = 108, Rflush   = 109,
    Twalk    = 110, Rwalk    = 111,
    Topen    = 112, Ropen    = 113,
    Tcreate  = 114, Rcreate  = 115,
    Tread    = 116, Rread    = 117,
    Twrite   = 118, Rwrite   = 119,
    Tclunk   = 120, Rclunk   = 121,
    Tremove  = 122, Rremove  = 123,
    Tstat    = 124, Rstat    = 125,
    Twstat   = 126, Rwstat   = 127,
    
    // DrawKern extensions
    Tdrawkern = 200, Rdrawkern = 201,
    Tglyph    = 202, Rglyph    = 203,
    Tvmspawn  = 204, Rvmspawn  = 205
};

struct StyxQid {
    uint8_t type;
    uint32_t vers;
    uint64_t path;
};

struct StyxStat {
    uint16_t size;
    uint16_t type;
    uint32_t dev;
    StyxQid qid;
    uint32_t mode;
    uint32_t atime;
    uint32_t mtime;
    uint64_t length;
    std::string name;
    std::string uid;
    std::string gid;
    std::string muid;
};

struct StyxMessage {
    uint32_t size = 0;
    StyxMessageType type = StyxMessageType::Terror;
    uint16_t tag = 0;
    std::vector<uint8_t> data;
    
    // Default constructor
    StyxMessage() = default;
    
    // Helper methods for serialization
    void serialize(std::vector<uint8_t>& buffer) const;
    static StyxMessage deserialize(const std::vector<uint8_t>& buffer);
};

class StyxConnection {
public:
    StyxConnection(const std::string& address);
    ~StyxConnection();
    
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }
    
    // Core protocol operations
    bool version(const std::string& version = "9P2000");
    bool attach(const std::string& aname = "", const std::string& uname = "drawkern");
    
    // File operations
    uint32_t walk(uint32_t fid, uint32_t newfid, const std::vector<std::string>& path);
    uint32_t open(uint32_t fid, uint8_t mode);
    std::vector<uint8_t> read(uint32_t fid, uint64_t offset, uint32_t count);
    uint32_t write(uint32_t fid, uint64_t offset, const std::vector<uint8_t>& data);
    bool clunk(uint32_t fid);
    
    // DrawKern specific operations
    bool send_glyph(const std::string& glyph_data);
    bool spawn_vm(const std::string& vm_spec);
    
    StyxMessage send_message(const StyxMessage& message);
    
private:
    std::string address_;
    int socket_fd_;
    bool connected_;
    uint16_t next_tag_;
    uint32_t next_fid_;
    
    bool send_data(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_data();
};

class StyxServer {
public:
    StyxServer(const std::string& address);
    ~StyxServer();
    
    bool start();
    void stop();
    void run();
    
    // File system interface for DrawKern
    void serve_file(const std::string& path, const std::string& content);
    void serve_vm_namespace(const std::string& vm_id);
    
    // DrawKern operations
    void register_glyph_handler(std::function<void(const std::string&)> handler);
    void register_vm_handler(std::function<void(const std::string&)> handler);
    
private:
    std::string address_;
    int listen_fd_;
    bool running_;
    std::map<std::string, std::string> virtual_files_;
    std::function<void(const std::string&)> glyph_handler_;
    std::function<void(const std::string&)> vm_handler_;
    
    void handle_client(int client_fd);
    StyxMessage handle_message(const StyxMessage& message);
    
    // Message handlers
    StyxMessage handle_version(const StyxMessage& message);
    StyxMessage handle_attach(const StyxMessage& message);
    StyxMessage handle_walk(const StyxMessage& message);
    StyxMessage handle_open(const StyxMessage& message);
    StyxMessage handle_read(const StyxMessage& message);
    StyxMessage handle_write(const StyxMessage& message);
    StyxMessage handle_clunk(const StyxMessage& message);
    
    // DrawKern extensions
    StyxMessage handle_glyph(const StyxMessage& message);
    StyxMessage handle_vm_spawn(const StyxMessage& message);
};

// Network file system mount point for DrawKern
class DrawKernNamespace {
public:
    DrawKernNamespace();
    ~DrawKernNamespace();
    
    // Mount DrawKern services
    bool mount(const std::string& address, const std::string& local_path);
    bool unmount(const std::string& local_path);
    
    // Access DrawKern resources as files
    std::string read_glyph(const std::string& glyph_id);
    bool write_glyph(const std::string& glyph_id, const std::string& content);
    
    // VM management through filesystem
    bool spawn_vm_from_file(const std::string& vm_spec_path);
    std::string get_vm_status(const std::string& vm_id);
    
private:
    std::map<std::string, std::unique_ptr<StyxConnection>> mounted_services_;
};

} // namespace drawkern
} // namespace bolt