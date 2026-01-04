#ifndef GIT_REPOSITORY_HPP
#define GIT_REPOSITORY_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>

namespace bolt {
namespace git {

/**
 * Git file status enumeration
 */
enum class GitFileStatus {
    UNTRACKED,
    MODIFIED,
    STAGED,
    DELETED,
    RENAMED,
    COPIED,
    IGNORED,
    CLEAN
};

/**
 * Information about a file's Git status
 */
struct GitFileInfo {
    std::string filePath;
    GitFileStatus status;
    std::string originalPath;  // For renames
    
    GitFileInfo(const std::string& path, GitFileStatus stat) 
        : filePath(path), status(stat) {}
    
    GitFileInfo(const std::string& path, GitFileStatus stat, const std::string& orig)
        : filePath(path), status(stat), originalPath(orig) {}
};

/**
 * Git branch information
 */
struct GitBranchInfo {
    std::string name;
    bool isCurrent;
    bool isRemote;
    std::string tracking;  // Tracking branch name if any
    
    GitBranchInfo(const std::string& branchName, bool current = false) 
        : name(branchName), isCurrent(current), isRemote(false) {}
};

/**
 * Git repository manager
 * Provides interface to Git operations for the IDE
 */
class GitRepository {
public:
    /**
     * Create a Git repository manager for the given path
     */
    explicit GitRepository(const std::string& repositoryPath);
    
    /**
     * Check if the current directory is a Git repository
     */
    bool isGitRepository() const;
    
    /**
     * Get the root path of the Git repository
     */
    std::string getRepositoryRoot() const;
    
    /**
     * Get current branch name
     */
    std::optional<std::string> getCurrentBranch() const;
    
    /**
     * Get all branches
     */
    std::vector<GitBranchInfo> getBranches() const;
    
    /**
     * Get status of all files in the repository
     */
    std::vector<GitFileInfo> getFileStatus() const;
    
    /**
     * Get status of a specific file
     */
    std::optional<GitFileInfo> getFileStatus(const std::string& filePath) const;
    
    /**
     * Stage a file for commit
     */
    bool addFile(const std::string& filePath);
    
    /**
     * Stage all changes
     */
    bool addAll();
    
    /**
     * Commit staged changes
     */
    bool commit(const std::string& message);
    
    /**
     * Push to remote repository
     */
    bool push(const std::string& remote = "origin", const std::string& branch = "");
    
    /**
     * Pull from remote repository
     */
    bool pull(const std::string& remote = "origin", const std::string& branch = "");
    
    /**
     * Get the diff for a file
     */
    std::string getFileDiff(const std::string& filePath) const;
    
    /**
     * Get recent commit log
     */
    std::vector<std::string> getCommitLog(int limit = 10) const;
    
    /**
     * Check if there are uncommitted changes
     */
    bool hasUncommittedChanges() const;
    
    /**
     * Check if repository is ahead/behind remote
     */
    struct RemoteStatus {
        int ahead = 0;
        int behind = 0;
        bool hasRemote = false;
    };
    RemoteStatus getRemoteStatus() const;
    
    /**
     * Set callback for Git operation results
     */
    void setStatusCallback(std::function<void(bool, const std::string&)> callback);
    
private:
    std::string repositoryPath_;
    mutable std::string cachedRepositoryRoot_;
    std::function<void(bool, const std::string&)> statusCallback_;
    
    /**
     * Execute a Git command and return output
     */
    std::pair<bool, std::string> executeGitCommand(const std::vector<std::string>& args) const;
    
    /**
     * Parse Git status porcelain output
     */
    std::vector<GitFileInfo> parseStatusOutput(const std::string& output) const;
    
    /**
     * Parse Git branch output
     */
    std::vector<GitBranchInfo> parseBranchOutput(const std::string& output) const;
    
    /**
     * Get relative path from repository root
     */
    std::string getRelativePath(const std::string& absolutePath) const;
    
    /**
     * Find repository root by walking up directories
     */
    std::string findRepositoryRoot() const;
};

} // namespace git
} // namespace bolt

#endif // GIT_REPOSITORY_HPP