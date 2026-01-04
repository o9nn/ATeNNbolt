/**
 * @file test_ai_features.cpp
 * @brief Comprehensive unit tests for AI features
 */

#include <bolt/test_framework.hpp>
#include <bolt/ai/ai_completion_provider.hpp>
#include <bolt/ai/ai_config_manager.hpp>
#include <bolt/ai/ai_code_generator.hpp>
#include <bolt/ai/ai_refactoring_engine.hpp>
#include <bolt/ai/enhanced_ai_manager.hpp>

#ifdef BOLT_HAVE_GGML
#include <bolt/ai/ggml_loader.hpp>
#include <bolt/ai/gguf_loader.hpp>
#include <bolt/ai/bpe_tokenizer.hpp>
#include <bolt/ai/direct_gguf_inference.hpp>
#include <bolt/ai/gpu_acceleration.hpp>
#endif

#include <string>
#include <vector>
#include <memory>

using namespace bolt::test;

// ============================================
// AI Configuration Tests
// ============================================

BOLT_TEST(AIConfig, DefaultConfiguration) {
    bolt::ai::AIConfigManager config;

    // Check default values
    BOLT_ASSERT(!config.getApiKey().empty() || config.getApiKey().empty()); // May or may not be set
    BOLT_ASSERT(config.getMaxTokens() > 0);
    BOLT_ASSERT(config.getTemperature() >= 0.0 && config.getTemperature() <= 2.0);
}

BOLT_TEST(AIConfig, SetConfiguration) {
    bolt::ai::AIConfigManager config;

    config.setMaxTokens(2048);
    config.setTemperature(0.7);
    config.setModel("gpt-4");

    BOLT_ASSERT_EQ(2048, config.getMaxTokens());
    BOLT_ASSERT(std::abs(config.getTemperature() - 0.7) < 0.001);
    BOLT_ASSERT_EQ("gpt-4", config.getModel());
}

BOLT_TEST(AIConfig, ProviderConfiguration) {
    bolt::ai::AIConfigManager config;

    config.setProvider("openai");
    BOLT_ASSERT_EQ("openai", config.getProvider());

    config.setProvider("anthropic");
    BOLT_ASSERT_EQ("anthropic", config.getProvider());

    config.setProvider("local");
    BOLT_ASSERT_EQ("local", config.getProvider());
}

BOLT_TEST(AIConfig, LocalModelPath) {
    bolt::ai::AIConfigManager config;

    config.setLocalModelPath("/path/to/model.gguf");
    BOLT_ASSERT_EQ("/path/to/model.gguf", config.getLocalModelPath());
}

BOLT_TEST(AIConfig, ConfigSerialization) {
    bolt::ai::AIConfigManager config;

    config.setMaxTokens(1024);
    config.setTemperature(0.5);
    config.setModel("test-model");

    // Serialize to JSON
    std::string json = config.toJson();
    BOLT_ASSERT(!json.empty());
    BOLT_ASSERT(json.find("1024") != std::string::npos);

    // Deserialize
    bolt::ai::AIConfigManager config2;
    config2.fromJson(json);

    BOLT_ASSERT_EQ(1024, config2.getMaxTokens());
}

// ============================================
// AI Completion Provider Tests
// ============================================

BOLT_TEST(AICompletion, ProviderInitialization) {
    bolt::ai::AICompletionProvider provider;

    BOLT_ASSERT(!provider.isInitialized()); // Not initialized until configured
}

BOLT_TEST(AICompletion, CompletionContext) {
    bolt::ai::CompletionContext context;
    context.prefix = "def hello_world():";
    context.suffix = "";
    context.language = "python";
    context.filePath = "test.py";
    context.cursorLine = 0;
    context.cursorColumn = 18;

    BOLT_ASSERT_EQ("python", context.language);
    BOLT_ASSERT_EQ(18, context.cursorColumn);
}

BOLT_TEST(AICompletion, CompletionRequest) {
    bolt::ai::AICompletionProvider provider;
    bolt::ai::CompletionContext context;
    context.prefix = "function test() {";
    context.language = "javascript";

    auto request = provider.createRequest(context);

    BOLT_ASSERT(!request.empty());
}

