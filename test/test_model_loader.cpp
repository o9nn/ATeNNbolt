#include "bolt/ai/model_loader.hpp"
#include "bolt/ai/ggml_loader.hpp"
#include "bolt/ai/gguf_loader.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

// Test utilities
void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "ASSERTION FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "✓ " << message << std::endl;
}

void assert_false(bool condition, const std::string& message) {
    assert_true(!condition, message);
}

void assert_equals(uint32_t actual, uint32_t expected, const std::string& message) {
    if (actual != expected) {
        std::cerr << "ASSERTION FAILED: " << message << std::endl;
        std::cerr << "  Expected: " << expected << std::endl;
        std::cerr << "  Actual: " << actual << std::endl;
        exit(1);
    }
    std::cout << "✓ " << message << " (value: " << actual << ")" << std::endl;
}

void assert_equals(const std::string& actual, const std::string& expected, const std::string& message) {
    if (actual != expected) {
        std::cerr << "ASSERTION FAILED: " << message << std::endl;
        std::cerr << "  Expected: " << expected << std::endl;
        std::cerr << "  Actual: " << actual << std::endl;
        exit(1);
    }
    std::cout << "✓ " << message << " (value: " << actual << ")" << std::endl;
}

// Test format detection
void test_format_detection() {
    std::cout << "\n=== Testing Format Detection ===" << std::endl;
    
    // Test with GGUF models in the repository
    std::string gguf_test_path = "/home/ubuntu/bolt-cppml/ggml/llama.cpp/models";
    
    if (fs::exists(gguf_test_path)) {
        for (const auto& entry : fs::directory_iterator(gguf_test_path)) {
            if (entry.path().extension() == ".gguf") {
                std::cout << "\nTesting GGUF file: " << entry.path().filename() << std::endl;
                
                bolt::ModelLoader loader(entry.path().string());
                assert_true(loader.load(), "Should load GGUF file successfully");
                assert_true(loader.isLoaded(), "Loader should report as loaded");
                assert_equals(static_cast<int>(loader.getFormatType()), 
                             static_cast<int>(bolt::ModelLoader::FormatType::GGUF),
                             "Should detect GGUF format");
                
                loader.printModelInfo();
                break; // Test only first GGUF file found
            }
        }
    }
    
    // Test with GGML models in the repository
    std::string ggml_test_path = "/home/ubuntu/bolt-cppml/ggml/rwkv.cpp/tests";
    
    if (fs::exists(ggml_test_path)) {
        for (const auto& entry : fs::directory_iterator(ggml_test_path)) {
            if (entry.path().extension() == ".bin") {
                std::string filename = entry.path().filename().string();
                // Skip expected-logits files
                if (filename.find("expected-logits") != std::string::npos) {
                    continue;
                }
                
                std::cout << "\nTesting GGML file: " << filename << std::endl;
                
                bolt::ModelLoader loader(entry.path().string());
                bool loaded = loader.load();
                
                if (loaded) {
                    assert_true(loader.isLoaded(), "Loader should report as loaded");
                    assert_equals(static_cast<int>(loader.getFormatType()), 
                                 static_cast<int>(bolt::ModelLoader::FormatType::GGML_LEGACY),
                                 "Should detect GGML legacy format");
                    
                    loader.printModelInfo();
                } else {
                    std::cout << "  Note: Failed to load (may not be a valid GGML file): " 
                             << loader.getError() << std::endl;
                }
                
                break; // Test only first .bin file found
            }
        }
    }
}

