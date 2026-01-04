#pragma once

#include "atomspace.hpp"
#include "tensor_embedding.hpp"
#include "neural_learnability.hpp"
#include "agent_orchestrator.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace bolt {
namespace cognitive {

/**
 * @brief Cognitive Fusion Reactor - Integrates knowledge representation, 
 *        neural embeddings, learning, and agent orchestration into a unified
 *        cognitive architecture inspired by relevance realization and 4E cognition.
 * 
 * This is the heart of the cognitive system, combining:
 * - AtomSpace for symbolic knowledge (from ATenSpace)
 * - Tensor embeddings for neural representations
 * - Neural learnability for adaptation
 * - Agent orchestration for multi-agent coordination (from cogzero)
 */
class CognitiveFusionReactor {
public:
    // Attention allocation structure (ECAN-inspired)
    struct AttentionValue {
        float sti;  // Short-term importance
        float lti;  // Long-term importance
        float vlti; // Very long-term importance
    };

private:
    // Core components
    std::unique_ptr<AtomSpace> atomspace_;
    std::unique_ptr<EmbeddingSpace> embedding_space_;
    std::unique_ptr<EmbeddingLearner> learner_;
    std::unique_ptr<AgentOrchestrator> orchestrator_;
    
    // Configuration
    size_t embedding_dim_;
    float learning_rate_;
    
    // Relevance realization - tracks importance/salience
    std::unordered_map<AtomHandle, float> relevance_scores_;
    mutable std::mutex relevance_mutex_;
    
    // Attention allocation (ECAN-inspired)
    std::unordered_map<AtomHandle, AttentionValue> attention_bank_;
    mutable std::mutex attention_mutex_;
    
public:
    /**
     * @brief Construct a new Cognitive Fusion Reactor
     * 
     * @param embedding_dim Dimensionality of neural embeddings
     * @param learning_rate Learning rate for neural adaptation
     */
    CognitiveFusionReactor(size_t embedding_dim = 128, float learning_rate = 0.001f)
        : atomspace_(std::make_unique<AtomSpace>())
        , embedding_space_(std::make_unique<EmbeddingSpace>(embedding_dim))
        , learner_(std::make_unique<EmbeddingLearner>(embedding_dim, learning_rate))
        , orchestrator_(std::make_unique<AgentOrchestrator>())
        , embedding_dim_(embedding_dim)
        , learning_rate_(learning_rate)
    {}
    
    // ========== Knowledge Management ==========
    
    /**
     * @brief Add a concept to the knowledge base with optional embedding
     * 
     * @param name Concept name
     * @param embedding Optional tensor embedding
     * @return NodePtr to the created concept
     */
    NodePtr addConcept(const std::string& name, const std::vector<float>& embedding = {}) {
        auto node = atomspace_->addNode(name, embedding);
        
        // If embedding provided, add to embedding space
        if (!embedding.empty() && embedding.size() == embedding_dim_) {
            embedding_space_->addEmbedding(name, TensorEmbedding(embedding));
        }
        
        // Initialize with neutral relevance
        setRelevance(node->getHandle(), 0.5f);
        
        // Initialize attention values
        setAttention(node->getHandle(), {0.5f, 0.5f, 0.5f});
        
        return node;
    }
    
    /**
     * @brief Create a relationship between concepts
     * 
     * @param type Relationship type (e.g., "InheritanceLink", "SimilarityLink")
     * @param outgoing Atoms involved in the relationship
     * @return LinkPtr to the created relationship
     */
    LinkPtr addRelation(const std::string& type, const std::vector<AtomPtr>& outgoing) {
        auto link = atomspace_->addLink(type, outgoing);
        
        // Initialize with neutral relevance
        setRelevance(link->getHandle(), 0.5f);
        
        return link;
    }
    
    /**
     * @brief Query similar concepts based on semantic similarity
     * 
     * @param query_embedding Query embedding vector
     * @param k Number of results
     * @param threshold Minimum similarity threshold
     * @return Vector of (concept_name, similarity_score) pairs
     */
    std::vector<std::pair<std::string, float>> querySimilar(
        const std::vector<float>& query_embedding,
        size_t k = 5,
        float threshold = 0.0f
    ) {
        TensorEmbedding query(query_embedding);
        return embedding_space_->findNearest(query, k, threshold);
    }
    
    // ========== Relevance Realization ==========
    
    /**
     * @brief Set relevance score for an atom (salience/importance)
     * 
     * @param handle Atom handle
     * @param relevance Relevance score [0, 1]
     */
    void setRelevance(AtomHandle handle, float relevance) {
        std::lock_guard<std::mutex> lock(relevance_mutex_);
        relevance_scores_[handle] = std::clamp(relevance, 0.0f, 1.0f);
    }
    
    /**
     * @brief Get relevance score for an atom
     * 
     * @param handle Atom handle
     * @return float Relevance score [0, 1]
     */
    float getRelevance(AtomHandle handle) const {
        std::lock_guard<std::mutex> lock(relevance_mutex_);
        auto it = relevance_scores_.find(handle);
        return (it != relevance_scores_.end()) ? it->second : 0.5f;
    }
    
    /**
     * @brief Update relevance based on usage and context
     * Implements basic relevance realization dynamics
     * 
     * @param handle Atom handle
     * @param delta Change in relevance
     */
    void updateRelevance(AtomHandle handle, float delta) {
        std::lock_guard<std::mutex> lock(relevance_mutex_);
        auto& relevance = relevance_scores_[handle];
        relevance = std::clamp(relevance + delta, 0.0f, 1.0f);
        
        // Update attention based on relevance change
        if (delta > 0) {
            auto it = attention_bank_.find(handle);
            if (it != attention_bank_.end()) {
                it->second.sti = std::min(1.0f, it->second.sti + delta * 0.5f);
            }
        }
    }
    
