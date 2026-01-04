
#include "bolt/editor/find_replace.hpp"
#include <algorithm>
#include <regex>

namespace bolt {

FindReplace::FindReplace() : case_sensitive_(false), use_regex_(false) {}

std::vector<TextRange> FindReplace::find(const std::string& text, const std::string& search_term) {
    std::vector<TextRange> results;
    
    if (search_term.empty() || text.empty()) {
        return results;
    }

    if (use_regex_) {
        try {
            std::regex pattern(search_term, 
                case_sensitive_ ? std::regex::ECMAScript : 
                                std::regex::ECMAScript | std::regex::icase);
            
            auto words_begin = std::sregex_iterator(text.begin(), text.end(), pattern);
            auto words_end = std::sregex_iterator();

            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                results.push_back({
                    static_cast<size_t>(match.position()), 
                    static_cast<size_t>(match.length())
                });
            }
        } catch (const std::regex_error&) {
            // Invalid regex pattern, return empty results
            return results;
        }
    } else {
        size_t pos = 0;
        std::string text_copy = text;
        std::string term_copy = search_term;
        
        if (!case_sensitive_) {
            std::transform(text_copy.begin(), text_copy.end(), text_copy.begin(), ::tolower);
            std::transform(term_copy.begin(), term_copy.end(), term_copy.begin(), ::tolower);
        }

        while ((pos = text_copy.find(term_copy, pos)) != std::string::npos) {
            results.push_back({pos, search_term.length()});
            pos += term_copy.length();
        }
    }

    return results;
}

std::string FindReplace::replace(const std::string& text, 
                               const std::string& search_term,
                               const std::string& replace_term) {
    if (search_term.empty() || text.empty()) {
        return text;
    }

    std::string result = text;
    
    if (use_regex_) {
        try {
            std::regex pattern(search_term,
                case_sensitive_ ? std::regex::ECMAScript :
                                std::regex::ECMAScript | std::regex::icase);
            result = std::regex_replace(text, pattern, replace_term);
        } catch (const std::regex_error&) {
            return text;
        }
    } else {
        size_t pos = 0;
        while ((pos = result.find(search_term, pos)) != std::string::npos) {
            result.replace(pos, search_term.length(), replace_term);
            pos += replace_term.length();
        }
    }

    return result;
}

void FindReplace::setCaseSensitive(bool sensitive) {
    case_sensitive_ = sensitive;
}

void FindReplace::setUseRegex(bool use_regex) {
    use_regex_ = use_regex;
}

bool FindReplace::isCaseSensitive() const {
    return case_sensitive_;
}

bool FindReplace::isUsingRegex() const {
    return use_regex_;
}

} // namespace bolt
