#ifndef OPERATIONAL_TRANSFORM_HPP
#define OPERATIONAL_TRANSFORM_HPP

#include "document_operation.hpp"
#include <vector>
#include <memory>
#include <algorithm>

namespace bolt {
namespace collaboration {

class OperationalTransform {
public:
    // Transform operation A against operation B (A was issued before B)
    // Returns transformed version of A that can be applied after B
    static std::unique_ptr<DocumentOperation> transform(
        const DocumentOperation& opA, 
        const DocumentOperation& opB) {
        
        // If operations are from same user, no transformation needed
        if (opA.getUserId() == opB.getUserId()) {
            return std::make_unique<DocumentOperation>(opA);
        }
        
        auto newOpA = std::make_unique<DocumentOperation>(opA);
        
        // Handle different operation type combinations
        switch (opA.getType()) {
            case OperationType::INSERT:
                transformInsert(*newOpA, opB);
                break;
            case OperationType::DELETE:
                transformDelete(*newOpA, opB);
                break;
            case OperationType::CURSOR_MOVE:
                transformCursor(*newOpA, opB);
                break;
            case OperationType::NOOP:
                // No transformation needed for NOOP
                break;
        }
        
        return newOpA;
    }
    
    // Transform a batch of operations against a single operation
    static std::vector<std::unique_ptr<DocumentOperation>> transformBatch(
        const std::vector<std::unique_ptr<DocumentOperation>>& operations,
        const DocumentOperation& against) {
        
        std::vector<std::unique_ptr<DocumentOperation>> transformed;
        transformed.reserve(operations.size());
        
        for (const auto& op : operations) {
            transformed.push_back(transform(*op, against));
        }
        
        return transformed;
    }
    
    // Resolve conflicts when operations arrive out of order
    static std::vector<std::unique_ptr<DocumentOperation>> resolveConflicts(
        std::vector<std::unique_ptr<DocumentOperation>>& operations) {
        
        // Sort operations by sequence number to establish a consistent order
        std::sort(operations.begin(), operations.end(),
            [](const std::unique_ptr<DocumentOperation>& a,
               const std::unique_ptr<DocumentOperation>& b) {
                return a->getSequence() < b->getSequence();
            });
        
        std::vector<std::unique_ptr<DocumentOperation>> resolved;
        resolved.reserve(operations.size());
        
        for (auto& op : operations) {
            // Transform this operation against all previously resolved operations
            auto transformedOp = std::make_unique<DocumentOperation>(*op);
            
            for (const auto& previousOp : resolved) {
                auto temp = transform(*transformedOp, *previousOp);
                transformedOp = std::move(temp);
            }
            
            resolved.push_back(std::move(transformedOp));
        }
        
        return resolved;
    }

private:
    static void transformInsert(DocumentOperation& insert, const DocumentOperation& other) {
        Position insertPos = insert.getPosition();
        Position otherPos = other.getPosition();
        
        switch (other.getType()) {
            case OperationType::INSERT: {
                // If other insert is at same line and before/at current position
                if (otherPos.line == insertPos.line && otherPos.character <= insertPos.character) {
                    insertPos.character += other.getContent().length();
                    insert.setPosition(insertPos);
                }
                // If other insert is on previous line
                else if (otherPos.line < insertPos.line) {
                    // Check if the insert contains newlines
                    size_t newlineCount = std::count(other.getContent().begin(), 
                                                   other.getContent().end(), '\n');
                    if (newlineCount > 0) {
                        insertPos.line += newlineCount;
                        insert.setPosition(insertPos);
                    }
                }
                break;
            }
            case OperationType::DELETE: {
                // If delete is at same line and before current position
                if (otherPos.line == insertPos.line && otherPos.character < insertPos.character) {
                    size_t deleteLen = std::min(other.getContent().length(),
                                              insertPos.character - otherPos.character);
                    insertPos.character -= deleteLen;
                    insert.setPosition(insertPos);
                }
                // If delete is on previous line
                else if (otherPos.line < insertPos.line) {
                    // Check if the delete removes newlines
                    size_t newlineCount = std::count(other.getContent().begin(),
                                                   other.getContent().end(), '\n');
                    if (newlineCount > 0) {
                        insertPos.line -= std::min(newlineCount, insertPos.line);
                        insert.setPosition(insertPos);
                    }
                }
                break;
            }
            case OperationType::CURSOR_MOVE:
            case OperationType::NOOP:
                // No transformation needed
                break;
        }
    }
    
