#include "bolt/ai/rag_router.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cmath>

namespace bolt {
namespace ai {

RAGRouter::RAGRouter() {
    initializeKeywords();
    initializePatterns();
}

RAGRouter::RAGRouter(const RAGRouterConfig& config) 
    : config_(config) {
    initializeKeywords();
    initializePatterns();
}

void RAGRouter::setConfig(const RAGRouterConfig& config) {
    config_ = config;
}

void RAGRouter::initializeKeywords() {
    // Narrative keywords
    narrative_keywords_ = {
        "story", "narrative", "plot", "character", "protagonist", "antagonist",
        "hero", "villain", "dialogue", "scene", "chapter", "tale", "legend",
        "adventure", "quest", "journey", "epic", "saga", "chronicle",
        "happened", "told", "said", "spoke", "whispered", "shouted",
        "castle", "kingdom", "forest", "dungeon", "tower", "village",
        "sword", "magic", "spell", "potion", "artifact", "treasure",
        "battle", "fight", "encounter", "meeting", "discovery"
    };
    
    // Temporal keywords
    temporal_keywords_ = {
        "trend", "forecast", "predict", "prediction", "time series", "temporal",
        "historical", "history", "pattern", "anomaly", "outlier", "spike",
        "correlation", "causation", "regression", "seasonality",
        "price", "stock", "market", "trading", "volume", "volatility",
        "sensor", "metric", "measurement", "reading", "data point",
        "yesterday", "today", "tomorrow", "last week", "next month",
        "hour", "day", "week", "month", "year", "quarter",
        "increase", "decrease", "rise", "fall", "growth", "decline",
        "over time", "through time", "across time", "time period",
        "from", "to", "between", "during", "since", "until"
    };
    
    // Character-related keywords
    character_keywords_ = {
        "character", "person", "people", "hero", "villain", "protagonist",
        "antagonist", "ally", "enemy", "friend", "companion", "npc"
    };
    
    // Event-related keywords
    event_keywords_ = {
        "event", "happened", "occurred", "took place", "incident",
        "encounter", "meeting", "battle", "discovery", "revelation"
    };
    
    // Metric-related keywords
    metric_keywords_ = {
        "metric", "measurement", "value", "number", "statistic",
        "price", "cost", "rate", "percentage", "ratio"
    };
    
    // Trend-related keywords
    trend_keywords_ = {
        "trend", "pattern", "tendency", "direction", "movement",
        "increase", "decrease", "growth", "decline", "change"
    };
}

void RAGRouter::initializePatterns() {
    // Temporal patterns
    temporal_patterns_ = {
        // Date patterns: YYYY, YYYY-MM-DD, MM/DD/YYYY
        std::regex(R"(\b\d{4}\b)"),
        std::regex(R"(\b\d{4}-\d{2}-\d{2}\b)"),
        std::regex(R"(\b\d{1,2}/\d{1,2}/\d{2,4}\b)"),
        
        // Time patterns: HH:MM, HH:MM:SS
        std::regex(R"(\b\d{1,2}:\d{2}(:\d{2})?\b)"),
        
        // Relative time: "last X", "next X", "past X"
        std::regex(R"(\b(last|next|past|previous|following)\s+(hour|day|week|month|year|quarter)s?\b)", 
                  std::regex::icase),
        
        // Time ranges: "from X to Y", "between X and Y"
        std::regex(R"(\b(from|between)\b.*\b(to|and)\b)", std::regex::icase),
        
        // Temporal quantifiers: "X days ago", "in X hours"
        std::regex(R"(\b\d+\s+(hour|day|week|month|year)s?\s+(ago|later)\b)", 
                  std::regex::icase)
    };
    
    // Narrative patterns
    narrative_patterns_ = {
        // Story-related phrases
        std::regex(R"(\b(tell me|what happened|the story|once upon)\b)", 
                  std::regex::icase),
        
        // Character mentions: "the hero", "a character named"
        std::regex(R"(\b(the|a)\s+(hero|villain|character|protagonist)\b)", 
                  std::regex::icase),
        
        // Location mentions: "in the castle", "at the village"
        std::regex(R"(\b(in|at|near|by)\s+the\s+\w+\b)", std::regex::icase),
        
        // Action phrases: "when X did Y", "after X happened"
        std::regex(R"(\b(when|after|before)\s+\w+\s+(did|happened|occurred)\b)", 
                  std::regex::icase)
    };
}

QueryFeatures RAGRouter::extractFeatures(const std::string& query) const {
    QueryFeatures features;
    
    // Convert to lowercase for case-insensitive matching
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), 
                  lower_query.begin(), ::tolower);
    
