#include "bolt/ai/ai_code_generator.hpp"
#include <algorithm>
#include <sstream>
#include <regex>

namespace bolt {

// TemplateCodeGenerator implementation
TemplateCodeGenerator::TemplateCodeGenerator() {
    loadDefaultTemplates();
}

void TemplateCodeGenerator::loadDefaultTemplates() {
    // C++ Function Template
    CodeTemplate funcTemplate;
    funcTemplate.name = "cpp_function";
    funcTemplate.description = "Basic C++ function template";
    funcTemplate.pattern = R"(/**
 * {DESCRIPTION}
 */
{RETURN_TYPE} {FUNCTION_NAME}({PARAMETERS}) {
    {BODY}
    return {DEFAULT_RETURN};
})";
    funcTemplate.placeholders = {"DESCRIPTION", "RETURN_TYPE", "FUNCTION_NAME", "PARAMETERS", "BODY", "DEFAULT_RETURN"};
    templates_["cpp_function"] = funcTemplate;

    // C++ Class Template
    CodeTemplate classTemplate;
    classTemplate.name = "cpp_class";
    classTemplate.description = "Basic C++ class template";
    classTemplate.pattern = R"(/**
 * {DESCRIPTION}
 */
class {CLASS_NAME} {
private:
    {PRIVATE_MEMBERS}

public:
    // Constructor
    {CLASS_NAME}();
    
    // Destructor
    ~{CLASS_NAME}();
    
    {PUBLIC_METHODS}
};)";
    classTemplate.placeholders = {"DESCRIPTION", "CLASS_NAME", "PRIVATE_MEMBERS", "PUBLIC_METHODS"};
    templates_["cpp_class"] = classTemplate;

    // C++ Method Template
    CodeTemplate methodTemplate;
    methodTemplate.name = "cpp_method";
    methodTemplate.description = "C++ class method template";
    methodTemplate.pattern = R"(/**
 * {DESCRIPTION}
 */
{RETURN_TYPE} {CLASS_NAME}::{METHOD_NAME}({PARAMETERS}) {
    {BODY}
    return {DEFAULT_RETURN};
})";
    methodTemplate.placeholders = {"DESCRIPTION", "RETURN_TYPE", "CLASS_NAME", "METHOD_NAME", "PARAMETERS", "BODY", "DEFAULT_RETURN"};
    templates_["cpp_method"] = methodTemplate;

    // Header Guard Template
    CodeTemplate headerTemplate;
    headerTemplate.name = "header_guard";
    headerTemplate.description = "C++ header file with include guards";
    headerTemplate.pattern = R"(#ifndef {GUARD_NAME}
#define {GUARD_NAME}

{INCLUDES}

namespace {NAMESPACE} {

{CONTENT}

} // namespace {NAMESPACE}

#endif // {GUARD_NAME})";
    headerTemplate.placeholders = {"GUARD_NAME", "INCLUDES", "NAMESPACE", "CONTENT"};
    templates_["header_guard"] = headerTemplate;
}

bool TemplateCodeGenerator::initialize() {
    if (!initialized_) {
        loadDefaultTemplates();
        initialized_ = true;
    }
    return initialized_;
}

std::string TemplateCodeGenerator::processTemplate(
    const CodeTemplate& tmpl, 
    const std::unordered_map<std::string, std::string>& values) const {
    
    std::string result = tmpl.pattern;
    
    for (const auto& placeholder : tmpl.placeholders) {
        std::string token = "{" + placeholder + "}";
        auto it = values.find(placeholder);
        std::string replacement = (it != values.end()) ? it->second : "/* TODO: " + placeholder + " */";
        
        size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos) {
            result.replace(pos, token.length(), replacement);
            pos += replacement.length();
        }
    }
    
    return result;
}

