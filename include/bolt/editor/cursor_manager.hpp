
#ifndef CURSOR_MANAGER_HPP
#define CURSOR_MANAGER_HPP

#include <vector>
#include <algorithm>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

struct Selection {
    size_t startLine;
    size_t startColumn;
    size_t endLine;
    size_t endColumn;
    
    bool hasSelection() const {
        return !(startLine == endLine && startColumn == endColumn);
    }
    
    bool operator==(const Selection& other) const {
        return startLine == other.startLine && startColumn == other.startColumn &&
               endLine == other.endLine && endColumn == other.endColumn;
    }
};

struct Cursor {
    size_t line;
    size_t column;
    bool isActive;
    Selection selection;  // Selection range for this cursor
    
    Cursor() : line(0), column(0), isActive(true), selection{0, 0, 0, 0} {}
    Cursor(size_t l, size_t c) : line(l), column(c), isActive(true), selection{l, c, l, c} {}
    
    bool operator==(const Cursor& other) const {
        return line == other.line && column == other.column;
    }
};

class CursorManager {
private:
    ThreadSafe<std::vector<Cursor>> cursors_;
    ThreadSafe<size_t> primaryCursorIndex_{0};

public:
    static CursorManager& getInstance();

    void addCursor(size_t line, size_t column);

    void addCursorWithSelection(size_t line, size_t column, const Selection& selection);

    void removeCursor(size_t line, size_t column);

    void clearCursors();

    std::vector<Cursor> getCursors() const;

    void setPrimaryCursor(size_t index);

    Cursor getPrimaryCursor() const;

    void moveCursors(int deltaLine, int deltaColumn);

    // Multi-cursor text editing operations
    void insertTextAtCursors(const std::string& text);
    void deleteAtCursors();
    void deleteSelectionAtCursors();
    std::vector<std::string> getSelectedText() const;
    
    // Selection operations
    void updateCursorSelection(size_t cursorIndex, const Selection& selection);
    void clearAllSelections();
    void selectWordAtCursors();
    void selectLineAtCursors();
    
    // Find and multi-cursor operations
    void findNextOccurrence(const std::string& text, const std::string& documentContent);
    void addCursorAtNextOccurrence(const std::string& text, const std::string& documentContent);
    void selectAllOccurrences(const std::string& text, const std::string& documentContent);
    
    // Utility methods
    size_t getCursorCount() const;
    bool hasCursors() const;
    void sortCursorsByPosition();
};

} // namespace bolt

#endif
