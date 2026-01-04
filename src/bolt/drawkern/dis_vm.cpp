#include "bolt/drawkern/dis_vm.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <thread>

namespace bolt {
namespace drawkern {

// DISVM implementation
DISVM::DISVM() : pc_(0), running_(false), halted_(false), at_breakpoint_(false), step_mode_(false), step_depth_(0) {
    reset();
}

DISVM::~DISVM() {
    halt();
}

bool DISVM::load_program(const DISProgram& program) {
    program_ = program;
    globals_ = program.globals;
    pc_ = 0;
    running_ = false;
    halted_ = false;
    
    // Clear stacks
    while (!stack_.empty()) stack_.pop();
    while (!call_stack_.empty()) call_stack_.pop();
    
    std::cout << "Loaded DIS program with " << program_.instructions.size() << " instructions" << std::endl;
    return true;
}

bool DISVM::load_limbo_source(const std::string& source) {
    LimboCompiler compiler;
    DISProgram program = compiler.compile(source);
    
    if (compiler.has_errors()) {
        std::cout << "Compilation errors:" << std::endl;
        for (const auto& error : compiler.get_errors()) {
            std::cout << "  " << error << std::endl;
        }
        return false;
    }
    
    return load_program(program);
}

bool DISVM::run() {
    if (program_.instructions.empty()) {
        std::cout << "No program loaded" << std::endl;
        return false;
    }
    
    running_ = true;
    halted_ = false;
    
    std::cout << "Starting DIS VM execution..." << std::endl;
    
    while (running_ && !halted_ && pc_ < program_.instructions.size()) {
        step();
    }
    
    if (halted_) {
        std::cout << "DIS VM halted normally" << std::endl;
    } else if (pc_ >= program_.instructions.size()) {
        std::cout << "DIS VM reached end of program" << std::endl;
    } else {
        std::cout << "DIS VM stopped" << std::endl;
    }
    
    running_ = false;
    return true;
}

void DISVM::step() {
    if (!running_ || halted_ || pc_ >= program_.instructions.size()) {
        // Check if program completed
        if (pc_ >= program_.instructions.size() && completion_callback_) {
            completion_callback_();
        }
        return;
    }

    // Check for breakpoint at current PC
    if (has_breakpoint(pc_) && !at_breakpoint_) {
        at_breakpoint_ = true;
        std::cout << "🔴 Breakpoint hit at PC " << pc_ << std::endl;

        // Invoke breakpoint callback
        if (breakpoint_callback_) {
            breakpoint_callback_(pc_);
        }
        return; // Pause execution at breakpoint
    }

    at_breakpoint_ = false;
    const DISInstruction& inst = program_.instructions[pc_];

    try {
        // Get instruction info for callback
        std::string inst_info = get_instruction_info(pc_);

        execute_instruction(inst);

        // Invoke step callback after successful execution
        if (step_callback_) {
            step_callback_(pc_, inst_info);
        }

        // Advance PC unless it was modified by a jump/call instruction
        if (inst.opcode != DISOpcode::JMP &&
            inst.opcode != DISOpcode::JMPT &&
            inst.opcode != DISOpcode::JMPF &&
            inst.opcode != DISOpcode::CALL &&
            inst.opcode != DISOpcode::RET) {
            pc_++;
        }

        // Handle step mode
        if (step_mode_) {
            if (step_depth_ > 0 && call_stack_.size() < step_depth_) {
                // Step out completed
                step_mode_ = false;
                step_depth_ = 0;
            } else if (step_depth_ == 0) {
                // Single step completed
                step_mode_ = false;
            }
        }
    } catch (const std::exception& e) {
        std::string error_msg = "Exception during execution: " + std::string(e.what());
        runtime_error(error_msg);

        // Invoke error callback
        if (error_callback_) {
            error_callback_(error_msg);
        }
    }
}

void DISVM::halt() {
    halted_ = true;
    running_ = false;
    std::cout << "DIS VM halted" << std::endl;
}

void DISVM::reset() {
    pc_ = 0;
    running_ = false;
    halted_ = false;
    at_breakpoint_ = false;
    step_mode_ = false;
    step_depth_ = 0;
    
    while (!stack_.empty()) stack_.pop();
    while (!call_stack_.empty()) call_stack_.pop();
    
    globals_.clear();
    program_ = DISProgram{};
    // Note: breakpoints are preserved across resets
}

void DISVM::push(const DISValue& value) {
    stack_.push(value);
}

DISValue DISVM::pop() {
    if (stack_.empty()) {
        runtime_error("Stack underflow");
        return DISValue(int64_t(0));
    }
    
    DISValue value = stack_.top();
    stack_.pop();
    return value;
}

DISValue DISVM::peek() const {
    if (stack_.empty()) {
        return DISValue(int64_t(0));
    }
    return stack_.top();
}

void DISVM::set_global(const std::string& name, const DISValue& value) {
    globals_[name] = value;
}

DISValue DISVM::get_global(const std::string& name) const {
    auto it = globals_.find(name);
    return it != globals_.end() ? it->second : DISValue(int64_t(0));
}

std::vector<std::string> DISVM::get_global_names() const {
    std::vector<std::string> names;
    names.reserve(globals_.size());
    for (const auto& pair : globals_) {
        names.push_back(pair.first);
    }
    // Also include program globals
    for (const auto& pair : program_.globals) {
        if (globals_.find(pair.first) == globals_.end()) {
            names.push_back(pair.first);
        }
    }
    return names;
}

void DISVM::set_ai_handler(std::function<std::string(const std::string&, const std::string&)> handler) {
    ai_handler_ = handler;
}

void DISVM::set_glyph_renderer(std::function<void(const std::string&)> renderer) {
    glyph_renderer_ = renderer;
}

void DISVM::set_vm_spawner(std::function<bool(const std::string&)> spawner) {
    vm_spawner_ = spawner;
}

void DISVM::set_namespace_mounter(std::function<bool(const std::string&, const std::string&)> mounter) {
    namespace_mounter_ = mounter;
}

void DISVM::set_step_callback(std::function<void(size_t pc, const std::string& instruction)> callback) {
    step_callback_ = callback;
}

void DISVM::set_breakpoint_callback(std::function<void(size_t pc)> callback) {
    breakpoint_callback_ = callback;
}

void DISVM::set_error_callback(std::function<void(const std::string& error)> callback) {
    error_callback_ = callback;
}

void DISVM::set_completion_callback(std::function<void()> callback) {
    completion_callback_ = callback;
}

void DISVM::set_output_callback(std::function<void(const std::string& output)> callback) {
    output_callback_ = callback;
}

void DISVM::execute_instruction(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::LOAD:
        case DISOpcode::STORE:
        case DISOpcode::POP:
        case DISOpcode::DUP:
            handle_load(inst);
            break;
            
        case DISOpcode::ADD:
        case DISOpcode::SUB:
        case DISOpcode::MUL:
        case DISOpcode::DIV:
        case DISOpcode::MOD:
            handle_arithmetic(inst);
            break;
            
        case DISOpcode::EQ:
        case DISOpcode::NE:
        case DISOpcode::LT:
        case DISOpcode::GT:
        case DISOpcode::LE:
        case DISOpcode::GE:
            handle_comparison(inst);
            break;
            
        case DISOpcode::JMP:
        case DISOpcode::JMPT:
        case DISOpcode::JMPF:
        case DISOpcode::CALL:
        case DISOpcode::RET:
            handle_control_flow(inst);
            break;
            
        case DISOpcode::NEWSTR:
        case DISOpcode::CONCAT:
        case DISOpcode::STRLEN:
            handle_string_ops(inst);
            break;
            
        case DISOpcode::PRINT:
        case DISOpcode::READ:
            handle_io_ops(inst);
            break;
            
        case DISOpcode::AI_INIT:
        case DISOpcode::AI_COMPLETE:
        case DISOpcode::AI_CHAT:
            handle_ai_ops(inst);
            break;
            
        case DISOpcode::RENDER_GLYPH:
        case DISOpcode::SPAWN_VM:
        case DISOpcode::MOUNT_NS:
            handle_drawkern_ops(inst);
            break;
            
        case DISOpcode::HALT:
            halt();
            break;
            
        default:
            runtime_error("Unknown opcode: " + std::to_string(static_cast<int>(inst.opcode)));
            break;
    }
}

void DISVM::handle_load(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::LOAD:
            if (!inst.operands.empty()) {
                push(inst.operands[0]);
            }
            break;
            
        case DISOpcode::POP:
            if (!stack_.empty()) {
                pop();
            }
            break;
            
        case DISOpcode::DUP:
            if (!stack_.empty()) {
                push(peek());
            }
            break;
            
        default:
            break;
    }
}

