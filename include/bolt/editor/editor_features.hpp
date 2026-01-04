#ifndef EDITOR_FEATURES_HPP
#define EDITOR_FEATURES_HPP

#include <string>
#include <vector>
#include <map>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

struct FoldingRange {
    int startLine;
    int endLine;
    bool isFolded;
};

class EditorFeatures {
private:
    ThreadSafe<std::map<std::string, std::vector<FoldingRange>>> foldingRanges_;
    ThreadSafe<bool> showFoldingMarkers_{true};
    ThreadSafe<bool> showLineNumbers_{true};

public:
    static EditorFeatures& getInstance() {
        static EditorFeatures instance;
        return instance;
    }

    void toggleLineNumbers() {
        showLineNumbers_.write([](bool& show) {
            show = !show;
            return show;
        });
    }

    bool areLineNumbersVisible() const {
        return showLineNumbers_.read([](const bool& show) { return show; });
    }

    void addFoldingRange(const std::string& filePath, int startLine, int endLine) {
        foldingRanges_.write([&](auto& ranges) {
            ranges[filePath].push_back({startLine, endLine, false});
        });
    }

    void toggleFold(const std::string& filePath, int line) {
        foldingRanges_.write([&](auto& ranges) {
            auto& fileRanges = ranges[filePath];
            for (auto& range : fileRanges) {
                if (line >= range.startLine && line <= range.endLine) {
                    range.isFolded = !range.isFolded;
                    break;
                }
            }
        });
    }

    std::vector<FoldingRange> getFoldingRanges(const std::string& filePath) const {
        return foldingRanges_.read([&](const auto& ranges) {
            auto it = ranges.find(filePath);
            return it != ranges.end() ? it->second : std::vector<FoldingRange>{};
        });
    }

    void detectFoldingRanges(const std::string& filePath, const std::string& content) {
        std::vector<FoldingRange> ranges;
        std::vector<int> bracketStack;
        int lineNumber = 0;

        for (size_t i = 0; i < content.length(); ++i) {
            if (content[i] == '\n') {
                lineNumber++;
            }
            else if (content[i] == '{') {
                bracketStack.push_back(lineNumber);
            }
            else if (content[i] == '}' && !bracketStack.empty()) {
                int startLine = bracketStack.back();
                bracketStack.pop_back();
                if (lineNumber > startLine) {
                    ranges.push_back({startLine, lineNumber, false});
                }
            }
        }

        foldingRanges_.write([&](auto& allRanges) {
            allRanges[filePath] = ranges;
        });
    }
};

} // namespace bolt

#endif