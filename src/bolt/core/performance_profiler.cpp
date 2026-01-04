#include "bolt/core/performance_profiler.hpp"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <unordered_set>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/resource.h>
#include <sys/time.h>
#endif

namespace bolt {

// Static member definitions
std::unique_ptr<PerformanceProfiler> PerformanceProfiler::instance_;
std::mutex PerformanceProfiler::instanceMutex_;

// Thread-local current session
thread_local std::shared_ptr<ProfilerSession> PerformanceProfiler::currentSession_;

// ProfilerSession implementation
ProfilerSession::ProfilerSession(const std::string& name)
    : sessionName_(name), isActive_(false) {
}

ProfilerSession::~ProfilerSession() {
    if (isActive_) {
        stop();
    }
}

void ProfilerSession::start() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    startTime_ = std::chrono::steady_clock::now();
    isActive_ = true;
}

void ProfilerSession::stop() {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    endTime_ = std::chrono::steady_clock::now();
    isActive_ = false;
}

void ProfilerSession::addMetric(std::shared_ptr<PerformanceMetric> metric) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    metrics_.push_back(metric);
}

std::vector<std::shared_ptr<PerformanceMetric>> ProfilerSession::getMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return metrics_;
}

double ProfilerSession::getTotalDurationMs() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    if (!isActive_ && startTime_.time_since_epoch().count() > 0 && endTime_.time_since_epoch().count() > 0) {
        auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(endTime_ - startTime_);
        return duration.count();
    }
    return 0.0;
}

size_t ProfilerSession::getMetricsCount() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return metrics_.size();
}

double ProfilerSession::getAverageDurationMs() const {
    auto metrics = getMetrics();
    if (metrics.empty()) return 0.0;
    
    double total = 0.0;
    for (const auto& metric : metrics) {
        total += metric->getDurationMs();
    }
    return total / metrics.size();
}

double ProfilerSession::getMinDurationMs() const {
    auto metrics = getMetrics();
    if (metrics.empty()) return 0.0;
    
    double minDuration = metrics[0]->getDurationMs();
    for (const auto& metric : metrics) {
        minDuration = std::min(minDuration, metric->getDurationMs());
    }
    return minDuration;
}

double ProfilerSession::getMaxDurationMs() const {
    auto metrics = getMetrics();
    if (metrics.empty()) return 0.0;
    
    double maxDuration = metrics[0]->getDurationMs();
    for (const auto& metric : metrics) {
        maxDuration = std::max(maxDuration, metric->getDurationMs());
    }
    return maxDuration;
}

// PerformanceProfiler implementation
PerformanceProfiler::PerformanceProfiler() 
    : isEnabled_(true), isSystemMonitoringEnabled_(false) {
}

PerformanceProfiler& PerformanceProfiler::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (!instance_) {
        instance_ = std::unique_ptr<PerformanceProfiler>(new PerformanceProfiler());
    }
    return *instance_;
}

std::shared_ptr<ProfilerSession> PerformanceProfiler::createSession(const std::string& name) {
    if (!isEnabled_) return nullptr;
    
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto session = std::make_shared<ProfilerSession>(name);
    sessions_[name] = session;
    session->start();
    return session;
}

std::shared_ptr<ProfilerSession> PerformanceProfiler::getSession(const std::string& name) const {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto it = sessions_.find(name);
    return (it != sessions_.end()) ? it->second : nullptr;
}

void PerformanceProfiler::setCurrentSession(const std::string& name) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto it = sessions_.find(name);
    if (it != sessions_.end()) {
        currentSession_ = it->second;
    }
}

std::shared_ptr<ProfilerSession> PerformanceProfiler::getCurrentSession() const {
    return currentSession_;
}

void PerformanceProfiler::endSession(const std::string& name) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    auto it = sessions_.find(name);
    if (it != sessions_.end()) {
        it->second->stop();
        if (currentSession_ == it->second) {
            currentSession_ = nullptr;
        }
    }
}