void DISVM::handle_arithmetic(const DISInstruction& inst) {
    if (!check_stack_size(2)) return;
    
    DISValue b = pop();
    DISValue a = pop();
    DISValue result(int64_t(0));
    
    if (a.type == DISValue::INT && b.type == DISValue::INT) {
        switch (inst.opcode) {
            case DISOpcode::ADD:
                result = DISValue(a.int_val + b.int_val);
                break;
            case DISOpcode::SUB:
                result = DISValue(a.int_val - b.int_val);
                break;
            case DISOpcode::MUL:
                result = DISValue(a.int_val * b.int_val);
                break;
            case DISOpcode::DIV:
                if (b.int_val != 0) {
                    result = DISValue(a.int_val / b.int_val);
                } else {
                    runtime_error("Division by zero");
                }
                break;
            case DISOpcode::MOD:
                if (b.int_val != 0) {
                    result = DISValue(a.int_val % b.int_val);
                } else {
                    runtime_error("Modulo by zero");
                }
                break;
            default:
                break;
        }
    }
    
    push(result);
}

void DISVM::handle_comparison(const DISInstruction& inst) {
    if (!check_stack_size(2)) return;
    
    DISValue b = pop();
    DISValue a = pop();
    bool result = false;
    
    if (a.type == DISValue::INT && b.type == DISValue::INT) {
        switch (inst.opcode) {
            case DISOpcode::EQ: result = a.int_val == b.int_val; break;
            case DISOpcode::NE: result = a.int_val != b.int_val; break;
            case DISOpcode::LT: result = a.int_val < b.int_val; break;
            case DISOpcode::GT: result = a.int_val > b.int_val; break;
            case DISOpcode::LE: result = a.int_val <= b.int_val; break;
            case DISOpcode::GE: result = a.int_val >= b.int_val; break;
            default: break;
        }
    }
    
    push(DISValue(result ? int64_t(1) : int64_t(0)));
}

