#ifndef BOLT_ERROR_HANDLING_HPP
#define BOLT_ERROR_HANDLING_HPP

#include <stdexcept>
#include <string>
#include <system_error>

namespace bolt {

// Error categories for different components
enum class ErrorCode {
    // Memory errors
    MEMORY_ALLOCATION_FAILED = 100,
    INVALID_MEMORY_SIZE,
    NULL_POINTER_ACCESS,
    MEMORY_CORRUPTION,
    
    // Message handling errors
    MESSAGE_QUEUE_FULL = 200,
    INVALID_MESSAGE_TYPE,
    MESSAGE_PROCESSING_FAILED,
    MESSAGE_HANDLER_NOT_INITIALIZED,
    
    // Store errors
    STORE_STATE_INVALID = 300,
    STORE_LISTENER_FAILED,
    STORE_DATA_CORRUPTED,
    
    // Editor errors
    EDITOR_DOCUMENT_INVALID = 400,
    EDITOR_OPERATION_FAILED,
    EDITOR_FOLDING_ERROR,
    
    // General errors
    COMPONENT_NOT_INITIALIZED = 500,
    INVALID_PARAMETER,
    OPERATION_TIMEOUT,
    UNKNOWN_ERROR = 999
};

// Base exception class for Bolt components
class BoltException : public std::runtime_error {
public:
    explicit BoltException(ErrorCode code, const std::string& message)
        : std::runtime_error(formatMessage(code, message)), errorCode_(code) {}
    
    ErrorCode getErrorCode() const noexcept { return errorCode_; }
    
private:
    ErrorCode errorCode_;
    
    static std::string formatMessage(ErrorCode code, const std::string& message) {
        return "[Bolt Error " + std::to_string(static_cast<int>(code)) + "] " + message;
    }
};

// Specific exception classes for different components
class MemoryException : public BoltException {
public:
    explicit MemoryException(ErrorCode code, const std::string& message)
        : BoltException(code, message) {}
};

class MessageException : public BoltException {
public:
    explicit MessageException(ErrorCode code, const std::string& message)
        : BoltException(code, message) {}
};

class StoreException : public BoltException {
public:
    explicit StoreException(ErrorCode code, const std::string& message)
        : BoltException(code, message) {}
};

class EditorException : public BoltException {
public:
    explicit EditorException(ErrorCode code, const std::string& message)
        : BoltException(code, message) {}
};

// Error handling utilities
class ErrorHandler {
public:
    // Validate common conditions
    static void validateNotNull(const void* ptr, const std::string& paramName) {
        if (!ptr) {
            throw BoltException(ErrorCode::NULL_POINTER_ACCESS, 
                              "Parameter '" + paramName + "' cannot be null");
        }
    }
    
    static void validateParameter(bool condition, const std::string& message) {
        if (!condition) {
            throw BoltException(ErrorCode::INVALID_PARAMETER, message);
        }
    }
    
    static void validateMemorySize(size_t size) {
        if (size == 0) {
            throw MemoryException(ErrorCode::INVALID_MEMORY_SIZE, 
                                "Memory size cannot be zero");
        }
        if (size > SIZE_MAX / 2) {
            throw MemoryException(ErrorCode::INVALID_MEMORY_SIZE, 
                                "Memory size too large: " + std::to_string(size));
        }
    }
    
    static void validateInitialized(bool initialized, const std::string& componentName) {
        if (!initialized) {
            throw BoltException(ErrorCode::COMPONENT_NOT_INITIALIZED,
                              "Component '" + componentName + "' not initialized");
        }
    }
};

// RAII-style error scope for automatic error handling
class ErrorScope {
public:
    explicit ErrorScope(const std::string& operation) : operation_(operation) {}
    
    ~ErrorScope() noexcept {
        // Log completion if needed
    }
    
    template<typename Func>
    auto execute(Func&& func) -> decltype(func()) {
        try {
            return func();
        } catch (const BoltException& e) {
            // Re-throw Bolt exceptions as-is
            throw;
        } catch (const std::exception& e) {
            // Wrap standard exceptions
            throw BoltException(ErrorCode::UNKNOWN_ERROR, 
                              "Error in " + operation_ + ": " + e.what());
        }
    }
    
private:
    std::string operation_;
};

// Macro for error scope usage
#define BOLT_ERROR_SCOPE(operation) \
    bolt::ErrorScope errorScope(operation); \
    errorScope.execute([&]()

#define BOLT_ERROR_SCOPE_END )

} // namespace bolt

#endif