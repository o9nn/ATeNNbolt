# Cognitive Fusion Reactor

A unified cognitive architecture integrating knowledge representation, neural embeddings, learning capabilities, and multi-agent orchestration. This system combines features from:

- **ATenSpace** (o9nn/ATenSpace) - Tensor-based knowledge representation
- **cogzero** (o9nn/cogzero) - Agent orchestration and cognitive architecture
- **Neural Learnability** - Adaptive learning mechanisms
- **Relevance Realization** - Inspired by John Vervaeke's meaning crisis framework

## 🎯 Overview

The Cognitive Fusion Reactor is the foundational system for building AI that can:

1. **Represent Knowledge** - Using hypergraph-based AtomSpace (inspired by OpenCog)
2. **Learn and Adapt** - Through neural embeddings and gradient-based learning
3. **Coordinate Agents** - Multi-agent orchestration for distributed cognition
4. **Realize Relevance** - Track salience and importance dynamically
5. **Allocate Attention** - ECAN-inspired attention mechanisms

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│              Cognitive Fusion Reactor                        │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐  │
│  │  AtomSpace   │  │  Embedding   │  │     Agent       │  │
│  │ (Knowledge)  │◄─┤    Space     │  │  Orchestrator   │  │
│  └──────────────┘  └──────────────┘  └─────────────────┘  │
│         │                 │                    │            │
│         │                 │                    │            │
│         ▼                 ▼                    ▼            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Relevance Realization Engine                  │  │
│  │  • Salience Tracking   • Attention Allocation        │  │
│  │  • Context Sensitivity • Dynamic Importance          │  │
│  └──────────────────────────────────────────────────────┘  │
│         │                 │                    │            │
│         ▼                 ▼                    ▼            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           Neural Learnability System                  │  │
│  │  • Forward/Backward Propagation                      │  │
│  │  • Embedding Transformation                          │  │
│  │  • Similarity Learning                               │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 📚 Core Components

### 1. AtomSpace - Knowledge Representation

Hypergraph-based knowledge base inspired by OpenCog's AtomSpace:

```cpp
#include "bolt/cognitive/atomspace.hpp"

// Create knowledge base
AtomSpace space;

// Add concepts
auto cat = space.addNode("cat", {0.8f, 0.2f, 0.9f});
auto mammal = space.addNode("mammal");

// Create relationships
auto inheritance = createInheritanceLink(space, cat, mammal);

// Set probabilistic truth values
cat->setTruthValue({0.95f, 0.9f});  // [strength, confidence]
```

**Features:**
- Nodes represent concepts/entities
- Links represent relationships (hypergraph edges)
- Truth values for probabilistic reasoning (PLN-inspired)
- Thread-safe concurrent access
- Tensor embeddings integrated directly

### 2. Tensor Embeddings

Neural representations for semantic similarity:

```cpp
#include "bolt/cognitive/tensor_embedding.hpp"

// Create embedding
TensorEmbedding emb = TensorEmbedding::random(128);

// Compute similarity
float similarity = emb1.cosineSimilarity(emb2);

// Vector operations
TensorEmbedding result = emb1 + emb2 * 0.5f;
result.normalize();

// Embedding space for collections
EmbeddingSpace space(128);
space.addEmbedding("concept1", emb);
auto nearest = space.findNearest(query, k=5);
```

**Features:**
- Cosine similarity and Euclidean distance
- Vector operations (add, subtract, scale)
- Normalization and dot products
- Nearest neighbor search
- Xavier/He initialization

### 3. Neural Learnability

Adaptive learning through neural networks:

```cpp
#include "bolt/cognitive/neural_learnability.hpp"

// Create learnable network
LearnableNetwork network(0.001f);
network.addLayer(128, 256, activation::relu);
network.addLayer(256, 128, activation::tanh);

// Train on examples
float loss = network.train(input, target);

// Embedding learner
EmbeddingLearner learner(128);
float loss = learner.learnFromSimilarity(emb1, emb2, target_similarity);
```

**Features:**
- Multi-layer neural networks
- Forward and backward propagation
- Activation functions (ReLU, sigmoid, tanh)
- Gradient descent optimization
- Embedding transformation learning

