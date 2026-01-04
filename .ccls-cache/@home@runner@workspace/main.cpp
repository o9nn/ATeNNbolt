
#include <iostream>
#include "bolt.hpp"
#include "websocket_server.hpp"

int main() {
    auto& wsServer = bolt::WebSocketServer::getInstance();
auto& completionEngine = bolt::CodeCompletionEngine::getInstance();
auto& highlighter = bolt::SyntaxHighlighter::getInstance();
auto& shortcuts = bolt::KeyboardShortcuts::getInstance();

// Register common shortcuts
shortcuts.registerShortcut({KEY_S, Modifier::CTRL, "Save"}, []() {
    // Save current file
    auto& editor = bolt::EditorStore::getInstance();
    editor.saveCurrentFile();
});

shortcuts.registerShortcut({KEY_F, Modifier::CTRL, "Find"}, []() {
    // Open find dialog
    auto& editor = bolt::EditorStore::getInstance();
    editor.openFindDialog();
});

shortcuts.registerShortcut({KEY_N, Modifier::CTRL, "New File"}, []() {
    // Create new file
    auto& workbench = bolt::WorkbenchStore::getInstance();
    workbench.createNewFile();
});

// Initialize syntax highlighting rules
highlighter.addLanguageRules("cpp", {
    {R"(\b(int|float|double|char|void|bool|auto|struct|class|namespace|template|typename|const|static|virtual)\b)", "keyword"},
    {R"(\b(if|else|for|while|do|switch|case|break|continue|return)\b)", "control"},
    {R"("(?:[^"\\]|\\.)*")", "string"},
    {R"(\b[0-9]+\b)", "number"},
    {R"(\b[A-Z][a-zA-Z0-9_]*\b)", "type"},
    {R"(//.*$)", "comment"},
    {R"(/\*[\s\S]*?\*/)", "comment"}
});

// Initialize with some basic completions
completionEngine.addCompletion({"class", "Define a class", "keyword", 100});
completionEngine.addCompletion({"struct", "Define a structure", "keyword", 100});
completionEngine.addCompletion({"namespace", "Define a namespace", "keyword", 100});
    wsServer.onMessage([](const std::string& msg, bolt::WebSocketConnection* conn) {
        std::cout << "Received message: " << msg << std::endl;
        conn->send("Server received: " + msg);
    });
    
    wsServer.start(8080);
    bolt::BoltApp::getInstance().run();
    wsServer.stop();
    std::cout << "String Operations:\n";
    std::cout << "Original string: " << test << std::endl;
    std::cout << "Reversed: " << StringUtils::reverseString(test) << std::endl;
    std::cout << "Capitalized: " << StringUtils::capitalizeString(test) << std::endl;
    std::cout << "Vowel count: " << StringUtils::countVowels(test) << std::endl;

    // Math operations demo
    double num = 16.0;
    std::cout << "\nMath Operations:\n";
    std::cout << "Square root of " << num << ": " << MathUtils::squareRoot(num) << std::endl;
    std::cout << num << " raised to power 3: " << MathUtils::power(num, 3) << std::endl;
    std::cout << "Factorial of 5: " << MathUtils::factorial(5) << std::endl;

    // Data processing demo
    std::vector<double> dataset = {4.5, 2.1, 6.7, 3.2, 9.1};
    std::cout << "\nData Processing:\n";
    try {
        std::cout << "Average: " << DataProcessor<double>::calculateAverage(dataset) << std::endl;
        std::cout << "Maximum: " << DataProcessor<double>::findMax(dataset) << std::endl;
        std::cout << "Minimum: " << DataProcessor<double>::findMin(dataset) << std::endl;
        
        auto sorted = DataProcessor<double>::sort(dataset);
        std::cout << "Sorted data: ";
        for (const auto& value : sorted) {
            std::cout << value << " ";
        }
        std::cout << std::endl;

        // File operations demo
        std::cout << "\nFile Operations:\n";
        FileSystemUtils::writeFile("test.txt", "This is a test file.\n");
        FileSystemUtils::appendToFile("test.txt", "This is appended text.\n");
        std::string content = FileSystemUtils::readFile("test.txt");
        std::cout << "File content:\n" << content;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
