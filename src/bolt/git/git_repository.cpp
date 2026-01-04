#include "bolt/git/git_repository.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

namespace bolt {
namespace git {

GitRepository::GitRepository(const std::string& repositoryPath) 
    : repositoryPath_(repositoryPath) {
    if (repositoryPath_.empty()) {
        repositoryPath_ = std::filesystem::current_path().string();
    }
}

bool GitRepository::isGitRepository() const {
    auto [success, output] = executeGitCommand({"rev-parse", "--git-dir"});
    return success;
}

std::string GitRepository::getRepositoryRoot() const {
    if (!cachedRepositoryRoot_.empty()) {
        return cachedRepositoryRoot_;
    }
    
    auto [success, output] = executeGitCommand({"rev-parse", "--show-toplevel"});
    if (success && !output.empty()) {
        // Remove trailing newline
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        cachedRepositoryRoot_ = output;
        return output;
    }
    
    return "";
}

std::optional<std::string> GitRepository::getCurrentBranch() const {
    auto [success, output] = executeGitCommand({"rev-parse", "--abbrev-ref", "HEAD"});
    if (success && !output.empty()) {
        // Remove trailing newline
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
    return std::nullopt;
}

std::vector<GitBranchInfo> GitRepository::getBranches() const {
    auto [success, output] = executeGitCommand({"branch", "-a"});
    if (!success) {
        return {};
    }
    return parseBranchOutput(output);
}

std::vector<GitFileInfo> GitRepository::getFileStatus() const {
    auto [success, output] = executeGitCommand({"status", "--porcelain"});
    if (!success) {
        return {};
    }
    return parseStatusOutput(output);
}

std::optional<GitFileInfo> GitRepository::getFileStatus(const std::string& filePath) const {
    std::string relativePath = getRelativePath(filePath);
    auto [success, output] = executeGitCommand({"status", "--porcelain", relativePath});
    if (!success || output.empty()) {
        return std::nullopt;
    }
    
    auto files = parseStatusOutput(output);
    if (!files.empty()) {
        return files[0];
    }
    return std::nullopt;
}

bool GitRepository::addFile(const std::string& filePath) {
    std::string relativePath = getRelativePath(filePath);
    auto [success, output] = executeGitCommand({"add", relativePath});
    
    if (statusCallback_) {
        statusCallback_(success, success ? "Added: " + relativePath : "Failed to add: " + output);
    }
    
    return success;
}

bool GitRepository::addAll() {
    auto [success, output] = executeGitCommand({"add", "."});
    
    if (statusCallback_) {
        statusCallback_(success, success ? "Added all changes" : "Failed to add all: " + output);
    }
    
    return success;
}

bool GitRepository::commit(const std::string& message) {
    auto [success, output] = executeGitCommand({"commit", "-m", message});
    
    if (statusCallback_) {
        statusCallback_(success, success ? "Committed: " + message : "Failed to commit: " + output);
    }
    
    return success;
}

bool GitRepository::push(const std::string& remote, const std::string& branch) {
    std::vector<std::string> args = {"push"};
    if (!remote.empty()) {
        args.push_back(remote);
        if (!branch.empty()) {
            args.push_back(branch);
        }
    }
    
    auto [success, output] = executeGitCommand(args);
    
    if (statusCallback_) {
        statusCallback_(success, success ? "Pushed to remote" : "Failed to push: " + output);
    }
    
    return success;
}

bool GitRepository::pull(const std::string& remote, const std::string& branch) {
    std::vector<std::string> args = {"pull"};
    if (!remote.empty()) {
        args.push_back(remote);
        if (!branch.empty()) {
            args.push_back(branch);
        }
    }
    
    auto [success, output] = executeGitCommand(args);
    
    if (statusCallback_) {
        statusCallback_(success, success ? "Pulled from remote" : "Failed to pull: " + output);
    }
    
    return success;
}

std::string GitRepository::getFileDiff(const std::string& filePath) const {
    std::string relativePath = getRelativePath(filePath);
    auto [success, output] = executeGitCommand({"diff", relativePath});
    return success ? output : "";
}

std::vector<std::string> GitRepository::getCommitLog(int limit) const {
    auto [success, output] = executeGitCommand({"log", "--oneline", "-n", std::to_string(limit)});
    if (!success) {
        return {};
    }
    
    std::vector<std::string> commits;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            commits.push_back(line);
        }
    }
    return commits;
}

bool GitRepository::hasUncommittedChanges() const {
    auto [success, output] = executeGitCommand({"status", "--porcelain"});
    return success && !output.empty();
}

GitRepository::RemoteStatus GitRepository::getRemoteStatus() const {
    RemoteStatus status;
    
    // Check if we have a remote
    auto [hasRemote, remoteOutput] = executeGitCommand({"remote"});
    status.hasRemote = hasRemote && !remoteOutput.empty();
    
    if (!status.hasRemote) {
        return status;
    }
    
    // Get ahead/behind info
    auto [success, output] = executeGitCommand({"rev-list", "--left-right", "--count", "HEAD...@{upstream}"});
    if (success && !output.empty()) {
        std::istringstream iss(output);
        iss >> status.ahead >> status.behind;
    }
    
    return status;
}

void GitRepository::setStatusCallback(std::function<void(bool, const std::string&)> callback) {
    statusCallback_ = callback;
}

std::pair<bool, std::string> GitRepository::executeGitCommand(const std::vector<std::string>& args) const {
    std::string command = "git";
    for (const auto& arg : args) {
        command += " " + arg;
    }
    
    // Change to repository directory
    std::string currentDir = std::filesystem::current_path().string();
    std::filesystem::current_path(repositoryPath_);
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    
    // Restore original directory
    std::filesystem::current_path(currentDir);
    
    if (!pipe) {
        return {false, "Failed to execute git command"};
    }
    
    std::string result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        result += buffer;
    }
    
