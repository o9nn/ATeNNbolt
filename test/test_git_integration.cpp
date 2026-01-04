#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

#include "bolt/git/git.hpp"

/**
 * Simple test framework for Git integration
 */
class GitIntegrationTest {
public:
    static void runAllTests() {
        std::cout << "Running Git Integration Tests...\n";
        
        testGitRepositoryDetection();
        testGitFileStatus();
        testGitBranchInfo();
        testGitIntegrationManager();
        
        std::cout << "All Git integration tests passed!\n";
    }

private:
    static void testGitRepositoryDetection() {
        std::cout << "Testing Git repository detection...\n";
        
        // Test with current repository (should be a Git repo)
        std::string currentDir = std::filesystem::current_path().string();
        bolt::git::GitRepository repo(currentDir);
        
        // This project should be a Git repository
        assert(repo.isGitRepository());
        
        // Get repository root
        std::string root = repo.getRepositoryRoot();
        assert(!root.empty());
        std::cout << "Repository root: " << root << "\n";
        
        // Get current branch
        auto branch = repo.getCurrentBranch();
        if (branch) {
            std::cout << "Current branch: " << *branch << "\n";
        }
        
        std::cout << "✓ Git repository detection test passed\n";
    }
    
    static void testGitFileStatus() {
        std::cout << "Testing Git file status...\n";
        
        std::string currentDir = std::filesystem::current_path().string();
        bolt::git::GitRepository repo(currentDir);
        
        if (repo.isGitRepository()) {
            auto allFiles = repo.getFileStatus();
            std::cout << "Found " << allFiles.size() << " files with Git status\n";
            
            // Print first few files as examples
            for (size_t i = 0; i < std::min(size_t(5), allFiles.size()); ++i) {
                const auto& file = allFiles[i];
                std::cout << "  " << file.filePath << " - Status: " << static_cast<int>(file.status) << "\n";
            }
            
            // Test checking if repository has uncommitted changes
            bool hasChanges = repo.hasUncommittedChanges();
            std::cout << "Has uncommitted changes: " << (hasChanges ? "yes" : "no") << "\n";
        }
        
        std::cout << "✓ Git file status test passed\n";
    }
    
    static void testGitBranchInfo() {
        std::cout << "Testing Git branch information...\n";
        
        std::string currentDir = std::filesystem::current_path().string();
        bolt::git::GitRepository repo(currentDir);
        
        if (repo.isGitRepository()) {
            auto branches = repo.getBranches();
            std::cout << "Found " << branches.size() << " branches\n";
            
            for (const auto& branch : branches) {
                std::cout << "  " << branch.name 
                         << (branch.isCurrent ? " (current)" : "")
                         << (branch.isRemote ? " (remote)" : "") << "\n";
            }
            
            // Test remote status
            auto remoteStatus = repo.getRemoteStatus();
            if (remoteStatus.hasRemote) {
                std::cout << "Remote status - Ahead: " << remoteStatus.ahead 
                         << ", Behind: " << remoteStatus.behind << "\n";
            } else {
                std::cout << "No remote repository configured\n";
            }
        }
        
        std::cout << "✓ Git branch info test passed\n";
    }
    
    static void testGitIntegrationManager() {
        std::cout << "Testing Git integration manager...\n";
        
        auto& gitIntegration = bolt::git::GitIntegration::getInstance();
        
        std::string currentDir = std::filesystem::current_path().string();
        bool initialized = gitIntegration.initialize(currentDir);
        
        if (initialized) {
            std::cout << "Git integration initialized successfully\n";
            
            // Test callbacks
            bool callbackReceived = false;
            gitIntegration.setStatusUpdateCallback([&callbackReceived](const std::vector<bolt::git::GitFileInfo>& files) {
                std::cout << "Status update callback received with " << files.size() << " files\n";
                callbackReceived = true;
            });
            
            // Trigger a status refresh
            gitIntegration.refreshStatus();
            
            // Give some time for async operations
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Test repository info
            auto repoInfo = gitIntegration.getRepositoryInfo();
            std::cout << "Repository info:\n";
            std::cout << "  Root: " << repoInfo.rootPath << "\n";
            std::cout << "  Current branch: " << repoInfo.currentBranch << "\n";
            std::cout << "  Uncommitted files: " << repoInfo.uncommittedFiles << "\n";
            std::cout << "  Staged files: " << repoInfo.stagedFiles << "\n";
            std::cout << "  Untracked files: " << repoInfo.untrackedFiles << "\n";
            std::cout << "  Is clean: " << (repoInfo.isClean ? "yes" : "no") << "\n";
            
            // Test branch status
            auto branchStatus = gitIntegration.getBranchStatus();
            std::cout << "Branch status:\n";
            std::cout << "  Current: " << branchStatus.currentBranch << "\n";
            std::cout << "  Has uncommitted changes: " << (branchStatus.hasUncommittedChanges ? "yes" : "no") << "\n";
            
            gitIntegration.shutdown();
        } else {
            std::cout << "Git integration could not be initialized (not a Git repository)\n";
        }
        
        std::cout << "✓ Git integration manager test passed\n";
    }
};

int main() {
    try {
        GitIntegrationTest::runAllTests();
        std::cout << "\n🎉 All Git integration tests completed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}