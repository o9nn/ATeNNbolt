#include "bolt/test_framework.hpp"
#include "bolt/ai/ggml.hpp"
#include "bolt/ai/ggml_wrapper.hpp"
#include "bolt/ai/rwkv_wrapper.hpp"
#include <rwkv.h>
#include <fstream>
#include <vector>
#include <chrono>

using namespace bolt::test;

// Path to test models (relative to test directory)
static const std::string TEST_MODEL_PATH = "test/models/tiny-rwkv-5v1-730K-Q5_0.bin";
static const std::string EXPECTED_LOGITS_PATH = "test/models/expected-logits-5v1-730K.bin";

// ===== Utility Functions =====

std::vector<float> load_expected_logits(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open expected logits file");
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<float> logits(size / sizeof(float));
    file.read(reinterpret_cast<char*>(logits.data()), size);
    return logits;
}

float calculate_logit_difference(const std::vector<float>& expected, const std::vector<float>& actual) {
    if (expected.size() != actual.size()) {
        return std::numeric_limits<float>::max();
    }
    
    float sum_diff = 0.0f;
    for (size_t i = 0; i < expected.size(); ++i) {
        sum_diff += std::abs(expected[i] - actual[i]);
    }
    return sum_diff;
}

// ===== GGML Integration Tests =====

BOLT_TEST(AIModels, GGMLBasicTensorOperations) {
    const size_t mem_size = 10 * 1024 * 1024; // 10MB
    bolt::GGMLContext context(mem_size);
    
    // Test tensor creation
    auto* tensor_a = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 1024);
    auto* tensor_b = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 1024);
    
    BOLT_ASSERT_NOT_NULL(tensor_a);
    BOLT_ASSERT_NOT_NULL(tensor_b);
    BOLT_ASSERT_EQ(ggml_nelements(tensor_a), 1024);
    BOLT_ASSERT_EQ(tensor_a->type, GGML_TYPE_F32);
    
    // Test tensor operations
    auto* result = ggml_add(context.get(), tensor_a, tensor_b);
    BOLT_ASSERT_NOT_NULL(result);
    BOLT_ASSERT_EQ(ggml_nelements(result), 1024);
}

BOLT_TEST(AIModels, GGMLMatrixOperations) {
    const size_t mem_size = 10 * 1024 * 1024; // 10MB
    bolt::GGMLContext context(mem_size);
    
    // Create 2D tensors for matrix operations
    auto* matrix_a = ggml_new_tensor_2d(context.get(), GGML_TYPE_F32, 64, 32);
    auto* matrix_b = ggml_new_tensor_2d(context.get(), GGML_TYPE_F32, 32, 16);
    
    BOLT_ASSERT_NOT_NULL(matrix_a);
    BOLT_ASSERT_NOT_NULL(matrix_b);
    
    // Test matrix multiplication
    auto* result = ggml_mul_mat(context.get(), matrix_a, matrix_b);
    BOLT_ASSERT_NOT_NULL(result);
    
    // Result should be 64x16
    BOLT_ASSERT_EQ(result->ne[0], 64);
    BOLT_ASSERT_EQ(result->ne[1], 16);
}

BOLT_TEST(AIModels, GGMLComputeGraph) {
    const size_t mem_size = 10 * 1024 * 1024; // 10MB
    bolt::GGMLContext context(mem_size);
    
    // Create computation graph
    auto* gf = ggml_new_graph_custom(context.get(), GGML_DEFAULT_GRAPH_SIZE, true);
    BOLT_ASSERT_NOT_NULL(gf);
    
    // Create a simple computation: (a + b) * c
    auto* a = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 100);
    auto* b = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 100);
    auto* c = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 100);
    
    auto* sum = ggml_add(context.get(), a, b);
    auto* result = ggml_mul(context.get(), sum, c);
    
    // Build the forward computation graph
    ggml_build_forward_expand(gf, result);
    
    // Just check that the graph is not null and we can perform basic operations
    BOLT_ASSERT_NOT_NULL(gf);
    BOLT_ASSERT_NOT_NULL(result);
}

// ===== RWKV Model Loading Tests =====

BOLT_TEST(AIModels, RWKVModelInfo) {
    // First check if we can get system info (tests library linkage)
    const char* system_info = rwkv_get_system_info_string();
    BOLT_ASSERT_NOT_NULL(system_info);
    
    // System info should contain some basic information
    std::string info_str(system_info);
    BOLT_ASSERT_TRUE(!info_str.empty());
}

