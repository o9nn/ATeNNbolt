#ifndef BPE_TOKENIZER_HPP
#define BPE_TOKENIZER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace bolt {

class BPETokenizer {
public:
    BPETokenizer() = default;
    
    // Load vocabulary from token and score arrays
    bool loadVocabulary(const std::vector<std::string>& tokens,
                       const std::vector<float>& scores = std::vector<float>());
    
    // Encode text to token IDs
    std::vector<int> encode(const std::string& text) const;
    
    // Decode token IDs to text
    std::string decode(const std::vector<int>& tokens) const;
    
    // Vocabulary information
    int getVocabSize() const { return vocab_size_; }
    int getBOSToken() const { return bos_token_; }
    int getEOSToken() const { return eos_token_; }
    int getPADToken() const { return pad_token_; }
    int getUNKToken() const { return unk_token_; }
    
    // Check if loaded
    bool isLoaded() const { return vocab_size_ > 0; }
    
    // Get token string by ID
    std::string getTokenString(int token_id) const;
    
    // Get token ID by string
    int getTokenId(const std::string& token) const;
    
private:
    int vocab_size_ = 0;
    int bos_token_ = 1;   // Beginning of sequence
    int eos_token_ = 2;   // End of sequence
    int pad_token_ = 0;   // Padding token
    int unk_token_ = 0;   // Unknown token
    
    // Vocabulary mappings
    std::vector<std::string> id_to_token_;
    std::unordered_map<std::string, int> token_to_id_;
    std::vector<float> token_scores_;
    
    // Helper methods
    void identifySpecialTokens();
    std::vector<std::string> splitToBytes(const std::string& text) const;
    std::vector<int> greedyEncode(const std::string& text) const;
};

} // namespace bolt

#endif // BPE_TOKENIZER_HPP
