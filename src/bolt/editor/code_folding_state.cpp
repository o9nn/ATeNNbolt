#include "bolt/editor/code_folding.hpp"
#include <map>

namespace bolt {

void CodeFoldingState::setFoldState(size_t lineNumber, bool folded) {
    foldedLines_[lineNumber] = folded;
}

bool CodeFoldingState::isFolded(size_t lineNumber) const {
    auto it = foldedLines_.find(lineNumber);
    return it != foldedLines_.end() && it->second;
}

void CodeFoldingState::toggleFold(size_t lineNumber) {
    foldedLines_[lineNumber] = !isFolded(lineNumber);
}

void CodeFoldingState::unfoldAll() {
    for (auto& [line, folded] : foldedLines_) {
        folded = false;
    }
}

void CodeFoldingState::clear() {
    foldedLines_.clear();
}

} // namespace bolt

#include <fstream>
#include <json/json.hpp>

namespace bolt {

class CodeFoldingStateManager {
private:
    std::map<std::string, std::vector<FoldRange>> storedStates_;
    const std::string stateFile_ = "code_folding_state.json";

public:
    void saveState(const std::string& filePath, const std::vector<FoldRange>& ranges) {
        storedStates_[filePath] = ranges;
        persistToDisk();
    }

    std::vector<FoldRange> loadState(const std::string& filePath) {
        auto it = storedStates_.find(filePath);
        return it != storedStates_.end() ? it->second : std::vector<FoldRange>{};
    }

    void clearState(const std::string& filePath) {
        storedStates_.erase(filePath);
        persistToDisk();
    }

private:
    void persistToDisk() {
        nlohmann::json j;
        for (const auto& [path, ranges] : storedStates_) {
            nlohmann::json rangeArray = nlohmann::json::array();
            for (const auto& range : ranges) {
                rangeArray.push_back({
                    {"startLine", range.startLine},
                    {"endLine", range.endLine},
                    {"isFolded", range.isFolded}
                });
            }
            j[path] = rangeArray;
        }

        std::ofstream file(stateFile_);
        file << j.dump(2);
    }

    void loadFromDisk() {
        try {
            std::ifstream file(stateFile_);
            if (file.is_open()) {
                nlohmann::json j;
                file >> j;

                for (auto& [path, ranges] : j.items()) {
                    std::vector<FoldRange> foldRanges;
                    for (auto& range : ranges) {
                        foldRanges.push_back({
                            range["startLine"],
                            range["endLine"],
                            range["isFolded"]
                        });
                    }
                    storedStates_[path] = foldRanges;
                }
            }
        } catch (...) {
            // Handle file read errors gracefully
            storedStates_.clear();
        }
    }
};

} // namespace bolt