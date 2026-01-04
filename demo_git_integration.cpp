#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#include "bolt/git/git.hpp"

class GitIntegrationDemo {
public:
    static void runDemo() {
        std::cout << "=== Bolt C++ IDE - Git Integration Demo ===\n\n";
        
        // Initialize Git integration
        auto& gitIntegration = bolt::git::getGitIntegration();
        std::string currentDir = std::filesystem::current_path().string();
        
        if (!gitIntegration.initialize(currentDir)) {
            std::cout << "❌ This directory is not a Git repository.\n";
            std::cout << "Please run this demo from within a Git repository.\n";
            return;
        }
        
        std::cout << "✅ Git integration initialized successfully!\n\n";
        
        // Set up callbacks
        setupCallbacks();
        
        // Demo the features
        showRepositoryInfo();
        showBranchInfo();
        showFileStatus();
        showGitOperations();
        
        // Enable auto-refresh for demonstration
        std::cout << "Enabling auto-refresh (5 second interval)...\n";
        gitIntegration.setAutoRefresh(true, 5);
        
        std::cout << "\n⏳ Auto-refresh enabled. File status will update automatically.\n";
        std::cout << "   You can modify files in another terminal to see status changes.\n";
        std::cout << "   Press Enter to continue...\n";
        std::cin.get();
        
        gitIntegration.shutdown();
        std::cout << "\n✅ Git integration demo completed!\n";
    }

private:
    static void setupCallbacks() {
        auto& gitIntegration = bolt::git::getGitIntegration();
        
        gitIntegration.setStatusUpdateCallback([](const std::vector<bolt::git::GitFileInfo>& files) {
            std::cout << "\n📊 Git status updated - " << files.size() << " files tracked\n";
            
            int modified = 0, staged = 0, untracked = 0;
            for (const auto& file : files) {
                switch (file.status) {
                    case bolt::git::GitFileStatus::MODIFIED:
                    case bolt::git::GitFileStatus::DELETED:
                        modified++;
                        break;
                    case bolt::git::GitFileStatus::STAGED:
                        staged++;
                        break;
                    case bolt::git::GitFileStatus::UNTRACKED:
                        untracked++;
                        break;
                    default:
                        break;
                }
            }
            
            std::cout << "   📝 " << modified << " modified, 📦 " << staged << " staged, ❓ " << untracked << " untracked\n";
        });
        
        gitIntegration.setOperationCallback([](bool success, const std::string& message) {
            if (success) {
                std::cout << "✅ Git operation: " << message << "\n";
            } else {
                std::cout << "❌ Git operation failed: " << message << "\n";
            }
        });
    }
    
    static void showRepositoryInfo() {
        std::cout << "📁 Repository Information:\n";
        std::cout << std::string(40, '-') << "\n";
        
        auto& gitIntegration = bolt::git::getGitIntegration();
        auto repoInfo = gitIntegration.getRepositoryInfo();
        
        std::cout << "Root Path:      " << repoInfo.rootPath << "\n";
        std::cout << "Current Branch: " << repoInfo.currentBranch << "\n";
        std::cout << "Status:         " << (repoInfo.isClean ? "🟢 Clean" : "🟡 Has changes") << "\n";
        std::cout << "Files:\n";
        std::cout << "  📝 Modified:   " << repoInfo.uncommittedFiles << "\n";
        std::cout << "  📦 Staged:     " << repoInfo.stagedFiles << "\n";
        std::cout << "  ❓ Untracked:  " << repoInfo.untrackedFiles << "\n";
        std::cout << "\n";
    }
    
