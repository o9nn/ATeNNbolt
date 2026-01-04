#ifndef VECTOR_DATABASE_HPP
#define VECTOR_DATABASE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace bolt {
namespace ai {

/**
 * A record in the vector database containing file metadata and embedding
 */
struct VectorRecord {
    std::string id;                    // Unique identifier (file path)
    std::vector<float> embedding;      // Vector embedding
    std::string content_preview;       // First N characters of content
    std::string file_type;             // File extension
    size_t file_size;                  // File size in bytes
    long last_modified;                // Last modification timestamp
    
    VectorRecord() : file_size(0), last_modified(0) {}
};

/**
 * Search result containing file path and similarity score
 */
struct SearchResult {
    std::string file_path;
    float similarity_score;
    std::string content_preview;
    std::string file_type;
    
    SearchResult(const std::string& path, float score, 
                 const std::string& preview = "", 
                 const std::string& type = "")
        : file_path(path), similarity_score(score), 
          content_preview(preview), file_type(type) {}
};

/**
 * In-memory vector database for semantic search
 * Thread-safe implementation for concurrent access
 */
class VectorDatabase {
private:
    std::unordered_map<std::string, VectorRecord> records_;
    mutable std::mutex mutex_;
    
public:
    VectorDatabase() = default;
    ~VectorDatabase() = default;
    
    // Record management
    void addOrUpdateRecord(const VectorRecord& record);
    bool removeRecord(const std::string& id);
    bool hasRecord(const std::string& id) const;
    VectorRecord getRecord(const std::string& id) const;
    
    // Search operations
    std::vector<SearchResult> searchSimilar(
        const std::vector<float>& query_embedding,
        int top_k = 5,
        float min_similarity = 0.0f
    ) const;
    
    // Statistics
    size_t getRecordCount() const;
    std::vector<std::string> getAllIds() const;
    void clear();
    
    // Similarity metrics
    static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    static float euclideanDistance(const std::vector<float>& a, const std::vector<float>& b);
    
    // Persistence (optional for future implementation)
    bool saveToFile(const std::string& file_path) const;
    bool loadFromFile(const std::string& file_path);
};

} // namespace ai
} // namespace bolt

#endif // VECTOR_DATABASE_HPP
