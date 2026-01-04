# Git Integration for Bolt C++ IDE

## Overview

The Git integration module provides comprehensive version control capabilities for the Bolt C++ IDE. It allows developers to interact with Git repositories directly from the IDE, monitor file changes, and perform common Git operations.

## Features

### ✅ Core Features Implemented

- **Repository Detection**: Automatically detect if the current project is a Git repository
- **File Status Monitoring**: Track the status of files (modified, staged, untracked, etc.)
- **Branch Information**: Display current branch and available branches
- **Basic Git Operations**: Add, commit, push, pull functionality
- **Real-time Updates**: Optional auto-refresh of Git status
- **Diff Viewing**: View file differences
- **Commit History**: Access recent commit information
- **Remote Status**: Check if local branch is ahead/behind remote

### 🔧 Integration Points

- **Editor Store Integration**: Git status can be integrated with file editing
- **File Tree Integration**: File status indicators in file navigation
- **Plugin System**: Git functionality can be extended via plugins
- **Callback System**: Real-time notifications for Git operations

## Architecture

### Core Components

1. **GitRepository** (`include/bolt/git/git_repository.hpp`)
   - Low-level Git operations interface
   - Direct Git command execution
   - File and branch status parsing

2. **GitIntegration** (`include/bolt/git/git_integration.hpp`)
   - High-level Git integration manager
   - Singleton pattern for global access
   - Auto-refresh and callback management
   - Thread-safe operations

3. **Main Git Header** (`include/bolt/git/git.hpp`)
   - Convenient API access
   - Quick operation functions
   - Module initialization

## API Reference

### Quick Operations

```cpp
#include "bolt/git/git.hpp"

// Initialize Git integration
bool success = bolt::git::initializeGit("/path/to/project");

// Quick file operations
bolt::git::quick::stage("myfile.cpp");
bolt::git::quick::stageAll();
bolt::git::quick::commit("Add new feature");
bolt::git::quick::push();
bolt::git::quick::pull();

// Check file status
auto status = bolt::git::quick::status("myfile.cpp");
```

### GitRepository API

```cpp
#include "bolt/git/git_repository.hpp"

bolt::git::GitRepository repo("/path/to/project");

// Repository information
bool isRepo = repo.isGitRepository();
std::string root = repo.getRepositoryRoot();
auto branch = repo.getCurrentBranch();

// File operations
auto fileStatus = repo.getFileStatus();
auto singleFileStatus = repo.getFileStatus("myfile.cpp");
bool success = repo.addFile("myfile.cpp");
success = repo.commit("Commit message");

// Branch operations
auto branches = repo.getBranches();
auto remoteStatus = repo.getRemoteStatus();

// Diff and history
std::string diff = repo.getFileDiff("myfile.cpp");
auto commits = repo.getCommitLog(10);
```

### GitIntegration API

```cpp
#include "bolt/git/git_integration.hpp"

auto& git = bolt::git::GitIntegration::getInstance();

// Initialize
git.initialize("/path/to/project");

// Set callbacks
git.setStatusUpdateCallback([](const std::vector<bolt::git::GitFileInfo>& files) {
    std::cout << "Status updated with " << files.size() << " files\n";
});

git.setOperationCallback([](bool success, const std::string& message) {
    std::cout << (success ? "✅" : "❌") << " " << message << "\n";
});

// Enable auto-refresh
git.setAutoRefresh(true, 5); // 5-second interval

// Get repository information
auto repoInfo = git.getRepositoryInfo();
auto branchStatus = git.getBranchStatus();
```

## File Status Types

```cpp
enum class GitFileStatus {
    UNTRACKED,   // New file not tracked by Git
    MODIFIED,    // File has been modified
    STAGED,      // File is staged for commit
    DELETED,     // File has been deleted
    RENAMED,     // File has been renamed
    COPIED,      // File has been copied
    IGNORED,     // File is ignored by Git
    CLEAN        // File is clean (no changes)
};
```

## Usage Examples

### Basic Repository Operations

