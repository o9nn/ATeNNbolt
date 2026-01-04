# Code Folding Integration Documentation

## Overview

The Bolt C++ IDE now includes complete code folding integration with the editor. This feature allows users to collapse and expand sections of code to improve readability and navigation of large files.

## Features

### Automatic Detection
- **Brace-based Folding**: Automatically detects foldable regions based on curly braces `{}`
- **Comment Folding**: Supports folding of multi-line comments `/* ... */`
- **Language-aware**: Recognizes common C++ constructs like classes, functions, if statements, loops

### Interactive Operations
- **Toggle Folding**: Click on folding indicators to expand/collapse individual regions
- **Expand All**: Expand all folded regions in the current document
- **Collapse All**: Collapse all foldable regions in the current document
- **Enable/Disable**: Global toggle for the entire folding system

### Integration Features
- **Document Persistence**: Folding state is maintained per document
- **Dynamic Updates**: Folding ranges are automatically re-detected when content changes
- **Thread-safe**: All folding operations are thread-safe for concurrent access
- **Performance Optimized**: Efficient detection and management of folding ranges

## Architecture

### Core Components

1. **FoldRange**: Data structure representing a foldable region
   ```cpp
   struct FoldRange {
       size_t startLine;
       size_t endLine;
       bool isFolded;
       std::string placeholder;
   };
   ```

2. **CodeFoldingDetector**: Analyzes code content and identifies foldable regions
   - Detects brace-based code blocks
   - Handles nested structures
   - Recognizes multi-line comments

3. **CodeFoldingManager**: Central manager for folding operations
   - Maintains folding state per file
   - Provides thread-safe access
   - Handles enable/disable functionality

4. **CodeFoldingVisibilityManager**: Manages line visibility based on folding state
   - Tracks which lines should be hidden/shown
   - Handles folded section visibility

5. **CodeFoldingUI**: User interface components for folding
   - Renders folding indicators
   - Handles user interactions
   - Customizable gutter icons

6. **IntegratedEditor**: High-level interface combining all editor features
   - Seamless integration with document management
   - Unified API for all editor operations
   - Automatic folding detection on document changes

### Integration with Editor Store

The code folding system is fully integrated with the existing EditorStore:

- **EditorDocument** now includes folding ranges as part of document state
- **Automatic synchronization** between folding manager and editor store
- **Event-driven updates** when folding state changes
- **Persistent state** maintained across document operations

## Usage Examples

### Basic Usage

```cpp
#include "bolt/editor/integrated_editor.hpp"

auto& editor = IntegratedEditor::getInstance();

// Open a document with automatic folding detection
editor.openDocument("example.cpp", codeContent);

// Get detected folding ranges
auto ranges = editor.getFoldingRanges("example.cpp");

// Toggle a fold at a specific line
editor.toggleFold("example.cpp", 10);

// Expand all folds
editor.expandAllFolds("example.cpp");

// Collapse all folds
editor.collapseAllFolds("example.cpp");
```

### Advanced Operations

```cpp
// Enable/disable folding globally
editor.setFoldingEnabled(false);
bool isEnabled = editor.isFoldingEnabled();

// Update content and refresh folding
editor.updateDocumentContent("example.cpp", newContent);
// Folding ranges are automatically re-detected

// Handle click events (for UI integration)
editor.handleClick("example.cpp", lineNumber, columnNumber);
```

### Custom Folding Detection

```cpp
#include "bolt/editor/code_folding_detector.hpp"

// Detect foldable ranges in code
auto ranges = CodeFoldingDetector::detectFoldableRanges(codeContent);

// Check if a specific line contains a foldable region
bool isFoldable = CodeFoldingDetector::isFoldableRegion("class MyClass {");
```

## Testing

The code folding system includes comprehensive testing:

- **Unit Tests**: Test individual components (detector, manager, UI)
- **Integration Tests**: Test full editor integration
- **Functional Tests**: Test user-facing operations
- **Performance Tests**: Ensure efficient operation on large files

### Running Tests

```bash
# Run all tests
cd build && ctest

# Run specific code folding tests
./test/bolt_unit_tests "CodeFolding"
./test/bolt_unit_tests "CodeFoldingDetector"
./test/bolt_unit_tests "CodeFoldingManager"
./test/bolt_unit_tests "IntegratedEditor"
```

### Demo

A complete demonstration is available:

```bash
# Compile and run the demo
g++ -I include -std=c++17 demo_code_folding.cpp -Lbuild -lbolt_lib -o demo_folding
./demo_folding
```

## Implementation Details

### Thread Safety

All folding operations use the `ThreadSafe<T>` wrapper for concurrent access:

```cpp
ThreadSafe<std::map<std::string, std::vector<FoldRange>>> foldingRanges_;
ThreadSafe<bool> foldingEnabled_{true};
```

### Performance Considerations

- **Lazy Detection**: Folding ranges are only computed when needed
- **Incremental Updates**: Only re-detect folding when content actually changes
- **Efficient Storage**: Minimal memory overhead per folding range
- **Fast Lookup**: O(1) access to folding state per file

### Extensibility

The system is designed for easy extension:

- **Custom Detectors**: Implement new folding detection algorithms
- **Language Support**: Add support for other programming languages
- **UI Customization**: Customize folding indicators and animations
- **Integration Points**: Easy integration with other editor features

## Configuration

### Default Settings

- **Folding Enabled**: true
- **Gutter Width**: 14 pixels
- **Collapsed Icon**: "▶"
- **Expanded Icon**: "▼"
- **Default Placeholder**: "{ ... }" for code blocks, "/* ... */" for comments

### Customization

UI elements can be customized through the CodeFoldingUI class:

```cpp
auto ui = std::make_unique<CodeFoldingUI>();
auto gutter = ui->createGutter();
gutter.iconCollapsed = "⮞";
gutter.iconExpanded = "⮟";
gutter.width = 16;
```

## Future Enhancements

Potential areas for future development:

1. **Custom Fold Regions**: User-defined folding regions with special comments
2. **Fold Persistence**: Save/restore folding state across sessions
3. **Animation**: Smooth expand/collapse animations
4. **Minimap Integration**: Show folded regions in editor minimap
5. **Language Server Protocol**: Integration with LSP for advanced folding
6. **Outline View**: Document outline based on folding structure

## Troubleshooting

### Common Issues

1. **Folding Not Working**: Check if folding is enabled with `isFoldingEnabled()`
2. **Incorrect Detection**: Verify the content has proper brace matching
3. **Performance**: Large files may need optimization for detection algorithms
4. **Thread Safety**: Ensure all access goes through the proper singleton instances

### Debug Information

Enable verbose logging to see folding operations:

```cpp
// Add debug output to see detected ranges
auto ranges = editor.getFoldingRanges("file.cpp");
for (const auto& range : ranges) {
    std::cout << "Fold: " << range.startLine << "-" << range.endLine 
              << " (" << range.placeholder << ")\n";
}
```