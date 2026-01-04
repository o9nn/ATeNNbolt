#include "bolt/ai/ai_refactoring_engine.hpp"
#include <algorithm>
#include <sstream>
#include <regex>

namespace bolt {

// TemplateRefactoringEngine implementation
TemplateRefactoringEngine::TemplateRefactoringEngine() {
    loadDefaultPatterns();
}

void TemplateRefactoringEngine::loadDefaultPatterns() {
    // Long function pattern
    RefactoringPattern longFunction;
    longFunction.name = "long_function";
    longFunction.description = "Detect long functions that should be extracted";
    longFunction.pattern = std::regex(R"((\w+)\s*\([^)]*\)\s*\{([^}]{200,})\})"); // Functions with 200+ chars
    longFunction.suggestionGenerator = [this](const std::smatch& match, const std::string& filePath) {
        return createExtractMethodSuggestion(match, filePath);
    };
    patterns_.push_back(longFunction);
    
    // Nested conditional pattern
    RefactoringPattern nestedConditionals;
    nestedConditionals.name = "nested_conditionals";
    nestedConditionals.description = "Detect deeply nested conditional statements";
    nestedConditionals.pattern = std::regex(R"(if\s*\([^{]*\{\s*if\s*\([^{]*\{\s*if)");
    nestedConditionals.suggestionGenerator = [this](const std::smatch& match, const std::string& filePath) {
        return createComplexityReductionSuggestion(match, filePath);
    };
    patterns_.push_back(nestedConditionals);
    
    // Duplicate code pattern
    RefactoringPattern duplicateCode;
    duplicateCode.name = "duplicate_code";
    duplicateCode.description = "Detect potential code duplication";
    duplicateCode.pattern = std::regex(R"((\w+\s*=\s*[^;]+;[\s]*){3,})"); // Similar assignment patterns
    duplicateCode.suggestionGenerator = [this](const std::smatch& match, const std::string& filePath) {
        return createDuplicationRemovalSuggestion(match, filePath);
    };
    patterns_.push_back(duplicateCode);
}

bool TemplateRefactoringEngine::initialize() {
    if (!initialized_) {
        loadDefaultPatterns();
        initialized_ = true;
    }
    return initialized_;
}

AIRefactoringSuggestion TemplateRefactoringEngine::createExtractMethodSuggestion(
    const std::smatch& match, const std::string& filePath) const {
    
    AIRefactoringSuggestion suggestion;
    suggestion.id = "EXTRACT_METHOD_" + std::to_string(std::hash<std::string>{}(match.str()));
    suggestion.title = "Extract method from long function";
    suggestion.description = "This function is quite long and could benefit from being broken down into smaller, more focused methods.";
    suggestion.filePath = filePath;
    suggestion.originalCode = match.str();
    suggestion.refactoringType = "extract_method";
    suggestion.confidenceScore = 0.8;
    suggestion.priority = AIRefactoringSuggestion::Priority::HIGH;
    suggestion.impact = AIRefactoringSuggestion::Impact::MODERATE;
    suggestion.difficulty = AIRefactoringSuggestion::Difficulty::MODERATE;
    suggestion.estimatedTimeMinutes = 45;
    
    suggestion.benefits = {
        "Improved readability and maintainability",
        "Better testability of individual components",
        "Reduced cognitive complexity",
        "Enhanced code reusability"
    };
    
    suggestion.risks = {
        "Potential over-fragmentation if taken too far",
        "May introduce slight performance overhead from additional function calls"
    };
    
    suggestion.aiReasoning = "Large functions are harder to understand, test, and maintain. "
                           "Breaking them down into smaller, focused methods improves code quality.";
    
    // Generate suggested refactored code
    std::string functionName = match[1].str();
    suggestion.suggestedCode = "// Extracted methods from " + functionName + "\n"
                             "void " + functionName + "_step1() {\n"
                             "    // TODO: Move first logical block here\n"
                             "}\n\n"
                             "void " + functionName + "_step2() {\n"
                             "    // TODO: Move second logical block here\n"
                             "}\n\n"
                             "void " + functionName + "() {\n"
                             "    " + functionName + "_step1();\n"
                             "    " + functionName + "_step2();\n"
                             "}";
    
    suggestion.alternativeApproaches = {
        "Use RAII pattern to manage resource lifecycle",
        "Apply Strategy pattern if the function has multiple behavioral variations",
        "Consider Command pattern for complex operations"
    };
    
    return suggestion;
}

AIRefactoringSuggestion TemplateRefactoringEngine::createComplexityReductionSuggestion(
    const std::smatch& match, const std::string& filePath) const {
    
    AIRefactoringSuggestion suggestion;
    suggestion.id = "REDUCE_COMPLEXITY_" + std::to_string(std::hash<std::string>{}(match.str()));
    suggestion.title = "Reduce conditional complexity";
    suggestion.description = "Deeply nested conditional statements make code harder to read and maintain.";
    suggestion.filePath = filePath;
    suggestion.originalCode = match.str();
    suggestion.refactoringType = "reduce_complexity";
    suggestion.confidenceScore = 0.7;
    suggestion.priority = AIRefactoringSuggestion::Priority::MEDIUM;
    suggestion.impact = AIRefactoringSuggestion::Impact::MODERATE;
    suggestion.difficulty = AIRefactoringSuggestion::Difficulty::MODERATE;
    suggestion.estimatedTimeMinutes = 30;
    
    suggestion.benefits = {
        "Improved readability and comprehension",
        "Reduced cyclomatic complexity",
        "Easier to test individual conditions",
        "Better error handling possibilities"
    };
    
    suggestion.risks = {
        "May require restructuring of logic flow",
        "Could introduce temporary variables"
    };
    
    suggestion.aiReasoning = "Nested conditionals increase cognitive load. Using early returns, "
                           "guard clauses, or extracting conditions into well-named functions improves clarity.";
    
    suggestion.suggestedCode = "// Refactored with early returns and guard clauses\n"
                             "if (!condition1) {\n"
                             "    return handleInvalidCondition1();\n"
                             "}\n"
                             "if (!condition2) {\n"
                             "    return handleInvalidCondition2();\n"
                             "}\n"
                             "// Main logic here with reduced nesting";
    
    suggestion.alternativeApproaches = {
        "Use polymorphism to replace conditional logic",
        "Apply State pattern for complex state-dependent behavior",
        "Consider using lookup tables for simple conditional mappings"
    };
    
    return suggestion;
}

AIRefactoringSuggestion TemplateRefactoringEngine::createDuplicationRemovalSuggestion(
    const std::smatch& match, const std::string& filePath) const {
    
    AIRefactoringSuggestion suggestion;
    suggestion.id = "REMOVE_DUPLICATION_" + std::to_string(std::hash<std::string>{}(match.str()));
    suggestion.title = "Extract common code to eliminate duplication";
    suggestion.description = "Similar code patterns detected that could be consolidated.";
    suggestion.filePath = filePath;
    suggestion.originalCode = match.str();
    suggestion.refactoringType = "remove_duplication";
    suggestion.confidenceScore = 0.6;
    suggestion.priority = AIRefactoringSuggestion::Priority::MEDIUM;
    suggestion.impact = AIRefactoringSuggestion::Impact::MODERATE;
    suggestion.difficulty = AIRefactoringSuggestion::Difficulty::MODERATE;
    suggestion.estimatedTimeMinutes = 25;
    
    suggestion.benefits = {
        "Reduced code duplication (DRY principle)",
        "Easier maintenance - changes in one place",
        "Smaller codebase and improved consistency",
        "Reduced chance of bugs from inconsistent duplicates"
    };
    
    suggestion.risks = {
        "May over-abstract if patterns aren't truly the same",
        "Could introduce coupling between previously independent code"
    };
    
    suggestion.aiReasoning = "Code duplication leads to maintenance issues. Extracting common patterns "
                           "into reusable functions or templates improves code quality.";
    
    suggestion.suggestedCode = "// Extract common pattern into helper function\n"
                             "template<typename T>\n"
                             "void setValueIfValid(T& target, const T& value, bool condition) {\n"
                             "    if (condition) {\n"
                             "        target = value;\n"
                             "    }\n"
                             "}\n\n"
                             "// Use the helper function to replace duplicated code";
    
    suggestion.alternativeApproaches = {
        "Use template metaprogramming for type-safe abstractions",
        "Apply Builder pattern for complex object construction",
        "Consider using factory functions for similar creation logic"
    };
    
    return suggestion;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::generateSuggestions(
    const std::string& code,
    const std::string& filePath,
    const std::string& language) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    for (const auto& pattern : patterns_) {
        std::sregex_iterator iter(code.begin(), code.end(), pattern.pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            auto suggestion = pattern.suggestionGenerator(*iter, filePath);
            suggestions.push_back(suggestion);
        }
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::findExtractMethodOpportunities(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    // Look for functions with high line count
    std::regex functionPattern(R"((\w+)\s*\([^)]*\)\s*\{([^}]*)\})");
    std::sregex_iterator iter(code.begin(), code.end(), functionPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string functionBody = (*iter)[2].str();
        size_t lineCount = std::count(functionBody.begin(), functionBody.end(), '\n');
        
        if (lineCount > 15) { // Threshold for "long" function
            auto suggestion = createExtractMethodSuggestion(*iter, filePath);
            suggestion.confidenceScore = std::min(1.0, 0.5 + (lineCount - 15) * 0.02);
            suggestions.push_back(suggestion);
        }
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::findComplexityReductions(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    // Look for nested conditionals
    std::regex nestedPattern(R"(if\s*\([^{]*\{[^}]*if\s*\([^{]*\{[^}]*if)");
    std::sregex_iterator iter(code.begin(), code.end(), nestedPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        auto suggestion = createComplexityReductionSuggestion(*iter, filePath);
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::findDuplicationRemoval(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    // Simple pattern for similar code blocks
    std::regex duplicatePattern(R"((\w+\s*=\s*[^;]+;[\s\n]*){3,})");
    std::sregex_iterator iter(code.begin(), code.end(), duplicatePattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        auto suggestion = createDuplicationRemovalSuggestion(*iter, filePath);
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::findDesignPatternOpportunities(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    // Look for switch statements that could benefit from polymorphism
    std::regex switchPattern(R"(switch\s*\([^{]*\{([^}]*case[^}]*){3,}\})");
    std::sregex_iterator iter(code.begin(), code.end(), switchPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        AIRefactoringSuggestion suggestion;
        suggestion.id = "DESIGN_PATTERN_" + std::to_string(std::hash<std::string>{}(iter->str()));
        suggestion.title = "Consider using polymorphism instead of switch";
        suggestion.description = "Large switch statements can often be replaced with polymorphic design.";
        suggestion.filePath = filePath;
        suggestion.originalCode = iter->str();
        suggestion.refactoringType = "design_pattern";
        suggestion.confidenceScore = 0.6;
        suggestion.priority = AIRefactoringSuggestion::Priority::LOW;
        suggestion.impact = AIRefactoringSuggestion::Impact::SIGNIFICANT;
        suggestion.difficulty = AIRefactoringSuggestion::Difficulty::HARD;
        suggestion.estimatedTimeMinutes = 120;
        
        suggestion.benefits = {
            "Improved extensibility - easy to add new cases",
            "Better adherence to Open-Closed Principle",
            "Eliminates need for large switch statements",
            "More object-oriented design"
        };
        
        suggestion.risks = {
            "Increased complexity for simple cases",
            "May be overkill for small switch statements"
        };
        
        suggestion.aiReasoning = "Large switch statements often indicate missing polymorphism. "
                               "Using virtual functions or strategy pattern can improve design.";
        
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::findPerformanceOptimizations(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    // Look for potential string concatenation in loops
    std::regex stringConcatPattern(R"(for\s*\([^{]*\{[^}]*\+\s*=\s*[^}]*std::string)");
    std::sregex_iterator iter(code.begin(), code.end(), stringConcatPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        AIRefactoringSuggestion suggestion;
        suggestion.id = "PERF_OPT_" + std::to_string(std::hash<std::string>{}(iter->str()));
        suggestion.title = "Use stringstream for string concatenation in loops";
        suggestion.description = "String concatenation in loops can be expensive. Consider using stringstream.";
        suggestion.filePath = filePath;
        suggestion.originalCode = iter->str();
        suggestion.refactoringType = "performance";
        suggestion.confidenceScore = 0.7;
        suggestion.priority = AIRefactoringSuggestion::Priority::MEDIUM;
        suggestion.impact = AIRefactoringSuggestion::Impact::MODERATE;
        suggestion.difficulty = AIRefactoringSuggestion::Difficulty::EASY;
        suggestion.estimatedTimeMinutes = 15;
        
        suggestion.benefits = {
            "Better performance for large strings",
            "Reduced memory allocations",
            "More efficient for multiple concatenations"
        };
        
        suggestion.suggestedCode = "std::ostringstream oss;\n"
                                 "for (...) {\n"
                                 "    oss << value;\n"
                                 "}\n"
                                 "std::string result = oss.str();";
        
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> TemplateRefactoringEngine::findSecurityImprovements(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    // Look for potential buffer overflow issues
    std::regex bufferPattern(R"(char\s+\w+\[\d+\].*gets\s*\()");
    std::sregex_iterator iter(code.begin(), code.end(), bufferPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        AIRefactoringSuggestion suggestion;
        suggestion.id = "SECURITY_" + std::to_string(std::hash<std::string>{}(iter->str()));
        suggestion.title = "Replace unsafe gets() with safer alternative";
        suggestion.description = "The gets() function is unsafe and can lead to buffer overflows.";
        suggestion.filePath = filePath;
        suggestion.originalCode = iter->str();
        suggestion.refactoringType = "security";
        suggestion.confidenceScore = 0.95;
        suggestion.priority = AIRefactoringSuggestion::Priority::CRITICAL;
        suggestion.impact = AIRefactoringSuggestion::Impact::MAJOR;
        suggestion.difficulty = AIRefactoringSuggestion::Difficulty::EASY;
        suggestion.estimatedTimeMinutes = 10;
        
        suggestion.benefits = {
            "Eliminates buffer overflow vulnerability",
            "Improves application security",
            "Follows modern C++ best practices"
        };
        
        suggestion.risks = {
            "Requires checking return value of safer function"
        };
        
        suggestion.suggestedCode = "std::string input;\n"
                                 "std::getline(std::cin, input);";
        
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

std::string TemplateRefactoringEngine::applyRefactoring(
    const std::string& code,
    const AIRefactoringSuggestion& suggestion) {
    
    // Simple string replacement for basic refactoring
    std::string result = code;
    
    if (!suggestion.originalCode.empty() && !suggestion.suggestedCode.empty()) {
        size_t pos = result.find(suggestion.originalCode);
        if (pos != std::string::npos) {
            result.replace(pos, suggestion.originalCode.length(), suggestion.suggestedCode);
        }
    }
    
    return result;
}

bool TemplateRefactoringEngine::validateRefactoring(
    const std::string& originalCode,
    const std::string& refactoredCode,
    const AIRefactoringSuggestion& suggestion) {
    
    // Basic validation - check that the refactored code is different and not empty
    return !refactoredCode.empty() && 
           refactoredCode != originalCode &&
           refactoredCode.length() > 10; // Basic sanity check
}

// AIEnhancedRefactoringEngine implementation
AIEnhancedRefactoringEngine::AIEnhancedRefactoringEngine()
    : templateEngine_(std::make_unique<TemplateRefactoringEngine>()) {
}

AIEnhancedRefactoringEngine::AIEnhancedRefactoringEngine(std::unique_ptr<AICompletionProvider> provider)
    : aiProvider_(std::move(provider))
    , templateEngine_(std::make_unique<TemplateRefactoringEngine>()) {
}

void AIEnhancedRefactoringEngine::setAIProvider(std::unique_ptr<AICompletionProvider> provider) {
    aiProvider_ = std::move(provider);
}

bool AIEnhancedRefactoringEngine::initialize() {
    bool result = true;
    
    if (templateEngine_) {
        result &= templateEngine_->initialize();
    }
    
    if (aiProvider_) {
        result &= aiProvider_->initialize();
    }
    
    initialized_ = result;
    return result;
}

bool AIEnhancedRefactoringEngine::isReady() const {
    return initialized_ && 
           ((aiProvider_ && aiProvider_->isReady()) || 
            (fallbackToTemplate_ && templateEngine_ && templateEngine_->isReady()));
}

void AIEnhancedRefactoringEngine::shutdown() {
    if (aiProvider_) {
        aiProvider_->shutdown();
    }
    if (templateEngine_) {
        templateEngine_->shutdown();
    }
    initialized_ = false;
}

std::string AIEnhancedRefactoringEngine::createRefactoringPrompt(
    const std::string& code,
    const std::string& refactoringType,
    const std::string& context) const {
    
    std::ostringstream prompt;
    prompt << "Analyze this C++ code and suggest refactoring improvements for: " << refactoringType << "\n\n";
    prompt << "Code to analyze:\n" << code << "\n\n";
    
    if (!context.empty()) {
        prompt << "Additional context: " << context << "\n\n";
    }
    
    prompt << "Please provide:\n";
    prompt << "1. Specific refactoring suggestions\n";
    prompt << "2. Improved code examples\n";
    prompt << "3. Benefits and potential risks\n";
    prompt << "4. Estimated difficulty and time\n";
    
    return prompt.str();
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::generateSuggestions(
    const std::string& code,
    const std::string& filePath,
    const std::string& language) {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    if (aiProvider_ && aiProvider_->isReady()) {
        std::string prompt = createRefactoringPrompt(code, "general refactoring", "");
        
        CodeContext aiContext;
        aiContext.content = prompt;
        aiContext.language = language;
        aiContext.filePath = filePath;
        aiContext.cursorPosition = prompt.length();
        
        auto completions = aiProvider_->generateCompletions(aiContext, prompt, 3);
        if (!completions.empty()) {
            auto aiSuggestions = processAIRefactoringResponse(completions, code, filePath, "general");
            suggestions.insert(suggestions.end(), aiSuggestions.begin(), aiSuggestions.end());
        }
    }
    
    // Fallback to template-based suggestions
    if (fallbackToTemplate_ && templateEngine_) {
        auto templateSuggestions = templateEngine_->generateSuggestions(code, filePath, language);
        suggestions.insert(suggestions.end(), templateSuggestions.begin(), templateSuggestions.end());
    }
    
    return suggestions;
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::processAIRefactoringResponse(
    const std::vector<CompletionItem>& completions,
    const std::string& originalCode,
    const std::string& filePath,
    const std::string& refactoringType) const {
    
    std::vector<AIRefactoringSuggestion> suggestions;
    
    for (const auto& completion : completions) {
        auto suggestion = parseRefactoringSuggestion(completion.label, originalCode, filePath, refactoringType);
        suggestion.confidenceScore = completion.score;
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

AIRefactoringSuggestion AIEnhancedRefactoringEngine::parseRefactoringSuggestion(
    const std::string& response,
    const std::string& originalCode,
    const std::string& filePath,
    const std::string& refactoringType) const {
    
    AIRefactoringSuggestion suggestion;
    suggestion.id = "AI_" + refactoringType + "_" + std::to_string(std::hash<std::string>{}(response));
    suggestion.title = "AI-suggested refactoring";
    suggestion.description = "AI-generated refactoring suggestion";
    suggestion.filePath = filePath;
    suggestion.originalCode = originalCode;
    suggestion.refactoringType = refactoringType;
    suggestion.aiReasoning = response;
    suggestion.priority = AIRefactoringSuggestion::Priority::MEDIUM;
    suggestion.impact = AIRefactoringSuggestion::Impact::MODERATE;
    suggestion.difficulty = AIRefactoringSuggestion::Difficulty::MODERATE;
    suggestion.estimatedTimeMinutes = 30;
    
    // Extract suggested code if present in response
    std::regex codePattern(R"(```(?:cpp|c\+\+)?\s*(.*?)\s*```)");
    std::smatch match;
    if (std::regex_search(response, match, codePattern)) {
        suggestion.suggestedCode = match[1].str();
    } else {
        suggestion.suggestedCode = "// See AI reasoning for detailed suggestions";
    }
    
    return suggestion;
}

// Implementation of other methods follows similar pattern...
// For brevity, implementing key methods and using template engine as fallback

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::findExtractMethodOpportunities(
    const std::string& code,
    const std::string& filePath) {
    
    if (aiProvider_ && aiProvider_->isReady()) {
        // AI-enhanced analysis would go here
        // For now, fallback to template engine
    }
    
    if (fallbackToTemplate_ && templateEngine_) {
        return templateEngine_->findExtractMethodOpportunities(code, filePath);
    }
    
    return {};
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::findComplexityReductions(
    const std::string& code,
    const std::string& filePath) {
    
    if (fallbackToTemplate_ && templateEngine_) {
        return templateEngine_->findComplexityReductions(code, filePath);
    }
    return {};
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::findDuplicationRemoval(
    const std::string& code,
    const std::string& filePath) {
    
    if (fallbackToTemplate_ && templateEngine_) {
        return templateEngine_->findDuplicationRemoval(code, filePath);
    }
    return {};
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::findDesignPatternOpportunities(
    const std::string& code,
    const std::string& filePath) {
    
    if (fallbackToTemplate_ && templateEngine_) {
        return templateEngine_->findDesignPatternOpportunities(code, filePath);
    }
    return {};
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::findPerformanceOptimizations(
    const std::string& code,
    const std::string& filePath) {
    
    if (fallbackToTemplate_ && templateEngine_) {
        return templateEngine_->findPerformanceOptimizations(code, filePath);
    }
    return {};
}

std::vector<AIRefactoringSuggestion> AIEnhancedRefactoringEngine::findSecurityImprovements(
    const std::string& code,
    const std::string& filePath) {
    
    if (fallbackToTemplate_ && templateEngine_) {
        return templateEngine_->findSecurityImprovements(code, filePath);
    }
    return {};
}

std::string AIEnhancedRefactoringEngine::applyRefactoring(
    const std::string& code,
    const AIRefactoringSuggestion& suggestion) {
    
    if (templateEngine_) {
        return templateEngine_->applyRefactoring(code, suggestion);
    }
    return code;
}

bool AIEnhancedRefactoringEngine::validateRefactoring(
    const std::string& originalCode,
    const std::string& refactoredCode,
    const AIRefactoringSuggestion& suggestion) {
    
    if (templateEngine_) {
        return templateEngine_->validateRefactoring(originalCode, refactoredCode, suggestion);
    }
    return false;
}

// RefactoringService implementation
std::unique_ptr<RefactoringService> RefactoringService::instance_;

RefactoringService& RefactoringService::getInstance() {
    if (!instance_) {
        instance_ = std::unique_ptr<RefactoringService>(new RefactoringService());
        instance_->setEngine(std::make_unique<AIEnhancedRefactoringEngine>());
    }
    return *instance_;
}

void RefactoringService::setEngine(std::unique_ptr<AIRefactoringEngine> engine) {
    engine_ = std::move(engine);
    if (engine_) {
        engine_->initialize();
    }
}

std::vector<AIRefactoringSuggestion> RefactoringService::analyzeCode(
    const std::string& code, 
    const std::string& filePath) {
    
    stats_.totalSuggestionsGenerated++;
    
    if (engine_) {
        return engine_->generateSuggestions(code, filePath);
    }
    
    return {};
}

std::vector<AIRefactoringSuggestion> RefactoringService::findAllOpportunities(
    const std::string& code,
    const std::string& filePath) {
    
    std::vector<AIRefactoringSuggestion> allSuggestions;
    
    if (engine_) {
        auto extractSuggestions = engine_->findExtractMethodOpportunities(code, filePath);
        auto complexitySuggestions = engine_->findComplexityReductions(code, filePath);
        auto duplicationSuggestions = engine_->findDuplicationRemoval(code, filePath);
        auto designSuggestions = engine_->findDesignPatternOpportunities(code, filePath);
        auto perfSuggestions = engine_->findPerformanceOptimizations(code, filePath);
        auto securitySuggestions = engine_->findSecurityImprovements(code, filePath);
        
        allSuggestions.insert(allSuggestions.end(), extractSuggestions.begin(), extractSuggestions.end());
        allSuggestions.insert(allSuggestions.end(), complexitySuggestions.begin(), complexitySuggestions.end());
        allSuggestions.insert(allSuggestions.end(), duplicationSuggestions.begin(), duplicationSuggestions.end());
        allSuggestions.insert(allSuggestions.end(), designSuggestions.begin(), designSuggestions.end());
        allSuggestions.insert(allSuggestions.end(), perfSuggestions.begin(), perfSuggestions.end());
        allSuggestions.insert(allSuggestions.end(), securitySuggestions.begin(), securitySuggestions.end());
    }
    
    stats_.totalSuggestionsGenerated += allSuggestions.size();
    
    return allSuggestions;
}

std::string RefactoringService::applyRefactoring(
    const std::string& code, 
    const AIRefactoringSuggestion& suggestion) {
    
    if (engine_) {
        auto result = engine_->applyRefactoring(code, suggestion);
        if (result != code) {
            stats_.suggestionsApplied++;
            if (engine_->validateRefactoring(code, result, suggestion)) {
                stats_.successfulRefactorings++;
            }
        }
        return result;
    }
    
    return code;
}

bool RefactoringService::validateRefactoring(
    const std::string& originalCode,
    const std::string& refactoredCode,
    const AIRefactoringSuggestion& suggestion) {
    
    if (engine_) {
        return engine_->validateRefactoring(originalCode, refactoredCode, suggestion);
    }
    
    return false;
}

bool RefactoringService::isReady() const {
    return engine_ && engine_->isReady();
}

RefactoringService::RefactoringStats RefactoringService::getStatistics() const {
    return stats_;
}

void RefactoringService::resetStatistics() {
    stats_ = RefactoringStats{};
}

} // namespace bolt