#include "bolt/ai/temporal_rag_manager.hpp"
#include "bolt/ai/temporal_tool_manager.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>

void printSeparator() {
    std::cout << "\n" << std::string(80, '=') << "\n\n";
}

// Simulate stock price data
std::vector<bolt::ai::TimeSeriesPoint> generateStockData(int days) {
    std::vector<bolt::ai::TimeSeriesPoint> data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> price_noise(0, 2.0);
    
    double base_price = 100.0;
    auto now = std::chrono::system_clock::now();
    
    for (int i = 0; i < days * 24; ++i) {  // Hourly data
        bolt::ai::TimeSeriesPoint point;
        point.timestamp = now - std::chrono::hours(days * 24 - i);
        
        // Simulate price movement with trend and noise
        double trend = std::sin(i * 0.1) * 10.0;  // Cyclical trend
        double noise = price_noise(gen);
        double price = base_price + trend + noise;
        
        // Add anomaly at specific points
        if (i == days * 12) {  // Midpoint anomaly
            price += 20.0;
        }
        
        point.values["price"] = price;
        point.values["volume"] = 1000000 + std::abs(noise) * 100000;
        point.source = "stock_exchange";
        point.category = "stock";
        
        data.push_back(point);
    }
    
    return data;
}

int main() {
    std::cout << "=== Temporal RAG & TOOL-USE Demo with RWKV.cpp ===\n";
    std::cout << "Time-Series Analysis and Predictive Agents\n";
    printSeparator();
    
    // 1. Initialize the Temporal RAG Manager
    std::cout << "Initializing Temporal RAG Manager...\n";
    bolt::ai::TemporalRAGManager rag_manager;
    
    if (!rag_manager.initialize()) {
        std::cerr << "❌ Failed to initialize Temporal RAG Manager\n";
        std::cerr << "Note: This demo shows the architecture even without a model\n";
    } else {
        std::cout << "✅ Temporal RAG Manager initialized\n";
    }
    std::cout << "\n";
    
    // 2. Initialize the Temporal Tool Manager
    std::cout << "Initializing Temporal Tool Manager...\n";
    bolt::ai::TemporalToolManager tool_manager;
    tool_manager.initialize();
    tool_manager.loadAllStandardTools();
    std::cout << "✅ Loaded " << tool_manager.getAvailableTools().size() << " temporal tools\n";
    printSeparator();
    
    // 3. Generate and ingest sample time-series data
    std::cout << "Generating sample stock market data...\n";
    auto stock_data = generateStockData(30);  // 30 days of hourly data
    
    std::cout << "Ingesting " << stock_data.size() << " data points...\n";
    rag_manager.ingestDataBatch(stock_data);
    std::cout << "✅ Data ingested successfully\n";
    
    auto time_range = rag_manager.getDataTimeRange();
    std::cout << "Data time range: " << std::chrono::duration_cast<std::chrono::hours>(
        time_range.second - time_range.first).count() << " hours\n";
    printSeparator();
    
    // 4. Demonstrate Temporal RAG: Query time-series data
    std::cout << "=== Temporal RAG: Analyzing Time Periods ===\n\n";
    
    bolt::ai::TemporalQuery query1;
    query1.start_time = std::chrono::system_clock::now() - std::chrono::hours(7 * 24);
    query1.end_time = std::chrono::system_clock::now();
    query1.metrics = {"price", "volume"};
    query1.source_filter = "stock_exchange";
    
    std::cout << "Query: Analyze stock price trends over the last 7 days\n\n";
    
    std::string analysis = rag_manager.analyzeTimePeriod(
        query1,
        "Analyze the stock price trends and identify any significant patterns or anomalies."
    );
    
    std::cout << "RWKV Analysis:\n";
    std::cout << "─────────────────────────────────\n";
    std::cout << analysis << "\n";
    std::cout << "─────────────────────────────────\n";
    printSeparator();
    
    // 5. Demonstrate pattern detection
    std::cout << "=== Pattern Detection ===\n\n";
    
    std::string patterns = rag_manager.detectPatterns(
        "price",
        std::chrono::system_clock::now() - std::chrono::hours(30 * 24),
        std::chrono::system_clock::now()
    );
    
    std::cout << "Detected Patterns:\n";
    std::cout << patterns << "\n";
    printSeparator();
    
    // 6. Demonstrate anomaly detection
    std::cout << "=== Anomaly Detection ===\n\n";
    
    bolt::ai::TemporalQuery anomaly_query;
    anomaly_query.start_time = std::chrono::system_clock::now() - std::chrono::hours(30 * 24);
    anomaly_query.end_time = std::chrono::system_clock::now();
    anomaly_query.metrics = {"price"};
    
    auto anomalies = rag_manager.findAnomalies("price", anomaly_query, 2.0);
    
    std::cout << "Found " << anomalies.size() << " anomalies:\n\n";
    for (size_t i = 0; i < std::min(size_t(5), anomalies.size()); ++i) {
        auto time_t = std::chrono::system_clock::to_time_t(anomalies[i].timestamp);
        std::cout << (i + 1) << ". Time: " << std::ctime(&time_t);
        std::cout << "   Price: $" << std::fixed << std::setprecision(2) 
                  << anomalies[i].values.at("price") << "\n";
    }
    printSeparator();
    
    // 7. Demonstrate Temporal TOOL-USE: Forecasting
    std::cout << "=== Temporal TOOL-USE: Predictive Agent ===\n\n";
    
    std::cout << "Available tools:\n";
    auto available_tools = tool_manager.getAvailableTools();
    for (const auto& tool : available_tools) {
        std::cout << "  • " << tool << "\n";
    }
    std::cout << "\n";
    
    // AI decides to use the forecast tool
    std::cout << "AI Decision: Use 'forecast_price' tool\n\n";
    
    std::unordered_map<std::string, std::string> forecast_params;
    forecast_params["metric"] = "price";
    forecast_params["n_steps"] = "24";  // Forecast next 24 hours
    forecast_params["confidence_level"] = "0.95";
    
    auto forecast_result = tool_manager.invokeTool("forecast_price", forecast_params);
    
    std::cout << "Forecast Result:\n";
    std::cout << "  Success: " << (forecast_result.success ? "Yes" : "No") << "\n";
    std::cout << "  Description: " << forecast_result.description << "\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(2) 
              << (forecast_result.confidence_score * 100) << "%\n";
    
    if (!forecast_result.predictions.empty()) {
        std::cout << "  Predictions (first 5):\n";
        for (size_t i = 0; i < std::min(size_t(5), forecast_result.predictions.size()); ++i) {
            std::cout << "    " << (i + 1) << ". " << forecast_result.predictions[i] << "\n";
        }
    }
    std::cout << "\n";
    printSeparator();
    
    // 8. Demonstrate automated triggers
    std::cout << "=== Automated Triggers ===\n\n";
    
    bolt::ai::TemporalTrigger price_alert;
    price_alert.name = "high_price_alert";
    price_alert.condition = "price > 110";
    price_alert.tool_to_invoke = "send_alert";
    price_alert.tool_params = {{"message", "Price exceeded threshold!"}};
    price_alert.enabled = true;
    price_alert.cooldown_seconds = 300;  // 5 minutes
    
    tool_manager.registerTrigger(price_alert);
    std::cout << "Registered trigger: " << price_alert.name << "\n";
    std::cout << "Condition: " << price_alert.condition << "\n";
    std::cout << "Action: Invoke '" << price_alert.tool_to_invoke << "'\n\n";
    
    // Simulate checking triggers with current state
    std::unordered_map<std::string, double> current_state;
    current_state["price"] = 115.0;  // Exceeds threshold
    current_state["volume"] = 1500000;
    
    std::cout << "Current State:\n";
    std::cout << "  Price: $" << current_state["price"] << "\n";
    std::cout << "  Volume: " << current_state["volume"] << "\n\n";
    
    std::cout << "Checking triggers...\n";
    tool_manager.checkTriggers(current_state);
    std::cout << "✅ Triggers evaluated\n";
    printSeparator();
    
    // 9. Show execution history
    std::cout << "=== Tool Execution History ===\n\n";
    auto history = tool_manager.getExecutionHistory(5);
    std::cout << "Recent executions: " << history.size() << "\n\n";
    
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << (i + 1) << ". " << history[i].description << "\n";
        std::cout << "   Confidence: " << (history[i].confidence_score * 100) << "%\n";
    }
    
    printSeparator();
    std::cout << "=== Demo Complete ===\n";
    std::cout << "\nThis demonstrates how RWKV.cpp can be enhanced with:\n";
    std::cout << "  • Efficient time-series analysis (Temporal RAG)\n";
    std::cout << "  • Predictive agents with tool use (Temporal TOOL-USE)\n";
    std::cout << "  • Automated monitoring and response triggers\n";
    std::cout << "  • Real-time forecasting and anomaly detection\n\n";
    
    return 0;
}
