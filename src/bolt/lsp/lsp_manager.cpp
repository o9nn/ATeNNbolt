#include "bolt/lsp/lsp_client.hpp"
#include <iostream>

namespace bolt {
namespace lsp {

void LSPManager::registerServer(const std::string& language_id, const std::string& server_command) {
    std::lock_guard<std::mutex> lock(mutex_);
    server_commands_[language_id] = server_command;
    std::cout << "Registered LSP server for " << language_id << ": " << server_command << std::endl;
}

LSPClient* LSPManager::getClient(const std::string& language_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = clients_.find(language_id);
    if (it != clients_.end()) {
        return it->second.get();
    }
    
    // Try to create client if server is registered
    auto cmd_it = server_commands_.find(language_id);
    if (cmd_it == server_commands_.end()) {
        std::cerr << "No LSP server registered for language: " << language_id << std::endl;
        return nullptr;
    }
    
    // Create new client
    auto client = std::make_unique<LSPClient>(cmd_it->second);
    auto* client_ptr = client.get();
    clients_[language_id] = std::move(client);
    
    return client_ptr;
}

void LSPManager::initializeAll(const std::string& root_uri) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [language_id, server_command] : server_commands_) {
        if (clients_.find(language_id) == clients_.end()) {
            auto client = std::make_unique<LSPClient>(server_command);
            if (client->initialize(root_uri)) {
                clients_[language_id] = std::move(client);
                std::cout << "Initialized LSP client for " << language_id << std::endl;
            } else {
                std::cerr << "Failed to initialize LSP client for " << language_id << std::endl;
            }
        }
    }
}

void LSPManager::shutdownAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [language_id, client] : clients_) {
        client->shutdown();
        std::cout << "Shutdown LSP client for " << language_id << std::endl;
    }
    
    clients_.clear();
}

} // namespace lsp
} // namespace bolt
