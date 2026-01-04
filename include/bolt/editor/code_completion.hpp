
#ifndef CODE_COMPLETION_HPP
#define CODE_COMPLETION_HPP

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <iterator>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string kind;
    int score;
};

class CodeCompletionEngine {
private:
    ThreadSafe<std::vector<CompletionItem>> completionCache_;
    
public:
    static CodeCompletionEngine& getInstance() {
        static CodeCompletionEngine instance;
        return instance;
    }

    std::vector<CompletionItem> getCompletions(const std::string& prefix, size_t limit = 10) {
        return completionCache_.read([&](const auto& cache) {
            std::vector<CompletionItem> results;
            std::copy_if(cache.begin(), cache.end(), 
                std::back_inserter(results),
                [&](const auto& item) {
                    return item.label.find(prefix) == 0;
                });
            
            std::sort(results.begin(), results.end(),
                [](const auto& a, const auto& b) {
                    return a.score > b.score;
                });
                
            if (results.size() > limit) {
                results.resize(limit);
            }
            return results;
        });
    }

    void addCompletion(CompletionItem item) {
        completionCache_.write([&](auto& cache) {
            cache.push_back(std::move(item));
        });
    }
};

// Forward declaration for PIMPL
class CodeCompletionImpl;

class CodeCompletion {
private:
    std::unique_ptr<CodeCompletionImpl> impl_;
    
public:
    CodeCompletion();
    ~CodeCompletion();
    
    void activate();
    void deactivate();
    bool isActive() const;
    
    void setSuggestions(const std::vector<CompletionItem>& suggestions);
    CompletionItem getSelectedSuggestion() const;
    
    void selectNext();
    void selectPrevious();
};

} // namespace bolt

#endif