    static void showBranchInfo() {
        std::cout << "🌿 Branch Information:\n";
        std::cout << std::string(40, '-') << "\n";
        
        auto& gitIntegration = bolt::git::getGitIntegration();
        auto branchStatus = gitIntegration.getBranchStatus();
        
        std::cout << "Current Branch:     " << branchStatus.currentBranch << "\n";
        std::cout << "Uncommitted Changes: " << (branchStatus.hasUncommittedChanges ? "Yes" : "No") << "\n";
        
        if (branchStatus.hasRemote) {
            std::cout << "Remote Status:\n";
            std::cout << "  🔼 Commits ahead:  " << branchStatus.commitsAhead << "\n";
            std::cout << "  🔽 Commits behind: " << branchStatus.commitsBehind << "\n";
        } else {
            std::cout << "Remote Status:      No remote configured\n";
        }
        
        // Show all branches
        auto repo = gitIntegration.getRepository();
        if (repo) {
            auto branches = repo->getBranches();
            std::cout << "\nAll Branches:\n";
            for (const auto& branch : branches) {
                std::cout << "  " << (branch.isCurrent ? "→ " : "  ");
                std::cout << branch.name;
                if (branch.isRemote) std::cout << " (remote)";
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
    
    static void showFileStatus() {
        std::cout << "📋 File Status:\n";
        std::cout << std::string(40, '-') << "\n";
        
        auto& gitIntegration = bolt::git::getGitIntegration();
        auto allFiles = gitIntegration.getAllFileStatus();
        
        if (allFiles.empty()) {
            std::cout << "🟢 Working directory is clean - no changes detected\n\n";
            return;
        }
        
        // Group files by status
        std::vector<std::string> modified, staged, untracked, deleted, renamed;
        
        for (const auto& file : allFiles) {
            switch (file.status) {
                case bolt::git::GitFileStatus::MODIFIED:
                    modified.push_back(file.filePath);
                    break;
                case bolt::git::GitFileStatus::STAGED:
                    staged.push_back(file.filePath);
                    break;
                case bolt::git::GitFileStatus::UNTRACKED:
                    untracked.push_back(file.filePath);
                    break;
                case bolt::git::GitFileStatus::DELETED:
                    deleted.push_back(file.filePath);
                    break;
                case bolt::git::GitFileStatus::RENAMED:
                    renamed.push_back(file.filePath + " (was: " + file.originalPath + ")");
                    break;
                default:
                    break;
            }
        }
        
        auto printFileList = [](const std::string& title, const std::string& icon, const std::vector<std::string>& files) {
            if (!files.empty()) {
                std::cout << icon << " " << title << " (" << files.size() << "):\n";
                for (const auto& file : files) {
                    std::cout << "    " << file << "\n";
                }
                std::cout << "\n";
            }
        };
        
        printFileList("Modified files", "📝", modified);
        printFileList("Staged files", "📦", staged);
        printFileList("Untracked files", "❓", untracked);
        printFileList("Deleted files", "🗑️", deleted);
        printFileList("Renamed files", "🔄", renamed);
    }
    
    static void showGitOperations() {
        std::cout << "⚡ Git Operations Demo:\n";
        std::cout << std::string(40, '-') << "\n";
        
        auto& gitIntegration = bolt::git::getGitIntegration();
        auto repo = gitIntegration.getRepository();
        
        if (!repo) {
            std::cout << "❌ No repository available\n";
            return;
        }
        
        // Show recent commits
        auto commits = repo->getCommitLog(5);
        if (!commits.empty()) {
            std::cout << "📜 Recent commits (last 5):\n";
            for (const auto& commit : commits) {
                std::cout << "    " << commit << "\n";
            }
            std::cout << "\n";
        }
        
        // Demonstrate file diff (if there are modified files)
        auto allFiles = gitIntegration.getAllFileStatus();
        for (const auto& file : allFiles) {
            if (file.status == bolt::git::GitFileStatus::MODIFIED) {
                std::cout << "🔍 Diff for " << file.filePath << ":\n";
                std::string diff = repo->getFileDiff(file.filePath);
                if (!diff.empty()) {
                    // Show first few lines of diff
                    std::istringstream iss(diff);
                    std::string line;
                    int lineCount = 0;
                    while (std::getline(iss, line) && lineCount < 10) {
                        std::cout << "    " << line << "\n";
                        lineCount++;
                    }
                    if (lineCount == 10) {
                        std::cout << "    ... (diff truncated)\n";
                    }
                    std::cout << "\n";
                }
                break; // Just show diff for first modified file
            }
        }
        
        // Demonstrate quick operations (read-only for safety)
        std::cout << "🛠️  Available quick operations:\n";
        std::cout << "   - Stage file: bolt::git::quick::stage(\"filename\")\n";
        std::cout << "   - Stage all: bolt::git::quick::stageAll()\n";
        std::cout << "   - Commit: bolt::git::quick::commit(\"message\")\n";
        std::cout << "   - Push: bolt::git::quick::push()\n";
        std::cout << "   - Pull: bolt::git::quick::pull()\n";
        std::cout << "   - Get status: bolt::git::quick::status(\"filename\")\n";
        std::cout << "\n";
    }
};

int main() {
    try {
        GitIntegrationDemo::runDemo();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Demo failed with unknown exception" << std::endl;
        return 1;
    }
}