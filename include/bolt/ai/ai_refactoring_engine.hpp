#ifndef BOLT_AI_REFACTORING_ENGINE_HPP
#define BOLT_AI_REFACTORING_ENGINE_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <regex>
#include "bolt/core/code_analyzer.hpp"
#include "bolt/ai/ai_completion_provider.hpp"

namespace bolt {

/**
 * Enhanced refactoring suggestion with AI insights
 */
struct AIRefactoringSuggestion {
    std::string id;
    std::string title;
    std::string description;
    std::string filePath;
    size_t startLine;
    size_t endLine;
    std::string originalCode;
    std::string suggestedCode;
    std::vector<std::string> benefits;
    std::vector<std::string> risks;
    double confidenceScore;
    std::string refactoringType;
    std::string aiReasoning;
    std::vector<std::string> alternativeApproaches;
    
    // Priority and impact assessment
    enum class Priority { LOW, MEDIUM, HIGH, CRITICAL };
    enum class Impact { MINIMAL, MODERATE, SIGNIFICANT, MAJOR };
    
    Priority priority = Priority::MEDIUM;
    Impact impact = Impact::MODERATE;
    
    // Difficulty estimation
    enum class Difficulty { EASY, MODERATE, HARD, EXPERT };
    Difficulty difficulty = Difficulty::MODERATE;
    
    // Estimated time to implement (in minutes)
    int estimatedTimeMinutes = 30;
};

/**
 * AI-powered refactoring suggestion engine
 */
class AIRefactoringEngine {
public:
    virtual ~AIRefactoringEngine() = default;
    
    // Generate AI-enhanced refactoring suggestions
    virtual std::vector<AIRefactoringSuggestion> generateSuggestions(
        const std::string& code,
        const std::string& filePath = "",
        const std::string& language = "cpp"
    ) = 0;
    
    // Analyze specific refactoring patterns
    virtual std::vector<AIRefactoringSuggestion> findExtractMethodOpportunities(
        const std::string& code,
        const std::string& filePath = ""
    ) = 0;
    
    virtual std::vector<AIRefactoringSuggestion> findComplexityReductions(
        const std::string& code,
        const std::string& filePath = ""
    ) = 0;
    
    virtual std::vector<AIRefactoringSuggestion> findDuplicationRemoval(
        const std::string& code,
        const std::string& filePath = ""
    ) = 0;
    
    virtual std::vector<AIRefactoringSuggestion> findDesignPatternOpportunities(
        const std::string& code,
        const std::string& filePath = ""
    ) = 0;
    
    // Performance-focused refactoring
    virtual std::vector<AIRefactoringSuggestion> findPerformanceOptimizations(
        const std::string& code,
        const std::string& filePath = ""
    ) = 0;
    
    // Security-focused refactoring
    virtual std::vector<AIRefactoringSuggestion> findSecurityImprovements(
        const std::string& code,
        const std::string& filePath = ""
    ) = 0;
    
    // Apply refactoring suggestion
    virtual std::string applyRefactoring(
        const std::string& code,
        const AIRefactoringSuggestion& suggestion
    ) = 0;
    
    // Validate refactoring result
    virtual bool validateRefactoring(
        const std::string& originalCode,
        const std::string& refactoredCode,
        const AIRefactoringSuggestion& suggestion
    ) = 0;
    
    virtual bool isReady() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

/**
 * Template-based refactoring engine with pattern recognition
 */
class TemplateRefactoringEngine : public AIRefactoringEngine {
private:
    struct RefactoringPattern {
        std::string name;
        std::string description;
        std::regex pattern;
        std::function<AIRefactoringSuggestion(const std::smatch&, const std::string&)> suggestionGenerator;
    };
    
    std::vector<RefactoringPattern> patterns_;
    bool initialized_ = false;
    
    void loadDefaultPatterns();
    AIRefactoringSuggestion createExtractMethodSuggestion(
        const std::smatch& match, const std::string& filePath) const;
    AIRefactoringSuggestion createComplexityReductionSuggestion(
        const std::smatch& match, const std::string& filePath) const;
    AIRefactoringSuggestion createDuplicationRemovalSuggestion(
        const std::smatch& match, const std::string& filePath) const;
    
public:
    TemplateRefactoringEngine();
    
    std::vector<AIRefactoringSuggestion> generateSuggestions(
        const std::string& code,
        const std::string& filePath = "",
        const std::string& language = "cpp"
    ) override;
    
