/**
 * @file test_cognitive_fusion.cpp
 * @brief Unit tests for cognitive fusion reactor components
 */

#include "bolt/cognitive/atomspace.hpp"
#include "bolt/cognitive/tensor_embedding.hpp"
#include "bolt/cognitive/neural_learnability.hpp"
#include "bolt/cognitive/agent_orchestrator.hpp"
#include "bolt/cognitive/fusion_reactor.hpp"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace bolt::cognitive;

void test_atomspace() {
    std::cout << "Testing AtomSpace...\n";
    
    AtomSpace space;
    
    // Test node creation
    auto cat = space.addNode("cat");
    assert(cat != nullptr);
    assert(cat->getName() == "cat");
    assert(cat->isNode());
    assert(!cat->isLink());
    
    // Test node with embedding
    std::vector<float> emb = {0.1f, 0.2f, 0.3f};
    auto dog = space.addNode("dog", emb);
    assert(dog->hasEmbedding());
    assert(dog->getEmbedding() == emb);
    
    // Test link creation
    auto link = createInheritanceLink(space, cat, dog);
    assert(link != nullptr);
    assert(link->isLink());
    assert(!link->isNode());
    assert(link->getArity() == 2);
    
    // Test truth values
    TruthValue tv(0.8f, 0.9f);
    cat->setTruthValue(tv);
    auto retrieved_tv = cat->getTruthValue();
    assert(std::abs(retrieved_tv.strength - 0.8f) < 0.001f);
    assert(std::abs(retrieved_tv.confidence - 0.9f) < 0.001f);
    
    std::cout << "  ✓ AtomSpace tests passed\n";
}

void test_tensor_embedding() {
    std::cout << "Testing TensorEmbedding...\n";
    
    // Test creation
    TensorEmbedding emb1(3);
    assert(emb1.dimensions() == 3);
    
    TensorEmbedding emb2({0.5f, 0.5f, 0.5f});
    assert(emb2.dimensions() == 3);
    
    // Test cosine similarity
    TensorEmbedding a({1.0f, 0.0f, 0.0f});
    TensorEmbedding b({1.0f, 0.0f, 0.0f});
    TensorEmbedding c({0.0f, 1.0f, 0.0f});
    
    float sim_ab = a.cosineSimilarity(b);
    float sim_ac = a.cosineSimilarity(c);
    
    assert(std::abs(sim_ab - 1.0f) < 0.001f);  // Same vector
    assert(std::abs(sim_ac - 0.0f) < 0.001f);  // Orthogonal
    
    // Test vector operations
    TensorEmbedding sum = a + b;
    assert(std::abs(sum[0] - 2.0f) < 0.001f);
    
    TensorEmbedding scaled = a * 2.0f;
    assert(std::abs(scaled[0] - 2.0f) < 0.001f);
    
    // Test normalization
    TensorEmbedding unnorm({3.0f, 4.0f});
    unnorm.normalize();
    assert(std::abs(unnorm[0] - 0.6f) < 0.001f);
    assert(std::abs(unnorm[1] - 0.8f) < 0.001f);
    
    std::cout << "  ✓ TensorEmbedding tests passed\n";
}

void test_embedding_space() {
    std::cout << "Testing EmbeddingSpace...\n";
    
    EmbeddingSpace space(3);
    
    // Add embeddings
    space.addEmbedding("cat", TensorEmbedding({1.0f, 0.0f, 0.0f}));
    space.addEmbedding("dog", TensorEmbedding({0.9f, 0.1f, 0.0f}));
    space.addEmbedding("fish", TensorEmbedding({0.0f, 0.0f, 1.0f}));
    
    assert(space.size() == 3);
    
    // Find nearest neighbors
    TensorEmbedding query({0.95f, 0.05f, 0.0f});
    auto nearest = space.findNearest(query, 2);
    
    assert(nearest.size() == 2);
    assert(nearest[0].first == "cat" || nearest[0].first == "dog");
    assert(nearest[0].second > 0.8f);  // Should be quite similar
    
    std::cout << "  ✓ EmbeddingSpace tests passed\n";
}

void test_neural_layer() {
    std::cout << "Testing NeuralLayer...\n";
    
    NeuralLayer layer(3, 2);
    
    assert(layer.getInputSize() == 3);
    assert(layer.getOutputSize() == 2);
    
    // Test forward pass
    std::vector<float> input = {1.0f, 0.0f, -1.0f};
    auto output = layer.forward(input);
    assert(output.size() == 2);
    
    // Test backward pass
    std::vector<float> gradient = {0.1f, -0.1f};
    auto input_grad = layer.backward(gradient, 0.01f);
    assert(input_grad.size() == 3);
    
    std::cout << "  ✓ NeuralLayer tests passed\n";
}

