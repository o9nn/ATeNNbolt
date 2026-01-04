/**
 * @file demo_lsp_ui_integration.cpp
 * @brief Demo application showcasing LSP UI integration with ImGui
 * 
 * This demo demonstrates:
 * - Code completion popup
 * - Hover tooltips
 * - Diagnostics gutter and inline markers
 * - Status bar with LSP status
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "bolt/editor/lsp_editor_bridge.hpp"
#include "bolt/gui/lsp_ui_components.hpp"

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#endif

using namespace bolt;

void printHeader() {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Bolt C++ LSP UI Integration Demo                     ║\n";
    std::cout << "║     Testing ImGui Components for LSP Features            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
}

#ifdef BOLT_HAVE_IMGUI

class LspUiDemo {
public:
    LspUiDemo() 
        : lspBridge_(std::make_unique<lsp::LspEditorBridge>())
        , completionPopup_(std::make_unique<gui::LspCompletionPopup>())
        , hoverTooltip_(std::make_unique<gui::LspHoverTooltip>())
        , diagnosticsGutter_(std::make_unique<gui::LspDiagnosticsGutter>())
        , inlineDiagnostics_(std::make_unique<gui::LspInlineDiagnostics>())
        , statusBar_(std::make_unique<gui::LspStatusBar>()) {
    }
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW\n";
            return false;
        }
        
        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window_ = glfwCreateWindow(1280, 720, "LSP UI Integration Demo", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create window\n";
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);
        
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        ImGui::StyleColorsDark();
        
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Initialize LSP bridge
        std::cout << "\n--- Initializing LSP Bridge ---\n";
        if (!lspBridge_->initialize()) {
            std::cout << "Warning: No language servers available\n";
        }
        
        // Set up completion callback
        completionPopup_->setAcceptCallback([this](const std::string& text) {
            std::cout << "Completion accepted: " << text << "\n";
            insertTextAtCursor(text);
        });
        
        // Create test file
        createTestFile();
        
        return true;
    }
    
    void run() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Update LSP bridge
            lspBridge_->update();
            
            // Render UI
            renderMainWindow();
            renderEditor();
            renderStatusBar();
            
            // Render LSP UI components
            renderLspComponents();
            
            ImGui::Render();
            
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window_);
        }
    }
    
    void shutdown() {
        lspBridge_->shutdown();
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
    
private:
    void createTestFile() {
        testFilePath_ = "/tmp/lsp_ui_demo_test.cpp";
        testContent_ = R"(#include <iostream>
#include <vector>
#include <string>

class MyClass {
public:
    void doSomething() {
        std::cout << "Hello, World!" << std::endl;
    }
    
    int getValue() const {
        return value_;
    }
    
private:
    int value_ = 42;
};

int main() {
    MyClass obj;
    obj.doSomething();
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    for (const auto& n : numbers) {
        std::cout << n << " ";
    }
    
    return 0;
}
)";
        
        std::ofstream file(testFilePath_);
        file << testContent_;
        file.close();
        
        // Open document in LSP
        lspBridge_->documentOpened(testFilePath_, testContent_, "cpp");
        
        std::cout << "Created test file: " << testFilePath_ << "\n";
    }
    
    void renderMainWindow() {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("LSP UI Demo Controls");
        
        ImGui::Text("LSP UI Integration Demo");
        ImGui::Separator();
        
        if (ImGui::Button("Request Completion")) {
            requestCompletion();
        }
        
        if (ImGui::Button("Request Hover")) {
            requestHover();
        }
        
        ImGui::Separator();
        
        ImGui::Text("Cursor Position:");
        ImGui::Text("  Line: %zu, Column: %zu", cursorLine_, cursorColumn_);
        
        ImGui::Separator();
        
        auto& completionState = lspBridge_->getCompletionState();
        ImGui::Text("Completion: %s", completionState.isVisible ? "Visible" : "Hidden");
        ImGui::Text("  Items: %zu", completionState.items.size());
        
        auto& hoverState = lspBridge_->getHoverState();
        ImGui::Text("Hover: %s", hoverState.isVisible ? "Visible" : "Hidden");
        
        ImGui::End();
    }
    
    void renderEditor() {
        ImGui::SetNextWindowPos(ImVec2(320, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
        
        ImGui::Begin("Code Editor");
        
        // Store editor position for LSP UI positioning
        editorPos_ = ImGui::GetCursorScreenPos();
        
        // Simple text editor
        ImGui::InputTextMultiline("##editor", &testContent_[0], testContent_.capacity() + 1,
                                  ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
        
        // Track cursor position (simplified)
        if (ImGui::IsItemActive()) {
            // In a real implementation, we'd track the actual cursor position
            // For demo, we'll use a fixed position
        }
        
        ImGui::End();
    }
    
    void renderStatusBar() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
        
        ImGui::Begin("##StatusBar", nullptr, flags);
        
        ImGui::Text("LSP UI Demo");
        ImGui::SameLine(200);
        
        // Render LSP status
        statusBar_->render(*lspBridge_, testFilePath_);
        
        ImGui::End();
    }
    
    void renderLspComponents() {
        // Render completion popup
        auto& completionState = lspBridge_->getCompletionState();
        if (completionState.isVisible) {
            float cursorX = editorPos_.x + cursorColumn_ * 8.0f;  // Approximate char width
            float cursorY = editorPos_.y + cursorLine_ * 16.0f;  // Approximate line height
            completionPopup_->render(completionState, cursorX, cursorY, 16.0f);
            completionPopup_->handleInput(completionState);
        }
        
        // Render hover tooltip
        auto& hoverState = lspBridge_->getHoverState();
        if (hoverState.isVisible) {
            ImVec2 mousePos = ImGui::GetMousePos();
            hoverTooltip_->render(hoverState, mousePos.x, mousePos.y);
        }
        
        // Render diagnostics (would be integrated into editor in real implementation)
        auto diagnostics = lspBridge_->getDiagnostics(testFilePath_);
        if (!diagnostics.empty()) {
            // In a real implementation, these would be rendered in the editor gutter
        }
    }
    
    void requestCompletion() {
        std::cout << "Requesting completion at line " << cursorLine_ << ", column " << cursorColumn_ << "\n";
        lspBridge_->requestCompletion(testFilePath_, cursorLine_, cursorColumn_);
        
        // Wait a bit for response
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void requestHover() {
        std::cout << "Requesting hover at line " << cursorLine_ << ", column " << cursorColumn_ << "\n";
        lspBridge_->requestHover(testFilePath_, cursorLine_, cursorColumn_);
        
        // Wait a bit for response
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void insertTextAtCursor(const std::string& text) {
        // In a real implementation, this would insert text at the cursor position
        std::cout << "Would insert: " << text << "\n";
    }
    
    GLFWwindow* window_ = nullptr;
    
    std::unique_ptr<lsp::LspEditorBridge> lspBridge_;
    std::unique_ptr<gui::LspCompletionPopup> completionPopup_;
    std::unique_ptr<gui::LspHoverTooltip> hoverTooltip_;
    std::unique_ptr<gui::LspDiagnosticsGutter> diagnosticsGutter_;
    std::unique_ptr<gui::LspInlineDiagnostics> inlineDiagnostics_;
    std::unique_ptr<gui::LspStatusBar> statusBar_;
    
    std::string testFilePath_;
    std::string testContent_;
    
    size_t cursorLine_ = 7;  // Position at "std::cout" line
    size_t cursorColumn_ = 12;
    
    ImVec2 editorPos_;
};

int main() {
    printHeader();
    
    LspUiDemo demo;
    
    if (!demo.initialize()) {
        std::cerr << "Failed to initialize demo\n";
        return 1;
    }
    
    std::cout << "\n--- Running LSP UI Demo ---\n";
    std::cout << "Press 'Request Completion' to test code completion\n";
    std::cout << "Press 'Request Hover' to test hover information\n";
    std::cout << "Close the window to exit\n\n";
    
    demo.run();
    demo.shutdown();
    
    std::cout << "\n--- Demo Complete ---\n";
    return 0;
}

#else // BOLT_HAVE_IMGUI

int main() {
    printHeader();
    
    std::cout << "\n❌ ImGui not available - cannot run UI demo\n";
    std::cout << "   Please build with ImGui support to test UI components\n";
    
    // Test LSP bridge without UI
    std::cout << "\n--- Testing LSP Bridge (non-UI) ---\n";
    
    lsp::LspEditorBridge bridge;
    if (bridge.initialize()) {
        std::cout << "✅ LSP Bridge initialized\n";
        
        // Test language detection
        std::cout << "\nLanguage Detection:\n";
        std::cout << "  test.cpp -> " << lsp::LspEditorBridge::detectLanguageId("test.cpp") << "\n";
        std::cout << "  test.py -> " << lsp::LspEditorBridge::detectLanguageId("test.py") << "\n";
        std::cout << "  test.rs -> " << lsp::LspEditorBridge::detectLanguageId("test.rs") << "\n";
        
        // Test language support
        std::cout << "\nLanguage Support:\n";
        std::cout << "  cpp: " << (bridge.isLanguageSupported("cpp") ? "✅" : "❌") << "\n";
        std::cout << "  python: " << (bridge.isLanguageSupported("python") ? "✅" : "❌") << "\n";
        
        bridge.shutdown();
    } else {
        std::cout << "❌ Failed to initialize LSP Bridge\n";
    }
    
    return 0;
}

#endif // BOLT_HAVE_IMGUI
