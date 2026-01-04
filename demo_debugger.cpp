#include "bolt/editor/debugger_interface.hpp"
#include "bolt/editor/debugger_ui.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include "bolt/drawkern/dis_vm.hpp"
#include <iostream>
#include <memory>

using namespace bolt;
using namespace bolt::drawkern;

void print_header() {
    std::cout << "\n=====================================" << std::endl;
    std::cout << "  Bolt C++ Integrated Debugger Demo" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;
}

void demo_basic_debugger() {
    std::cout << "📱 Demo: Basic Debugger Functionality" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Set up event callback
    debugger->set_event_callback([](DebugEvent event, const std::string& message) {
        std::cout << "🔔 Event: " << message << std::endl;
    });
    
    // Create a simple Limbo program
    std::string program = R"(
        print "Starting AI workbench..."
        ai_chat "Hello AI"
        render_glyph "debug-symbol"
        print "Program completed"
        halt
    )";
    
    std::cout << "📝 Starting debug session with program:" << std::endl;
    std::cout << program << std::endl;
    
    // Start debug session
    if (debugger->start_debug_session_from_source(program)) {
        std::cout << "✅ Debug session started successfully" << std::endl;
        
        // Set some breakpoints
        std::cout << "\n🔴 Setting breakpoints..." << std::endl;
        debugger->set_breakpoint(2);
        debugger->set_breakpoint(4);
        
        auto breakpoints = debugger->get_all_breakpoints();
        std::cout << "📍 Active breakpoints: " << breakpoints.size() << std::endl;
        for (const auto& bp : breakpoints) {
            std::cout << "  - PC " << bp.pc << (bp.enabled ? " (enabled)" : " (disabled)") << std::endl;
        }
        
        // Step through execution
        std::cout << "\n🔧 Stepping through execution..." << std::endl;
        std::cout << "Current PC: " << debugger->get_current_pc() << std::endl;
        std::cout << "Current instruction: " << debugger->get_current_instruction() << std::endl;
        
        debugger->step_over();
        std::cout << "After step - PC: " << debugger->get_current_pc() << std::endl;
        
        debugger->step_over();
        std::cout << "After step - PC: " << debugger->get_current_pc() << std::endl;
        
        // Continue execution
        std::cout << "\n▶️ Continuing execution..." << std::endl;
        debugger->continue_execution();
        
        std::cout << "Final state: " << static_cast<int>(debugger->get_debug_state()) << std::endl;
        
        // Stop session
        debugger->stop_debug_session();
        std::cout << "🛑 Debug session stopped" << std::endl;
    } else {
        std::cout << "❌ Failed to start debug session" << std::endl;
    }
}

void demo_debugger_ui() {
    std::cout << "\n📱 Demo: Debugger UI Integration" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    auto debugger = std::make_shared<DebuggerInterface>();
    auto ui = std::make_unique<DebuggerUI>();
    
    // Connect UI to debugger
    ui->set_debugger(debugger);
    ui->set_visible(true);
    
    // Create a program for debugging
    std::string program = "print \"UI Demo\" ai_chat \"Testing UI\" halt";
    
    if (debugger->start_debug_session_from_source(program)) {
        std::cout << "✅ UI Debug session started" << std::endl;
        
        // Set breakpoint via UI
        ui->handle_breakpoint_toggle(1);
        std::cout << "🔴 Breakpoint set via UI" << std::endl;
        
        // Render UI state
        std::cout << "\n📺 Rendering debugger UI:" << std::endl;
        ui->render();
        
        // Test UI controls
        std::cout << "\n🎮 Testing UI controls..." << std::endl;
        ui->handle_step_over_clicked();
        ui->render_controls();
        
        ui->handle_continue_clicked();
        ui->render_controls();
        
        ui->handle_stop_clicked();
        std::cout << "🛑 Session stopped via UI" << std::endl;
    }
    
    ui->set_visible(false);
}

