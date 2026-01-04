#ifndef BOLT_PERFORMANCE_PROFILER_HPP
#define BOLT_PERFORMANCE_PROFILER_HPP

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <thread>
#include <fstream>
#include "logging.hpp"
#include "thread_safety.hpp"

namespace bolt {

// Forward declarations
class PerformanceProfiler;
class ProfilerSession;

// Performance metrics data structure
struct PerformanceMetric {
    std::string name;
    std::string category;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::chrono::duration<double, std::milli> duration;
    size_t memoryUsage;
    double cpuUsage;
    std::thread::id threadId;
    std::unordered_map<std::string, std::string> metadata;
    
    PerformanceMetric(const std::string& metricName, const std::string& metricCategory)
        : name(metricName), category(metricCategory), 
          startTime(std::chrono::steady_clock::now()),
          memoryUsage(0), cpuUsage(0.0),
          threadId(std::this_thread::get_id()) {}
    
    void finish() {
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(endTime - startTime);
    }
    
    double getDurationMs() const {
        return duration.count();
    }
};

// Profiler session for grouping related metrics
class ProfilerSession {
private:
    std::string sessionName_;
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point endTime_;
    std::vector<std::shared_ptr<PerformanceMetric>> metrics_;
    mutable std::mutex metricsMutex_;
    bool isActive_;
    
public:
    explicit ProfilerSession(const std::string& name);
    ~ProfilerSession();
    
    const std::string& getName() const { return sessionName_; }
    bool isActive() const { return isActive_; }
    
    void start();
    void stop();
    void addMetric(std::shared_ptr<PerformanceMetric> metric);
    
    std::vector<std::shared_ptr<PerformanceMetric>> getMetrics() const;
    double getTotalDurationMs() const;
    size_t getMetricsCount() const;
    
    // Statistics
    double getAverageDurationMs() const;
    double getMinDurationMs() const;
    double getMaxDurationMs() const;
};

// Main performance profiler class
class PerformanceProfiler {
private:
    static std::unique_ptr<PerformanceProfiler> instance_;
    static std::mutex instanceMutex_;
    
    mutable std::mutex sessionsMutex_;
    mutable std::mutex metricsMutex_;
    
    std::unordered_map<std::string, std::shared_ptr<ProfilerSession>> sessions_;
    std::vector<std::shared_ptr<PerformanceMetric>> globalMetrics_;
    
    std::atomic<bool> isEnabled_;
    std::atomic<bool> isSystemMonitoringEnabled_;
    
    // Thread-local current session storage
    thread_local static std::shared_ptr<ProfilerSession> currentSession_;
    
    PerformanceProfiler();
    
public:
    static PerformanceProfiler& getInstance();
    
    // Core functionality
    void enable() { isEnabled_ = true; }
    void disable() { isEnabled_ = false; }
    bool isEnabled() const { return isEnabled_; }
    
    void enableSystemMonitoring() { isSystemMonitoringEnabled_ = true; }
    void disableSystemMonitoring() { isSystemMonitoringEnabled_ = false; }
    bool isSystemMonitoringEnabled() const { return isSystemMonitoringEnabled_; }
    
    // Session management
    std::shared_ptr<ProfilerSession> createSession(const std::string& name);
    std::shared_ptr<ProfilerSession> getSession(const std::string& name) const;
    void setCurrentSession(const std::string& name);
    std::shared_ptr<ProfilerSession> getCurrentSession() const;
    void endSession(const std::string& name);
    
    // Metric recording
    std::shared_ptr<PerformanceMetric> startMetric(const std::string& name, const std::string& category = "GENERAL");
    void endMetric(std::shared_ptr<PerformanceMetric> metric);
    void recordInstantMetric(const std::string& name, double value, const std::string& category = "GENERAL");
    
    // System monitoring
    size_t getCurrentMemoryUsage() const;
    double getCurrentCPUUsage() const;
    
    // Data export and reporting
    void exportToJson(const std::string& filename) const;
    void exportToCsv(const std::string& filename) const;
    void printSummary() const;
    void printDetailedReport() const;
    
    // Integration with logging system
    void logMetric(const PerformanceMetric& metric, LogLevel level = LogLevel::INFO) const;
    void logSessionSummary(const ProfilerSession& session, LogLevel level = LogLevel::INFO) const;
    
    // Cleanup
    void reset();
    void clearOldMetrics(std::chrono::minutes olderThan);
    
    // Statistics
    size_t getTotalMetricsCount() const;
    double getAverageMetricDuration() const;
    std::vector<std::string> getCategories() const;
    std::vector<std::shared_ptr<PerformanceMetric>> getMetricsByCategory(const std::string& category) const;
};

// RAII performance measurement class that integrates with existing ScopedLogger
class ScopedProfiler {
private:
    std::shared_ptr<PerformanceMetric> metric_;
    bool wasEnabled_;
    
public:
    ScopedProfiler(const std::string& name, const std::string& category = "GENERAL");
    ~ScopedProfiler();
    
    void addMetadata(const std::string& key, const std::string& value);
    double getCurrentDurationMs() const;
};

// Convenience macros
#define BOLT_PROFILE_FUNCTION() \
    bolt::ScopedProfiler __profiler__(__FUNCTION__, "FUNCTION")

#define BOLT_PROFILE_SCOPE(name) \
    bolt::ScopedProfiler __profiler__(name, "SCOPE")

#define BOLT_PROFILE_CATEGORY(name, category) \
    bolt::ScopedProfiler __profiler__(name, category)

#define BOLT_PROFILE_SESSION(sessionName) \
    auto __session__ = bolt::PerformanceProfiler::getInstance().createSession(sessionName); \
    bolt::PerformanceProfiler::getInstance().setCurrentSession(sessionName)

// Integration macros with existing logging categories
#define BOLT_PROFILE_CORE(name) BOLT_PROFILE_CATEGORY(name, "CORE")
#define BOLT_PROFILE_MEMORY(name) BOLT_PROFILE_CATEGORY(name, "MEMORY")
#define BOLT_PROFILE_EDITOR(name) BOLT_PROFILE_CATEGORY(name, "EDITOR")
#define BOLT_PROFILE_AI(name) BOLT_PROFILE_CATEGORY(name, "AI")
#define BOLT_PROFILE_GUI(name) BOLT_PROFILE_CATEGORY(name, "GUI")
#define BOLT_PROFILE_NETWORK(name) BOLT_PROFILE_CATEGORY(name, "NETWORK")

} // namespace bolt

#endif // BOLT_PERFORMANCE_PROFILER_HPP