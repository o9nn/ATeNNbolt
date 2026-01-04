#include "bolt/git/git_integration.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>

namespace bolt {
namespace git {

GitIntegration& GitIntegration::getInstance() {
    static GitIntegration instance;
    return instance;
}

bool GitIntegration::initialize(const std::string& projectPath) {
    if (isActive_) {
        shutdown();
    }
    
    projectPath_ = projectPath;
    repository_ = std::make_shared<GitRepository>(projectPath);
    
    if (!repository_->isGitRepository()) {
        return false;
    }
    
    // Set up repository callback
    repository_->setStatusCallback([this](bool success, const std::string& message) {
        if (operationCallback_) {
            operationCallback_(success, message);
        }
        if (success) {
            refreshStatus();
        }
    });
    
    isActive_ = true;
    
    // Initial status refresh
    refreshStatus();
    
    return true;
}

void GitIntegration::shutdown() {
    isActive_ = false;
    
    // Stop auto refresh
    setAutoRefresh(false);
    
    // Clear caches
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        fileStatusCache_.clear();
        allFileStatus_.clear();
    }
    
    repository_.reset();
    projectPath_.clear();
}

bool GitIntegration::isActive() const {
    return isActive_;
}

std::shared_ptr<GitRepository> GitIntegration::getRepository() const {
    return repository_;
}

void GitIntegration::refreshStatus() {
    if (!isActive_ || !repository_) {
        return;
    }
    
    updateStatusCache();
    updateBranchStatusCache();
    notifyStatusUpdate();
}

std::optional<GitFileStatus> GitIntegration::getFileStatus(const std::string& filePath) const {
    if (!isActive_) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(statusMutex_);
    auto it = fileStatusCache_.find(filePath);
    if (it != fileStatusCache_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<GitFileInfo> GitIntegration::getAllFileStatus() const {
    if (!isActive_) {
        return {};
    }
    
    std::lock_guard<std::mutex> lock(statusMutex_);
    return allFileStatus_;
}

void GitIntegration::setStatusUpdateCallback(std::function<void(const std::vector<GitFileInfo>&)> callback) {
    statusUpdateCallback_ = callback;
}

void GitIntegration::setOperationCallback(std::function<void(bool, const std::string&)> callback) {
    operationCallback_ = callback;
}

void GitIntegration::setAutoRefresh(bool enabled, int intervalSeconds) {
    if (autoRefreshEnabled_ == enabled) {
        return;
    }
    
    autoRefreshEnabled_ = enabled;
    refreshIntervalSeconds_ = intervalSeconds;
    
    if (enabled && isActive_) {
        shouldStopRefresh_ = false;
        refreshThread_ = std::make_unique<std::thread>(&GitIntegration::refreshLoop, this);
    } else {
        shouldStopRefresh_ = true;
        if (refreshThread_ && refreshThread_->joinable()) {
            refreshThread_->join();
        }
        refreshThread_.reset();
    }
}

void GitIntegration::notifyFileChanged(const std::string& filePath) {
    if (!isActive_) {
        return;
    }
    
    // For now, just trigger a refresh
    // In the future, we could optimize by only checking specific files
    refreshStatus();
}

bool GitIntegration::stageFile(const std::string& filePath) {
    if (!isActive_ || !repository_) {
        return false;
    }
    
    return repository_->addFile(filePath);
}

bool GitIntegration::stageAllFiles() {
    if (!isActive_ || !repository_) {
        return false;
    }
    
    return repository_->addAll();
}

bool GitIntegration::commitChanges(const std::string& message) {
    if (!isActive_ || !repository_) {
        return false;
    }
    
    return repository_->commit(message);
}

bool GitIntegration::pushChanges() {
    if (!isActive_ || !repository_) {
        return false;
    }
    
    return repository_->push();
}

bool GitIntegration::pullChanges() {
    if (!isActive_ || !repository_) {
        return false;
    }
    
    return repository_->pull();
}

GitIntegration::BranchStatus GitIntegration::getBranchStatus() const {
    if (!isActive_) {
        return {};
    }
    
    std::lock_guard<std::mutex> lock(statusMutex_);
    return cachedBranchStatus_;
}

GitIntegration::RepositoryInfo GitIntegration::getRepositoryInfo() const {
    if (!isActive_ || !repository_) {
        return {};
    }
    
    RepositoryInfo info;
    info.rootPath = repository_->getRepositoryRoot();
    
    auto branch = repository_->getCurrentBranch();
    info.currentBranch = branch.value_or("(detached)");
    
    std::lock_guard<std::mutex> lock(statusMutex_);
    
    info.uncommittedFiles = 0;
    info.stagedFiles = 0;
    info.untrackedFiles = 0;
    
    for (const auto& file : allFileStatus_) {
        switch (file.status) {
            case GitFileStatus::MODIFIED:
            case GitFileStatus::DELETED:
                info.uncommittedFiles++;
                break;
            case GitFileStatus::STAGED:
                info.stagedFiles++;
                break;
            case GitFileStatus::UNTRACKED:
                info.untrackedFiles++;
                break;
            default:
                break;
        }
    }
    
    info.isClean = (info.uncommittedFiles == 0 && info.stagedFiles == 0 && info.untrackedFiles == 0);
    
    return info;
}

void GitIntegration::refreshLoop() {
    while (!shouldStopRefresh_ && autoRefreshEnabled_) {
        refreshStatus();
        
        // Sleep for the specified interval
        auto sleepDuration = std::chrono::seconds(refreshIntervalSeconds_);
        auto start = std::chrono::steady_clock::now();
        
        while (!shouldStopRefresh_ && 
               std::chrono::steady_clock::now() - start < sleepDuration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void GitIntegration::updateStatusCache() {
    if (!repository_) {
        return;
    }
    
    auto allFiles = repository_->getFileStatus();
    
    std::lock_guard<std::mutex> lock(statusMutex_);
    
    // Clear old cache
    fileStatusCache_.clear();
    
    // Update cache with new status
    for (const auto& fileInfo : allFiles) {
        fileStatusCache_[fileInfo.filePath] = fileInfo.status;
    }
    
    allFileStatus_ = std::move(allFiles);
}

void GitIntegration::updateBranchStatusCache() {
    if (!repository_) {
        return;
    }
    
    BranchStatus status;
    
    auto branch = repository_->getCurrentBranch();
    status.currentBranch = branch.value_or("(detached)");
    
    status.hasUncommittedChanges = repository_->hasUncommittedChanges();
    
    auto remoteStatus = repository_->getRemoteStatus();
    status.commitsAhead = remoteStatus.ahead;
    status.commitsBehind = remoteStatus.behind;
    status.hasRemote = remoteStatus.hasRemote;
    
    std::lock_guard<std::mutex> lock(statusMutex_);
    cachedBranchStatus_ = status;
}

void GitIntegration::notifyStatusUpdate() {
    if (statusUpdateCallback_) {
        std::lock_guard<std::mutex> lock(statusMutex_);
        statusUpdateCallback_(allFileStatus_);
    }
}

} // namespace git
} // namespace bolt