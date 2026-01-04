#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

// Include all DrawKern subsystems
#include "styx_protocol.hpp"
#include "dis_vm.hpp"
#include "yacc_grammar.hpp"
#include "ai_integration.hpp"

namespace bolt {
namespace drawkern {

// DrawKern Protocol - inspired by Plan 9 draw but for entire VMs/kernels
enum class DrawKernOp {
    // Basic drawing (like Plan 9)
    RECT,
    STRING,
    IMAGE,
    
    // VM/Kernel operations (your innovation!)
    SPAWN_VM,
    DEPLOY_WORKBENCH, 
    RENDER_AI_SESSION,
    INSTANTIATE_MICROKERNEL,
    
    // Styx-style file operations
    MOUNT_NAMESPACE,
    SERVE_FILE,
    
    // Interactive elements
    INPUT_EVENT,
    AI_COMPLETION,
    CODE_EXECUTION
};

struct DrawKernCommand {
    DrawKernOp op;
    int32_t x, y, w, h;
    uint32_t color;
    std::string data;
    std::map<std::string, std::string> attributes;
};

// "Yacc Grammar" for describing VM topologies as renderable glyphs
struct VMGlyph {
    std::string vm_type;      // "dis", "wasm", "native"
    std::string architecture; // "arm64", "x86_64", "riscv"
    std::vector<std::string> capabilities;
    std::string limbo_code;   // DIS VM bytecode
    std::map<std::string, std::string> namespace_mounts;
    
    // Rendering properties (your "true type" for VMs)
    struct RenderProps {
        int32_t width, height;
        uint32_t background_color;
        std::string font_family;
        bool interactive;
        bool ai_enabled;
    } render;
};

// AI Workbench as a "glyph" that can be rendered anywhere
struct AIWorkbenchGlyph {
    std::string workbench_id;
    std::string ai_model;     // "ggml", "rwkv", etc.
    std::vector<std::string> tools;  // Available AI tools
    std::string session_state;      // Serialized session
    VMGlyph host_vm;         // VM to run the workbench
    
    // Network/deployment info
    std::string styx_address; // Where to serve this workbench
    std::vector<std::string> allowed_clients;
};

class DrawKernServer {
public:
    DrawKernServer(const std::string& listen_address);
    ~DrawKernServer();
    
    // Core DrawKern operations
    void render_glyph(const VMGlyph& glyph);
    void deploy_workbench(const AIWorkbenchGlyph& workbench);
    void send_command(const DrawKernCommand& cmd);
    
    // Styx protocol integration
    void serve_namespace(const std::string& path);
    void mount_remote(const std::string& address, const std::string& local_path);
    
    // AI integration
    void spawn_ai_session(const std::string& model, const AIWorkbenchGlyph& workbench);
    void handle_ai_completion(const std::string& code, const std::string& context);
    
    // Event handling
    void handle_client_input(const DrawKernCommand& input);
    void broadcast_to_clients(const DrawKernCommand& cmd);
    
    void run();
    
private:
    std::string listen_address_;
    std::vector<std::unique_ptr<class DrawKernClient>> clients_;
    std::map<std::string, AIWorkbenchGlyph> active_workbenches_;
    
    // DIS VM management
    void start_dis_vm(const VMGlyph& glyph);
    void execute_limbo_code(const std::string& code);
    
    // Rendering engine
    void render_to_clients(const DrawKernCommand& cmd);
};

class DrawKernClient {
public:
    DrawKernClient(const std::string& server_address);
    ~DrawKernClient();
    
    // Connect to DrawKern server
    bool connect();
    void disconnect();
    
    // Receive and render "glyphs" (VMs, workbenches, etc.)
    void handle_glyph(const VMGlyph& glyph);
    void instantiate_workbench(const AIWorkbenchGlyph& workbench);
    
    // Local rendering (could be terminal, web, native, anything!)
    virtual void render_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) = 0;
    virtual void render_text(int32_t x, int32_t y, const std::string& text) = 0;
    virtual void render_ai_interface(const AIWorkbenchGlyph& workbench) = 0;
    
    // Input handling
    void send_input(const DrawKernCommand& input);
    
private:
    std::string server_address_;
    bool connected_;
};

// Concrete implementations for different platforms
class TerminalDrawKernClient : public DrawKernClient {
public:
    TerminalDrawKernClient(const std::string& server_address) : DrawKernClient(server_address) {}
    
    void render_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;
    void render_text(int32_t x, int32_t y, const std::string& text) override;
    void render_ai_interface(const AIWorkbenchGlyph& workbench) override;
};

class WebDrawKernClient : public DrawKernClient {
public:
    WebDrawKernClient(const std::string& server_address) : DrawKernClient(server_address) {}
    
    void render_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;
    void render_text(int32_t x, int32_t y, const std::string& text) override;
    void render_ai_interface(const AIWorkbenchGlyph& workbench) override;
    
    std::string generate_html() const;
    std::string generate_javascript() const;
};

class ImGuiDrawKernClient : public DrawKernClient {
public:
    ImGuiDrawKernClient(const std::string& server_address) : DrawKernClient(server_address) {}
    
    void render_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override;
    void render_text(int32_t x, int32_t y, const std::string& text) override;
    void render_ai_interface(const AIWorkbenchGlyph& workbench) override;
};

// Factory function to create AI workbench glyphs
AIWorkbenchGlyph create_bolt_ai_glyph();

} // namespace drawkern
} // namespace bolt