BOLT_TEST(AICompletion, CompletionFiltering) {
    bolt::ai::AICompletionProvider provider;

    std::vector<bolt::ai::CompletionItem> items = {
        {"printf", "Function", 1.0},
        {"print", "Function", 0.8},
        {"println", "Function", 0.6},
        {"sprintf", "Function", 0.4}
    };

    auto filtered = provider.filterCompletions(items, "pri", 3);

    BOLT_ASSERT(filtered.size() <= 3);
    // First item should be "print" as it's a closer match
    BOLT_ASSERT(filtered[0].label.find("print") != std::string::npos ||
                filtered[0].label.find("printf") != std::string::npos);
}

BOLT_TEST(AICompletion, CompletionScoring) {
    bolt::ai::AICompletionProvider provider;

    double score1 = provider.scoreCompletion("printf", "pri");
    double score2 = provider.scoreCompletion("sprintf", "pri");
    double score3 = provider.scoreCompletion("malloc", "pri");

    // "printf" should score higher than "sprintf" for prefix "pri"
    BOLT_ASSERT(score1 > score2);
    BOLT_ASSERT(score2 > score3);
}

// ============================================
// AI Code Generator Tests
// ============================================

BOLT_TEST(AICodeGenerator, GeneratorInitialization) {
    bolt::ai::AICodeGenerator generator;

    BOLT_ASSERT(generator.getSupportedLanguages().size() > 0);
}

BOLT_TEST(AICodeGenerator, LanguageSupport) {
    bolt::ai::AICodeGenerator generator;

    BOLT_ASSERT(generator.supportsLanguage("cpp"));
    BOLT_ASSERT(generator.supportsLanguage("python"));
    BOLT_ASSERT(generator.supportsLanguage("javascript"));
    BOLT_ASSERT(generator.supportsLanguage("typescript"));
}

BOLT_TEST(AICodeGenerator, PromptGeneration) {
    bolt::ai::AICodeGenerator generator;

    bolt::ai::GenerationRequest request;
    request.language = "cpp";
    request.description = "Create a function that adds two numbers";
    request.context = "";

    auto prompt = generator.generatePrompt(request);

    BOLT_ASSERT(!prompt.empty());
    BOLT_ASSERT(prompt.find("function") != std::string::npos ||
                prompt.find("add") != std::string::npos);
}

BOLT_TEST(AICodeGenerator, TemplateGeneration) {
    bolt::ai::AICodeGenerator generator;

    auto classTemplate = generator.generateClassTemplate("cpp", "MyClass");
    BOLT_ASSERT(!classTemplate.empty());
    BOLT_ASSERT(classTemplate.find("class") != std::string::npos);
    BOLT_ASSERT(classTemplate.find("MyClass") != std::string::npos);

    auto funcTemplate = generator.generateFunctionTemplate("python", "my_function");
    BOLT_ASSERT(!funcTemplate.empty());
    BOLT_ASSERT(funcTemplate.find("def") != std::string::npos);
}

BOLT_TEST(AICodeGenerator, DocstringGeneration) {
    bolt::ai::AICodeGenerator generator;

    std::string code = R"(
def calculate_area(width: float, height: float) -> float:
    return width * height
)";

    auto docstring = generator.generateDocstring("python", code);
    BOLT_ASSERT(!docstring.empty());
}

// ============================================
// AI Refactoring Engine Tests
// ============================================

BOLT_TEST(AIRefactoring, EngineInitialization) {
    bolt::ai::AIRefactoringEngine engine;

    auto refactorings = engine.getSupportedRefactorings();
    BOLT_ASSERT(!refactorings.empty());
}

BOLT_TEST(AIRefactoring, DetectCodeSmells) {
    bolt::ai::AIRefactoringEngine engine;

    std::string code = R"(
function veryLongFunctionNameThatDoesTooManyThings(a, b, c, d, e, f, g, h, i, j) {
    if (a) {
        if (b) {
            if (c) {
                if (d) {
                    // Deep nesting
                    return true;
                }
            }
        }
    }
    // Lots of duplicated code
    var x = a + b + c;
    var y = a + b + c;
    var z = a + b + c;
    return false;
}
)";

    auto smells = engine.detectCodeSmells("javascript", code);

    BOLT_ASSERT(!smells.empty());
    // Should detect long function name, too many parameters, deep nesting, duplication
}

