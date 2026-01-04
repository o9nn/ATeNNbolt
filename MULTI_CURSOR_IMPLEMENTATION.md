# Multi-Cursor Editing Implementation

## Overview

The Bolt C++ IDE includes comprehensive multi-cursor editing support, allowing users to edit multiple locations in their code simultaneously. This feature dramatically improves productivity when making repetitive edits across a document.

## Core Components

### CursorManager Class

The `CursorManager` is the central component managing all multi-cursor operations:

**Location:** `include/bolt/editor/cursor_manager.hpp` and `src/bolt/editor/cursor_manager.cpp`

**Key Features:**
- Thread-safe cursor management
- Support for unlimited cursors
- Selection management per cursor
- Primary cursor designation
- Text editing operations across all cursors

### Cursor Structure

```cpp
struct Cursor {
    size_t line;           // Line position (0-based)
    size_t column;         // Column position (0-based)
    bool isActive;         // Whether cursor is active
    Selection selection;   // Selection range for this cursor
};
```

### Selection Structure

```cpp
struct Selection {
    size_t startLine, startColumn;   // Selection start
    size_t endLine, endColumn;       // Selection end
    bool hasSelection();             // Check if selection exists
};
```

## Key Operations

### Basic Cursor Management

```cpp
CursorManager& manager = CursorManager::getInstance();

// Add cursors at specific positions
manager.addCursor(line, column);
manager.addCursorWithSelection(line, column, selection);

// Remove cursors
manager.removeCursor(line, column);
manager.clearCursors();

// Query cursors
auto cursors = manager.getCursors();
size_t count = manager.getCursorCount();
bool hasCursors = manager.hasCursors();
```

### Text Editing Operations

```cpp
// Insert text at all cursor positions
manager.insertTextAtCursors("Hello World");

// Delete at all cursor positions
manager.deleteAtCursors();

// Delete selections at all cursors
manager.deleteSelectionAtCursors();

// Get selected text from all cursors
auto selections = manager.getSelectedText();
```

### Movement Operations

```cpp
// Move all cursors by delta amounts
manager.moveCursors(deltaLine, deltaColumn);

// Primary cursor management
manager.setPrimaryCursor(index);
auto primary = manager.getPrimaryCursor();
```

### Find and Replace Operations

```cpp
// Find next occurrence and add cursor
manager.findNextOccurrence("searchText", documentContent);

// Add cursor at next occurrence of selection
manager.addCursorAtNextOccurrence("searchText", documentContent);

// Select all occurrences of text
manager.selectAllOccurrences("searchText", documentContent);
```

### Selection Operations

```cpp
// Update selection for specific cursor
manager.updateCursorSelection(cursorIndex, selection);

// Clear all selections
manager.clearAllSelections();

// Select word/line at all cursors
manager.selectWordAtCursors();
manager.selectLineAtCursors();
```

## Integration with IntegratedEditor

The `IntegratedEditor` class provides high-level multi-cursor operations:

```cpp
IntegratedEditor& editor = IntegratedEditor::getInstance();

// Add cursors through editor
editor.addCursorAtPosition(line, column);
editor.addCursorAtNextOccurrence("searchText");
editor.selectAllOccurrences("searchText");

// Clear extra cursors (keep only primary)
editor.clearExtraCursors();

// Text operations
editor.insertTextAtCursors("text");
editor.deleteAtCursors();

// Query operations  
size_t count = editor.getCursorCount();
auto allCursors = editor.getAllCursors();
```

## Keyboard Shortcuts

The system includes pre-configured keyboard shortcuts for common multi-cursor operations:

- **Ctrl+D**: Add cursor at next occurrence of selected text
- **Ctrl+Shift+L**: Select all occurrences of selected text  
- **Ctrl+Alt+Up**: Add cursor above current line
- **Ctrl+Alt+Down**: Add cursor below current line
- **Escape**: Clear all extra cursors (keep only primary)

### Custom Shortcuts

New shortcuts can be registered through the keyboard system:

```cpp
keyboardShortcuts_.registerShortcut("Ctrl+Shift+D", "duplicateLine", [this]() {
    // Custom multi-cursor operation
}, ShortcutContext::Editor, "Duplicate line at all cursors");
```

## Thread Safety

All multi-cursor operations are thread-safe using the `ThreadSafe<>` wrapper:

```cpp
ThreadSafe<std::vector<Cursor>> cursors_;
ThreadSafe<size_t> primaryCursorIndex_;
```

This ensures safe concurrent access from UI threads, background operations, and plugin systems.

## Testing

Comprehensive test coverage is provided in `test/test_multi_cursor.cpp`:

- **BasicOperations**: Adding, removing, querying cursors
- **SelectionOperations**: Managing selections per cursor
- **MovementOperations**: Moving all cursors simultaneously  
- **PrimaryCursorOperations**: Primary cursor management
- **TextOperations**: Text insertion/deletion at multiple positions
- **RemovalOperations**: Removing specific cursors
- **IntegratedEditorIntegration**: High-level editor operations
- **FindOperations**: Find and replace with multi-cursor

Run tests with: `./build/test/bolt_multi_cursor_tests`

## Demo Application  

A comprehensive demo is available in `demo_multi_cursor.cpp` showing all features:

Build and run: `./build/demo_multi_cursor`

The demo demonstrates:
- Basic multi-cursor operations
- Text editing at multiple positions
- Selection management
- Cursor movement
- Integration with the editor
- Find and replace operations

## Usage Examples

### Example 1: Rename Variables

```cpp
// Select all occurrences of a variable name
manager.selectAllOccurrences("oldName", documentContent);

// Type new name - it will replace at all positions simultaneously
manager.insertTextAtCursors("newName");
```

### Example 2: Add Similar Lines

```cpp
// Add cursors at ends of multiple lines
manager.addCursor(5, 20);   // End of line 5
manager.addCursor(8, 15);   // End of line 8  
manager.addCursor(12, 30);  // End of line 12

// Add semicolon to all lines
manager.insertTextAtCursors(";");
```

### Example 3: Column Editing

```cpp
// Add cursors in a column
for (size_t line = 10; line < 20; ++line) {
    manager.addCursor(line, 25);  // Column 25 on lines 10-19
}

// Insert text at all positions
manager.insertTextAtCursors("// TODO: ");
```

## Integration Points

The multi-cursor system integrates with:

- **Code Completion**: Completions can be applied at all cursor positions
- **Syntax Highlighting**: Highlights update for all edited regions
- **Code Folding**: Folding operations consider all cursor positions
- **Find/Replace**: Enhanced to work with multiple selections
- **Split View**: Each pane maintains its own cursor state
- **Theme System**: Cursor rendering adapts to active theme

## Performance Considerations

- Cursors are stored in sorted order for efficient operations
- Text operations are batched to minimize document updates
- Thread-safe operations use efficient locking strategies
- Memory usage scales linearly with cursor count
- Optimized algorithms for large numbers of cursors

## Future Enhancements

Potential improvements for future versions:

- **Visual cursor feedback**: Enhanced rendering of multiple cursors
- **Cursor history**: Undo/redo support for cursor operations
- **Smart cursor placement**: AI-assisted cursor positioning
- **Cursor templates**: Saved cursor configurations
- **Plugin API**: Extensible multi-cursor operations

## Conclusion

The multi-cursor editing implementation in Bolt C++ provides a robust, thread-safe, and feature-complete system for advanced code editing. It supports all common multi-cursor operations found in modern editors while maintaining the performance and reliability expected in a professional IDE.