void demo_integrated_editor() {
    std::cout << "\n📱 Demo: IntegratedEditor Debugger Integration" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
    
    auto& editor = IntegratedEditor::getInstance();
    
    // Create a test Limbo file content
    std::string testContent = R"(print "Editor integration test"
ai_init "test-model"
ai_chat "Hello from editor"
render_glyph "editor-test"
halt)";
    
    std::cout << "📝 Opening test document in editor..." << std::endl;
    editor.openDocument("/tmp/test_debug.limbo", testContent);
    
    // Start debugging from editor
    std::cout << "🔧 Starting debug session from editor..." << std::endl;
    if (editor.startDebugSessionFromSource(testContent)) {
        std::cout << "✅ Editor debug session started" << std::endl;
        
        // Set breakpoints via editor
        editor.setBreakpointAtLine("/tmp/test_debug.limbo", 2);
        editor.setBreakpointAtLine("/tmp/test_debug.limbo", 4);
        
        auto breakpoints = editor.getAllBreakpoints();
        std::cout << "📍 Breakpoints set via editor: " << breakpoints.size() << std::endl;
        
        // Show debugger UI
        editor.showDebugger();
        std::cout << "📺 Debugger UI shown: " << (editor.isDebuggerVisible() ? "Yes" : "No") << std::endl;
        
        // Step through via editor
        std::cout << "🔧 Stepping through via editor..." << std::endl;
        std::cout << "PC: " << editor.getCurrentDebugPC() << std::endl;
        
        editor.debugStepOver();
        std::cout << "After step - PC: " << editor.getCurrentDebugPC() << std::endl;
        
        editor.debugContinue();
        std::cout << "Debug state: " << static_cast<int>(editor.getDebugState()) << std::endl;
        
        // Clean up
        editor.stopDebugSession();
        editor.hideDebugger();
        std::cout << "🛑 Editor debug session stopped" << std::endl;
    } else {
        std::cout << "❌ Failed to start editor debug session" << std::endl;
    }
}

void demo_watch_expressions() {
    std::cout << "\n📱 Demo: Watch Expressions" << std::endl;
    std::cout << "---------------------------" << std::endl;
    
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Add watch expressions
    std::cout << "👁️ Adding watch expressions..." << std::endl;
    debugger->add_watch_expression("counter");
    debugger->add_watch_expression("message.length");
    debugger->add_watch_expression("ai_model_status");
    
    auto watches = debugger->get_watch_expressions();
    std::cout << "📊 Watch expressions: " << watches.size() << std::endl;
    for (const auto& watch : watches) {
        std::cout << "  - " << watch.expression << " = " << watch.value 
                  << " (" << (watch.valid ? "valid" : "invalid") << ")" << std::endl;
    }
    
    // Update watch expressions (would happen during debugging)
    debugger->update_watch_expressions();
    
    // Remove a watch expression
    debugger->remove_watch_expression("message.length");
    std::cout << "Removed 'message.length' watch" << std::endl;
    
    watches = debugger->get_watch_expressions();
    std::cout << "📊 Remaining watch expressions: " << watches.size() << std::endl;
    
    // Clear all
    debugger->clear_watch_expressions();
    std::cout << "🧹 Cleared all watch expressions" << std::endl;
}

void demo_vm_integration() {
    std::cout << "\n📱 Demo: DIS VM Direct Integration" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    auto vm = std::make_unique<DISVM>();
    
    // Create a simple program using the factory
    auto program = DISProgramFactory::create_ai_workbench("test-model");
    
    std::cout << "📝 Loading DIS program..." << std::endl;
    if (vm->load_program(program)) {
        std::cout << "✅ Program loaded successfully" << std::endl;
        
        // Set breakpoints directly on VM
        vm->set_breakpoint(2);
        vm->set_breakpoint(4);
        
        auto breakpoints = vm->get_breakpoints();
        std::cout << "📍 VM breakpoints: " << breakpoints.size() << std::endl;
        
        // Step through execution
        std::cout << "🔧 VM stepping:" << std::endl;
        std::cout << "PC: " << vm->get_pc() << std::endl;
        
        vm->step_into();
        std::cout << "After step - PC: " << vm->get_pc() << std::endl;
        
        // Check if at breakpoint
        if (vm->is_at_breakpoint()) {
            std::cout << "🔴 Hit breakpoint!" << std::endl;
        }
        
        // Continue until breakpoint or completion
        vm->continue_execution();
        
        std::cout << "VM running: " << (vm->is_running() ? "Yes" : "No") << std::endl;
        std::cout << "Call stack depth: " << vm->get_call_stack_depth() << std::endl;
        
        vm->halt();
        std::cout << "🛑 VM halted" << std::endl;
    } else {
        std::cout << "❌ Failed to load program" << std::endl;
    }
}

int main() {
    print_header();
    
    try {
        // Run all demos
        demo_basic_debugger();
        demo_debugger_ui();
        demo_integrated_editor();
        demo_watch_expressions();
        demo_vm_integration();
        
        std::cout << "\n🎉 All debugger demos completed successfully!" << std::endl;
        std::cout << std::endl;
        std::cout << "The integrated debugger interface provides:" << std::endl;
        std::cout << "✅ Breakpoint management (set, remove, toggle, enable/disable)" << std::endl;
        std::cout << "✅ Step debugging (step over, step into, step out, continue)" << std::endl;
        std::cout << "✅ Watch expressions for variable monitoring" << std::endl;
        std::cout << "✅ Call stack inspection" << std::endl;
        std::cout << "✅ Debug event notifications" << std::endl;
        std::cout << "✅ UI integration with the editor" << std::endl;
        std::cout << "✅ Direct integration with DIS Virtual Machine" << std::endl;
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}