    // Count narrative keywords
    for (const auto& keyword : narrative_keywords_) {
        if (lower_query.find(keyword) != std::string::npos) {
            features.narrative_keyword_count++;
        }
    }
    
    // Count temporal keywords
    for (const auto& keyword : temporal_keywords_) {
        if (lower_query.find(keyword) != std::string::npos) {
            features.temporal_keyword_count++;
        }
    }
    
    // Check for temporal patterns
    for (const auto& pattern : temporal_patterns_) {
        if (std::regex_search(query, pattern)) {
            features.has_time_pattern = true;
            break;
        }
    }
    
    // Check for narrative patterns
    for (const auto& pattern : narrative_patterns_) {
        if (std::regex_search(query, pattern)) {
            features.has_narrative_pattern = true;
            break;
        }
    }
    
    // Check for question patterns
    if (query.find('?') != std::string::npos ||
        lower_query.find("what") == 0 ||
        lower_query.find("when") == 0 ||
        lower_query.find("where") == 0 ||
        lower_query.find("who") == 0 ||
        lower_query.find("how") == 0 ||
        lower_query.find("why") == 0) {
        features.has_question_pattern = true;
    }
    
    // Check contextual features
    for (const auto& keyword : character_keywords_) {
        if (lower_query.find(keyword) != std::string::npos) {
            features.mentions_characters = true;
            break;
        }
    }
    
    for (const auto& keyword : event_keywords_) {
        if (lower_query.find(keyword) != std::string::npos) {
            features.mentions_events = true;
            break;
        }
    }
    
    for (const auto& keyword : metric_keywords_) {
        if (lower_query.find(keyword) != std::string::npos) {
            features.mentions_metrics = true;
            break;
        }
    }
    
    for (const auto& keyword : trend_keywords_) {
        if (lower_query.find(keyword) != std::string::npos) {
            features.mentions_trends = true;
            break;
        }
    }
    
    return features;
}

float RAGRouter::calculateNarrativeScore(const QueryFeatures& features) const {
    float score = 0.0f;
    
    // Keyword contribution
    score += features.narrative_keyword_count * config_.keyword_weight;
    
    // Pattern contribution
    if (features.has_narrative_pattern) {
        score += 2.0f * config_.pattern_weight;
    }
    
    // Context contribution
    if (features.mentions_characters) {
        score += 1.5f * config_.context_weight;
    }
    if (features.mentions_events) {
        score += 1.5f * config_.context_weight;
    }
    
    // Normalize to 0-1 range (using sigmoid-like function)
    features.narrative_score = 1.0f / (1.0f + std::exp(-score / 5.0f));
    
    return features.narrative_score;
}

float RAGRouter::calculateTemporalScore(const QueryFeatures& features) const {
    float score = 0.0f;
    
    // Keyword contribution
    score += features.temporal_keyword_count * config_.keyword_weight;
    
    // Pattern contribution (time patterns are strong indicators)
    if (features.has_time_pattern) {
        score += 3.0f * config_.pattern_weight;
    }
    
    // Context contribution
    if (features.mentions_metrics) {
        score += 1.5f * config_.context_weight;
    }
    if (features.mentions_trends) {
        score += 1.5f * config_.context_weight;
    }
    
    // Normalize to 0-1 range
    features.temporal_score = 1.0f / (1.0f + std::exp(-score / 5.0f));
    
    return features.temporal_score;
}

