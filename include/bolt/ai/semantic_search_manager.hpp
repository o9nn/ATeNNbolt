#ifndef SEMANTIC_SEARCH_MANAGER_HPP
#define SEMANTIC_SEARCH_MANAGER_HPP

#include "bolt/ai/vector_database.hpp"
#include "bolt/ai/direct_gguf_inference.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

namespace bolt {
namespace ai {

/**
 * Configuration for semantic search indexing
 */
struct IndexingConfig {
    size_t max_chunk_size = 1000;      // Max characters per chunk
    size_t chunk_overlap = 200;        // Overlap between chunks
    bool index_hidden_files = false;   // Whether to index hidden files
    std::vector<std::string> file_extensions = {".cpp", ".hpp", ".h", ".c", ".txt", ".md"};
    size_t max_file_size = 1024 * 1024; // Max file size (1MB)
};

/**
 * Statistics for indexing operations
 */
struct IndexingStats {
    size_t files_indexed = 0;
    size_t files_skipped = 0;
    size_t total_chunks = 0;
    size_t errors = 0;
    double total_time_seconds = 0.0;
};

/**
 * Manager for semantic search operations on files
 * Handles indexing, embedding generation, and similarity search
 */
class SemanticSearchManager {
private:
    std::unique_ptr<DirectGGUFInference> inference_engine_;
    VectorDatabase vector_db_;
    IndexingConfig config_;
    IndexingStats stats_;
    bool initialized_ = false;
    std::atomic<bool> indexing_in_progress_{false};
    
    // Helper methods
    std::vector<std::string> chunkText(const std::string& text) const;
    bool shouldIndexFile(const std::string& file_path) const;
    std::string readFileContent(const std::string& file_path) const;
    std::vector<float> generateEmbedding(const std::string& text);

public:
    SemanticSearchManager();
    explicit SemanticSearchManager(const IndexingConfig& config);
    ~SemanticSearchManager() = default;
    
    // Initialization
    bool initialize(const std::string& model_path = "");
    bool isInitialized() const { return initialized_; }
    void shutdown();
    
    // Configuration
    void setConfig(const IndexingConfig& config) { config_ = config; }
    IndexingConfig getConfig() const { return config_; }
    
    // Indexing operations
    bool indexFile(const std::string& file_path);
    bool indexDirectory(const std::string& directory_path, bool recursive = true);
    bool removeFileFromIndex(const std::string& file_path);
    void clearIndex();
    
    // Search operations
    std::vector<SearchResult> search(
        const std::string& query, 
        int top_k = 5,
        float min_similarity = 0.3f
    );
    
    std::vector<SearchResult> searchInDirectory(
        const std::string& query,
        const std::string& directory_path,
        int top_k = 5
    );
    
    // Statistics and info
    IndexingStats getStats() const { return stats_; }
    size_t getIndexedFileCount() const;
    bool isIndexingInProgress() const { return indexing_in_progress_; }
    
    // Callbacks for progress tracking
    using ProgressCallback = std::function<void(const std::string&, size_t, size_t)>;
    void setProgressCallback(ProgressCallback callback) { progress_callback_ = callback; }
    
private:
    ProgressCallback progress_callback_;
};

} // namespace ai
} // namespace bolt

#endif // SEMANTIC_SEARCH_MANAGER_HPP
