#ifndef BOLT_GPU_ACCELERATION_HPP
#define BOLT_GPU_ACCELERATION_HPP

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <ggml.h>
#include <ggml-backend.h>
#include <ggml-cpu.h>

// Optional GPU backend headers (conditionally included based on availability)
#ifdef GGML_USE_CUDA
#include <ggml-cuda.h>
#endif

#ifdef GGML_USE_OPENCL
#include <ggml-opencl.h>
#endif

#ifdef GGML_USE_VULKAN
#include <ggml-vulkan.h>
#endif

namespace bolt {
namespace ai {

/**
 * Enum for GPU backend types
 */
enum class GPUBackendType {
    CPU,      // Fallback CPU processing
    CUDA,     // NVIDIA CUDA
    OpenCL,   // Cross-platform OpenCL
    Vulkan,   // Modern Vulkan compute
    Metal     // Apple Metal (future)
};

/**
 * GPU device information structure
 */
struct GPUDeviceInfo {
    int deviceId;
    std::string name;
    std::string description;
    size_t totalMemory;
    size_t availableMemory;
    GPUBackendType backendType;
    bool isAvailable;
};

/**
 * GPU acceleration configuration
 */
struct GPUConfig {
    GPUBackendType preferredBackend = GPUBackendType::CUDA;
    std::vector<GPUBackendType> fallbackBackends = {
        GPUBackendType::Vulkan,
        GPUBackendType::OpenCL,
        GPUBackendType::CPU
    };
    int preferredDeviceId = 0;
    bool enableAutomaticFallback = true;
    size_t minMemoryRequirement = 1024 * 1024 * 1024; // 1GB minimum
    bool enableMemoryOptimization = true;
    bool enableAsynchronousCompute = true;
};

/**
 * GPU memory manager for efficient allocation and deallocation
 */
class GPUMemoryManager {
public:
    GPUMemoryManager(ggml_backend_t backend, ggml_backend_buffer_type_t bufferType);
    ~GPUMemoryManager();

    // Memory allocation
    ggml_backend_buffer_t allocateBuffer(size_t size);
    void deallocateBuffer(ggml_backend_buffer_t buffer);
    
    // Memory monitoring
    size_t getTotalMemory() const;
    size_t getUsedMemory() const;
    size_t getAvailableMemory() const;
    
    // Memory optimization
    void optimizeMemoryUsage();
    void defragmentMemory();

private:
    ggml_backend_t backend_;
    ggml_backend_buffer_type_t bufferType_;
    std::vector<ggml_backend_buffer_t> allocatedBuffers_;
    size_t totalAllocated_;
    size_t peakUsage_;
};

/**
 * Main GPU acceleration manager
 */
class GPUAccelerationManager {
public:
    static GPUAccelerationManager& getInstance();
    
    // Initialization and configuration
    bool initialize(const GPUConfig& config = GPUConfig{});
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Device detection and selection
    std::vector<GPUDeviceInfo> detectAvailableDevices();
    bool selectDevice(GPUBackendType backendType, int deviceId = 0);
    GPUDeviceInfo getCurrentDevice() const;
    
    // Backend management
    ggml_backend_t getBackend() const { return currentBackend_; }
    GPUBackendType getCurrentBackendType() const { return currentBackendType_; }
    ggml_backend_buffer_type_t getBufferType() const { return currentBufferType_; }
    
    // Memory management
    GPUMemoryManager& getMemoryManager() { return *memoryManager_; }
    
    // Model operations
    bool loadModelToGPU(ggml_context* ctx, const std::vector<ggml_tensor*>& tensors);
    bool transferTensorToGPU(ggml_tensor* tensor);
    bool transferTensorToCPU(ggml_tensor* tensor);
    
    // Compute operations
    bool computeGraph(ggml_cgraph* graph);
    bool computeGraphAsync(ggml_cgraph* graph);
    void synchronize();
    
    // Performance monitoring
    struct PerformanceStats {
        size_t totalComputeOps = 0;
        double totalComputeTime = 0.0;
        size_t totalMemoryTransfers = 0;
        double totalTransferTime = 0.0;
        double averageOpsPerSecond = 0.0;
        double peakMemoryUsage = 0.0;
    };
    
    PerformanceStats getPerformanceStats() const { return performanceStats_; }
    void resetPerformanceStats();
    
