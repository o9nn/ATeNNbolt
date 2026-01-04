#ifndef UNIFIED_RAG_MANAGER_HPP
#define UNIFIED_RAG_MANAGER_HPP

#include "bolt/ai/i_rag_manager.hpp"
#include "bolt/ai/semantic_search_manager.hpp"
#include "bolt/ai/narrative_rag_manager.hpp"
#include "bolt/ai/temporal_rag_manager.hpp"
#include "bolt/ai/rag_router.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace bolt {
namespace ai {

/**
 * Query type classification for RAG routing
 */
enum class RAGQueryType {
    GENERAL_SEMANTIC,   // General semantic search
    NARRATIVE,          // Story/narrative-related query
    TEMPORAL,           // Time-series/temporal query
    HYBRID              // Requires multiple backends
};

/**
 * Configuration for unified RAG system
 */
struct UnifiedRAGConfig {
    bool enable_narrative_rag = true;
    bool enable_temporal_rag = true;
    bool enable_general_rag = true;
    bool enable_hybrid_queries = true;
    int default_top_k = 5;
    float confidence_threshold = 0.3f;
};

/**
 * Result from the RAG router
 */
struct RoutingDecision {
    RAGQueryType query_type;
    std::string backend_name;
    float confidence;
    std::string reasoning;
};

/**
 * Unified RAG Manager that orchestrates multiple specialized RAG backends
 * Provides a single interface for all RAG operations
 */
class UnifiedRAGManager {
private:
    // Specialized RAG backends
    std::unique_ptr<SemanticSearchManager> general_rag_;
    std::unique_ptr<NarrativeRAGManager> narrative_rag_;
    std::unique_ptr<TemporalRAGManager> temporal_rag_;
    
    // RAG Router
    std::unique_ptr<RAGRouter> router_;
    
    // Configuration
    UnifiedRAGConfig config_;
    bool initialized_ = false;
    
    // Hybrid query processing
    std::vector<RAGResult> processHybridQuery(const std::string& query, int top_k);
    std::vector<RAGResult> mergeResults(
        const std::vector<RAGResult>& results1,
        const std::vector<RAGResult>& results2
    ) const;

public:
    UnifiedRAGManager();
    explicit UnifiedRAGManager(const UnifiedRAGConfig& config);
    ~UnifiedRAGManager() = default;
    
    // Initialization
    bool initialize(const std::string& general_model_path = "",
                   const std::string& narrative_model_path = "",
                   const std::string& temporal_model_path = "");
    bool isInitialized() const { return initialized_; }
    void shutdown();
    
    // Configuration
    void setConfig(const UnifiedRAGConfig& config) { config_ = config; }
    UnifiedRAGConfig getConfig() const { return config_; }
    
    // Main query interface
    std::vector<RAGResult> query(const std::string& query, int top_k = -1);
    std::vector<RAGResult> queryWithType(const std::string& query, 
                                         RAGQueryType type, 
                                         int top_k = -1);
    
    // Routing inspection
    RoutingDecision analyzeQuery(const std::string& query) const;
    
    // Router access
    RAGRouter* getRouter() { return router_.get(); }
    const RAGRouter* getRouter() const { return router_.get(); }
    
    // Data ingestion (routes to appropriate backend)
    void ingestGeneralDocument(const std::string& content, const std::string& id);
    void ingestNarrativeEvent(const NarrativeEvent& event);
    void ingestTemporalData(const TimeSeriesPoint& point);
    
    // Batch ingestion
    void ingestGeneralDocuments(const std::vector<std::pair<std::string, std::string>>& docs);
    void ingestNarrativeEvents(const std::vector<NarrativeEvent>& events);
    void ingestTemporalData(const std::vector<TimeSeriesPoint>& points);
    
    // Backend access (for specialized operations)
    SemanticSearchManager* getGeneralRAG() { return general_rag_.get(); }
    NarrativeRAGManager* getNarrativeRAG() { return narrative_rag_.get(); }
    TemporalRAGManager* getTemporalRAG() { return temporal_rag_.get(); }
    
    // Statistics
    std::unordered_map<std::string, size_t> getBackendStatistics() const;
    std::vector<std::string> getAvailableBackends() const;
};

} // namespace ai
} // namespace bolt

#endif // UNIFIED_RAG_MANAGER_HPP
