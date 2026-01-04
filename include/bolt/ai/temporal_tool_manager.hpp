#ifndef TEMPORAL_TOOL_MANAGER_HPP
#define TEMPORAL_TOOL_MANAGER_HPP

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace bolt {
namespace ai {

/**
 * Result of a temporal tool invocation
 */
struct TemporalToolResult {
    bool success;
    std::string description;
    std::unordered_map<std::string, double> numeric_results;
    std::vector<std::string> predictions;
    std::chrono::system_clock::time_point execution_time;
    double confidence_score;
};

/**
 * Definition of a temporal tool (time-series operation)
 */
struct TemporalTool {
    std::string name;
    std::string description;
    std::vector<std::string> parameters;
    std::string category;       // forecasting, monitoring, trading, etc.
    bool requires_historical_data;
    std::function<TemporalToolResult(const std::unordered_map<std::string, std::string>&)> handler;
};

/**
 * Trigger condition for automated tool invocation
 */
struct TemporalTrigger {
    std::string name;
    std::string condition;      // e.g., "price > 100", "anomaly_detected"
    std::string tool_to_invoke;
    std::unordered_map<std::string, std::string> tool_params;
    bool enabled = true;
    int cooldown_seconds = 60;  // Minimum time between invocations
    std::chrono::system_clock::time_point last_triggered;
};

/**
 * Manager for temporal tool-use with RWKV
 * Enables predictive agents that can act on time-series data
 */
class TemporalToolManager {
private:
    std::unordered_map<std::string, TemporalTool> tools_;
    std::vector<TemporalTrigger> triggers_;
    std::vector<TemporalToolResult> execution_history_;
    bool initialized_ = false;
    
    // Tool execution
    TemporalToolResult executeTool(const std::string& tool_name,
                                   const std::unordered_map<std::string, std::string>& params);
    
    // Trigger evaluation
    bool evaluateTriggerCondition(const TemporalTrigger& trigger,
                                  const std::unordered_map<std::string, double>& current_state);

public:
    TemporalToolManager();
    ~TemporalToolManager() = default;
    
    // Initialization
    bool initialize();
    bool isInitialized() const { return initialized_; }
    
    // Tool registration
    void registerTool(const TemporalTool& tool);
    void unregisterTool(const std::string& tool_name);
    std::vector<std::string> getAvailableTools() const;
    TemporalTool getTool(const std::string& tool_name) const;
    
    // Tool invocation
    TemporalToolResult invokeTool(const std::string& tool_name,
                                  const std::unordered_map<std::string, std::string>& params);
    
    // Trigger management
    void registerTrigger(const TemporalTrigger& trigger);
    void removeTrigger(const std::string& trigger_name);
    void enableTrigger(const std::string& trigger_name, bool enabled);
    std::vector<TemporalTrigger> getActiveTriggers() const;
    
    // Automated monitoring
    void checkTriggers(const std::unordered_map<std::string, double>& current_state);
    
    // Execution history
    std::vector<TemporalToolResult> getExecutionHistory(int last_n = 10) const;
    void clearExecutionHistory();
    
    // Tool discovery (for AI to query available actions)
    std::vector<std::string> getToolsByCategory(const std::string& category) const;
    std::string getToolsAsJSON() const;
    
    // Predefined tool sets
    void loadForecastingTools();
    void loadMonitoringTools();
    void loadTradingTools();
    void loadAnalyticsTools();
    void loadAllStandardTools();
};

} // namespace ai
} // namespace bolt

#endif // TEMPORAL_TOOL_MANAGER_HPP
