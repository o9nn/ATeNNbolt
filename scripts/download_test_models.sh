#!/bin/bash
# Script to download RWKV test models for integration testing
# Models are downloaded from HuggingFace and stored locally

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MODEL_DIR="$PROJECT_ROOT/test/data/models"

# Create model directory
mkdir -p "$MODEL_DIR"

echo "================================================"
echo "RWKV Model Download Script"
echo "================================================"
echo "Model directory: $MODEL_DIR"
echo ""

# Function to download a file with progress
download_file() {
    local url="$1"
    local output="$2"
    local name="$3"
    
    if [ -f "$output" ]; then
        echo "✓ $name already exists, skipping download"
        return 0
    fi
    
    echo "Downloading $name..."
    echo "URL: $url"
    
    # Use wget with progress bar
    if command -v wget &> /dev/null; then
        wget -O "$output" "$url" --progress=bar:force 2>&1
    elif command -v curl &> /dev/null; then
        curl -L -o "$output" "$url" --progress-bar
    else
        echo "Error: Neither wget nor curl is available"
        return 1
    fi
    
    if [ -f "$output" ]; then
        local size=$(du -h "$output" | cut -f1)
        echo "✓ Downloaded successfully ($size)"
        return 0
    else
        echo "✗ Download failed"
        return 1
    fi
}

# Model 1: RWKV-4-Pile-169M (smallest, best for testing)
echo "================================================"
echo "Model 1: RWKV-4-Pile-169M"
echo "================================================"
echo "Size: ~350 MB"
echo "Use case: Fast testing and development"
echo ""

# Try multiple sources for RWKV-4-Pile-169M
MODEL1_NAME="rwkv-4-pile-169m-q8_0.gguf"
MODEL1_PATH="$MODEL_DIR/$MODEL1_NAME"

# HuggingFace URL (primary source)
MODEL1_URL="https://huggingface.co/BlinkDL/rwkv-4-pile-169m/resolve/main/RWKV-4-Pile-169M-20220807-8023.pth"

# Alternative: Try to find GGUF version
# Note: RWKV models on HuggingFace are typically in .pth format
# We may need to convert or find pre-converted GGUF versions

echo "Note: RWKV models are typically distributed in .pth (PyTorch) format."
echo "For GGUF format, we need to either:"
echo "  1. Find pre-converted GGUF models"
echo "  2. Convert .pth to GGUF using rwkv.cpp tools"
echo ""

# Check for existing GGUF models in common locations
GGUF_SOURCES=(
    "https://huggingface.co/latestissue/rwkv-4-pile-169m-gguf/resolve/main/rwkv-4-pile-169m-q8_0.gguf"
    "https://huggingface.co/cmp-nct/rwkv-4-pile-169m-gguf/resolve/main/rwkv-4-pile-169m-q8_0.gguf"
)

downloaded=false
for url in "${GGUF_SOURCES[@]}"; do
    echo "Trying source: $url"
    if download_file "$url" "$MODEL1_PATH" "$MODEL1_NAME"; then
        downloaded=true
        break
    fi
    echo "Source not available, trying next..."
    echo ""
done

if [ "$downloaded" = false ]; then
    echo "⚠ Could not find pre-converted GGUF model"
    echo ""
    echo "Alternative options:"
    echo "  1. Download .pth model and convert using rwkv.cpp"
    echo "  2. Use a different model source"
    echo "  3. Create a minimal test GGUF file for unit testing"
    echo ""
fi

# Model 2: RWKV-5-World-0.1B (multilingual, slightly larger)
echo "================================================"
echo "Model 2: RWKV-5-World-0.1B (Optional)"
echo "================================================"
echo "Size: ~200 MB"
echo "Use case: Multilingual testing"
echo ""

MODEL2_NAME="rwkv-5-world-0.1b-q8_0.gguf"
MODEL2_PATH="$MODEL_DIR/$MODEL2_NAME"

# Try to find RWKV-5 GGUF
echo "Searching for RWKV-5 World GGUF model..."
echo "(Skipping for now, focus on RWKV-4 first)"
echo ""

# Summary
echo "================================================"
echo "Download Summary"
echo "================================================"

if [ -f "$MODEL1_PATH" ]; then
    echo "✓ RWKV-4-Pile-169M: $(du -h "$MODEL1_PATH" | cut -f1)"
else
    echo "✗ RWKV-4-Pile-169M: Not available"
fi

if [ -f "$MODEL2_PATH" ]; then
    echo "✓ RWKV-5-World-0.1B: $(du -h "$MODEL2_PATH" | cut -f1)"
else
    echo "✗ RWKV-5-World-0.1B: Not available"
fi

echo ""
echo "================================================"
echo "Next Steps"
echo "================================================"
echo ""
echo "If models were downloaded successfully:"
echo "  cd $PROJECT_ROOT/build"
echo "  ./test_rwkv_integration"
echo ""
echo "If models are not available in GGUF format:"
echo "  1. Install rwkv.cpp: git clone https://github.com/RWKV/rwkv.cpp"
echo "  2. Download .pth model from HuggingFace"
echo "  3. Convert using: rwkv.cpp/convert-pth-to-gguf.py"
echo ""
echo "For testing without real models:"
echo "  - Unit tests will still run and validate the API"
echo "  - Integration tests will skip model-dependent tests"
echo ""
