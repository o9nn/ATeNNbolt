#ifndef LSP_JSON_RPC_HPP
#define LSP_JSON_RPC_HPP

#include <string>
#include <memory>
#include <optional>
#include <map>
#include <functional>

namespace bolt {
namespace lsp {

// Forward declaration for value type
class JsonValue;

// JSON-RPC Message Types
enum class MessageType {
    Request,
    Response,
    Notification
};

// JSON-RPC Request
struct JsonRpcRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    std::optional<std::string> id;
    std::shared_ptr<JsonValue> params;
    
    JsonRpcRequest() = default;
    JsonRpcRequest(const std::string& m, const std::string& reqId = "") 
        : method(m), id(reqId.empty() ? std::nullopt : std::optional<std::string>(reqId)) {}
};

// JSON-RPC Response
struct JsonRpcResponse {
    std::string jsonrpc = "2.0";
    std::string id;
    std::shared_ptr<JsonValue> result;
    std::shared_ptr<JsonValue> error;
    
    JsonRpcResponse() = default;
    JsonRpcResponse(const std::string& reqId) : id(reqId) {}
};

// JSON-RPC Notification
struct JsonRpcNotification {
    std::string jsonrpc = "2.0";
    std::string method;
    std::shared_ptr<JsonValue> params;
    
    JsonRpcNotification() = default;
    JsonRpcNotification(const std::string& m) : method(m) {}
};

// Simple JSON Value representation (minimal implementation)
class JsonValue {
public:
    enum Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };
    
private:
    Type type_;
    std::string stringValue_;
    double numberValue_;
    bool boolValue_;
    std::vector<std::shared_ptr<JsonValue>> arrayValue_;
    std::map<std::string, std::shared_ptr<JsonValue>> objectValue_;
    
public:
    JsonValue() : type_(Null) {}
    explicit JsonValue(Type type) : type_(type) {}  // Explicit type constructor
    JsonValue(const std::string& value) : type_(String), stringValue_(value) {}
    JsonValue(const char* value) : type_(String), stringValue_(value) {}  // Prevent implicit bool conversion
    JsonValue(double value) : type_(Number), numberValue_(value) {}
    explicit JsonValue(int value) : type_(Number), numberValue_(static_cast<double>(value)) {}  // Explicit int constructor
    JsonValue(bool value) : type_(Bool), boolValue_(value) {}
    
    Type getType() const { return type_; }
    
    const std::string& asString() const { return stringValue_; }
    double asNumber() const { return numberValue_; }
    bool asBool() const { return boolValue_; }
    
    void setString(const std::string& value) { type_ = String; stringValue_ = value; }
    void setNumber(double value) { type_ = Number; numberValue_ = value; }
    void setBool(bool value) { type_ = Bool; boolValue_ = value; }
    
    // Object operations
    void setObject() { type_ = Object; objectValue_.clear(); }
    void setProperty(const std::string& key, std::shared_ptr<JsonValue> value) {
        if (type_ != Object) setObject();
        objectValue_[key] = value;
    }
    std::shared_ptr<JsonValue> getProperty(const std::string& key) const {
        if (type_ != Object) return nullptr;
        auto it = objectValue_.find(key);
        return it != objectValue_.end() ? it->second : nullptr;
    }
    
    // Array operations
    void setArray() { type_ = Array; arrayValue_.clear(); }
    void addArrayElement(std::shared_ptr<JsonValue> value) {
        if (type_ != Array) setArray();
        arrayValue_.push_back(value);
    }
    size_t getArraySize() const { return type_ == Array ? arrayValue_.size() : 0; }
    std::shared_ptr<JsonValue> getArrayElement(size_t index) const {
        return (type_ == Array && index < arrayValue_.size()) ? arrayValue_[index] : nullptr;
    }
    
    // Serialization (basic implementation)
    std::string toString() const;
    static std::shared_ptr<JsonValue> fromString(const std::string& json);
};

// JSON-RPC Message Handler
class JsonRpcHandler {
public:
    using RequestHandler = std::function<std::shared_ptr<JsonValue>(const std::string&, std::shared_ptr<JsonValue>)>;
    using NotificationHandler = std::function<void(const std::string&, std::shared_ptr<JsonValue>)>;
    
private:
    std::map<std::string, RequestHandler> requestHandlers_;
    std::map<std::string, NotificationHandler> notificationHandlers_;
    
public:
    // Register handlers
    void registerRequestHandler(const std::string& method, RequestHandler handler);
    void registerNotificationHandler(const std::string& method, NotificationHandler handler);
    
    // Process incoming messages
    std::string processMessage(const std::string& message);
    
    // Send outgoing messages
    std::string createRequest(const std::string& method, std::shared_ptr<JsonValue> params, const std::string& id = "");
    std::string createNotification(const std::string& method, std::shared_ptr<JsonValue> params);
    std::string createResponse(const std::string& id, std::shared_ptr<JsonValue> result);
    std::string createErrorResponse(const std::string& id, int code, const std::string& message);
    
private:
    JsonRpcRequest parseRequest(std::shared_ptr<JsonValue> json);
    JsonRpcResponse parseResponse(std::shared_ptr<JsonValue> json);
    JsonRpcNotification parseNotification(std::shared_ptr<JsonValue> json);
    
    std::shared_ptr<JsonValue> createJsonRpcObject(const JsonRpcRequest& request);
    std::shared_ptr<JsonValue> createJsonRpcObject(const JsonRpcResponse& response);
    std::shared_ptr<JsonValue> createJsonRpcObject(const JsonRpcNotification& notification);
};

} // namespace lsp
} // namespace bolt

#endif