    std::vector<AIRefactoringSuggestion> findExtractMethodOpportunities(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findComplexityReductions(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findDuplicationRemoval(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findDesignPatternOpportunities(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findPerformanceOptimizations(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findSecurityImprovements(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::string applyRefactoring(
        const std::string& code,
        const AIRefactoringSuggestion& suggestion
    ) override;
    
    bool validateRefactoring(
        const std::string& originalCode,
        const std::string& refactoredCode,
        const AIRefactoringSuggestion& suggestion
    ) override;
    
    bool isReady() const override { return initialized_; }
    bool initialize() override;
    void shutdown() override { initialized_ = false; }
};

/**
 * AI-enhanced refactoring engine using AI completion providers
 */
class AIEnhancedRefactoringEngine : public AIRefactoringEngine {
private:
    std::unique_ptr<AICompletionProvider> aiProvider_;
    std::unique_ptr<TemplateRefactoringEngine> templateEngine_;
    bool initialized_ = false;
    bool fallbackToTemplate_ = true;
    
    std::string createRefactoringPrompt(
        const std::string& code,
        const std::string& refactoringType,
        const std::string& context = ""
    ) const;
    
    std::vector<AIRefactoringSuggestion> processAIRefactoringResponse(
        const std::vector<CompletionItem>& completions,
        const std::string& originalCode,
        const std::string& filePath,
        const std::string& refactoringType
    ) const;
    
    AIRefactoringSuggestion parseRefactoringSuggestion(
        const std::string& response,
        const std::string& originalCode,
        const std::string& filePath,
        const std::string& refactoringType
    ) const;
    
public:
    AIEnhancedRefactoringEngine();
    explicit AIEnhancedRefactoringEngine(std::unique_ptr<AICompletionProvider> provider);
    
    void setAIProvider(std::unique_ptr<AICompletionProvider> provider);
    
    std::vector<AIRefactoringSuggestion> generateSuggestions(
        const std::string& code,
        const std::string& filePath = "",
        const std::string& language = "cpp"
    ) override;
    
    std::vector<AIRefactoringSuggestion> findExtractMethodOpportunities(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findComplexityReductions(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findDuplicationRemoval(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findDesignPatternOpportunities(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findPerformanceOptimizations(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::vector<AIRefactoringSuggestion> findSecurityImprovements(
        const std::string& code,
        const std::string& filePath = ""
    ) override;
    
    std::string applyRefactoring(
        const std::string& code,
        const AIRefactoringSuggestion& suggestion
    ) override;
    
    bool validateRefactoring(
        const std::string& originalCode,
        const std::string& refactoredCode,
        const AIRefactoringSuggestion& suggestion
    ) override;
    
    bool isReady() const override;
    bool initialize() override;
    void shutdown() override;
    
    // Configuration
    void setFallbackToTemplate(bool enabled) { fallbackToTemplate_ = enabled; }
    bool isFallbackToTemplateEnabled() const { return fallbackToTemplate_; }
};

/**
 * Refactoring service manager that integrates with the existing code analyzer
 */
class RefactoringService {
private:
    std::unique_ptr<AIRefactoringEngine> engine_;
    static std::unique_ptr<RefactoringService> instance_;
    
    RefactoringService() = default;
    
public:
    static RefactoringService& getInstance();
    
    void setEngine(std::unique_ptr<AIRefactoringEngine> engine);
    AIRefactoringEngine* getEngine() const { return engine_.get(); }
    
    // Convenience methods
    std::vector<AIRefactoringSuggestion> analyzeCode(const std::string& code, 
                                                   const std::string& filePath = "");
    std::vector<AIRefactoringSuggestion> findAllOpportunities(const std::string& code,
                                                            const std::string& filePath = "");
    
    std::string applyRefactoring(const std::string& code, 
                               const AIRefactoringSuggestion& suggestion);
    bool validateRefactoring(const std::string& originalCode,
                           const std::string& refactoredCode,
                           const AIRefactoringSuggestion& suggestion);
    
    bool isReady() const;
    
    // Statistics and reporting
    struct RefactoringStats {
        size_t totalSuggestionsGenerated = 0;
        size_t suggestionsApplied = 0;
        size_t successfulRefactorings = 0;
        double averageConfidenceScore = 0.0;
    };
    
    RefactoringStats getStatistics() const;
    void resetStatistics();
    
private:
    mutable RefactoringStats stats_;
};

} // namespace bolt

#endif // BOLT_AI_REFACTORING_ENGINE_HPP