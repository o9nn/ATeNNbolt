#include "bolt/ai/model_loader.hpp"
#include <iostream>
#include <fstream>

namespace bolt {

ModelLoader::ModelLoader(const std::string& file_path)
    : file_path_(file_path)
    , loaded_(false)
    , format_type_(FormatType::UNKNOWN)
{
}

ModelLoader::~ModelLoader() {
}

bool ModelLoader::load() {
    // Detect format
    format_type_ = detectFormat();
    
    if (format_type_ == FormatType::UNKNOWN) {
        setError("Unable to detect file format for: " + file_path_);
        return false;
    }
    
    // Load using appropriate loader
    bool success = false;
    
    if (format_type_ == FormatType::GGUF) {
        std::cout << "Loading as GGUF format..." << std::endl;
        gguf_loader_ = std::make_unique<GGUFLoader>(file_path_);
        success = gguf_loader_->load();
        
        if (!success) {
            setError("GGUF loader failed: " + gguf_loader_->getError());
        }
    }
    else if (format_type_ == FormatType::GGML_LEGACY) {
        std::cout << "Loading as legacy GGML format..." << std::endl;
        ggml_loader_ = std::make_unique<GGMLLoader>(file_path_);
        success = ggml_loader_->load();
        
        if (!success) {
            setError("GGML loader failed: " + ggml_loader_->getError());
        }
    }
    
    loaded_ = success;
    return success;
}

ModelLoader::FormatType ModelLoader::detectFormat() {
    std::ifstream file(file_path_, std::ios::binary);
    if (!file.is_open()) {
        setError("Failed to open file: " + file_path_);
        return FormatType::UNKNOWN;
    }
    
    // Read magic number
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    
    if (!file.good()) {
        setError("Failed to read magic number");
        return FormatType::UNKNOWN;
    }
    
    file.close();
    
    // Check for GGUF magic
    if (magic == GGUF_MAGIC) {
        return FormatType::GGUF;
    }
    
    // Check for legacy GGML formats
    if (magic == GGML_MAGIC_UNVERSIONED ||
        magic == GGML_MAGIC_GGMF_V1 ||
        magic == GGML_MAGIC_GGJT_V1) {
        return FormatType::GGML_LEGACY;
    }
    
    return FormatType::UNKNOWN;
}

std::string ModelLoader::getFormatName() const {
    switch (format_type_) {
        case FormatType::GGUF:
            if (gguf_loader_) {
                return "GGUF v" + std::to_string(gguf_loader_->getVersion());
            }
            return "GGUF";
            
        case FormatType::GGML_LEGACY:
            if (ggml_loader_) {
                switch (ggml_loader_->getFormatVersion()) {
                    case GGMLFormatVersion::UNVERSIONED:
                        return "GGML (Unversioned)";
                    case GGMLFormatVersion::GGMF_V1:
                        return "GGMF v1";
                    case GGMLFormatVersion::GGJT_V1:
                        return "GGJT v1";
                    case GGMLFormatVersion::GGJT_V2:
                        return "GGJT v2";
                    case GGMLFormatVersion::GGJT_V3:
                        return "GGJT v3";
                    default:
                        return "GGML (Unknown version)";
                }
            }
            return "GGML Legacy";
            
        default:
            return "Unknown";
    }
}

std::string ModelLoader::getArchitecture() const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->getArchitecture();
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->getArchitecture();
    }
    return "unknown";
}

uint32_t ModelLoader::getVocabSize() const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->getVocabSize();
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->getHyperparameters().n_vocab;
    }
    return 0;
}

uint32_t ModelLoader::getEmbeddingDim() const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->getEmbedDim();
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->getHyperparameters().n_embd;
    }
    return 0;
}

uint32_t ModelLoader::getNumLayers() const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->getNumLayers();
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->getHyperparameters().n_layer;
    }
    return 0;
}

uint32_t ModelLoader::getNumHeads() const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        // GGUF stores this in metadata
        return gguf_loader_->getMetadataInt(getArchitecture() + ".attention.head_count", 0);
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->getHyperparameters().n_head;
    }
    return 0;
}

bool ModelLoader::hasTensor(const std::string& name) const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->hasTensor(name);
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->hasTensor(name);
    }
    return false;
}

std::vector<std::string> ModelLoader::getTensorNames() const {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->getTensorNames();
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->getTensorNames();
    }
    return {};
}

ggml_tensor* ModelLoader::loadTensor(ggml_context* ctx, const std::string& name) {
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        return gguf_loader_->loadTensor(ctx, name);
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        return ggml_loader_->loadTensor(ctx, name);
    }
    return nullptr;
}

void ModelLoader::printModelInfo() const {
    if (!loaded_) {
        std::cout << "Model not loaded" << std::endl;
        return;
    }
    
    std::cout << "\n=== Model Information ===" << std::endl;
    std::cout << "File: " << file_path_ << std::endl;
    std::cout << "Format: " << getFormatName() << std::endl;
    std::cout << "Architecture: " << getArchitecture() << std::endl;
    std::cout << "Vocabulary Size: " << getVocabSize() << std::endl;
    std::cout << "Embedding Dimension: " << getEmbeddingDim() << std::endl;
    std::cout << "Number of Layers: " << getNumLayers() << std::endl;
    std::cout << "Number of Heads: " << getNumHeads() << std::endl;
    
    auto tensor_names = getTensorNames();
    std::cout << "Number of Tensors: " << tensor_names.size() << std::endl;
    
    if (format_type_ == FormatType::GGUF && gguf_loader_) {
        std::cout << "GGUF Version: " << gguf_loader_->getVersion() << std::endl;
        std::cout << "Metadata Count: " << gguf_loader_->getMetadataCount() << std::endl;
    }
    else if (format_type_ == FormatType::GGML_LEGACY && ggml_loader_) {
        const auto& hparams = ggml_loader_->getHyperparameters();
        std::cout << "File Type (Quantization): " << hparams.ftype << std::endl;
        std::cout << "Feedforward Dimension: " << hparams.n_ff << std::endl;
        std::cout << "Rotary Dimension: " << hparams.n_rot << std::endl;
        
        const auto& vocab = ggml_loader_->getVocabulary();
        if (!vocab.empty()) {
            std::cout << "Vocabulary loaded: " << vocab.size() << " tokens" << std::endl;
        }
    }
    
    std::cout << "========================\n" << std::endl;
}

void ModelLoader::setError(const std::string& message) {
    error_message_ = message;
    std::cerr << "Model Loader Error: " << message << std::endl;
}

} // namespace bolt