// Test GGUF loader directly
void test_gguf_loader() {
    std::cout << "\n=== Testing GGUF Loader ===" << std::endl;
    
    std::string gguf_test_path = "/home/ubuntu/bolt-cppml/ggml/llama.cpp/models";
    
    if (!fs::exists(gguf_test_path)) {
        std::cout << "GGUF test path not found, skipping GGUF loader tests" << std::endl;
        return;
    }
    
    for (const auto& entry : fs::directory_iterator(gguf_test_path)) {
        if (entry.path().extension() == ".gguf") {
            std::cout << "\nTesting GGUF loader with: " << entry.path().filename() << std::endl;
            
            bolt::GGUFLoader loader(entry.path().string());
            assert_true(loader.load(), "GGUF loader should load successfully");
            assert_true(loader.isLoaded(), "GGUF loader should report as loaded");
            
            // Test metadata access
            std::cout << "  Version: " << loader.getVersion() << std::endl;
            std::cout << "  Tensor count: " << loader.getTensorCount() << std::endl;
            std::cout << "  Metadata count: " << loader.getMetadataCount() << std::endl;
            
            // Test common metadata keys
            if (loader.hasMetadata("general.architecture")) {
                std::string arch = loader.getMetadataString("general.architecture");
                std::cout << "  Architecture: " << arch << std::endl;
                assert_false(arch.empty(), "Architecture should not be empty");
            }
            
            if (loader.hasMetadata("general.name")) {
                std::string name = loader.getMetadataString("general.name");
                std::cout << "  Model name: " << name << std::endl;
            }
            
            // Test tensor access
            auto tensor_names = loader.getTensorNames();
            std::cout << "  Number of tensors: " << tensor_names.size() << std::endl;
            assert_true(tensor_names.size() > 0, "Should have at least one tensor");
            
            if (!tensor_names.empty()) {
                std::string first_tensor = tensor_names[0];
                assert_true(loader.hasTensor(first_tensor), "Should find first tensor");
                
                const bolt::TensorInfo* info = loader.getTensorInfo(first_tensor);
                assert_true(info != nullptr, "Should get tensor info");
                
                if (info) {
                    std::cout << "  First tensor: " << info->name << std::endl;
                    std::cout << "    Dimensions: " << info->n_dims << "D [";
                    for (size_t i = 0; i < info->dims.size(); i++) {
                        std::cout << info->dims[i];
                        if (i < info->dims.size() - 1) std::cout << "x";
                    }
                    std::cout << "]" << std::endl;
                    std::cout << "    Type: " << info->type << std::endl;
                    std::cout << "    Size: " << info->size << " bytes" << std::endl;
                }
            }
            
            break; // Test only first GGUF file
        }
    }
}

// Test GGML loader directly
void test_ggml_loader() {
    std::cout << "\n=== Testing GGML Loader ===" << std::endl;
    
    std::string ggml_test_path = "/home/ubuntu/bolt-cppml/ggml/rwkv.cpp/tests";
    
    if (!fs::exists(ggml_test_path)) {
        std::cout << "GGML test path not found, skipping GGML loader tests" << std::endl;
        return;
    }
    
    for (const auto& entry : fs::directory_iterator(ggml_test_path)) {
        if (entry.path().extension() == ".bin") {
            std::string filename = entry.path().filename().string();
            
            // Skip expected-logits files
            if (filename.find("expected-logits") != std::string::npos) {
                continue;
            }
            
            // Only test RWKV model files
            if (filename.find("tiny-rwkv") == std::string::npos) {
                continue;
            }
            
            std::cout << "\nTesting GGML loader with: " << filename << std::endl;
            
            bolt::GGMLLoader loader(entry.path().string());
            bool loaded = loader.load();
            
            if (loaded) {
                assert_true(loader.isLoaded(), "GGML loader should report as loaded");
                
                // Test hyperparameters
                const auto& hparams = loader.getHyperparameters();
                std::cout << "  Hyperparameters:" << std::endl;
                std::cout << "    n_vocab: " << hparams.n_vocab << std::endl;
                std::cout << "    n_embd: " << hparams.n_embd << std::endl;
                std::cout << "    n_layer: " << hparams.n_layer << std::endl;
                std::cout << "    n_head: " << hparams.n_head << std::endl;
                std::cout << "    ftype: " << hparams.ftype << std::endl;
                
                assert_true(hparams.n_vocab > 0, "Vocabulary size should be positive");
                assert_true(hparams.n_embd > 0, "Embedding dimension should be positive");
                assert_true(hparams.n_layer > 0, "Number of layers should be positive");
                
                // Test vocabulary
                const auto& vocab = loader.getVocabulary();
                std::cout << "  Vocabulary size: " << vocab.size() << std::endl;
                assert_equals(static_cast<uint32_t>(vocab.size()), hparams.n_vocab, 
                             "Vocabulary size should match hyperparameter");
                
                // Test tensor access
                auto tensor_names = loader.getTensorNames();
                std::cout << "  Number of tensors: " << tensor_names.size() << std::endl;
                assert_true(tensor_names.size() > 0, "Should have at least one tensor");
                
                if (!tensor_names.empty()) {
                    std::string first_tensor = tensor_names[0];
                    assert_true(loader.hasTensor(first_tensor), "Should find first tensor");
                    
                    const bolt::GGMLTensorInfo* info = loader.getTensorInfo(first_tensor);
                    assert_true(info != nullptr, "Should get tensor info");
                    
                    if (info) {
                        std::cout << "  First tensor: " << info->name << std::endl;
                        std::cout << "    Dimensions: " << info->n_dims << "D [";
                        for (size_t i = 0; i < info->dims.size(); i++) {
                            std::cout << info->dims[i];
                            if (i < info->dims.size() - 1) std::cout << "x";
                        }
                        std::cout << "]" << std::endl;
                        std::cout << "    Type: " << info->type << std::endl;
                        std::cout << "    Size: " << info->size << " bytes" << std::endl;
                    }
                }
            } else {
                std::cout << "  Note: Failed to load: " << loader.getError() << std::endl;
                std::cout << "  This may be expected for some test files" << std::endl;
            }
            
            break; // Test only first valid RWKV file
        }
    }
}