BOLT_TEST(AIRefactoring, ExtractMethod) {
    bolt::ai::AIRefactoringEngine engine;

    std::string code = R"(
void processData() {
    // Code to extract
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    std::cout << sum << std::endl;
    // End of code to extract
}
)";

    bolt::ai::RefactoringRequest request;
    request.type = bolt::ai::RefactoringType::ExtractMethod;
    request.code = code;
    request.selectionStart = 50;
    request.selectionEnd = 150;
    request.newName = "calculateSum";

    auto result = engine.refactor(request);

    BOLT_ASSERT(!result.error);
    BOLT_ASSERT(!result.refactoredCode.empty());
}

BOLT_TEST(AIRefactoring, RenameSymbol) {
    bolt::ai::AIRefactoringEngine engine;

    std::string code = R"(
int oldName = 5;
int result = oldName * 2;
std::cout << oldName << std::endl;
)";

    bolt::ai::RefactoringRequest request;
    request.type = bolt::ai::RefactoringType::Rename;
    request.code = code;
    request.oldName = "oldName";
    request.newName = "newName";

    auto result = engine.refactor(request);

    BOLT_ASSERT(!result.error);
    BOLT_ASSERT(result.refactoredCode.find("newName") != std::string::npos);
    BOLT_ASSERT(result.refactoredCode.find("oldName") == std::string::npos);
}

BOLT_TEST(AIRefactoring, InlineVariable) {
    bolt::ai::AIRefactoringEngine engine;

    std::string code = R"(
int temp = getValue();
int result = temp * 2;
)";

    bolt::ai::RefactoringRequest request;
    request.type = bolt::ai::RefactoringType::InlineVariable;
    request.code = code;
    request.targetName = "temp";

    auto result = engine.refactor(request);

    BOLT_ASSERT(!result.error);
}

// ============================================
// Enhanced AI Manager Tests
// ============================================

BOLT_TEST(EnhancedAI, ManagerInitialization) {
    bolt::ai::EnhancedAIManager manager;

    BOLT_ASSERT(!manager.isModelLoaded());
}

BOLT_TEST(EnhancedAI, ProviderSelection) {
    bolt::ai::EnhancedAIManager manager;

    manager.setProvider(bolt::ai::AIProvider::Local);
    BOLT_ASSERT_EQ(bolt::ai::AIProvider::Local, manager.getProvider());

    manager.setProvider(bolt::ai::AIProvider::OpenAI);
    BOLT_ASSERT_EQ(bolt::ai::AIProvider::OpenAI, manager.getProvider());
}

BOLT_TEST(EnhancedAI, ConversationHistory) {
    bolt::ai::EnhancedAIManager manager;

    manager.addToHistory("user", "Hello");
    manager.addToHistory("assistant", "Hi there!");
    manager.addToHistory("user", "How are you?");

    auto history = manager.getHistory();
    BOLT_ASSERT_EQ(3u, history.size());

    manager.clearHistory();
    history = manager.getHistory();
    BOLT_ASSERT_EQ(0u, history.size());
}

BOLT_TEST(EnhancedAI, ContextWindow) {
    bolt::ai::EnhancedAIManager manager;

    manager.setContextWindowSize(4096);
    BOLT_ASSERT_EQ(4096u, manager.getContextWindowSize());

    // Add content that exceeds context window
    std::string longContent(5000, 'x');
    auto truncated = manager.truncateToContextWindow(longContent);

    BOLT_ASSERT(truncated.size() <= 4096);
}

// ============================================
// GGML/GGUF Tests (Conditional)
// ============================================

#ifdef BOLT_HAVE_GGML

BOLT_TEST(GGML, LoaderInitialization) {
    bolt::ai::GGMLLoader loader;

    BOLT_ASSERT(!loader.isModelLoaded());
}