std::shared_ptr<PerformanceMetric> PerformanceProfiler::startMetric(const std::string& name, const std::string& category) {
    if (!isEnabled_) return nullptr;
    
    auto metric = std::make_shared<PerformanceMetric>(name, category);
    
    // Add system monitoring if enabled
    if (isSystemMonitoringEnabled_) {
        metric->memoryUsage = getCurrentMemoryUsage();
        metric->cpuUsage = getCurrentCPUUsage();
    }
    
    return metric;
}

void PerformanceProfiler::endMetric(std::shared_ptr<PerformanceMetric> metric) {
    if (!metric || !isEnabled_) return;
    
    metric->finish();
    
    // Add to current session if available
    auto currentSession = getCurrentSession();
    if (currentSession && currentSession->isActive()) {
        currentSession->addMetric(metric);
    } else {
        // Add to global metrics
        std::lock_guard<std::mutex> lock(metricsMutex_);
        globalMetrics_.push_back(metric);
    }
    
    // Log the metric
    logMetric(*metric);
}

void PerformanceProfiler::recordInstantMetric(const std::string& name, double value, const std::string& category) {
    if (!isEnabled_) return;
    
    auto metric = std::make_shared<PerformanceMetric>(name, category);
    metric->finish();
    metric->duration = std::chrono::duration<double, std::milli>(value);
    
    // Add to current session or global metrics
    auto currentSession = getCurrentSession();
    if (currentSession && currentSession->isActive()) {
        currentSession->addMetric(metric);
    } else {
        std::lock_guard<std::mutex> lock(metricsMutex_);
        globalMetrics_.push_back(metric);
    }
}

size_t PerformanceProfiler::getCurrentMemoryUsage() const {
#ifdef __linux__
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream iss(line);
            std::string key, unit;
            size_t value;
            iss >> key >> value >> unit;
            return value * 1024; // Convert from KB to bytes
        }
    }
#endif
    return 0;
}

double PerformanceProfiler::getCurrentCPUUsage() const {
    // Simplified CPU usage calculation
    // In a real implementation, you'd want to track CPU time over an interval
#ifdef __linux__
    static auto lastTime = std::chrono::steady_clock::now();
    static clock_t lastCpuTime = clock();
    
    auto currentTime = std::chrono::steady_clock::now();
    clock_t currentCpuTime = clock();
    
    auto wallTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count();
    auto cpuTime = (currentCpuTime - lastCpuTime) * 1000000 / CLOCKS_PER_SEC;
    
    lastTime = currentTime;
    lastCpuTime = currentCpuTime;
    
    if (wallTime > 0) {
        return (double)cpuTime / wallTime * 100.0;
    }
#endif
    return 0.0;
}

void PerformanceProfiler::exportToJson(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LogManager::getInstance().error("Failed to open file for JSON export: " + filename);
        return;
    }
    
    file << "{\n";
    file << "  \"sessions\": [\n";
    
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    bool firstSession = true;
    for (const auto& [name, session] : sessions_) {
        if (!firstSession) file << ",\n";
        firstSession = false;
        
        file << "    {\n";
        file << "      \"name\": \"" << name << "\",\n";
        file << "      \"totalDuration\": " << session->getTotalDurationMs() << ",\n";
        file << "      \"metricsCount\": " << session->getMetricsCount() << ",\n";
        file << "      \"metrics\": [\n";
        
        auto metrics = session->getMetrics();
        bool firstMetric = true;
        for (const auto& metric : metrics) {
            if (!firstMetric) file << ",\n";
            firstMetric = false;
            
            file << "        {\n";
            file << "          \"name\": \"" << metric->name << "\",\n";
            file << "          \"category\": \"" << metric->category << "\",\n";
            file << "          \"duration\": " << metric->getDurationMs() << ",\n";
            file << "          \"memoryUsage\": " << metric->memoryUsage << "\n";
            file << "        }";
        }
        
        file << "\n      ]\n";
        file << "    }";
    }
    
    file << "\n  ],\n";
    file << "  \"globalMetrics\": [\n";
    
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    bool firstGlobalMetric = true;
    for (const auto& metric : globalMetrics_) {
        if (!firstGlobalMetric) file << ",\n";
        firstGlobalMetric = false;
        
        file << "    {\n";
        file << "      \"name\": \"" << metric->name << "\",\n";
        file << "      \"category\": \"" << metric->category << "\",\n";
        file << "      \"duration\": " << metric->getDurationMs() << ",\n";
        file << "      \"memoryUsage\": " << metric->memoryUsage << "\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
}

