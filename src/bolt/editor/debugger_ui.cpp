#include "bolt/editor/debugger_ui.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace bolt {

DebuggerUI::DebuggerUI()
    : debugger_(nullptr)
    , visible_(false)
    , initialized_(false)
    , auto_refresh_enabled_(true)
    , refresh_interval_ms_(100.0f)
    , last_refresh_time_(0.0f)
    , controls_panel_height_(60.0f)
    , variables_panel_width_(300.0f)
    , output_panel_height_(200.0f) {
}

DebuggerUI::~DebuggerUI() {
    shutdown();
}

void DebuggerUI::initialize() {
    if (initialized_) {
        return;
    }
    
    initialized_ = true;
    status_message_ = "Debugger UI initialized";
    
    std::cout << "🔧 Debugger UI initialized" << std::endl;
}

void DebuggerUI::shutdown() {
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    visible_ = false;
    
    std::cout << "🔧 Debugger UI shutdown" << std::endl;
}

bool DebuggerUI::is_visible() const {
    return visible_;
}

void DebuggerUI::set_visible(bool visible) {
    if (visible && !initialized_) {
        initialize();
    }
    
    visible_ = visible;
    
    if (visible) {
        std::cout << "🔧 Debugger UI shown" << std::endl;
    } else {
        std::cout << "🔧 Debugger UI hidden" << std::endl;
    }
}

