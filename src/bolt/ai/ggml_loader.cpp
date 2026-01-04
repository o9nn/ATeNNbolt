#include "bolt/ai/ggml_loader.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace bolt {

GGMLLoader::GGMLLoader(const std::string& file_path)
    : file_path_(file_path)
    , loaded_(false)
    , magic_(0)
    , format_version_(GGMLFormatVersion::UNKNOWN)
    , format_version_number_(0)
    , tensor_data_offset_(0)
{
}

GGMLLoader::~GGMLLoader() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool GGMLLoader::load() {
    // Open file
    file_.open(file_path_, std::ios::binary);
    if (!file_.is_open()) {
        setError("Failed to open file: " + file_path_);
        return false;
    }
    
    // Detect format version
    if (!detectFormat()) {
        return false;
    }
    
    // Read header based on format
    if (!readHeader()) {
        return false;
    }
    
    // Read hyperparameters
    if (!readHyperparameters()) {
        return false;
    }
    
    // Read vocabulary (if present)
    if (!readVocabulary()) {
        return false;
    }
    
    // Read tensor information
    if (!readTensorInfo()) {
        return false;
    }
    
    loaded_ = true;
    return true;
}

bool GGMLLoader::detectFormat() {
    // Read magic number
    if (!readValue(magic_)) {
        setError("Failed to read magic number");
        return false;
    }
    
    // Detect format based on magic number
    if (magic_ == GGML_MAGIC_UNVERSIONED) {
        format_version_ = GGMLFormatVersion::UNVERSIONED;
        std::cout << "Detected GGML format: Unversioned" << std::endl;
        return true;
    }
    else if (magic_ == GGML_MAGIC_GGMF_V1) {
        format_version_ = GGMLFormatVersion::GGMF_V1;
        
        // Read version number
        if (!readValue(format_version_number_)) {
            setError("Failed to read GGMF version number");
            return false;
        }
        
        std::cout << "Detected GGML format: GGMF v" << format_version_number_ << std::endl;
        return true;
    }
    else if (magic_ == GGML_MAGIC_GGJT_V1) {
        // GGJT format - read version number to determine v1/v2/v3
        if (!readValue(format_version_number_)) {
            setError("Failed to read GGJT version number");
            return false;
        }
        
        switch (format_version_number_) {
            case 1:
                format_version_ = GGMLFormatVersion::GGJT_V1;
                break;
            case 2:
                format_version_ = GGMLFormatVersion::GGJT_V2;
                break;
            case 3:
                format_version_ = GGMLFormatVersion::GGJT_V3;
                break;
            default:
                setError("Unsupported GGJT version: " + std::to_string(format_version_number_));
                return false;
        }
        
        std::cout << "Detected GGML format: GGJT v" << format_version_number_ << std::endl;
        return true;
    }
    else {
        setError("Unknown magic number: 0x" + std::to_string(magic_) + 
                 ". This may not be a GGML format file, or it may be a GGUF file.");
        return false;
    }
}

bool GGMLLoader::readHeader() {
    switch (format_version_) {
        case GGMLFormatVersion::UNVERSIONED:
            return readHeaderUnversioned();
        case GGMLFormatVersion::GGMF_V1:
            return readHeaderGGMF();
        case GGMLFormatVersion::GGJT_V1:
        case GGMLFormatVersion::GGJT_V2:
        case GGMLFormatVersion::GGJT_V3:
            return readHeaderGGJT();
        default:
            setError("Unknown format version");
            return false;
    }
}

bool GGMLLoader::readHeaderUnversioned() {
    // Unversioned GGML has minimal header
    // Just the magic number, followed by hyperparameters
    return true;
}

bool GGMLLoader::readHeaderGGMF() {
    // GGMF v1 header structure:
    // - magic (4 bytes) - already read
    // - version (4 bytes) - already read
    // Hyperparameters follow directly
    return true;
}

bool GGMLLoader::readHeaderGGJT() {
    // GGJT header structure:
    // - magic (4 bytes) - already read
    // - version (4 bytes) - already read
    // Hyperparameters follow directly
    return true;
}

