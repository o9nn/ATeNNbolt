#ifndef GIT_INTEGRATION_HPP
#define GIT_INTEGRATION_HPP

#include "git_repository.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace bolt {
namespace git {

/**
 * Git integration manager for the IDE
 * Provides high-level Git integration with editor components
 */
class GitIntegration {
public:
    /**
     * Get the singleton instance
     */
    static GitIntegration& getInstance();
    
    /**
     * Initialize Git integration for a project directory
     */
    bool initialize(const std::string& projectPath);
    
    /**
     * Shutdown Git integration
     */
    void shutdown();
    
    /**
     * Check if Git integration is active
     */
    bool isActive() const;
    
    /**
     * Get the Git repository instance
     */
    std::shared_ptr<GitRepository> getRepository() const;
    
    /**
     * Refresh Git status (async)
     */
    void refreshStatus();
    
    /**
     * Get cached Git status for a file
     */
    std::optional<GitFileStatus> getFileStatus(const std::string& filePath) const;
    
    /**
     * Get all files with their Git status
     */
    std::vector<GitFileInfo> getAllFileStatus() const;
    
    /**
     * Register callback for Git status updates
     */
    void setStatusUpdateCallback(std::function<void(const std::vector<GitFileInfo>&)> callback);
    
    /**
     * Register callback for Git operation results
     */
    void setOperationCallback(std::function<void(bool, const std::string&)> callback);
    
    /**
     * Enable/disable automatic status refresh
     */
    void setAutoRefresh(bool enabled, int intervalSeconds = 5);
    
    /**
     * File change notification (call when files are modified)
     */
    void notifyFileChanged(const std::string& filePath);
    
    /**
     * Quick Git operations
     */
    bool stageFile(const std::string& filePath);
    bool stageAllFiles();
    bool commitChanges(const std::string& message);
    bool pushChanges();
    bool pullChanges();
    
    /**
     * Get current branch information
     */
    struct BranchStatus {
        std::string currentBranch;
        bool hasUncommittedChanges;
        int commitsAhead;
        int commitsBehind;
        bool hasRemote;
    };
    BranchStatus getBranchStatus() const;
    
    /**
     * Get repository summary
     */
    struct RepositoryInfo {
        std::string rootPath;
        std::string currentBranch;
        int uncommittedFiles;
        int stagedFiles;
        int untrackedFiles;
        bool isClean;
    };
    RepositoryInfo getRepositoryInfo() const;

private:
    GitIntegration() = default;
    ~GitIntegration() = default;
    
    // Non-copyable
    GitIntegration(const GitIntegration&) = delete;
    GitIntegration& operator=(const GitIntegration&) = delete;
    
    std::shared_ptr<GitRepository> repository_;
    std::string projectPath_;
    std::atomic<bool> isActive_{false};
    std::atomic<bool> autoRefreshEnabled_{false};
    std::atomic<int> refreshIntervalSeconds_{5};
    
    // Cached status
    mutable std::mutex statusMutex_;
    std::unordered_map<std::string, GitFileStatus> fileStatusCache_;
    std::vector<GitFileInfo> allFileStatus_;
    BranchStatus cachedBranchStatus_;
    
    // Callbacks
    std::function<void(const std::vector<GitFileInfo>&)> statusUpdateCallback_;
    std::function<void(bool, const std::string&)> operationCallback_;
    
    // Background refresh
    std::unique_ptr<std::thread> refreshThread_;
    std::atomic<bool> shouldStopRefresh_{false};
    
    /**
     * Background refresh loop
     */
    void refreshLoop();
    
    /**
     * Update internal status cache
     */
    void updateStatusCache();
    
    /**
     * Update branch status cache
     */
    void updateBranchStatusCache();
    
    /**
     * Notify callbacks of status update
     */
    void notifyStatusUpdate();
};

} // namespace git
} // namespace bolt

#endif // GIT_INTEGRATION_HPP