float RAGRouter::calculateGeneralScore(const QueryFeatures& features) const {
    // General score is inversely related to specialized scores
    float specialized_score = std::max(features.narrative_score, features.temporal_score);
    
    // If the query doesn't strongly match any specialized category,
    // it's likely a general query
    features.general_score = 1.0f - specialized_score;
    
    // Boost general score if it's a question without specific context
    if (features.has_question_pattern && 
        features.narrative_keyword_count == 0 && 
        features.temporal_keyword_count == 0) {
        features.general_score = std::min(1.0f, features.general_score + 0.3f);
    }
    
    return features.general_score;
}

RAGQueryType RAGRouter::classifyFromScores(float narrative_score, 
                                           float temporal_score, 
                                           float general_score) const {
    // Check for hybrid queries
    if (narrative_score >= config_.hybrid_threshold && 
        temporal_score >= config_.hybrid_threshold) {
        return RAGQueryType::HYBRID;
    }
    
    // Find the highest score
    float max_score = std::max({narrative_score, temporal_score, general_score});
    
    // Classify based on highest score and thresholds
    if (narrative_score == max_score && narrative_score >= config_.narrative_threshold) {
        return RAGQueryType::NARRATIVE;
    } else if (temporal_score == max_score && temporal_score >= config_.temporal_threshold) {
        return RAGQueryType::TEMPORAL;
    } else {
        // Default to general if no clear winner or below thresholds
        return config_.default_fallback;
    }
}

float RAGRouter::calculateConfidence(const QueryFeatures& features, 
                                     RAGQueryType type) const {
    float confidence = 0.0f;
    
    switch (type) {
        case RAGQueryType::NARRATIVE:
            confidence = features.narrative_score;
            // Boost confidence if multiple indicators present
            if (features.has_narrative_pattern && features.mentions_characters) {
                confidence = std::min(1.0f, confidence + 0.1f);
            }
            break;
            
        case RAGQueryType::TEMPORAL:
            confidence = features.temporal_score;
            // Time patterns are strong indicators
            if (features.has_time_pattern) {
                confidence = std::min(1.0f, confidence + 0.15f);
            }
            break;
            
        case RAGQueryType::HYBRID:
            // Confidence for hybrid is the average of both scores
            confidence = (features.narrative_score + features.temporal_score) / 2.0f;
            break;
            
        case RAGQueryType::GENERAL_SEMANTIC:
            confidence = features.general_score;
            break;
    }
    
    return confidence;
}

std::string RAGRouter::generateReasoning(const QueryFeatures& features, 
                                        RAGQueryType type) const {
    std::ostringstream reasoning;
    
    switch (type) {
        case RAGQueryType::NARRATIVE:
            reasoning << "Query classified as NARRATIVE (score: " 
                     << std::fixed << std::setprecision(2) 
                     << features.narrative_score << "). ";
            reasoning << "Found " << features.narrative_keyword_count 
                     << " narrative keywords. ";
            if (features.has_narrative_pattern) {
                reasoning << "Detected narrative patterns. ";
            }
            if (features.mentions_characters) {
                reasoning << "Mentions characters. ";
            }
            if (features.mentions_events) {
                reasoning << "Mentions events. ";
            }
            break;
            
        case RAGQueryType::TEMPORAL:
            reasoning << "Query classified as TEMPORAL (score: " 
                     << features.temporal_score << "). ";
            reasoning << "Found " << features.temporal_keyword_count 
                     << " temporal keywords. ";
            if (features.has_time_pattern) {
                reasoning << "Detected time patterns. ";
            }
            if (features.mentions_metrics) {
                reasoning << "Mentions metrics. ";
            }
            if (features.mentions_trends) {
                reasoning << "Mentions trends. ";
            }
            break;
            
        case RAGQueryType::HYBRID:
            reasoning << "Query classified as HYBRID. ";
            reasoning << "Narrative score: " << features.narrative_score << ", ";
            reasoning << "Temporal score: " << features.temporal_score << ". ";
            reasoning << "Requires multiple backends.";
            break;
            
        case RAGQueryType::GENERAL_SEMANTIC:
            reasoning << "Query classified as GENERAL (score: " 
                     << features.general_score << "). ";
            reasoning << "No strong specialized indicators found.";
            break;
    }
    
    return reasoning.str();
}

