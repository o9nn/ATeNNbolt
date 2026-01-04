#include "bolt/test_framework.hpp"
#include "bolt/editor/debugger_interface.hpp"
#include "bolt/editor/debugger_ui.hpp"
#include "bolt/drawkern/dis_vm.hpp"
#include <memory>

using namespace bolt;
using namespace bolt::drawkern;

BOLT_TEST(Debugger, DebuggerInterface_Creation) {
    auto debugger = std::make_unique<DebuggerInterface>();
    BOLT_ASSERT_NOT_NULL(debugger.get());
    BOLT_ASSERT_EQ(static_cast<int>(DebugState::STOPPED), static_cast<int>(debugger->get_debug_state()));
    BOLT_ASSERT_FALSE(debugger->is_debugging());
}

BOLT_TEST(Debugger, BreakpointManagement) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Start a simple debug session first
    std::string limboSource = "print \"test\" halt";
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    
    // Test setting breakpoints (now within a valid program)
    BOLT_ASSERT_TRUE(debugger->set_breakpoint(1));
    BOLT_ASSERT_TRUE(debugger->has_breakpoint(1));
    
    // Test breakpoint list
    auto breakpoints = debugger->get_all_breakpoints();
    BOLT_ASSERT_EQ(1, breakpoints.size());
    BOLT_ASSERT_EQ(1, breakpoints[0].pc);
    BOLT_ASSERT_TRUE(breakpoints[0].enabled);
    
    // Test removing breakpoints
    BOLT_ASSERT_TRUE(debugger->remove_breakpoint(1));
    BOLT_ASSERT_FALSE(debugger->has_breakpoint(1));
    BOLT_ASSERT_EQ(0, debugger->get_all_breakpoints().size());
}

BOLT_TEST(Debugger, BreakpointToggle) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Start a simple debug session first
    std::string limboSource = "print \"test\" halt";
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    
    // Test toggle (should add)
    BOLT_ASSERT_TRUE(debugger->toggle_breakpoint(0));
    BOLT_ASSERT_TRUE(debugger->has_breakpoint(0));
    
    // Test toggle again (should remove)
    BOLT_ASSERT_TRUE(debugger->toggle_breakpoint(0));
    BOLT_ASSERT_FALSE(debugger->has_breakpoint(0));
}

BOLT_TEST(Debugger, BreakpointEnableDisable) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Start a simple debug session first
    std::string limboSource = "print \"test\" halt";
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    
    // Set a breakpoint
    BOLT_ASSERT_TRUE(debugger->set_breakpoint(1));
    
    // Test disabling
    BOLT_ASSERT_TRUE(debugger->enable_breakpoint(1, false));
    auto breakpoints = debugger->get_all_breakpoints();
    BOLT_ASSERT_EQ(1, breakpoints.size());
    BOLT_ASSERT_FALSE(breakpoints[0].enabled);
    
    // Test re-enabling
    BOLT_ASSERT_TRUE(debugger->enable_breakpoint(1, true));
    breakpoints = debugger->get_all_breakpoints();
    BOLT_ASSERT_EQ(1, breakpoints.size());
    BOLT_ASSERT_TRUE(breakpoints[0].enabled);
}

BOLT_TEST(Debugger, ClearAllBreakpoints) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Start a simple debug session first
    std::string limboSource = "print \"test1\" print \"test2\" print \"test3\" halt";
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    
    // Set multiple breakpoints
    debugger->set_breakpoint(0);
    debugger->set_breakpoint(2);
    debugger->set_breakpoint(4);
    
    BOLT_ASSERT_EQ(3, debugger->get_all_breakpoints().size());
    
    // Clear all
    debugger->clear_all_breakpoints();
    BOLT_ASSERT_EQ(0, debugger->get_all_breakpoints().size());
}

BOLT_TEST(Debugger, WatchExpressions) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Test adding watch expressions
    BOLT_ASSERT_TRUE(debugger->add_watch_expression("variable1"));
    BOLT_ASSERT_TRUE(debugger->add_watch_expression("counter + 1"));
    
    // Test duplicate (should fail)
    BOLT_ASSERT_FALSE(debugger->add_watch_expression("variable1"));
    
    auto watches = debugger->get_watch_expressions();
    BOLT_ASSERT_EQ(2, watches.size());
    BOLT_ASSERT_EQ("variable1", watches[0].expression);
    BOLT_ASSERT_EQ("counter + 1", watches[1].expression);
    
    // Test removing watch expressions
    BOLT_ASSERT_TRUE(debugger->remove_watch_expression("variable1"));
    BOLT_ASSERT_FALSE(debugger->remove_watch_expression("nonexistent"));
    
    watches = debugger->get_watch_expressions();
    BOLT_ASSERT_EQ(1, watches.size());
    BOLT_ASSERT_EQ("counter + 1", watches[0].expression);
    
    // Test clearing all
    debugger->clear_watch_expressions();
    BOLT_ASSERT_EQ(0, debugger->get_watch_expressions().size());
}