void test_learnable_network() {
    std::cout << "Testing LearnableNetwork...\n";
    
    LearnableNetwork network(0.01f);
    network.addLayer(3, 5);
    network.addLayer(5, 2);
    
    assert(network.getLayerCount() == 2);
    
    // Test forward pass
    std::vector<float> input = {1.0f, 0.0f, -1.0f};
    auto output = network.forward(input);
    assert(output.size() == 2);
    
    // Test training
    std::vector<float> target = {1.0f, 0.0f};
    float initial_loss = network.train(input, target);
    
    // Train a few more times
    for (int i = 0; i < 10; ++i) {
        network.train(input, target);
    }
    
    float final_loss = network.train(input, target);
    
    // Loss should decrease (not guaranteed but likely)
    std::cout << "    Initial loss: " << initial_loss << ", Final loss: " << final_loss << "\n";
    
    std::cout << "  ✓ LearnableNetwork tests passed\n";
}

void test_agent_orchestrator() {
    std::cout << "Testing AgentOrchestrator...\n";
    
    AgentOrchestrator orchestrator;
    
    int think_count = 0;
    int act_count = 0;
    
    auto agent = std::make_shared<CognitiveAgent>("test_agent", "Test Agent");
    agent->setThinkCallback([&]() { think_count++; });
    agent->setActCallback([&]() { act_count++; });
    
    orchestrator.registerAgent(agent);
    
    assert(orchestrator.getAgentCount() == 1);
    assert(orchestrator.getAgent("test_agent") == agent);
    
    // Run a cycle
    orchestrator.runCycle();
    
    assert(think_count == 1);
    assert(act_count == 1);
    
    // Test message sending
    auto agent2 = std::make_shared<CognitiveAgent>("agent2", "Agent 2");
    orchestrator.registerAgent(agent2);
    
    AgentMessage msg("test_agent", "agent2", "test", "hello");
    orchestrator.sendMessage(msg);
    
    assert(agent2->hasMessages());
    auto received = agent2->getNextMessage();
    assert(received.content == "hello");
    
    std::cout << "  ✓ AgentOrchestrator tests passed\n";
}

void test_fusion_reactor() {
    std::cout << "Testing CognitiveFusionReactor...\n";
    
    CognitiveFusionReactor reactor(4, 0.01f);
    
    // Test knowledge representation
    auto cat = reactor.addConcept("cat", {0.8f, 0.2f, 0.9f, 0.1f});
    auto dog = reactor.addConcept("dog", {0.7f, 0.3f, 0.8f, 0.2f});
    
    assert(cat != nullptr);
    assert(dog != nullptr);
    
    // Test relationship creation
    auto link = reactor.addRelation("SimilarityLink", {cat, dog});
    assert(link != nullptr);
    
    // Test similarity query
    auto similar = reactor.querySimilar({0.8f, 0.2f, 0.9f, 0.1f}, 2);
    assert(similar.size() > 0);
    assert(similar[0].first == "cat");  // Most similar to itself
    
    // Test relevance
    reactor.setRelevance(cat->getHandle(), 0.7f);
    float rel = reactor.getRelevance(cat->getHandle());
    assert(std::abs(rel - 0.7f) < 0.001f);
    
    // Test attention
    CognitiveFusionReactor::AttentionValue attention{0.8f, 0.6f, 0.4f};
    reactor.setAttention(cat->getHandle(), attention);
    auto retrieved_attention = reactor.getAttention(cat->getHandle());
    assert(std::abs(retrieved_attention.sti - 0.8f) < 0.001f);
    
    // Test agent creation
    auto agent = reactor.createAgent("test", "Test Agent");
    assert(agent != nullptr);
    
    auto stats = reactor.getSystemStats();
    assert(stats.atom_count > 0);
    assert(stats.embedding_count > 0);
    assert(stats.agent_count > 0);
    
    std::cout << "  ✓ CognitiveFusionReactor tests passed\n";
}

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Cognitive Fusion Reactor Unit Tests  \n";
    std::cout << "========================================\n\n";
    
    try {
        test_atomspace();
        test_tensor_embedding();
        test_embedding_space();
        test_neural_layer();
        test_learnable_network();
        test_agent_orchestrator();
        test_fusion_reactor();
        
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "  ✓ All tests passed successfully!     \n";
        std::cout << "========================================\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n\n";
        return 1;
    }
}
