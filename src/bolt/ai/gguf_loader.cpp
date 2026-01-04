#include "bolt/ai/gguf_loader.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace bolt {

GGUFLoader::GGUFLoader(const std::string& file_path)
    : file_path_(file_path)
    , loaded_(false)
    , magic_(0)
    , version_(0)
    , tensor_count_(0)
    , metadata_kv_count_(0)
    , tensor_data_offset_(0)
    , alignment_(32) // Default alignment
{
}

GGUFLoader::~GGUFLoader() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool GGUFLoader::load() {
    // Open file
    file_.open(file_path_, std::ios::binary);
    if (!file_.is_open()) {
        setError("Failed to open file: " + file_path_);
        return false;
    }
    
    // Read header
    if (!readHeader()) {
        return false;
    }
    
    // Read metadata
    if (!readMetadata()) {
        return false;
    }
    
    // Get alignment from metadata
    if (hasMetadata("general.alignment")) {
        alignment_ = getMetadataInt("general.alignment", 32);
    }
    
    // Read tensor information
    if (!readTensorInfo()) {
        return false;
    }
    
    loaded_ = true;
    return true;
}

bool GGUFLoader::readHeader() {
    // Read magic number
    if (!readValue(magic_)) {
        setError("Failed to read magic number");
        return false;
    }
    
    if (magic_ != GGUF_MAGIC) {
        setError("Invalid GGUF magic number. Expected 0x46554747, got 0x" + 
                 std::to_string(magic_));
        return false;
    }
    
    // Read version
    if (!readValue(version_)) {
        setError("Failed to read version");
        return false;
    }
    
    if (version_ != GGUF_VERSION_V2 && version_ != GGUF_VERSION_V3) {
        setError("Unsupported GGUF version: " + std::to_string(version_) + 
                 ". Supported versions: 2, 3");
        return false;
    }
    
    // Read tensor count
    if (!readValue(tensor_count_)) {
        setError("Failed to read tensor count");
        return false;
    }
    
    // Read metadata count
    if (!readValue(metadata_kv_count_)) {
        setError("Failed to read metadata count");
        return false;
    }
    
    return true;
}

bool GGUFLoader::readString(std::string& str) {
    uint64_t len;
    if (!readValue(len)) {
        return false;
    }
    
    if (len == 0) {
        str.clear();
        return true;
    }
    
    str.resize(len);
    file_.read(&str[0], len);
    
    if (!file_.good()) {
        setError("Failed to read string data");
        return false;
    }
    
    return true;
}