void DISVM::handle_control_flow(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::JMP:
            if (!inst.operands.empty() && inst.operands[0].type == DISValue::INT) {
                pc_ = inst.operands[0].int_val;
            }
            break;
            
        case DISOpcode::JMPT:
            if (!check_stack_size(1)) break;
            {
                DISValue condition = pop();
                if (condition.int_val != 0 && !inst.operands.empty()) {
                    pc_ = inst.operands[0].int_val;
                } else {
                    pc_++;
                }
            }
            break;
            
        case DISOpcode::JMPF:
            if (!check_stack_size(1)) break;
            {
                DISValue condition = pop();
                if (condition.int_val == 0 && !inst.operands.empty()) {
                    pc_ = inst.operands[0].int_val;
                } else {
                    pc_++;
                }
            }
            break;
            
        case DISOpcode::CALL:
            call_stack_.push(pc_ + 1);
            if (!inst.operands.empty()) {
                pc_ = inst.operands[0].int_val;
            }
            break;
            
        case DISOpcode::RET:
            if (!call_stack_.empty()) {
                pc_ = call_stack_.top();
                call_stack_.pop();
            } else {
                halt();
            }
            break;
            
        default:
            break;
    }
}

void DISVM::handle_string_ops(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::NEWSTR:
            if (!inst.operands.empty()) {
                push(DISValue(inst.operands[0].string_val));
            }
            break;
            
        case DISOpcode::CONCAT:
            if (!check_stack_size(2)) break;
            {
                DISValue b = pop();
                DISValue a = pop();
                std::string result = a.string_val + b.string_val;
                push(DISValue(result));
            }
            break;
            
        case DISOpcode::STRLEN:
            if (!check_stack_size(1)) break;
            {
                DISValue str = pop();
                push(DISValue(static_cast<int64_t>(str.string_val.length())));
            }
            break;
            
        default:
            break;
    }
}

