#include "bolt/ai/unified_rag_manager.hpp"
#include <iostream>
#include <iomanip>

void printSeparator() {
    std::cout << "\n" << std::string(80, '=') << "\n\n";
}

void printResults(const std::vector<bolt::ai::RAGResult>& results) {
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }
    
    std::cout << "Found " << results.size() << " results:\n\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << (i + 1) << ". [" << results[i].source << "] "
                  << "(Score: " << std::fixed << std::setprecision(3) << results[i].score << ")\n";
        std::cout << "   " << results[i].content << "\n\n";
    }
}

int main() {
    std::cout << "=== Unified RAG System Demo ===\n";
    std::cout << "Intelligent Query Routing Across Multiple RAG Backends\n";
    printSeparator();
    
    // 1. Initialize the Unified RAG Manager
    std::cout << "Initializing Unified RAG Manager...\n";
    bolt::ai::UnifiedRAGManager rag_system;
    
    if (!rag_system.initialize()) {
        std::cerr << "❌ Failed to initialize Unified RAG Manager\n";
        std::cerr << "Note: Some backends may not be available\n";
    }
    
    auto available_backends = rag_system.getAvailableBackends();
    std::cout << "✅ Initialized with " << available_backends.size() << " backends:\n";
    for (const auto& backend : available_backends) {
        std::cout << "   • " << backend << "\n";
    }
    printSeparator();
    
    // 2. Ingest sample data into different backends
    std::cout << "Ingesting sample data...\n\n";
    
    // Narrative events
    bolt::ai::NarrativeEvent event1;
    event1.id = "event_001";
    event1.description = "The hero discovered an ancient sword in the castle's armory. "
                        "It glowed with a mysterious blue light.";
    event1.characters = {"Hero"};
    event1.locations = {"Castle Armory"};
    event1.timestamp = std::chrono::system_clock::now();
    event1.event_type = "discovery";
    event1.importance = 9;
    rag_system.ingestNarrativeEvent(event1);
    std::cout << "✓ Ingested narrative event: " << event1.description.substr(0, 50) << "...\n";
    
    bolt::ai::NarrativeEvent event2;
    event2.id = "event_002";
    event2.description = "A mysterious stranger warned the hero about the dangers "
                        "that lie ahead in the dark forest.";
    event2.characters = {"Hero", "Stranger"};
    event2.locations = {"Village Tavern"};
    event2.timestamp = std::chrono::system_clock::now();
    event2.event_type = "dialogue";
    event2.importance = 7;
    rag_system.ingestNarrativeEvent(event2);
    std::cout << "✓ Ingested narrative event: " << event2.description.substr(0, 50) << "...\n";
    
    // Temporal data
    bolt::ai::TimeSeriesPoint point1;
    point1.timestamp = std::chrono::system_clock::now() - std::chrono::hours(24);
    point1.values["stock_price"] = 105.50;
    point1.values["volume"] = 1500000;
    point1.source = "market_data";
    point1.category = "stock";
    rag_system.ingestTemporalData(point1);
    std::cout << "✓ Ingested temporal data point (24h ago)\n";
    
    bolt::ai::TimeSeriesPoint point2;
    point2.timestamp = std::chrono::system_clock::now();
    point2.values["stock_price"] = 112.75;
    point2.values["volume"] = 1800000;
    point2.source = "market_data";
    point2.category = "stock";
    rag_system.ingestTemporalData(point2);
    std::cout << "✓ Ingested temporal data point (now)\n";
    
    printSeparator();
    
    // 3. Test query routing with different query types
    std::cout << "=== Query Routing Demonstration ===\n\n";
    
    std::vector<std::string> test_queries = {
        "What happened in the castle?",
        "Show me the stock price trend over time",
        "Tell me about the hero's journey and the market performance",
        "Find information about machine learning algorithms"
    };
    
    for (const auto& query : test_queries) {
        std::cout << "Query: \"" << query << "\"\n";
        
        // Analyze the query
        auto routing = rag_system.analyzeQuery(query);
        std::cout << "  → Routing Decision:\n";
        std::cout << "     Backend: " << routing.backend_name << "\n";
        std::cout << "     Confidence: " << std::fixed << std::setprecision(2) 
                  << (routing.confidence * 100) << "%\n";
        std::cout << "     Reasoning: " << routing.reasoning << "\n\n";
        
        // Execute the query
        auto results = rag_system.query(query, 3);
        printResults(results);
        
        printSeparator();
    }
    
    // 4. Test forced routing (queryWithType)
    std::cout << "=== Forced Routing Test ===\n\n";
    
    std::cout << "Query: \"Tell me everything\"\n";
    std::cout << "Forcing backend: NarrativeRAG\n\n";
    
    auto forced_results = rag_system.queryWithType(
        "Tell me everything", 
        bolt::ai::RAGQueryType::NARRATIVE, 
        5
    );
    printResults(forced_results);
    
    printSeparator();
    
    // 5. Display statistics
    std::cout << "=== System Statistics ===\n\n";
    
    auto stats = rag_system.getBackendStatistics();
    for (const auto& stat : stats) {
        std::cout << stat.first << ": " << stat.second << "\n";
    }
    
    printSeparator();
    std::cout << "=== Demo Complete ===\n";
    std::cout << "\nThis demonstrates:\n";
    std::cout << "  • Intelligent query routing based on content analysis\n";
    std::cout << "  • Seamless integration of multiple specialized RAG backends\n";
    std::cout << "  • Hybrid queries that combine multiple backends\n";
    std::cout << "  • Forced routing for explicit backend selection\n";
    std::cout << "  • Unified interface for all RAG operations\n\n";
    
    return 0;
}
