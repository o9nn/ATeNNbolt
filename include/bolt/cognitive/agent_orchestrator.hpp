#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>

namespace bolt {
namespace cognitive {

// Forward declarations
class Agent;
class AgentOrchestrator;

using AgentPtr = std::shared_ptr<Agent>;

// Agent state
enum class AgentState {
    IDLE,
    THINKING,
    ACTING,
    LEARNING,
    COMMUNICATING,
    SUSPENDED,
    TERMINATED
};

// Agent message for inter-agent communication
struct AgentMessage {
    std::string from;
    std::string to;
    std::string type;
    std::string content;
    uint64_t timestamp;
    
    AgentMessage(const std::string& f, const std::string& t, const std::string& typ, const std::string& c)
        : from(f), to(t), type(typ), content(c), timestamp(0) {}
};

// Base Agent class
class Agent {
protected:
    std::string id_;
    std::string name_;
    AgentState state_;
    std::queue<AgentMessage> message_queue_;
    mutable std::mutex mutex_;
    AgentOrchestrator* orchestrator_;
    
public:
    Agent(const std::string& id, const std::string& name)
        : id_(id)
        , name_(name)
        , state_(AgentState::IDLE)
        , orchestrator_(nullptr)
    {}
    
    virtual ~Agent() = default;
    
    std::string getId() const { return id_; }
    std::string getName() const { return name_; }
    
    AgentState getState() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }
    
    void setState(AgentState state) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = state;
    }
    
    void setOrchestrator(AgentOrchestrator* orchestrator) {
        orchestrator_ = orchestrator;
    }
    
    // Receive a message
    void receiveMessage(const AgentMessage& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        message_queue_.push(message);
    }
    
    // Check if there are pending messages
    bool hasMessages() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !message_queue_.empty();
    }
    
    // Get next message
    AgentMessage getNextMessage() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (message_queue_.empty()) {
            throw std::runtime_error("No messages available");
        }
        AgentMessage msg = message_queue_.front();
        message_queue_.pop();
        return msg;
    }
    
    // Main agent loop - to be implemented by subclasses
    virtual void think() = 0;
    virtual void act() = 0;
    virtual void learn() {}
    
    // Process one cycle
    virtual void cycle() {
        setState(AgentState::THINKING);
        think();
        
        setState(AgentState::ACTING);
        act();
        
        setState(AgentState::IDLE);
    }
    
    // Process incoming messages
    virtual void processMessages() {
        while (hasMessages()) {
            AgentMessage msg = getNextMessage();
            onMessage(msg);
        }
    }
    
    // Handle a message - to be overridden by subclasses
    virtual void onMessage(const AgentMessage& message) {}
};

// Agent orchestrator for coordinating multiple agents
class AgentOrchestrator {
private:
    std::vector<AgentPtr> agents_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_;
    std::vector<std::thread> agent_threads_;
    std::condition_variable cv_;
    
    // Orchestration strategy
    enum class Strategy {
        SEQUENTIAL,    // Agents run one after another
        PARALLEL,      // Agents run concurrently
        PRIORITY       // Agents run based on priority
    };
    
    Strategy strategy_;
    
public:
    AgentOrchestrator() 
        : running_(false)
        , strategy_(Strategy::PARALLEL)
    {}
    
    ~AgentOrchestrator() {
        stop();
    }
    
    // Register an agent
    void registerAgent(AgentPtr agent) {
        std::lock_guard<std::mutex> lock(mutex_);
        agent->setOrchestrator(this);
        agents_.push_back(agent);
    }
    
    // Unregister an agent
    void unregisterAgent(const std::string& agent_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        agents_.erase(
            std::remove_if(agents_.begin(), agents_.end(),
                          [&agent_id](const AgentPtr& agent) {
                              return agent->getId() == agent_id;
                          }),
            agents_.end()
        );
    }
    
    // Get agent by ID
    AgentPtr getAgent(const std::string& agent_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& agent : agents_) {
            if (agent->getId() == agent_id) {
                return agent;
            }
        }
        return nullptr;
    }
    
    // Send message from one agent to another
    void sendMessage(const AgentMessage& message) {
        auto recipient = getAgent(message.to);
        if (recipient) {
            recipient->receiveMessage(message);
        }
    }
    
    // Broadcast message to all agents
    void broadcast(const AgentMessage& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& agent : agents_) {
            if (agent->getId() != message.from) {
                agent->receiveMessage(message);
            }
        }
    }
    
    // Start orchestration
    void start() {
        if (running_) return;
        
        running_ = true;
        
        if (strategy_ == Strategy::PARALLEL) {
            startParallel();
        } else if (strategy_ == Strategy::SEQUENTIAL) {
            startSequential();
        }
    }
    
    // Stop orchestration
    void stop() {
        running_ = false;
        cv_.notify_all();
        
        for (auto& thread : agent_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        agent_threads_.clear();
    }
    
    // Run a single orchestration cycle
    void runCycle() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& agent : agents_) {
            if (agent->getState() != AgentState::SUSPENDED &&
                agent->getState() != AgentState::TERMINATED) {
                agent->cycle();
            }
        }
    }
    
    bool isRunning() const { return running_; }
    
    size_t getAgentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return agents_.size();
    }
    
    void setStrategy(Strategy strategy) { strategy_ = strategy; }
    
private:
    void startParallel() {
        for (auto& agent : agents_) {
            agent_threads_.emplace_back([this, agent]() {
                while (running_) {
                    if (agent->getState() != AgentState::SUSPENDED &&
                        agent->getState() != AgentState::TERMINATED) {
                        agent->cycle();
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
        }
    }
    
    void startSequential() {
        agent_threads_.emplace_back([this]() {
            while (running_) {
                runCycle();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
};

// Simple cognitive agent implementation
class CognitiveAgent : public Agent {
private:
    std::vector<std::string> knowledge_;
    std::vector<std::string> goals_;
    std::function<void()> think_callback_;
    std::function<void()> act_callback_;
    
public:
    CognitiveAgent(const std::string& id, const std::string& name)
        : Agent(id, name)
    {}
    
    void setThinkCallback(std::function<void()> callback) {
        think_callback_ = callback;
    }
    
    void setActCallback(std::function<void()> callback) {
        act_callback_ = callback;
    }
    
    void addKnowledge(const std::string& knowledge) {
        std::lock_guard<std::mutex> lock(mutex_);
        knowledge_.push_back(knowledge);
    }
    
    void addGoal(const std::string& goal) {
        std::lock_guard<std::mutex> lock(mutex_);
        goals_.push_back(goal);
    }
    
    void think() override {
        if (think_callback_) {
            think_callback_();
        }
    }
    
    void act() override {
        if (act_callback_) {
            act_callback_();
        }
    }
    
    void onMessage(const AgentMessage& message) override {
        // Simple message handling - add to knowledge
        addKnowledge("Received: " + message.content + " from " + message.from);
    }
};

} // namespace cognitive
} // namespace bolt