bool GGMLLoader::readHyperparameters() {
    // Read common hyperparameters
    // Order: n_vocab, n_embd, n_mult, n_head, n_layer, n_rot, ftype
    
    if (!readValue(hparams_.n_vocab)) {
        setError("Failed to read n_vocab");
        return false;
    }
    
    if (!readValue(hparams_.n_embd)) {
        setError("Failed to read n_embd");
        return false;
    }
    
    if (!readValue(hparams_.n_mult)) {
        setError("Failed to read n_mult");
        return false;
    }
    
    if (!readValue(hparams_.n_head)) {
        setError("Failed to read n_head");
        return false;
    }
    
    if (!readValue(hparams_.n_layer)) {
        setError("Failed to read n_layer");
        return false;
    }
    
    if (!readValue(hparams_.n_rot)) {
        setError("Failed to read n_rot");
        return false;
    }
    
    if (!readValue(hparams_.ftype)) {
        setError("Failed to read ftype");
        return false;
    }
    
    // Calculate derived parameters
    hparams_.n_ff = ((2 * (4 * hparams_.n_embd) / 3 + hparams_.n_mult - 1) / hparams_.n_mult) * hparams_.n_mult;
    
    // For older formats, n_head_kv defaults to n_head (no GQA)
    hparams_.n_head_kv = hparams_.n_head;
    
    // Read additional parameters for GGJT v2+
    if (format_version_ == GGMLFormatVersion::GGJT_V2 || 
        format_version_ == GGMLFormatVersion::GGJT_V3) {
        
        // GGJT v2+ may have additional hyperparameters
        // For now, we'll use defaults if not present
        // In a full implementation, we'd read these conditionally
    }
    
    std::cout << "Hyperparameters:" << std::endl;
    std::cout << "  n_vocab: " << hparams_.n_vocab << std::endl;
    std::cout << "  n_embd: " << hparams_.n_embd << std::endl;
    std::cout << "  n_mult: " << hparams_.n_mult << std::endl;
    std::cout << "  n_head: " << hparams_.n_head << std::endl;
    std::cout << "  n_layer: " << hparams_.n_layer << std::endl;
    std::cout << "  n_rot: " << hparams_.n_rot << std::endl;
    std::cout << "  ftype: " << hparams_.ftype << std::endl;
    std::cout << "  n_ff: " << hparams_.n_ff << std::endl;
    
    return true;
}

bool GGMLLoader::readVocabulary() {
    // Read vocabulary tokens
    vocabulary_.reserve(hparams_.n_vocab);
    
    for (uint32_t i = 0; i < hparams_.n_vocab; i++) {
        GGMLVocabEntry entry;
        
        // Read token length
        uint32_t len;
        if (!readValue(len)) {
            setError("Failed to read token length at index " + std::to_string(i));
            return false;
        }
        
        // Read token string
        if (!readString(entry.token, len)) {
            setError("Failed to read token at index " + std::to_string(i));
            return false;
        }
        
        // Read token score (for GGJT formats)
        if (format_version_ == GGMLFormatVersion::GGJT_V1 ||
            format_version_ == GGMLFormatVersion::GGJT_V2 ||
            format_version_ == GGMLFormatVersion::GGJT_V3) {
            
            if (!readValue(entry.score)) {
                setError("Failed to read token score at index " + std::to_string(i));
                return false;
            }
        } else {
            entry.score = 0.0f;  // Default for formats without scores
        }
        
        entry.token_type = 0;  // Default: normal token
        vocabulary_.push_back(entry);
    }
    
    std::cout << "Loaded " << vocabulary_.size() << " vocabulary tokens" << std::endl;
    return true;
}

