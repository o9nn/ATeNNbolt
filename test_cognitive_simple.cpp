#include "bolt/cognitive/fusion_reactor.hpp"
#include <iostream>

using namespace bolt::cognitive;

int main() {
    std::cout << "Creating reactor...\n";
    CognitiveFusionReactor reactor(4, 0.01f);
    
    std::cout << "Adding concepts...\n";
    auto cat = reactor.addConcept("cat", {0.8f, 0.2f, 0.9f, 0.1f});
    auto dog = reactor.addConcept("dog", {0.7f, 0.3f, 0.8f, 0.2f});
    
    std::cout << "Querying similar concepts...\n";
    auto similar = reactor.querySimilar({0.8f, 0.2f, 0.9f, 0.1f}, 2);
    for (const auto& [name, sim] : similar) {
        std::cout << "  " << name << ": " << sim << "\n";
    }
    
    std::cout << "Creating agent...\n";
    auto agent = reactor.createAgent("test_agent", "Test Agent");
    
    std::cout << "Setting callbacks...\n";
    agent->setThinkCallback([]() {
        std::cout << "    Agent thinking...\n";
    });
    
    agent->setActCallback([]() {
        std::cout << "    Agent acting...\n";
    });
    
    std::cout << "Running one cycle...\n";
    reactor.runOrchestrationCycle();
    
    std::cout << "Done!\n";
    return 0;
}
