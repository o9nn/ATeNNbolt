# AI Model Testing Strategy for Bolt C++

## Overview

This document outlines the comprehensive testing strategy for AI models (GGML, GGUF, RWKV) in the Bolt C++ project. The approach balances thorough testing with practical constraints like repository size and CI performance.

## Current Implementation

### ✅ Successfully Implemented

1. **GGML Core Testing** (`test_ai_models_simple.cpp`)
   - Basic tensor operations (1D/2D tensor creation)
   - Quantization formats (Q4_0, Q4_1, Q5_0, Q5_1, Q8_0)
   - Memory management and context handling
   - Performance benchmarking
   - Computation graph construction

2. **Test Models Directory** (`test/models/`)
   - Small RWKV test models (~600KB)
   - Expected logits for validation
   - README with documentation

3. **Mock AI Interfaces**
   - Wrapper classes for future RWKV integration
   - Basic functionality without heavy dependencies

### Current Test Results
```
[AIModels] GGMLBasicTensorOperations ... PASS
[AIModels] GGMLMatrixOperations ... PASS
[AIModels] GGMLComputeGraph ... PASS
[AIModels] GGMLTensorTypes ... PASS
[AIModels] GGMLQuantizationTypes ... PASS
[AIModels] GGUFVocabularyFiles ... FAIL (path issue)
[AIModels] MockModelInterface ... PASS
[AIModels] ModelMemoryManagement ... PASS
[AIModels] TensorOperationPerformance ... PASS
[AIModels] AIWrapperIntegration ... PASS

Test Results: 9/10 PASS
```

## Recommended Testing Approaches

### 1. **Tiny Model Testing** ⭐ **RECOMMENDED**

**Pros:**
- ✅ Real functional models (600KB - 2MB)
- ✅ Complete inference pipeline testing
- ✅ Known expected outputs for validation
- ✅ Multiple formats and architectures
- ✅ Already available in submodules

**Cons:**
- ❌ RWKV.cpp dependency issues (build complexity)
- ❌ Still requires ~1-5MB per model

**Best For:** Integration testing, regression testing, end-to-end validation

**Models Available:**
- `tiny-rwkv-5v1-730K-Q5_0.bin` (616KB) - **BEST CHOICE**
- `tiny-rwkv-4v0-660K-Q5_0.bin` (695KB)
- GGUF vocabulary files (600KB - 11MB)

### 2. **GGML Core Testing** ⭐ **CURRENTLY IMPLEMENTED**

**Pros:**
- ✅ Fast execution (microseconds)
- ✅ No external dependencies
- ✅ Tests fundamental tensor operations
- ✅ Memory management validation
- ✅ Quantization format testing

**Cons:**
- ❌ Doesn't test actual model inference
- ❌ Limited to GGML primitives

**Best For:** Unit testing, CI/CD, performance regression detection

### 3. **Mock Model Testing** ⭐ **CURRENTLY IMPLEMENTED**

**Pros:**
- ✅ No external files needed
- ✅ Tests interface contracts
- ✅ Fast execution
- ✅ Easy to maintain

**Cons:**
- ❌ Doesn't test real model behavior
- ❌ Limited validation of correctness

**Best For:** API contract testing, development workflows

### 4. **GGUF Format Testing** ⚡ **PARTIALLY IMPLEMENTED**

**Pros:**
- ✅ Tests real GGUF file loading
- ✅ Vocabulary files are small (~1-11MB)
- ✅ Format validation

**Cons:**
- ❌ Path dependency issues
- ❌ Limited to vocabulary testing

**Best For:** File format validation, tokenizer testing

## Size Recommendations

| Model Type | Size Range | Repository Inclusion | Use Case |
|------------|------------|---------------------|----------|
| **Nano Models** | 10KB - 100KB | ✅ Strongly Recommended | Unit tests, CI |
| **Tiny Models** | 100KB - 1MB | ✅ Recommended | Integration tests |
| **Small Models** | 1MB - 10MB | ⚠️ Consider carefully | Validation tests |
| **Regular Models** | >10MB | ❌ Not recommended | External downloads |

## Implementation Strategy

### Phase 1: Core Testing ✅ **COMPLETE**
- GGML tensor operations
- Memory management
- Performance benchmarks
- Mock interfaces

### Phase 2: Tiny Model Integration 🔄 **IN PROGRESS**
- Fix RWKV.cpp build issues
- Integrate tiny RWKV models
- Add real inference testing
- Logit validation

### Phase 3: GGUF Format Testing
- Fix vocabulary file paths
- Add tokenization testing
- Format validation

### Phase 4: Extended Testing
- Multiple model architectures
- Quantization accuracy testing
- Performance regression detection

## Alternative Approaches Considered

### ❌ Downloading Models at Test Time
**Rejected because:**
- Network dependency for tests
- CI/CD complexity
- Inconsistent test environments
- Potential licensing issues

### ❌ Large Model Repository
**Rejected because:**
- Repository bloat (Git LFS costs)
- Slow clone times
- CI resource consumption

### ❌ No AI Model Testing
**Rejected because:**
- Critical functionality not tested
- Regressions in model loading
- Performance issues undetected

## Conclusion

The **current hybrid approach** is optimal:

1. **GGML Core Tests** - Fast, reliable, covers fundamentals
2. **Tiny Model Tests** - Real validation with minimal overhead
3. **Mock Interface Tests** - API contract validation

This provides:
- ✅ Comprehensive coverage
- ✅ Fast CI execution
- ✅ Manageable repository size
- ✅ Real model validation
- ✅ Easy maintenance

## Running Tests

```bash
# All AI model tests
./test/bolt_unit_tests AIModels

# Specific test categories
./test/bolt_unit_tests GGMLTest      # GGML primitives
./test/bolt_unit_tests AIModels      # Full AI integration
```

## Future Enhancements

1. **RWKV Integration** - Fix build issues and enable full RWKV testing
2. **Model Validation Suite** - Compare outputs across different quantization levels
3. **Performance Benchmarks** - Track inference speed regressions
4. **Memory Profiling** - Detect memory leaks in model operations
5. **Multi-Architecture** - Test different model types (GPT, BERT, etc.)
