#ifndef BOLT_EDITOR_FIND_REPLACE_HPP
#define BOLT_EDITOR_FIND_REPLACE_HPP

#include <string>
#include <vector>
#include <regex>
#include <algorithm> //Added for std::transform


namespace bolt {

struct TextRange {
    size_t position;
    size_t length;
};

class FindReplace {
public:
    FindReplace() : case_sensitive_(false), use_regex_(false) {}

    std::vector<TextRange> find(const std::string& text, const std::string& search_term) {
        std::vector<TextRange> results;
        if (search_term.empty()) return results;

        if (use_regex_) {
            results = findWithRegex(text, search_term);
        } else {
            results = findWithString(text, search_term);
        }
        return results;
    }

    std::string replace(const std::string& text, const std::string& search_term, const std::string& replace_term) {
        std::string result = text;
        auto matches = find(text, search_term);
        // Replace from end to start to maintain indices
        for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
            result.replace(it->position, it->length, replace_term);
        }
        return result;
    }

    void setCaseSensitive(bool sensitive) { case_sensitive_ = sensitive; }
    void setUseRegex(bool use_regex) { use_regex_ = use_regex; }

    bool isCaseSensitive() const { return case_sensitive_; }
    bool isUsingRegex() const { return use_regex_; }

private:
    bool case_sensitive_;
    bool use_regex_;

    std::vector<TextRange> findWithString(const std::string& text, const std::string& term) {
        std::vector<TextRange> results;
        std::string searchText = text;
        std::string searchTerm = term;

        if (!case_sensitive_) {
            std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
            std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);
        }

        size_t pos = 0;
        while ((pos = searchText.find(searchTerm, pos)) != std::string::npos) {
            results.push_back({pos, term.length()});
            pos += 1;
        }

        return results;
    }

    std::vector<TextRange> findWithRegex(const std::string& text, const std::string& pattern) {
        std::vector<TextRange> results;
        try {
            std::regex::flag_type flags = std::regex::ECMAScript;
            if (!case_sensitive_) {
                flags |= std::regex::icase;
            }

            std::regex rx(pattern, flags);
            auto begin = std::sregex_iterator(text.begin(), text.end(), rx);
            auto end = std::sregex_iterator();

            for (std::sregex_iterator i = begin; i != end; ++i) {
                std::smatch match = *i;
                results.push_back({static_cast<size_t>(match.position()), static_cast<size_t>(match.length())});
            }
        } catch (const std::regex_error&) {
            // Handle invalid regex pattern (currently does nothing)
        }
        return results;
    }
};

} // namespace bolt

#endif // BOLT_EDITOR_FIND_REPLACE_HPP