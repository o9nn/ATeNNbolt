#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <memory>
#include <functional>
#include <cstdint>

namespace bolt {
namespace drawkern {

// DIS VM - Inferno Distributed Systems Virtual Machine
// Executes Limbo bytecode for DrawKern workbenches

enum class DISOpcode : uint8_t {
    // Stack operations
    LOAD    = 0x01,  // Load from memory to stack
    STORE   = 0x02,  // Store from stack to memory
    POP     = 0x03,  // Pop stack
    DUP     = 0x04,  // Duplicate top of stack
    
    // Arithmetic operations
    ADD     = 0x10,  // Add two values
    SUB     = 0x11,  // Subtract
    MUL     = 0x12,  // Multiply
    DIV     = 0x13,  // Divide
    MOD     = 0x14,  // Modulo
    
    // Comparison operations
    EQ      = 0x20,  // Equal
    NE      = 0x21,  // Not equal
    LT      = 0x22,  // Less than
    GT      = 0x23,  // Greater than
    LE      = 0x24,  // Less or equal
    GE      = 0x25,  // Greater or equal
    
    // Control flow
    JMP     = 0x30,  // Unconditional jump
    JMPT    = 0x31,  // Jump if true
    JMPF    = 0x32,  // Jump if false
    CALL    = 0x33,  // Function call
    RET     = 0x34,  // Return from function
    
    // String operations
    NEWSTR  = 0x40,  // Create new string
    CONCAT  = 0x41,  // Concatenate strings
    STRLEN  = 0x42,  // String length
    
    // List/Array operations
    NEWLIST = 0x50,  // Create new list
    APPEND  = 0x51,  // Append to list
    INDEX   = 0x52,  // Index into list
    
    // I/O operations
    PRINT   = 0x60,  // Print to output
    READ    = 0x61,  // Read from input
    
    // AI operations (DrawKern extensions)
    AI_INIT     = 0x70,  // Initialize AI model
    AI_COMPLETE = 0x71,  // AI completion
    AI_CHAT     = 0x72,  // AI chat
    
    // DrawKern operations
    RENDER_GLYPH = 0x80,  // Render a glyph
    SPAWN_VM     = 0x81,  // Spawn new VM
    MOUNT_NS     = 0x82,  // Mount namespace
    
    // System operations
    HALT    = 0xFF   // Halt VM
};

struct DISValue {
    enum Type { INT, FLOAT, STRING, LIST, REF } type;
    
    union {
        int64_t int_val;
        double float_val;
        void* ptr_val;
    };
    
    std::string string_val;
    std::vector<DISValue> list_val;
    
    DISValue() : type(INT), int_val(0) {}
    DISValue(int64_t v) : type(INT), int_val(v) {}
    DISValue(double v) : type(FLOAT), float_val(v) {}
    DISValue(const std::string& v) : type(STRING), string_val(v) {}
    DISValue(const std::vector<DISValue>& v) : type(LIST), list_val(v) {}
};

struct DISInstruction {
    DISOpcode opcode;
    std::vector<DISValue> operands;
    
    DISInstruction(DISOpcode op) : opcode(op) {}
    DISInstruction(DISOpcode op, const DISValue& operand) : opcode(op) {
        operands.push_back(operand);
    }
};

// Limbo bytecode program
struct DISProgram {
    std::vector<DISInstruction> instructions;
    std::map<std::string, size_t> labels;    // Label -> instruction index
    std::map<std::string, DISValue> globals; // Global variables
    std::string source_limbo;                // Original Limbo source
    
    void add_instruction(const DISInstruction& inst) {
        instructions.push_back(inst);
    }
    
    void add_label(const std::string& label) {
        labels[label] = instructions.size();
    }
    
    size_t get_label_address(const std::string& label) const {
        auto it = labels.find(label);
        return it != labels.end() ? it->second : 0;
    }
};

class DISVM {
public:
    DISVM();
    ~DISVM();
    
    // Program execution
    bool load_program(const DISProgram& program);
    bool load_limbo_source(const std::string& source);
    bool run();
    void step();
    void halt();
    
    // State management
    void reset();
    bool is_running() const { return running_; }
    size_t get_pc() const { return pc_; }
    size_t get_program_size() const { return program_.instructions.size(); }
    
    // Stack operations
    void push(const DISValue& value);
    DISValue pop();
    DISValue peek() const;
    size_t stack_size() const { return stack_.size(); }
    
    // Memory management
    void set_global(const std::string& name, const DISValue& value);
    DISValue get_global(const std::string& name) const;
    std::vector<std::string> get_global_names() const;
    
    // AI integration
    void set_ai_handler(std::function<std::string(const std::string&, const std::string&)> handler);
    
    // DrawKern integration
    void set_glyph_renderer(std::function<void(const std::string&)> renderer);
    void set_vm_spawner(std::function<bool(const std::string&)> spawner);
    void set_namespace_mounter(std::function<bool(const std::string&, const std::string&)> mounter);

