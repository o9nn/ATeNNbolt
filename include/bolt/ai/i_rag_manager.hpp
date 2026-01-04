#ifndef I_RAG_MANAGER_HPP
#define I_RAG_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>

namespace bolt {
namespace ai {

/**
 * Generic RAG result structure
 */
struct RAGResult {
    std::string id;             // Unique identifier
    std::string content;        // Retrieved content
    float score;                // Relevance/similarity score
    std::string source;         // Source identifier
    std::string metadata;       // Additional metadata (JSON format)
    
    RAGResult() : score(0.0f) {}
    
    RAGResult(const std::string& id_, const std::string& content_, 
              float score_, const std::string& source_)
        : id(id_), content(content_), score(score_), source(source_) {}
};

/**
 * Abstract interface for all RAG managers
 * Ensures consistent API across different RAG backends
 */
class IRAGManager {
public:
    virtual ~IRAGManager() = default;
    
    /**
     * Initialize the RAG manager with a model
     * @param model_path Path to the model file (optional)
     * @return true if initialization succeeded
     */
    virtual bool initialize(const std::string& model_path = "") = 0;
    
    /**
     * Query the RAG system
     * @param query The query string
     * @param top_k Number of results to return
     * @return Vector of RAG results
     */
    virtual std::vector<RAGResult> query(const std::string& query, int top_k = 5) = 0;
    
    /**
     * Ingest content into the RAG system
     * @param content The content to ingest
     * @param id Unique identifier for the content
     */
    virtual void ingest(const std::string& content, const std::string& id) = 0;
    
    /**
     * Get the name of this RAG manager
     * @return Name identifier (e.g., "NarrativeRAG", "TemporalRAG")
     */
    virtual std::string getName() const = 0;
    
    /**
     * Check if the manager is initialized
     * @return true if initialized
     */
    virtual bool isInitialized() const = 0;
};

} // namespace ai
} // namespace bolt

#endif // I_RAG_MANAGER_HPP
