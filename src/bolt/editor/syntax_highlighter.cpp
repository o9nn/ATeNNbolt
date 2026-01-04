
#include "bolt/editor/syntax_highlighter.hpp"
#include <algorithm>
#include <sstream>

namespace bolt {

SyntaxHighlighter& SyntaxHighlighter::getInstance() {
    static SyntaxHighlighter instance;
    return instance;
}

void SyntaxHighlighter::addLanguageRules(const std::string& language, 
                     const std::vector<std::pair<std::string, std::string>>& patterns) {
    std::vector<std::pair<std::regex, std::string>> compiledRules;
    for (const auto& pattern : patterns) {
        compiledRules.emplace_back(std::regex(pattern.first), pattern.second);
    }
    
    rules_.write([&](auto& rulesMap) {
        rulesMap[language] = compiledRules;
    });
}

std::vector<Token> SyntaxHighlighter::highlight(const std::string& code, const std::string& language) {
    std::vector<Token> tokens;
    
    auto rules = rules_.read([&](const auto& rulesMap) {
        auto it = rulesMap.find(language);
        return it != rulesMap.end() ? it->second : std::vector<std::pair<std::regex, std::string>>();
    });

    size_t pos = 0;
    std::string remaining = code;

    while (!remaining.empty()) {
        bool matched = false;
        size_t bestMatch = remaining.length();
        std::string bestType;
        std::string matchedText;

        for (const auto& rule : rules) {
            std::smatch match;
            if (std::regex_search(remaining, match, rule.first, std::regex_constants::match_continuous)) {
                if (match.position(0) < bestMatch) {
                    bestMatch = match.position(0);
                    bestType = rule.second;
                    matchedText = match.str(0);
                    matched = true;
                }
            }
        }

        if (matched) {
            tokens.push_back({matchedText, bestType, pos});
            pos += matchedText.length();
            remaining = remaining.substr(matchedText.length());
        } else {
            tokens.push_back({remaining.substr(0, 1), "text", pos});
            pos += 1;
            remaining = remaining.substr(1);
        }
    }

    return tokens;
}

} // namespace bolt
#include "bolt/editor/syntax_highlighter.hpp"
#include <map>
#include <regex>
#include <string>

namespace bolt {

class SyntaxHighlighter {
private:
    std::map<std::string, std::vector<std::pair<std::regex, std::string>>> languageRules;
    bool enabled = true;

public:
    void addLanguageRule(const std::string& language, const std::string& pattern, const std::string& tokenType) {
        if (!enabled) return;
        languageRules[language].emplace_back(std::regex(pattern), tokenType);
    }

    std::vector<Token> highlightCode(const std::string& code, const std::string& language) {
        std::vector<Token> tokens;
        if (!enabled || languageRules.find(language) == languageRules.end()) {
            tokens.push_back({code, "text", 0});
            return tokens;
        }

        const auto& rules = languageRules[language];
        size_t pos = 0;
        std::string remaining = code;

        while (!remaining.empty()) {
            bool matched = false;
            size_t bestMatch = remaining.length();
            std::string bestType;
            std::string matchedText;

            for (const auto& rule : rules) {
                std::smatch match;
                if (std::regex_search(remaining, match, rule.first, std::regex_constants::match_continuous)) {
                    if (match.position(0) < bestMatch) {
                        bestMatch = match.position(0);
                        bestType = rule.second;
                        matchedText = match.str(0);
                        matched = true;
                    }
                }
            }

            if (matched) {
                tokens.push_back({matchedText, bestType, pos});
                pos += matchedText.length();
                remaining = remaining.substr(matchedText.length());
            } else {
                tokens.push_back({remaining.substr(0, 1), "text", pos});
                pos += 1;
                remaining = remaining.substr(1);
            }
        }

        return tokens;
    }

    void setEnabled(bool enable) {
        enabled = enable;
    }

    bool isEnabled() const {
        return enabled;
    }

    void clearRules() {
        languageRules.clear();
    }

    void loadDefaultRules() {
        // C++ syntax rules
        addLanguageRule("cpp", R"(\b(int|char|bool|float|double|void|auto|class|struct|enum|namespace|template|typename|const|static|virtual|override|public|private|protected)\b)", "keyword");
        addLanguageRule("cpp", R"(\b(if|else|while|for|do|switch|case|break|continue|return|try|catch|throw)\b)", "control");
        addLanguageRule("cpp", R"(\b(new|delete|nullptr|true|false)\b)", "literal");
        addLanguageRule("cpp", R"(//[^\n]*)", "comment");
        addLanguageRule("cpp", R"(/\*(?:[^*]|\*[^/])*\*/)", "comment");
        addLanguageRule("cpp", R"("(?:[^"\\]|\\.)*")", "string");
        addLanguageRule("cpp", R"('(?:[^'\\]|\\.)*')", "char");
        addLanguageRule("cpp", R"(\b\d+\b)", "number");
        addLanguageRule("cpp", R"(\b[A-Za-z_][A-Za-z0-9_]*::\b)", "scope");
        addLanguageRule("cpp", R"(\b[A-Z][A-Za-z0-9_]*\b)", "type");
    }
};

} // namespace bolt
