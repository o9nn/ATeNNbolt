/**
 * @file demo_cognitive_fusion_reactor.cpp
 * @brief Demonstration of the Cognitive Fusion Reactor integrating
 *        knowledge representation, neural embeddings, learning, and
 *        agent orchestration.
 * 
 * This example showcases:
 * - AtomSpace knowledge representation (from ATenSpace)
 * - Tensor embeddings for semantic similarity
 * - Neural learnability for adaptation
 * - Multi-agent orchestration (from cogzero)
 * - Relevance realization and attention allocation
 */

#include "bolt/cognitive/fusion_reactor.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace bolt::cognitive;

void printSeparator(const std::string& title = "") {
    std::cout << "\n" << std::string(70, '=') << "\n";
    if (!title.empty()) {
        std::cout << "  " << title << "\n";
        std::cout << std::string(70, '=') << "\n";
    }
}

void demoKnowledgeRepresentation(CognitiveFusionReactor& reactor) {
    printSeparator("Knowledge Representation with Embeddings");
    
    std::cout << "Creating conceptual knowledge base...\n\n";
    
    // Create concepts with simple embeddings
    auto cat = reactor.addConcept("cat", {0.8f, 0.2f, 0.9f, 0.1f});
    auto dog = reactor.addConcept("dog", {0.7f, 0.3f, 0.8f, 0.2f});
    auto fish = reactor.addConcept("fish", {0.2f, 0.8f, 0.3f, 0.9f});
    auto mammal = reactor.addConcept("mammal");
    auto animal = reactor.addConcept("animal");
    
    std::cout << "Created concepts: cat, dog, fish, mammal, animal\n";
    
    // Create relationships (inheritance hierarchy)
    auto cat_is_mammal = createInheritanceLink(*reactor.getAtomSpace(), cat, mammal);
    auto dog_is_mammal = createInheritanceLink(*reactor.getAtomSpace(), dog, mammal);
    auto mammal_is_animal = createInheritanceLink(*reactor.getAtomSpace(), mammal, animal);
    
    std::cout << "Created inheritance links:\n";
    std::cout << "  - " << cat_is_mammal->toString() << "\n";
    std::cout << "  - " << dog_is_mammal->toString() << "\n";
    std::cout << "  - " << mammal_is_animal->toString() << "\n";
    
    // Set truth values for probabilistic reasoning
    cat->setTruthValue({0.95f, 0.9f});  // high confidence that cats exist
    dog->setTruthValue({0.95f, 0.9f});
    fish->setTruthValue({0.90f, 0.85f});
    
    std::cout << "\nTruth values set for probabilistic reasoning\n";
    
    // Query similar concepts
    std::cout << "\nQuerying concepts similar to 'cat' based on embeddings:\n";
    std::vector<float> cat_query = {0.8f, 0.2f, 0.9f, 0.1f};
    auto similar = reactor.querySimilar(cat_query, 3);
    
    for (const auto& [name, similarity] : similar) {
        std::cout << "  - " << name << " (similarity: " 
                  << std::fixed << std::setprecision(3) << similarity << ")\n";
    }
}

void demoRelevanceRealization(CognitiveFusionReactor& reactor) {
    printSeparator("Relevance Realization and Attention");
    
    std::cout << "Setting relevance scores for knowledge atoms...\n\n";
    
    // Get concepts
    auto atoms = reactor.getAtomSpace()->getAtomsByName("cat");
    if (!atoms.empty()) {
        auto cat = atoms[0];
        
        // Simulate relevance changes based on usage
        std::cout << "Initial relevance for 'cat': " 
                  << reactor.getRelevance(cat->getHandle()) << "\n";
        
        reactor.updateRelevance(cat->getHandle(), 0.3f);
        std::cout << "After usage increase: " 
                  << reactor.getRelevance(cat->getHandle()) << "\n";
        
        // Check attention values
        auto attention = reactor.getAttention(cat->getHandle());
        std::cout << "\nAttention values for 'cat':\n";
        std::cout << "  STI (Short-term): " << attention.sti << "\n";
        std::cout << "  LTI (Long-term): " << attention.lti << "\n";
        std::cout << "  VLTI (Very long-term): " << attention.vlti << "\n";
    }
    
    std::cout << "\nSpreading attention through knowledge graph...\n";
    reactor.spreadAttention();
    std::cout << "Attention spread complete\n";
}

void demoNeuralLearning(CognitiveFusionReactor& reactor) {
    printSeparator("Neural Learnability");
    
    std::cout << "Training embeddings based on similarity feedback...\n\n";
    
    // Learn that cat and dog should be more similar
    std::cout << "Learning: 'cat' and 'dog' should be similar (target: 0.9)\n";
    float loss1 = reactor.learnSimilarity("cat", "dog", 0.9f);
    std::cout << "  Loss: " << loss1 << "\n";
    
    // Learn that cat and fish should be less similar
    std::cout << "Learning: 'cat' and 'fish' should be dissimilar (target: 0.2)\n";
    float loss2 = reactor.learnSimilarity("cat", "fish", 0.2f);
    std::cout << "  Loss: " << loss2 << "\n";
    
    // Query again after learning
    std::cout << "\nQuerying similar concepts after learning:\n";
    std::vector<float> cat_query = {0.8f, 0.2f, 0.9f, 0.1f};
    auto similar = reactor.querySimilar(cat_query, 3);
    
    for (const auto& [name, similarity] : similar) {
        std::cout << "  - " << name << " (similarity: " 
                  << std::fixed << std::setprecision(3) << similarity << ")\n";
    }
}