    // Debugging callbacks
    void set_step_callback(std::function<void(size_t pc, const std::string& instruction)> callback);
    void set_breakpoint_callback(std::function<void(size_t pc)> callback);
    void set_error_callback(std::function<void(const std::string& error)> callback);
    void set_completion_callback(std::function<void()> callback);
    void set_output_callback(std::function<void(const std::string& output)> callback);
    
    // Debugging
    void dump_stack() const;
    void dump_globals() const;
    std::string get_instruction_info(size_t pc) const;
    
    // Breakpoint management
    bool set_breakpoint(size_t pc);
    bool remove_breakpoint(size_t pc);
    void clear_all_breakpoints();
    std::vector<size_t> get_breakpoints() const;
    bool has_breakpoint(size_t pc) const;
    
    // Advanced debugging controls
    void step_over();
    void step_into();
    void step_out();
    void continue_execution();
    bool is_at_breakpoint() const;
    
    // Call stack inspection
    std::vector<size_t> get_call_stack() const;
    size_t get_call_stack_depth() const;
    
private:
    DISProgram program_;
    std::stack<DISValue> stack_;
    std::map<std::string, DISValue> globals_;
    size_t pc_;          // Program counter
    bool running_;
    bool halted_;
    
    // Call stack for function calls
    std::stack<size_t> call_stack_;
    
    // Debugging state
    std::set<size_t> breakpoints_;
    bool at_breakpoint_;
    bool step_mode_;
    size_t step_depth_;  // For step over/out functionality
    
    // Handler functions
    std::function<std::string(const std::string&, const std::string&)> ai_handler_;
    std::function<void(const std::string&)> glyph_renderer_;
    std::function<bool(const std::string&)> vm_spawner_;
    std::function<bool(const std::string&, const std::string&)> namespace_mounter_;

    // Debugging callbacks
    std::function<void(size_t, const std::string&)> step_callback_;
    std::function<void(size_t)> breakpoint_callback_;
    std::function<void(const std::string&)> error_callback_;
    std::function<void()> completion_callback_;
    std::function<void(const std::string&)> output_callback_;
    
    // Instruction execution
    void execute_instruction(const DISInstruction& inst);
    
    // Opcode handlers
    void handle_load(const DISInstruction& inst);
    void handle_store(const DISInstruction& inst);
    void handle_arithmetic(const DISInstruction& inst);
    void handle_comparison(const DISInstruction& inst);
    void handle_control_flow(const DISInstruction& inst);
    void handle_string_ops(const DISInstruction& inst);
    void handle_list_ops(const DISInstruction& inst);
    void handle_io_ops(const DISInstruction& inst);
    void handle_ai_ops(const DISInstruction& inst);
    void handle_drawkern_ops(const DISInstruction& inst);
    
    // Helper methods
    bool check_stack_size(size_t required) const;
    void runtime_error(const std::string& message);
};

// Limbo to DIS bytecode compiler (simplified)
class LimboCompiler {
public:
    LimboCompiler();
    ~LimboCompiler();
    
    // Compile Limbo source to DIS bytecode
    DISProgram compile(const std::string& source);
    
    // Get compilation errors
    std::vector<std::string> get_errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }
    
private:
    std::vector<std::string> errors_;
    
    // Parsing helpers
    std::vector<std::string> tokenize(const std::string& source);
    DISProgram parse_tokens(const std::vector<std::string>& tokens);
    
    void add_error(const std::string& error) {
        errors_.push_back(error);
    }
};

// Factory for creating common DIS programs
class DISProgramFactory {
public:
    // Create AI workbench program
    static DISProgram create_ai_workbench(const std::string& model_name);
    
    // Create simple echo server
    static DISProgram create_echo_server();
    
    // Create file server
    static DISProgram create_file_server(const std::string& root_path);
    
    // Create DrawKern renderer
    static DISProgram create_drawkern_renderer();
    
    // Load program from Limbo source
    static DISProgram from_limbo_source(const std::string& source);
};

// VM manager for multiple DIS VMs
class DISVMManager {
public:
    DISVMManager();
    ~DISVMManager();
    
    // VM lifecycle
    std::string create_vm(const DISProgram& program);
    bool start_vm(const std::string& vm_id);
    bool stop_vm(const std::string& vm_id);
    bool destroy_vm(const std::string& vm_id);
    
    // VM communication
    bool send_message(const std::string& vm_id, const DISValue& message);
    std::vector<DISValue> get_messages(const std::string& vm_id);
    
    // VM status
    std::vector<std::string> list_vms() const;
    bool is_vm_running(const std::string& vm_id) const;
    std::string get_vm_status(const std::string& vm_id) const;
    
private:
    std::map<std::string, std::unique_ptr<DISVM>> vms_;
    std::map<std::string, std::vector<DISValue>> vm_message_queues_;
    size_t next_vm_id_;
    
    std::string generate_vm_id();
};

} // namespace drawkern
} // namespace bolt