    // ========== Attention Allocation (ECAN-inspired) ==========
    
    /**
     * @brief Set attention values for an atom
     * 
     * @param handle Atom handle
     * @param attention Attention values (STI, LTI, VLTI)
     */
    void setAttention(AtomHandle handle, const AttentionValue& attention) {
        std::lock_guard<std::mutex> lock(attention_mutex_);
        attention_bank_[handle] = attention;
    }
    
    /**
     * @brief Get attention values for an atom
     * 
     * @param handle Atom handle
     * @return AttentionValue Current attention values
     */
    AttentionValue getAttention(AtomHandle handle) const {
        std::lock_guard<std::mutex> lock(attention_mutex_);
        auto it = attention_bank_.find(handle);
        return (it != attention_bank_.end()) ? it->second : AttentionValue{0.5f, 0.5f, 0.5f};
    }
    
    /**
     * @brief Spread attention from focused atoms to related atoms
     * Implements importance spreading mechanism
     */
    void spreadAttention() {
        std::lock_guard<std::mutex> lock(attention_mutex_);
        
        // Find high-attention atoms
        std::vector<AtomHandle> high_attention;
        for (const auto& [handle, attention] : attention_bank_) {
            if (attention.sti > 0.7f) {
                high_attention.push_back(handle);
            }
        }
        
        // Spread attention to related atoms (simplified)
        for (auto handle : high_attention) {
            auto atom = atomspace_->getAtom(handle);
            if (atom && atom->isLink()) {
                auto link = std::static_pointer_cast<Link>(atom);
                for (auto& outgoing : link->getOutgoing()) {
                    auto& target_attention = attention_bank_[outgoing->getHandle()];
                    target_attention.sti = std::min(1.0f, target_attention.sti + 0.1f);
                }
            }
        }
    }
    
    // ========== Neural Learning ==========
    
    /**
     * @brief Learn from similarity feedback between concepts
     * 
     * @param concept1 First concept name
     * @param concept2 Second concept name
     * @param target_similarity Desired similarity [0, 1]
     * @return float Learning loss
     */
    float learnSimilarity(const std::string& concept1, const std::string& concept2, float target_similarity) {
        auto emb1 = embedding_space_->getEmbedding(concept1);
        auto emb2 = embedding_space_->getEmbedding(concept2);
        
        if (!emb1 || !emb2) {
            return -1.0f;  // Embedding not found
        }
        
        return learner_->learnFromSimilarity(*emb1, *emb2, target_similarity);
    }
    
    /**
     * @brief Transform an embedding using learned transformations
     * 
     * @param embedding Input embedding
     * @return TensorEmbedding Transformed embedding
     */
    TensorEmbedding transformEmbedding(const TensorEmbedding& embedding) {
        return learner_->transform(embedding);
    }
    
    // ========== Agent Orchestration ==========
    
    /**
     * @brief Register an agent with the orchestrator
     * 
     * @param agent Agent to register
     */
    void registerAgent(AgentPtr agent) {
        orchestrator_->registerAgent(agent);
    }
    
    /**
     * @brief Create and register a simple cognitive agent
     * 
     * @param id Agent ID
     * @param name Agent name
     * @return AgentPtr to the created agent
     */
    std::shared_ptr<CognitiveAgent> createAgent(const std::string& id, const std::string& name) {
        auto agent = std::make_shared<CognitiveAgent>(id, name);
        orchestrator_->registerAgent(agent);
        return agent;
    }
    
    /**
     * @brief Start agent orchestration
     */
    void startOrchestration() {
        orchestrator_->start();
    }
    
    /**
     * @brief Stop agent orchestration
     */
    void stopOrchestration() {
        orchestrator_->stop();
    }
    
    /**
     * @brief Run a single orchestration cycle
     */
    void runOrchestrationCycle() {
        orchestrator_->runCycle();
    }
    
    /**
     * @brief Send a message between agents
     * 
     * @param message Message to send
     */
    void sendMessage(const AgentMessage& message) {
        orchestrator_->sendMessage(message);
    }
    
    // ========== Access to Components ==========
    
    AtomSpace* getAtomSpace() { return atomspace_.get(); }
    EmbeddingSpace* getEmbeddingSpace() { return embedding_space_.get(); }
    AgentOrchestrator* getOrchestrator() { return orchestrator_.get(); }
    
    const AtomSpace* getAtomSpace() const { return atomspace_.get(); }
    const EmbeddingSpace* getEmbeddingSpace() const { return embedding_space_.get(); }
    const AgentOrchestrator* getOrchestrator() const { return orchestrator_.get(); }
    
    size_t getEmbeddingDim() const { return embedding_dim_; }
    float getLearningRate() const { return learning_rate_; }
    
    void setLearningRate(float rate) {
        learning_rate_ = rate;
        learner_->setLearningRate(rate);
    }
    
    // ========== System State ==========
    
    /**
     * @brief Get comprehensive system statistics
     */
    struct SystemStats {
        size_t atom_count;
        size_t embedding_count;
        size_t agent_count;
        bool orchestration_running;
    };
    
    SystemStats getSystemStats() const {
        return {
            atomspace_->size(),
            embedding_space_->size(),
            orchestrator_->getAgentCount(),
            orchestrator_->isRunning()
        };
    }
};

} // namespace cognitive
} // namespace bolt