GeneratedCode TemplateCodeGenerator::generateFunction(
    const std::string& description,
    const CodeGenerationContext& context) {
    
    GeneratedCode result;
    result.generatorType = "template";
    result.confidence = 0.8;
    result.description = "Generated function from template";
    
    std::unordered_map<std::string, std::string> values;
    values["DESCRIPTION"] = description.empty() ? "Generated function" : description;
    values["RETURN_TYPE"] = context.language == "cpp" ? "void" : "void";
    values["FUNCTION_NAME"] = "generatedFunction";
    values["PARAMETERS"] = "";
    values["BODY"] = "    // TODO: Implement function logic";
    values["DEFAULT_RETURN"] = context.language == "cpp" ? "" : "";
    
    // Try to extract function name from description
    std::regex namePattern(R"(\b(\w+)\s*function|\bfunction\s+(\w+)|\b(\w+)\(\))");
    std::smatch match;
    if (std::regex_search(description, match, namePattern)) {
        for (size_t i = 1; i < match.size(); ++i) {
            if (!match[i].str().empty()) {
                values["FUNCTION_NAME"] = match[i].str();
                break;
            }
        }
    }
    
    result.code = processTemplate(templates_.at("cpp_function"), values);
    result.suggestions.push_back("Consider adding parameter validation");
    result.suggestions.push_back("Add proper error handling");
    
    return result;
}

GeneratedCode TemplateCodeGenerator::generateClass(
    const std::string& className,
    const std::string& description,
    const CodeGenerationContext& context) {
    
    GeneratedCode result;
    result.generatorType = "template";
    result.confidence = 0.8;
    result.description = "Generated class from template";
    
    std::unordered_map<std::string, std::string> values;
    values["DESCRIPTION"] = description.empty() ? "Generated class" : description;
    values["CLASS_NAME"] = className.empty() ? "GeneratedClass" : className;
    values["PRIVATE_MEMBERS"] = "    // TODO: Add private member variables";
    values["PUBLIC_METHODS"] = "    // TODO: Add public methods";
    
    result.code = processTemplate(templates_.at("cpp_class"), values);
    result.suggestions.push_back("Consider using RAII principles");
    result.suggestions.push_back("Add copy/move constructors if needed");
    result.suggestions.push_back("Consider making destructor virtual if inheritance is expected");
    
    return result;
}

GeneratedCode TemplateCodeGenerator::generateMethod(
    const std::string& methodName,
    const std::string& description,
    const std::string& className,
    const CodeGenerationContext& context) {
    
    GeneratedCode result;
    result.generatorType = "template";
    result.confidence = 0.8;
    result.description = "Generated method from template";
    
    std::unordered_map<std::string, std::string> values;
    values["DESCRIPTION"] = description.empty() ? "Generated method" : description;
    values["RETURN_TYPE"] = "void";
    values["CLASS_NAME"] = className.empty() ? "GeneratedClass" : className;
    values["METHOD_NAME"] = methodName.empty() ? "generatedMethod" : methodName;
    values["PARAMETERS"] = "";
    values["BODY"] = "    // TODO: Implement method logic";
    values["DEFAULT_RETURN"] = "";
    
    result.code = processTemplate(templates_.at("cpp_method"), values);
    result.suggestions.push_back("Consider const-correctness");
    result.suggestions.push_back("Add parameter validation");
    
    return result;
}

GeneratedCode TemplateCodeGenerator::generateTemplate(
    const std::string& templateType,
    const CodeGenerationContext& context) {
    
    GeneratedCode result;
    result.generatorType = "template";
    result.confidence = 0.9;
    
    if (templates_.find(templateType) != templates_.end()) {
        std::unordered_map<std::string, std::string> values;
        
        if (templateType == "header_guard") {
            std::string guardName = context.fileType;
            std::transform(guardName.begin(), guardName.end(), guardName.begin(), ::toupper);
            std::replace(guardName.begin(), guardName.end(), '.', '_');
            
            values["GUARD_NAME"] = guardName + "_HPP";
            values["INCLUDES"] = "#include <string>\n#include <vector>";
            values["NAMESPACE"] = "bolt";
            values["CONTENT"] = "// TODO: Add your declarations here";
        }
        
        result.code = processTemplate(templates_.at(templateType), values);
        result.description = "Generated " + templateType + " template";
    } else {
        result.code = "// Template '" + templateType + "' not found";
        result.description = "Template not available";
        result.confidence = 0.1;
    }
    
    return result;
}

