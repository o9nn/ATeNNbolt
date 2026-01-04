#pragma once

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#endif

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

// Forward declaration
namespace bolt { namespace ai { class EnhancedAIManager; } }

namespace bolt {
namespace gui {

#ifdef BOLT_HAVE_IMGUI

struct ChatMessage {
    std::string author;
    std::string content;
    bool is_user;
    
    ChatMessage(const std::string& auth, const std::string& msg, bool user) 
        : author(auth), content(msg), is_user(user) {}
};

class BoltGuiApp {
public:
    BoltGuiApp();
    ~BoltGuiApp();
    
    bool Initialize();
    void Run();
    void Shutdown();
    
private:
    // Window management
    GLFWwindow* window_;
    int window_width_ = 1400;
    int window_height_ = 900;
    
    // AI Management
    std::unique_ptr<bolt::ai::EnhancedAIManager> ai_manager_;
    bool ai_ready_ = false;
    std::mutex chat_mutex_;
    std::vector<ChatMessage> pending_messages_;
    
    // UI State
    char chat_input_buffer_[1024] = "";
    char code_buffer_[8192] = "// Welcome to Bolt AI IDE!\n// Start typing your code here...\n\n";
    std::vector<ChatMessage> chat_history_;
    std::vector<std::string> file_tree_;
    std::vector<std::string> console_log_;
    int selected_file_index_ = -1;
    std::string current_file_path_;
    bool show_demo_window_ = false;
    bool show_file_tree_ = true;
    bool show_chat_panel_ = true;
    bool show_code_editor_ = true;
    bool show_ai_completion_ = false;
    bool show_ai_settings_ = false;
    bool show_theme_settings_ = false;
    bool show_console_window_ = false;
    bool show_about_window_ = false;
    
    // Theme Management
    enum class ThemeMode {
        Dark,
        Light,
        HighContrast,
        Custom
    };
    ThemeMode current_theme_ = ThemeMode::Dark;
    
    // UI Colors and Style
    void SetupCustomStyle();
    void ApplyDarkTheme();
    void ApplyLightTheme();
    void ApplyHighContrastTheme();
    void SetTheme(ThemeMode theme);
    
    // UI Panels
    void RenderMainMenuBar();
    void RenderFileTree();
    void RenderChatPanel();
    void RenderCodeEditor();
    void RenderAiCompletionOverlay();
    void RenderStatusBar();
    void RenderAiSettingsWindow();
    void RenderThemeSettingsWindow();
    void RenderConsoleWindow();
    void RenderAboutWindow();
    
    // Chat functionality
    void ProcessChatInput();
    void AddChatMessage(const std::string& author, const std::string& message, bool is_user);
    void AddConsoleLog(const std::string& message);
    std::string GenerateAiResponse(const std::string& input);
    std::string GenerateFallbackResponse(const std::string& input);
    void ProcessPendingMessages();
    
    // File operations
    void InitializeFileTree();
    void OnFileSelected(int index);
    void OpenFileDialog();
    void SaveCurrentFile();
    void SaveFileAsDialog();
    
    // Keyboard shortcuts
    void HandleKeyboardShortcuts();
};

#endif // BOLT_HAVE_IMGUI

} // namespace gui
} // namespace bolt