### 4. Agent Orchestration

Multi-agent coordination inspired by cogzero:

```cpp
#include "bolt/cognitive/agent_orchestrator.hpp"

// Create orchestrator
AgentOrchestrator orchestrator;

// Create and register agents
auto agent = std::make_shared<CognitiveAgent>("agent1", "Perceiver");
orchestrator.registerAgent(agent);

// Start orchestration
orchestrator.start();  // Parallel execution

// Send messages between agents
AgentMessage msg("agent1", "agent2", "data", "Hello");
orchestrator.sendMessage(msg);
```

**Features:**
- Sequential or parallel agent execution
- Inter-agent communication via messages
- Agent lifecycle management (idle, thinking, acting, learning)
- Cognitive loop: perceive → think → act → learn
- Thread-safe message passing

### 5. Cognitive Fusion Reactor

Unified integration of all components:

```cpp
#include "bolt/cognitive/fusion_reactor.hpp"

// Create the reactor
CognitiveFusionReactor reactor(128, 0.001f);

// Add knowledge with embeddings
auto cat = reactor.addConcept("cat", {0.8f, 0.2f, 0.9f});
auto dog = reactor.addConcept("dog", {0.7f, 0.3f, 0.8f});

// Query similar concepts
auto similar = reactor.querySimilar({0.8f, 0.2f, 0.9f}, k=5);

// Learn from feedback
reactor.learnSimilarity("cat", "dog", 0.9f);

// Manage relevance
reactor.updateRelevance(cat->getHandle(), 0.3f);
reactor.spreadAttention();

// Create and orchestrate agents
auto agent = reactor.createAgent("agent1", "Cognitive Agent");
reactor.startOrchestration();
```

**Features:**
- Unified API for all cognitive functions
- Knowledge + embeddings integration
- Relevance realization and attention allocation
- Agent orchestration built-in
- Learning from similarity feedback
- System statistics and monitoring

## 🚀 Quick Start

### Building

```bash
# Configure with CMake
mkdir build && cd build
cmake ..

# Build
make -j$(nproc)

# Run the demo
./demo_cognitive_fusion_reactor
```

### Simple Example

```cpp
#include "bolt/cognitive/fusion_reactor.hpp"
using namespace bolt::cognitive;

int main() {
    // Create reactor with 128D embeddings
    CognitiveFusionReactor reactor(128);
    
    // Build knowledge base
    auto cat = reactor.addConcept("cat", TensorEmbedding::random(128).data());
    auto dog = reactor.addConcept("dog", TensorEmbedding::random(128).data());
    auto mammal = reactor.addConcept("mammal");
    
    // Create inheritance hierarchy
    reactor.addRelation("InheritanceLink", {cat, mammal});
    reactor.addRelation("InheritanceLink", {dog, mammal});
    
    // Query semantically similar concepts
    auto similar = reactor.querySimilar(cat->getEmbedding(), 5);
    
    // Learn from feedback
    reactor.learnSimilarity("cat", "dog", 0.85f);
    
    // Track relevance
    reactor.updateRelevance(cat->getHandle(), 0.2f);
    
    return 0;
}
```

## 🧠 Cognitive Architecture Principles

### 4E Cognition

The system embodies principles of 4E cognition:

- **Embodied**: Grounded in sensorimotor-like tensor representations
- **Embedded**: Knowledge is contextually situated in the graph
- **Enacted**: Learning through interaction and feedback
- **Extended**: Distributed across multiple agents

### Relevance Realization

Inspired by John Vervaeke's framework for addressing the meaning crisis:

```cpp
// Relevance as dynamic salience
reactor.setRelevance(atom_handle, 0.8f);  // High relevance
reactor.updateRelevance(atom_handle, 0.1f);  // Increase from usage

// Attention allocation (ECAN-inspired)
AttentionValue attention = {
    .sti = 0.8f,   // Short-term importance
    .lti = 0.6f,   // Long-term importance
    .vlti = 0.4f   // Very long-term importance
};
reactor.setAttention(atom_handle, attention);

// Spread attention through knowledge graph
reactor.spreadAttention();
```

