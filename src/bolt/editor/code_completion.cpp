
#include "bolt/editor/code_completion.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace bolt {

class CodeCompletionImpl {
public:
    CodeCompletionImpl() : active_(false) {}

    void activate() {
        active_ = true;
        suggestions_.clear();
    }

    void deactivate() {
        active_ = false;
        suggestions_.clear();
    }

    bool isActive() const { 
        return active_; 
    }

    void setSuggestions(const std::vector<CompletionItem>& suggestions) {
        suggestions_ = suggestions;
        currentIndex_ = 0;
    }

    CompletionItem getSelectedSuggestion() const {
        if (suggestions_.empty()) return CompletionItem();
        return suggestions_[currentIndex_];
    }

    void selectNext() {
        if (!suggestions_.empty()) {
            currentIndex_ = (currentIndex_ + 1) % suggestions_.size();
        }
    }

    void selectPrevious() {
        if (!suggestions_.empty()) {
            currentIndex_ = (currentIndex_ + suggestions_.size() - 1) % suggestions_.size();
        }
    }

private:
    bool active_;
    std::vector<CompletionItem> suggestions_;
    size_t currentIndex_{0};
};

CodeCompletion::CodeCompletion() : impl_(std::make_unique<CodeCompletionImpl>()) {}
CodeCompletion::~CodeCompletion() = default;

void CodeCompletion::activate() {
    impl_->activate();
}

void CodeCompletion::deactivate() {
    impl_->deactivate();
}

bool CodeCompletion::isActive() const {
    return impl_->isActive();
}

void CodeCompletion::setSuggestions(const std::vector<CompletionItem>& suggestions) {
    impl_->setSuggestions(suggestions);
}

CompletionItem CodeCompletion::getSelectedSuggestion() const {
    return impl_->getSelectedSuggestion();
}

void CodeCompletion::selectNext() {
    impl_->selectNext();
}

void CodeCompletion::selectPrevious() {
    impl_->selectPrevious();
}

} // namespace bolt