std::string TemplateCodeGenerator::generateDocumentation(
    const std::string& code,
    const CodeGenerationContext& context) {
    
    std::ostringstream doc;
    doc << "/**\n";
    doc << " * Auto-generated documentation\n";
    doc << " * \n";
    
    // Simple analysis of the code to generate basic documentation
    if (code.find("class ") != std::string::npos) {
        doc << " * This class provides functionality for code management.\n";
        doc << " * \n";
        doc << " * @note This is auto-generated documentation\n";
    } else if (code.find("(") != std::string::npos && code.find(")") != std::string::npos) {
        doc << " * This function performs a specific operation.\n";
        doc << " * \n";
        doc << " * @return Description of return value\n";
        doc << " * @note This is auto-generated documentation\n";
    } else {
        doc << " * Code documentation\n";
    }
    
    doc << " */";
    return doc.str();
}

std::string TemplateCodeGenerator::explainCode(
    const std::string& code,
    const CodeGenerationContext& context) {
    
    std::ostringstream explanation;
    explanation << "Code Explanation:\n\n";
    
    // Basic pattern recognition
    if (code.find("class ") != std::string::npos) {
        explanation << "This code defines a C++ class. Classes are fundamental building blocks ";
        explanation << "in object-oriented programming that encapsulate data and behavior.\n\n";
    }
    
    if (code.find("public:") != std::string::npos) {
        explanation << "The 'public:' section contains members that can be accessed from outside the class.\n\n";
    }
    
    if (code.find("private:") != std::string::npos) {
        explanation << "The 'private:' section contains members that can only be accessed from within the class.\n\n";
    }
    
    if (code.find("for") != std::string::npos) {
        explanation << "This code contains a for loop, which is used for iterative operations.\n\n";
    }
    
    if (code.find("if") != std::string::npos) {
        explanation << "This code contains conditional logic using if statements.\n\n";
    }
    
    explanation << "Note: This is a basic template-based explanation. ";
    explanation << "For more detailed analysis, consider using an AI-powered explanation service.";
    
    return explanation.str();
}

void TemplateCodeGenerator::addTemplate(const std::string& name, const CodeTemplate& tmpl) {
    templates_[name] = tmpl;
}

std::vector<std::string> TemplateCodeGenerator::getAvailableTemplates() const {
    std::vector<std::string> names;
    for (const auto& [name, tmpl] : templates_) {
        names.push_back(name);
    }
    return names;
}

// AIEnhancedCodeGenerator implementation
AIEnhancedCodeGenerator::AIEnhancedCodeGenerator() 
    : templateGenerator_(std::make_unique<TemplateCodeGenerator>()) {
}

AIEnhancedCodeGenerator::AIEnhancedCodeGenerator(std::unique_ptr<AICompletionProvider> provider)
    : aiProvider_(std::move(provider))
    , templateGenerator_(std::make_unique<TemplateCodeGenerator>()) {
}

void AIEnhancedCodeGenerator::setAIProvider(std::unique_ptr<AICompletionProvider> provider) {
    aiProvider_ = std::move(provider);
}

bool AIEnhancedCodeGenerator::initialize() {
    bool result = true;
    
    if (templateGenerator_) {
        result &= templateGenerator_->initialize();
    }
    
    if (aiProvider_) {
        result &= aiProvider_->initialize();
    }
    
    initialized_ = result;
    return result;
}

bool AIEnhancedCodeGenerator::isReady() const {
    return initialized_ && 
           ((aiProvider_ && aiProvider_->isReady()) || 
            (fallbackToTemplate_ && templateGenerator_ && templateGenerator_->isReady()));
}

void AIEnhancedCodeGenerator::shutdown() {
    if (aiProvider_) {
        aiProvider_->shutdown();
    }
    if (templateGenerator_) {
        templateGenerator_->shutdown();
    }
    initialized_ = false;
}

