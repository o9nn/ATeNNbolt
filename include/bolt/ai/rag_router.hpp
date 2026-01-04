#ifndef RAG_ROUTER_HPP
#define RAG_ROUTER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <memory>

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
 * Feature scores for query classification
 */
struct QueryFeatures {
    float narrative_score = 0.0f;
    float temporal_score = 0.0f;
    float general_score = 0.0f;
    
    // Detailed feature breakdown
    int narrative_keyword_count = 0;
    int temporal_keyword_count = 0;
    bool has_time_pattern = false;
    bool has_narrative_pattern = false;
    bool has_question_pattern = false;
    
    // Contextual features
    bool mentions_characters = false;
    bool mentions_events = false;
    bool mentions_metrics = false;
    bool mentions_trends = false;
};

/**
 * Routing decision with detailed reasoning
 */
struct RoutingDecision {
    RAGQueryType query_type;
    std::string backend_name;
    float confidence;
    std::string reasoning;
    QueryFeatures features;
    
    // Alternative backends (for fallback)
    std::vector<std::pair<std::string, float>> alternatives;
};

/**
 * Configuration for the RAG Router
 */
struct RAGRouterConfig {
    // Thresholds
    float narrative_threshold = 0.4f;
    float temporal_threshold = 0.4f;
    float hybrid_threshold = 0.3f;
    float confidence_threshold = 0.5f;
    
    // Weights for different features
    float keyword_weight = 1.0f;
    float pattern_weight = 1.5f;
    float context_weight = 1.2f;
    
    // Fallback behavior
    bool enable_fallback = true;
    RAGQueryType default_fallback = RAGQueryType::GENERAL_SEMANTIC;
};

/**
 * RAG Router: Intelligent query classification and routing
 * 
 * This component analyzes incoming queries and determines which RAG backend
 * (Narrative, Temporal, or General) is most appropriate for handling the query.
 * 
 * Features:
 * - Keyword-based classification
 * - Pattern matching (regex)
 * - Confidence scoring
 * - Hybrid query detection
 * - Fallback routing
 */
class RAGRouter {
private:
    RAGRouterConfig config_;
    
    // Keyword dictionaries
    std::vector<std::string> narrative_keywords_;
    std::vector<std::string> temporal_keywords_;
    std::vector<std::string> character_keywords_;
    std::vector<std::string> event_keywords_;
    std::vector<std::string> metric_keywords_;
    std::vector<std::string> trend_keywords_;
    
    // Pattern matchers
    std::vector<std::regex> temporal_patterns_;
    std::vector<std::regex> narrative_patterns_;
    
    // Helper methods
    void initializeKeywords();
    void initializePatterns();
    
    QueryFeatures extractFeatures(const std::string& query) const;
    float calculateNarrativeScore(const QueryFeatures& features) const;
    float calculateTemporalScore(const QueryFeatures& features) const;
    float calculateGeneralScore(const QueryFeatures& features) const;
    
    RAGQueryType classifyFromScores(float narrative_score, 
                                    float temporal_score, 
                                    float general_score) const;
    
    float calculateConfidence(const QueryFeatures& features, 
                             RAGQueryType type) const;
    
    std::string generateReasoning(const QueryFeatures& features, 
                                 RAGQueryType type) const;
    
    std::vector<std::pair<std::string, float>> generateAlternatives(
        const QueryFeatures& features,
        RAGQueryType primary_type) const;

public:
    RAGRouter();
    explicit RAGRouter(const RAGRouterConfig& config);
    ~RAGRouter() = default;
    
    // Configuration
    void setConfig(const RAGRouterConfig& config);
    RAGRouterConfig getConfig() const { return config_; }
    
    // Main routing method
    RoutingDecision route(const std::string& query) const;
    
    // Query analysis
    QueryFeatures analyzeQuery(const std::string& query) const;
    RAGQueryType classifyQuery(const std::string& query) const;
    
    // Utilities
    std::string queryTypeToString(RAGQueryType type) const;
    std::string backendNameFromType(RAGQueryType type) const;
    
    // Keyword management (for customization)
    void addNarrativeKeyword(const std::string& keyword);
    void addTemporalKeyword(const std::string& keyword);
    void clearCustomKeywords();
};

} // namespace ai
} // namespace bolt

#endif // RAG_ROUTER_HPP