    int exitCode = pclose(pipe.release());
    return {exitCode == 0, result};
}

std::vector<GitFileInfo> GitRepository::parseStatusOutput(const std::string& output) const {
    std::vector<GitFileInfo> files;
    std::istringstream iss(output);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.length() < 3) continue;
        
        char indexStatus = line[0];
        char workingTreeStatus = line[1];
        std::string filePath = line.substr(3);
        
        GitFileStatus status = GitFileStatus::CLEAN;
        
        // Parse Git porcelain status format
        if (indexStatus == '?' && workingTreeStatus == '?') {
            status = GitFileStatus::UNTRACKED;
        } else if (indexStatus == 'A' || workingTreeStatus == 'A') {
            status = GitFileStatus::STAGED;
        } else if (indexStatus == 'M' || workingTreeStatus == 'M') {
            if (indexStatus == 'M') {
                status = GitFileStatus::STAGED;
            } else {
                status = GitFileStatus::MODIFIED;
            }
        } else if (indexStatus == 'D' || workingTreeStatus == 'D') {
            status = GitFileStatus::DELETED;
        } else if (indexStatus == 'R' || workingTreeStatus == 'R') {
            status = GitFileStatus::RENAMED;
        } else if (indexStatus == 'C' || workingTreeStatus == 'C') {
            status = GitFileStatus::COPIED;
        }
        
        // Handle renames (format: "R  old_name -> new_name")
        size_t arrowPos = filePath.find(" -> ");
        if (status == GitFileStatus::RENAMED && arrowPos != std::string::npos) {
            std::string oldName = filePath.substr(0, arrowPos);
            std::string newName = filePath.substr(arrowPos + 4);
            files.emplace_back(newName, status, oldName);
        } else {
            files.emplace_back(filePath, status);
        }
    }
    
    return files;
}

std::vector<GitBranchInfo> GitRepository::parseBranchOutput(const std::string& output) const {
    std::vector<GitBranchInfo> branches;
    std::istringstream iss(output);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        
        bool isCurrent = false;
        bool isRemote = false;
        std::string branchName;
        
        // Check if this is the current branch (marked with *)
        if (line.length() > 0 && line[0] == '*') {
            isCurrent = true;
            line = line.substr(1); // Remove the *
        }
        
        // Remove leading whitespace
        size_t nameStart = line.find_first_not_of(" ");
        if (nameStart != std::string::npos) {
            branchName = line.substr(nameStart);
            
            // Remove any trailing whitespace
            size_t nameEnd = branchName.find_last_not_of(" \t\r\n");
            if (nameEnd != std::string::npos) {
                branchName = branchName.substr(0, nameEnd + 1);
            }
            
            // Check if it's a remote branch
            if (branchName.find("remotes/") == 0) {
                isRemote = true;
                // Keep the full remote branch name for clarity
            }
        }
        
        if (!branchName.empty()) {
            GitBranchInfo branch(branchName, isCurrent);
            branch.isRemote = isRemote;
            branches.push_back(branch);
        }
    }
    
    return branches;
}

std::string GitRepository::getRelativePath(const std::string& absolutePath) const {
    std::string repoRoot = getRepositoryRoot();
    if (repoRoot.empty()) {
        return absolutePath;
    }
    
    std::filesystem::path absolute(absolutePath);
    std::filesystem::path root(repoRoot);
    
    try {
        auto relative = std::filesystem::relative(absolute, root);
        return relative.string();
    } catch (const std::exception&) {
        return absolutePath;
    }
}

std::string GitRepository::findRepositoryRoot() const {
    std::filesystem::path current(repositoryPath_);
    
    while (!current.empty() && current != current.root_path()) {
        std::filesystem::path gitDir = current / ".git";
        if (std::filesystem::exists(gitDir)) {
            return current.string();
        }
        current = current.parent_path();
    }
    
    return "";
}

} // namespace git
} // namespace bolt