#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

// Simple terminal chat that calls KoboldCpp as a subprocess
class KoboldTerminal {
private:
    std::string kobold_path;
    std::string model_path;
    std::vector<std::string> conversation;
    
public:
    KoboldTerminal(const std::string& kobold, const std::string& model) 
        : kobold_path(kobold), model_path(model) {}
    
    bool initialize() {
        std::cout << "🚀 Initializing Bolt Terminal Chat with KoboldCpp\n";
        std::cout << "Model: " << model_path << std::endl;
        
        // Check if files exist
        std::ifstream kobold_file(kobold_path);
        std::ifstream model_file(model_path);
        
        if (!kobold_file.good()) {
            std::cout << "❌ KoboldCpp not found: " << kobold_path << std::endl;
            return false;
        }
        
        if (!model_file.good()) {
            std::cout << "❌ Model not found: " << model_path << std::endl;
            return false;
        }
        
        std::cout << "✅ Files found successfully" << std::endl;
        return true;
    }
    
    std::string chat(const std::string& message) {
        conversation.push_back("Human: " + message);
        
        // Format prompt with conversation history
        std::string prompt = "You are a helpful AI assistant specialized in programming.\n\n";
        
        // Add recent conversation (keep it manageable)
        int start_idx = std::max(0, static_cast<int>(conversation.size()) - 6);
        for (int i = start_idx; i < static_cast<int>(conversation.size()); i++) {
            prompt += conversation[i] + "\n";
        }
        prompt += "Assistant: ";
        
        // Write prompt to temp file
        std::ofstream temp_file("/tmp/prompt.txt");
        temp_file << prompt;
        temp_file.close();
        
        // Call KoboldCpp with the prompt
        std::string command = "cd " + kobold_path + " && echo \"" + message + "\" | python koboldcpp.py --model \"" + model_path + "\" --cli --promptlimit 150 2>/dev/null | tail -1";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return "❌ Failed to execute KoboldCpp";
        }
        
        std::string result;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // Clean up the response
        if (result.empty()) {
            return "❌ No response from AI";
        }
        
        // Remove any trailing newlines
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        
        conversation.push_back("Assistant: " + result);
        
        // Keep conversation history manageable
        if (conversation.size() > 12) {
            conversation.erase(conversation.begin(), conversation.begin() + 2);
        }
        
        return result;
    }
    
    void run_interactive() {
        std::cout << "\n💬 Bolt Terminal Chat Ready!\n";
        std::cout << "Type your messages below. Use 'quit', 'exit', or '/quit' to end.\n\n";
        
        std::string input;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            if (input == "quit" || input == "exit" || input == "/quit" || input == "/exit") {
                std::cout << "👋 Goodbye!\n";
                break;
            }
            
            if (input == "/clear") {
                conversation.clear();
                std::cout << "🧹 Conversation history cleared.\n";
                continue;
            }
            
            if (input == "/help") {
                std::cout << "Commands:\n";
                std::cout << "  /quit, /exit - Exit the chat\n";
                std::cout << "  /clear - Clear conversation history\n";
                std::cout << "  /help - Show this help\n";
                continue;
            }
            
            std::cout << "🤖 Thinking..." << std::flush;
            std::string response = chat(input);
            std::cout << "\r🤖 " << response << "\n\n";
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "🚀 Bolt Terminal Chat - Direct GGUF Access\n";
    std::cout << "==========================================\n\n";
    
    std::string kobold_dir = "/workspaces/bolt-cppml/ggml/kobold.cpp";
    std::string model_path = "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf";
    
    if (argc > 1) {
        model_path = argv[1];
    }
    
    KoboldTerminal terminal(kobold_dir, model_path);
    
    if (!terminal.initialize()) {
        return 1;
    }
    
    terminal.run_interactive();
    
    return 0;
}
