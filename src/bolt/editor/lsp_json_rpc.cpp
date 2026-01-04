#include "bolt/editor/lsp_json_rpc.hpp"
#include <sstream>
#include <iostream>
#include <cctype>

namespace bolt {
namespace lsp {

// Basic JSON Value implementation
std::string JsonValue::toString() const {
    switch (type_) {
        case Null:
            return "null";
        case Bool:
            return boolValue_ ? "true" : "false";
        case Number:
            return std::to_string(numberValue_);
        case String: {
            std::ostringstream oss;
            oss << "\"";
            // Basic string escaping
            for (char c : stringValue_) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            oss << "\"";
            return oss.str();
        }
        case Array: {
            std::ostringstream oss;
            oss << "[";
            for (size_t i = 0; i < arrayValue_.size(); ++i) {
                if (i > 0) oss << ",";
                oss << arrayValue_[i]->toString();
            }
            oss << "]";
            return oss.str();
        }
        case Object: {
            std::ostringstream oss;
            oss << "{";
            bool first = true;
            for (const auto& pair : objectValue_) {
                if (!first) oss << ",";
                first = false;
                oss << "\"" << pair.first << "\":" << pair.second->toString();
            }
            oss << "}";
            return oss.str();
        }
    }
    return "null";
}

// Simple built-in JSON parser (no external dependencies)
class SimpleJsonParser {
private:
    const std::string& json_;
    size_t pos_ = 0;

    void skipWhitespace() {
        while (pos_ < json_.size() && std::isspace(json_[pos_])) {
            ++pos_;
        }
    }

    char peek() const {
        return pos_ < json_.size() ? json_[pos_] : '\0';
    }

    char consume() {
        return pos_ < json_.size() ? json_[pos_++] : '\0';
    }

    bool match(char c) {
        if (peek() == c) {
            consume();
            return true;
        }
        return false;
    }

    bool matchString(const std::string& s) {
        if (json_.substr(pos_, s.length()) == s) {
            pos_ += s.length();
            return true;
        }
        return false;
    }

