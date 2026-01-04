# llama.cpp Breaking Changes Research

## Key Findings from GitHub Discussion #9276

**Source**: https://github.com/ggml-org/llama.cpp/discussions/9276

### Stability and Versioning Issues

1. **No Semantic Versioning**: llama.cpp uses rolling releases instead of semantic versioning, making it impossible to target specific stable versions.

2. **Frequent Breaking Changes**: Changes can break production deployments without warning.

3. **No Communication Channel**: There is no official notification system for breaking changes before they happen.

4. **Backward Compatibility**: The project prioritizes rapid development over backward compatibility.

### Community Concerns

- Production users have been caught by surprise with breaking changes
- Third-party projects have difficulty maintaining compatibility
- "Duck checking" is required to determine API compatibility
- Infrastructure teams need to rebuild and redeploy unexpectedly

### Developer Response (ggerganov)

- Acknowledged the concern about breaking changes
- Suggested creating changelog documents for API changes
- Mentioned that stability is a consideration but not always prioritized

### Implications for bolt-cppml

1. **Version Pinning Risk**: Pinning to an older llama.cpp version means missing security updates and performance improvements
2. **Update Risk**: Updating llama.cpp may introduce new breaking changes
3. **GGML Sync**: llama.cpp and standalone GGML repositories are synced manually, leading to version mismatches


## GGML Synchronization Process

**Source**: https://github.com/ggml-org/ggml/discussions/1066

### Key Findings

1. **Manual Sync Process**: GGML source code is synced manually across ggml, whisper.cpp, and llama.cpp repositories using Bash scripts in the `scripts` folder.

2. **Sync Frequency**: Synchronization happens approximately once per week (as of January 2025).

3. **No Versioning**: GGML library is currently not versioned, so there are no guarantees of compatibility between different projects using different GGML versions.

4. **Sync Points**: Each repository has sync reference files:
   - `sync-llama.last` - Last sync point with llama.cpp
   - `sync-whisper.last` - Last sync point with whisper.cpp

5. **Partial Syncs**: Sometimes synchronization is partial, and features may be missing or incomplete in the standalone GGML repo compared to embedded versions.

6. **Future Plans**: The maintainers acknowledge that versioning needs to be fixed eventually, but it's not a current priority.

### Implications for bolt-cppml

**Problem**: bolt-cppml uses:
- Standalone `ggml.cpp` repository (embedded in `ggml/ggml.cpp/`)
- Standalone `llama.cpp` repository (embedded in `ggml/llama.cpp/`)

These two repositories are **not synchronized** and have different GGML API versions, causing the incompatibility.

**Root Cause**: llama.cpp has newer GGML APIs that haven't been synced to the standalone ggml repository yet.


## GGML vs llama.cpp: When to Use Which

**Source**: https://github.com/ggml-org/ggml/discussions/141

### Key Differences

**Standalone GGML (ggml-org/ggml)**:
- General-purpose machine learning library
- Moves slower, more stable
- Supports more model types (Whisper, etc.)
- Less bleeding-edge features
- Better for diverse ML applications

**llama.cpp (ggml-org/llama.cpp)**:
- Focused on LLaMA-based model inference
- Faster development cycle
- More bleeding-edge features
- Better performance for LLM inference
- Acts as testbed for new GGML features
- Has its own embedded version of GGML

### Relationship

- llama.cpp embeds its own version of GGML
- Changes are synchronized back and forth periodically
- llama.cpp is a testbed; proven features get merged back to standalone GGML
- They are part of an ecosystem but not always in sync

### Recommendation for bolt-cppml

**Current Architecture Issue**: bolt-cppml tries to use BOTH:
1. Standalone ggml.cpp (for general ML features)
2. llama.cpp (for LLM inference)

**Problem**: These have incompatible GGML versions because:
- llama.cpp's embedded GGML is newer
- Standalone ggml.cpp hasn't received the sync yet
- No version guarantees between them

**Better Approach**: Choose ONE:
- **Option A**: Use llama.cpp exclusively (includes GGML)
- **Option B**: Use standalone GGML exclusively (no llama.cpp)
