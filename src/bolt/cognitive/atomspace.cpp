#include "bolt/cognitive/atomspace.hpp"
#include <algorithm>
#include <cmath>

namespace bolt {
namespace cognitive {

// Compute cosine similarity between two vectors
static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }
    
    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    float denominator = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denominator < 1e-8f) {
        return 0.0f;
    }
    
    return dot_product / denominator;
}

std::vector<std::pair<NodePtr, float>> AtomSpace::querySimilar(
    const std::vector<float>& query_embedding,
    size_t k,
    float threshold
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Collect all nodes with embeddings and their similarities
    std::vector<std::pair<NodePtr, float>> similarities;
    
    for (const auto& [handle, atom] : atoms_) {
        if (atom->isNode()) {
            auto node = std::static_pointer_cast<Node>(atom);
            if (node->hasEmbedding()) {
                float sim = cosineSimilarity(query_embedding, node->getEmbedding());
                if (sim >= threshold) {
                    similarities.emplace_back(node, sim);
                }
            }
        }
    }
    
    // Sort by similarity (descending)
    std::sort(similarities.begin(), similarities.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second;
             });
    
    // Return top k results
    if (similarities.size() > k) {
        similarities.resize(k);
    }
    
    return similarities;
}

} // namespace cognitive
} // namespace bolt
