#!/usr/bin/env python3
"""
Download RWKV models from HuggingFace Hub
Supports both .pth and .gguf formats
"""

import os
import sys
import urllib.request
import json
from pathlib import Path

def download_with_progress(url, output_path):
    """Download a file with progress bar"""
    def reporthook(count, block_size, total_size):
        percent = int(count * block_size * 100 / total_size)
        sys.stdout.write(f"\r{percent}% [{count * block_size / (1024*1024):.1f} MB / {total_size / (1024*1024):.1f} MB]")
        sys.stdout.flush()
    
    print(f"Downloading to: {output_path}")
    urllib.request.urlretrieve(url, output_path, reporthook)
    print("\n✓ Download complete")

def find_gguf_models():
    """Search for RWKV GGUF models on HuggingFace"""
    print("Searching for RWKV GGUF models on HuggingFace...")
    
    # Known GGUF model repositories
    known_models = [
        {
            "name": "RWKV-4-Pile-169M (Q8_0)",
            "repo": "latestissue/rwkv-4-pile-169m-gguf",
            "file": "rwkv-4-pile-169m-q8_0.gguf",
            "size": "~180 MB",
            "url": "https://huggingface.co/latestissue/rwkv-4-pile-169m-gguf/resolve/main/rwkv-4-pile-169m-q8_0.gguf"
        },
        {
            "name": "RWKV-4-Pile-169M (Q4_0)",
            "repo": "latestissue/rwkv-4-pile-169m-gguf",
            "file": "rwkv-4-pile-169m-q4_0.gguf",
            "size": "~100 MB",
            "url": "https://huggingface.co/latestissue/rwkv-4-pile-169m-gguf/resolve/main/rwkv-4-pile-169m-q4_0.gguf"
        },
        {
            "name": "RWKV-4-Pile-430M (Q8_0)",
            "repo": "latestissue/rwkv-4-pile-430m-gguf",
            "file": "rwkv-4-pile-430m-q8_0.gguf",
            "size": "~460 MB",
            "url": "https://huggingface.co/latestissue/rwkv-4-pile-430m-gguf/resolve/main/rwkv-4-pile-430m-q8_0.gguf"
        }
    ]
    
    return known_models

def main():
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    model_dir = project_root / "test" / "data" / "models"
    model_dir.mkdir(parents=True, exist_ok=True)
    
    print("=" * 60)
    print("RWKV Model Downloader")
    print("=" * 60)
    print(f"Model directory: {model_dir}")
    print()
    
    # Find available models
    models = find_gguf_models()
    
    print("Available models:")
    for i, model in enumerate(models, 1):
        print(f"\n{i}. {model['name']}")
        print(f"   Repository: {model['repo']}")
        print(f"   File: {model['file']}")
        print(f"   Size: {model['size']}")
    
    print("\n" + "=" * 60)
    
    # Download the smallest model (Q4_0 quantized)
    selected = models[1]  # Q4_0 version, smallest
    
    output_path = model_dir / selected['file']
    
    if output_path.exists():
        print(f"✓ Model already exists: {output_path}")
        print(f"  Size: {output_path.stat().st_size / (1024*1024):.1f} MB")
        return 0
    
    print(f"\nDownloading: {selected['name']}")
    print(f"URL: {selected['url']}")
    print()
    
    try:
        download_with_progress(selected['url'], str(output_path))
        
        # Verify download
        if output_path.exists():
            size_mb = output_path.stat().st_size / (1024*1024)
            print(f"\n✓ Model downloaded successfully!")
            print(f"  Path: {output_path}")
            print(f"  Size: {size_mb:.1f} MB")
            return 0
        else:
            print("\n✗ Download failed: File not found after download")
            return 1
            
    except Exception as e:
        print(f"\n✗ Download failed: {e}")
        print("\nAlternative: Download manually from:")
        print(f"  {selected['url']}")
        print(f"\nSave to: {output_path}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
