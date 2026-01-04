
#ifndef BOLT_COMMANDS_HPP
#define BOLT_COMMANDS_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include "bolt/utils/filesystem_utils.hpp"

namespace bolt {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual std::string getDescription() const = 0;
};

class CommandProcessor {
public:
    static CommandProcessor& getInstance() {
        static CommandProcessor instance;
        return instance;
    }

    void registerCommand(const std::string& name, std::unique_ptr<Command> command) {
        commands_[name] = std::move(command);
    }

    bool executeCommand(const std::string& name) {
        auto it = commands_.find(name);
        if (it != commands_.end()) {
            it->second->execute();
            return true;
        }
        return false;
    }

    std::string getCommandDescription(const std::string& name) const {
        auto it = commands_.find(name);
        return it != commands_.end() ? it->second->getDescription() : "Command not found";
    }

private:
    CommandProcessor() = default;
    std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
};

} // namespace bolt

#endif
