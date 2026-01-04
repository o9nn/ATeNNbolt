# AI Test Models

This directory contains small AI models for testing the Bolt AI integration.

## RWKV Models

### tiny-rwkv-5v1-730K-Q5_0.bin
- **Size**: 616KB
- **Architecture**: RWKV-5 v1
- **Parameters**: ~730K
- **Quantization**: Q5_0 (5-bit quantization)
- **Purpose**: Basic RWKV functionality testing
- **Expected logits**: `expected-logits-5v1-730K.bin`

## Usage in Tests

These models are used by the test suite to verify:

1. **Model Loading**: Can we successfully load the model?
2. **Basic Inference**: Can we run inference and get reasonable outputs?
3. **Logit Validation**: Do outputs match expected values?
4. **Memory Management**: Proper cleanup and no memory leaks
5. **Performance**: Reasonable inference speed

## Why These Models?

- **Small enough** for version control (< 1MB)
- **Real functional models** (not just mock data)
- **Multiple formats** available for comprehensive testing
- **Known expected outputs** for validation
- **Widely used** in the RWKV community

## Adding More Models

To add additional test models:

1. Keep models under **2MB** to avoid repo bloat
2. Include expected outputs for validation
3. Document the model architecture and purpose
4. Add corresponding tests in `test_ai_models.cpp`

## Alternative Model Sources

For even smaller models, consider:

### Nano GPT Models
- **DistilGPT-2 quantized**: ~40MB (too large for repo)
- **GPT-2 vocabulary only**: ~1-5MB (good for tokenizer tests)

### Custom Tiny Models
- **Single layer transformer**: ~100KB-1MB
- **Minimal RWKV**: ~500KB-2MB
- **Toy models for unit testing**: ~10KB-100KB

### GGUF Vocabulary Files
- Already available in `ggml/llama.cpp/models/`
- Perfect for tokenizer and vocabulary testing
- Range from 600KB to 11MB