bool GGUFLoader::readMetadataValue(GGUFValueType type, MetadataValue& value) {
    switch (type) {
        case GGUFValueType::UINT8: {
            uint8_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::INT8: {
            int8_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::UINT16: {
            uint16_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::INT16: {
            int16_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::UINT32: {
            uint32_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::INT32: {
            int32_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::UINT64: {
            uint64_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::INT64: {
            int64_t v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::FLOAT32: {
            float v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::FLOAT64: {
            double v;
            if (!readValue(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::BOOL: {
            uint8_t v;
            if (!readValue(v)) return false;
            value = static_cast<bool>(v);
            break;
        }
        case GGUFValueType::STRING: {
            std::string v;
            if (!readString(v)) return false;
            value = v;
            break;
        }
        case GGUFValueType::ARRAY: {
            // Read array type
            uint32_t array_type_raw;
            if (!readValue(array_type_raw)) return false;
            GGUFValueType array_type = static_cast<GGUFValueType>(array_type_raw);
            
            // Read array count
            uint64_t count;
            if (!readValue(count)) return false;
            
            // Read array elements based on type
            switch (array_type) {
                case GGUFValueType::STRING: {
                    std::vector<std::string> arr;
                    arr.reserve(count);
                    for (uint64_t i = 0; i < count; i++) {
                        std::string elem;
                        if (!readString(elem)) return false;
                        arr.push_back(std::move(elem));
                    }
                    value = arr;
                    break;
                }
                case GGUFValueType::FLOAT32: {
                    std::vector<float> arr;
                    if (!readArray(arr, count)) return false;
                    value = arr;
                    break;
                }
                case GGUFValueType::INT32: {
                    std::vector<int32_t> arr;
                    if (!readArray(arr, count)) return false;
                    value = arr;
                    break;
                }
                case GGUFValueType::UINT32: {
                    std::vector<uint32_t> arr;
                    if (!readArray(arr, count)) return false;
                    value = arr;
                    break;
                }
                default:
                    setError("Unsupported array element type: " + std::to_string(static_cast<uint32_t>(array_type)));
                    return false;
            }
            break;
        }
        default:
            setError("Unsupported metadata value type: " + std::to_string(static_cast<uint32_t>(type)));
            return false;
    }
    
    return true;
}

bool GGUFLoader::readMetadata() {
    for (uint64_t i = 0; i < metadata_kv_count_; i++) {
        // Read key
        std::string key;
        if (!readString(key)) {
            setError("Failed to read metadata key at index " + std::to_string(i));
            return false;
        }
        
        // Read value type
        uint32_t type_raw;
        if (!readValue(type_raw)) {
            setError("Failed to read metadata value type for key: " + key);
            return false;
        }
        GGUFValueType type = static_cast<GGUFValueType>(type_raw);
        
        // Read value
        MetadataValue value;
        if (!readMetadataValue(type, value)) {
            setError("Failed to read metadata value for key: " + key);
            return false;
        }
        
        metadata_[key] = value;
    }
    
    return true;
}

bool GGUFLoader::readTensorInfo() {
    // Read tensor information
    for (uint64_t i = 0; i < tensor_count_; i++) {
        TensorInfo info;
        
        // Read tensor name
        if (!readString(info.name)) {
            setError("Failed to read tensor name at index " + std::to_string(i));
            return false;
        }
        
        // Read number of dimensions
        if (!readValue(info.n_dims)) {
            setError("Failed to read tensor dimensions count for: " + info.name);
            return false;
        }
        
        // Read dimensions
        info.dims.resize(info.n_dims);
        for (uint32_t d = 0; d < info.n_dims; d++) {
            if (!readValue(info.dims[d])) {
                setError("Failed to read tensor dimension for: " + info.name);
                return false;
            }
        }
        
        // Read tensor type
        uint32_t type_raw;
        if (!readValue(type_raw)) {
            setError("Failed to read tensor type for: " + info.name);
            return false;
        }
        info.type = static_cast<ggml_type>(type_raw);
        
        // Read tensor offset
        if (!readValue(info.offset)) {
            setError("Failed to read tensor offset for: " + info.name);
            return false;
        }
        
        // Calculate tensor size
        size_t n_elements = 1;
        for (uint64_t dim : info.dims) {
            n_elements *= dim;
        }
        info.size = n_elements * ggml_type_size(info.type) / ggml_blck_size(info.type);
        
        tensors_[info.name] = info;
    }
    
    // Calculate tensor data offset (current position, aligned)
    tensor_data_offset_ = alignOffset(file_.tellg());
    
    return true;
}

bool GGUFLoader::hasMetadata(const std::string& key) const {
    return metadata_.find(key) != metadata_.end();
}

MetadataValue GGUFLoader::getMetadata(const std::string& key) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        return it->second;
    }
    return MetadataValue();
}

std::string GGUFLoader::getMetadataString(const std::string& key, const std::string& default_value) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return default_value;
}

int64_t GGUFLoader::getMetadataInt(const std::string& key, int64_t default_value) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        if (std::holds_alternative<int64_t>(it->second)) {
            return std::get<int64_t>(it->second);
        } else if (std::holds_alternative<int32_t>(it->second)) {
            return std::get<int32_t>(it->second);
        } else if (std::holds_alternative<uint32_t>(it->second)) {
            return std::get<uint32_t>(it->second);
        } else if (std::holds_alternative<uint64_t>(it->second)) {
            return std::get<uint64_t>(it->second);
        }
    }
    return default_value;
}

double GGUFLoader::getMetadataFloat(const std::string& key, double default_value) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end()) {
        if (std::holds_alternative<double>(it->second)) {
            return std::get<double>(it->second);
        } else if (std::holds_alternative<float>(it->second)) {
            return std::get<float>(it->second);
        }
    }
    return default_value;
}

bool GGUFLoader::getMetadataBool(const std::string& key, bool default_value) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end() && std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return default_value;
}

std::vector<std::string> GGUFLoader::getMetadataStringArray(const std::string& key) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end() && std::holds_alternative<std::vector<std::string>>(it->second)) {
        return std::get<std::vector<std::string>>(it->second);
    }
    return std::vector<std::string>();
}

std::vector<float> GGUFLoader::getMetadataFloatArray(const std::string& key) const {
    auto it = metadata_.find(key);
    if (it != metadata_.end() && std::holds_alternative<std::vector<float>>(it->second)) {
        return std::get<std::vector<float>>(it->second);
    }
    return std::vector<float>();
}

bool GGUFLoader::hasTensor(const std::string& name) const {
    return tensors_.find(name) != tensors_.end();
}

const TensorInfo* GGUFLoader::getTensorInfo(const std::string& name) const {
    auto it = tensors_.find(name);
    if (it != tensors_.end()) {
        return &it->second;
    }
    return nullptr;
}

ggml_tensor* GGUFLoader::loadTensor(ggml_context* ctx, const std::string& name) {
    auto it = tensors_.find(name);
    if (it == tensors_.end()) {
        setError("Tensor not found: " + name);
        return nullptr;
    }
    
    const TensorInfo& info = it->second;
    
    // Create GGML tensor
    ggml_tensor* tensor = nullptr;
    switch (info.n_dims) {
        case 1:
            tensor = ggml_new_tensor_1d(ctx, info.type, info.dims[0]);
            break;
        case 2:
            tensor = ggml_new_tensor_2d(ctx, info.type, info.dims[0], info.dims[1]);
            break;
        case 3:
            tensor = ggml_new_tensor_3d(ctx, info.type, info.dims[0], info.dims[1], info.dims[2]);
            break;
        case 4:
            tensor = ggml_new_tensor_4d(ctx, info.type, info.dims[0], info.dims[1], info.dims[2], info.dims[3]);
            break;
        default:
            setError("Unsupported tensor dimensionality: " + std::to_string(info.n_dims));
            return nullptr;
    }
    
    if (!tensor) {
        setError("Failed to create GGML tensor for: " + name);
        return nullptr;
    }
    
    // Set tensor name
    ggml_set_name(tensor, name.c_str());
    
    // Seek to tensor data
    uint64_t data_offset = tensor_data_offset_ + info.offset;
    file_.seekg(data_offset);
    
    if (!file_.good()) {
        setError("Failed to seek to tensor data for: " + name);
        return nullptr;
    }
    
    // Read tensor data
    file_.read(static_cast<char*>(tensor->data), info.size);
    
    if (!file_.good()) {
        setError("Failed to read tensor data for: " + name);
        return nullptr;
    }
    
    return tensor;
}

std::vector<std::string> GGUFLoader::getTensorNames() const {
    std::vector<std::string> names;
    names.reserve(tensors_.size());
    for (const auto& pair : tensors_) {
        names.push_back(pair.first);
    }
    return names;
}

int GGUFLoader::getNumLayers() const {
    // Try different possible keys
    if (hasMetadata("rwkv.block_count")) {
        return getMetadataInt("rwkv.block_count");
    }
    if (hasMetadata("llama.block_count")) {
        return getMetadataInt("llama.block_count");
    }
    return 0;
}

int GGUFLoader::getEmbedDim() const {
    if (hasMetadata("rwkv.embedding_length")) {
        return getMetadataInt("rwkv.embedding_length");
    }
    if (hasMetadata("llama.embedding_length")) {
        return getMetadataInt("llama.embedding_length");
    }
    return 0;
}

int GGUFLoader::getVocabSize() const {
    // Try to get from tokenizer tokens array
    auto tokens = getMetadataStringArray("tokenizer.ggml.tokens");
    if (!tokens.empty()) {
        return tokens.size();
    }
    
    // Fallback to metadata
    if (hasMetadata("rwkv.vocab_size")) {
        return getMetadataInt("rwkv.vocab_size");
    }
    return 0;
}

std::string GGUFLoader::getArchitecture() const {
    return getMetadataString("general.architecture", "unknown");
}

template<typename T>
bool GGUFLoader::readValue(T& value) {
    file_.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!file_.good()) {
        setError("Failed to read value of size " + std::to_string(sizeof(T)));
        return false;
    }
    return true;
}

template<typename T>
bool GGUFLoader::readArray(std::vector<T>& arr, uint64_t count) {
    arr.resize(count);
    file_.read(reinterpret_cast<char*>(arr.data()), count * sizeof(T));
    if (!file_.good()) {
        setError("Failed to read array of " + std::to_string(count) + " elements");
        return false;
    }
    return true;
}

void GGUFLoader::setError(const std::string& message) {
    error_message_ = message;
    std::cerr << "GGUF Loader Error: " << message << std::endl;
}

uint64_t GGUFLoader::alignOffset(uint64_t offset) const {
    return (offset + alignment_ - 1) & ~(alignment_ - 1);
}

} // namespace bolt