void DebuggerUI::render() {
    if (!visible_ || !initialized_) {
        return;
    }
    
    // Check if we should refresh the UI
    if (should_refresh()) {
        update_status_message();
        last_refresh_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    // Render all panels
    render_controls();
    render_breakpoints();
    render_call_stack();
    render_variables();
    render_watch_expressions();
    render_debug_output();
}

void DebuggerUI::render_controls() {
    std::cout << "=== Debug Controls ===" << std::endl;
    
    if (!debugger_) {
        std::cout << "No debugger attached" << std::endl;
        return;
    }
    
    DebugState state = debugger_->get_debug_state();
    std::cout << "State: " << format_debug_state(state) << std::endl;
    std::cout << "PC: " << format_pc_address(debugger_->get_current_pc()) << std::endl;
    
    // Show available controls based on state
    switch (state) {
        case DebugState::STOPPED:
            std::cout << "Controls: [Start Debug Session]" << std::endl;
            break;
            
        case DebugState::PAUSED:
            std::cout << "Controls: [Continue] [Step Over] [Step Into] [Step Out] [Stop]" << std::endl;
            break;
            
        case DebugState::RUNNING:
            std::cout << "Controls: [Pause] [Stop]" << std::endl;
            break;
            
        case DebugState::FINISHED:
            std::cout << "Controls: [Stop]" << std::endl;
            break;
            
        case DebugState::ERROR:
            std::cout << "Controls: [Stop] (Error occurred)" << std::endl;
            break;
    }
    
    if (!status_message_.empty()) {
        std::cout << "Status: " << status_message_ << std::endl;
    }
    
    std::cout << std::endl;
}

void DebuggerUI::render_breakpoints() {
    std::cout << "=== Breakpoints ===" << std::endl;
    
    if (!debugger_) {
        std::cout << "No debugger attached" << std::endl;
        return;
    }
    
    auto breakpoints = debugger_->get_all_breakpoints();
    if (breakpoints.empty()) {
        std::cout << "No breakpoints set" << std::endl;
    } else {
        for (const auto& bp : breakpoints) {
            render_breakpoint_item(bp);
        }
    }
    
    std::cout << std::endl;
}

void DebuggerUI::render_call_stack() {
    std::cout << "=== Call Stack ===" << std::endl;
    
    if (!debugger_) {
        std::cout << "No debugger attached" << std::endl;
        return;
    }
    
    auto call_stack = debugger_->get_call_stack();
    if (call_stack.empty()) {
        std::cout << "No active calls" << std::endl;
    } else {
        for (size_t i = 0; i < call_stack.size(); ++i) {
            render_call_stack_item(call_stack[i], i);
        }
    }
    
    std::cout << std::endl;
}

void DebuggerUI::render_variables() {
    std::cout << "=== Variables ===" << std::endl;
    
    if (!debugger_) {
        std::cout << "No debugger attached" << std::endl;
        return;
    }
    
    auto globals = debugger_->get_global_variables();
    if (globals.empty()) {
        std::cout << "No global variables" << std::endl;
    } else {
        for (const auto& [name, value] : globals) {
            std::cout << "  " << name << " = " << value << std::endl;
        }
    }
    
    auto stack_contents = debugger_->get_stack_contents();
    if (!stack_contents.empty()) {
        std::cout << "Stack:" << std::endl;
        for (const auto& item : stack_contents) {
            std::cout << "  " << item << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void DebuggerUI::render_watch_expressions() {
    std::cout << "=== Watch Expressions ===" << std::endl;
    
    if (!debugger_) {
        std::cout << "No debugger attached" << std::endl;
        return;
    }
    
    auto watches = debugger_->get_watch_expressions();
    if (watches.empty()) {
        std::cout << "No watch expressions" << std::endl;
    } else {
        for (const auto& watch : watches) {
            render_watch_item(watch);
        }
    }
    
    std::cout << "Add new watch: [expression]" << std::endl;
    std::cout << std::endl;
}

void DebuggerUI::render_debug_output() {
    std::cout << "=== Debug Output ===" << std::endl;
    
    if (!debugger_) {
        std::cout << "No debugger attached" << std::endl;
        return;
    }
    
    auto log = debugger_->get_debug_log();
    if (log.empty()) {
        std::cout << "No debug output" << std::endl;
    } else {
        // Show last few entries
        size_t start = log.size() > 5 ? log.size() - 5 : 0;
        for (size_t i = start; i < log.size(); ++i) {
            std::cout << "  " << log[i] << std::endl;
        }
        
        if (log.size() > 5) {
            std::cout << "  (" << (log.size() - 5) << " more entries...)" << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void DebuggerUI::set_debugger(std::shared_ptr<DebuggerInterface> debugger) {
    debugger_ = debugger;
    
    if (debugger_) {
        // Set up event callback
        debugger_->set_event_callback([this](DebugEvent event, const std::string& message) {
            on_debug_event(event, message);
        });
        
        std::cout << "🔧 Debugger attached to UI" << std::endl;
    } else {
        std::cout << "🔧 Debugger detached from UI" << std::endl;
    }
}

void DebuggerUI::on_debug_event(DebugEvent event, const std::string& message) {
    switch (event) {
        case DebugEvent::SESSION_STARTED:
            status_message_ = "Debug session started";
            break;
            
        case DebugEvent::SESSION_STOPPED:
            status_message_ = "Debug session stopped";
            break;
            
        case DebugEvent::BREAKPOINT_HIT:
            status_message_ = "Breakpoint hit: " + message;
            break;
            
        case DebugEvent::STEP_COMPLETED:
            status_message_ = "Step completed";
            break;
            
        case DebugEvent::EXECUTION_FINISHED:
            status_message_ = "Execution finished";
            break;
            
        case DebugEvent::ERROR_OCCURRED:
            status_message_ = "Error: " + message;
            break;
    }
    
    std::cout << "🔧 Debug event: " << status_message_ << std::endl;
}

// UI Action handlers
void DebuggerUI::handle_continue_clicked() {
    if (debugger_) {
        debugger_->continue_execution();
    }
}

void DebuggerUI::handle_step_over_clicked() {
    if (debugger_) {
        debugger_->step_over();
    }
}

void DebuggerUI::handle_step_into_clicked() {
    if (debugger_) {
        debugger_->step_into();
    }
}

void DebuggerUI::handle_step_out_clicked() {
    if (debugger_) {
        debugger_->step_out();
    }
}

void DebuggerUI::handle_pause_clicked() {
    if (debugger_) {
        debugger_->pause_execution();
    }
}

void DebuggerUI::handle_stop_clicked() {
    if (debugger_) {
        debugger_->stop_debug_session();
    }
}

void DebuggerUI::handle_breakpoint_toggle(size_t pc) {
    if (debugger_) {
        debugger_->toggle_breakpoint(pc);
    }
}

void DebuggerUI::handle_breakpoint_condition_edit(size_t pc, const std::string& condition) {
    if (!debugger_) {
        std::cout << "❌ No debugger attached" << std::endl;
        return;
    }
    
    // Set or update the breakpoint condition
    if (condition.empty()) {
        // Remove condition (make it unconditional)
        debugger_->set_breakpoint_condition(pc, "");
        std::cout << "✅ Removed condition from breakpoint at PC " << pc << std::endl;
    } else {
        // Set new condition
        debugger_->set_breakpoint_condition(pc, condition);
        std::cout << "✅ Set breakpoint condition at PC " << pc << ": " << condition << std::endl;
    }
    
    status_message_ = "Breakpoint condition updated";
}

void DebuggerUI::handle_add_watch_expression() {
    if (debugger_ && !new_watch_expression_.empty()) {
        debugger_->add_watch_expression(new_watch_expression_);
        new_watch_expression_.clear();
    }
}

void DebuggerUI::handle_remove_watch_expression(const std::string& expression) {
    if (debugger_) {
        debugger_->remove_watch_expression(expression);
    }
}

void DebuggerUI::set_layout_style(const std::string& style) {
    std::cout << "🔧 Layout style set to: " << style << std::endl;
}

void DebuggerUI::set_panel_size(float width, float height) {
    variables_panel_width_ = width;
    output_panel_height_ = height;
}

void DebuggerUI::set_auto_refresh(bool enabled, float interval_ms) {
    auto_refresh_enabled_ = enabled;
    refresh_interval_ms_ = interval_ms;
}

// Private helper methods
void DebuggerUI::render_control_button(const std::string& label, bool enabled, std::function<void()> callback) {
    std::cout << "[" << label << "]";
    if (enabled) {
        std::cout << " (enabled)";
    } else {
        std::cout << " (disabled)";
    }
    std::cout << " ";
}

void DebuggerUI::render_breakpoint_item(const BreakpointInfo& bp) {
    std::cout << "  ";
    std::cout << (bp.enabled ? "🔴" : "⚪") << " ";
    std::cout << "PC " << format_pc_address(bp.pc);
    
    if (!bp.file_path.empty()) {
        std::cout << " (" << bp.file_path << ":" << bp.line_number << ")";
    }
    
    if (!bp.condition.empty()) {
        std::cout << " [" << bp.condition << "]";
    }
    
    std::cout << std::endl;
}

void DebuggerUI::render_watch_item(const WatchExpression& watch) {
    std::cout << "  " << watch.expression << " = ";
    
    if (watch.valid) {
        std::cout << watch.value << " (" << watch.type << ")";
    } else {
        std::cout << "<invalid>";
    }
    
    std::cout << std::endl;
}

void DebuggerUI::render_call_stack_item(size_t pc, size_t index) {
    std::cout << "  #" << index << " " << format_pc_address(pc);
    
    if (debugger_) {
        std::string instruction = debugger_->get_instruction_at(pc);
        if (!instruction.empty()) {
            std::cout << " - " << instruction;
        }
    }
    
    std::cout << std::endl;
}

void DebuggerUI::update_status_message() {
    if (!debugger_) {
        status_message_ = "No debugger attached";
        return;
    }
    
    DebugState state = debugger_->get_debug_state();
    if (state == DebugState::RUNNING) {
        status_message_ = "Running...";
    } else if (state == DebugState::PAUSED) {
        status_message_ = "Paused at PC " + format_pc_address(debugger_->get_current_pc());
    }
}

bool DebuggerUI::should_refresh() const {
    if (!auto_refresh_enabled_) {
        return false;
    }
    
    float current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
        
    return (current_time - last_refresh_time_) >= refresh_interval_ms_;
}

std::string DebuggerUI::format_debug_state(DebugState state) const {
    switch (state) {
        case DebugState::STOPPED: return "STOPPED";
        case DebugState::RUNNING: return "RUNNING";
        case DebugState::PAUSED: return "PAUSED";
        case DebugState::FINISHED: return "FINISHED";
        case DebugState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string DebuggerUI::format_pc_address(size_t pc) const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << pc;
    return ss.str();
}

} // namespace bolt