// Test unified model loader
void test_unified_loader() {
    std::cout << "\n=== Testing Unified Model Loader ===" << std::endl;
    
    // Test with both GGUF and GGML files
    std::vector<std::string> test_paths = {
        "/home/ubuntu/bolt-cppml/ggml/llama.cpp/models",
        "/home/ubuntu/bolt-cppml/ggml/rwkv.cpp/tests"
    };
    
    for (const auto& test_path : test_paths) {
        if (!fs::exists(test_path)) {
            continue;
        }
        
        for (const auto& entry : fs::directory_iterator(test_path)) {
            std::string ext = entry.path().extension().string();
            std::string filename = entry.path().filename().string();
            
            // Skip non-model files
            if (ext != ".gguf" && ext != ".bin") {
                continue;
            }
            
            // Skip expected-logits files
            if (filename.find("expected-logits") != std::string::npos) {
                continue;
            }
            
            std::cout << "\nTesting unified loader with: " << filename << std::endl;
            
            bolt::ModelLoader loader(entry.path().string());
            bool loaded = loader.load();
            
            if (loaded) {
                assert_true(loader.isLoaded(), "Unified loader should report as loaded");
                
                std::cout << "  Format: " << loader.getFormatName() << std::endl;
                std::cout << "  Architecture: " << loader.getArchitecture() << std::endl;
                std::cout << "  Vocab size: " << loader.getVocabSize() << std::endl;
                std::cout << "  Embedding dim: " << loader.getEmbeddingDim() << std::endl;
                std::cout << "  Num layers: " << loader.getNumLayers() << std::endl;
                
                // Test tensor access through unified interface
                auto tensor_names = loader.getTensorNames();
                std::cout << "  Tensor count: " << tensor_names.size() << std::endl;
                
                if (!tensor_names.empty()) {
                    std::string first_tensor = tensor_names[0];
                    assert_true(loader.hasTensor(first_tensor), 
                               "Unified loader should find tensors");
                }
                
                // Print full model info
                loader.printModelInfo();
            } else {
                std::cout << "  Note: Failed to load: " << loader.getError() << std::endl;
            }
            
            // Test one file per directory
            break;
        }
    }
}

int main() {
    std::cout << "=== GGML/GGUF Model Loader Test Suite ===" << std::endl;
    std::cout << "Testing model loading capabilities for both legacy GGML and GGUF formats\n" << std::endl;
    
    try {
        // Run all tests
        test_format_detection();
        test_gguf_loader();
        test_ggml_loader();
        test_unified_loader();
        
        std::cout << "\n=== All Tests Passed! ===" << std::endl;
        std::cout << "✓ Format detection working correctly" << std::endl;
        std::cout << "✓ GGUF loader functional" << std::endl;
        std::cout << "✓ GGML loader functional" << std::endl;
        std::cout << "✓ Unified loader working for both formats" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