void DISVM::handle_io_ops(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::PRINT:
            if (!check_stack_size(1)) break;
            {
                DISValue value = pop();
                std::string output;
                if (value.type == DISValue::STRING) {
                    output = value.string_val;
                } else if (value.type == DISValue::INT) {
                    output = std::to_string(value.int_val);
                } else if (value.type == DISValue::FLOAT) {
                    output = std::to_string(value.float_val);
                }

                // Print to stdout
                std::cout << output;
                std::cout.flush();

                // Also invoke output callback for debugger integration
                if (output_callback_) {
                    output_callback_(output);
                }
            }
            break;

        case DISOpcode::READ:
            // Simplified - just push a placeholder
            push(DISValue("input"));
            break;

        default:
            break;
    }
}

void DISVM::handle_ai_ops(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::AI_INIT:
            if (!check_stack_size(1)) break;
            {
                DISValue model = pop();
                std::cout << "🤖 Initializing AI model: " << model.string_val << std::endl;
                push(DISValue(int64_t(1))); // Success
            }
            break;
            
        case DISOpcode::AI_COMPLETE:
            if (!check_stack_size(2)) break;
            {
                DISValue context = pop();
                DISValue prompt = pop();
                
                std::string response;
                if (ai_handler_) {
                    response = ai_handler_(prompt.string_val, context.string_val);
                } else {
                    response = "🤖 AI: " + prompt.string_val + " (completed)";
                }
                
                push(DISValue(response));
            }
            break;
            
        case DISOpcode::AI_CHAT:
            if (!check_stack_size(1)) break;
            {
                DISValue message = pop();
                std::string response;
                
                if (ai_handler_) {
                    response = ai_handler_(message.string_val, "chat");
                } else {
                    response = "🤖 AI Chat: Hello! You said: " + message.string_val;
                }
                
                push(DISValue(response));
            }
            break;
            
        default:
            break;
    }
}

void DISVM::handle_drawkern_ops(const DISInstruction& inst) {
    switch (inst.opcode) {
        case DISOpcode::RENDER_GLYPH:
            if (!check_stack_size(1)) break;
            {
                DISValue glyph = pop();
                if (glyph_renderer_) {
                    glyph_renderer_(glyph.string_val);
                } else {
                    std::cout << "🎨 Rendering glyph: " << glyph.string_val << std::endl;
                }
                push(DISValue(int64_t(1))); // Success
            }
            break;
            
        case DISOpcode::SPAWN_VM:
            if (!check_stack_size(1)) break;
            {
                DISValue spec = pop();
                bool success = false;
                
                if (vm_spawner_) {
                    success = vm_spawner_(spec.string_val);
                } else {
                    std::cout << "🚀 Spawning VM: " << spec.string_val << std::endl;
                    success = true;
                }
                
                push(DISValue(success ? int64_t(1) : int64_t(0)));
            }
            break;
            
        case DISOpcode::MOUNT_NS:
            if (!check_stack_size(2)) break;
            {
                DISValue local_path = pop();
                DISValue remote_addr = pop();
                bool success = false;
                
                if (namespace_mounter_) {
                    success = namespace_mounter_(remote_addr.string_val, local_path.string_val);
                } else {
                    std::cout << "🔗 Mounting namespace: " << remote_addr.string_val 
                              << " -> " << local_path.string_val << std::endl;
                    success = true;
                }
                
                push(DISValue(success ? int64_t(1) : int64_t(0)));
            }
            break;
            
        default:
            break;
    }
}

bool DISVM::check_stack_size(size_t required) const {
    if (stack_.size() < required) {
        const_cast<DISVM*>(this)->runtime_error("Stack underflow - need " + 
                                                std::to_string(required) + " items, have " + 
                                                std::to_string(stack_.size()));
        return false;
    }
    return true;
}

void DISVM::runtime_error(const std::string& message) {
    std::cerr << "DIS VM Runtime Error at PC " << pc_ << ": " << message << std::endl;
    halt();
}

void DISVM::dump_stack() const {
    std::cout << "Stack (" << stack_.size() << " items):" << std::endl;
    // Note: Can't easily iterate through std::stack, so just show size
}

