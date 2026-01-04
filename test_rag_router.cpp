#include "bolt/ai/rag_router.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

void printSeparator() {
    std::cout << "\n" << std::string(80, '=') << "\n\n";
}

void printRoutingDecision(const std::string& query, const bolt::ai::RoutingDecision& decision) {
    std::cout << "Query: \"" << query << "\"\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << "Backend: " << decision.backend_name << "\n";
    std::cout << "Query Type: " << static_cast<int>(decision.query_type) << "\n";
    std::cout << "Confidence: " << std::fixed << std::setprecision(2) 
              << (decision.confidence * 100) << "%\n";
    std::cout << "Reasoning: " << decision.reasoning << "\n";
    
    std::cout << "\nFeature Scores:\n";
    std::cout << "  Narrative: " << std::setprecision(3) << decision.features.narrative_score << "\n";
    std::cout << "  Temporal:  " << decision.features.temporal_score << "\n";
    std::cout << "  General:   " << decision.features.general_score << "\n";
    
    std::cout << "\nFeature Details:\n";
    std::cout << "  Narrative keywords: " << decision.features.narrative_keyword_count << "\n";
    std::cout << "  Temporal keywords:  " << decision.features.temporal_keyword_count << "\n";
    std::cout << "  Has time pattern:   " << (decision.features.has_time_pattern ? "Yes" : "No") << "\n";
    std::cout << "  Has narrative pattern: " << (decision.features.has_narrative_pattern ? "Yes" : "No") << "\n";
    std::cout << "  Mentions characters: " << (decision.features.mentions_characters ? "Yes" : "No") << "\n";
    std::cout << "  Mentions events:     " << (decision.features.mentions_events ? "Yes" : "No") << "\n";
    std::cout << "  Mentions metrics:    " << (decision.features.mentions_metrics ? "Yes" : "No") << "\n";
    std::cout << "  Mentions trends:     " << (decision.features.mentions_trends ? "Yes" : "No") << "\n";
    
    if (!decision.alternatives.empty()) {
        std::cout << "\nAlternative Backends:\n";
        for (const auto& alt : decision.alternatives) {
            std::cout << "  " << alt.first << " (score: " 
                     << std::setprecision(3) << alt.second << ")\n";
        }
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "=== RAG Router Test Suite ===\n";
    printSeparator();
    
    // Initialize the router
    bolt::ai::RAGRouter router;
    std::cout << "✅ RAG Router initialized\n";
    printSeparator();
    
    // Test queries
    std::vector<std::string> test_queries = {
        // Narrative queries
        "What happened in the castle yesterday?",
        "Tell me the story of the hero's journey",
        "Who is the protagonist in this adventure?",
        "Describe the battle scene in chapter 3",
        "What did the character say to the villain?",
        
        // Temporal queries
        "Show me the stock price trend from 2023 to 2024",
        "What was the temperature at 10:00 AM?",
        "Predict the market movement for next week",
        "Analyze the sensor data over the last 24 hours",
        "Find anomalies in the time series between January and March",
        
        // Hybrid queries
        "How did the hero's actions affect the market prices over time?",
        "Show me the story events and their timestamps",
        "What happened in the castle and when did the stock price change?",
        
        // General queries
        "What is machine learning?",
        "Explain the concept of neural networks",
        "Find information about Python programming",
        "How does a database work?",
        "What are the benefits of cloud computing?"
    };
    
    std::cout << "=== Test Results ===\n\n";
    
    int narrative_count = 0;
    int temporal_count = 0;
    int hybrid_count = 0;
    int general_count = 0;
    
    for (const auto& query : test_queries) {
        auto decision = router.route(query);
        printRoutingDecision(query, decision);
        
        // Count classifications
        switch (decision.query_type) {
            case bolt::ai::RAGQueryType::NARRATIVE:
                narrative_count++;
                break;
            case bolt::ai::RAGQueryType::TEMPORAL:
                temporal_count++;
                break;
            case bolt::ai::RAGQueryType::HYBRID:
                hybrid_count++;
                break;
            case bolt::ai::RAGQueryType::GENERAL_SEMANTIC:
                general_count++;
                break;
        }
        
        printSeparator();
    }
    
    // Print summary statistics
    std::cout << "=== Classification Summary ===\n\n";
    std::cout << "Total queries: " << test_queries.size() << "\n";
    std::cout << "Narrative:     " << narrative_count << " (" 
              << std::fixed << std::setprecision(1) 
              << (100.0 * narrative_count / test_queries.size()) << "%)\n";
    std::cout << "Temporal:      " << temporal_count << " (" 
              << (100.0 * temporal_count / test_queries.size()) << "%)\n";
    std::cout << "Hybrid:        " << hybrid_count << " (" 
              << (100.0 * hybrid_count / test_queries.size()) << "%)\n";
    std::cout << "General:       " << general_count << " (" 
              << (100.0 * general_count / test_queries.size()) << "%)\n";
    
    printSeparator();
    
    // Test custom keyword addition
    std::cout << "=== Custom Keyword Test ===\n\n";
    
    std::cout << "Adding custom narrative keyword: 'spaceship'\n";
    router.addNarrativeKeyword("spaceship");
    
    std::string custom_query = "The spaceship landed on the alien planet";
    auto custom_decision = router.route(custom_query);
    printRoutingDecision(custom_query, custom_decision);
    
    printSeparator();
    
    // Test configuration
    std::cout << "=== Configuration Test ===\n\n";
    
    bolt::ai::RAGRouterConfig config;
    config.narrative_threshold = 0.6f;  // Higher threshold
    config.temporal_threshold = 0.6f;
    config.hybrid_threshold = 0.5f;
    
    bolt::ai::RAGRouter strict_router(config);
    std::cout << "Created router with stricter thresholds\n";
    std::cout << "  Narrative threshold: " << config.narrative_threshold << "\n";
    std::cout << "  Temporal threshold:  " << config.temporal_threshold << "\n";
    std::cout << "  Hybrid threshold:    " << config.hybrid_threshold << "\n\n";
    
    std::string ambiguous_query = "Tell me about the data";
    std::cout << "Testing with ambiguous query: \"" << ambiguous_query << "\"\n\n";
    
    auto lenient_decision = router.route(ambiguous_query);
    auto strict_decision = strict_router.route(ambiguous_query);
    
    std::cout << "Lenient router: " << lenient_decision.backend_name 
              << " (confidence: " << std::setprecision(2) 
              << (lenient_decision.confidence * 100) << "%)\n";
    std::cout << "Strict router:  " << strict_decision.backend_name 
              << " (confidence: " << (strict_decision.confidence * 100) << "%)\n";
    
    printSeparator();
    std::cout << "=== Test Complete ===\n";
    
    return 0;
}
