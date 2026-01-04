#include "bolt/ai/semantic_search_manager.hpp"
#include <iostream>
#include <iomanip>

void printSearchResults(const std::vector<bolt::ai::SearchResult>& results) {
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }
    
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Search Results (" << results.size() << " found)\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        std::cout << (i + 1) << ". " << result.file_path << "\n";
        std::cout << "   Similarity: " << std::fixed << std::setprecision(4) 
                  << result.similarity_score << "\n";
        std::cout << "   Type: " << result.file_type << "\n";
        
        if (!result.content_preview.empty()) {
            std::cout << "   Preview: ";
            std::string preview = result.content_preview;
            // Replace newlines with spaces for compact display
            std::replace(preview.begin(), preview.end(), '\n', ' ');
            if (preview.length() > 100) {
                preview = preview.substr(0, 100) + "...";
            }
            std::cout << preview << "\n";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "=== Semantic File Search Demo ===\n\n";
    
    // 1. Initialize the semantic search manager
    std::cout << "Initializing semantic search manager...\n";
    bolt::ai::SemanticSearchManager search_manager;
    
    if (!search_manager.initialize()) {
        std::cerr << "❌ Failed to initialize semantic search manager.\n";
        std::cerr << "Make sure a GGUF model is available.\n";
        return 1;
    }
    
    std::cout << "✅ Semantic search manager initialized\n\n";
    
    // 2. Configure indexing
    bolt::ai::IndexingConfig config;
    config.max_chunk_size = 500;
    config.chunk_overlap = 100;
    config.file_extensions = {".cpp", ".hpp", ".h", ".txt", ".md"};
    config.max_file_size = 1024 * 1024; // 1MB
    search_manager.setConfig(config);
    
    // 3. Index some files
    std::cout << "Indexing files...\n";
    
    std::vector<std::string> files_to_index = {
        "./bolt_chat.cpp",
        "./demo_direct_gguf.cpp",
        "./include/bolt/ai/direct_gguf_inference.hpp",
        "./src/bolt/ai/direct_gguf_inference.cpp",
        "./README.md"
    };
    
    // Set progress callback
    search_manager.setProgressCallback(
        [](const std::string& file, size_t current, size_t total) {
            std::cout << "  [" << current << "/" << total << "] " << file << "\n";
        }
    );
    
    for (const auto& file : files_to_index) {
        if (search_manager.indexFile(file)) {
            std::cout << "  ✓ Indexed: " << file << "\n";
        } else {
            std::cout << "  ✗ Skipped: " << file << "\n";
        }
    }
    
    // 4. Display indexing statistics
    auto stats = search_manager.getStats();
    std::cout << "\nIndexing Statistics:\n";
    std::cout << "  Files indexed: " << stats.files_indexed << "\n";
    std::cout << "  Files skipped: " << stats.files_skipped << "\n";
    std::cout << "  Total chunks: " << stats.total_chunks << "\n";
    std::cout << "  Errors: " << stats.errors << "\n";
    std::cout << "  Total time: " << std::fixed << std::setprecision(2) 
              << stats.total_time_seconds << " seconds\n";
    
    // 5. Perform semantic searches
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Performing Semantic Searches\n";
    std::cout << std::string(80, '=') << "\n";
    
    // Search 1: Chat functionality
    std::cout << "\nQuery 1: \"chat interface implementation\"\n";
    auto results1 = search_manager.search("chat interface implementation", 3);
    printSearchResults(results1);
    
    // Search 2: Model loading
    std::cout << "\nQuery 2: \"loading GGUF models\"\n";
    auto results2 = search_manager.search("loading GGUF models", 3);
    printSearchResults(results2);
    
    // Search 3: AI inference
    std::cout << "\nQuery 3: \"AI inference and text generation\"\n";
    auto results3 = search_manager.search("AI inference and text generation", 3);
    printSearchResults(results3);
    
    // 6. Interactive search
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Interactive Search Mode\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << "Enter search queries (or 'quit' to exit):\n\n";
    
    std::string query;
    while (true) {
        std::cout << "Search> ";
        std::getline(std::cin, query);
        
        if (query == "quit" || query == "exit") {
            break;
        }
        
        if (query.empty()) {
            continue;
        }
        
        auto results = search_manager.search(query, 5, 0.2f);
        printSearchResults(results);
    }
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
}