void DISVM::dump_globals() const {
    std::cout << "Globals:" << std::endl;
    for (const auto& [name, value] : globals_) {
        std::cout << "  " << name << " = ";
        if (value.type == DISValue::INT) {
            std::cout << value.int_val;
        } else if (value.type == DISValue::STRING) {
            std::cout << "\"" << value.string_val << "\"";
        }
        std::cout << std::endl;
    }
}

std::string DISVM::get_instruction_info(size_t pc) const {
    if (pc >= program_.instructions.size()) {
        return "Invalid PC";
    }
    
    const DISInstruction& inst = program_.instructions[pc];
    std::stringstream ss;
    ss << "PC " << pc << ": ";
    
    // Convert opcode to string representation
    switch (inst.opcode) {
        case DISOpcode::LOAD: ss << "LOAD"; break;
        case DISOpcode::STORE: ss << "STORE"; break;
        case DISOpcode::POP: ss << "POP"; break;
        case DISOpcode::DUP: ss << "DUP"; break;
        case DISOpcode::ADD: ss << "ADD"; break;
        case DISOpcode::SUB: ss << "SUB"; break;
        case DISOpcode::MUL: ss << "MUL"; break;
        case DISOpcode::DIV: ss << "DIV"; break;
        case DISOpcode::MOD: ss << "MOD"; break;
        case DISOpcode::EQ: ss << "EQ"; break;
        case DISOpcode::NE: ss << "NE"; break;
        case DISOpcode::LT: ss << "LT"; break;
        case DISOpcode::GT: ss << "GT"; break;
        case DISOpcode::LE: ss << "LE"; break;
        case DISOpcode::GE: ss << "GE"; break;
        case DISOpcode::JMP: ss << "JMP"; break;
        case DISOpcode::JMPT: ss << "JMPT"; break;
        case DISOpcode::JMPF: ss << "JMPF"; break;
        case DISOpcode::CALL: ss << "CALL"; break;
        case DISOpcode::RET: ss << "RET"; break;
        case DISOpcode::NEWSTR: ss << "NEWSTR"; break;
        case DISOpcode::CONCAT: ss << "CONCAT"; break;
        case DISOpcode::STRLEN: ss << "STRLEN"; break;
        case DISOpcode::PRINT: ss << "PRINT"; break;
        case DISOpcode::READ: ss << "READ"; break;
        case DISOpcode::AI_INIT: ss << "AI_INIT"; break;
        case DISOpcode::AI_COMPLETE: ss << "AI_COMPLETE"; break;
        case DISOpcode::AI_CHAT: ss << "AI_CHAT"; break;
        case DISOpcode::RENDER_GLYPH: ss << "RENDER_GLYPH"; break;
        case DISOpcode::SPAWN_VM: ss << "SPAWN_VM"; break;
        case DISOpcode::MOUNT_NS: ss << "MOUNT_NS"; break;
        case DISOpcode::HALT: ss << "HALT"; break;
        default: ss << "UNKNOWN(" << static_cast<int>(inst.opcode) << ")"; break;
    }
    
    if (!inst.operands.empty()) {
        ss << " ";
        for (size_t i = 0; i < inst.operands.size(); ++i) {
            if (i > 0) ss << ", ";
            const auto& operand = inst.operands[i];
            if (operand.type == DISValue::INT) {
                ss << operand.int_val;
            } else if (operand.type == DISValue::STRING) {
                ss << "\"" << operand.string_val << "\"";
            }
        }
    }
    
    return ss.str();
}

// Breakpoint management
bool DISVM::set_breakpoint(size_t pc) {
    if (pc >= program_.instructions.size()) {
        return false;
    }
    
    breakpoints_.insert(pc);
    std::cout << "🔴 Breakpoint set at PC " << pc << std::endl;
    return true;
}

bool DISVM::remove_breakpoint(size_t pc) {
    auto it = breakpoints_.find(pc);
    if (it != breakpoints_.end()) {
        breakpoints_.erase(it);
        std::cout << "🔴 Breakpoint removed from PC " << pc << std::endl;
        return true;
    }
    return false;
}

void DISVM::clear_all_breakpoints() {
    breakpoints_.clear();
    std::cout << "🔴 All breakpoints cleared" << std::endl;
}

