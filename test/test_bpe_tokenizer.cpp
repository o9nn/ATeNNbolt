#include "bolt/ai/bpe_tokenizer.hpp"
#include "bolt/test/test_framework.hpp"
#include <iostream>

using namespace bolt;

// Test tokenizer creation
BOLT_TEST(BPETokenizer, BasicCreation) {
    BPETokenizer tokenizer;
    BOLT_ASSERT_FALSE(tokenizer.isLoaded());
    BOLT_ASSERT_EQUAL(tokenizer.getVocabSize(), 0);
}

// Test vocabulary loading
BOLT_TEST(BPETokenizer, VocabularyLoading) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {
        "<pad>", "<s>", "</s>", "hello", " world", "test"
    };
    
    bool result = tokenizer.loadVocabulary(tokens);
    BOLT_ASSERT_TRUE(result);
    BOLT_ASSERT_TRUE(tokenizer.isLoaded());
    BOLT_ASSERT_EQUAL(tokenizer.getVocabSize(), 6);
}

// Test empty vocabulary
BOLT_TEST(BPETokenizer, EmptyVocabulary) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens;
    bool result = tokenizer.loadVocabulary(tokens);
    BOLT_ASSERT_FALSE(result);
    BOLT_ASSERT_FALSE(tokenizer.isLoaded());
}

// Test basic encoding
BOLT_TEST(BPETokenizer, BasicEncoding) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {
        "<pad>", "<s>", "</s>", "h", "e", "l", "o", " ", "w", "r", "d",
        "hello", " world"
    };
    
    tokenizer.loadVocabulary(tokens);
    
    // Test encoding
    auto encoded = tokenizer.encode("hello");
    BOLT_ASSERT_FALSE(encoded.empty());
    
    // Should prefer longer token "hello" over individual characters
    BOLT_ASSERT_EQUAL(encoded.size(), 1);
    BOLT_ASSERT_EQUAL(encoded[0], 11); // Index of "hello"
}

// Test basic decoding
BOLT_TEST(BPETokenizer, BasicDecoding) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {
        "<pad>", "<s>", "</s>", "hello", " world"
    };
    
    tokenizer.loadVocabulary(tokens);
    
    // Test decoding
    std::vector<int> token_ids = {3, 4}; // "hello" + " world"
    std::string decoded = tokenizer.decode(token_ids);
    BOLT_ASSERT_EQUAL(decoded, std::string("hello world"));
}

// Test round-trip encoding/decoding
BOLT_TEST(BPETokenizer, RoundTrip) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {
        "<pad>", "<s>", "</s>", "h", "e", "l", "o", " ", "w", "r", "d",
        "hello", " world"
    };
    
    tokenizer.loadVocabulary(tokens);
    
    std::string original = "hello world";
    auto encoded = tokenizer.encode(original);
    std::string decoded = tokenizer.decode(encoded);
    
    BOLT_ASSERT_EQUAL(decoded, original);
}

// Test special tokens
BOLT_TEST(BPETokenizer, SpecialTokens) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {
        "<pad>", "<s>", "</s>", "test"
    };
    
    tokenizer.loadVocabulary(tokens);
    
    // Check special token IDs are identified
    int pad = tokenizer.getPADToken();
    int bos = tokenizer.getBOSToken();
    int eos = tokenizer.getEOSToken();
    
    // At least PAD should be identified
    BOLT_ASSERT_EQUAL(pad, 0);
}

// Test token lookup
BOLT_TEST(BPETokenizer, TokenLookup) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {
        "<pad>", "<s>", "</s>", "hello", "world"
    };
    
    tokenizer.loadVocabulary(tokens);
    
    // Test getting token string by ID
    BOLT_ASSERT_EQUAL(tokenizer.getTokenString(3), std::string("hello"));
    BOLT_ASSERT_EQUAL(tokenizer.getTokenString(4), std::string("world"));
    
    // Test getting token ID by string
    BOLT_ASSERT_EQUAL(tokenizer.getTokenId("hello"), 3);
    BOLT_ASSERT_EQUAL(tokenizer.getTokenId("world"), 4);
}

// Test empty string encoding
BOLT_TEST(BPETokenizer, EmptyString) {
    BPETokenizer tokenizer;
    
    std::vector<std::string> tokens = {"<pad>", "test"};
    tokenizer.loadVocabulary(tokens);
    
    auto encoded = tokenizer.encode("");
    BOLT_ASSERT_TRUE(encoded.empty());
}

int main() {
    std::cout << "Running BPE Tokenizer tests..." << std::endl;
    return bolt::test::run_all_tests();
}