std::string AIEnhancedCodeGenerator::createGenerationPrompt(
    const std::string& type,
    const std::string& description,
    const CodeGenerationContext& context) const {
    
    std::ostringstream prompt;
    prompt << "Generate " << type << " in " << context.language << ":\n";
    prompt << "Description: " << description << "\n";
    
    if (!context.projectContext.empty()) {
        prompt << "Project context: " << context.projectContext << "\n";
    }
    
    if (!context.existingCode.empty()) {
        prompt << "Existing code context:\n" << context.existingCode << "\n";
    }
    
    if (!context.requirements.empty()) {
        prompt << "Requirements:\n";
        for (const auto& req : context.requirements) {
            prompt << "- " << req << "\n";
        }
    }
    
    prompt << "\nPlease provide clean, well-documented code following best practices.";
    
    return prompt.str();
}

GeneratedCode AIEnhancedCodeGenerator::processAIResponse(
    const std::vector<CompletionItem>& completions,
    const std::string& generatorType) const {
    
    GeneratedCode result;
    result.generatorType = "ai_enhanced";
    
    if (!completions.empty()) {
        result.code = completions[0].label;
        result.confidence = completions[0].score;
        result.description = "AI-generated code";
        
        // Extract suggestions from other completions
        for (size_t i = 1; i < std::min(completions.size(), static_cast<size_t>(3)); ++i) {
            result.suggestions.push_back("Alternative: " + completions[i].label.substr(0, 50) + "...");
        }
    } else {
        result.code = "// No AI suggestions available";
        result.confidence = 0.0;
        result.description = "AI generation failed";
    }
    
    return result;
}

GeneratedCode AIEnhancedCodeGenerator::generateFunction(
    const std::string& description,
    const CodeGenerationContext& context) {
    
    if (aiProvider_ && aiProvider_->isReady()) {
        std::string prompt = createGenerationPrompt("function", description, context);
        
        CodeContext aiContext;
        aiContext.content = prompt;
        aiContext.language = context.language;
        aiContext.cursorPosition = prompt.length();
        
        auto completions = aiProvider_->generateCompletions(aiContext, prompt, 3);
        if (!completions.empty()) {
            return processAIResponse(completions, "function");
        }
    }
    
    // Fallback to template generation
    if (fallbackToTemplate_ && templateGenerator_) {
        auto result = templateGenerator_->generateFunction(description, context);
        result.description += " (fallback to template)";
        return result;
    }
    
    GeneratedCode result;
    result.code = "// Code generation not available";
    result.confidence = 0.0;
    result.generatorType = "none";
    return result;
}

GeneratedCode AIEnhancedCodeGenerator::generateClass(
    const std::string& className,
    const std::string& description,
    const CodeGenerationContext& context) {
    
    if (aiProvider_ && aiProvider_->isReady()) {
        std::string prompt = createGenerationPrompt("class named " + className, description, context);
        
        CodeContext aiContext;
        aiContext.content = prompt;
        aiContext.language = context.language;
        aiContext.cursorPosition = prompt.length();
        
        auto completions = aiProvider_->generateCompletions(aiContext, prompt, 3);
        if (!completions.empty()) {
            return processAIResponse(completions, "class");
        }
    }
    
    // Fallback to template generation
    if (fallbackToTemplate_ && templateGenerator_) {
        auto result = templateGenerator_->generateClass(className, description, context);
        result.description += " (fallback to template)";
        return result;
    }
    
    GeneratedCode result;
    result.code = "// Code generation not available";
    result.confidence = 0.0;
    result.generatorType = "none";
    return result;
}

GeneratedCode AIEnhancedCodeGenerator::generateMethod(
    const std::string& methodName,
    const std::string& description,
    const std::string& className,
    const CodeGenerationContext& context) {
    
    if (aiProvider_ && aiProvider_->isReady()) {
        std::string prompt = createGenerationPrompt("method " + methodName + " for class " + className, description, context);
        
        CodeContext aiContext;
        aiContext.content = prompt;
        aiContext.language = context.language;
        aiContext.cursorPosition = prompt.length();
        
        auto completions = aiProvider_->generateCompletions(aiContext, prompt, 3);
        if (!completions.empty()) {
            return processAIResponse(completions, "method");
        }
    }
    
    // Fallback to template generation
    if (fallbackToTemplate_ && templateGenerator_) {
        auto result = templateGenerator_->generateMethod(methodName, description, className, context);
        result.description += " (fallback to template)";
        return result;
    }
    
    GeneratedCode result;
    result.code = "// Code generation not available";
    result.confidence = 0.0;
    result.generatorType = "none";
    return result;
}

