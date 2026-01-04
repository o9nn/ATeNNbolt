#ifndef BOLT_COMMANDS_HPP
#define BOLT_COMMANDS_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include "filesystem_utils.hpp"

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

namespace bolt {

// Concrete command implementations
class CreateFileCommand : public Command {
public:
    CreateFileCommand(const std::string& path, const std::string& content)
        : path_(path), content_(content) {}
    
    void execute() override {
        FileSystemUtils::writeFile(path_, content_);
    }
    
    std::string getDescription() const override {
        return "Create file: " + path_;
    }

private:
    std::string path_;
    std::string content_;
};

class DeleteFileCommand : public Command {
public:
    DeleteFileCommand(const std::string& path)
        : path_(path) {}
    
    void execute() override {
        std::filesystem::remove(path_);
    }
    
    std::string getDescription() const override {
        return "Delete file: " + path_;
    }

private:
    std::string path_;
};

class RenameFileCommand : public Command {
public:
    RenameFileCommand(const std::string& oldPath, const std::string& newPath)
        : oldPath_(oldPath), newPath_(newPath) {}
    
    void execute() override {
        std::filesystem::rename(oldPath_, newPath_);
    }
    
    std::string getDescription() const override {
        return "Rename file: " + oldPath_ + " to " + newPath_;
    }

private:
    std::string oldPath_;
    std::string newPath_;
};


        return it != commands_.end() ? it->second->getDescription() : "Command not found";
    }

private:
    CommandProcessor() = default;
    std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
};

} // namespace bolt

#endif