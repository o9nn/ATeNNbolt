#ifndef TEMPORAL_RAG_MANAGER_HPP
#define TEMPORAL_RAG_MANAGER_HPP

#include "bolt/ai/rwkv_wrapper.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace bolt {
namespace ai {

/**
 * Represents a time-series data point
 */
struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, double> values; // metric_name -> value
    std::string source;                             // Data source identifier
    std::string category;                           // Data category (e.g., "stock", "sensor")
};

/**
 * A temporal query for retrieving time-series data
 */
struct TemporalQuery {
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::vector<std::string> metrics;               // Which metrics to retrieve
    std::string source_filter;                      // Optional source filter
    std::string category_filter;                    // Optional category filter
    int max_points = 1000;                          // Max data points to retrieve
};

/**
 * Configuration for temporal RAG
 */
struct TemporalRAGConfig {
    int context_window_size = 2048;                 // RWKV context size
    bool normalize_values = true;                   // Normalize time-series values
    bool include_timestamps = true;                 // Include timestamps in sequence
    std::string time_format = "relative";           // "relative" or "absolute"
    int aggregation_window_seconds = 60;            // Aggregate data into windows
};

/**
 * Manager for temporal RAG using RWKV architecture
 * Specialized for time-series analysis and prediction
 */
class TemporalRAGManager {
private:
    std::unique_ptr<RWKVWrapper> rwkv_model_;
    std::vector<TimeSeriesPoint> time_series_db_;
    TemporalRAGConfig config_;
    bool initialized_ = false;
    
    // Data processing
    std::vector<TimeSeriesPoint> queryTimeSeriesDB(const TemporalQuery& query) const;
    std::string formatTimeSeriesForRWKV(const std::vector<TimeSeriesPoint>& data) const;
    std::vector<TimeSeriesPoint> aggregateData(const std::vector<TimeSeriesPoint>& data, 
                                                int window_seconds) const;
    
    // Analysis helpers
    double calculateTrend(const std::vector<double>& values) const;
    std::vector<size_t> detectAnomalies(const std::vector<double>& values, 
                                         double threshold = 2.0) const;

public:
    TemporalRAGManager();
    explicit TemporalRAGManager(const TemporalRAGConfig& config);
    ~TemporalRAGManager() = default;
    
    // Initialization
    bool initialize(const std::string& model_path = "");
    bool isInitialized() const { return initialized_; }
    void shutdown();
    
    // Configuration
    void setConfig(const TemporalRAGConfig& config) { config_ = config; }
    TemporalRAGConfig getConfig() const { return config_; }
    
    // Data ingestion
    void ingestDataPoint(const TimeSeriesPoint& point);
    void ingestDataBatch(const std::vector<TimeSeriesPoint>& points);
    size_t getDataPointCount() const { return time_series_db_.size(); }
    
    // Temporal queries and analysis
    std::string analyzeTimePeriod(const TemporalQuery& query, 
                                  const std::string& analysis_prompt);
    
    std::string detectPatterns(const std::string& metric, 
                              const std::chrono::system_clock::time_point& start,
                              const std::chrono::system_clock::time_point& end);
    
    std::string comparePeriods(const std::string& metric,
                              const TemporalQuery& period1,
                              const TemporalQuery& period2);
    
    // Forecasting
    std::vector<double> forecastNextN(const std::string& metric, 
                                      int n_steps,
                                      const std::chrono::system_clock::time_point& from_time);
    
    // Anomaly detection
    std::vector<TimeSeriesPoint> findAnomalies(const std::string& metric,
                                               const TemporalQuery& query,
                                               double threshold = 2.0);
    
    // Correlation analysis
    double calculateCorrelation(const std::string& metric1,
                               const std::string& metric2,
                               const TemporalQuery& query);
    
    // Statistics
    std::vector<std::string> getAvailableMetrics() const;
    std::vector<std::string> getAvailableSources() const;
    std::pair<std::chrono::system_clock::time_point, 
              std::chrono::system_clock::time_point> getDataTimeRange() const;
};

} // namespace ai
} // namespace bolt

#endif // TEMPORAL_RAG_MANAGER_HPP