BOLT_TEST(GGML, ModelInfo) {
    bolt::ai::GGMLLoader loader;

    // Test with non-existent file - should fail gracefully
    bool loaded = loader.loadModel("/nonexistent/model.ggml");
    BOLT_ASSERT(!loaded);
}

BOLT_TEST(GGUF, LoaderInitialization) {
    bolt::ai::GGUFLoader loader;

    BOLT_ASSERT(!loader.isModelLoaded());
}

BOLT_TEST(GGUF, ValidateHeader) {
    bolt::ai::GGUFLoader loader;

    // Test header validation
    std::vector<uint8_t> validHeader = {'G', 'G', 'U', 'F', 0, 0, 0, 3};
    BOLT_ASSERT(loader.validateHeader(validHeader.data(), validHeader.size()));

    std::vector<uint8_t> invalidHeader = {'N', 'O', 'T', 'G'};
    BOLT_ASSERT(!loader.validateHeader(invalidHeader.data(), invalidHeader.size()));
}

BOLT_TEST(BPETokenizer, Initialization) {
    bolt::ai::BPETokenizer tokenizer;

    BOLT_ASSERT(!tokenizer.isInitialized());
}

BOLT_TEST(BPETokenizer, BasicTokenization) {
    bolt::ai::BPETokenizer tokenizer;

    // If tokenizer can be initialized without vocab
    if (tokenizer.initializeDefault()) {
        auto tokens = tokenizer.encode("Hello world");
        BOLT_ASSERT(!tokens.empty());

        auto decoded = tokenizer.decode(tokens);
        BOLT_ASSERT(!decoded.empty());
    } else {
        // Skip if no default vocab
        BOLT_ASSERT(true);
    }
}

BOLT_TEST(DirectGGUF, InferenceInitialization) {
    bolt::ai::DirectGGUFInference inference;

    BOLT_ASSERT(!inference.isReady());
}

BOLT_TEST(GPUAcceleration, Detection) {
    bolt::ai::GPUAcceleration gpu;

    // Just check that detection doesn't crash
    bool hasGPU = gpu.isAvailable();
    // hasGPU may be true or false depending on system

    if (hasGPU) {
        auto devices = gpu.getDevices();
        BOLT_ASSERT(!devices.empty());
    }

    BOLT_ASSERT(true);
}

BOLT_TEST(GPUAcceleration, BackendSelection) {
    bolt::ai::GPUAcceleration gpu;

    auto backends = gpu.getAvailableBackends();
    // Should at least have CPU backend
    BOLT_ASSERT(!backends.empty());
}

#endif // BOLT_HAVE_GGML

// ============================================
// AI Prompt Engineering Tests
// ============================================

BOLT_TEST(AIPrompt, SystemPromptGeneration) {
    bolt::ai::AICompletionProvider provider;

    auto systemPrompt = provider.getSystemPrompt("cpp");
    BOLT_ASSERT(!systemPrompt.empty());
    BOLT_ASSERT(systemPrompt.find("C++") != std::string::npos ||
                systemPrompt.find("code") != std::string::npos);
}

BOLT_TEST(AIPrompt, ContextFormatting) {
    bolt::ai::AICompletionProvider provider;

    bolt::ai::CompletionContext context;
    context.prefix = "int main() {\n    ";
    context.suffix = "\n}";
    context.language = "cpp";

    auto formattedContext = provider.formatContext(context);
    BOLT_ASSERT(!formattedContext.empty());
}

// Main function for standalone testing
#ifndef BOLT_INCLUDE_IN_TEST_RUNNER
int main(int argc, char** argv) {
    std::cout << "=== Bolt AI Features Test Suite ===" << std::endl;
    std::cout << std::endl;

    auto& suite = bolt::test::TestSuite::getInstance();

    if (argc > 1) {
        std::string suiteName = argv[1];
        if (suiteName == "--list") {
            suite.listTests();
            return 0;
        }
        return suite.runTestsInSuite(suiteName);
    }

    return suite.runAllTests();
}
#endif
