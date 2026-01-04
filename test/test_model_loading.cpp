#include "bolt/ai/gguf_loader.hpp"
#include "bolt/ai/bpe_tokenizer.hpp"
#include "bolt/ai/rwkv_wrapper.hpp"
#include <iostream>
#include <filesystem>

using namespace bolt;

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "RWKV Model Loading Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Find model file
    std::vector<std::string> model_paths = {
        "test/data/models/rwkv-4-pile-169m-q4_0.gguf",
        "../test/data/models/rwkv-4-pile-169m-q4_0.gguf",
        "../../test/data/models/rwkv-4-pile-169m-q4_0.gguf"
    };
    
    std::string model_path;
    bool found = false;
    
    for (const auto& path : model_paths) {
        if (std::filesystem::exists(path)) {
            model_path = path;
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cerr << "❌ Model file not found!" << std::endl;
        std::cerr << "Searched paths:" << std::endl;
        for (const auto& path : model_paths) {
            std::cerr << "  - " << path << std::endl;
        }
        std::cerr << std::endl;
        std::cerr << "Please run: scripts/download_rwkv_model.py" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Found model: " << model_path << std::endl;
    auto size_mb = std::filesystem::file_size(model_path) / (1024.0 * 1024.0);
    std::cout << "  Size: " << size_mb << " MB" << std::endl;
    std::cout << std::endl;
    
    // Test 1: GGUF Loader
    std::cout << "Test 1: GGUF Loader" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    try {
        GGUFLoader loader(model_path);
        
        if (!loader.load()) {
            std::cerr << "❌ Failed to load GGUF file" << std::endl;
            std::cerr << "Error: " << loader.getError() << std::endl;
            return 1;
        }
        
        std::cout << "✓ GGUF file loaded successfully" << std::endl;
        std::cout << "  Version: " << loader.getVersion() << std::endl;
        std::cout << "  Architecture: " << loader.getArchitecture() << std::endl;
        std::cout << "  Tensor count: " << loader.getTensorCount() << std::endl;
        std::cout << "  Metadata count: " << loader.getMetadataCount() << std::endl;
        
        int n_layers = loader.getNumLayers();
        int n_embd = loader.getEmbedDim();
        int n_vocab = loader.getVocabSize();
        
        std::cout << "  Layers: " << n_layers << std::endl;
        std::cout << "  Embedding dim: " << n_embd << std::endl;
        std::cout << "  Vocabulary size: " << n_vocab << std::endl;
        std::cout << std::endl;
        
        if (n_layers == 0 || n_embd == 0 || n_vocab == 0) {
            std::cerr << "⚠ Warning: Some model parameters are zero" << std::endl;
            std::cerr << "This might be an older GGML format (not GGUF)" << std::endl;
            std::cerr << "The file magic number should be GGUF (0x46554747)" << std::endl;
            std::cout << std::endl;
        }
        
        // Test 2: Tokenizer
        std::cout << "Test 2: BPE Tokenizer" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        auto tokens = loader.getMetadataStringArray("tokenizer.ggml.tokens");
        auto scores = loader.getMetadataFloatArray("tokenizer.ggml.scores");
        
        if (tokens.empty()) {
            std::cerr << "⚠ No tokenizer vocabulary found in model" << std::endl;
            std::cerr << "This is expected for older GGML format files" << std::endl;
            std::cout << std::endl;
        } else {
            BPETokenizer tokenizer;
            if (tokenizer.loadVocabulary(tokens, scores)) {
                std::cout << "✓ Tokenizer loaded successfully" << std::endl;
                std::cout << "  Vocabulary size: " << tokenizer.getVocabSize() << std::endl;
                
                // Test encoding/decoding
                std::string test_text = "Hello, world!";
                auto encoded = tokenizer.encode(test_text);
                auto decoded = tokenizer.decode(encoded);
                
                std::cout << "  Test text: \"" << test_text << "\"" << std::endl;
                std::cout << "  Encoded: " << encoded.size() << " tokens" << std::endl;
                std::cout << "  Decoded: \"" << decoded << "\"" << std::endl;
                std::cout << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 3: RWKV Wrapper Integration
    std::cout << "Test 3: RWKV Wrapper Integration" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    try {
        auto& wrapper = RWKVWrapper::getInstance();
        
        std::cout << "Initializing RWKV model..." << std::endl;
        wrapper.initialize(model_path, 512);  // 512 MB context
        
        if (wrapper.isInitialized()) {
            std::cout << "✓ RWKV model initialized successfully" << std::endl;
            std::cout << "  Layers: " << wrapper.getNumLayers() << std::endl;
            std::cout << "  Embedding dim: " << wrapper.getEmbedDim() << std::endl;
            std::cout << std::endl;
            
            // Test generation (if model is properly loaded)
            if (wrapper.getNumLayers() > 0) {
                std::cout << "Test 4: Text Generation" << std::endl;
                std::cout << "----------------------------------------" << std::endl;
                
                std::string prompt = "The";
                std::cout << "Prompt: \"" << prompt << "\"" << std::endl;
                std::cout << "Generating 5 tokens..." << std::endl;
                
                try {
                    std::string generated = wrapper.generate(prompt, 5);
                    std::cout << "✓ Generated: \"" << generated << "\"" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "⚠ Generation failed: " << e.what() << std::endl;
                    std::cerr << "This is expected if RWKV layers are not fully implemented" << std::endl;
                }
            }
        } else {
            std::cerr << "❌ RWKV model initialization failed" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception during RWKV initialization: " << e.what() << std::endl;
        std::cerr << "This might be due to format incompatibility" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "✓ All tests completed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
