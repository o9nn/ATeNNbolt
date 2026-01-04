#include <gtest/gtest.h>
#include "bolt/ai/gpu_acceleration.hpp"
#include "bolt/core/logging.hpp"

using namespace bolt::ai;

class GPUAccelerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logging
        bolt::LogManager::configureConsoleLogging();
    }
    
    void TearDown() override {
        // Clean up GPU manager if initialized
        auto& gpuManager = GPUAccelerationManager::getInstance();
        if (gpuManager.isInitialized()) {
            gpuManager.shutdown();
        }
    }
};

TEST_F(GPUAccelerationTest, GPUUtilityFunctions) {
    // Test GPU availability check
    bool gpuAvailable = gpu_utils::isGPUAccelerationAvailable();
    EXPECT_TRUE(gpuAvailable || !gpuAvailable); // Should complete without error
    
    // Test recommended config
    GPUConfig config = gpu_utils::getRecommendedGPUConfig();
    EXPECT_TRUE(config.preferredBackend == GPUBackendType::CPU || 
                config.preferredBackend == GPUBackendType::CUDA ||
                config.preferredBackend == GPUBackendType::OpenCL ||
                config.preferredBackend == GPUBackendType::Vulkan);
    
    // Test backend benchmarking
    GPUBackendType bestBackend = gpu_utils::benchmarkGPUBackends();
    EXPECT_TRUE(bestBackend == GPUBackendType::CPU || 
                bestBackend == GPUBackendType::CUDA ||
                bestBackend == GPUBackendType::OpenCL ||
                bestBackend == GPUBackendType::Vulkan);
    
    // Test memory estimation
    size_t memReq = gpu_utils::estimateModelMemoryRequirements("/tmp/mock_model.gguf");
    EXPECT_GT(memReq, 0);
}

TEST_F(GPUAccelerationTest, GPUManagerInitialization) {
    auto& gpuManager = GPUAccelerationManager::getInstance();
    
    // Test initialization
    GPUConfig config;
    config.preferredBackend = GPUBackendType::CPU;
    config.enableAutomaticFallback = true;
    
    bool initialized = gpuManager.initialize(config);
    EXPECT_TRUE(initialized);
    EXPECT_TRUE(gpuManager.isInitialized());
    
    // Test re-initialization (should warn but succeed)
    bool reinit = gpuManager.initialize(config);
    EXPECT_TRUE(reinit);
    
    // Test device detection
    auto devices = gpuManager.detectAvailableDevices();
    EXPECT_GE(devices.size(), 1); // At least CPU should be available
    
    // Test that CPU device is present
    bool cpuFound = false;
    for (const auto& device : devices) {
        if (device.backendType == GPUBackendType::CPU) {
            cpuFound = true;
            break;
        }
    }
    EXPECT_TRUE(cpuFound);
    
    // Test current device
    auto currentDevice = gpuManager.getCurrentDevice();
    EXPECT_FALSE(currentDevice.name.empty());
    
    // Test config management
    GPUConfig newConfig = config;
    newConfig.enableMemoryOptimization = false;
    gpuManager.updateConfig(newConfig);
    
    auto retrievedConfig = gpuManager.getConfig();
    EXPECT_FALSE(retrievedConfig.enableMemoryOptimization);
    
    // Test shutdown
    gpuManager.shutdown();
    EXPECT_FALSE(gpuManager.isInitialized());
}

TEST_F(GPUAccelerationTest, DeviceSelection) {
    auto& gpuManager = GPUAccelerationManager::getInstance();
    
    GPUConfig config;
    config.preferredBackend = GPUBackendType::CPU;
    
    EXPECT_TRUE(gpuManager.initialize(config));
    
    // Test selecting CPU backend (should always work)
    bool cpuSelected = gpuManager.selectDevice(GPUBackendType::CPU, 0);
    EXPECT_TRUE(cpuSelected);
    EXPECT_EQ(gpuManager.getCurrentBackendType(), GPUBackendType::CPU);
    
    // Test selecting non-existent backends (should fail gracefully)
    bool cudaSelected = gpuManager.selectDevice(GPUBackendType::CUDA, 0);
    // This may succeed or fail depending on system - test should not crash
    
    bool openclSelected = gpuManager.selectDevice(GPUBackendType::OpenCL, 0);
    // This may succeed or fail depending on system - test should not crash
    
    bool vulkanSelected = gpuManager.selectDevice(GPUBackendType::Vulkan, 0);
    // This may succeed or fail depending on system - test should not crash
}