GeneratedCode AIEnhancedCodeGenerator::generateTemplate(
    const std::string& templateType,
    const CodeGenerationContext& context) {
    
    // Template generation always falls back to template generator
    if (templateGenerator_) {
        return templateGenerator_->generateTemplate(templateType, context);
    }
    
    GeneratedCode result;
    result.code = "// Template generation not available";
    result.confidence = 0.0;
    result.generatorType = "none";
    return result;
}

std::string AIEnhancedCodeGenerator::generateDocumentation(
    const std::string& code,
    const CodeGenerationContext& context) {
    
    if (aiProvider_ && aiProvider_->isReady()) {
        std::string prompt = "Generate comprehensive documentation for this code:\n\n" + code;
        
        CodeContext aiContext;
        aiContext.content = prompt;
        aiContext.language = context.language;
        aiContext.cursorPosition = prompt.length();
        
        auto completions = aiProvider_->generateCompletions(aiContext, prompt, 1);
        if (!completions.empty()) {
            return completions[0].label;
        }
    }
    
    // Fallback to template generation
    if (fallbackToTemplate_ && templateGenerator_) {
        return templateGenerator_->generateDocumentation(code, context);
    }
    
    return "// Documentation generation not available";
}

std::string AIEnhancedCodeGenerator::explainCode(
    const std::string& code,
    const CodeGenerationContext& context) {
    
    if (aiProvider_ && aiProvider_->isReady()) {
        std::string prompt = "Explain this code in detail:\n\n" + code;
        
        CodeContext aiContext;
        aiContext.content = prompt;
        aiContext.language = context.language;
        aiContext.cursorPosition = prompt.length();
        
        auto completions = aiProvider_->generateCompletions(aiContext, prompt, 1);
        if (!completions.empty()) {
            return completions[0].label;
        }
    }
    
    // Fallback to template generation
    if (fallbackToTemplate_ && templateGenerator_) {
        return templateGenerator_->explainCode(code, context);
    }
    
    return "Code explanation not available";
}

// CodeGenerationService implementation
std::unique_ptr<CodeGenerationService> CodeGenerationService::instance_;

CodeGenerationService& CodeGenerationService::getInstance() {
    if (!instance_) {
        instance_ = std::unique_ptr<CodeGenerationService>(new CodeGenerationService());
        // Initialize with AI-enhanced generator by default
        instance_->setGenerator(std::make_unique<AIEnhancedCodeGenerator>());
    }
    return *instance_;
}

void CodeGenerationService::setGenerator(std::unique_ptr<AICodeGenerator> generator) {
    generator_ = std::move(generator);
    if (generator_) {
        generator_->initialize();
    }
}

GeneratedCode CodeGenerationService::generateFunction(
    const std::string& description, 
    const CodeGenerationContext& context) {
    
    if (generator_) {
        return generator_->generateFunction(description, context);
    }
    
    GeneratedCode result;
    result.code = "// Code generation service not available";
    result.confidence = 0.0;
    return result;
}

GeneratedCode CodeGenerationService::generateClass(
    const std::string& className, 
    const std::string& description,
    const CodeGenerationContext& context) {
    
    if (generator_) {
        return generator_->generateClass(className, description, context);
    }
    
    GeneratedCode result;
    result.code = "// Code generation service not available";
    result.confidence = 0.0;
    return result;
}

std::string CodeGenerationService::generateDocumentation(
    const std::string& code,
    const CodeGenerationContext& context) {
    
    if (generator_) {
        return generator_->generateDocumentation(code, context);
    }
    
    return "// Documentation generation service not available";
}

std::string CodeGenerationService::explainCode(
    const std::string& code,
    const CodeGenerationContext& context) {
    
    if (generator_) {
        return generator_->explainCode(code, context);
    }
    
    return "Code explanation service not available";
}

bool CodeGenerationService::isReady() const {
    return generator_ && generator_->isReady();
}

} // namespace bolt