### Four Ways of Knowing

The system supports multiple epistemological modes:

1. **Propositional** (knowing-that) - Symbolic knowledge in AtomSpace
2. **Procedural** (knowing-how) - Agent behaviors and skills
3. **Perspectival** (knowing-as) - Context-dependent embeddings
4. **Participatory** (knowing-by-being) - Agent embodiment and learning

## 📊 Performance Characteristics

- **Knowledge Capacity**: Scales to millions of atoms
- **Embedding Operations**: O(d) for d-dimensional embeddings
- **Similarity Search**: O(n) linear scan, O(log n) with indexing
- **Learning**: Configurable learning rate, gradient descent
- **Concurrency**: Thread-safe operations throughout
- **Memory**: ~100 bytes per atom, ~4d bytes per embedding

## 🔬 Advanced Features

### Probabilistic Logic Networks (PLN)

Truth values enable uncertain reasoning:

```cpp
// Set probabilistic truth value
TruthValue tv(0.8f, 0.9f);  // 80% strength, 90% confidence
atom->setTruthValue(tv);

// PLN operations
float expectation = tv.expectation();  // strength * confidence
bool high_conf = tv.isHighConfidence();  // confidence > 0.8
```

### Economic Attention Networks (ECAN)

Attention spreads through the knowledge graph:

```cpp
// High-attention atoms spread importance to connected atoms
reactor.spreadAttention();

// Attention values guide resource allocation
auto attention = reactor.getAttention(handle);
if (attention.sti > 0.7f) {
    // Focus computational resources here
}
```

### Neural Transformation

Learn optimal embedding transformations:

```cpp
// Transform embeddings through learned network
TensorEmbedding transformed = reactor.transformEmbedding(original);

// Train on similarity feedback
reactor.learnSimilarity("concept1", "concept2", target_similarity);
```

## 🎓 Theoretical Foundations

### OpenCog AtomSpace

The knowledge representation follows OpenCog's design:
- Hypergraph structure for complex relationships
- Truth values for probabilistic reasoning
- Attention allocation for resource management
- Plug-in inference mechanisms

### PyTorch ATen Tensors

Embeddings leverage tensor operations:
- Efficient vector computations
- GPU-ready architecture (extensible)
- Standard ML operations (dot, cosine similarity)

### Cognitive Architectures

Inspired by established architectures:
- ACT-R: Cognitive chunks and production rules
- SOAR: Goal-directed reasoning
- CLARION: Dual-process (implicit/explicit) learning
- OpenCog: Integrative cognitive framework

## 📖 Further Reading

- [AtomSpace Documentation](https://wiki.opencog.org/w/AtomSpace)
- [OpenCog PLN](https://wiki.opencog.org/w/Probabilistic_Logic_Networks)
- [4E Cognition](https://en.wikipedia.org/wiki/Embodied_cognition)
- [John Vervaeke - Meaning Crisis](https://www.youtube.com/playlist?list=PLND1JCRq8Vuh3f0P5qjrSdb5eC1ZfZwWJ)
- [Cognitive Architectures Overview](https://en.wikipedia.org/wiki/Cognitive_architecture)

## 🤝 Contributing

This cognitive architecture is designed to be extensible. Key areas for contribution:

- **Pattern Matching**: Variable binding and unification
- **Inference Engines**: Forward/backward chaining
- **Temporal Reasoning**: Time-aware knowledge representation
- **Distributed AtomSpace**: Multi-machine knowledge bases
- **GPU Acceleration**: CUDA/OpenCL for embeddings
- **Python Bindings**: pybind11 integration

## 📄 License

MIT License - See LICENSE file for details

## 🙏 Acknowledgments

- **o9nn/ATenSpace** - Tensor-based AtomSpace implementation
- **o9nn/cogzero** - Agent-Zero C++ cognitive architecture
- **OpenCog Foundation** - Original AtomSpace design
- **John Vervaeke** - Relevance realization framework
- **Cognitive Science Community** - Theoretical foundations

---

*"The meaning crisis requires not just information, but wisdom - systematic optimization of relevance realization."* - Inspired by John Vervaeke
