#include "bolt/ai/semantic_search_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iostream>

namespace bolt {
namespace ai {

SemanticSearchManager::SemanticSearchManager() {
    inference_engine_ = std::make_unique<DirectGGUFInference>();
}

SemanticSearchManager::SemanticSearchManager(const IndexingConfig& config)
    : config_(config) {
    inference_engine_ = std::make_unique<DirectGGUFInference>();
}

bool SemanticSearchManager::initialize(const std::string& model_path) {
    if (initialized_) {
        return true;
    }
    
    std::string actual_model_path = model_path;
    
    // Auto-detect model if not specified
    if (actual_model_path.empty()) {
        std::vector<std::string> search_paths = {
            "./models/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf",
            "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf"
        };
        
        for (const auto& path : search_paths) {
            if (std::filesystem::exists(path)) {
                actual_model_path = path;
                break;
            }
        }
    }
    
    if (actual_model_path.empty()) {
        std::cerr << "No model found for semantic search" << std::endl;
        return false;
    }
    
    initialized_ = inference_engine_->load_model(actual_model_path);
    return initialized_;
}

void SemanticSearchManager::shutdown() {
    initialized_ = false;
    inference_engine_.reset();
    vector_db_.clear();
}

std::vector<std::string> SemanticSearchManager::chunkText(const std::string& text) const {
    std::vector<std::string> chunks;
    
    if (text.length() <= config_.max_chunk_size) {
        chunks.push_back(text);
        return chunks;
    }
    
    size_t pos = 0;
    while (pos < text.length()) {
        size_t chunk_size = std::min(config_.max_chunk_size, text.length() - pos);
        chunks.push_back(text.substr(pos, chunk_size));
        
        // Move position forward, accounting for overlap
        pos += chunk_size - config_.chunk_overlap;
        if (pos >= text.length()) break;
    }
    
    return chunks;
}

bool SemanticSearchManager::shouldIndexFile(const std::string& file_path) const {
    namespace fs = std::filesystem;
    
    // Check if file exists
    if (!fs::exists(file_path)) {
        return false;
    }
    
    // Check if it's a regular file
    if (!fs::is_regular_file(file_path)) {
        return false;
    }
    
    // Check file size
    auto file_size = fs::file_size(file_path);
    if (file_size > config_.max_file_size) {
        return false;
    }
    
    // Check extension
    std::string extension = fs::path(file_path).extension().string();
    if (std::find(config_.file_extensions.begin(), 
                  config_.file_extensions.end(), 
                  extension) == config_.file_extensions.end()) {
        return false;
    }
    
    // Check hidden files
    if (!config_.index_hidden_files) {
        std::string filename = fs::path(file_path).filename().string();
        if (!filename.empty() && filename[0] == '.') {
            return false;
        }
    }
    
    return true;
}

std::string SemanticSearchManager::readFileContent(const std::string& file_path) const {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<float> SemanticSearchManager::generateEmbedding(const std::string& text) {
    if (!initialized_ || !inference_engine_) {
        return {};
    }
    
    // For now, use a simple prompt-based approach
    // In a production system, you'd use a dedicated embedding model
    std::string prompt = "Summarize the following text in a semantic vector: " + text;
    
    // This is a placeholder - the actual implementation would use
    // llama_get_embeddings() from llama.cpp
    // For now, we'll generate a simple hash-based pseudo-embedding
    std::vector<float> embedding(384, 0.0f); // Common embedding dimension
    
    for (size_t i = 0; i < text.length() && i < embedding.size(); ++i) {
        embedding[i % embedding.size()] += static_cast<float>(text[i]) / 255.0f;
    }
    
    // Normalize
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 0.0f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

bool SemanticSearchManager::indexFile(const std::string& file_path) {
    if (!initialized_) {
        return false;
    }
    
    if (!shouldIndexFile(file_path)) {
        stats_.files_skipped++;
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::string content = readFileContent(file_path);
        if (content.empty()) {
            stats_.errors++;
            return false;
        }
        
        // Chunk the content
        auto chunks = chunkText(content);
        
        // Generate embeddings for each chunk
        for (size_t i = 0; i < chunks.size(); ++i) {
            auto embedding = generateEmbedding(chunks[i]);
            
            if (!embedding.empty()) {
                VectorRecord record;
                record.id = file_path + "#chunk" + std::to_string(i);
                record.embedding = embedding;
                record.content_preview = chunks[i].substr(0, 200);
                record.file_type = std::filesystem::path(file_path).extension().string();
                record.file_size = std::filesystem::file_size(file_path);
                record.last_modified = std::filesystem::last_write_time(file_path).time_since_epoch().count();
                
                vector_db_.addOrUpdateRecord(record);
                stats_.total_chunks++;
            }
        }
        
        stats_.files_indexed++;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time);
        stats_.total_time_seconds += duration.count();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error indexing file " << file_path << ": " << e.what() << std::endl;
        stats_.errors++;
        return false;
    }
}

bool SemanticSearchManager::indexDirectory(const std::string& directory_path, bool recursive) {
    if (!initialized_) {
        return false;
    }
    
    indexing_in_progress_ = true;
    namespace fs = std::filesystem;
    
    try {
        auto iterator = recursive 
            ? fs::recursive_directory_iterator(directory_path)
            : fs::directory_iterator(directory_path);
        
        size_t total_files = 0;
        size_t processed = 0;
        
        for (const auto& entry : iterator) {
            if (entry.is_regular_file()) {
                total_files++;
            }
        }
        
        iterator = recursive 
            ? fs::recursive_directory_iterator(directory_path)
            : fs::directory_iterator(directory_path);
        
        for (const auto& entry : iterator) {
            if (entry.is_regular_file()) {
                indexFile(entry.path().string());
                processed++;
                
                if (progress_callback_) {
                    progress_callback_(entry.path().string(), processed, total_files);
                }
            }
        }
        
        indexing_in_progress_ = false;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error indexing directory: " << e.what() << std::endl;
        indexing_in_progress_ = false;
        return false;
    }
}

bool SemanticSearchManager::removeFileFromIndex(const std::string& file_path) {
    // Remove all chunks for this file
    auto all_ids = vector_db_.getAllIds();
    bool removed_any = false;
    
    for (const auto& id : all_ids) {
        if (id.find(file_path) == 0) {
            vector_db_.removeRecord(id);
            removed_any = true;
        }
    }
    
    return removed_any;
}

void SemanticSearchManager::clearIndex() {
    vector_db_.clear();
    stats_ = IndexingStats();
}

std::vector<SearchResult> SemanticSearchManager::search(
    const std::string& query, 
    int top_k,
    float min_similarity) {
    
    if (!initialized_) {
        return {};
    }
    
    auto query_embedding = generateEmbedding(query);
    if (query_embedding.empty()) {
        return {};
    }
    
    return vector_db_.searchSimilar(query_embedding, top_k, min_similarity);
}

std::vector<SearchResult> SemanticSearchManager::searchInDirectory(
    const std::string& query,
    const std::string& directory_path,
    int top_k) {
    
    auto all_results = search(query, top_k * 2); // Get more results to filter
    
    std::vector<SearchResult> filtered_results;
    for (const auto& result : all_results) {
        if (result.file_path.find(directory_path) == 0) {
            filtered_results.push_back(result);
            if (filtered_results.size() >= static_cast<size_t>(top_k)) {
                break;
            }
        }
    }
    
    return filtered_results;
}

size_t SemanticSearchManager::getIndexedFileCount() const {
    return stats_.files_indexed;
}

} // namespace ai
} // namespace bolt