```cpp
#include "bolt/git/git.hpp"

// Initialize Git for current project
if (bolt::git::initializeGit(".")) {
    auto& git = bolt::git::getGitIntegration();
    
    // Get repository summary
    auto info = git.getRepositoryInfo();
    std::cout << "Current branch: " << info.currentBranch << "\n";
    std::cout << "Modified files: " << info.uncommittedFiles << "\n";
    
    // Stage and commit changes
    if (git.stageAllFiles()) {
        git.commitChanges("Update documentation");
    }
}
```

### Monitoring File Changes

```cpp
auto& git = bolt::git::getGitIntegration();

// Set up status monitoring
git.setStatusUpdateCallback([](const std::vector<bolt::git::GitFileInfo>& files) {
    for (const auto& file : files) {
        std::cout << file.filePath << " - ";
        switch (file.status) {
            case bolt::git::GitFileStatus::MODIFIED:
                std::cout << "Modified";
                break;
            case bolt::git::GitFileStatus::UNTRACKED:
                std::cout << "Untracked";
                break;
            case bolt::git::GitFileStatus::STAGED:
                std::cout << "Staged";
                break;
            default:
                std::cout << "Other";
        }
        std::cout << "\n";
    }
});

// Enable auto-refresh
git.setAutoRefresh(true, 3); // Check every 3 seconds
```

## Integration with Editor Components

### File Tree Integration

The Git integration can be connected to the file tree to show status indicators:

```cpp
// In file tree rendering code
auto& git = bolt::git::getGitIntegration();
auto status = git.getFileStatus(filePath);
if (status) {
    switch (*status) {
        case bolt::git::GitFileStatus::MODIFIED:
            // Show orange indicator
            break;
        case bolt::git::GitFileStatus::UNTRACKED:
            // Show blue indicator
            break;
        case bolt::git::GitFileStatus::STAGED:
            // Show green indicator
            break;
    }
}
```

### Editor Store Integration

```cpp
// In editor store, notify Git when files change
auto& git = bolt::git::getGitIntegration();
git.notifyFileChanged(filePath);
```

## Testing

### Running Tests

```bash
# Build and run standalone test
cd build
make test_git_standalone
./test_git_standalone

# Build and run integration test (requires full build)
make test_git_integration
./test_git_integration
```

### Demo Application

```bash
# Build and run demo
make demo_git_integration
./demo_git_integration
```

## Implementation Details

### Git Command Execution

The implementation uses system Git commands via `popen()` for maximum compatibility:

- All Git operations are executed as subprocess calls
- Output is parsed using Git's porcelain formats
- Error handling includes both exit codes and stderr output
- Working directory is properly managed for each operation

### Thread Safety

- `GitIntegration` uses mutex protection for shared state
- Auto-refresh runs in a separate thread
- Callback invocations are thread-safe
- Repository operations are generally read-only and safe

### Performance Considerations

- Git commands are cached where possible
- Auto-refresh can be disabled for better performance
- File status is batched to minimize Git calls
- Diff viewing is on-demand only

## Future Enhancements

- **Visual Diff Viewer**: Enhanced diff display with syntax highlighting
- **Merge Conflict Resolution**: Built-in merge conflict editor
- **Blame/Annotate**: Show Git blame information inline
- **Interactive Staging**: Stage/unstage individual lines
- **Branch Management**: GUI for creating/switching branches
- **Stash Support**: Save and restore work in progress
- **Submodule Support**: Handle Git submodules
- **Git Hooks**: Support for Git hooks integration
- **Performance Optimization**: Reduce Git command overhead

## Troubleshooting

### Common Issues

1. **Git not found**: Ensure Git is installed and in PATH
2. **Permission errors**: Check file/directory permissions
3. **Not a Git repository**: Initialize Git repository first
4. **Network timeouts**: Check remote repository connectivity

### Debug Information

Enable debug logging to troubleshoot issues:

```cpp
// Set operation callback to see all Git operations
git.setOperationCallback([](bool success, const std::string& message) {
    std::cout << "[GIT] " << (success ? "SUCCESS" : "ERROR") << ": " << message << "\n";
});
```

## Dependencies

- **C++17**: Required for filesystem and optional support
- **Git**: System Git installation required
- **POSIX**: Uses `popen()` for command execution
- **Standard Library**: filesystem, thread, mutex, chrono

## License

This Git integration module is part of the Bolt C++ IDE project and follows the same licensing terms.