void demoAgentOrchestration(CognitiveFusionReactor& reactor) {
    printSeparator("Agent Orchestration");
    
    std::cout << "Creating cognitive agents...\n\n";
    
    // Create agents
    auto perceiver = reactor.createAgent("perceiver", "Perception Agent");
    auto reasoner = reactor.createAgent("reasoner", "Reasoning Agent");
    auto actor = reactor.createAgent("actor", "Action Agent");
    
    int perceiver_count = 0;
    int reasoner_count = 0;
    int actor_count = 0;
    
    // Configure agent behaviors
    perceiver->setThinkCallback([&]() {
        // Perceiver observes the environment
        perceiver->addKnowledge("Observed: new pattern in data");
        perceiver_count++;
    });
    
    perceiver->setActCallback([&]() {
        // Send message to reasoner
        if (perceiver_count == 1) {  // Only on first cycle
            AgentMessage msg("perceiver", "reasoner", "observation", "New pattern detected");
            reactor.sendMessage(msg);
        }
    });
    
    reasoner->setThinkCallback([&]() {
        // Process messages and reason
        reasoner->addKnowledge("Processing information");
        reasoner_count++;
    });
    
    reasoner->setActCallback([&]() {
        // Send action recommendation
        if (reasoner_count == 1 && reasoner->hasMessages()) {  // Only after receiving message
            reasoner->getNextMessage();  // Process the message
            AgentMessage msg("reasoner", "actor", "command", "Execute plan A");
            reactor.sendMessage(msg);
        }
    });
    
    actor->setThinkCallback([&]() {
        // Process commands
        actor->addKnowledge("Ready for action");
        actor_count++;
    });
    
    actor->setActCallback([&]() {
        // Take action in environment
        if (actor->hasMessages()) {
            actor->getNextMessage();  // Process command
            actor->addKnowledge("Action completed");
        }
    });
    
    std::cout << "Agents created: Perceiver, Reasoner, Actor\n";
    std::cout << "\nRunning 3 orchestration cycles...\n";
    
    // Run a few cycles
    for (int i = 0; i < 3; ++i) {
        std::cout << "\n--- Cycle " << (i + 1) << " ---\n";
        reactor.runOrchestrationCycle();
        std::cout << "  Perceiver: " << perceiver_count << " cycles\n";
        std::cout << "  Reasoner: " << reasoner_count << " cycles\n";
        std::cout << "  Actor: " << actor_count << " cycles\n";
    }
    
    auto stats = reactor.getSystemStats();
    std::cout << "\nAgent orchestration statistics:\n";
    std::cout << "  Active agents: " << stats.agent_count << "\n";
}

void demoSystemIntegration(CognitiveFusionReactor& reactor) {
    printSeparator("Integrated Cognitive System");
    
    auto stats = reactor.getSystemStats();
    
    std::cout << "\nCognitive Fusion Reactor Status:\n";
    std::cout << std::string(40, '-') << "\n";
    std::cout << "  Atoms in knowledge base: " << stats.atom_count << "\n";
    std::cout << "  Neural embeddings: " << stats.embedding_count << "\n";
    std::cout << "  Active agents: " << stats.agent_count << "\n";
    std::cout << "  Orchestration running: " << (stats.orchestration_running ? "Yes" : "No") << "\n";
    std::cout << "  Embedding dimensions: " << reactor.getEmbeddingDim() << "\n";
    std::cout << "  Learning rate: " << reactor.getLearningRate() << "\n";
    
    std::cout << "\n✓ Cognitive Fusion Reactor fully operational!\n";
    std::cout << "\nThis system integrates:\n";
    std::cout << "  • Knowledge representation (AtomSpace from ATenSpace)\n";
    std::cout << "  • Neural tensor embeddings\n";
    std::cout << "  • Neural learnability with gradient descent\n";
    std::cout << "  • Multi-agent orchestration (from cogzero)\n";
    std::cout << "  • Relevance realization (salience tracking)\n";
    std::cout << "  • Attention allocation (ECAN-inspired)\n";
    std::cout << "  • Probabilistic reasoning (truth values)\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         COGNITIVE FUSION REACTOR DEMONSTRATION                     ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║  Integrating knowledge, embeddings, learning & orchestration       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    
    try {
        // Create the cognitive fusion reactor with 4D embeddings for demo
        CognitiveFusionReactor reactor(4, 0.01f);
        
        // Run demonstrations
        demoKnowledgeRepresentation(reactor);
        demoRelevanceRealization(reactor);
        demoNeuralLearning(reactor);
        demoAgentOrchestration(reactor);
        demoSystemIntegration(reactor);
        
        printSeparator();
        std::cout << "\n✨ Demonstration complete! ✨\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n\n";
        return 1;
    }
}
