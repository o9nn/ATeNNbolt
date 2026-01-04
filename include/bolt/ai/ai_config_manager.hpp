#ifndef BOLT_AI_CONFIG_MANAGER_HPP
#define BOLT_AI_CONFIG_MANAGER_HPP

#include <string>
#include <map>

namespace bolt {
namespace ai {

/**
 * @brief AI Configuration Manager
 * Manages configuration settings for AI providers and models.
 */
class AIConfigManager {
public:
    AIConfigManager()
        : apiKey_("")
        , maxTokens_(4096)
        , temperature_(0.7)
        , provider_("local")
        , modelName_("default")
    {}

    // API Key management
    void setApiKey(const std::string& key) { apiKey_ = key; }
    std::string getApiKey() const { return apiKey_; }

    // Token settings
    void setMaxTokens(int tokens) { maxTokens_ = tokens; }
    int getMaxTokens() const { return maxTokens_; }

    // Temperature settings
    void setTemperature(double temp) { temperature_ = temp; }
    double getTemperature() const { return temperature_; }

    // Provider settings
    void setProvider(const std::string& provider) { provider_ = provider; }
    std::string getProvider() const { return provider_; }

    // Model settings
    void setModelName(const std::string& name) { modelName_ = name; }
    std::string getModelName() const { return modelName_; }

    // Custom options
    void setOption(const std::string& key, const std::string& value) {
        options_[key] = value;
    }

    std::string getOption(const std::string& key, const std::string& defaultValue = "") const {
        auto it = options_.find(key);
        return (it != options_.end()) ? it->second : defaultValue;
    }

    // Validation
    bool isValid() const {
        return maxTokens_ > 0 && temperature_ >= 0.0 && temperature_ <= 2.0;
    }

private:
    std::string apiKey_;
    int maxTokens_;
    double temperature_;
    std::string provider_;
    std::string modelName_;
    std::map<std::string, std::string> options_;
};

} // namespace ai
} // namespace bolt

#endif // BOLT_AI_CONFIG_MANAGER_HPP