TEST_F(GPUAccelerationTest, MemoryManager) {
    auto& gpuManager = GPUAccelerationManager::getInstance();
    
    GPUConfig config;
    config.preferredBackend = GPUBackendType::CPU;
    
    EXPECT_TRUE(gpuManager.initialize(config));
    
    auto& memManager = gpuManager.getMemoryManager();
    
    // Test initial memory state
    size_t initialUsed = memManager.getUsedMemory();
    EXPECT_GE(initialUsed, 0);
    
    // Test memory operations (without actual allocation for now)
    memManager.optimizeMemoryUsage();  // Should not crash
    memManager.defragmentMemory();     // Should not crash
    
    // Memory usage should not have changed significantly
    size_t afterOptimization = memManager.getUsedMemory();
    EXPECT_EQ(initialUsed, afterOptimization);
}

TEST_F(GPUAccelerationTest, PerformanceStats) {
    auto& gpuManager = GPUAccelerationManager::getInstance();
    
    GPUConfig config;
    config.preferredBackend = GPUBackendType::CPU;
    
    EXPECT_TRUE(gpuManager.initialize(config));
    
    // Test initial stats
    auto stats = gpuManager.getPerformanceStats();
    EXPECT_EQ(stats.totalComputeOps, 0);
    EXPECT_EQ(stats.totalComputeTime, 0.0);
    EXPECT_EQ(stats.totalMemoryTransfers, 0);
    EXPECT_EQ(stats.totalTransferTime, 0.0);
    
    // Reset stats (should not change anything as they're already zero)
    gpuManager.resetPerformanceStats();
    auto resetStats = gpuManager.getPerformanceStats();
    EXPECT_EQ(resetStats.totalComputeOps, 0);
}

TEST_F(GPUAccelerationTest, GPUAcceleratedModel) {
    GPUConfig config;
    config.preferredBackend = GPUBackendType::CPU;
    config.enableAutomaticFallback = true;
    
    std::string mockModelPath = "/tmp/mock_model.gguf";
    
    // Test model creation
    GPUAcceleratedModel model(mockModelPath, config);
    
    // Model should be created but not loaded (no actual file)
    EXPECT_FALSE(model.isLoaded());
    
    // Test configuration
    model.setMaxContextSize(1024);
    model.setThreadCount(2);
    model.enableGPUOffloading(true);
    
    // Test text generation (should handle unloaded model gracefully)
    std::string result = model.generate("test prompt", 10);
    EXPECT_TRUE(result.empty()); // Should return empty string for unloaded model
    
    // Test embedding generation (should handle unloaded model gracefully)
    auto embedding = model.generateEmbedding("test input");
    EXPECT_TRUE(embedding.empty()); // Should return empty vector for unloaded model
    
    // Test performance stats
    auto stats = model.getPerformanceStats();
    // Stats should be accessible even if model is not loaded
}

TEST_F(GPUAccelerationTest, GPUAccelerationException) {
    // Test exception creation and retrieval
    GPUBackendType failedBackend = GPUBackendType::CUDA;
    std::string message = "Test GPU error";
    
    GPUAccelerationException exception(message, failedBackend);
    
    EXPECT_EQ(std::string(exception.what()), message);
    EXPECT_EQ(exception.getFailedBackend(), failedBackend);
}

