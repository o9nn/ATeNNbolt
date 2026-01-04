#pragma once

#include "bolt/editor/debugger_interface.hpp"
#include <string>
#include <vector>
#include <memory>

namespace bolt {

/**
 * @brief Debugger UI Component
 * 
 * Provides user interface elements for the integrated debugger,
 * including control panels, variable inspection, and debug output.
 */
class DebuggerUI {
public:
    DebuggerUI();
    ~DebuggerUI();
    
    // UI State management
    void initialize();
    void shutdown();
    bool is_visible() const;
    void set_visible(bool visible);
    
    // Rendering
    void render();
    void render_controls();
    void render_breakpoints();
    void render_call_stack();
    void render_variables();
    void render_watch_expressions();
    void render_debug_output();
    
    // Debugger integration
    void set_debugger(std::shared_ptr<DebuggerInterface> debugger);
    void on_debug_event(DebugEvent event, const std::string& message);
    
    // UI Actions
    void handle_continue_clicked();
    void handle_step_over_clicked();
    void handle_step_into_clicked();
    void handle_step_out_clicked();
    void handle_pause_clicked();
    void handle_stop_clicked();
    
    // Breakpoint UI
    void handle_breakpoint_toggle(size_t pc);
    void handle_breakpoint_condition_edit(size_t pc, const std::string& condition);
    
    // Watch expression UI
    void handle_add_watch_expression();
    void handle_remove_watch_expression(const std::string& expression);
    
    // Layout and appearance
    void set_layout_style(const std::string& style);
    void set_panel_size(float width, float height);
    void set_auto_refresh(bool enabled, float interval_ms = 100.0f);
    
private:
    std::shared_ptr<DebuggerInterface> debugger_;
    bool visible_;
    bool initialized_;
    
    // UI State
    std::string new_watch_expression_;
    std::string status_message_;
    bool auto_refresh_enabled_;
    float refresh_interval_ms_;
    float last_refresh_time_;
    
    // Panel sizes and layout
    float controls_panel_height_;
    float variables_panel_width_;
    float output_panel_height_;
    
    // Helper methods
    void render_control_button(const std::string& label, bool enabled, std::function<void()> callback);
    void render_breakpoint_item(const BreakpointInfo& bp);
    void render_watch_item(const WatchExpression& watch);
    void render_call_stack_item(size_t pc, size_t index);
    void update_status_message();
    bool should_refresh() const;
    std::string format_debug_state(DebugState state) const;
    std::string format_pc_address(size_t pc) const;
};

} // namespace bolt