bool GGMLLoader::readTensorInfo() {
    // Read tensor information
    // In GGML formats, tensors are stored sequentially
    // Each tensor has: name_len, name, n_dims, dims[], type
    
    tensor_data_offset_ = file_.tellg();
    
    while (file_.good() && !file_.eof()) {
        GGMLTensorInfo info;
        
        // Read tensor name length
        uint32_t name_len;
        if (!readValue(name_len)) {
            // End of tensor info section
            break;
        }
        
        // Sanity check on name length
        if (name_len == 0 || name_len > 1024) {
            // Likely reached tensor data section
            break;
        }
        
        // Read tensor name
        if (!readString(info.name, name_len)) {
            setError("Failed to read tensor name");
            return false;
        }
        
        // Read number of dimensions
        if (!readValue(info.n_dims)) {
            setError("Failed to read tensor dimensions count for: " + info.name);
            return false;
        }
        
        // Sanity check on dimensions
        if (info.n_dims == 0 || info.n_dims > 4) {
            setError("Invalid number of dimensions for tensor: " + info.name);
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
        
        // Store current position as tensor offset
        info.offset = file_.tellg();
        
        // Calculate tensor size
        size_t n_elements = 1;
        for (uint32_t dim : info.dims) {
            n_elements *= dim;
        }
        
        // Calculate size based on type
        size_t type_size = ggml_type_size(info.type);
        size_t block_size = ggml_blck_size(info.type);
        info.size = (n_elements * type_size) / block_size;
        
        // Skip tensor data
        file_.seekg(info.size, std::ios::cur);
        
        tensors_[info.name] = info;
        
        std::cout << "Tensor: " << info.name 
                  << " [" << info.n_dims << "D: ";
        for (uint32_t d = 0; d < info.n_dims; d++) {
            std::cout << info.dims[d];
            if (d < info.n_dims - 1) std::cout << "x";
        }
        std::cout << "] type=" << info.type 
                  << " size=" << info.size << " bytes" << std::endl;
    }
    
    std::cout << "Loaded " << tensors_.size() << " tensors" << std::endl;
    return true;
}

bool GGMLLoader::hasTensor(const std::string& name) const {
    return tensors_.find(name) != tensors_.end();
}

const GGMLTensorInfo* GGMLLoader::getTensorInfo(const std::string& name) const {
    auto it = tensors_.find(name);
    if (it != tensors_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> GGMLLoader::getTensorNames() const {
    std::vector<std::string> names;
    names.reserve(tensors_.size());
    for (const auto& pair : tensors_) {
        names.push_back(pair.first);
    }
    return names;
}

ggml_tensor* GGMLLoader::loadTensor(ggml_context* ctx, const std::string& name) {
    if (!ctx) {
        setError("Invalid GGML context");
        return nullptr;
    }
    
    const GGMLTensorInfo* info = getTensorInfo(name);
    if (!info) {
        setError("Tensor not found: " + name);
        return nullptr;
    }
    
    // Create tensor in context
    ggml_tensor* tensor = nullptr;
    
    switch (info->n_dims) {
        case 1:
            tensor = ggml_new_tensor_1d(ctx, info->type, info->dims[0]);
            break;
        case 2:
            tensor = ggml_new_tensor_2d(ctx, info->type, info->dims[0], info->dims[1]);
            break;
        case 3:
            tensor = ggml_new_tensor_3d(ctx, info->type, info->dims[0], info->dims[1], info->dims[2]);
            break;
        case 4:
            tensor = ggml_new_tensor_4d(ctx, info->type, info->dims[0], info->dims[1], 
                                       info->dims[2], info->dims[3]);
            break;
        default:
            setError("Unsupported number of dimensions: " + std::to_string(info->n_dims));
            return nullptr;
    }
    
    if (!tensor) {
        setError("Failed to create tensor in context");
        return nullptr;
    }
    
    // Set tensor name
    ggml_set_name(tensor, name.c_str());
    
    // Read tensor data from file
    file_.seekg(info->offset);
    file_.read(reinterpret_cast<char*>(tensor->data), info->size);
    
    if (!file_.good()) {
        setError("Failed to read tensor data for: " + name);
        return nullptr;
    }
    
    return tensor;
}

bool GGMLLoader::readString(std::string& str, uint32_t len) {
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

template<typename T>
bool GGMLLoader::readValue(T& value) {
    file_.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!file_.good()) {
        return false;
    }
    return true;
}

void GGMLLoader::setError(const std::string& message) {
    error_message_ = message;
    std::cerr << "GGML Loader Error: " << message << std::endl;
}

ggml_type GGMLLoader::fileTypeToGGMLType(GGMLFileType ftype) const {
    switch (ftype) {
        case GGMLFileType::F32:   return GGML_TYPE_F32;
        case GGMLFileType::F16:   return GGML_TYPE_F16;
        case GGMLFileType::Q4_0:  return GGML_TYPE_Q4_0;
        case GGMLFileType::Q4_1:  return GGML_TYPE_Q4_1;
        case GGMLFileType::Q5_0:  return GGML_TYPE_Q5_0;
        case GGMLFileType::Q5_1:  return GGML_TYPE_Q5_1;
        case GGMLFileType::Q8_0:  return GGML_TYPE_Q8_0;
        case GGMLFileType::Q8_1:  return GGML_TYPE_Q8_1;
        case GGMLFileType::Q2_K:  return GGML_TYPE_Q2_K;
        case GGMLFileType::Q3_K:  return GGML_TYPE_Q3_K;
        case GGMLFileType::Q4_K:  return GGML_TYPE_Q4_K;
        case GGMLFileType::Q5_K:  return GGML_TYPE_Q5_K;
        case GGMLFileType::Q6_K:  return GGML_TYPE_Q6_K;
        case GGMLFileType::Q8_K:  return GGML_TYPE_Q8_K;
        default:                  return GGML_TYPE_F32;
    }
}

} // namespace bolt