void PerformanceProfiler::exportToCsv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LogManager::getInstance().error("Failed to open file for CSV export: " + filename);
        return;
    }
    
    // CSV header
    file << "Session,Name,Category,Duration(ms),MemoryUsage(bytes)\n";
    
    // Sessions
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    for (const auto& [sessionName, session] : sessions_) {
        auto metrics = session->getMetrics();
        for (const auto& metric : metrics) {
            file << sessionName << ","
                 << metric->name << ","
                 << metric->category << ","
                 << metric->getDurationMs() << ","
                 << metric->memoryUsage << "\n";
        }
    }
    
    // Global metrics
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    for (const auto& metric : globalMetrics_) {
        file << "Global,"
             << metric->name << ","
             << metric->category << ","
             << metric->getDurationMs() << ","
             << metric->memoryUsage << "\n";
    }
    
    file.close();
}

void PerformanceProfiler::printSummary() const {
    auto& logger = LogManager::getInstance();
    
    logger.info("=== Performance Profiler Summary ===");
    
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    logger.info("Sessions: " + std::to_string(sessions_.size()));
    
    for (const auto& [name, session] : sessions_) {
        logger.info("Session '" + name + "': " + 
                   std::to_string(session->getMetricsCount()) + " metrics, " +
                   std::to_string(session->getTotalDurationMs()) + " ms total");
    }
    
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    logger.info("Global metrics: " + std::to_string(globalMetrics_.size()));
    
    if (!globalMetrics_.empty()) {
        double totalDuration = 0.0;
        for (const auto& metric : globalMetrics_) {
            totalDuration += metric->getDurationMs();
        }
        double avgDuration = totalDuration / globalMetrics_.size();
        logger.info("Average global metric duration: " + std::to_string(avgDuration) + " ms");
    }
}

void PerformanceProfiler::printDetailedReport() const {
    auto& logger = LogManager::getInstance();
    
    logger.info("=== Detailed Performance Report ===");
    
    // Sessions
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    for (const auto& [name, session] : sessions_) {
        logger.info("Session: " + name);
        logger.info("  Total Duration: " + std::to_string(session->getTotalDurationMs()) + " ms");
        logger.info("  Metrics Count: " + std::to_string(session->getMetricsCount()));
        logger.info("  Average Duration: " + std::to_string(session->getAverageDurationMs()) + " ms");
        logger.info("  Min Duration: " + std::to_string(session->getMinDurationMs()) + " ms");
        logger.info("  Max Duration: " + std::to_string(session->getMaxDurationMs()) + " ms");
        
        auto metrics = session->getMetrics();
        for (const auto& metric : metrics) {
            logger.info("    " + metric->category + "::" + metric->name + " - " + 
                       std::to_string(metric->getDurationMs()) + " ms");
        }
    }
    
    // Global metrics by category
    std::unordered_map<std::string, std::vector<std::shared_ptr<PerformanceMetric>>> categorizedMetrics;
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    for (const auto& metric : globalMetrics_) {
        categorizedMetrics[metric->category].push_back(metric);
    }
    
    logger.info("Global Metrics by Category:");
    for (const auto& [category, metrics] : categorizedMetrics) {
        double totalDuration = 0.0;
        for (const auto& metric : metrics) {
            totalDuration += metric->getDurationMs();
        }
        double avgDuration = totalDuration / metrics.size();
        
        logger.info("  " + category + ": " + std::to_string(metrics.size()) + " metrics, " +
                   "avg " + std::to_string(avgDuration) + " ms");
    }
}

