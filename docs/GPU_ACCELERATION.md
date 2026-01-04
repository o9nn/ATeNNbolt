# GPU Acceleration for Bolt C++ AI Models

## Overview

The Bolt C++ GPU acceleration system provides hardware-accelerated AI model inference across multiple backends including CUDA, OpenCL, Vulkan, and CPU fallback. This implementation extends the existing GGML integration to support modern GPU computing platforms.

## Features

### Multi-Backend Support
- **CUDA**: NVIDIA GPU acceleration with cuBLAS optimization
- **OpenCL**: Cross-platform GPU acceleration
- **Vulkan**: Modern compute shaders and GPU compute
- **CPU Fallback**: Always available fallback using optimized CPU backends

### Architecture Components

#### GPUAccelerationManager
Central manager for GPU backend initialization and device management.

```cpp
auto& gpuManager = GPUAccelerationManager::getInstance();
GPUConfig config;
config.preferredBackend = GPUBackendType::CUDA;
config.enableAutomaticFallback = true;

if (gpuManager.initialize(config)) {
    // GPU acceleration ready
}
```

#### GPUMemoryManager  
Efficient GPU memory allocation and optimization.

```cpp
auto& memManager = gpuManager.getMemoryManager();
auto buffer = memManager.allocateBuffer(1024 * 1024); // 1MB
memManager.optimizeMemoryUsage();
memManager.defragmentMemory();
```

#### GPUAcceleratedModel
Enhanced AI model wrapper with GPU inference capabilities.

```cpp
GPUConfig config;
config.preferredBackend = GPUBackendType::CUDA;

GPUAcceleratedModel model("/path/to/model.gguf", config);
std::string response = model.generate("Hello world", 100);
auto embedding = model.generateEmbedding("Input text");
```

#### Device Detection
Automatic detection of available GPU devices with detailed information.

```cpp
auto devices = gpuManager.detectAvailableDevices();
for (const auto& device : devices) {
    std::cout << "Device: " << device.name << std::endl;
    std::cout << "Type: " << backendTypeToString(device.backendType) << std::endl;
    std::cout << "Memory: " << (device.totalMemory / 1024 / 1024) << " MB" << std::endl;
}
```

### Performance Monitoring

Real-time performance statistics tracking:

```cpp
auto stats = gpuManager.getPerformanceStats();
std::cout << "Total operations: " << stats.totalComputeOps << std::endl;
std::cout << "Average ops/sec: " << stats.averageOpsPerSecond << std::endl;
std::cout << "Memory transfers: " << stats.totalMemoryTransfers << std::endl;
```

### Configuration Management

Flexible configuration system with fallback priorities:

```cpp
GPUConfig config;
config.preferredBackend = GPUBackendType::CUDA;
config.fallbackBackends = {
    GPUBackendType::Vulkan,
    GPUBackendType::OpenCL,
    GPUBackendType::CPU
};
config.enableAutomaticFallback = true;
config.minMemoryRequirement = 1024 * 1024 * 1024; // 1GB
config.enableMemoryOptimization = true;
config.enableAsynchronousCompute = true;
```

## API Reference

### Core Classes

#### GPUAccelerationManager
- `getInstance()`: Get singleton instance
- `initialize(config)`: Initialize with configuration
- `detectAvailableDevices()`: Find available GPU devices
- `selectDevice(type, id)`: Select specific device
- `getCurrentDevice()`: Get current device info
- `getMemoryManager()`: Access memory manager
- `computeGraph(graph)`: Execute computation graph
- `shutdown()`: Clean shutdown

#### GPUAcceleratedModel  
- `GPUAcceleratedModel(path, config)`: Constructor
- `loadModel(path)`: Load model from file
- `generate(prompt, maxTokens)`: Generate text
- `generateEmbedding(input)`: Generate embeddings
- `isGPUAccelerated()`: Check if using GPU
- `getPerformanceStats()`: Get performance metrics

#### Utility Functions
- `gpu_utils::isGPUAccelerationAvailable()`: Check availability
- `gpu_utils::getRecommendedGPUConfig()`: Get optimal config  
- `gpu_utils::benchmarkGPUBackends()`: Find best backend
- `gpu_utils::estimateModelMemoryRequirements()`: Memory estimation

### Error Handling

```cpp
try {
    GPUAcceleratedModel model("model.gguf");
    auto result = model.generate("prompt");
} catch (const GPUAccelerationException& e) {
    std::cerr << "GPU Error: " << e.what() << std::endl;
    std::cerr << "Failed backend: " << e.getFailedBackend() << std::endl;
}
```

## Integration with GGML

The GPU acceleration system builds on top of the existing GGML backend system:

