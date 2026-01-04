#ifndef SPLIT_VIEW_MANAGER_HPP
#define SPLIT_VIEW_MANAGER_HPP

#include "bolt/editor/editor_pane.hpp"
#include "bolt/core/thread_safety.hpp"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>

namespace bolt {

/**
 * Manages split view editing functionality
 * Handles creation, management, and layout of multiple editor panes
 */
class SplitViewManager {
public:
    enum class SplitDirection {
        Horizontal,  // Split left/right
        Vertical     // Split top/bottom
    };
    
    enum class PaneOperation {
        Create,
        Close,
        Focus,
        Resize
    };

    struct SplitLayout {
        std::string paneId;
        EditorPane::PanePosition position;
        std::vector<std::string> childPanes;  // For nested splits
        SplitDirection splitDirection = SplitDirection::Horizontal;
    };

    // Callback type for pane operation notifications
    using PaneOperationCallback = std::function<void(PaneOperation, const std::string&)>;

private:
    ThreadSafe<std::map<std::string, std::unique_ptr<EditorPane>>> panes_;
    ThreadSafe<std::string> activePaneId_;
    ThreadSafe<std::vector<SplitLayout>> layouts_;
    ThreadSafe<int> nextPaneId_{1};
    ThreadSafe<bool> isEnabled_{true};
    
    // Callbacks
    PaneOperationCallback onPaneOperation_;
    
    // Layout constraints
    static constexpr int MIN_PANE_WIDTH = 100;
    static constexpr int MIN_PANE_HEIGHT = 50;

public:
    static SplitViewManager& getInstance() {
        static SplitViewManager instance;
        return instance;
    }

    // Core split operations
    std::string createHorizontalSplit(const std::string& sourcePaneId = "");
    std::string createVerticalSplit(const std::string& sourcePaneId = "");
    bool closePane(const std::string& paneId);
    bool focusPane(const std::string& paneId);
    
    // Pane management
    std::string createPane();
    EditorPane* getPane(const std::string& paneId);
    const EditorPane* getPane(const std::string& paneId) const;
    EditorPane* getActivePane();
    const EditorPane* getActivePane() const;
    std::string getActivePaneId() const;
    
    // Layout management
    void setContainerSize(int width, int height);
    void resizePane(const std::string& paneId, int width, int height);
    void redistributeLayout();
    std::vector<std::string> getAllPaneIds() const;
    size_t getPaneCount() const;
    
    // Document operations across panes
    void openDocumentInPane(const std::string& paneId, const std::string& filePath);
    void openDocumentInNewPane(const std::string& filePath, SplitDirection direction = SplitDirection::Horizontal);
    std::vector<std::string> getOpenDocuments() const;
    std::string findPaneWithDocument(const std::string& filePath) const;
    
    // Split view state
    bool isEnabled() const;
    void setEnabled(bool enabled);
    bool hasSplits() const;
    void collapseAllSplits();
    
    // Navigation
    std::string getNextPane(const std::string& currentPaneId, bool forward = true) const;
    bool navigateToNextPane();
    bool navigateToPreviousPane();
    
    // Callbacks
    void setOnPaneOperation(PaneOperationCallback callback);
    
    // Layout persistence (for future use)
    std::string serializeLayout() const;
    bool restoreLayout(const std::string& layoutData);
    
    // Cleanup
    void clearAllPanes();

private:
    SplitViewManager();
    ~SplitViewManager() = default;
    
    // Layout calculations
    void calculatePanePositions();
    void updateLayout(const std::string& modifiedPaneId = "");
    EditorPane::PanePosition calculateSplitPosition(
        const EditorPane::PanePosition& sourcePosition, 
        SplitDirection direction, 
        float splitRatio = 0.5f
    );
    
    // Internal helpers
    std::string generatePaneId();
    void notifyPaneOperation(PaneOperation op, const std::string& paneId);
    bool isValidPaneId(const std::string& paneId) const;
    void removePaneFromLayout(const std::string& paneId);
    void addPaneToLayout(const std::string& paneId, const EditorPane::PanePosition& position);
    
    // Layout constraints
    bool validatePaneSize(int width, int height) const;
    void enforceSizeConstraints(EditorPane::PanePosition& position) const;
    
    // Thread safety helpers
    template<typename Func>
    auto withPanes(Func&& func) const -> decltype(func(std::declval<const std::map<std::string, std::unique_ptr<EditorPane>>&>()));
    
    template<typename Func>
    auto withPanes(Func&& func) -> decltype(func(std::declval<std::map<std::string, std::unique_ptr<EditorPane>>&>()));
};

} // namespace bolt

#endif