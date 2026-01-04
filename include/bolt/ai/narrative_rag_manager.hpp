#ifndef NARRATIVE_RAG_MANAGER_HPP
#define NARRATIVE_RAG_MANAGER_HPP

#include "bolt/ai/vector_database.hpp"
#include "bolt/ai/direct_gguf_inference.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace bolt {
namespace ai {

/**
 * Represents a narrative event in the story world
 */
struct NarrativeEvent {
    std::string id;                     // Unique event ID
    std::string description;            // Event description
    std::vector<std::string> characters; // Characters involved
    std::vector<std::string> locations;  // Locations involved
    std::chrono::system_clock::time_point timestamp; // When it occurred
    std::string event_type;             // Type: action, dialogue, revelation, etc.
    int importance;                     // Importance score (1-10)
    std::vector<std::string> tags;      // Semantic tags
};

/**
 * Configuration for narrative RAG
 */
struct NarrativeRAGConfig {
    int max_retrieved_events = 5;       // Max events to retrieve per query
    float min_relevance_score = 0.3f;   // Minimum relevance threshold
    bool prioritize_recent = true;      // Weight recent events higher
    bool character_aware = true;        // Filter by character involvement
    int recency_window_hours = 24;      // Time window for recency boost
};

/**
 * Manager for narrative-driven RAG in story generation
 * Maintains a dynamic "living world" that evolves with the story
 */
class NarrativeRAGManager {
private:
    std::unique_ptr<DirectGGUFInference> inference_engine_;
    VectorDatabase event_db_;
    NarrativeRAGConfig config_;
    bool initialized_ = false;
    
    // Event tracking
    std::vector<NarrativeEvent> event_history_;
    std::unordered_map<std::string, std::vector<std::string>> character_events_;
    std::unordered_map<std::string, std::vector<std::string>> location_events_;
    
    // Helper methods
    std::vector<float> generateEventEmbedding(const NarrativeEvent& event);
    float calculateRelevanceScore(const SearchResult& result, 
                                   const std::chrono::system_clock::time_point& query_time) const;
    std::string formatEventsForInjection(const std::vector<NarrativeEvent>& events) const;

public:
    NarrativeRAGManager();
    explicit NarrativeRAGManager(const NarrativeRAGConfig& config);
    ~NarrativeRAGManager() = default;
    
    // Initialization
    bool initialize(const std::string& model_path = "");
    bool isInitialized() const { return initialized_; }
    void shutdown();
    
    // Configuration
    void setConfig(const NarrativeRAGConfig& config) { config_ = config; }
    NarrativeRAGConfig getConfig() const { return config_; }
    
    // Event management
    void logEvent(const NarrativeEvent& event);
    std::vector<NarrativeEvent> getEventHistory() const { return event_history_; }
    std::vector<NarrativeEvent> getCharacterEvents(const std::string& character) const;
    std::vector<NarrativeEvent> getLocationEvents(const std::string& location) const;
    
    // Narrative queries
    std::vector<NarrativeEvent> queryNarrativeMemory(
        const std::string& query,
        const std::vector<std::string>& active_characters = {}
    );
    
    std::string generateMemoryInjection(
        const std::string& narrative_context,
        const std::vector<std::string>& active_characters = {}
    );
    
    // Character-specific queries
    std::string getCharacterMemory(
        const std::string& character,
        const std::string& about_what
    );
    
    // World state queries
    std::string getWorldState(const std::string& location);
    std::vector<NarrativeEvent> getRecentEvents(int hours = 24);
    
    // Statistics
    size_t getEventCount() const { return event_history_.size(); }
    std::vector<std::string> getKnownCharacters() const;
    std::vector<std::string> getKnownLocations() const;
};

} // namespace ai
} // namespace bolt

#endif // NARRATIVE_RAG_MANAGER_HPP
