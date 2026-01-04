#ifndef DOCUMENT_OPERATION_HPP
#define DOCUMENT_OPERATION_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <iostream>
#include <cctype>
#include <algorithm>

// Windows compatibility: undefine conflicting macros
#ifdef _WIN32
#ifdef DELETE
#undef DELETE
#endif
#ifdef INSERT  
#undef INSERT
#endif
#endif

namespace bolt {
namespace collaboration {

enum class OperationType {
    INSERT,
    DELETE,
    CURSOR_MOVE,
    NOOP
};

struct Position {
    size_t line;
    size_t character;
    
    Position(size_t l = 0, size_t c = 0) : line(l), character(c) {}
    
    bool operator==(const Position& other) const {
        return line == other.line && character == other.character;
    }
    
    bool operator<(const Position& other) const {
        if (line != other.line) return line < other.line;
        return character < other.character;
    }
    
    // Convert to linear position in document
    size_t toLinear(const std::vector<std::string>& lines) const {
        size_t pos = 0;
        for (size_t i = 0; i < line && i < lines.size(); ++i) {
            pos += lines[i].length() + 1; // +1 for newline
        }
        if (line < lines.size()) {
            pos += std::min(character, lines[line].length());
        }
        return pos;
    }
    
    // Convert from linear position
    static Position fromLinear(size_t linear, const std::vector<std::string>& lines) {
        size_t pos = 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            size_t lineEnd = pos + lines[i].length();
            if (linear <= lineEnd) {
                return Position(i, linear - pos);
            }
            pos = lineEnd + 1; // +1 for newline
        }
        return Position(lines.size(), 0);
    }
};

class DocumentOperation {
public:
    DocumentOperation(OperationType type, const std::string& userId, 
                     const Position& pos, const std::string& content = "")
        : type_(type), userId_(userId), position_(pos), content_(content),
          timestamp_(std::chrono::steady_clock::now()),
          sequence_(nextSequence()) {}
    
    virtual ~DocumentOperation() = default;
    
    OperationType getType() const { return type_; }
    const std::string& getUserId() const { return userId_; }
    const Position& getPosition() const { return position_; }
    const std::string& getContent() const { return content_; }
    uint64_t getSequence() const { return sequence_; }
    
    void setPosition(const Position& pos) { position_ = pos; }
    
    // Apply operation to document lines
    virtual bool apply(std::vector<std::string>& lines) const {
        switch (type_) {
            case OperationType::INSERT:
                return applyInsert(lines);
            case OperationType::DELETE:
                return applyDelete(lines);
            case OperationType::CURSOR_MOVE:
                return true; // Cursor moves don't modify document
            case OperationType::NOOP:
                return true;
        }
        return false;
    }
    
    // Create inverse operation
    virtual std::unique_ptr<DocumentOperation> createInverse() const {
        switch (type_) {
            case OperationType::INSERT:
                return std::make_unique<DocumentOperation>(
                    OperationType::DELETE, userId_, position_, content_);
            case OperationType::DELETE:
                return std::make_unique<DocumentOperation>(
                    OperationType::INSERT, userId_, position_, content_);
            default:
                return std::make_unique<DocumentOperation>(
                    OperationType::NOOP, userId_, position_);
        }
    }
    
    // Serialize for network transmission
    std::string serialize() const {
        std::string result = "{";
        result += "\"type\":" + std::to_string(static_cast<int>(type_)) + ",";
        result += "\"userId\":\"" + userId_ + "\",";
        result += "\"position\":{\"line\":" + std::to_string(position_.line) + 
                  ",\"character\":" + std::to_string(position_.character) + "},";
        result += "\"content\":\"" + escapeString(content_) + "\",";
        result += "\"sequence\":" + std::to_string(sequence_);
        result += "}";
        return result;
    }
    
    // Deserialize from JSON-like string (simplified)
    static std::unique_ptr<DocumentOperation> deserialize(const std::string& data) {
        // Simplified JSON parsing - in production would use proper JSON library
        auto type = extractInt(data, "type");
        auto userId = extractString(data, "userId");
        auto line = extractInt(data, "position\":{\"line");
        auto character = extractInt(data, "character");
        auto content = extractString(data, "content");
        
        return std::make_unique<DocumentOperation>(
            static_cast<OperationType>(type), userId, Position(line, character), content);
    }

private:
    OperationType type_;
    std::string userId_;
    Position position_;
    std::string content_;
    std::chrono::steady_clock::time_point timestamp_;
    uint64_t sequence_;
    
    static uint64_t nextSequence() {
        static uint64_t counter = 0;
        return ++counter;
    }
    
    bool applyInsert(std::vector<std::string>& lines) const {
        if (position_.line >= lines.size()) {
            // Extend lines if necessary
            lines.resize(position_.line + 1);
        }
        
        std::string& line = lines[position_.line];
        if (position_.character > line.length()) {
            line.resize(position_.character, ' ');
        }
        
        line.insert(position_.character, content_);
        return true;
    }
    
    bool applyDelete(std::vector<std::string>& lines) const {
        if (position_.line >= lines.size()) return false;
        
        std::string& line = lines[position_.line];
        if (position_.character >= line.length()) return false;
        
        size_t deleteLen = std::min(content_.length(), line.length() - position_.character);
        line.erase(position_.character, deleteLen);
        return true;
    }
    
    std::string escapeString(const std::string& str) const {
        std::string result;
        for (char c : str) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else result += c;
        }
        return result;
    }
    
    static int extractInt(const std::string& data, const std::string& key) {
        auto pos = data.find("\"" + key + "\":");
        if (pos == std::string::npos) return 0;
        pos = data.find(":", pos) + 1;
        
        // Skip whitespace
        while (pos < data.length() && std::isspace(data[pos])) pos++;
        
        auto end = data.find_first_of(",}", pos);
        if (end == std::string::npos) end = data.length();
        
        std::string numberStr = data.substr(pos, end - pos);
        
        // Trim whitespace
        numberStr.erase(0, numberStr.find_first_not_of(" \t\n\r"));
        numberStr.erase(numberStr.find_last_not_of(" \t\n\r") + 1);
        
        if (numberStr.empty()) return 0;
        
        try {
            return std::stoi(numberStr);
        } catch (...) {
            return 0;
        }
    }
    
    static std::string extractString(const std::string& data, const std::string& key) {
        auto pos = data.find("\"" + key + "\":\"");
        if (pos == std::string::npos) return "";
        pos = data.find("\":\"", pos) + 3;
        
        std::string result;
        bool escaped = false;
        for (size_t i = pos; i < data.length(); ++i) {
            char c = data[i];
            if (escaped) {
                if (c == 'n') result += '\n';
                else if (c == 'r') result += '\r';
                else if (c == 't') result += '\t';
                else if (c == '"') result += '"';
                else if (c == '\\') result += '\\';
                else result += c;
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                break;
            } else {
                result += c;
            }
        }
        return result;
    }
};

} // namespace collaboration
} // namespace bolt

#endif