#include "bolt/test_framework.hpp"
#include "bolt/ai/ggml.hpp"
#include "bolt/ai/ggml_wrapper.hpp"
#include "bolt/ai/rwkv_wrapper.hpp"

using namespace bolt::test;

// ===== GGML Component Tests =====

BOLT_TEST(GGMLTest, GGMLContextCreation) {
    const size_t mem_size = 1024 * 1024; // 1MB
    
    // Test context creation
    bolt::GGMLContext context(mem_size);
    BOLT_ASSERT_TRUE(context.get() != nullptr);
    
    // Test basic tensor operations
    auto* tensor = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 100);
    BOLT_ASSERT_TRUE(tensor != nullptr);
    BOLT_ASSERT_EQ(ggml_nelements(tensor), 100);
    BOLT_ASSERT_TRUE(tensor->type == GGML_TYPE_F32);
}

BOLT_TEST(GGMLTest, GGMLWrapperBasics) {
    auto& wrapper = bolt::GGMLWrapper::getInstance();
    
    // Test that wrapper exists and is accessible (reference is always valid)
    // Just verify we can access the instance
    (void)wrapper; // Suppress unused variable warning
}

BOLT_TEST(GGMLTest, RWKVWrapperBasics) {
    auto& wrapper = bolt::RWKVWrapper::getInstance();
    
    // Test that wrapper starts uninitialized
    BOLT_ASSERT_FALSE(wrapper.isInitialized());
    
    // Test basic API accessibility
    BOLT_ASSERT_EQ(wrapper.getNumLayers(), 2);
    BOLT_ASSERT_EQ(wrapper.getEmbedDim(), 64);
}

BOLT_TEST(GGMLTest, TensorOperations) {
    const size_t mem_size = 1024 * 1024;
    bolt::GGMLContext context(mem_size);
    
    // Create tensors
    auto* a = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 10);
    auto* b = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 10);
    
    BOLT_ASSERT_TRUE(a != nullptr);
    BOLT_ASSERT_TRUE(b != nullptr);
    
    // Test basic operations
    auto* c = ggml_add(context.get(), a, b);
    BOLT_ASSERT_TRUE(c != nullptr);
    
    auto* d = ggml_mul(context.get(), a, b);
    BOLT_ASSERT_TRUE(d != nullptr);
}

BOLT_TEST(GGMLTest, GraphOperations) {
    const size_t mem_size = 1024 * 1024;
    bolt::GGMLContext context(mem_size);
    
    // Create computation graph
    auto* gf = ggml_new_graph_custom(context.get(), GGML_DEFAULT_GRAPH_SIZE, true);
    BOLT_ASSERT_TRUE(gf != nullptr);
    
    // Create simple computation
    auto* x = ggml_new_tensor_1d(context.get(), GGML_TYPE_F32, 5);
    auto* y = ggml_dup(context.get(), x);
    
    // Build graph - this tests that the graph building works
    ggml_build_forward_expand(gf, y);
    
    // Graph should be valid after building
    BOLT_ASSERT_TRUE(gf != nullptr);
}