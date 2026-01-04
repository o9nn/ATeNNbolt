#include <iostream>
#include <vector>
#include <chrono>
#include "bolt/ai/gpu_acceleration.hpp"
#include "bolt/core/logging.hpp"

using namespace bolt::ai;

int main() {
    std::cout << "=== GPU Acceleration Demo for Bolt C++ ===" << std::endl;
    
    // Initialize logging
    bolt::LogManager::configureConsoleLogging();
    
    // Check if GPU acceleration is available
    std::cout << "\n1. Checking GPU availability..." << std::endl;
    bool gpuAvailable = gpu_utils::isGPUAccelerationAvailable();
    std::cout << "GPU acceleration available: " << (gpuAvailable ? "Yes" : "No") << std::endl;
    
    // Get recommended GPU configuration
    std::cout << "\n2. Getting recommended GPU configuration..." << std::endl;
    GPUConfig recommendedConfig = gpu_utils::getRecommendedGPUConfig();
    std::cout << "Recommended backend: ";
    switch (recommendedConfig.preferredBackend) {
        case GPUBackendType::CUDA:   std::cout << "CUDA"; break;
        case GPUBackendType::OpenCL: std::cout << "OpenCL"; break;
        case GPUBackendType::Vulkan: std::cout << "Vulkan"; break;
        case GPUBackendType::CPU:    std::cout << "CPU"; break;
        default:                     std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
    
    // Initialize GPU acceleration manager
    std::cout << "\n3. Initializing GPU acceleration manager..." << std::endl;
    auto& gpuManager = GPUAccelerationManager::getInstance();
    
    if (!gpuManager.initialize(recommendedConfig)) {
        std::cerr << "Failed to initialize GPU acceleration manager" << std::endl;
        return 1;
    }
    
    // Detect available devices
    std::cout << "\n4. Detecting available GPU devices..." << std::endl;
    auto devices = gpuManager.detectAvailableDevices();
    std::cout << "Found " << devices.size() << " device(s):" << std::endl;
    
    for (size_t i = 0; i < devices.size(); i++) {
        const auto& device = devices[i];
        std::cout << "  Device " << i << ": " << device.name << std::endl;
        std::cout << "    Type: ";
        switch (device.backendType) {
            case GPUBackendType::CUDA:   std::cout << "CUDA"; break;
            case GPUBackendType::OpenCL: std::cout << "OpenCL"; break;
            case GPUBackendType::Vulkan: std::cout << "Vulkan"; break;
            case GPUBackendType::CPU:    std::cout << "CPU"; break;
            default:                     std::cout << "Unknown"; break;
        }
        std::cout << std::endl;
        std::cout << "    Available: " << (device.isAvailable ? "Yes" : "No") << std::endl;
        if (device.totalMemory > 0) {
            std::cout << "    Total Memory: " << (device.totalMemory / (1024 * 1024)) << " MB" << std::endl;
            std::cout << "    Available Memory: " << (device.availableMemory / (1024 * 1024)) << " MB" << std::endl;
        }
        std::cout << "    Description: " << device.description << std::endl;
    }
    
    // Get current device information
    std::cout << "\n5. Current device information..." << std::endl;
    auto currentDevice = gpuManager.getCurrentDevice();
    std::cout << "Currently using: " << currentDevice.name << " (" << currentDevice.description << ")" << std::endl;
    
    // Test GPU-accelerated model
    std::cout << "\n6. Testing GPU-accelerated model..." << std::endl;
    
    // Configure GPU settings
    GPUConfig gpuConfig;
    gpuConfig.enableAutomaticFallback = true;
    gpuConfig.enableMemoryOptimization = true;
    
    // Create a mock model path (in real use, this would be a path to an actual model file)
    std::string mockModelPath = "/tmp/mock_model.gguf";
    
    try {
        GPUAcceleratedModel model(mockModelPath, gpuConfig);
        
        std::cout << "Model created successfully" << std::endl;
        std::cout << "GPU accelerated: " << (model.isGPUAccelerated() ? "Yes" : "No") << std::endl;
        
        // Test text generation (simplified)
        std::cout << "\n7. Testing text generation..." << std::endl;
        std::string prompt = "Hello, this is a test prompt";
        
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string response = model.generate(prompt, 50);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "Prompt: " << prompt << std::endl;
        std::cout << "Generated response: " << response << std::endl;
        std::cout << "Generation time: " << duration.count() << " ms" << std::endl;
        
        // Test embedding generation
        std::cout << "\n8. Testing embedding generation..." << std::endl;
        auto embedding = model.generateEmbedding("Test text for embedding");
        std::cout << "Generated embedding dimension: " << embedding.size() << std::endl;
        if (!embedding.empty()) {
            std::cout << "First few values: ";
            for (size_t i = 0; i < std::min(size_t(5), embedding.size()); i++) {
                std::cout << embedding[i] << " ";
            }
            std::cout << "..." << std::endl;
        }
        
        // Display performance statistics
        std::cout << "\n9. Performance statistics..." << std::endl;
        auto stats = model.getPerformanceStats();
        std::cout << "Total compute operations: " << stats.totalComputeOps << std::endl;
        std::cout << "Total compute time: " << stats.totalComputeTime << " seconds" << std::endl;
        std::cout << "Total memory transfers: " << stats.totalMemoryTransfers << std::endl;
        std::cout << "Total transfer time: " << stats.totalTransferTime << " seconds" << std::endl;
        if (stats.totalComputeTime > 0) {
            std::cout << "Average ops per second: " << stats.averageOpsPerSecond << std::endl;
        }
        
    } catch (const GPUAccelerationException& e) {
        std::cerr << "GPU acceleration error: " << e.what() << std::endl;
        std::cout << "Failed backend: ";
        switch (e.getFailedBackend()) {
            case GPUBackendType::CUDA:   std::cout << "CUDA"; break;
            case GPUBackendType::OpenCL: std::cout << "OpenCL"; break;
            case GPUBackendType::Vulkan: std::cout << "Vulkan"; break;
            case GPUBackendType::CPU:    std::cout << "CPU"; break;
            default:                     std::cout << "Unknown"; break;
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // Test memory management
    std::cout << "\n10. Memory management test..." << std::endl;
    if (gpuManager.isInitialized()) {
        auto& memManager = gpuManager.getMemoryManager();
        std::cout << "Used GPU memory: " << (memManager.getUsedMemory() / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "Available GPU memory: " << (memManager.getAvailableMemory() / (1024 * 1024)) << " MB" << std::endl;
        
        // Test memory optimization
        memManager.optimizeMemoryUsage();
        memManager.defragmentMemory();
    }
    
    // Benchmark different backends (if available)
    std::cout << "\n11. Benchmarking GPU backends..." << std::endl;
    GPUBackendType bestBackend = gpu_utils::benchmarkGPUBackends();
    std::cout << "Best performing backend: ";
    switch (bestBackend) {
        case GPUBackendType::CUDA:   std::cout << "CUDA"; break;
        case GPUBackendType::OpenCL: std::cout << "OpenCL"; break;
        case GPUBackendType::Vulkan: std::cout << "Vulkan"; break;
        case GPUBackendType::CPU:    std::cout << "CPU"; break;
        default:                     std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
    
    // Cleanup
    std::cout << "\n12. Shutting down..." << std::endl;
    gpuManager.shutdown();
    
    std::cout << "\n=== GPU Acceleration Demo Complete ===" << std::endl;
    
    return 0;
}