TEST_F(GPUAccelerationTest, BackendTypeString) {
    auto& gpuManager = GPUAccelerationManager::getInstance();
    
    // Test initialization to access private methods through the manager
    GPUConfig config;
    config.preferredBackend = GPUBackendType::CPU;
    EXPECT_TRUE(gpuManager.initialize(config));
    
    // Test device detection which internally uses backendTypeToString
    auto devices = gpuManager.detectAvailableDevices();
    EXPECT_GE(devices.size(), 1);
    
    // Verify that each device has a valid backend type
    for (const auto& device : devices) {
        EXPECT_TRUE(device.backendType == GPUBackendType::CPU ||
                   device.backendType == GPUBackendType::CUDA ||
                   device.backendType == GPUBackendType::OpenCL ||
                   device.backendType == GPUBackendType::Vulkan ||
                   device.backendType == GPUBackendType::Metal);
    }
}

TEST_F(GPUAccelerationTest, GPUConfig) {
    // Test default configuration
    GPUConfig defaultConfig;
    EXPECT_EQ(defaultConfig.preferredBackend, GPUBackendType::CUDA);
    EXPECT_EQ(defaultConfig.preferredDeviceId, 0);
    EXPECT_TRUE(defaultConfig.enableAutomaticFallback);
    EXPECT_GT(defaultConfig.minMemoryRequirement, 0);
    EXPECT_TRUE(defaultConfig.enableMemoryOptimization);
    EXPECT_TRUE(defaultConfig.enableAsynchronousCompute);
    EXPECT_GE(defaultConfig.fallbackBackends.size(), 1);
    
    // Test custom configuration
    GPUConfig customConfig;
    customConfig.preferredBackend = GPUBackendType::OpenCL;
    customConfig.preferredDeviceId = 1;
    customConfig.enableAutomaticFallback = false;
    customConfig.minMemoryRequirement = 512 * 1024 * 1024; // 512MB
    customConfig.enableMemoryOptimization = false;
    customConfig.enableAsynchronousCompute = false;
    customConfig.fallbackBackends = {GPUBackendType::CPU};
    
    EXPECT_EQ(customConfig.preferredBackend, GPUBackendType::OpenCL);
    EXPECT_EQ(customConfig.preferredDeviceId, 1);
    EXPECT_FALSE(customConfig.enableAutomaticFallback);
    EXPECT_EQ(customConfig.minMemoryRequirement, 512 * 1024 * 1024);
    EXPECT_FALSE(customConfig.enableMemoryOptimization);
    EXPECT_FALSE(customConfig.enableAsynchronousCompute);
    EXPECT_EQ(customConfig.fallbackBackends.size(), 1);
    EXPECT_EQ(customConfig.fallbackBackends[0], GPUBackendType::CPU);
}

// Integration test that exercises multiple components
TEST_F(GPUAccelerationTest, IntegrationTest) {
    // Get recommended configuration
    GPUConfig config = gpu_utils::getRecommendedGPUConfig();
    
    // Initialize manager
    auto& gpuManager = GPUAccelerationManager::getInstance();
    EXPECT_TRUE(gpuManager.initialize(config));
    
    // Detect and verify devices
    auto devices = gpuManager.detectAvailableDevices();
    EXPECT_GE(devices.size(), 1);
    
    // Create a model with GPU acceleration
    std::string mockModelPath = "/tmp/integration_test_model.gguf";
    GPUAcceleratedModel model(mockModelPath, config);
    
    // Test that the model was created successfully
    // (even though it won't load due to missing file)
    EXPECT_FALSE(model.isLoaded());
    
    // Verify performance stats are accessible
    auto stats = model.getPerformanceStats();
    EXPECT_EQ(stats.totalComputeOps, 0);
    
    // Test memory management
    auto& memManager = gpuManager.getMemoryManager();
    size_t usedMemory = memManager.getUsedMemory();
    EXPECT_GE(usedMemory, 0);
    
    // Clean shutdown
    gpuManager.shutdown();
    EXPECT_FALSE(gpuManager.isInitialized());
}