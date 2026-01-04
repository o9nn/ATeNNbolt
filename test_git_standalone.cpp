#include <iostream>
#include <cassert>
#include <string>
#include <filesystem>

// Simple standalone test for Git integration
// Include only the necessary Git headers
#include "bolt/git/git_repository.hpp"

int main() {
    std::cout << "Testing Git Integration (Standalone)...\n";
    
    try {
        // Test Git repository detection
        std::string currentDir = std::filesystem::current_path().string();
        bolt::git::GitRepository repo(currentDir);
        
        std::cout << "Current directory: " << currentDir << "\n";
        
        bool isGitRepo = repo.isGitRepository();
        std::cout << "Is Git repository: " << (isGitRepo ? "Yes" : "No") << "\n";
        
        if (isGitRepo) {
            // Test repository root
            std::string root = repo.getRepositoryRoot();
            std::cout << "Repository root: " << root << "\n";
            
            // Test current branch
            auto branch = repo.getCurrentBranch();
            if (branch) {
                std::cout << "Current branch: " << *branch << "\n";
            } else {
                std::cout << "Could not determine current branch\n";
            }
            
            // Test file status
            auto fileStatus = repo.getFileStatus();
            std::cout << "Files with Git status: " << fileStatus.size() << "\n";
            
            // Test uncommitted changes
            bool hasChanges = repo.hasUncommittedChanges();
            std::cout << "Has uncommitted changes: " << (hasChanges ? "Yes" : "No") << "\n";
            
            // Test branches
            auto branches = repo.getBranches();
            std::cout << "Total branches found: " << branches.size() << "\n";
            
            std::cout << "\n✅ Git integration basic functionality works!\n";
        } else {
            std::cout << "\n⚠️  Not testing Git operations (not a Git repository)\n";
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}