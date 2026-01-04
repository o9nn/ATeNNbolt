#include "bolt/ai/gguf_loader.hpp"
#include "bolt/test/test_framework.hpp"
#include <iostream>
#include <fstream>

using namespace bolt;

// Test basic GGUF loader creation
BOLT_TEST(GGUFLoader, BasicCreation) {
    GGUFLoader loader("nonexistent.gguf");
    BOLT_ASSERT_FALSE(loader.isLoaded());
}

// Test GGUF loader with invalid file
BOLT_TEST(GGUFLoader, InvalidFile) {
    GGUFLoader loader("nonexistent.gguf");
    bool result = loader.load();
    BOLT_ASSERT_FALSE(result);
    BOLT_ASSERT_FALSE(loader.getError().empty());
}

// Test metadata access
BOLT_TEST(GGUFLoader, MetadataAccess) {
    // This test would require a real GGUF file
    // For now, just verify the API works
    GGUFLoader loader("test.gguf");
    
    // These should not crash even with no file loaded
    BOLT_ASSERT_FALSE(loader.hasMetadata("test.key"));
    BOLT_ASSERT_EQUAL(loader.getMetadataString("test.key", "default"), std::string("default"));
    BOLT_ASSERT_EQUAL(loader.getMetadataInt("test.key", 42), 42);
}

// Test tensor access
BOLT_TEST(GGUFLoader, TensorAccess) {
    GGUFLoader loader("test.gguf");
    
    // Should not crash with no file loaded
    BOLT_ASSERT_FALSE(loader.hasTensor("test.tensor"));
    BOLT_ASSERT_EQUAL(loader.getTensorInfo("test.tensor"), nullptr);
    BOLT_ASSERT_TRUE(loader.getTensorNames().empty());
}

// Test model information extraction
BOLT_TEST(GGUFLoader, ModelInformation) {
    GGUFLoader loader("test.gguf");
    
    // Should return 0 for unloaded model
    BOLT_ASSERT_EQUAL(loader.getNumLayers(), 0);
    BOLT_ASSERT_EQUAL(loader.getEmbedDim(), 0);
    BOLT_ASSERT_EQUAL(loader.getVocabSize(), 0);
    BOLT_ASSERT_EQUAL(loader.getArchitecture(), std::string("unknown"));
}

int main() {
    std::cout << "Running GGUF Loader tests..." << std::endl;
    
    // Note: Full integration tests with real GGUF files should be added
    // when test models are available
    
    return bolt::test::run_all_tests();
}
