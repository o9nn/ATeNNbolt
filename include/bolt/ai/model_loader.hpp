#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include "bolt/ai/gguf_loader.hpp"
#include "bolt/ai/ggml_loader.hpp"
#include <string>
#include <memory>
#include <ggml.h>

namespace bolt {

/**
 * @brief Unified model loader that automatically detects and loads both GGUF and legacy GGML formats
 * 
 * This class provides a unified interface for loading model files in either:
 * - GGUF format (current standard)
 * - Legacy GGML formats (GGML, GGMF, GGJT)
 * 
 * The loader automatically detects the format by examining the file's magic number
 * and uses the appropriate parser.
 */
class ModelLoader {
public:
    enum class FormatType {
        UNKNOWN,
        GGUF,
        GGML_LEGACY
    };
    
    explicit ModelLoader(const std::string& file_path);
    ~ModelLoader();
    
    /**
     * @brief Load and parse the model file
     * @return true if successful, false otherwise
     */
    bool load();
    
    /**
     * @brief Check if file is loaded
     */
    bool isLoaded() const { return loaded_; }
    
    /**
     * @brief Get the detected format type
     */
    FormatType getFormatType() const { return format_type_; }
    
    /**
     * @brief Get format name as string
     */
    std::string getFormatName() const;
    
    /**
     * @brief Get model architecture
     */
    std::string getArchitecture() const;
    
    /**
     * @brief Get vocabulary size
     */
    uint32_t getVocabSize() const;
    
    /**
     * @brief Get embedding dimension
     */
    uint32_t getEmbeddingDim() const;
    
    /**
     * @brief Get number of layers
     */
    uint32_t getNumLayers() const;
    
    /**
     * @brief Get number of attention heads
     */
    uint32_t getNumHeads() const;
    
    /**
     * @brief Check if tensor exists
     */
    bool hasTensor(const std::string& name) const;
    
    /**
     * @brief Get list of all tensor names
     */
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
     * @brief Get GGUF loader (if format is GGUF)
     * @return Pointer to GGUF loader, or nullptr if not GGUF format
     */
    GGUFLoader* getGGUFLoader() { return gguf_loader_.get(); }
    
    /**
     * @brief Get GGML loader (if format is legacy GGML)
     * @return Pointer to GGML loader, or nullptr if not GGML format
     */
    GGMLLoader* getGGMLLoader() { return ggml_loader_.get(); }
    
    /**
     * @brief Print model information to stdout
     */
    void printModelInfo() const;
    
private:
    std::string file_path_;
    bool loaded_;
    std::string error_message_;
    FormatType format_type_;
    
    // Format-specific loaders
    std::unique_ptr<GGUFLoader> gguf_loader_;
    std::unique_ptr<GGMLLoader> ggml_loader_;
    
    /**
     * @brief Detect file format by reading magic number
     */
    FormatType detectFormat();
    
    void setError(const std::string& message);
};

} // namespace bolt

#endif // MODEL_LOADER_HPP