### GGML Backend Integration
```cpp
// CUDA backend
ggml_backend_t cuda_backend = ggml_backend_cuda_init(device_id);
ggml_backend_buffer_type_t cuda_buffer = ggml_backend_cuda_buffer_type(device_id);

// OpenCL backend  
ggml_backend_t opencl_backend = ggml_backend_opencl_init();
ggml_backend_buffer_type_t opencl_buffer = ggml_backend_opencl_buffer_type();

// Vulkan backend
ggml_backend_t vulkan_backend = ggml_backend_vk_init(device_id);
ggml_backend_buffer_type_t vulkan_buffer = ggml_backend_vk_buffer_type(device_id);
```

### Graph Computation
```cpp
ggml_cgraph* graph = ggml_new_graph(context);
// Build computation graph...
ggml_backend_graph_compute(backend, graph);
```

## Building with GPU Support

### CMake Configuration
GPU acceleration is automatically enabled when GGML is built with GPU support:

```cmake
# Enable CUDA
set(GGML_CUDA ON)

# Enable OpenCL
set(GGML_OPENCL ON)  

# Enable Vulkan
set(GGML_VULKAN ON)
```

### Dependencies
- **CUDA**: NVIDIA CUDA Toolkit 11.0+
- **OpenCL**: OpenCL 3.0+ implementation
- **Vulkan**: Vulkan SDK 1.3+
- **GGML**: Latest version with GPU backend support

## Examples

### Basic GPU Acceleration
```cpp
#include "bolt/ai/gpu_acceleration.hpp"

int main() {
    // Initialize GPU acceleration
    auto& gpuManager = GPUAccelerationManager::getInstance();
    GPUConfig config = gpu_utils::getRecommendedGPUConfig();
    
    if (!gpuManager.initialize(config)) {
        std::cerr << "GPU initialization failed" << std::endl;
        return 1;
    }
    
    // Create GPU-accelerated model
    GPUAcceleratedModel model("model.gguf", config);
    
    // Generate text with GPU acceleration
    std::string response = model.generate("Hello world", 100);
    std::cout << "Response: " << response << std::endl;
    
    // Cleanup
    gpuManager.shutdown();
    return 0;
}
```

### Device Selection
```cpp
auto devices = gpuManager.detectAvailableDevices();

// Find best CUDA device
for (const auto& device : devices) {
    if (device.backendType == GPUBackendType::CUDA && 
        device.totalMemory > 4ULL * 1024 * 1024 * 1024) { // > 4GB
        gpuManager.selectDevice(GPUBackendType::CUDA, device.deviceId);
        break;
    }
}
```

### Performance Monitoring
```cpp
auto startStats = model.getPerformanceStats();

// Perform operations...
model.generate("test", 50);

auto endStats = model.getPerformanceStats();
double opsPerformed = endStats.totalComputeOps - startStats.totalComputeOps;
double timeElapsed = endStats.totalComputeTime - startStats.totalComputeTime;
double throughput = opsPerformed / timeElapsed;

std::cout << "Throughput: " << throughput << " ops/sec" << std::endl;
```

## Testing

Run the comprehensive test suite:

```bash
# Build and run tests
cd build
make test_gpu_acceleration
./test_gpu_acceleration

# Run standalone demo
make demo_gpu_acceleration
./demo_gpu_acceleration
```

Simple standalone test:
```bash
# Compile and run simple test
g++ -std=c++17 -I./ggml/ggml.cpp/include test_gpu_simple.cpp \
    -L./build/ggml/ggml.cpp/src -lggml -lggml-cpu -lggml-base -pthread \
    -o test_gpu_simple
    
LD_LIBRARY_PATH=./build/ggml/ggml.cpp/src ./test_gpu_simple
```

## Performance Considerations

### Memory Management
- Use `GPUMemoryManager` for efficient allocation
- Enable memory optimization for better performance
- Monitor memory usage with performance stats

### Backend Selection
- CUDA generally provides best performance on NVIDIA GPUs
- Vulkan offers good cross-vendor compatibility  
- OpenCL provides broad compatibility but may have overhead
- CPU fallback ensures universal compatibility

### Optimization Tips
- Enable asynchronous compute when possible
- Use appropriate tensor data types (F16 vs F32)
- Batch operations for better GPU utilization
- Profile different backends for your specific use case

## Troubleshooting

### Common Issues

1. **GPU not detected**: Ensure proper drivers are installed
2. **Out of memory**: Reduce model size or context length
3. **Slow performance**: Check if GPU is actually being used
4. **Backend initialization failure**: Verify GPU backend dependencies

### Debug Information
Enable detailed logging for troubleshooting:
```cpp
gpuManager.getCurrentDevice(); // Check current device
auto stats = gpuManager.getPerformanceStats(); // Monitor performance
```

## Future Enhancements

- **Multi-GPU support**: Distribute computation across multiple GPUs
- **Mixed precision**: Automatic FP16/FP32 optimization
- **Model sharding**: Support for models larger than GPU memory
- **Advanced profiling**: Detailed GPU kernel performance analysis
- **Custom kernels**: Domain-specific GPU kernel optimization

---

*The GPU acceleration system provides a modern, efficient foundation for accelerating AI model inference while maintaining compatibility and ease of use.*