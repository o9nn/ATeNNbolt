#ifndef CODE_FOLDING_MANAGER_HPP
#define CODE_FOLDING_MANAGER_HPP

#include "code_folding.hpp"
#include "code_folding_detector.hpp"
#include "code_folding_visibility.hpp"
#include <map>
#include <string>
#include <vector>
#include <functional>

namespace bolt {

class CodeFoldingManager {
private:
    std::map<std::string, std::vector<FoldRange>> foldingRanges_;
    ThreadSafe<bool> foldingEnabled_{true};
    CodeFoldingVisibilityManager visibilityManager_;
    std::function<void()> onFoldingChanged_;

public:
    static CodeFoldingManager& getInstance() {
        static CodeFoldingManager instance;
        return instance;
    }

    void updateFoldingRanges(const std::string& filePath, const std::string& content);
    void toggleFold(const std::string& filePath, size_t line);
    std::vector<FoldRange> getFoldingRanges(const std::string& filePath) const;
    void setFoldingEnabled(bool enabled);
    bool isFoldingEnabled() const;
    void handleClick(const std::string& filePath, size_t lineNumber);
    void setOnFoldingChanged(std::function<void()> callback);

private:
    void refreshFoldingRanges();
};

} // namespace bolt

#endif