    static void transformDelete(DocumentOperation& delete_op, const DocumentOperation& other) {
        Position deletePos = delete_op.getPosition();
        Position otherPos = other.getPosition();
        
        switch (other.getType()) {
            case OperationType::INSERT: {
                // If insert is at same line and before/at current position
                if (otherPos.line == deletePos.line && otherPos.character <= deletePos.character) {
                    deletePos.character += other.getContent().length();
                    delete_op.setPosition(deletePos);
                }
                // If insert is on previous line with newlines
                else if (otherPos.line < deletePos.line) {
                    size_t newlineCount = std::count(other.getContent().begin(),
                                                   other.getContent().end(), '\n');
                    if (newlineCount > 0) {
                        deletePos.line += newlineCount;
                        delete_op.setPosition(deletePos);
                    }
                }
                break;
            }
            case OperationType::DELETE: {
                // If other delete is at same line and before current position
                if (otherPos.line == deletePos.line && otherPos.character < deletePos.character) {
                    size_t deleteLen = std::min(other.getContent().length(),
                                              deletePos.character - otherPos.character);
                    deletePos.character -= deleteLen;
                    delete_op.setPosition(deletePos);
                }
                // If delete is on previous line with newlines
                else if (otherPos.line < deletePos.line) {
                    size_t newlineCount = std::count(other.getContent().begin(),
                                                   other.getContent().end(), '\n');
                    if (newlineCount > 0) {
                        deletePos.line -= std::min(newlineCount, deletePos.line);
                        delete_op.setPosition(deletePos);
                    }
                }
                break;
            }
            case OperationType::CURSOR_MOVE:
            case OperationType::NOOP:
                // No transformation needed
                break;
        }
    }
    
    static void transformCursor(DocumentOperation& cursor, const DocumentOperation& other) {
        Position cursorPos = cursor.getPosition();
        Position otherPos = other.getPosition();
        
        switch (other.getType()) {
            case OperationType::INSERT: {
                // If insert is at same line and before/at cursor position
                if (otherPos.line == cursorPos.line && otherPos.character <= cursorPos.character) {
                    cursorPos.character += other.getContent().length();
                    cursor.setPosition(cursorPos);
                }
                // If insert is on previous line with newlines
                else if (otherPos.line < cursorPos.line) {
                    size_t newlineCount = std::count(other.getContent().begin(),
                                                   other.getContent().end(), '\n');
                    if (newlineCount > 0) {
                        cursorPos.line += newlineCount;
                        cursor.setPosition(cursorPos);
                    }
                }
                break;
            }
            case OperationType::DELETE: {
                // If delete is at same line and before cursor position
                if (otherPos.line == cursorPos.line && otherPos.character < cursorPos.character) {
                    size_t deleteLen = std::min(other.getContent().length(),
                                              cursorPos.character - otherPos.character);
                    cursorPos.character -= deleteLen;
                    cursor.setPosition(cursorPos);
                }
                // If delete is on previous line with newlines
                else if (otherPos.line < cursorPos.line) {
                    size_t newlineCount = std::count(other.getContent().begin(),
                                                   other.getContent().end(), '\n');
                    if (newlineCount > 0) {
                        cursorPos.line -= std::min(newlineCount, cursorPos.line);
                        cursor.setPosition(cursorPos);
                    }
                }
                break;
            }
            case OperationType::CURSOR_MOVE:
            case OperationType::NOOP:
                // No transformation needed
                break;
        }
    }
};

} // namespace collaboration
} // namespace bolt

#endif