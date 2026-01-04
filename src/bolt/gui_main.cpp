// gui_main.cpp - Main entry point for Bolt GUI application with AI chat
#include <iostream>
#include "../../include/bolt/gui/bolt_gui_app.hpp"

using namespace bolt::gui;

int main() {
    std::cout << "🚀 Starting Bolt C++ IDE with AI Chat..." << std::endl;
    
    try {
        BoltGuiApp app;
        
        if (!app.Initialize()) {
            std::cerr << "❌ Failed to initialize Bolt GUI application" << std::endl;
            return 1;
        }
        
        std::cout << "✅ Bolt GUI initialized successfully!" << std::endl;
        std::cout << "💬 AI Chat ready - type messages and press Enter!" << std::endl;
        std::cout << "⚙️ Use F1 or menu for AI Settings panel" << std::endl;
        
        app.Run();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "👋 Bolt GUI application closed" << std::endl;
    return 0;
}