std::vector<size_t> DISVM::get_breakpoints() const {
    return std::vector<size_t>(breakpoints_.begin(), breakpoints_.end());
}

bool DISVM::has_breakpoint(size_t pc) const {
    return breakpoints_.find(pc) != breakpoints_.end();
}

// Advanced debugging controls
void DISVM::step_over() {
    if (!running_ || halted_) {
        return;
    }
    
    step_mode_ = true;
    step_depth_ = call_stack_.size(); // Don't step into calls
    at_breakpoint_ = false;
    
    // Execute one instruction
    step();
    
    std::cout << "🔧 Step over at PC " << pc_ << std::endl;
}

void DISVM::step_into() {
    if (!running_ || halted_) {
        return;
    }
    
    step_mode_ = true;
    step_depth_ = 0; // Step into calls
    at_breakpoint_ = false;
    
    // Execute one instruction
    step();
    
    std::cout << "🔧 Step into at PC " << pc_ << std::endl;
}

void DISVM::step_out() {
    if (!running_ || halted_ || call_stack_.empty()) {
        return;
    }
    
    step_mode_ = true;
    step_depth_ = call_stack_.size() - 1; // Step out of current function
    at_breakpoint_ = false;
    
    std::cout << "🔧 Stepping out from call depth " << call_stack_.size() << std::endl;
    
    // Continue until we return from current function
    while (running_ && !halted_ && call_stack_.size() > step_depth_ && pc_ < program_.instructions.size()) {
        step();
    }
    
    step_mode_ = false;
    step_depth_ = 0;
}

void DISVM::continue_execution() {
    if (!running_ || halted_) {
        return;
    }
    
    step_mode_ = false;
    step_depth_ = 0;
    at_breakpoint_ = false;
    
    std::cout << "▶️ Continuing execution from PC " << pc_ << std::endl;
    
    // Continue until breakpoint or halt
    while (running_ && !halted_ && !at_breakpoint_ && pc_ < program_.instructions.size()) {
        step();
    }
}

bool DISVM::is_at_breakpoint() const {
    return at_breakpoint_;
}

// Call stack inspection
std::vector<size_t> DISVM::get_call_stack() const {
    std::vector<size_t> result;
    std::stack<size_t> temp = call_stack_;
    
    while (!temp.empty()) {
        result.push_back(temp.top());
        temp.pop();
    }
    
    // Reverse to show most recent call first
    std::reverse(result.begin(), result.end());
    return result;
}

size_t DISVM::get_call_stack_depth() const {
    return call_stack_.size();
}

// LimboCompiler implementation (simplified)
LimboCompiler::LimboCompiler() {
}

LimboCompiler::~LimboCompiler() {
}

DISProgram LimboCompiler::compile(const std::string& source) {
    errors_.clear();
    
    std::vector<std::string> tokens = tokenize(source);
    return parse_tokens(tokens);
}

std::vector<std::string> LimboCompiler::tokenize(const std::string& source) {
    std::vector<std::string> tokens;
    std::istringstream stream(source);
    std::string token;
    
    while (stream >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

DISProgram LimboCompiler::parse_tokens(const std::vector<std::string>& tokens) {
    DISProgram program;
    program.source_limbo = "";
    
    // Very simplified compiler - just handle basic cases for demo
    for (size_t i = 0; i < tokens.size(); i++) {
        const std::string& token = tokens[i];
        
        if (token == "print") {
            if (i + 1 < tokens.size()) {
                program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue(tokens[i + 1])));
                program.add_instruction(DISInstruction(DISOpcode::PRINT));
                i++; // Skip next token
            }
        } else if (token == "ai_chat") {
            if (i + 1 < tokens.size()) {
                program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue(tokens[i + 1])));
                program.add_instruction(DISInstruction(DISOpcode::AI_CHAT));
                program.add_instruction(DISInstruction(DISOpcode::PRINT));
                i++; // Skip next token
            }
        } else if (token == "render_glyph") {
            if (i + 1 < tokens.size()) {
                program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue(tokens[i + 1])));
                program.add_instruction(DISInstruction(DISOpcode::RENDER_GLYPH));
                i++; // Skip next token
            }
        } else if (token == "halt") {
            program.add_instruction(DISInstruction(DISOpcode::HALT));
        }
    }
    
    return program;
}