std::vector<std::pair<std::string, float>> RAGRouter::generateAlternatives(
    const QueryFeatures& features,
    RAGQueryType primary_type) const {
    
    std::vector<std::pair<std::string, float>> alternatives;
    
    // Add all backends with their scores (except the primary)
    if (primary_type != RAGQueryType::NARRATIVE && features.narrative_score > 0.2f) {
        alternatives.emplace_back("NarrativeRAG", features.narrative_score);
    }
    
    if (primary_type != RAGQueryType::TEMPORAL && features.temporal_score > 0.2f) {
        alternatives.emplace_back("TemporalRAG", features.temporal_score);
    }
    
    if (primary_type != RAGQueryType::GENERAL_SEMANTIC && features.general_score > 0.2f) {
        alternatives.emplace_back("GeneralRAG", features.general_score);
    }
    
    // Sort by score (descending)
    std::sort(alternatives.begin(), alternatives.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    return alternatives;
}

RoutingDecision RAGRouter::route(const std::string& query) const {
    RoutingDecision decision;
    
    // Extract features
    decision.features = extractFeatures(query);
    
    // Calculate scores
    calculateNarrativeScore(decision.features);
    calculateTemporalScore(decision.features);
    calculateGeneralScore(decision.features);
    
    // Classify query
    decision.query_type = classifyFromScores(
        decision.features.narrative_score,
        decision.features.temporal_score,
        decision.features.general_score
    );
    
    // Set backend name
    decision.backend_name = backendNameFromType(decision.query_type);
    
    // Calculate confidence
    decision.confidence = calculateConfidence(decision.features, decision.query_type);
    
    // Generate reasoning
    decision.reasoning = generateReasoning(decision.features, decision.query_type);
    
    // Generate alternatives
    decision.alternatives = generateAlternatives(decision.features, decision.query_type);
    
    return decision;
}

QueryFeatures RAGRouter::analyzeQuery(const std::string& query) const {
    QueryFeatures features = extractFeatures(query);
    calculateNarrativeScore(features);
    calculateTemporalScore(features);
    calculateGeneralScore(features);
    return features;
}

RAGQueryType RAGRouter::classifyQuery(const std::string& query) const {
    auto features = analyzeQuery(query);
    return classifyFromScores(
        features.narrative_score,
        features.temporal_score,
        features.general_score
    );
}

std::string RAGRouter::queryTypeToString(RAGQueryType type) const {
    switch (type) {
        case RAGQueryType::NARRATIVE: return "NARRATIVE";
        case RAGQueryType::TEMPORAL: return "TEMPORAL";
        case RAGQueryType::HYBRID: return "HYBRID";
        case RAGQueryType::GENERAL_SEMANTIC: return "GENERAL_SEMANTIC";
        default: return "UNKNOWN";
    }
}

std::string RAGRouter::backendNameFromType(RAGQueryType type) const {
    switch (type) {
        case RAGQueryType::NARRATIVE: return "NarrativeRAG";
        case RAGQueryType::TEMPORAL: return "TemporalRAG";
        case RAGQueryType::HYBRID: return "Hybrid (Multiple Backends)";
        case RAGQueryType::GENERAL_SEMANTIC: return "GeneralRAG";
        default: return "Unknown";
    }
}

void RAGRouter::addNarrativeKeyword(const std::string& keyword) {
    narrative_keywords_.push_back(keyword);
}

void RAGRouter::addTemporalKeyword(const std::string& keyword) {
    temporal_keywords_.push_back(keyword);
}

void RAGRouter::clearCustomKeywords() {
    // Re-initialize to default keywords
    initializeKeywords();
}

} // namespace ai
} // namespace bolt