void PerformanceProfiler::logMetric(const PerformanceMetric& metric, LogLevel level) const {
    auto& logger = LogManager::getInstance();
    std::string message = "PERF [" + metric.category + "] " + metric.name + 
                         " completed in " + std::to_string(metric.getDurationMs()) + " ms";
    
    if (metric.memoryUsage > 0) {
        message += " (memory: " + std::to_string(metric.memoryUsage) + " bytes)";
    }
    
    logger.log(level, LogCategory::CORE, message);
}

void PerformanceProfiler::logSessionSummary(const ProfilerSession& session, LogLevel level) const {
    auto& logger = LogManager::getInstance();
    std::string message = "Session '" + session.getName() + "' completed: " +
                         std::to_string(session.getMetricsCount()) + " metrics in " +
                         std::to_string(session.getTotalDurationMs()) + " ms";
    
    logger.log(level, LogCategory::CORE, message);
}

void PerformanceProfiler::reset() {
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    
    sessions_.clear();
    globalMetrics_.clear();
    currentSession_ = nullptr;
}

void PerformanceProfiler::clearOldMetrics(std::chrono::minutes olderThan) {
    auto cutoffTime = std::chrono::steady_clock::now() - olderThan;
    
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    globalMetrics_.erase(
        std::remove_if(globalMetrics_.begin(), globalMetrics_.end(),
            [cutoffTime](const std::shared_ptr<PerformanceMetric>& metric) {
                return metric->startTime < cutoffTime;
            }),
        globalMetrics_.end());
}

size_t PerformanceProfiler::getTotalMetricsCount() const {
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    
    size_t total = globalMetrics_.size();
    for (const auto& [name, session] : sessions_) {
        total += session->getMetricsCount();
    }
    return total;
}

double PerformanceProfiler::getAverageMetricDuration() const {
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    
    double totalDuration = 0.0;
    size_t totalCount = 0;
    
    for (const auto& metric : globalMetrics_) {
        totalDuration += metric->getDurationMs();
        totalCount++;
    }
    
    for (const auto& [name, session] : sessions_) {
        auto metrics = session->getMetrics();
        for (const auto& metric : metrics) {
            totalDuration += metric->getDurationMs();
            totalCount++;
        }
    }
    
    return totalCount > 0 ? totalDuration / totalCount : 0.0;
}

std::vector<std::string> PerformanceProfiler::getCategories() const {
    std::unordered_set<std::string> categorySet;
    
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    
    for (const auto& metric : globalMetrics_) {
        categorySet.insert(metric->category);
    }
    
    for (const auto& [name, session] : sessions_) {
        auto metrics = session->getMetrics();
        for (const auto& metric : metrics) {
            categorySet.insert(metric->category);
        }
    }
    
    return std::vector<std::string>(categorySet.begin(), categorySet.end());
}

std::vector<std::shared_ptr<PerformanceMetric>> PerformanceProfiler::getMetricsByCategory(const std::string& category) const {
    std::vector<std::shared_ptr<PerformanceMetric>> result;
    
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex_);
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    
    for (const auto& metric : globalMetrics_) {
        if (metric->category == category) {
            result.push_back(metric);
        }
    }
    
    for (const auto& [name, session] : sessions_) {
        auto metrics = session->getMetrics();
        for (const auto& metric : metrics) {
            if (metric->category == category) {
                result.push_back(metric);
            }
        }
    }
    
    return result;
}

// ScopedProfiler implementation
ScopedProfiler::ScopedProfiler(const std::string& name, const std::string& category) 
    : wasEnabled_(PerformanceProfiler::getInstance().isEnabled()) {
    if (wasEnabled_) {
        metric_ = PerformanceProfiler::getInstance().startMetric(name, category);
    }
}

ScopedProfiler::~ScopedProfiler() {
    if (wasEnabled_ && metric_) {
        PerformanceProfiler::getInstance().endMetric(metric_);
    }
}

void ScopedProfiler::addMetadata(const std::string& key, const std::string& value) {
    if (metric_) {
        metric_->metadata[key] = value;
    }
}

double ScopedProfiler::getCurrentDurationMs() const {
    if (!metric_) return 0.0;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - metric_->startTime);
    return duration.count();
}

} // namespace bolt