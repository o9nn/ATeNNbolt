#ifndef CODE_FOLDING_DETECTOR_HPP
#define CODE_FOLDING_DETECTOR_HPP

#include "code_folding.hpp"
#include <string>
#include <vector>

namespace bolt {

class CodeFoldingDetector {
public:
    static std::vector<FoldRange> detectFoldableRanges(const std::string& content);
    static bool isFoldableRegion(const std::string& line);
    static int findMatchingEnd(const std::vector<std::string>& lines, int startLine);
};

} // namespace bolt

#endif