BOLT_TEST(Debugger, DebugSession_SimpleProgram) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Create a simple Limbo program
    std::string limboSource = "print \"Hello, World!\" halt";
    
    // Start debug session
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    BOLT_ASSERT_TRUE(debugger->is_debugging());
    BOLT_ASSERT_EQ(static_cast<int>(DebugState::PAUSED), static_cast<int>(debugger->get_debug_state()));
    
    // Check initial state
    BOLT_ASSERT_EQ(0, debugger->get_current_pc());
    BOLT_ASSERT_EQ(0, debugger->get_call_stack_depth());
    
    // Stop session
    debugger->stop_debug_session();
    BOLT_ASSERT_FALSE(debugger->is_debugging());
    BOLT_ASSERT_EQ(static_cast<int>(DebugState::STOPPED), static_cast<int>(debugger->get_debug_state()));
}

BOLT_TEST(Debugger, DebuggerUI_Creation) {
    auto ui = std::make_unique<DebuggerUI>();
    BOLT_ASSERT_NOT_NULL(ui.get());
    BOLT_ASSERT_FALSE(ui->is_visible());
    
    // Test visibility toggle
    ui->set_visible(true);
    BOLT_ASSERT_TRUE(ui->is_visible());
    
    ui->set_visible(false);
    BOLT_ASSERT_FALSE(ui->is_visible());
}

BOLT_TEST(Debugger, DebuggerUI_DebuggerIntegration) {
    auto debugger = std::make_shared<DebuggerInterface>();
    auto ui = std::make_unique<DebuggerUI>();
    
    // Test setting debugger
    ui->set_debugger(debugger);
    
    // Start a debug session first
    std::string limboSource = "print \"test\" halt";
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    
    // Test UI operations (should not crash)
    ui->handle_continue_clicked();
    ui->handle_step_over_clicked();
    ui->handle_step_into_clicked();
    ui->handle_step_out_clicked();
    ui->handle_pause_clicked();
    ui->handle_stop_clicked();
    
    // Test breakpoint operations
    ui->handle_breakpoint_toggle(0);
    BOLT_ASSERT_TRUE(debugger->has_breakpoint(0));
    
    ui->handle_breakpoint_toggle(0);
    BOLT_ASSERT_FALSE(debugger->has_breakpoint(0));
}

BOLT_TEST(Debugger, VM_BreakpointIntegration) {
    auto vm = std::make_unique<drawkern::DISVM>();
    
    // Create and load a simple program first
    auto program = DISProgramFactory::create_echo_server();
    BOLT_ASSERT_TRUE(vm->load_program(program));
    
    // Test VM breakpoint functionality (PC 0 should be valid)
    BOLT_ASSERT_TRUE(vm->set_breakpoint(0));
    BOLT_ASSERT_TRUE(vm->has_breakpoint(0));
    
    auto breakpoints = vm->get_breakpoints();
    BOLT_ASSERT_EQ(1, breakpoints.size());
    BOLT_ASSERT_EQ(0, breakpoints[0]);
    
    // Test removing breakpoints
    BOLT_ASSERT_TRUE(vm->remove_breakpoint(0));
    BOLT_ASSERT_FALSE(vm->has_breakpoint(0));
    BOLT_ASSERT_EQ(0, vm->get_breakpoints().size());
}

BOLT_TEST(Debugger, VM_CallStackInspection) {
    auto vm = std::make_unique<drawkern::DISVM>();
    
    // Initial state should have empty call stack
    BOLT_ASSERT_EQ(0, vm->get_call_stack_depth());
    auto stack = vm->get_call_stack();
    BOLT_ASSERT_EQ(0, stack.size());
}

BOLT_TEST(Debugger, DebuggerInterface_EventCallback) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    bool callbackCalled = false;
    DebugEvent lastEvent;
    std::string lastMessage;
    
    // Set up event callback
    debugger->set_event_callback([&](DebugEvent event, const std::string& message) {
        callbackCalled = true;
        lastEvent = event;
        lastMessage = message;
    });
    
    // Start a debug session to trigger events
    std::string limboSource = "print \"Test\" halt";
    debugger->start_debug_session_from_source(limboSource);
    
    // Check that callback was called
    BOLT_ASSERT_TRUE(callbackCalled);
    BOLT_ASSERT_EQ(static_cast<int>(DebugEvent::SESSION_STARTED), static_cast<int>(lastEvent));
}

BOLT_TEST(Debugger, DebugOutput_Logging) {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Enable debug output
    debugger->enable_debug_output(true);
    
    // Start a debug session to generate log entries
    std::string limboSource = "print \"test\" halt";
    BOLT_ASSERT_TRUE(debugger->start_debug_session_from_source(limboSource));
    
    // Perform some operations that should generate log entries
    debugger->set_breakpoint(0);
    debugger->remove_breakpoint(0);
    
    auto log = debugger->get_debug_log();
    BOLT_ASSERT_TRUE(log.size() > 0);
    
    // Clear log
    debugger->clear_debug_log();
    BOLT_ASSERT_EQ(0, debugger->get_debug_log().size());
    
    // Disable debug output
    debugger->enable_debug_output(false);
    debugger->set_breakpoint(1);
    
    // Should not have added to log (debug output disabled)
    BOLT_ASSERT_EQ(0, debugger->get_debug_log().size());
}