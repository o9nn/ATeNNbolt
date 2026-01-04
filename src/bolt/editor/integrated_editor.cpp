#include "bolt/editor/integrated_editor.hpp"
#include "bolt/editor/cursor_manager.hpp"
#include "bolt/editor/keyboard_shortcuts.hpp"
#include "bolt/editor/split_view_manager.hpp"
#include "bolt/ai/ai_completion_provider.hpp"
#include <fstream>

namespace bolt {

IntegratedEditor::IntegratedEditor() 
    : editorStore_(EditorStore::getInstance())
    , foldingManager_(CodeFoldingManager::getInstance())
    , foldingUI_(std::make_unique<CodeFoldingUI>())
    , fileTreeManager_(FileTreeManager::getInstance())
    , fileTreeUI_(std::make_unique<FileTreeUI>())
    , cursorManager_(CursorManager::getInstance())
    , keyboardShortcuts_(KeyboardShortcuts::getInstance())
    , splitViewManager_(SplitViewManager::getInstance())
    , tabBar_(TabBar::getInstance())
    , aiCompletionEngine_(AICodeCompletionEngine::getInstance())
    , codeCompletion_(std::make_unique<CodeCompletion>())
    , debugger_(std::make_shared<DebuggerInterface>())
    , debuggerUI_(std::make_unique<DebuggerUI>()) {
    
    // Set up folding change callback to synchronize with editor store
    foldingManager_.setOnFoldingChanged([this]() {
        auto currentFile = editorStore_.getSelectedFile();
        if (!currentFile.empty()) {
            synchronizeFoldingState(currentFile);
        }
    });
    
    // Set up file tree callbacks for editor integration
    fileTreeManager_.setOnFileSelected([this](const std::string& path) {
        openFileFromTree(path);
    });
    
    // Initialize keyboard shortcuts with multi-cursor support
    initializeKeyboardShortcuts();
    
    // Set up split view callbacks
    splitViewManager_.setOnPaneOperation([this](SplitViewManager::PaneOperation op, const std::string& paneId) {
        // Handle pane operations (focus changes, etc.)
        if (op == SplitViewManager::PaneOperation::Focus) {
            // Sync editor store with focused pane
            EditorPane* pane = splitViewManager_.getPane(paneId);
            if (pane && pane->hasDocument()) {
                editorStore_.setSelectedFile(pane->getDocumentPath());
            }
        }
    });
    
    // Set up debugger integration
    debugger_->set_editor(this);
    debuggerUI_->set_debugger(debugger_);
    debuggerUI_->initialize();
}

void IntegratedEditor::openDocument(const std::string& filePath, const std::string& content) {
    EditorDocument doc;
    doc.value = content;
    doc.filePath = filePath;
    doc.scroll = {0, 0};
    doc.cursor = {0, std::nullopt};
    
    // Detect and set up folding ranges
    detectAndUpdateFolding(filePath, content);
    doc.foldingRanges = getFoldingRanges(filePath);
    
    editorStore_.setDocument(filePath, doc);
    editorStore_.setSelectedFile(filePath);
    
    // Open in active pane if split view is enabled
    if (splitViewManager_.isEnabled()) {
        EditorPane* activePane = splitViewManager_.getActivePane();
        if (activePane) {
            activePane->openDocument(filePath);
        }
    }
}

void IntegratedEditor::updateDocumentContent(const std::string& filePath, const std::string& content) {
    auto* doc = editorStore_.getCurrentDocument();
    if (doc && doc->filePath == filePath) {
        doc->value = content;
    }
    
    // Re-detect folding ranges when content changes
    detectAndUpdateFolding(filePath, content);
    synchronizeFoldingState(filePath);
}

void IntegratedEditor::closeDocument(const std::string& filePath) {
    // Clean up folding state
    foldingManager_.updateFoldingRanges(filePath, "");
    
    // In a real implementation, we would remove the document from the store
    // For now, we'll just update the folding ranges
}

void IntegratedEditor::toggleFold(const std::string& filePath, size_t line) {
    // Toggle in both the folding manager and editor store
    foldingManager_.toggleFold(filePath, line);
    editorStore_.toggleFold(filePath, line);
}

void IntegratedEditor::expandAllFolds(const std::string& filePath) {
    auto ranges = foldingManager_.getFoldingRanges(filePath);
    for (auto& range : ranges) {
        if (range.isFolded) {
            toggleFold(filePath, range.startLine);
        }
    }
}

void IntegratedEditor::collapseAllFolds(const std::string& filePath) {
    auto ranges = foldingManager_.getFoldingRanges(filePath);
    for (auto& range : ranges) {
        if (!range.isFolded) {
            toggleFold(filePath, range.startLine);
        }
    }
}

void IntegratedEditor::refreshFolding(const std::string& filePath) {
    auto* doc = editorStore_.getCurrentDocument();
    if (doc && doc->filePath == filePath) {
        detectAndUpdateFolding(filePath, doc->value);
        synchronizeFoldingState(filePath);
    }
}

void IntegratedEditor::renderEditor(const std::string& filePath) {
    if (foldingUI_->isVisible()) {
        foldingUI_->renderFoldingMarkers(filePath);
    }
    
    if (fileTreeUI_->isVisible()) {
        renderFileTree();
    }
}

void IntegratedEditor::handleClick(const std::string& filePath, size_t line, size_t character) {
    // Check if click is in the folding gutter area
    // For now, we'll assume any click on a foldable line can toggle folding
    auto ranges = getFoldingRanges(filePath);
    for (const auto& range : ranges) {
        if (line == range.startLine) {
            toggleFold(filePath, line);
            return;
        }
    }
    
    // Handle regular editor click...
}

std::vector<FoldRange> IntegratedEditor::getFoldingRanges(const std::string& filePath) const {
    return foldingManager_.getFoldingRanges(filePath);
}

bool IntegratedEditor::isFoldingEnabled() const {
    return foldingManager_.isFoldingEnabled();
}

void IntegratedEditor::setFoldingEnabled(bool enabled) {
    foldingManager_.setFoldingEnabled(enabled);
    foldingUI_->setVisibility(enabled);
}

void IntegratedEditor::detectAndUpdateFolding(const std::string& filePath, const std::string& content) {
    // Use the folding manager to update ranges
    foldingManager_.updateFoldingRanges(filePath, content);
}

void IntegratedEditor::synchronizeFoldingState(const std::string& filePath) {
    // Synchronize folding state between manager and editor store
    auto ranges = foldingManager_.getFoldingRanges(filePath);
    editorStore_.updateFoldingRanges(filePath, ranges);
}

// File tree operations
void IntegratedEditor::setFileTreeRootDirectory(const std::string& path) {
    fileTreeManager_.setRootDirectory(path);
}

void IntegratedEditor::refreshFileTree() {
    fileTreeManager_.refreshTree();
}

void IntegratedEditor::toggleFileTreeVisibility() {
    bool currentVisibility = fileTreeUI_->isVisible();
    fileTreeUI_->setVisible(!currentVisibility);
}

bool IntegratedEditor::isFileTreeVisible() const {
    return fileTreeUI_->isVisible();
}

void IntegratedEditor::renderFileTree() {
    if (fileTreeUI_->isVisible()) {
        fileTreeUI_->render();
    }
}

void IntegratedEditor::openFileFromTree(const std::string& filePath) {
    try {
        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return; // File couldn't be opened
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Open the document in the editor
        openDocument(filePath, content);
        
    } catch (const std::exception&) {
        // File reading failed, ignore silently for now
    }
}

void IntegratedEditor::showFileInTree(const std::string& filePath) {
    // Select the file in the tree without opening it
    fileTreeManager_.selectFile(filePath);
}

// Multi-cursor operations
void IntegratedEditor::addCursorAtPosition(size_t line, size_t column) {
    cursorManager_.addCursor(line, column);
}

void IntegratedEditor::addCursorAtNextOccurrence(const std::string& text) {
    auto* doc = editorStore_.getCurrentDocument();
    if (doc) {
        cursorManager_.addCursorAtNextOccurrence(text, doc->value);
    }
}

void IntegratedEditor::selectAllOccurrences(const std::string& text) {
    auto* doc = editorStore_.getCurrentDocument();
    if (doc) {
        cursorManager_.selectAllOccurrences(text, doc->value);
    }
}

void IntegratedEditor::clearExtraCursors() {
    // Keep only the primary cursor
    auto primaryCursor = cursorManager_.getPrimaryCursor();
    cursorManager_.clearCursors();
    cursorManager_.addCursor(primaryCursor.line, primaryCursor.column);
}

void IntegratedEditor::insertTextAtCursors(const std::string& text) {
    cursorManager_.insertTextAtCursors(text);
    // In a real implementation, would update document content
}

void IntegratedEditor::deleteAtCursors() {
    cursorManager_.deleteAtCursors();
    // In a real implementation, would update document content
}

size_t IntegratedEditor::getCursorCount() const {
    return cursorManager_.getCursorCount();
}

std::vector<Cursor> IntegratedEditor::getAllCursors() const {
    return cursorManager_.getCursors();
}

// Keyboard shortcut handling
bool IntegratedEditor::handleKeyboardShortcut(const std::string& key, const std::string& command) {
    // Execute shortcut in the current context (Editor context for IntegratedEditor)
    return keyboardShortcuts_.executeShortcut(key, ShortcutContext::Editor);
}

void IntegratedEditor::initializeKeyboardShortcuts() {
    // Set editor as active context
    keyboardShortcuts_.setActiveContext(ShortcutContext::Editor);
    
    // Register multi-cursor specific shortcuts with proper context
    keyboardShortcuts_.registerShortcut("Ctrl+D", "addCursorAtNextOccurrence", [this]() {
        // Get selected text and add cursor at next occurrence
        auto cursors = getAllCursors();
        if (!cursors.empty()) {
            // Simplified: add cursor at next line
            auto& lastCursor = cursors.back();
            addCursorAtPosition(lastCursor.line + 1, lastCursor.column);
        }
    }, ShortcutContext::Editor, "Add cursor at next occurrence of selected text");
    
    keyboardShortcuts_.registerShortcut("Ctrl+Shift+L", "selectAllOccurrences", [this]() {
        // Get selected text and select all occurrences
        selectAllOccurrences(""); // Simplified implementation
    }, ShortcutContext::Editor, "Select all occurrences of selected text");
    
    keyboardShortcuts_.registerShortcut("Ctrl+Alt+Up", "addCursorAbove", [this]() {
        auto cursors = getAllCursors();
        if (!cursors.empty()) {
            auto& primary = cursors[0];
            if (primary.line > 0) {
                addCursorAtPosition(primary.line - 1, primary.column);
            }
        }
    }, ShortcutContext::Editor, "Add cursor above current line");
    
    keyboardShortcuts_.registerShortcut("Ctrl+Alt+Down", "addCursorBelow", [this]() {
        auto cursors = getAllCursors();
        if (!cursors.empty()) {
            auto& primary = cursors[0];
            addCursorAtPosition(primary.line + 1, primary.column);
        }
    }, ShortcutContext::Editor, "Add cursor below current line");
    
    keyboardShortcuts_.registerShortcut("Escape", "clearExtraCursors", [this]() {
        clearExtraCursors();
    }, ShortcutContext::Editor, "Clear all extra cursors");
    
    // Initialize default shortcuts for all contexts
    keyboardShortcuts_.initDefaultShortcuts();
    
    // Add split view shortcuts with global context
    keyboardShortcuts_.registerShortcut("Ctrl+\\", "createHorizontalSplit", [this]() {
        createHorizontalSplit();
    }, ShortcutContext::Global, "Create horizontal split");
    
    keyboardShortcuts_.registerShortcut("Ctrl+Shift+\\", "createVerticalSplit", [this]() {
        createVerticalSplit();
    }, ShortcutContext::Global, "Create vertical split");
    
    keyboardShortcuts_.registerShortcut("Ctrl+W", "closePane", [this]() {
        std::string activeId = splitViewManager_.getActivePaneId();
        closePane(activeId);
    }, ShortcutContext::Global, "Close current pane");
    
    keyboardShortcuts_.registerShortcut("Ctrl+Tab", "navigateToNextPane", [this]() {
        navigateToNextPane();
    }, ShortcutContext::Global, "Navigate to next pane");
    
    keyboardShortcuts_.registerShortcut("Ctrl+Shift+Tab", "navigateToPreviousPane", [this]() {
        navigateToPreviousPane();
    }, ShortcutContext::Global, "Navigate to previous pane");
    
    // AI Code Completion shortcuts
    keyboardShortcuts_.registerShortcut("Ctrl+Space", "triggerCodeCompletion", [this]() {
        auto currentFile = editorStore_.getSelectedFile();
        if (!currentFile.empty()) {
            auto* doc = editorStore_.getCurrentDocument();
            if (doc) {
                // Convert cursor position to line/column (simplified implementation)
                size_t line = 0, column = 0;
                for (size_t i = 0; i < doc->cursor.position && i < doc->value.length(); ++i) {
                    if (doc->value[i] == '\n') {
                        line++;
                        column = 0;
                    } else {
                        column++;
                    }
                }
                triggerCodeCompletion(currentFile, line, column);
            }
        }
    }, ShortcutContext::Editor, "Trigger AI code completion");
    
    keyboardShortcuts_.registerShortcut("Enter", "acceptCompletion", [this]() {
        if (isCodeCompletionActive()) {
            acceptCompletion();
        }
    }, ShortcutContext::Editor, "Accept code completion");
    
    keyboardShortcuts_.registerShortcut("Escape", "cancelCompletion", [this]() {
        if (isCodeCompletionActive()) {
            cancelCompletion();
        }
    }, ShortcutContext::Editor, "Cancel code completion");
    
    keyboardShortcuts_.registerShortcut("Down", "selectNextCompletion", [this]() {
        if (isCodeCompletionActive()) {
            selectNextCompletion();
        }
    }, ShortcutContext::Editor, "Select next completion");
    
    keyboardShortcuts_.registerShortcut("Up", "selectPreviousCompletion", [this]() {
        if (isCodeCompletionActive()) {
            selectPreviousCompletion();
        }
    }, ShortcutContext::Editor, "Select previous completion");
    
    // Tab navigation shortcuts
    keyboardShortcuts_.registerShortcut("Ctrl+PageDown", "nextTab", [this]() {
        switchToNextTab();
    }, ShortcutContext::Global, "Switch to next tab");
    
    keyboardShortcuts_.registerShortcut("Ctrl+PageUp", "previousTab", [this]() {
        switchToPreviousTab();
    }, ShortcutContext::Global, "Switch to previous tab");
    
    keyboardShortcuts_.registerShortcut("Ctrl+W", "closeCurrentTab", [this]() {
        auto activeTab = getActiveTab();
        if (activeTab.has_value()) {
            closeTabById(activeTab->id);
        }
    }, ShortcutContext::Global, "Close current tab");
}

// Split view operations
std::string IntegratedEditor::createHorizontalSplit(const std::string& filePath) {
    std::string newPaneId = splitViewManager_.createHorizontalSplit();
    
    if (!newPaneId.empty() && !filePath.empty()) {
        openDocumentInPane(newPaneId, filePath);
    }
    
    return newPaneId;
}

std::string IntegratedEditor::createVerticalSplit(const std::string& filePath) {
    std::string newPaneId = splitViewManager_.createVerticalSplit();
    
    if (!newPaneId.empty() && !filePath.empty()) {
        openDocumentInPane(newPaneId, filePath);
    }
    
    return newPaneId;
}

bool IntegratedEditor::closePane(const std::string& paneId) {
    return splitViewManager_.closePane(paneId);
}

bool IntegratedEditor::focusPane(const std::string& paneId) {
    return splitViewManager_.focusPane(paneId);
}

void IntegratedEditor::navigateToNextPane() {
    splitViewManager_.navigateToNextPane();
}

void IntegratedEditor::navigateToPreviousPane() {
    splitViewManager_.navigateToPreviousPane();
}

// Split view management
bool IntegratedEditor::isSplitViewEnabled() const {
    return splitViewManager_.isEnabled();
}

void IntegratedEditor::setSplitViewEnabled(bool enabled) {
    splitViewManager_.setEnabled(enabled);
}

bool IntegratedEditor::hasSplits() const {
    return splitViewManager_.hasSplits();
}

void IntegratedEditor::collapseAllSplits() {
    splitViewManager_.collapseAllSplits();
}

std::vector<std::string> IntegratedEditor::getAllPaneIds() const {
    return splitViewManager_.getAllPaneIds();
}

std::string IntegratedEditor::getActivePaneId() const {
    return splitViewManager_.getActivePaneId();
}

EditorPane* IntegratedEditor::getActivePane() {
    return splitViewManager_.getActivePane();
}

// Document operations in split view
void IntegratedEditor::openDocumentInPane(const std::string& paneId, const std::string& filePath) {
    splitViewManager_.openDocumentInPane(paneId, filePath);
}

void IntegratedEditor::openDocumentInNewPane(const std::string& filePath, SplitViewManager::SplitDirection direction) {
    splitViewManager_.openDocumentInNewPane(filePath, direction);
}

std::vector<std::string> IntegratedEditor::getOpenDocuments() const {
    return splitViewManager_.getOpenDocuments();
}

std::string IntegratedEditor::findPaneWithDocument(const std::string& filePath) const {
    return splitViewManager_.findPaneWithDocument(filePath);
}

// AI Code Completion implementations
void IntegratedEditor::triggerCodeCompletion(const std::string& filePath, size_t line, size_t column) {
    auto* doc = editorStore_.getCurrentDocument();
    if (!doc || doc->filePath != filePath) {
        return; // Document not found or not active
    }
    
    // Calculate cursor position in the document
    size_t cursorPosition = 0;
    std::string content = doc->value;
    size_t currentLine = 0;
    
    for (size_t i = 0; i < content.length() && currentLine < line; ++i) {
        if (content[i] == '\n') {
            currentLine++;
            if (currentLine == line) {
                cursorPosition = i + 1 + column;
                break;
            }
        }
    }
    
    // Extract prefix (word before cursor)
    std::string prefix;
    if (cursorPosition > 0) {
        size_t start = cursorPosition;
        while (start > 0 && (std::isalnum(content[start - 1]) || content[start - 1] == '_' || content[start - 1] == ':')) {
            start--;
        }
        prefix = content.substr(start, cursorPosition - start);
    }
    
    triggerCodeCompletion(filePath, prefix);
}

void IntegratedEditor::triggerCodeCompletion(const std::string& filePath, const std::string& prefix) {
    auto* doc = editorStore_.getCurrentDocument();
    if (!doc || doc->filePath != filePath) {
        return;
    }
    
    // Get AI completions
    auto completions = getAICompletions(filePath, prefix);
    
    if (!completions.empty()) {
        codeCompletion_->setSuggestions(completions);
        codeCompletion_->activate();
    }
}

std::vector<CompletionItem> IntegratedEditor::getAICompletions(const std::string& filePath, const std::string& prefix, size_t limit) {
    auto* doc = editorStore_.getCurrentDocument();
    if (!doc || doc->filePath != filePath) {
        return {};
    }
    
    // Determine file language from extension
    std::string language = "text";
    if (filePath.length() >= 4) {
        std::string ext = filePath.substr(filePath.length() - 4);
        if (ext == ".cpp" || ext == ".hpp") {
            language = "cpp";
        }
    }
    if (filePath.length() >= 2 && filePath.substr(filePath.length() - 2) == ".h") {
        language = "cpp";
    }
    if (filePath.length() >= 3 && filePath.substr(filePath.length() - 3) == ".py") {
        language = "python";
    }
    if (filePath.length() >= 3) {
        std::string ext = filePath.substr(filePath.length() - 3);
        if (ext == ".js" || ext == ".ts") {
            language = "javascript";
        }
    }
    
    // Convert cursor position to line/column
    size_t line = 0, column = 0;
    for (size_t i = 0; i < doc->cursor.position && i < doc->value.length(); ++i) {
        if (doc->value[i] == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
    }
    
    // Create context for AI completion
    CodeContext context;
    context.filePath = filePath;
    context.content = doc->value;
    context.cursorPosition = doc->cursor.position;
    context.line = line;
    context.column = column;
    context.language = language;
    
    return aiCompletionEngine_.getCompletions(context, prefix, limit);
}

bool IntegratedEditor::isCodeCompletionActive() const {
    return codeCompletion_->isActive();
}

void IntegratedEditor::acceptCompletion() {
    if (codeCompletion_->isActive()) {
        auto selectedCompletion = codeCompletion_->getSelectedSuggestion();
        
        // Get current document
        auto* doc = editorStore_.getCurrentDocument();
        if (!doc || selectedCompletion.label.empty()) {
            codeCompletion_->deactivate();
            return;
        }
        
        // Calculate the prefix to remove (word before cursor)
        size_t cursorPos = doc->cursor.position;
        std::string content = doc->value;
        
        // Bounds check for cursor position
        if (cursorPos > content.length()) {
            cursorPos = content.length();
        }
        
        size_t prefixStart = cursorPos;
        
        // Find the start of the current word/prefix
        while (prefixStart > 0 && prefixStart <= content.length() &&
               (std::isalnum(content[prefixStart - 1]) || 
                content[prefixStart - 1] == '_' || 
                content[prefixStart - 1] == ':')) {
            prefixStart--;
        }
        
        // Determine what text to insert based on completion kind
        std::string insertText;
        if (selectedCompletion.kind == "snippet") {
            // For snippets, use the detail which contains the full code
            insertText = selectedCompletion.detail;
        } else {
            // For simple completions, use the label
            insertText = selectedCompletion.label;
        }
        
        // Remove the prefix and insert the completion
        std::string newContent = content.substr(0, prefixStart) + 
                                 insertText + 
                                 content.substr(cursorPos);
        
        // Update document content
        doc->value = newContent;
        
        // Move cursor to end of inserted text
        size_t newCursorPos = prefixStart + insertText.length();
        doc->cursor.position = newCursorPos;
        
        // Update editor store
        editorStore_.setDocument(doc->filePath, *doc);
        
        // Deactivate completion
        codeCompletion_->deactivate();
    }
}

void IntegratedEditor::cancelCompletion() {
    codeCompletion_->deactivate();
}

void IntegratedEditor::selectNextCompletion() {
    if (codeCompletion_->isActive()) {
        codeCompletion_->selectNext();
    }
}

void IntegratedEditor::selectPreviousCompletion() {
    if (codeCompletion_->isActive()) {
        codeCompletion_->selectPrevious();
    }
}

CompletionItem IntegratedEditor::getSelectedCompletion() const {
    return codeCompletion_->getSelectedSuggestion();
}

// AI completion settings
void IntegratedEditor::setAICompletionEnabled(bool enabled) {
    aiCompletionEngine_.setAIEnabled(enabled);
}

bool IntegratedEditor::isAICompletionEnabled() const {
    return aiCompletionEngine_.isAIEnabled();
}

bool IntegratedEditor::isAICompletionReady() const {
    return aiCompletionEngine_.isAIReady();
}

// Debugger operations
bool IntegratedEditor::startDebugSession(const std::string& filePath) {
    try {
        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Start debug session from Limbo source
        return debugger_->start_debug_session_from_source(content);
        
    } catch (const std::exception&) {
        return false;
    }
}

bool IntegratedEditor::startDebugSessionFromSource(const std::string& limboSource) {
    return debugger_->start_debug_session_from_source(limboSource);
}

void IntegratedEditor::stopDebugSession() {
    debugger_->stop_debug_session();
}

bool IntegratedEditor::isDebugging() const {
    return debugger_->is_debugging();
}

DebugState IntegratedEditor::getDebugState() const {
    return debugger_->get_debug_state();
}

// Debug execution control
void IntegratedEditor::debugContinue() {
    debugger_->continue_execution();
}

void IntegratedEditor::debugStepOver() {
    debugger_->step_over();
}

void IntegratedEditor::debugStepInto() {
    debugger_->step_into();
}

void IntegratedEditor::debugStepOut() {
    debugger_->step_out();
}

void IntegratedEditor::debugPause() {
    debugger_->pause_execution();
}

// Debug breakpoint management
bool IntegratedEditor::toggleBreakpointAtLine(const std::string& filePath, size_t line) {
    return debugger_->set_breakpoint_at_line(filePath, line);
}

bool IntegratedEditor::setBreakpointAtLine(const std::string& filePath, size_t line) {
    return debugger_->set_breakpoint_at_line(filePath, line);
}

bool IntegratedEditor::removeBreakpointAtLine(const std::string& filePath, size_t line) {
    return debugger_->remove_breakpoint_at_line(filePath, line);
}

void IntegratedEditor::clearAllBreakpoints() {
    debugger_->clear_all_breakpoints();
}

std::vector<BreakpointInfo> IntegratedEditor::getAllBreakpoints() const {
    return debugger_->get_all_breakpoints();
}

// Debug UI
void IntegratedEditor::showDebugger() {
    debuggerUI_->set_visible(true);
}

void IntegratedEditor::hideDebugger() {
    debuggerUI_->set_visible(false);
}

bool IntegratedEditor::isDebuggerVisible() const {
    return debuggerUI_->is_visible();
}

void IntegratedEditor::toggleDebuggerVisibility() {
    debuggerUI_->set_visible(!debuggerUI_->is_visible());
}

// Debug information
size_t IntegratedEditor::getCurrentDebugPC() const {
    return debugger_->get_current_pc();
}

std::string IntegratedEditor::getCurrentDebugInstruction() const {
    return debugger_->get_current_instruction();
}

std::vector<size_t> IntegratedEditor::getDebugCallStack() const {
    return debugger_->get_call_stack();
}

std::vector<std::string> IntegratedEditor::getDebugStackContents() const {
    return debugger_->get_stack_contents();
}

std::map<std::string, std::string> IntegratedEditor::getDebugGlobalVariables() const {
    return debugger_->get_global_variables();
}

// Tab management operations
size_t IntegratedEditor::openDocumentInTab(const std::string& filePath, const std::string& content) {
    // Add tab first
    size_t tabId = tabBar_.addTab(filePath);
    
    // Open the document
    openDocument(filePath, content);
    
    return tabId;
}

bool IntegratedEditor::closeTabById(size_t tabId) {
    // Get the tab to find its file path
    auto tab = tabBar_.getTab(tabId);
    if (!tab.has_value()) {
        return false;
    }
    
    // Close the document
    closeDocument(tab->filePath);
    
    // Close the tab
    return tabBar_.closeTab(tabId);
}

bool IntegratedEditor::closeTabByPath(const std::string& filePath) {
    // Close the document
    closeDocument(filePath);
    
    // Close the tab
    return tabBar_.closeTabByPath(filePath);
}

void IntegratedEditor::closeAllTabs() {
    // Get all tabs first (to avoid modification during iteration)
    auto tabs = tabBar_.getAllTabs();
    
    // Close all documents
    for (const auto& tab : tabs) {
        if (!tab.isPinned) {
            closeDocument(tab.filePath);
        }
    }
    
    // Close all tabs
    tabBar_.closeAllTabs();
}

void IntegratedEditor::closeOtherTabs(size_t exceptTabId) {
    // Get all tabs first
    auto tabs = tabBar_.getAllTabs();
    
    // Close all documents except the one we're keeping
    for (const auto& tab : tabs) {
        if (tab.id != exceptTabId && !tab.isPinned) {
            closeDocument(tab.filePath);
        }
    }
    
    // Close other tabs
    tabBar_.closeOtherTabs(exceptTabId);
}

bool IntegratedEditor::switchToTab(size_t tabId) {
    // Get the tab to find its file path
    auto tab = tabBar_.getTab(tabId);
    if (!tab.has_value()) {
        return false;
    }
    
    // Set as selected file in editor store
    editorStore_.setSelectedFile(tab->filePath);
    
    // Activate the tab
    return tabBar_.activateTab(tabId);
}

bool IntegratedEditor::switchToTabByPath(const std::string& filePath) {
    // Set as selected file in editor store
    editorStore_.setSelectedFile(filePath);
    
    // Activate the tab
    return tabBar_.activateTabByPath(filePath);
}

void IntegratedEditor::switchToNextTab() {
    tabBar_.activateNextTab();
    
    // Update selected file in editor store
    auto activeTab = tabBar_.getActiveTab();
    if (activeTab.has_value()) {
        editorStore_.setSelectedFile(activeTab->filePath);
    }
}

void IntegratedEditor::switchToPreviousTab() {
    tabBar_.activatePreviousTab();
    
    // Update selected file in editor store
    auto activeTab = tabBar_.getActiveTab();
    if (activeTab.has_value()) {
        editorStore_.setSelectedFile(activeTab->filePath);
    }
}

std::optional<EditorTab> IntegratedEditor::getActiveTab() const {
    return tabBar_.getActiveTab();
}

std::vector<EditorTab> IntegratedEditor::getAllTabs() const {
    return tabBar_.getAllTabs();
}

size_t IntegratedEditor::getTabCount() const {
    return tabBar_.getTabCount();
}

bool IntegratedEditor::hasOpenTabs() const {
    return tabBar_.hasTabs();
}

void IntegratedEditor::setTabDirty(size_t tabId, bool isDirty) {
    tabBar_.setTabDirty(tabId, isDirty);
}

void IntegratedEditor::setTabPinned(size_t tabId, bool isPinned) {
    tabBar_.setTabPinned(tabId, isPinned);
}

// ============================================================================
// Debugger-Editor Integration: Line highlighting
// ============================================================================

void IntegratedEditor::highlightLine(const std::string& filePath, size_t line, HighlightType type) {
    lineHighlights_[filePath][line].insert(type);

    // If this is a debug highlight, scroll to make the line visible
    if (type == HighlightType::DebugCurrent || type == HighlightType::DebugBreakpoint) {
        revealLine(filePath, line, true);
    }
}

void IntegratedEditor::clearHighlight(const std::string& filePath, size_t line, HighlightType type) {
    auto fileIt = lineHighlights_.find(filePath);
    if (fileIt != lineHighlights_.end()) {
        auto lineIt = fileIt->second.find(line);
        if (lineIt != fileIt->second.end()) {
            lineIt->second.erase(type);
            // Clean up empty entries
            if (lineIt->second.empty()) {
                fileIt->second.erase(lineIt);
            }
            if (fileIt->second.empty()) {
                lineHighlights_.erase(fileIt);
            }
        }
    }
}

void IntegratedEditor::clearAllHighlights(const std::string& filePath) {
    lineHighlights_.erase(filePath);
}

void IntegratedEditor::clearAllHighlightsOfType(HighlightType type) {
    for (auto& [filePath, lines] : lineHighlights_) {
        for (auto lineIt = lines.begin(); lineIt != lines.end(); ) {
            lineIt->second.erase(type);
            if (lineIt->second.empty()) {
                lineIt = lines.erase(lineIt);
            } else {
                ++lineIt;
            }
        }
    }
    // Clean up empty file entries
    for (auto it = lineHighlights_.begin(); it != lineHighlights_.end(); ) {
        if (it->second.empty()) {
            it = lineHighlights_.erase(it);
        } else {
            ++it;
        }
    }
}

bool IntegratedEditor::hasHighlight(const std::string& filePath, size_t line, HighlightType type) const {
    auto fileIt = lineHighlights_.find(filePath);
    if (fileIt != lineHighlights_.end()) {
        auto lineIt = fileIt->second.find(line);
        if (lineIt != fileIt->second.end()) {
            return lineIt->second.count(type) > 0;
        }
    }
    return false;
}

std::vector<std::pair<size_t, IntegratedEditor::HighlightType>> IntegratedEditor::getHighlights(const std::string& filePath) const {
    std::vector<std::pair<size_t, HighlightType>> result;
    auto fileIt = lineHighlights_.find(filePath);
    if (fileIt != lineHighlights_.end()) {
        for (const auto& [line, types] : fileIt->second) {
            for (const auto& type : types) {
                result.emplace_back(line, type);
            }
        }
    }
    return result;
}

// ============================================================================
// Debugger-Editor Integration: Breakpoint markers
// ============================================================================

void IntegratedEditor::setBreakpointMarker(const std::string& filePath, size_t line, bool enabled) {
    breakpointMarkers_[filePath][line] = enabled;
}

void IntegratedEditor::clearBreakpointMarker(const std::string& filePath, size_t line) {
    auto fileIt = breakpointMarkers_.find(filePath);
    if (fileIt != breakpointMarkers_.end()) {
        fileIt->second.erase(line);
        if (fileIt->second.empty()) {
            breakpointMarkers_.erase(fileIt);
        }
    }
}

void IntegratedEditor::clearAllBreakpointMarkers(const std::string& filePath) {
    breakpointMarkers_.erase(filePath);
}

bool IntegratedEditor::hasBreakpointMarker(const std::string& filePath, size_t line) const {
    auto fileIt = breakpointMarkers_.find(filePath);
    if (fileIt != breakpointMarkers_.end()) {
        return fileIt->second.count(line) > 0;
    }
    return false;
}

std::vector<std::pair<size_t, bool>> IntegratedEditor::getBreakpointMarkers(const std::string& filePath) const {
    std::vector<std::pair<size_t, bool>> result;
    auto fileIt = breakpointMarkers_.find(filePath);
    if (fileIt != breakpointMarkers_.end()) {
        for (const auto& [line, enabled] : fileIt->second) {
            result.emplace_back(line, enabled);
        }
    }
    return result;
}

// ============================================================================
// Debugger-Editor Integration: Navigation
// ============================================================================

void IntegratedEditor::scrollToLine(const std::string& filePath, size_t line) {
    auto* doc = editorStore_.getCurrentDocument();
    if (doc && doc->filePath == filePath) {
        // Update scroll position to the target line
        doc->scroll.line = static_cast<int>(line);
        doc->scroll.character = 0;
    }
}

void IntegratedEditor::revealLine(const std::string& filePath, size_t line, bool center) {
    // First ensure the document is open and focused
    focusDocument(filePath);

    auto* doc = editorStore_.getCurrentDocument();
    if (doc && doc->filePath == filePath) {
        if (center) {
            // Adjust scroll to center the line (assuming ~30 visible lines)
            size_t visibleLines = 30;
            if (line > visibleLines / 2) {
                doc->scroll.line = static_cast<int>(line - visibleLines / 2);
            } else {
                doc->scroll.line = 0;
            }
        } else {
            doc->scroll.line = static_cast<int>(line);
        }
        doc->scroll.character = 0;

        // Move cursor to the beginning of the target line
        size_t offset = 0;
        size_t currentLine = 0;
        for (size_t i = 0; i < doc->value.length() && currentLine < line; ++i) {
            if (doc->value[i] == '\n') {
                currentLine++;
                offset = i + 1;
            }
        }
        doc->cursor.position = offset;
    }
}

void IntegratedEditor::focusDocument(const std::string& filePath) {
    // Set as selected file in editor store
    editorStore_.setSelectedFile(filePath);

    // If using tabs, switch to the appropriate tab
    switchToTabByPath(filePath);

    // If using split view, focus the pane containing this document
    if (splitViewManager_.isEnabled()) {
        std::string paneId = findPaneWithDocument(filePath);
        if (!paneId.empty()) {
            focusPane(paneId);
        }
    }
}

// ============================================================================
// Debugger-Editor Integration: Source mapping
// ============================================================================

void IntegratedEditor::registerSourceMapping(const std::string& filePath, size_t line, size_t pc) {
    std::string key = filePath + ":" + std::to_string(line);
    lineToPc_[key] = pc;
    pcToLine_[pc] = {filePath, line};
}

void IntegratedEditor::clearSourceMappings(const std::string& filePath) {
    // Remove all mappings for this file
    std::string prefix = filePath + ":";

    // Find and remove line->PC mappings
    for (auto it = lineToPc_.begin(); it != lineToPc_.end(); ) {
        if (it->first.compare(0, prefix.length(), prefix) == 0) {
            // Also remove the corresponding PC->line mapping
            pcToLine_.erase(it->second);
            it = lineToPc_.erase(it);
        } else {
            ++it;
        }
    }
}

std::optional<size_t> IntegratedEditor::getPCForLine(const std::string& filePath, size_t line) const {
    std::string key = filePath + ":" + std::to_string(line);
    auto it = lineToPc_.find(key);
    if (it != lineToPc_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> IntegratedEditor::getLineForPC(size_t pc) const {
    auto it = pcToLine_.find(pc);
    if (it != pcToLine_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace bolt