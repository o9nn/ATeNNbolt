
#include "bolt/editor/code_folding_detector.hpp"
#include <stack>
#include <regex>
#include <algorithm>

namespace bolt {

std::vector<FoldRange> CodeFoldingDetector::detectFoldableRanges(const std::string& content) {
    std::vector<FoldRange> ranges;
    std::stack<size_t> bracketStack;
    size_t lineNumber = 0;
    bool inComment = false;
    
    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\n') {
            lineNumber++;
            continue;
        }
        
        // Handle multi-line comments
        if (i < content.length() - 1 && content[i] == '/' && content[i + 1] == '*') {
            bracketStack.push(lineNumber);
            inComment = true;
            i++;
            continue;
        }
        
        if (inComment && i < content.length() - 1 && content[i] == '*' && content[i + 1] == '/') {
            if (!bracketStack.empty()) {
                size_t startLine = bracketStack.top();
                bracketStack.pop();
                ranges.push_back({startLine, lineNumber, false, "/* ... */"});
            }
            inComment = false;
            i++;
            continue;
        }
        
        // Handle code blocks
        if (content[i] == '{' && !inComment) {
            bracketStack.push(lineNumber);
        }
        else if (content[i] == '}' && !inComment && !bracketStack.empty()) {
            size_t startLine = bracketStack.top();
            bracketStack.pop();
            if (lineNumber > startLine) {
                ranges.push_back({startLine, lineNumber, false, "{ ... }"});
            }
        }
    }
    
    return ranges;
}

bool CodeFoldingDetector::isFoldableRegion(const std::string& line) {
    // Common patterns for foldable regions
    static const std::vector<std::regex> patterns = {
        std::regex(R"(\{[\s]*$)"),                  // Opening brace
        std::regex(R"(^\s*function.*\{[\s]*$)"),    // Function definition
        std::regex(R"(^\s*class.*\{[\s]*$)"),       // Class definition
        std::regex(R"(^\s*if.*\{[\s]*$)"),          // If statement
        std::regex(R"(^\s*for.*\{[\s]*$)"),         // For loop
        std::regex(R"(^\s*while.*\{[\s]*$)")        // While loop
    };

    for (const auto& pattern : patterns) {
        if (std::regex_search(line, pattern)) {
            return true;
        }
    }
    return false;
}

int CodeFoldingDetector::findMatchingEnd(const std::vector<std::string>& lines, int startLine) {
    int braceCount = 1;
    int currentLine = startLine + 1;
    
    while (currentLine < static_cast<int>(lines.size()) && braceCount > 0) {
        const std::string& line = lines[currentLine];
        braceCount += std::count(line.begin(), line.end(), '{');
        braceCount -= std::count(line.begin(), line.end(), '}');
        currentLine++;
    }
    
    return braceCount == 0 ? currentLine - 1 : -1;
}

} // namespace bolt
