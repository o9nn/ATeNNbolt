#ifndef GGUF_LOADER_HPP
#define GGUF_LOADER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <fstream>
#include <ggml.h>

namespace bolt {

// GGUF file format constants
constexpr uint32_t GGUF_MAGIC = 0x46554747; // "GGUF" in little-endian
constexpr uint32_t GGUF_VERSION_V2 = 2;
constexpr uint32_t GGUF_VERSION_V3 = 3;

// GGUF value types
enum class GGUFValueType : uint32_t {
    UINT8 = 0,
    INT8 = 1,
    UINT16 = 2,
    INT16 = 3,
    UINT32 = 4,
    INT32 = 5,
    FLOAT32 = 6,
    BOOL = 7,
    STRING = 8,
    ARRAY = 9,
    UINT64 = 10,
    INT64 = 11,
    FLOAT64 = 12,
};

// Metadata value variant
using MetadataValue = std::variant<
    uint8_t, int8_t, uint16_t, int16_t,
    uint32_t, int32_t, uint64_t, int64_t,
    float, double, bool, std::string,
    std::vector<uint8_t>, std::vector<int8_t>,
    std::vector<uint16_t>, std::vector<int16_t>,
    std::vector<uint32_t>, std::vector<int32_t>,
    std::vector<uint64_t>, std::vector<int64_t>,
    std::vector<float>, std::vector<double>,
    std::vector<bool>, std::vector<std::string>
>;

// Tensor information
struct TensorInfo {
    std::string name;
    uint32_t n_dims;
    std::vector<uint64_t> dims;
    ggml_type type;
    uint64_t offset;
    size_t size;
};

// GGUF Loader class
class GGUFLoader {
public:
    explicit GGUFLoader(const std::string& file_path);
    ~GGUFLoader();
    
    // Load and parse the GGUF file
    bool load();
    
    // Check if file is loaded
    bool isLoaded() const { return loaded_; }
    
    // Metadata access
    bool hasMetadata(const std::string& key) const;
    MetadataValue getMetadata(const std::string& key) const;
    
    // Convenience methods for common types
    std::string getMetadataString(const std::string& key, const std::string& default_value = "") const;
    int64_t getMetadataInt(const std::string& key, int64_t default_value = 0) const;
    double getMetadataFloat(const std::string& key, double default_value = 0.0) const;
    bool getMetadataBool(const std::string& key, bool default_value = false) const;
    std::vector<std::string> getMetadataStringArray(const std::string& key) const;
    std::vector<float> getMetadataFloatArray(const std::string& key) const;
    
    // Tensor access
    bool hasTensor(const std::string& name) const;
    const TensorInfo* getTensorInfo(const std::string& name) const;
    ggml_tensor* loadTensor(ggml_context* ctx, const std::string& name);
    std::vector<std::string> getTensorNames() const;
    
    // Model information
    uint32_t getVersion() const { return version_; }
    uint64_t getTensorCount() const { return tensor_count_; }
    uint64_t getMetadataCount() const { return metadata_kv_count_; }
    
    // Model parameters (convenience methods)
    int getNumLayers() const;
    int getEmbedDim() const;
    int getVocabSize() const;
    std::string getArchitecture() const;
    
    // Get error message if loading failed
    std::string getError() const { return error_message_; }
    
private:
    std::string file_path_;
    std::ifstream file_;
    bool loaded_;
    std::string error_message_;
    
    // GGUF header
    uint32_t magic_;
    uint32_t version_;
    uint64_t tensor_count_;
    uint64_t metadata_kv_count_;
    
    // Metadata storage
    std::unordered_map<std::string, MetadataValue> metadata_;
    
    // Tensor storage
    std::unordered_map<std::string, TensorInfo> tensors_;
    uint64_t tensor_data_offset_;
    
    // Alignment
    uint64_t alignment_;
    
    // Internal methods
    bool readHeader();
    bool readMetadata();
    bool readTensorInfo();
    
    bool readString(std::string& str);
    bool readMetadataValue(GGUFValueType type, MetadataValue& value);
    
    template<typename T>
    bool readValue(T& value);
    
    template<typename T>
    bool readArray(std::vector<T>& arr, uint64_t count);
    
    void setError(const std::string& message);
    
    // Calculate aligned offset
    uint64_t alignOffset(uint64_t offset) const;
};

} // namespace bolt

#endif // GGUF_LOADER_HPP
