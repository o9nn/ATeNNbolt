
#ifndef EDITOR_STORE_HPP 
#define EDITOR_STORE_HPP

#include <string>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <vector>
#include <mutex>
#include "bolt/gui_components.hpp"
#include "bolt/editor/code_folding.hpp"
#include "bolt/core/error_handling.hpp"

namespace bolt {

struct EditorDocument {
    std::string value;
    std::string filePath;
    struct ScrollPosition {
        int line;
        int character;
        
        void validate() const {
            if (line < 0 || character < 0) {
                throw EditorException(ErrorCode::INVALID_PARAMETER,
                    "Scroll position cannot be negative: line=" + std::to_string(line) + 
                    ", character=" + std::to_string(character));
            }
        }
    } scroll;
    struct Cursor {
        size_t position;
        std::optional<BracketMatch> bracketMatch;
    } cursor;
    // Code folding state
    std::vector<FoldRange> foldingRanges;
    
    void validateDocument() const {
        if (filePath.empty()) {
            throw EditorException(ErrorCode::INVALID_PARAMETER, "Document file path cannot be empty");
        }
        if (filePath.length() > MAX_FILE_PATH_LENGTH) {
            throw EditorException(ErrorCode::INVALID_PARAMETER,
                "File path too long: " + std::to_string(filePath.length()) + 
                " > " + std::to_string(MAX_FILE_PATH_LENGTH));
        }
        if (value.length() > MAX_DOCUMENT_SIZE) {
            throw EditorException(ErrorCode::INVALID_PARAMETER,
                "Document too large: " + std::to_string(value.length()) + 
                " > " + std::to_string(MAX_DOCUMENT_SIZE));
        }
        scroll.validate();
    }
    
    static constexpr size_t MAX_FILE_PATH_LENGTH = 2048;
    static constexpr size_t MAX_DOCUMENT_SIZE = 50 * 1024 * 1024; // 50MB
};

class EditorStore {
public:
    static EditorStore& getInstance() {
        static EditorStore instance;
        return instance;
    }

    void setDocument(const std::string& path, const EditorDocument& doc) {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        doc.validateDocument();
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (documents_.size() >= MAX_OPEN_DOCUMENTS && documents_.find(path) == documents_.end()) {
            throw EditorException(ErrorCode::EDITOR_OPERATION_FAILED,
                "Too many open documents: " + std::to_string(documents_.size()) + 
                " >= " + std::to_string(MAX_OPEN_DOCUMENTS));
        }
        
        documents_[path] = doc;
        notifyListeners();
    }

    void updateScrollPosition(const std::string& path, int line, int character) {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = documents_.find(path);
        if (it == documents_.end()) {
            throw EditorException(ErrorCode::EDITOR_DOCUMENT_INVALID,
                "Document not found: " + path);
        }
        
        EditorDocument::ScrollPosition pos{line, character};
        pos.validate();
        
        it->second.scroll = pos;
        notifyListeners();
    }

    void updateFoldingRanges(const std::string& path, const std::vector<FoldRange>& ranges) {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = documents_.find(path);
        if (it == documents_.end()) {
            throw EditorException(ErrorCode::EDITOR_DOCUMENT_INVALID,
                "Document not found: " + path);
        }
        
        if (ranges.size() > MAX_FOLDING_RANGES) {
            throw EditorException(ErrorCode::EDITOR_FOLDING_ERROR,
                "Too many folding ranges: " + std::to_string(ranges.size()) + 
                " > " + std::to_string(MAX_FOLDING_RANGES));
        }
        
        it->second.foldingRanges = ranges;
        notifyListeners();
    }

    void toggleFold(const std::string& path, size_t line) {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = documents_.find(path);
        if (it == documents_.end()) {
            throw EditorException(ErrorCode::EDITOR_DOCUMENT_INVALID,
                "Document not found: " + path);
        }
        
        auto& ranges = it->second.foldingRanges;
        bool found = false;
        for (auto& range : ranges) {
            if (line >= range.startLine && line <= range.endLine) {
                range.isFolded = !range.isFolded;
                found = true;
                notifyListeners();
                break;
            }
        }
        
        if (!found) {
            throw EditorException(ErrorCode::EDITOR_FOLDING_ERROR,
                "No foldable range found at line " + std::to_string(line) + " in " + path);
        }
    }

    std::vector<FoldRange> getFoldingRanges(const std::string& path) const {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = documents_.find(path);
        if (it != documents_.end()) {
            return it->second.foldingRanges;
        }
        return {};
    }

    EditorDocument* getCurrentDocument() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!selectedFile_.empty() && documents_.count(selectedFile_)) {
            return &documents_[selectedFile_];
        }
        return nullptr;
    }

    void setSelectedFile(const std::string& path) {
        if (!path.empty()) {
            ErrorHandler::validateParameter(path.length() <= EditorDocument::MAX_FILE_PATH_LENGTH,
                "File path too long: " + std::to_string(path.length()));
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        selectedFile_ = path;
        notifyListeners();
    }

    const std::string& getSelectedFile() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return selectedFile_;
    }

    void addListener(std::function<void()> listener) {
        ErrorHandler::validateParameter(listener != nullptr, "Listener cannot be null");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (listeners_.size() >= MAX_LISTENERS) {
            throw EditorException(ErrorCode::STORE_STATE_INVALID,
                "Too many listeners: " + std::to_string(listeners_.size()) + 
                " >= " + std::to_string(MAX_LISTENERS));
        }
        
        listeners_.push_back(listener);
    }
    
    size_t getDocumentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return documents_.size();
    }
    
    bool hasDocument(const std::string& path) const {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        std::lock_guard<std::mutex> lock(mutex_);
        return documents_.find(path) != documents_.end();
    }
    
    void closeDocument(const std::string& path) {
        ErrorHandler::validateParameter(!path.empty(), "Document path cannot be empty");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = documents_.find(path);
        if (it == documents_.end()) {
            throw EditorException(ErrorCode::EDITOR_DOCUMENT_INVALID,
                "Cannot close document that is not open: " + path);
        }
        
        documents_.erase(it);
        
        // Clear selection if this was the selected file
        if (selectedFile_ == path) {
            selectedFile_.clear();
        }
        
        notifyListeners();
    }

private:
    EditorStore() {}

    void notifyListeners() {
        // Note: mutex should already be locked when this is called
        for (auto& listener : listeners_) {
            try {
                if (listener) {
                    listener();
                }
            } catch (const std::exception& e) {
                // Don't let listener failures break the store
                // In a real implementation, this would go to a logging system
            }
        }
    }

    std::string selectedFile_;
    std::map<std::string, EditorDocument> documents_;
    std::vector<std::function<void()>> listeners_;
    mutable std::mutex mutex_;
    
    static constexpr size_t MAX_OPEN_DOCUMENTS = 50;
    static constexpr size_t MAX_FOLDING_RANGES = 10000;
    static constexpr size_t MAX_LISTENERS = 100;
};

} // namespace bolt

#endif
