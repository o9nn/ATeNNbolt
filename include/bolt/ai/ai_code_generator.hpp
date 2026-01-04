#ifndef BOLT_AI_CODE_GENERATOR_HPP
#define BOLT_AI_CODE_GENERATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "bolt/ai/ai_completion_provider.hpp"

namespace bolt {

/**
 * Code generation context for AI-powered code creation
 */
struct CodeGenerationContext {
    std::string language = "cpp";
    std::string fileType;
    std::string projectContext;
    std::string existingCode;
    std::vector<std::string> requirements;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> metadata;
};

/**
 * Generated code result with metadata
 */
struct GeneratedCode {
    std::string code;
    std::string description;
    std::vector<std::string> dependencies;
    std::vector<std::string> suggestions;
    double confidence = 0.0;
    std::string generatorType;
};

/**
 * AI-powered code generator interface
 */
class AICodeGenerator {
public:
    virtual ~AICodeGenerator() = default;
    
    // Function generation
    virtual GeneratedCode generateFunction(
        const std::string& description,
        const CodeGenerationContext& context = {}
    ) = 0;
    
    // Class generation
    virtual GeneratedCode generateClass(
        const std::string& className,
        const std::string& description,
        const CodeGenerationContext& context = {}
    ) = 0;
    
    // Method generation
    virtual GeneratedCode generateMethod(
        const std::string& methodName,
        const std::string& description,
        const std::string& className,
        const CodeGenerationContext& context = {}
    ) = 0;
    
    // Template/boilerplate generation
    virtual GeneratedCode generateTemplate(
        const std::string& templateType,
        const CodeGenerationContext& context = {}
    ) = 0;
    
    // Documentation generation
    virtual std::string generateDocumentation(
        const std::string& code,
        const CodeGenerationContext& context = {}
    ) = 0;
    
    // Code explanation
    virtual std::string explainCode(
        const std::string& code,
        const CodeGenerationContext& context = {}
    ) = 0;
    
    // Utility methods
    virtual bool isReady() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

/**
 * Template-based code generator for common patterns
 */
class TemplateCodeGenerator : public AICodeGenerator {
private:
    struct CodeTemplate {
        std::string name;
        std::string pattern;
        std::vector<std::string> placeholders;
        std::string description;
    };
    
    std::unordered_map<std::string, CodeTemplate> templates_;
    bool initialized_ = false;
    
    void loadDefaultTemplates();
    std::string processTemplate(const CodeTemplate& tmpl, 
                              const std::unordered_map<std::string, std::string>& values) const;
    
public:
    TemplateCodeGenerator();
    
    // AICodeGenerator interface
    GeneratedCode generateFunction(
        const std::string& description,
        const CodeGenerationContext& context = {}
    ) override;
    
    GeneratedCode generateClass(
        const std::string& className,
        const std::string& description,
        const CodeGenerationContext& context = {}
    ) override;
    
    GeneratedCode generateMethod(
        const std::string& methodName,
        const std::string& description,
        const std::string& className,
        const CodeGenerationContext& context = {}
    ) override;
    
    GeneratedCode generateTemplate(
        const std::string& templateType,
        const CodeGenerationContext& context = {}
    ) override;
    
    std::string generateDocumentation(
        const std::string& code,
        const CodeGenerationContext& context = {}
    ) override;
    
    std::string explainCode(
        const std::string& code,
        const CodeGenerationContext& context = {}
    ) override;
    
    bool isReady() const override { return initialized_; }
    bool initialize() override;
    void shutdown() override { initialized_ = false; }
    
    // Template management
    void addTemplate(const std::string& name, const CodeTemplate& tmpl);
    std::vector<std::string> getAvailableTemplates() const;
};

/**
 * AI-enhanced code generator that uses AI completion providers
 */
class AIEnhancedCodeGenerator : public AICodeGenerator {
private:
    std::unique_ptr<AICompletionProvider> aiProvider_;
    std::unique_ptr<TemplateCodeGenerator> templateGenerator_;
    bool initialized_ = false;
    
    std::string createGenerationPrompt(
        const std::string& type,
        const std::string& description,
        const CodeGenerationContext& context
    ) const;
    
    GeneratedCode processAIResponse(
        const std::vector<CompletionItem>& completions,
        const std::string& generatorType
    ) const;
    
public:
    AIEnhancedCodeGenerator();
    explicit AIEnhancedCodeGenerator(std::unique_ptr<AICompletionProvider> provider);
    
    void setAIProvider(std::unique_ptr<AICompletionProvider> provider);
    
    // AICodeGenerator interface
    GeneratedCode generateFunction(
        const std::string& description,
        const CodeGenerationContext& context = {}
    ) override;
    
    GeneratedCode generateClass(
        const std::string& className,
        const std::string& description,
        const CodeGenerationContext& context = {}
    ) override;
    
    GeneratedCode generateMethod(
        const std::string& methodName,
        const std::string& description,
        const std::string& className,
        const CodeGenerationContext& context = {}
    ) override;
    
    GeneratedCode generateTemplate(
        const std::string& templateType,
        const CodeGenerationContext& context = {}
    ) override;
    
    std::string generateDocumentation(
        const std::string& code,
        const CodeGenerationContext& context = {}
    ) override;
    
    std::string explainCode(
        const std::string& code,
        const CodeGenerationContext& context = {}
    ) override;
    
    bool isReady() const override;
    bool initialize() override;
    void shutdown() override;
    
    // Configuration
    void setFallbackToTemplate(bool enabled) { fallbackToTemplate_ = enabled; }
    bool isFallbackToTemplateEnabled() const { return fallbackToTemplate_; }
    
private:
    bool fallbackToTemplate_ = true;
};

/**
 * Code generation service manager
 */
class CodeGenerationService {
private:
    std::unique_ptr<AICodeGenerator> generator_;
    static std::unique_ptr<CodeGenerationService> instance_;
    
    CodeGenerationService() = default;
    
public:
    static CodeGenerationService& getInstance();
    
    void setGenerator(std::unique_ptr<AICodeGenerator> generator);
    AICodeGenerator* getGenerator() const { return generator_.get(); }
    
    // Convenience methods
    GeneratedCode generateFunction(const std::string& description, 
                                 const CodeGenerationContext& context = {});
    GeneratedCode generateClass(const std::string& className, 
                              const std::string& description,
                              const CodeGenerationContext& context = {});
    std::string generateDocumentation(const std::string& code,
                                    const CodeGenerationContext& context = {});
    std::string explainCode(const std::string& code,
                          const CodeGenerationContext& context = {});
    
    bool isReady() const;
};

} // namespace bolt

#endif // BOLT_AI_CODE_GENERATOR_HPP