#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <random>
#include <string>
#include <limits>

namespace bolt {
namespace cognitive {

// Tensor embedding system for neural representations
class TensorEmbedding {
private:
    std::vector<float> data_;
    size_t dimensions_;
    
public:
    TensorEmbedding() : dimensions_(0) {}
    
    explicit TensorEmbedding(size_t dims) 
        : data_(dims, 0.0f), dimensions_(dims) {}
    
    TensorEmbedding(const std::vector<float>& data)
        : data_(data), dimensions_(data.size()) {}
    
    // Initialize with random values (Xavier/He initialization)
    static TensorEmbedding random(size_t dims, float scale = 1.0f) {
        TensorEmbedding embedding(dims);
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Xavier initialization: scale / sqrt(dims)
        float std_dev = scale / std::sqrt(static_cast<float>(dims));
        std::normal_distribution<float> dist(0.0f, std_dev);
        
        for (size_t i = 0; i < dims; ++i) {
            embedding.data_[i] = dist(gen);
        }
        
        return embedding;
    }
    
    // Get dimensions
    size_t dimensions() const { return dimensions_; }
    
    // Get raw data
    const std::vector<float>& data() const { return data_; }
    std::vector<float>& data() { return data_; }
    
    // Access individual elements
    float operator[](size_t i) const { return data_[i]; }
    float& operator[](size_t i) { return data_[i]; }
    
    // Compute cosine similarity with another embedding
    float cosineSimilarity(const TensorEmbedding& other) const {
        if (dimensions_ != other.dimensions_ || dimensions_ == 0) {
            return 0.0f;
        }
        
        float dot_product = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        for (size_t i = 0; i < dimensions_; ++i) {
            dot_product += data_[i] * other.data_[i];
            norm_a += data_[i] * data_[i];
            norm_b += other.data_[i] * other.data_[i];
        }
        
        float denominator = std::sqrt(norm_a) * std::sqrt(norm_b);
        if (denominator < 1e-8f) {
            return 0.0f;
        }
        
        return dot_product / denominator;
    }
    
    // Compute Euclidean distance
    float euclideanDistance(const TensorEmbedding& other) const {
        if (dimensions_ != other.dimensions_) {
            return std::numeric_limits<float>::max();
        }
        
        float sum = 0.0f;
        for (size_t i = 0; i < dimensions_; ++i) {
            float diff = data_[i] - other.data_[i];
            sum += diff * diff;
        }
        
        return std::sqrt(sum);
    }
    
    // Normalize to unit length
    void normalize() {
        float norm = 0.0f;
        for (float val : data_) {
            norm += val * val;
        }
        norm = std::sqrt(norm);
        
        if (norm > 1e-8f) {
            for (float& val : data_) {
                val /= norm;
            }
        }
    }
    
    // Get normalized copy
    TensorEmbedding normalized() const {
        TensorEmbedding result = *this;
        result.normalize();
        return result;
    }
    
    // Vector operations
    TensorEmbedding operator+(const TensorEmbedding& other) const {
        if (dimensions_ != other.dimensions_) {
            throw std::runtime_error("Dimension mismatch in embedding addition");
        }
        
        TensorEmbedding result(dimensions_);
        for (size_t i = 0; i < dimensions_; ++i) {
            result.data_[i] = data_[i] + other.data_[i];
        }
        return result;
    }
    
    TensorEmbedding operator-(const TensorEmbedding& other) const {
        if (dimensions_ != other.dimensions_) {
            throw std::runtime_error("Dimension mismatch in embedding subtraction");
        }
        
        TensorEmbedding result(dimensions_);
        for (size_t i = 0; i < dimensions_; ++i) {
            result.data_[i] = data_[i] - other.data_[i];
        }
        return result;
    }
    
    TensorEmbedding operator*(float scalar) const {
        TensorEmbedding result(dimensions_);
        for (size_t i = 0; i < dimensions_; ++i) {
            result.data_[i] = data_[i] * scalar;
        }
        return result;
    }
    
    // Dot product
    float dot(const TensorEmbedding& other) const {
        if (dimensions_ != other.dimensions_) {
            throw std::runtime_error("Dimension mismatch in dot product");
        }
        
        float result = 0.0f;
        for (size_t i = 0; i < dimensions_; ++i) {
            result += data_[i] * other.data_[i];
        }
        return result;
    }
};

// Embedding space for managing collections of embeddings
class EmbeddingSpace {
private:
    size_t dimensions_;
    std::vector<std::pair<std::string, TensorEmbedding>> embeddings_;
    
public:
    explicit EmbeddingSpace(size_t dims) : dimensions_(dims) {}
    
    // Add an embedding to the space
    void addEmbedding(const std::string& id, const TensorEmbedding& embedding) {
        if (embedding.dimensions() != dimensions_) {
            throw std::runtime_error("Embedding dimension mismatch");
        }
        embeddings_.emplace_back(id, embedding);
    }
    
    // Find k nearest neighbors
    std::vector<std::pair<std::string, float>> findNearest(
        const TensorEmbedding& query,
        size_t k,
        float threshold = 0.0f
    ) const {
        if (query.dimensions() != dimensions_) {
            throw std::runtime_error("Query dimension mismatch");
        }
        
        // Compute similarities
        std::vector<std::pair<std::string, float>> similarities;
        for (const auto& [id, embedding] : embeddings_) {
            float sim = query.cosineSimilarity(embedding);
            if (sim >= threshold) {
                similarities.emplace_back(id, sim);
            }
        }
        
        // Sort by similarity (descending)
        std::sort(similarities.begin(), similarities.end(),
                 [](const auto& a, const auto& b) {
                     return a.second > b.second;
                 });
        
        // Return top k
        if (similarities.size() > k) {
            similarities.resize(k);
        }
        
        return similarities;
    }
    
    // Get embedding by ID
    const TensorEmbedding* getEmbedding(const std::string& id) const {
        for (const auto& [emb_id, embedding] : embeddings_) {
            if (emb_id == id) {
                return &embedding;
            }
        }
        return nullptr;
    }
    
    // Get all embeddings
    const std::vector<std::pair<std::string, TensorEmbedding>>& getAllEmbeddings() const {
        return embeddings_;
    }
    
    size_t size() const { return embeddings_.size(); }
    size_t dimensions() const { return dimensions_; }
};

} // namespace cognitive
} // namespace bolt