    // Configuration management
    void updateConfig(const GPUConfig& config);
    GPUConfig getConfig() const { return config_; }

private:
    GPUAccelerationManager() = default;
    ~GPUAccelerationManager() = default;
    
    // Prevent copying
    GPUAccelerationManager(const GPUAccelerationManager&) = delete;
    GPUAccelerationManager& operator=(const GPUAccelerationManager&) = delete;
    
    // Backend initialization helpers
    bool initializeCUDA(int deviceId = 0);
    bool initializeOpenCL();
    bool initializeVulkan(int deviceId = 0);
    bool initializeCPU();
    
    // Device detection helpers
    std::vector<GPUDeviceInfo> detectCUDADevices();
    std::vector<GPUDeviceInfo> detectOpenCLDevices();
    std::vector<GPUDeviceInfo> detectVulkanDevices();
    
    // Utility functions
    bool isBackendAvailable(GPUBackendType backendType);
    std::string backendTypeToString(GPUBackendType backendType);
    
    // Member variables
    bool initialized_ = false;
    GPUConfig config_;
    
    ggml_backend_t currentBackend_ = nullptr;
    GPUBackendType currentBackendType_ = GPUBackendType::CPU;
    ggml_backend_buffer_type_t currentBufferType_ = nullptr;
    GPUDeviceInfo currentDevice_;
    
    std::unique_ptr<GPUMemoryManager> memoryManager_;
    std::vector<GPUDeviceInfo> availableDevices_;
    
    PerformanceStats performanceStats_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

/**
 * GPU-accelerated model wrapper
 * Extends the existing GGMLModel with GPU capabilities
 */
class GPUAcceleratedModel {
public:
    GPUAcceleratedModel(const std::string& modelPath, const GPUConfig& config = GPUConfig{});
    ~GPUAcceleratedModel();
    
    // Model loading and initialization
    bool loadModel(const std::string& path);
    bool isLoaded() const { return modelLoaded_; }
    bool isGPUAccelerated() const { return gpuAccelerated_; }
    
    // Inference operations
    std::string generate(const std::string& prompt, size_t maxTokens = 100);
    std::vector<float> generateEmbedding(const std::string& input);
    
    // Configuration
    void setMaxContextSize(size_t contextSize) { maxContextSize_ = contextSize; }
    void setThreadCount(int threads) { threadCount_ = threads; }
    void enableGPUOffloading(bool enable) { gpuOffloadingEnabled_ = enable; }
    
    // Performance monitoring
    GPUAccelerationManager::PerformanceStats getPerformanceStats() const;

private:
    // Model state
    std::unique_ptr<ggml_context, void(*)(ggml_context*)> context_{nullptr, ggml_free};
    std::vector<ggml_tensor*> modelTensors_;
    std::string modelPath_;
    bool modelLoaded_ = false;
    bool gpuAccelerated_ = false;
    bool gpuOffloadingEnabled_ = true;
    
    // Configuration
    GPUConfig gpuConfig_;
    size_t maxContextSize_ = 2048;
    int threadCount_ = -1;
    
    // Helper methods
    bool setupGPUContext();
    bool loadModelWeights();
    ggml_tensor* runInference(const std::vector<int>& tokens);
    std::vector<int> tokenize(const std::string& text);
    std::string detokenize(const std::vector<int>& tokens);
};

/**
 * Exception class for GPU acceleration errors
 */
class GPUAccelerationException : public std::runtime_error {
public:
    explicit GPUAccelerationException(const std::string& message, GPUBackendType failedBackend)
        : std::runtime_error(message), failedBackend_(failedBackend) {}
    
    GPUBackendType getFailedBackend() const { return failedBackend_; }

private:
    GPUBackendType failedBackend_;
};

// Utility functions
namespace gpu_utils {
    /**
     * Check if GPU acceleration is available on this system
     */
    bool isGPUAccelerationAvailable();
    
    /**
     * Get recommended GPU configuration for this system
     */
    GPUConfig getRecommendedGPUConfig();
    
    /**
     * Benchmark different GPU backends and return the best performing one
     */
    GPUBackendType benchmarkGPUBackends();
    
    /**
     * Estimate memory requirements for a model
     */
    size_t estimateModelMemoryRequirements(const std::string& modelPath);
}

} // namespace ai
} // namespace bolt

#endif // BOLT_GPU_ACCELERATION_HPP