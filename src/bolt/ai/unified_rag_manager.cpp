#include "bolt/ai/unified_rag_manager.hpp"
#include <algorithm>
#include <regex>
#include <iostream>

namespace bolt {
namespace ai {

UnifiedRAGManager::UnifiedRAGManager() {
    general_rag_ = std::make_unique<SemanticSearchManager>();
    narrative_rag_ = std::make_unique<NarrativeRAGManager>();
    temporal_rag_ = std::make_unique<TemporalRAGManager>();
    router_ = std::make_unique<RAGRouter>();
}

UnifiedRAGManager::UnifiedRAGManager(const UnifiedRAGConfig& config)
    : config_(config) {
    general_rag_ = std::make_unique<SemanticSearchManager>();
    narrative_rag_ = std::make_unique<NarrativeRAGManager>();
    temporal_rag_ = std::make_unique<TemporalRAGManager>();
    router_ = std::make_unique<RAGRouter>();
}

bool UnifiedRAGManager::initialize(const std::string& general_model_path,
                                   const std::string& narrative_model_path,
                                   const std::string& temporal_model_path) {
    bool success = true;
    
    if (config_.enable_general_rag) {
        if (!general_rag_->initialize(general_model_path)) {
            std::cerr << "Warning: Failed to initialize general RAG backend\n";
            success = false;
        }
    }
    
    if (config_.enable_narrative_rag) {
        if (!narrative_rag_->initialize(narrative_model_path)) {
            std::cerr << "Warning: Failed to initialize narrative RAG backend\n";
            success = false;
        }
    }
    
    if (config_.enable_temporal_rag) {
        if (!temporal_rag_->initialize(temporal_model_path)) {
            std::cerr << "Warning: Failed to initialize temporal RAG backend\n";
            success = false;
        }
    }
    
    initialized_ = success;
    return success;
}

void UnifiedRAGManager::shutdown() {
    if (general_rag_) general_rag_->shutdown();
    if (narrative_rag_) narrative_rag_->shutdown();
    if (temporal_rag_) temporal_rag_->shutdown();
    initialized_ = false;
}

RoutingDecision UnifiedRAGManager::analyzeQuery(const std::string& query) const {
    return router_->route(query);
}

// Legacy method - now uses router
RAGQueryType UnifiedRAGManager::classifyQuery(const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    // Narrative indicators
    std::vector<std::string> narrative_keywords = {
        "story", "character", "happened", "event", "narrative", "plot",
        "dialogue", "scene", "chapter", "protagonist", "antagonist",
        "castle", "adventure", "quest", "journey", "remember", "told"
    };
    
    // Temporal indicators
    std::vector<std::string> temporal_keywords = {
        "trend", "forecast", "predict", "time series", "over time",
        "historical", "pattern", "anomaly", "correlation", "price",
        "stock", "market", "sensor", "metric", "yesterday", "last week",
        "next", "future", "past", "hour", "day", "month", "year"
    };
    
    int narrative_score = 0;
    int temporal_score = 0;
    
    // Count keyword matches
    for (const auto& keyword : narrative_keywords) {
        if (lower_query.find(keyword) != std::string::npos) {
            narrative_score++;
        }
    }
    
    for (const auto& keyword : temporal_keywords) {
        if (lower_query.find(keyword) != std::string::npos) {
            temporal_score++;
        }
    }
    
    // Check for time-related patterns (e.g., "in 2023", "from 10:00 to 12:00")
    std::regex time_pattern(R"(\b\d{4}\b|\b\d{1,2}:\d{2}\b)");
    if (std::regex_search(lower_query, time_pattern)) {
        temporal_score += 2;
    }
    
    // Determine query type
    if (narrative_score > 0 && temporal_score > 0) {
        return RAGQueryType::HYBRID;
    } else if (narrative_score > temporal_score && narrative_score >= 1) {
        return RAGQueryType::NARRATIVE;
    } else if (temporal_score > narrative_score && temporal_score >= 1) {
        return RAGQueryType::TEMPORAL;
    } else {
        return RAGQueryType::GENERAL_SEMANTIC;
    }
}

RoutingDecision UnifiedRAGManager::routeQuery(const std::string& query) const {
    // Delegate to the RAG Router
    return router_->route(query);
}

std::vector<RAGResult> UnifiedRAGManager::query(const std::string& query, int top_k) {
    if (!initialized_) {
        std::cerr << "Error: UnifiedRAGManager not initialized\n";
        return {};
    }
    
    if (top_k < 0) {
        top_k = config_.default_top_k;
    }
    
    auto routing = routeQuery(query);
    
    switch (routing.query_type) {
        case RAGQueryType::NARRATIVE:
            if (config_.enable_narrative_rag && narrative_rag_->isInitialized()) {
                auto events = narrative_rag_->queryNarrativeMemory(query);
                std::vector<RAGResult> results;
                for (const auto& event : events) {
                    RAGResult result;
                    result.id = event.id;
                    result.content = event.description;
                    result.score = static_cast<float>(event.importance) / 10.0f;
                    result.source = "NarrativeRAG";
                    results.push_back(result);
                    if (results.size() >= static_cast<size_t>(top_k)) break;
                }
                return results;
            }
            break;
            
        case RAGQueryType::TEMPORAL:
            if (config_.enable_temporal_rag && temporal_rag_->isInitialized()) {
                // For temporal queries, we'd need to parse time ranges
                // This is a simplified implementation
                std::vector<RAGResult> results;
                RAGResult result;
                result.id = "temporal_analysis";
                result.content = "Temporal analysis would be performed here";
                result.score = 0.9f;
                result.source = "TemporalRAG";
                results.push_back(result);
                return results;
            }
            break;
            
        case RAGQueryType::HYBRID:
            if (config_.enable_hybrid_queries) {
                return processHybridQuery(query, top_k);
            }
            break;
            
        default:
            if (config_.enable_general_rag && general_rag_->isInitialized()) {
                auto search_results = general_rag_->search(query, top_k, config_.confidence_threshold);
                std::vector<RAGResult> results;
                for (const auto& sr : search_results) {
                    RAGResult result;
                    result.id = sr.file_path;
                    result.content = sr.content_preview;
                    result.score = sr.similarity_score;
                    result.source = "GeneralRAG";
                    results.push_back(result);
                }
                return results;
            }
            break;
    }
    
    return {};
}

std::vector<RAGResult> UnifiedRAGManager::queryWithType(const std::string& query, 
                                                        RAGQueryType type, 
                                                        int top_k) {
    if (top_k < 0) {
        top_k = config_.default_top_k;
    }
    
    // Force a specific backend regardless of query classification
    switch (type) {
        case RAGQueryType::NARRATIVE:
            if (narrative_rag_->isInitialized()) {
                auto events = narrative_rag_->queryNarrativeMemory(query);
                std::vector<RAGResult> results;
                for (const auto& event : events) {
                    RAGResult result;
                    result.id = event.id;
                    result.content = event.description;
                    result.score = static_cast<float>(event.importance) / 10.0f;
                    result.source = "NarrativeRAG";
                    results.push_back(result);
                    if (results.size() >= static_cast<size_t>(top_k)) break;
                }
                return results;
            }
            break;
            
        case RAGQueryType::TEMPORAL:
            // Temporal query implementation
            break;
            
        case RAGQueryType::GENERAL_SEMANTIC:
            if (general_rag_->isInitialized()) {
                auto search_results = general_rag_->search(query, top_k, config_.confidence_threshold);
                std::vector<RAGResult> results;
                for (const auto& sr : search_results) {
                    RAGResult result;
                    result.id = sr.file_path;
                    result.content = sr.content_preview;
                    result.score = sr.similarity_score;
                    result.source = "GeneralRAG";
                    results.push_back(result);
                }
                return results;
            }
            break;
            
        default:
            break;
    }
    
    return {};
}

std::vector<RAGResult> UnifiedRAGManager::processHybridQuery(const std::string& query, int top_k) {
    std::vector<RAGResult> all_results;
    
    // Query both narrative and temporal backends
    if (narrative_rag_->isInitialized()) {
        auto narrative_results = queryWithType(query, RAGQueryType::NARRATIVE, top_k / 2);
        all_results.insert(all_results.end(), narrative_results.begin(), narrative_results.end());
    }
    
    if (temporal_rag_->isInitialized()) {
        auto temporal_results = queryWithType(query, RAGQueryType::TEMPORAL, top_k / 2);
        all_results.insert(all_results.end(), temporal_results.begin(), temporal_results.end());
    }
    
    // Sort by score
    std::sort(all_results.begin(), all_results.end(), 
        [](const RAGResult& a, const RAGResult& b) {
            return a.score > b.score;
        });
    
    // Trim to top_k
    if (all_results.size() > static_cast<size_t>(top_k)) {
        all_results.resize(top_k);
    }
    
    return all_results;
}

RoutingDecision UnifiedRAGManager::analyzeQuery(const std::string& query) const {
    return routeQuery(query);
}

void UnifiedRAGManager::ingestGeneralDocument(const std::string& content, const std::string& id) {
    if (general_rag_ && general_rag_->isInitialized()) {
        general_rag_->indexFile(id); // Assuming id is a file path
    }
}

void UnifiedRAGManager::ingestNarrativeEvent(const NarrativeEvent& event) {
    if (narrative_rag_ && narrative_rag_->isInitialized()) {
        narrative_rag_->logEvent(event);
    }
}

void UnifiedRAGManager::ingestTemporalData(const TimeSeriesPoint& point) {
    if (temporal_rag_ && temporal_rag_->isInitialized()) {
        temporal_rag_->ingestDataPoint(point);
    }
}

std::unordered_map<std::string, size_t> UnifiedRAGManager::getBackendStatistics() const {
    std::unordered_map<std::string, size_t> stats;
    
    if (general_rag_ && general_rag_->isInitialized()) {
        stats["GeneralRAG_files"] = general_rag_->getIndexedFileCount();
    }
    
    if (narrative_rag_ && narrative_rag_->isInitialized()) {
        stats["NarrativeRAG_events"] = narrative_rag_->getEventCount();
    }
    
    if (temporal_rag_ && temporal_rag_->isInitialized()) {
        stats["TemporalRAG_points"] = temporal_rag_->getDataPointCount();
    }
    
    return stats;
}

std::vector<std::string> UnifiedRAGManager::getAvailableBackends() const {
    std::vector<std::string> backends;
    
    if (general_rag_ && general_rag_->isInitialized()) {
        backends.push_back("GeneralRAG");
    }
    if (narrative_rag_ && narrative_rag_->isInitialized()) {
        backends.push_back("NarrativeRAG");
    }
    if (temporal_rag_ && temporal_rag_->isInitialized()) {
        backends.push_back("TemporalRAG");
    }
    
    return backends;
}

} // namespace ai
} // namespace bolt