    std::string parseString() {
        std::string result;
        if (!match('"')) return result;

        while (pos_ < json_.size()) {
            char c = consume();
            if (c == '"') break;
            if (c == '\\') {
                c = consume();
                switch (c) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        // Unicode escape - simplified handling
                        std::string hex;
                        for (int i = 0; i < 4 && pos_ < json_.size(); ++i) {
                            hex += consume();
                        }
                        // For simplicity, just skip unicode escapes for now
                        result += '?';
                        break;
                    }
                    default: result += c; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    double parseNumber() {
        std::string numStr;
        if (peek() == '-') numStr += consume();
        while (pos_ < json_.size() && (std::isdigit(peek()) || peek() == '.' || peek() == 'e' || peek() == 'E' || peek() == '+' || peek() == '-')) {
            char c = consume();
            // Don't add sign if it's not at start or after e/E
            if ((c == '+' || c == '-') && !numStr.empty() && numStr.back() != 'e' && numStr.back() != 'E') {
                --pos_;
                break;
            }
            numStr += c;
        }
        try {
            return std::stod(numStr);
        } catch (...) {
            return 0.0;
        }
    }

public:
    explicit SimpleJsonParser(const std::string& json) : json_(json) {}

    std::shared_ptr<JsonValue> parse() {
        skipWhitespace();
        return parseValue();
    }

    std::shared_ptr<JsonValue> parseValue() {
        skipWhitespace();
        char c = peek();

        if (c == '\0') {
            return std::make_shared<JsonValue>();
        }

        if (c == 'n') {
            if (matchString("null")) {
                return std::make_shared<JsonValue>();
            }
        }

        if (c == 't') {
            if (matchString("true")) {
                return std::make_shared<JsonValue>(true);
            }
        }

        if (c == 'f') {
            if (matchString("false")) {
                return std::make_shared<JsonValue>(false);
            }
        }

        if (c == '"') {
            std::string str = parseString();
            return std::make_shared<JsonValue>(str);
        }

        if (c == '-' || std::isdigit(c)) {
            double num = parseNumber();
            return std::make_shared<JsonValue>(num);
        }

        if (c == '[') {
            return parseArray();
        }

        if (c == '{') {
            return parseObject();
        }

        // Unknown - return null
        return std::make_shared<JsonValue>();
    }

    std::shared_ptr<JsonValue> parseArray() {
        auto arr = std::make_shared<JsonValue>();
        arr->setArray();

        if (!match('[')) return arr;
        skipWhitespace();

        if (peek() == ']') {
            consume();
            return arr;
        }

        while (true) {
            skipWhitespace();
            auto elem = parseValue();
            arr->addArrayElement(elem);

            skipWhitespace();
            if (peek() == ']') {
                consume();
                break;
            }
            if (!match(',')) break;
        }

        return arr;
    }

    std::shared_ptr<JsonValue> parseObject() {
        auto obj = std::make_shared<JsonValue>();
        obj->setObject();

        if (!match('{')) return obj;
        skipWhitespace();

        if (peek() == '}') {
            consume();
            return obj;
        }

        while (true) {
            skipWhitespace();
            if (peek() != '"') break;

            std::string key = parseString();
            skipWhitespace();

            if (!match(':')) break;

            skipWhitespace();
            auto value = parseValue();
            obj->setProperty(key, value);

            skipWhitespace();
            if (peek() == '}') {
                consume();
                break;
            }
            if (!match(',')) break;
        }

        return obj;
    }
};

std::shared_ptr<JsonValue> JsonValue::fromString(const std::string& json) {
    if (json.empty()) {
        return std::make_shared<JsonValue>();
    }

    SimpleJsonParser parser(json);
    return parser.parse();
}

// JSON-RPC Handler implementation
void JsonRpcHandler::registerRequestHandler(const std::string& method, RequestHandler handler) {
    requestHandlers_[method] = handler;
}

void JsonRpcHandler::registerNotificationHandler(const std::string& method, NotificationHandler handler) {
    notificationHandlers_[method] = handler;
}

std::string JsonRpcHandler::processMessage(const std::string& message) {
    try {
        auto json = JsonValue::fromString(message);
        if (!json || json->getType() != JsonValue::Object) {
            return createErrorResponse("", -32700, "Parse error");
        }

        auto methodProperty = json->getProperty("method");
        auto idProperty = json->getProperty("id");

        if (!methodProperty || methodProperty->getType() != JsonValue::String) {
            return createErrorResponse(idProperty ? idProperty->asString() : "", -32600, "Invalid Request");
        }

        std::string method = methodProperty->asString();
        auto paramsProperty = json->getProperty("params");

        // Check if it's a notification (no id) or request (has id)
        if (!idProperty) {
            // Notification
            auto it = notificationHandlers_.find(method);
            if (it != notificationHandlers_.end()) {
                it->second(method, paramsProperty);
            }
            return ""; // No response for notifications
        } else {
            // Request
            std::string id = idProperty->asString();
            auto it = requestHandlers_.find(method);
            if (it != requestHandlers_.end()) {
                auto result = it->second(method, paramsProperty);
                return createResponse(id, result);
            } else {
                return createErrorResponse(id, -32601, "Method not found");
            }
        }
    } catch (const std::exception& e) {
        return createErrorResponse("", -32603, "Internal error");
    }
}

std::string JsonRpcHandler::createRequest(const std::string& method, std::shared_ptr<JsonValue> params, const std::string& id) {
    auto request = std::make_shared<JsonValue>();
    request->setObject();
    request->setProperty("jsonrpc", std::make_shared<JsonValue>("2.0"));
    request->setProperty("method", std::make_shared<JsonValue>(method));
    if (!id.empty()) {
        request->setProperty("id", std::make_shared<JsonValue>(id));
    }
    if (params) {
        request->setProperty("params", params);
    }
    return request->toString();
}

std::string JsonRpcHandler::createNotification(const std::string& method, std::shared_ptr<JsonValue> params) {
    return createRequest(method, params, ""); // No id for notifications
}

std::string JsonRpcHandler::createResponse(const std::string& id, std::shared_ptr<JsonValue> result) {
    auto response = std::make_shared<JsonValue>();
    response->setObject();
    response->setProperty("jsonrpc", std::make_shared<JsonValue>("2.0"));
    response->setProperty("id", std::make_shared<JsonValue>(id));
    if (result) {
        response->setProperty("result", result);
    } else {
        response->setProperty("result", std::make_shared<JsonValue>()); // null
    }
    return response->toString();
}

std::string JsonRpcHandler::createErrorResponse(const std::string& id, int code, const std::string& message) {
    auto error = std::make_shared<JsonValue>();
    error->setObject();
    error->setProperty("code", std::make_shared<JsonValue>(static_cast<double>(code)));
    error->setProperty("message", std::make_shared<JsonValue>(message));

    auto response = std::make_shared<JsonValue>();
    response->setObject();
    response->setProperty("jsonrpc", std::make_shared<JsonValue>("2.0"));
    response->setProperty("id", std::make_shared<JsonValue>(id));
    response->setProperty("error", error);

    return response->toString();
}

} // namespace lsp
} // namespace bolt
