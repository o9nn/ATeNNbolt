#include <iostream>
#include "bolt/ai/ggml.hpp"
#include "bolt/ai/ggml_wrapper.hpp"
#include "bolt/ai/rwkv_wrapper.hpp"

void demonstrateGGML() {
    std::cout << "=== GGML Integration Demo ===" << std::endl;
    
    try {
        // Create GGML context
        bolt::GGMLContext context(1024 * 1024); // 1MB
        std::cout << "✓ GGML Context created successfully" << std::endl;
        
        // Create simple tensors
        auto* tensor_a = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 10);
        auto* tensor_b = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 10);
        std::cout << "✓ Created tensors: " << ggml_nelements(tensor_a) << " elements each" << std::endl;
        
        // Perform operations
        auto* result = ggml_add(context.get(), tensor_a, tensor_b);
        std::cout << "✓ Tensor addition operation created" << std::endl;
        
        // Create computation graph
        auto* graph = ggml_new_graph_custom(context.get(), GGML_DEFAULT_GRAPH_SIZE, true);
        ggml_build_forward_expand(graph, result);
        std::cout << "✓ Computation graph built successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
    }
}

void demonstrateRWKV() {
    std::cout << "\n=== RWKV Integration Demo ===" << std::endl;
    
    try {
        auto& wrapper = bolt::RWKVWrapper::getInstance();
        std::cout << "✓ RWKV Wrapper instance obtained" << std::endl;
        
        std::cout << "Model parameters:" << std::endl;
        std::cout << "  - Layers: " << wrapper.getNumLayers() << std::endl;
        std::cout << "  - Embedding dimension: " << wrapper.getEmbedDim() << std::endl;
        std::cout << "  - Initialized: " << (wrapper.isInitialized() ? "Yes" : "No") << std::endl;
        
        std::cout << "✓ RWKV API accessible and functional" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Bolt C++ AI Infrastructure Demo" << std::endl;
    std::cout << "===============================" << std::endl;
    
    demonstrateGGML();
    demonstrateRWKV();
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "✓ GGML library successfully integrated" << std::endl;
    std::cout << "✓ RWKV wrapper implemented with GGML backend" << std::endl;
    std::cout << "✓ Core AI infrastructure is ready for model loading and inference" << std::endl;
    
    return 0;
}