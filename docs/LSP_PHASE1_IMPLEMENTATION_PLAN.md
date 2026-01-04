# LSP Phase 1: Dependency Resolution - Complete Implementation Plan

## Project: bolt-cppml LSP Integration
**Date**: December 12, 2025
**Phase**: 1 of 4 (Dependency Resolution)
**Estimated Time**: 2-3 hours
**Difficulty**: MEDIUM

---

## Table of Contents

1. [Overview](#overview)
2. [Current State Analysis](#current-state-analysis)
3. [Dependency Requirements](#dependency-requirements)
4. [Implementation Steps](#implementation-steps)
5. [Verification Procedures](#verification-procedures)
6. [Troubleshooting Guide](#troubleshooting-guide)
7. [Success Criteria](#success-criteria)

---

## Overview

### Objective
Prepare the bolt-cppml development environment with all necessary dependencies to enable LSP (Language Server Protocol) components that are currently disabled in the build system.

### Why This Matters
- LSP is **essential** for modern IDE functionality
- Code completion, go-to-definition, diagnostics are baseline expectations
- The code already exists but is disabled due to missing dependencies
- **High ROI**: Unlock existing functionality vs. building from scratch

### Current Status
- ✅ LSP code exists: `src/bolt/editor/lsp_*.cpp`
- ❌ Commented out in CMakeLists.txt (lines 142-145)
- ✅ Custom JSON implementation (no external JSON dependency needed)
- ❌ Missing: LSP server binary (clangd)
- ⚠️ Potential: Build configuration issues

---

## Current State Analysis

### What We Have

#### 1. LSP Implementation Files
```
src/bolt/editor/
├── lsp_json_rpc.cpp       ✅ Custom JSON-RPC implementation
├── lsp_server.cpp         ✅ LSP server wrapper
├── lsp_client.cpp         ✅ LSP client with process management
└── lsp_plugin_adapter.cpp ✅ Editor integration adapter

include/bolt/editor/
├── lsp_json_rpc.hpp       ✅ JSON-RPC headers
├── lsp_protocol.hpp       ✅ LSP protocol definitions
└── lsp_client.hpp         ✅ Client interface
```

#### 2. Existing Dependencies (Already Installed)
- ✅ **jsoncpp** (1.9.6 via vcpkg) - Available but LSP uses custom JSON
- ✅ **curl** (8.15.0 via vcpkg) - For potential HTTP-based LSP
- ✅ **C++17 compiler** - Required language features
- ✅ **POSIX/Windows APIs** - Process management (already in code)

#### 3. Build System Status
```cmake
# CMakeLists.txt lines 142-145 (COMMENTED OUT)
# src/bolt/editor/lsp_json_rpc.cpp
# src/bolt/editor/lsp_server.cpp
# src/bolt/editor/lsp_client.cpp
# src/bolt/editor/lsp_plugin_adapter.cpp
```

### What We Need

#### 1. LSP Server Binary (clangd)
**Status**: ❌ NOT INSTALLED
**Purpose**: Provides C++ language intelligence
**Required for**: Code completion, diagnostics, go-to-definition

#### 2. Optional: nlohmann-json
**Status**: ⚠️ AVAILABLE (not installed)
**Purpose**: Modern C++ JSON library (alternative to custom implementation)
**Required**: NO (custom JSON already implemented)
**Benefit**: Better performance, more features

#### 3. Build Configuration
**Status**: ⚠️ NEEDS VERIFICATION
**Purpose**: Ensure LSP files compile without errors
**Required**: YES

---

## Dependency Requirements

### Critical Dependencies

#### 1. clangd (LSP Server)
**Priority**: CRITICAL
**Installation Method**: System package manager

**Why clangd?**
- Most mature C++ LSP server
- Developed by LLVM project
- Excellent C++ support
- Well-documented protocol

**Alternatives**:
- ccls (lighter weight)
- cquery (older, less maintained)

#### 2. LLVM/Clang Libraries
**Priority**: HIGH (dependency of clangd)
**Installation Method**: System package manager

### Optional Dependencies

#### 1. nlohmann-json
**Priority**: LOW (custom JSON already works)
**Installation Method**: vcpkg
**Benefit**: Replace custom JSON with battle-tested library

#### 2. Boost.Process
**Priority**: LOW (custom process management already works)
**Installation Method**: vcpkg
**Benefit**: Cross-platform process management

---

## Implementation Steps

### Step 1: Environment Preparation

#### 1.1 Update Package Lists
```bash
sudo apt-get update
```

**Expected Output**:
```
Hit:1 http://archive.ubuntu.com/ubuntu jammy InRelease
...
Reading package lists... Done
```

**Verification**:
```bash
echo $?  # Should output: 0
```

#### 1.2 Check Disk Space
```bash
df -h /
```

**Required**: At least 2GB free space
**Expected Output**:
```
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        50G   20G   28G  42% /
```

---

### Step 2: Install clangd

#### 2.1 Check Available Versions
```bash
apt-cache search clangd | grep "^clangd"
```

**Expected Output**:
```
clangd - Language server for C/C++
clangd-12 - Language server for C/C++ (version 12)
clangd-14 - Language server for C/C++ (version 14)
clangd-15 - Language server for C/C++ (version 15)
```

#### 2.2 Install clangd (Latest Version)
```bash
sudo apt-get install -y clangd-15
```

**Expected Output**:
```
Reading package lists... Done
Building dependency tree... Done
...
Setting up clangd-15 (1:15.0.7-0ubuntu0.22.04.3) ...
```

**Alternative**: Install default version
```bash
sudo apt-get install -y clangd
```

#### 2.3 Create Symlink (if needed)
```bash
# If clangd-15 is installed but 'clangd' command doesn't work
sudo update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-15 100
```

**Verification**:
```bash
which clangd
clangd --version
```

**Expected Output**:
```
/usr/bin/clangd
Ubuntu clangd version 15.0.7
Features: linux+grpc
Platform: x86_64-pc-linux-gnu
```

---

### Step 3: Install LLVM Development Libraries (Optional)

#### 3.1 Install LLVM Libraries
```bash
sudo apt-get install -y \
    libllvm15 \
    llvm-15-dev \
    libclang-15-dev
```

**Purpose**: Required if we want to embed clangd or use LLVM APIs directly

**Expected Output**:
```
Setting up libllvm15:amd64 (1:15.0.7-0ubuntu0.22.04.3) ...
Setting up llvm-15-dev (1:15.0.7-0ubuntu0.22.04.3) ...
Setting up libclang-15-dev (1:15.0.7-0ubuntu0.22.04.3) ...
```

**Verification**:
```bash
llvm-config-15 --version
```

**Expected Output**:
```
15.0.7
```

---

### Step 4: Install Optional Dependencies (nlohmann-json)

#### 4.1 Check if Already Installed
```bash
cd /home/ubuntu/bolt-cppml
./vcpkg_tool/vcpkg list | grep nlohmann-json
```

**Expected Output** (if not installed):
```
(empty)
```

#### 4.2 Install via vcpkg
```bash
cd /home/ubuntu/bolt-cppml
./vcpkg_tool/vcpkg install nlohmann-json
```

**Expected Output**:
```
Computing installation plan...
The following packages will be built and installed:
    nlohmann-json:x64-linux -> 3.12.0
...
nlohmann-json:x64-linux package ABI: <hash>
Total install time: 5s
```

**Verification**:
```bash
./vcpkg_tool/vcpkg list | grep nlohmann-json
```

**Expected Output**:
```
nlohmann-json:x64-linux    3.12.0    JSON for Modern C++
```

---

### Step 5: Verify Build Environment

#### 5.1 Check Compiler Version
```bash
g++ --version
```

**Required**: GCC 9.0+ or Clang 10.0+
**Expected Output**:
```
g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
```

#### 5.2 Check CMake Version
```bash
cmake --version
```

**Required**: CMake 3.15+
**Expected Output**:
```
cmake version 3.22.1
```

#### 5.3 Check Thread Support
```bash
echo "#include <thread>" | g++ -x c++ -std=c++17 -E - > /dev/null && echo "✅ Thread support OK" || echo "❌ Thread support missing"
```

**Expected Output**:
```
✅ Thread support OK
```

---

### Step 6: Create LSP Configuration

#### 6.1 Create compile_commands.json
```bash
cd /home/ubuntu/bolt-cppml/build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_TOOLCHAIN_FILE=../vcpkg_tool/scripts/buildsystems/vcpkg.cmake \
      ..
```

**Purpose**: Tells clangd how to compile the project

**Expected Output**:
```
-- Configuring done
-- Generating done
-- Build files have been written to: /home/ubuntu/bolt-cppml/build
```

**Verification**:
```bash
ls -lh compile_commands.json
```

**Expected Output**:
```
-rw-r--r-- 1 ubuntu ubuntu 150K Dec 12 14:30 compile_commands.json
```

#### 6.2 Create Symlink to Project Root
```bash
cd /home/ubuntu/bolt-cppml
ln -sf build/compile_commands.json .
```

**Verification**:
```bash
ls -l compile_commands.json
```

**Expected Output**:
```
lrwxrwxrwx 1 ubuntu ubuntu 29 Dec 12 14:30 compile_commands.json -> build/compile_commands.json
```

---

### Step 7: Test clangd Manually

#### 7.1 Create Test File
```bash
cat > /tmp/test_lsp.cpp << 'EOF'
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    for (auto n : numbers) {
        std::cout << n << std::endl;
    }
    return 0;
}
EOF
```

#### 7.2 Run clangd on Test File
```bash
cd /home/ubuntu/bolt-cppml
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootUri":"file:///tmp","capabilities":{}}}' | clangd --log=verbose 2>&1 | head -50
```

**Expected Output** (should contain):
```
Content-Length: ...

{"jsonrpc":"2.0","id":1,"result":{"capabilities":{...}}}
```

**Success Indicators**:
- No crash or error messages
- JSON response with capabilities
- Server version info in logs

---

## Verification Procedures

### Verification Checklist

#### ✅ Step 1: clangd Installation
```bash
# Test 1: Command exists
which clangd
# Expected: /usr/bin/clangd

# Test 2: Version check
clangd --version | grep -i "clangd version"
# Expected: Ubuntu clangd version 15.0.7 (or similar)

# Test 3: Help output
clangd --help | grep -i "language server"
# Expected: Output containing "Language Server Protocol"
```

#### ✅ Step 2: LLVM Libraries (if installed)
```bash
# Test 1: llvm-config exists
llvm-config-15 --version
# Expected: 15.0.7

# Test 2: Library path
llvm-config-15 --libdir
# Expected: /usr/lib/llvm-15/lib (or similar)

# Test 3: Include path
llvm-config-15 --includedir
# Expected: /usr/lib/llvm-15/include (or similar)
```

#### ✅ Step 3: vcpkg Dependencies (if installed)
```bash
cd /home/ubuntu/bolt-cppml

# Test 1: nlohmann-json
./vcpkg_tool/vcpkg list | grep nlohmann-json
# Expected: nlohmann-json:x64-linux    3.12.0

# Test 2: Header availability
find vcpkg_installed -name "json.hpp" 2>/dev/null | head -1
# Expected: vcpkg_installed/x64-linux/include/nlohmann/json.hpp
```

#### ✅ Step 4: Build Environment
```bash
# Test 1: compile_commands.json exists
ls -l /home/ubuntu/bolt-cppml/compile_commands.json
# Expected: Symlink to build/compile_commands.json

# Test 2: compile_commands.json is valid JSON
jq '.[0].file' /home/ubuntu/bolt-cppml/build/compile_commands.json 2>/dev/null || python3 -m json.tool /home/ubuntu/bolt-cppml/build/compile_commands.json > /dev/null
# Expected: No errors

# Test 3: Contains bolt files
grep -c "bolt" /home/ubuntu/bolt-cppml/build/compile_commands.json
# Expected: > 0 (number of bolt source files)
```

#### ✅ Step 5: LSP Communication Test
```bash
# Test 1: clangd responds to initialize
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootUri":"file:///tmp","capabilities":{}}}' | clangd 2>/dev/null | grep -q "result" && echo "✅ LSP communication OK" || echo "❌ LSP communication failed"
# Expected: ✅ LSP communication OK
```

---

## Troubleshooting Guide

### Issue 1: clangd Not Found

**Symptoms**:
```bash
$ which clangd
(no output)
```

**Solution A**: Install from apt
```bash
sudo apt-get update
sudo apt-get install -y clangd-15
sudo update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-15 100
```

**Solution B**: Download binary manually
```bash
# Download from LLVM releases
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/clang+llvm-15.0.7-x86_64-linux-gnu-ubuntu-22.04.tar.xz
tar xf clang+llvm-15.0.7-x86_64-linux-gnu-ubuntu-22.04.tar.xz
sudo cp clang+llvm-15.0.7-x86_64-linux-gnu-ubuntu-22.04/bin/clangd /usr/local/bin/
```

**Verification**:
```bash
clangd --version
```

---

### Issue 2: clangd Crashes on Startup

**Symptoms**:
```bash
$ clangd
Segmentation fault (core dumped)
```

**Diagnosis**:
```bash
# Check dependencies
ldd $(which clangd) | grep "not found"
```

**Solution**: Install missing libraries
```bash
sudo apt-get install -y libllvm15 libclang-common-15-dev
```

---

### Issue 3: compile_commands.json Not Generated

**Symptoms**:
```bash
$ ls build/compile_commands.json
ls: cannot access 'build/compile_commands.json': No such file or directory
```

**Solution**:
```bash
cd /home/ubuntu/bolt-cppml/build
rm -rf *
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_TOOLCHAIN_FILE=../vcpkg_tool/scripts/buildsystems/vcpkg.cmake \
      ..
```

**Verification**:
```bash
ls -lh compile_commands.json
```

---

### Issue 4: vcpkg Install Fails

**Symptoms**:
```bash
$ ./vcpkg_tool/vcpkg install nlohmann-json
error: Failed to download ...
```

**Solution A**: Update vcpkg
```bash
cd /home/ubuntu/bolt-cppml/vcpkg_tool
git pull
./bootstrap-vcpkg.sh
```

**Solution B**: Use system package
```bash
# nlohmann-json is header-only, can install via apt
sudo apt-get install -y nlohmann-json3-dev
```

**Verification**:
```bash
ls /usr/include/nlohmann/json.hpp
```

---

### Issue 5: Permission Denied

**Symptoms**:
```bash
$ sudo apt-get install clangd
E: Could not open lock file ...
```

**Solution**:
```bash
# Wait for other apt processes to finish
ps aux | grep apt
# Kill if stuck
sudo killall apt apt-get
sudo rm /var/lib/apt/lists/lock
sudo rm /var/cache/apt/archives/lock
sudo rm /var/lib/dpkg/lock*
sudo dpkg --configure -a
sudo apt-get update
```

---

## Success Criteria

### Phase 1 Complete When:

#### ✅ Criterion 1: clangd Installed and Working
```bash
clangd --version
# Output contains version number (e.g., 15.0.7)
```

#### ✅ Criterion 2: compile_commands.json Generated
```bash
ls -l /home/ubuntu/bolt-cppml/compile_commands.json
# Symlink exists and points to build/compile_commands.json
```

#### ✅ Criterion 3: LSP Communication Test Passes
```bash
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootUri":"file:///tmp","capabilities":{}}}' | clangd 2>/dev/null | grep -q "result"
echo $?
# Output: 0 (success)
```

#### ✅ Criterion 4: Environment Variables Set (if needed)
```bash
# Check if LLVM paths are accessible
llvm-config-15 --version 2>/dev/null || echo "LLVM not needed for basic LSP"
# Either works or not needed
```

#### ✅ Criterion 5: Documentation Complete
- [x] This implementation plan created
- [x] All commands documented
- [x] Troubleshooting guide provided
- [x] Verification procedures defined

---

## Next Steps (Phase 2)

After completing Phase 1, proceed to:

### Phase 2: Fix Build Issues
1. Uncomment LSP files in CMakeLists.txt
2. Attempt compilation
3. Fix any header/linking errors
4. Verify all LSP components build

### Phase 3: Basic Integration
1. Connect LSP client to clangd
2. Test initialize/shutdown sequence
3. Verify basic requests work

### Phase 4: UI Integration
1. Show completions in editor
2. Display diagnostics
3. Add keyboard shortcuts

---

## Command Reference Sheet

### Quick Start (Copy-Paste)
```bash
# 1. Update system
sudo apt-get update

# 2. Install clangd
sudo apt-get install -y clangd-15
sudo update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-15 100

# 3. Verify installation
clangd --version

# 4. Generate compile_commands.json
cd /home/ubuntu/bolt-cppml/build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_TOOLCHAIN_FILE=../vcpkg_tool/scripts/buildsystems/vcpkg.cmake \
      ..

# 5. Create symlink
cd /home/ubuntu/bolt-cppml
ln -sf build/compile_commands.json .

# 6. Test LSP
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"rootUri":"file:///tmp","capabilities":{}}}' | clangd 2>/dev/null | grep -q "result" && echo "✅ SUCCESS" || echo "❌ FAILED"
```

### Verification Commands
```bash
# Check all installations
which clangd && echo "✅ clangd installed" || echo "❌ clangd missing"
ls -l /home/ubuntu/bolt-cppml/compile_commands.json && echo "✅ compile_commands.json exists" || echo "❌ compile_commands.json missing"
./vcpkg_tool/vcpkg list | grep -q "nlohmann-json" && echo "✅ nlohmann-json installed" || echo "⚠️ nlohmann-json not installed (optional)"
```

---

## Estimated Timeline

| Task | Time | Cumulative |
|------|------|------------|
| Environment preparation | 5 min | 5 min |
| Install clangd | 10 min | 15 min |
| Install LLVM libraries (optional) | 15 min | 30 min |
| Install vcpkg dependencies (optional) | 10 min | 40 min |
| Generate compile_commands.json | 5 min | 45 min |
| Test and verify | 15 min | 60 min |
| Troubleshooting buffer | 30 min | 90 min |
| **Total** | **90 min** | **1.5 hours** |

**Note**: Times assume good internet connection and no major issues.

---

## Conclusion

Phase 1 focuses on preparing the environment with all necessary dependencies. The key insight is that **bolt-cppml already has custom JSON-RPC and LSP protocol implementations**, so we primarily need:

1. **clangd binary** (critical)
2. **compile_commands.json** (critical)
3. **Optional enhancements** (nlohmann-json, LLVM libs)

Once Phase 1 is complete, we can proceed to Phase 2 (fixing build issues) with confidence that all dependencies are in place.

---

**Prepared by**: Manus AI Agent
**Date**: December 12, 2025
**Status**: READY FOR EXECUTION
**Next Phase**: Phase 2 - Fix Build Issues
