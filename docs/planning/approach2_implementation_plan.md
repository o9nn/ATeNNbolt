# Implementation Plan: Downgrade llama.cpp (Approach 2)

## Overview

This document provides a step-by-step implementation plan for downgrading `llama.cpp` to a version compatible with the current standalone `ggml.cpp`.

## Prerequisites

- Backup of current `ggml/llama.cpp` directory
- Git access to llama.cpp repository history
- Understanding of GGML API evolution

## Phase 1: Identify Compatible Version

### 1.1. Find When Missing Functions Were Introduced

The missing functions that cause build failures are:
- `ggml_add_id()`
- `ggml_swiglu_oai()`
- `ggml_flash_attn_ext_add_sinks()`
- `ggml_soft_max_add_sinks()`

### 1.2. Search llama.cpp Git History

Since `llama.cpp` is not currently a git submodule, we need to:

1. Clone the official llama.cpp repository temporarily
2. Search for the commit that introduced these functions
3. Identify the last commit before these functions were added

```bash
cd /tmp
git clone https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
git log --all --oneline --grep="ggml_add_id" | head -10
git log -S "ggml_add_id" --source --all --oneline | head -10
```

### 1.3. Verify Compatibility

Once a candidate commit is found:
1. Check out that commit
2. Verify that `llama-graph.cpp` does NOT use the missing functions
3. Check the GGML version embedded in that commit

## Phase 2: Replace llama.cpp Source

### 2.1. Backup Current Version

```bash
cd /home/ubuntu/bolt-cppml
tar -czf /home/ubuntu/llama_cpp_backup.tar.gz ggml/llama.cpp/
```

### 2.2. Download Compatible Version

```bash
cd /tmp/llama.cpp
git checkout <compatible_commit_hash>
cd /home/ubuntu/bolt-cppml
rm -rf ggml/llama.cpp/*
cp -r /tmp/llama.cpp/* ggml/llama.cpp/
```

### 2.3. Document Version

Create a `VERSION.txt` file in `ggml/llama.cpp/`:

```bash
echo "llama.cpp pinned to commit: <commit_hash>" > ggml/llama.cpp/VERSION.txt
echo "Reason: Compatibility with standalone ggml.cpp" >> ggml/llama.cpp/VERSION.txt
echo "Date: $(date)" >> ggml/llama.cpp/VERSION.txt
```

## Phase 3: Build and Test

### 3.1. Clean Build

```bash
cd /home/ubuntu/bolt-cppml
rm -rf build
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_LLAMA_CPP=ON ..
make -j$(nproc)
```

### 3.2. Verify Build Success

Check for:
- No GGML API errors
- Successful compilation of llama.cpp
- Successful linking of all components

### 3.3. Functional Testing

Test basic llama.cpp functionality:
```bash
# Test model loading (if a model is available)
./bolt --test-llama
```

## Phase 4: Documentation

### 4.1. Update README.md

Add a section explaining the version pinning:

```markdown
## llama.cpp Version

This project uses a specific version of llama.cpp (commit: <hash>) 
for compatibility with the standalone GGML library. 

**Do not update llama.cpp without verifying GGML compatibility.**
```

### 4.2. Create Update Guide

Document the process for future updates:

```markdown
## Updating llama.cpp

1. Check if standalone ggml.cpp has been updated
2. Verify GGML API compatibility
3. Test build with new llama.cpp version
4. Update VERSION.txt
5. Run full test suite
```

## Phase 5: Git Commit

```bash
git add ggml/llama.cpp/
git commit -m "Pin llama.cpp to compatible version

- Downgraded llama.cpp to commit <hash>
- This version is compatible with standalone ggml.cpp
- Added VERSION.txt to track pinned version
- Resolves GGML API incompatibility

Note: This is a temporary fix. Future updates require
careful GGML version coordination."
```

## Challenges and Limitations

### Challenge 1: Finding the Right Commit

**Problem**: Without git history in the current `llama.cpp` directory, finding the exact compatible commit is difficult.

**Solution**: 
- Clone the official repository
- Use `git bisect` to find the breaking commit
- Test each candidate commit

**Estimated Time**: 2-3 hours

### Challenge 2: Missing Features

**Problem**: Older llama.cpp versions lack:
- Recent performance optimizations
- New model architecture support
- Bug fixes
- Security patches

**Impact**: 
- Reduced performance
- Limited model support
- Potential security vulnerabilities

### Challenge 3: Future Updates

**Problem**: Every time standalone `ggml.cpp` is updated, llama.cpp compatibility must be re-verified.

**Solution**:
- Establish a testing protocol
- Document the update process
- Consider automated compatibility checks

**Ongoing Effort**: 1-2 hours per update cycle

### Challenge 4: Diverging Codebases

**Problem**: As time passes, the pinned llama.cpp version will diverge significantly from the latest version.

**Impact**:
- Increasing difficulty to update
- Missing critical features
- Community support issues

**Long-term Risk**: High

## Rollback Plan

If the downgraded version has issues:

```bash
cd /home/ubuntu/bolt-cppml
rm -rf ggml/llama.cpp
tar -xzf /home/ubuntu/llama_cpp_backup.tar.gz
git checkout ggml/llama.cpp
```

## Success Criteria

- ✅ Project builds without GGML API errors
- ✅ llama.cpp functionality is preserved
- ✅ Version is documented
- ✅ Update process is documented
- ⚠️ Aware of missing features
- ⚠️ Aware of security implications

## Estimated Time

- Phase 1: 2-3 hours (finding compatible version)
- Phase 2: 30 minutes
- Phase 3: 1 hour
- Phase 4: 30 minutes
- Phase 5: 15 minutes

**Total: 4-5 hours**

## Comparison with Approach 1

| Aspect | Approach 2 (Downgrade) | Approach 1 (Unify) |
|--------|------------------------|---------------------|
| Initial Time | 4-5 hours | 3-4 hours |
| Ongoing Maintenance | High | Low |
| Feature Access | Limited | Full |
| Security | Risky | Secure |
| Long-term Viability | Poor | Excellent |

## Recommendation

**Approach 2 is NOT recommended** except as a temporary measure while preparing for Approach 1. The long-term maintenance burden, security risks, and feature limitations make this approach unsuitable for production use.

If Approach 2 must be used:
1. Set a deadline for migration to Approach 1
2. Document all known limitations
3. Establish a security monitoring process
4. Plan for eventual unification
