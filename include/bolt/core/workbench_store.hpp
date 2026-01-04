#ifndef WORKBENCH_STORE_HPP
#define WORKBENCH_STORE_HPP

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "bolt/gui_components.hpp"
#include "bolt/core/error_handling.hpp"

namespace bolt {

class WorkbenchStore {
public:
    static WorkbenchStore& getInstance() {
        static WorkbenchStore instance;
        return instance;
    }

    void setShowWorkbench(bool show) { 
        std::lock_guard<std::mutex> lock(mutex_);
        showWorkbench_ = show;
        notifyListeners();
    }

    bool getShowWorkbench() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return showWorkbench_; 
    }

    void setCurrentView(const std::string& view) {
        ErrorHandler::validateParameter(!view.empty(), "View name cannot be empty");
        if (view.length() > MAX_VIEW_NAME_LENGTH) {
            throw StoreException(ErrorCode::INVALID_PARAMETER,
                "View name too long: " + std::to_string(view.length()) + 
                " > " + std::to_string(MAX_VIEW_NAME_LENGTH));
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        currentView_ = view;
        notifyListeners();
    }

    std::string getCurrentView() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return currentView_; 
    }

    void setSelectedFile(const std::string& file) {
        if (!file.empty() && file.length() > MAX_FILE_PATH_LENGTH) {
            throw StoreException(ErrorCode::INVALID_PARAMETER,
                "File path too long: " + std::to_string(file.length()) + 
                " > " + std::to_string(MAX_FILE_PATH_LENGTH));
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        selectedFile_ = file;
        notifyListeners();
    }

    std::string getSelectedFile() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return selectedFile_;
    }

    void toggleTerminal(bool show = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        showTerminal_ = show;
        notifyListeners();
    }

    bool getShowTerminal() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return showTerminal_;
    }

    void addListener(std::function<void()> listener) {
        ErrorHandler::validateParameter(listener != nullptr, "Listener cannot be null");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (listeners_.size() >= MAX_LISTENERS) {
            throw StoreException(ErrorCode::STORE_STATE_INVALID,
                "Too many listeners: " + std::to_string(listeners_.size()) + 
                " >= " + std::to_string(MAX_LISTENERS));
        }
        
        listeners_.push_back(listener);
    }

    Workbench& getWorkbench() { 
        std::lock_guard<std::mutex> lock(mutex_);
        ErrorHandler::validateNotNull(workbench_.get(), "workbench");
        return *workbench_; 
    }
    
    size_t getListenerCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

private:
    WorkbenchStore() : workbench_(std::make_unique<Workbench>()) {
        showWorkbench_ = false;
        currentView_ = "code";
        showTerminal_ = false;
    }

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

    bool showWorkbench_;
    bool showTerminal_;
    std::string currentView_;
    std::string selectedFile_;
    std::unique_ptr<Workbench> workbench_;
    std::vector<std::function<void()>> listeners_;
    mutable std::mutex mutex_;
    
    static constexpr size_t MAX_VIEW_NAME_LENGTH = 256;
    static constexpr size_t MAX_FILE_PATH_LENGTH = 2048;
    static constexpr size_t MAX_LISTENERS = 100;
};

} // namespace bolt

#endif