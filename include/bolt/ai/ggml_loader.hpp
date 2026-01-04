#ifndef GGML_LOADER_HPP
#define GGML_LOADER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <ggml.h>

namespace bolt {

// GGML legacy file format constants
constexpr uint32_t GGML_MAGIC_UNVERSIONED = 0x67676d6c; // "ggml" in little-endian
constexpr uint32_t GGML_MAGIC_GGMF_V1 = 0x67676d66; // "ggmf" in little-endian  
constexpr uint32_t GGML_MAGIC_GGJT_V1 = 0x67676a74; // "ggjt" v1
constexpr uint32_t GGML_MAGIC_GGJT_V2 = 0x67676a74; // "ggjt" v2
constexpr uint32_t GGML_MAGIC_GGJT_V3 = 0x67676a74; // "ggjt" v3

// GGML format versions
enum class GGMLFormatVersion {
    UNVERSIONED = 0,  // Original GGML format
    GGMF_V1 = 1,      // GGMF version 1
    GGJT_V1 = 2,      // GGJT version 1
    GGJT_V2 = 3,      // GGJT version 2
    GGJT_V3 = 4,      // GGJT version 3
    UNKNOWN = 99
};

// GGML file type (quantization)
enum class GGMLFileType : uint32_t {
    F32 = 0,      // All weights are float32
    F16 = 1,      // All weights are float16
    Q4_0 = 2,     // 4-bit quantization, type 0
    Q4_1 = 3,     // 4-bit quantization, type 1
    Q5_0 = 6,     // 5-bit quantization, type 0
    Q5_1 = 7,     // 5-bit quantization, type 1
    Q8_0 = 8,     // 8-bit quantization
    Q8_1 = 9,     // 8-bit quantization, type 1
    Q2_K = 10,    // K-quantization, 2-bit
    Q3_K = 11,    // K-quantization, 3-bit
    Q4_K = 12,    // K-quantization, 4-bit
    Q5_K = 13,    // K-quantization, 5-bit
    Q6_K = 14,    // K-quantization, 6-bit
    Q8_K = 15,    // K-quantization, 8-bit
};

// GGML hyperparameters (architecture-specific)
struct GGMLHyperparameters {
    // Common parameters
    uint32_t n_vocab;           // Vocabulary size
    uint32_t n_embd;            // Embedding dimension
    uint32_t n_mult;            // Multiplier for FFN dimension
    uint32_t n_head;            // Number of attention heads
    uint32_t n_layer;           // Number of layers
    uint32_t n_rot;             // Rotary embedding dimension
    uint32_t ftype;             // File type (quantization)
    
    // Optional parameters (model-specific)
    uint32_t n_ctx;             // Context length
    uint32_t n_ff;              // Feedforward dimension
    uint32_t n_head_kv;         // Number of KV heads (for GQA)
    float rope_freq_base;       // RoPE frequency base
    float rope_freq_scale;      // RoPE frequency scale
    
    GGMLHyperparameters() 
        : n_vocab(0), n_embd(0), n_mult(0), n_head(0), n_layer(0), n_rot(0), ftype(0),
          n_ctx(0), n_ff(0), n_head_kv(0), rope_freq_base(10000.0f), rope_freq_scale(1.0f) {}
};

// GGML Tensor information
struct GGMLTensorInfo {
    std::string name;
    uint32_t n_dims;
    std::vector<uint32_t> dims;  // Note: GGML uses uint32_t for dimensions
    ggml_type type;
    uint64_t offset;             // Offset in file
    size_t size;                 // Size in bytes
};

// GGML Vocabulary entry
struct GGMLVocabEntry {
    std::string token;
    float score;
    uint32_t token_type;  // 0=normal, 1=unknown, 2=control, 3=user-defined, 4=unused, 5=byte
};

/**
 * @brief Loader for legacy GGML file formats (GGML, GGMF, GGJT)
 * 
 * This class provides support for loading legacy GGML format model files,
 * which preceded the GGUF format. It supports:
 * - Original unversioned GGML format
 * - GGMF (GGML Format) v1
 * - GGJT (GGML with JSON Tokenizer) v1, v2, v3
 * 
 * The loader automatically detects the format version and parses accordingly.
 */
class GGMLLoader {
public:
    explicit GGMLLoader(const std::string& file_path);
    ~GGMLLoader();
    
    /**
     * @brief Load and parse the GGML file
     * @return true if successful, false otherwise
     */
    bool load();
    
    /**
     * @brief Check if file is loaded
     */
    bool isLoaded() const { return loaded_; }
    
    /**
     * @brief Get the detected format version
     */
    GGMLFormatVersion getFormatVersion() const { return format_version_; }
    
    /**
     * @brief Get hyperparameters
     */
    const GGMLHyperparameters& getHyperparameters() const { return hparams_; }
    
    /**
     * @brief Get vocabulary
     */
    const std::vector<GGMLVocabEntry>& getVocabulary() const { return vocabulary_; }
    
    /**
     * @brief Tensor access
     */
    bool hasTensor(const std::string& name) const;
    const GGMLTensorInfo* getTensorInfo(const std::string& name) const;
    std::vector<std::string> getTensorNames() const;
    
    /**
     * @brief Load tensor data into GGML context
     * @param ctx GGML context to load tensor into
     * @param name Name of the tensor to load
     * @return Pointer to loaded tensor, or nullptr on failure
     */
    ggml_tensor* loadTensor(ggml_context* ctx, const std::string& name);
    
    /**
     * @brief Get error message if loading failed
     */
    std::string getError() const { return error_message_; }
    
    /**
     * @brief Get file type (quantization method)
     */
    GGMLFileType getFileType() const { return static_cast<GGMLFileType>(hparams_.ftype); }
    
    /**
     * @brief Get architecture name (if available)
     */
    std::string getArchitecture() const { return architecture_; }
    
private:
    std::string file_path_;
    std::ifstream file_;
    bool loaded_;
    std::string error_message_;
    std::string architecture_;  // Model architecture (llama, gpt, etc.)
    
    // Format information
    uint32_t magic_;
    GGMLFormatVersion format_version_;
    uint32_t format_version_number_;  // Numeric version within format
    
    // Model hyperparameters
    GGMLHyperparameters hparams_;
    
    // Vocabulary
    std::vector<GGMLVocabEntry> vocabulary_;
    
    // Tensor storage
    std::unordered_map<std::string, GGMLTensorInfo> tensors_;
    uint64_t tensor_data_offset_;
    
    // Internal loading methods
    bool detectFormat();
    bool readHeader();
    bool readHyperparameters();
    bool readVocabulary();
    bool readTensorInfo();
    
    // Format-specific readers
    bool readHeaderUnversioned();
    bool readHeaderGGMF();
    bool readHeaderGGJT();
    
    // Utility methods
    bool readString(std::string& str, uint32_t len);
    
    template<typename T>
    bool readValue(T& value);
    
    void setError(const std::string& message);
    
    // Convert GGML file type to ggml_type
    ggml_type fileTypeToGGMLType(GGMLFileType ftype) const;
};

} // namespace bolt

#endif // GGML_LOADER_HPP
