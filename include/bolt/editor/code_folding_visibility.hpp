#ifndef CODE_FOLDING_VISIBILITY_HPP
#define CODE_FOLDING_VISIBILITY_HPP

#include "code_folding.hpp"
#include <map>
#include <string>
#include <vector>

namespace bolt {

class CodeFoldingVisibilityManager {
private:
    std::map<std::string, std::map<size_t, bool>> visibilityMap_;

public:
    void setLineVisibility(const std::string& filePath, size_t line, bool visible);
    bool isLineVisible(const std::string& filePath, size_t line) const;
    void toggleFoldedSection(const std::string& filePath, size_t startLine, size_t endLine);
    void showAllLines(const std::string& filePath);
    void updateFoldedSections(const std::string& filePath, const std::vector<FoldRange>& foldRanges);
};

} // namespace bolt

#endif