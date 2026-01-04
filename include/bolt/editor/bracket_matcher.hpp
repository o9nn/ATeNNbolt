
#ifndef BRACKET_MATCHER_HPP
#define BRACKET_MATCHER_HPP

#include <string>
#include <stack>
#include <optional>

namespace bolt {

struct BracketMatch {
    size_t openPos;
    size_t closePos;
};

class BracketMatcher {
public:
    static std::optional<BracketMatch> findMatchingBracket(const std::string& text, size_t cursorPos) {
        if (cursorPos >= text.length()) return std::nullopt;

        char current = text[cursorPos];
        if (isOpenBracket(current)) {
            return findClosingBracket(text, cursorPos);
        } else if (isCloseBracket(current)) {
            return findOpeningBracket(text, cursorPos);
        }
        
        return std::nullopt;
    }

private:
    static bool isOpenBracket(char c) {
        return c == '(' || c == '{' || c == '[';
    }

    static bool isCloseBracket(char c) {
        return c == ')' || c == '}' || c == ']';
    }

    static char getMatchingBracket(char c) {
        switch (c) {
            case '(': return ')';
            case '{': return '}';
            case '[': return ']';
            case ')': return '(';
            case '}': return '{';
            case ']': return '[';
            default: return '\0';
        }
    }

    static std::optional<BracketMatch> findClosingBracket(const std::string& text, size_t openPos) {
        char openBracket = text[openPos];
        char closeBracket = getMatchingBracket(openBracket);
        std::stack<char> brackets;
        
        for (size_t i = openPos; i < text.length(); ++i) {
            if (text[i] == openBracket) {
                brackets.push(openBracket);
            } else if (text[i] == closeBracket) {
                brackets.pop();
                if (brackets.empty()) {
                    return BracketMatch{openPos, i};
                }
            }
        }
        
        return std::nullopt;
    }

    static std::optional<BracketMatch> findOpeningBracket(const std::string& text, size_t closePos) {
        char closeBracket = text[closePos];
        char openBracket = getMatchingBracket(closeBracket);
        std::stack<char> brackets;
        
        for (size_t i = closePos; i-- > 0;) {
            if (text[i] == closeBracket) {
                brackets.push(closeBracket);
            } else if (text[i] == openBracket) {
                brackets.pop();
                if (brackets.empty()) {
                    return BracketMatch{i, closePos};
                }
            }
        }
        
        return std::nullopt;
    }
};

} // namespace bolt

#endif
