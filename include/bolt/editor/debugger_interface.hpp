#pragma once

#include "bolt/drawkern/dis_vm.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// Fix Windows macro collision
#ifdef ERROR
#undef ERROR
#endif

namespace bolt {

// Forward declarations
class IntegratedEditor;

// Debugger session state
enum class DebugState {
    STOPPED,      // No debugging session active
    RUNNING,      // Executing code
    PAUSED,       // Paused at breakpoint or after step
    FINISHED,     // Execution completed
    ERROR         // Error occurred during debugging
};

// Debug event types for UI notifications
enum class DebugEvent {
    SESSION_STARTED,
    SESSION_STOPPED,
    BREAKPOINT_HIT,
    STEP_COMPLETED,
    EXECUTION_FINISHED,
    ERROR_OCCURRED
};

// Watch expression for variable monitoring
struct WatchExpression {
    std::string expression;
    std::string value;
    std::string type;
    bool valid;
    
    WatchExpression(const std::string& expr) 
        : expression(expr), valid(false) {}
};

// Breakpoint information
struct BreakpointInfo {
    size_t pc;
    std::string file_path;
    size_t line_number;
    bool enabled;
    std::string condition;  // For conditional breakpoints (future enhancement)
    
    BreakpointInfo(size_t pc_val, const std::string& file = "", size_t line = 0)
        : pc(pc_val), file_path(file), line_number(line), enabled(true) {}
};

// Debug event callback
using DebugEventCallback = std::function<void(DebugEvent event, const std::string& message)>;

/**
 * @brief Integrated Debugger Interface
 * 
 * Provides a high-level interface for debugging DIS VM programs,
 * integrating with the editor to provide breakpoint management,
 * step debugging, and variable inspection.
 */
class DebuggerInterface {
public:
    DebuggerInterface();
    ~DebuggerInterface();
    
    // Session management
    bool start_debug_session(const drawkern::DISProgram& program);
    bool start_debug_session_from_source(const std::string& limbo_source);
    void stop_debug_session();
    bool is_debugging() const;
    DebugState get_debug_state() const;
    
    // Execution control
    void continue_execution();
    void step_over();
    void step_into();
    void step_out();
    void pause_execution();
    
    // Breakpoint management
    bool set_breakpoint(size_t pc);
    bool set_breakpoint_at_line(const std::string& file_path, size_t line);
    bool remove_breakpoint(size_t pc);
    bool remove_breakpoint_at_line(const std::string& file_path, size_t line);
    void clear_all_breakpoints();
    bool toggle_breakpoint(size_t pc);
    bool enable_breakpoint(size_t pc, bool enabled);
    bool set_breakpoint_condition(size_t pc, const std::string& condition);
    std::vector<BreakpointInfo> get_all_breakpoints() const;
    bool has_breakpoint(size_t pc) const;
    
    // State inspection
    size_t get_current_pc() const;
    std::string get_current_instruction() const;
    std::vector<size_t> get_call_stack() const;
    size_t get_call_stack_depth() const;
    std::vector<std::string> get_stack_contents() const;
    std::map<std::string, std::string> get_global_variables() const;
    
    // Watch expressions
    bool add_watch_expression(const std::string& expression);
    bool remove_watch_expression(const std::string& expression);
    void clear_watch_expressions();
    std::vector<WatchExpression> get_watch_expressions() const;
    void update_watch_expressions();
    
    // Event handling
    void set_event_callback(DebugEventCallback callback);
    
    // Integration with editor
    void set_editor(IntegratedEditor* editor);
    void highlight_current_line();
    void clear_current_line_highlight();
    void refresh_breakpoint_markers();
    
    // Program information
    size_t get_program_size() const;
    std::string get_instruction_at(size_t pc) const;
    bool is_valid_pc(size_t pc) const;
    
    // Debug output and logging
    void enable_debug_output(bool enabled);
    std::vector<std::string> get_debug_log() const;
    void clear_debug_log();
    
private:
    std::unique_ptr<drawkern::DISVM> vm_;
    std::vector<BreakpointInfo> breakpoints_;
    std::vector<WatchExpression> watch_expressions_;
    std::map<std::string, size_t> file_line_to_pc_;  // File/line to PC mapping
    std::map<size_t, std::pair<std::string, size_t>> pc_to_file_line_;  // PC to file/line mapping
    
    DebugState state_;
    DebugEventCallback event_callback_;
    IntegratedEditor* editor_;
    
    std::vector<std::string> debug_log_;
    bool debug_output_enabled_;
    
    // Current debug position tracking
    std::string current_debug_file_;
    size_t current_debug_line_;
    
    // Internal helpers
    void transition_state(DebugState new_state, const std::string& message = "");
    void fire_event(DebugEvent event, const std::string& message = "");
    void log_debug_message(const std::string& message);
    void update_breakpoint_mapping();
    size_t find_pc_for_line(const std::string& file_path, size_t line) const;
    std::pair<std::string, size_t> find_line_for_pc(size_t pc) const;
    std::string evaluate_watch_expression(const std::string& expression);
    void setup_vm_handlers();
};

} // namespace bolt