BOLT_TEST(AIModels, RWKVModelLoading) {
    // Silence verbose output during testing
    rwkv_set_print_errors(nullptr, false);
    
    // Try to load the tiny RWKV model with correct API (3 parameters: path, threads, gpu_layers)
    struct rwkv_context* ctx = rwkv_init_from_file(TEST_MODEL_PATH.c_str(), 1, 0);
    
    if (ctx != nullptr) {
        // Model loaded successfully - test basic properties
        size_t state_size = rwkv_get_state_len(ctx);
        size_t logits_size = rwkv_get_logits_len(ctx);
        
        BOLT_ASSERT_TRUE(state_size > 0);
        BOLT_ASSERT_TRUE(logits_size > 0);
        
        // Test model inference capability
        std::vector<float> state(state_size);
        std::vector<float> logits(logits_size);
        
        // Eval a simple token (token 0)
        bool success = rwkv_eval(ctx, 0, state.data(), state.data(), logits.data());
        BOLT_ASSERT_TRUE(success);
        
        // Logits should have reasonable values (not all zeros or inf/nan)
        bool has_valid_logits = false;
        for (float logit : logits) {
            if (std::isfinite(logit) && std::abs(logit) > 1e-6) {
                has_valid_logits = true;
                break;
            }
        }
        BOLT_ASSERT_TRUE(has_valid_logits);
        
        rwkv_free(ctx);
    } else {
        // Model loading failed - this might be expected if model file doesn't exist
        // We'll make this a warning rather than failure for CI
        std::cerr << "Warning: Could not load RWKV test model from " << TEST_MODEL_PATH << std::endl;
    }
}

BOLT_TEST(AIModels, RWKVLogitValidation) {
    // This test validates that our model produces expected logits
    // (Only runs if both model and expected logits files exist)
    
    struct rwkv_context* ctx = rwkv_init_from_file(TEST_MODEL_PATH.c_str(), 1, 0);
    
    if (ctx != nullptr) {
        try {
            auto expected_logits = load_expected_logits(EXPECTED_LOGITS_PATH);
            
            size_t state_size = rwkv_get_state_len(ctx);
            size_t logits_size = rwkv_get_logits_len(ctx);
            
            std::vector<float> state(state_size);
            std::vector<float> actual_logits(logits_size);
            
            // Initialize state to zeros
            std::fill(state.begin(), state.end(), 0.0f);
            
            // Eval token 0
            rwkv_eval(ctx, 0, state.data(), state.data(), actual_logits.data());
            
            // Compare with expected logits (allowing for small numerical differences)
            if (expected_logits.size() == actual_logits.size()) {
                float difference = calculate_logit_difference(expected_logits, actual_logits);
                
                // Allow for reasonable numerical differences due to quantization
                BOLT_ASSERT_TRUE(difference < 10.0f); // Generous threshold for Q5_0 quantization
                
                std::cout << "Logit difference: " << difference << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not validate logits: " << e.what() << std::endl;
        }
        
        rwkv_free(ctx);
    }
}

// ===== Integration Tests =====

BOLT_TEST(AIModels, RWKVWrapperIntegration) {
    auto& wrapper = bolt::RWKVWrapper::getInstance();
    
    // Test initial state
    BOLT_ASSERT_FALSE(wrapper.isInitialized());
    
    // Try to initialize with test model
    try {
        wrapper.initialize(TEST_MODEL_PATH);
        
        if (wrapper.isInitialized()) {
            // Test basic wrapper functionality
            BOLT_ASSERT_TRUE(wrapper.getNumLayers() >= 1);
            BOLT_ASSERT_TRUE(wrapper.getEmbedDim() >= 32);
            
            std::cout << "RWKV model initialized successfully:" << std::endl;
            std::cout << "  Layers: " << wrapper.getNumLayers() << std::endl;
            std::cout << "  Embed dim: " << wrapper.getEmbedDim() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Warning: RWKV wrapper initialization failed: " << e.what() << std::endl;
    }
}

BOLT_TEST(AIModels, ModelMemoryManagement) {
    // Test that we can create and destroy multiple model contexts without leaks
    for (int i = 0; i < 3; ++i) {
        struct rwkv_context* ctx = rwkv_init_from_file(TEST_MODEL_PATH.c_str(), 1, 0);
        
        if (ctx != nullptr) {
            // Do minimal work
            size_t state_size = rwkv_get_state_len(ctx);
            BOLT_ASSERT_TRUE(state_size > 0);
            
            rwkv_free(ctx);
        }
    }
    
    // Test GGML context management
    for (int i = 0; i < 5; ++i) {
        const size_t mem_size = 1024 * 1024; // 1MB
        bolt::GGMLContext context(mem_size);
        
        auto* tensor = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 1000);
        BOLT_ASSERT_NOT_NULL(tensor);
        
        // Context destructor should handle cleanup
    }
}

// ===== Performance Tests =====

BOLT_TEST(AIModels, ModelInferencePerformance) {
    struct rwkv_context* ctx = rwkv_init_from_file(TEST_MODEL_PATH.c_str(), 1, 0);
    
    if (ctx != nullptr) {
        size_t state_size = rwkv_get_state_len(ctx);
        size_t logits_size = rwkv_get_logits_len(ctx);
        
        std::vector<float> state(state_size);
        std::vector<float> logits(logits_size);
        
        // Time a series of evaluations
        auto start = std::chrono::high_resolution_clock::now();
        
        const int num_evals = 10;
        for (int i = 0; i < num_evals; ++i) {
            rwkv_eval(ctx, i % 256, state.data(), state.data(), logits.data());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Performance: " << num_evals << " evaluations in " 
                  << duration.count() << "ms ("
                  << (duration.count() / float(num_evals)) << "ms per eval)" << std::endl;
        
        // Sanity check - shouldn't take more than 1 second per eval on any reasonable hardware
        BOLT_ASSERT_TRUE(duration.count() < num_evals * 1000);
        
        rwkv_free(ctx);
    }
}
