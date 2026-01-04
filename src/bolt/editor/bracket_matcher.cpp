
#include "bolt/editor/bracket_matcher.hpp"
#include <stack>
#include <string>
#include <optional>

namespace bolt {

bool BracketMatcher::isOpenBracket(char c) {
    return c == '(' || c == '{' || c == '[';
}

bool BracketMatcher::isCloseBracket(char c) {
    return c == ')' || c == '}' || c == ']';
}

char BracketMatcher::getMatchingBracket(char c) {
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

std::optional<BracketMatch> BracketMatcher::findMatchingBracket(const std::string& text, size_t cursorPos) {
    if (cursorPos >= text.length()) return std::nullopt;

    char current = text[cursorPos];
    if (isOpenBracket(current)) {
        return findClosingBracket(text, cursorPos);
    } else if (isCloseBracket(current)) {
        return findOpeningBracket(text, cursorPos);
    }
    
    return std::nullopt;
}

std::optional<BracketMatch> BracketMatcher::findClosingBracket(const std::string& text, size_t openPos) {
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

std::optional<BracketMatch> BracketMatcher::findOpeningBracket(const std::string& text, size_t closePos) {
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

} // namespace bolt
