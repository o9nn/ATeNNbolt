# RWKV Model Setup and Testing Guide

## Overview

This guide explains how to download and test RWKV models with the bolt-cpp implementation.

## Model Format Requirements

The current implementation supports **GGUF format** (GGUF v2/v3). The older GGML format is not yet supported.

### Format Identification

- **GGUF format**: Magic number `0x46554747` ("GGUF" in ASCII)
- **GGML format** (old): Magic number `0x67676d66` ("ggmf" in ASCII)

You can check the format of a model file:
```bash
head -c 4 model.gguf | od -A n -t x1
```

## Downloading Models

### Option 1: Automated Download (Recommended)

Use the provided Python script:
```bash
cd /path/to/bolt-cppml
python3 scripts/download_rwkv_model.py
```

### Option 2: Manual Download

Download from HuggingFace repositories that provide GGUF format:

**Small Models (for testing):**
- RWKV-6-World-0.4B: https://huggingface.co/BlinkDL/rwkv-6-world
- RWKV-7-World-0.1B: https://huggingface.co/BlinkDL/rwkv-7-world

**Note:** Many RWKV models on HuggingFace are in `.pth` (PyTorch) format and need conversion.

### Option 3: Convert from .pth to GGUF

If you have a `.pth` model file:

1. Clone rwkv.cpp:
   ```bash
   git clone https://github.com/RWKV/rwkv.cpp
   cd rwkv.cpp
   ```

2. Install dependencies:
   ```bash
   pip3 install torch numpy
   ```

3. Convert the model:
   ```bash
   python3 convert-pth-to-gguf.py /path/to/model.pth /path/to/output.gguf
   ```

## Model Storage

### Recommended Location

Store models in:
```
bolt-cppml/test/data/models/
```

This directory is:
- Gitignored (models won't be committed)
- Referenced by integration tests
- Easy to manage

### DO NOT Store in Git Repository

Model files are large binary files (100MB - 10GB+) and should NOT be committed to git:
- ❌ Don't add to git repository
- ❌ Don't use GitHub LFS (expensive for large files)
- ✅ Download locally as needed
- ✅ Use external hosting (HuggingFace, etc.)

## Running Tests

### 1. Model Loading Test

```bash
cd bolt-cppml/build
export LD_LIBRARY_PATH=.:./ggml/ggml.cpp/src:$LD_LIBRARY_PATH
./test_model_loading
```

This test will:
- ✅ Load the GGUF file
- ✅ Extract model parameters
- ✅ Load tokenizer vocabulary
- ✅ Initialize RWKV wrapper
- ⚠️ Attempt text generation (may fail if layers not fully implemented)

### 2. Integration Tests

```bash
cd bolt-cppml/build
export LD_LIBRARY_PATH=.:./ggml/ggml.cpp/src:$LD_LIBRARY_PATH
./test_rwkv_integration
```

### 3. Unit Tests

```bash
cd bolt-cppml/build
export LD_LIBRARY_PATH=.:./ggml/ggml.cpp/src:$LD_LIBRARY_PATH
./bolt_unit_tests
```

## Current Status

### ✅ Working

- GGUF file format parsing (v2/v3)
- Metadata extraction
- Tensor information loading
- BPE tokenizer with vocabulary
- Model initialization pipeline

### ⚠️ In Progress

- RWKV layer implementation (time-mixing, channel-mixing)
- Text generation (depends on layer implementation)
- State management during generation

### ❌ Not Yet Supported

- Old GGML format (pre-GGUF)
- GPU acceleration
- Streaming generation
- Model caching

## Troubleshooting

### "Invalid GGUF magic number"

**Problem:** The model file is in old GGML format, not GGUF.

**Solution:**
1. Download a GGUF format model, or
2. Convert your .pth model to GGUF using rwkv.cpp tools

### "Model file not found"

**Problem:** Model not in expected location.

**Solution:**
```bash
# Create directory
mkdir -p test/data/models

# Download model
cd test/data/models
# ... download command ...
```

### "Failed to load tokenizer vocabulary"

**Problem:** Model file doesn't contain tokenizer metadata.

**Solution:**
- Ensure you're using a complete GGUF model with tokenizer
- Some older models may not include tokenizer data

### "Generation failed"

**Problem:** RWKV layers not fully implemented yet.

**Solution:**
- This is expected in the current implementation
- Layer implementation is in progress
- Basic model loading and initialization still work

## Example: Complete Setup

```bash
# 1. Navigate to project
cd /home/ubuntu/bolt-cppml

# 2. Create model directory
mkdir -p test/data/models

# 3. Download a GGUF model (example - adjust URL as needed)
cd test/data/models
wget -O rwkv-model.gguf "https://huggingface.co/.../model.gguf"

# 4. Build the project
cd /home/ubuntu/bolt-cppml
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 5. Run tests
export LD_LIBRARY_PATH=.:./ggml/ggml.cpp/src:$LD_LIBRARY_PATH
./test_model_loading
```

## Next Steps

After successful model loading:

1. **Complete RWKV Layers**
   - Implement time-mixing operation
   - Implement channel-mixing operation
   - Add WKV operation for RWKV-6/7

2. **Validate Output**
   - Compare with rwkv.cpp reference
   - Test with known prompts
   - Verify token-by-token generation

3. **Performance Optimization**
   - Profile inference speed
   - Optimize hot paths
   - Add GPU support

## Resources

- **RWKV Official**: https://www.rwkv.com/
- **RWKV Wiki**: https://wiki.rwkv.com/
- **RWKV.cpp**: https://github.com/RWKV/rwkv.cpp
- **HuggingFace Models**: https://huggingface.co/models?search=rwkv
- **GGUF Specification**: https://github.com/ggml-org/ggml/blob/master/docs/gguf.md

## Support

For issues or questions:
1. Check this documentation
2. Review error messages carefully
3. Consult RWKV wiki and documentation
4. Check GitHub issues in rwkv.cpp repository