// DISProgramFactory implementation
DISProgram DISProgramFactory::create_ai_workbench(const std::string& model_name) {
    DISProgram program;
    
    // Initialize AI model
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue(model_name)));
    program.add_instruction(DISInstruction(DISOpcode::AI_INIT));
    
    // Print welcome message
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("🤖 AI Workbench ready!\n")));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    // Simple chat loop (simplified for demo)
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("Hello from AI workbench")));
    program.add_instruction(DISInstruction(DISOpcode::AI_CHAT));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    program.add_instruction(DISInstruction(DISOpcode::HALT));
    
    return program;
}

DISProgram DISProgramFactory::create_echo_server() {
    DISProgram program;
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("Echo server started\n")));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    program.add_instruction(DISInstruction(DISOpcode::HALT));
    
    return program;
}

DISProgram DISProgramFactory::create_file_server(const std::string& root_path) {
    DISProgram program;
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("🗂️ File server starting...\n")));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("Root path: " + root_path + "\n")));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    program.add_instruction(DISInstruction(DISOpcode::HALT));
    
    return program;
}

DISProgram DISProgramFactory::create_drawkern_renderer() {
    DISProgram program;
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("🎨 DrawKern Renderer\n")));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("ai-workbench-glyph")));
    program.add_instruction(DISInstruction(DISOpcode::RENDER_GLYPH));
    
    program.add_instruction(DISInstruction(DISOpcode::HALT));
    
    return program;
}

DISProgram DISProgramFactory::from_limbo_source(const std::string& source) {
    LimboCompiler compiler;
    return compiler.compile(source);
}

// DISVMManager implementation
DISVMManager::DISVMManager() : next_vm_id_(1) {
}

DISVMManager::~DISVMManager() {
    // Clean up all VMs
    for (auto& [vm_id, vm] : vms_) {
        vm->halt();
    }
}

std::string DISVMManager::create_vm(const DISProgram& program) {
    std::string vm_id = generate_vm_id();
    
    auto vm = std::make_unique<DISVM>();
    if (!vm->load_program(program)) {
        return "";
    }
    
    vms_[vm_id] = std::move(vm);
    vm_message_queues_[vm_id] = {};
    
    std::cout << "Created DIS VM: " << vm_id << std::endl;
    return vm_id;
}

bool DISVMManager::start_vm(const std::string& vm_id) {
    auto it = vms_.find(vm_id);
    if (it == vms_.end()) {
        return false;
    }
    
    // Run VM in separate thread
    std::thread vm_thread([&vm = it->second]() {
        vm->run();
    });
    vm_thread.detach();
    
    std::cout << "Started DIS VM: " << vm_id << std::endl;
    return true;
}

bool DISVMManager::stop_vm(const std::string& vm_id) {
    auto it = vms_.find(vm_id);
    if (it == vms_.end()) {
        return false;
    }
    
    it->second->halt();
    std::cout << "Stopped DIS VM: " << vm_id << std::endl;
    return true;
}

bool DISVMManager::destroy_vm(const std::string& vm_id) {
    auto it = vms_.find(vm_id);
    if (it == vms_.end()) {
        return false;
    }
    
    it->second->halt();
    vms_.erase(it);
    vm_message_queues_.erase(vm_id);
    
    std::cout << "Destroyed DIS VM: " << vm_id << std::endl;
    return true;
}

std::vector<std::string> DISVMManager::list_vms() const {
    std::vector<std::string> vm_list;
    for (const auto& [vm_id, vm] : vms_) {
        vm_list.push_back(vm_id);
    }
    return vm_list;
}

bool DISVMManager::is_vm_running(const std::string& vm_id) const {
    auto it = vms_.find(vm_id);
    return it != vms_.end() && it->second->is_running();
}

std::string DISVMManager::get_vm_status(const std::string& vm_id) const {
    auto it = vms_.find(vm_id);
    if (it == vms_.end()) {
        return "not found";
    }
    
    return it->second->is_running() ? "running" : "stopped";
}

std::string DISVMManager::generate_vm_id() {
    return "vm_" + std::to_string(next_vm_id_++);
}

} // namespace drawkern
} // namespace bolt