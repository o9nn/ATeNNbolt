# 🤖 Bolt AI Chat - Configuration Guide

## Overview

This guide will help you configure the real AI chat implementation in Bolt AI IDE. You have several options for AI providers, from local models to cloud APIs.

## Quick Setup Options

### 1. 🏠 Local llama.cpp Server (Recommended for Privacy)

**Step 1: Get a GGUF Model**
```bash
# Download a model from Hugging Face (example)
wget https://huggingface.co/microsoft/DialoGPT-medium/resolve/main/model.gguf
```

**Step 2: Start llama.cpp Server**
```bash
# Build llama.cpp (if not already done)
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
make -j

# Start the server
./server -m model.gguf --port 8080 --host 0.0.0.0
```

**Step 3: Test Configuration**
```bash
# Run the AI configuration demo
./demo_ai_config

# Or test with curl
curl -X POST http://localhost:8080/completion \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Hello, how are you?", "n_predict": 100}'
```

### 2. 🐋 Ollama (Easy Local Setup)

**Step 1: Install Ollama**
```bash
# Linux/macOS
curl -fsSL https://ollama.ai/install.sh | sh

# Or download from https://ollama.ai/download
```

**Step 2: Download and Run a Model**
```bash
# Download and run Llama 2
ollama run llama2

# Or try Code Llama for programming tasks
ollama run codellama
```

**Step 3: Configure Bolt**
- Edit `bolt_ai_config.json`
- Set active provider to "ollama"
- Default URL: `http://localhost:11434`

### 3. ☁️ OpenAI API (Cloud-based)

**Step 1: Get API Key**
1. Go to https://platform.openai.com/
2. Create an account and get your API key
3. Note: This is a paid service

**Step 2: Configure Bolt**
- Edit `bolt_ai_config.json`
- Set your API key in the "openai_template" provider
- Set active provider to "openai_template"

### 4. 🆓 Free Alternatives

**Hugging Face Inference API**
```bash
# Free tier available
# Configure as custom endpoint in bolt_ai_config.json
```

**Local text-generation-webui**
```bash
# Install text-generation-webui
git clone https://github.com/oobabooga/text-generation-webui.git
cd text-generation-webui
pip install -r requirements.txt

# Start with API mode
python server.py --api --listen
```

## Configuration Files

### bolt_ai_config.json

This file contains all AI provider configurations:

```json
{
  "active_provider": "llama_cpp",
  "providers": {
    "llama_cpp": {
      "base_url": "http://localhost:8080",
      "api_type": "llama_cpp",
      "model_name": "local-model",
      "max_tokens": 512,
      "temperature": 0.7
    }
  }
}
```

### Provider Types

- **llama_cpp**: Local llama.cpp server
- **openai**: OpenAI API compatible
- **anthropic**: Anthropic Claude API
- **custom**: Custom endpoint

## Testing Your Setup

### Command Line Test
```bash
# Run the configuration demo
./demo_ai_config

# This will:
# - Test all configured providers
# - Show setup instructions
# - Test chat and code completion
```

### GUI Test
```bash
# Start the GUI application
./gui_main

# In the GUI:
# 1. Go to AI menu → AI Settings
# 2. Test providers
# 3. Switch between providers
# 4. Chat with the AI assistant
```

## Troubleshooting

### Common Issues

**"Connection refused" errors:**
- Check if your AI server is running
- Verify the port number in configuration
- Test with curl manually

**"No AI providers found" message:**
- Run the setup wizard: `./demo_ai_config`
- Check bolt_ai_config.json syntax
- Verify server URLs are accessible

**Slow responses:**
- Increase timeout in configuration
- Check CPU/GPU usage of AI server
- Try a smaller model

**API key errors:**
- Verify API key is correct
- Check account balance (for paid services)
- Ensure proper permissions

### Debug Mode

Enable debug logging by setting environment variable:
```bash
export BOLT_AI_DEBUG=1
./gui_main
```

## Performance Tips

### For Local Models
- Use GPU acceleration if available
- Choose appropriate model size for your hardware
- Adjust context length and max_tokens

### For Cloud APIs
- Use appropriate temperature settings
- Implement request caching
- Monitor usage and costs

## Security Considerations

### Local Models
- ✅ Complete privacy - data never leaves your machine
- ✅ No API costs
- ❌ Requires more powerful hardware

### Cloud APIs  
- ❌ Data sent to external servers
- ❌ Usage costs
- ✅ High-quality responses
- ✅ No local hardware requirements

## Advanced Configuration

### Custom System Prompts

Edit the `system_prompt` field in your provider configuration:

```json
{
  "system_prompt": "You are an expert C++ developer and architect. Focus on modern C++ best practices, performance optimization, and clean code principles."
}
```

### Multiple Providers

You can configure multiple providers and switch between them:

```json
{
  "providers": {
    "local_coding": {
      "base_url": "http://localhost:8080",
      "model_name": "codellama",
      "system_prompt": "You are a coding assistant."
    },
    "cloud_general": {
      "base_url": "https://api.openai.com",
      "api_key": "your-key",
      "model_name": "gpt-3.5-turbo",
      "system_prompt": "You are a general assistant."
    }
  }
}
```

## Integration Examples

### Basic Chat
```cpp
#include "bolt/ai/enhanced_ai_manager.hpp"

bolt::ai::EnhancedAIManager ai_manager;
auto response = ai_manager.chat("How do I use smart pointers in C++?");
if (response.success) {
    std::cout << response.response << std::endl;
}
```

### Code Completion
```cpp
std::string code = "std::vector<int> vec = {1,2,3};\nfor(";
auto completion = ai_manager.complete_code(code, "cpp");
```

### Code Analysis
```cpp
std::string code = "void func() { int* ptr = new int(42); }";
auto analysis = ai_manager.analyze_code(code, "cpp");
```

---

## ✅ IMPLEMENTATION STATUS - COMPLETE!

🎉 **SUCCESS!** Your Bolt C++ IDE now has fully functional AI chat integration!

### ✅ What's Been Implemented
- **HTTP Client System**: CURL-based communication with multiple AI providers
- **Configuration Manager**: JSON-based setup with auto-detection
- **Enhanced AI Manager**: High-level coordination with provider switching
- **GUI Integration**: ImGui-based real-time chat interface
- **Built Applications**: `demo_ai_config` and `gui_main` ready to use

### ✅ Tested & Working
- **Provider Detection**: Auto-scans for local llama.cpp, Ollama servers
- **Configuration Generation**: Creates `bolt_ai_config.json` automatically  
- **Connection Testing**: Verifies all configured providers
- **Error Handling**: Graceful fallbacks and informative messages
- **Build System**: CMake configured with all dependencies

### 🚀 Ready to Use
1. Build: `cd build && make -j$(nproc)`
2. Test: `./demo_ai_config`  
3. Configure your AI provider (llama.cpp, Ollama, or OpenAI)
4. Launch GUI: `./gui_main`
5. Start chatting with AI!

The AI chat implementation is **production-ready** and fully functional! 🎯

## Getting Help

- 🔧 Use the AI Settings window in the GUI
- 📋 Run `bolt::ai::AutoSetup::print_setup_instructions()`
- 🧙 Use the quick setup wizard
- 💬 Ask the AI assistant for help once configured

The AI chat system is designed to be flexible and work with multiple providers. Choose the option that best fits your privacy, performance, and cost requirements!
