#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <cstdint>

namespace bolt {
namespace cognitive {

// Forward declarations
class Atom;
class Node;
class Link;
class AtomSpace;

using AtomPtr = std::shared_ptr<Atom>;
using NodePtr = std::shared_ptr<Node>;
using LinkPtr = std::shared_ptr<Link>;
using AtomHandle = uint64_t;

// Truth value for probabilistic reasoning (inspired by OpenCog PLN)
struct TruthValue {
    float strength;     // Probability [0, 1]
    float confidence;   // Confidence in the probability [0, 1]
    
    TruthValue(float s = 1.0f, float c = 1.0f) : strength(s), confidence(c) {}
    
    // PLN-inspired operations
    float expectation() const { return strength * confidence; }
    bool isHighConfidence() const { return confidence > 0.8f; }
};

// Base class for all atoms in the knowledge graph
class Atom {
protected:
    AtomHandle handle_;
    std::string name_;
    TruthValue truth_value_;
    mutable std::mutex mutex_;
    
public:
    Atom(AtomHandle handle, const std::string& name) 
        : handle_(handle), name_(name), truth_value_(1.0f, 1.0f) {}
    
    virtual ~Atom() = default;
    
    AtomHandle getHandle() const { return handle_; }
    std::string getName() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return name_; 
    }
    
    TruthValue getTruthValue() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return truth_value_;
    }
    
    void setTruthValue(const TruthValue& tv) {
        std::lock_guard<std::mutex> lock(mutex_);
        truth_value_ = tv;
    }
    
    virtual std::string toString() const = 0;
    virtual bool isNode() const = 0;
    virtual bool isLink() const = 0;
};

// Nodes represent concepts, entities, and atomic knowledge units
class Node : public Atom {
private:
    std::vector<float> embedding_;  // Tensor embedding for neural operations
    
public:
    Node(AtomHandle handle, const std::string& name) 
        : Atom(handle, name) {}
    
    Node(AtomHandle handle, const std::string& name, const std::vector<float>& embedding)
        : Atom(handle, name), embedding_(embedding) {}
    
    bool hasEmbedding() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !embedding_.empty();
    }
    
    std::vector<float> getEmbedding() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return embedding_;
    }
    
    void setEmbedding(const std::vector<float>& embedding) {
        std::lock_guard<std::mutex> lock(mutex_);
        embedding_ = embedding;
    }
    
    std::string toString() const override {
        return "Node(\"" + name_ + "\")";
    }
    
    bool isNode() const override { return true; }
    bool isLink() const override { return false; }
};

// Links represent relationships between atoms (hypergraph edges)
class Link : public Atom {
private:
    std::vector<AtomPtr> outgoing_;  // Atoms this link connects
    
public:
    Link(AtomHandle handle, const std::string& type, const std::vector<AtomPtr>& outgoing)
        : Atom(handle, type), outgoing_(outgoing) {}
    
    std::vector<AtomPtr> getOutgoing() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return outgoing_;
    }
    
    size_t getArity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return outgoing_.size();
    }
    
    std::string toString() const override {
        std::string result = name_ + "(";
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < outgoing_.size(); ++i) {
            if (i > 0) result += ", ";
            result += outgoing_[i]->getName();
        }
        result += ")";
        return result;
    }
    
    bool isNode() const override { return false; }
    bool isLink() const override { return true; }
};

// AtomSpace: The hypergraph knowledge base (inspired by OpenCog)
class AtomSpace {
private:
    std::unordered_map<AtomHandle, AtomPtr> atoms_;
    std::unordered_map<std::string, std::vector<AtomHandle>> name_index_;
    AtomHandle next_handle_;
    mutable std::mutex mutex_;
    
    AtomHandle generateHandle() {
        return ++next_handle_;
    }
    
public:
    AtomSpace() : next_handle_(0) {}
    
    // Add a node to the knowledge base
    NodePtr addNode(const std::string& name, const std::vector<float>& embedding = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if node already exists
        auto it = name_index_.find(name);
        if (it != name_index_.end() && !it->second.empty()) {
            auto existing = atoms_[it->second[0]];
            if (existing->isNode()) {
                return std::static_pointer_cast<Node>(existing);
            }
        }
        
        AtomHandle handle = generateHandle();
        auto node = std::make_shared<Node>(handle, name, embedding);
        atoms_[handle] = node;
        name_index_[name].push_back(handle);
        
        return node;
    }
    
    // Add a link to the knowledge base
    LinkPtr addLink(const std::string& type, const std::vector<AtomPtr>& outgoing) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        AtomHandle handle = generateHandle();
        auto link = std::make_shared<Link>(handle, type, outgoing);
        atoms_[handle] = link;
        name_index_[type].push_back(handle);
        
        return link;
    }
    
    // Get atom by handle
    AtomPtr getAtom(AtomHandle handle) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = atoms_.find(handle);
        return (it != atoms_.end()) ? it->second : nullptr;
    }
    
    // Find atoms by name
    std::vector<AtomPtr> getAtomsByName(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AtomPtr> result;
        auto it = name_index_.find(name);
        if (it != name_index_.end()) {
            for (auto handle : it->second) {
                result.push_back(atoms_.at(handle));
            }
        }
        return result;
    }
    
    // Get all atoms
    std::vector<AtomPtr> getAllAtoms() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AtomPtr> result;
        for (const auto& pair : atoms_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    // Query similar nodes based on embedding similarity
    std::vector<std::pair<NodePtr, float>> querySimilar(
        const std::vector<float>& query_embedding, 
        size_t k = 5,
        float threshold = 0.0f
    ) const;
    
    // Get total number of atoms
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return atoms_.size();
    }
    
    // Clear the atomspace
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        atoms_.clear();
        name_index_.clear();
        next_handle_ = 0;
    }
};

// Helper functions for creating common link types
inline LinkPtr createInheritanceLink(AtomSpace& space, const AtomPtr& child, const AtomPtr& parent) {
    return space.addLink("InheritanceLink", {child, parent});
}

inline LinkPtr createSimilarityLink(AtomSpace& space, const AtomPtr& a, const AtomPtr& b) {
    return space.addLink("SimilarityLink", {a, b});
}

inline LinkPtr createEvaluationLink(AtomSpace& space, const AtomPtr& predicate, const std::vector<AtomPtr>& args) {
    std::vector<AtomPtr> outgoing = {predicate};
    outgoing.insert(outgoing.end(), args.begin(), args.end());
    return space.addLink("EvaluationLink", outgoing);
}

} // namespace cognitive
} // namespace bolt
