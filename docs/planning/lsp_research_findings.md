# LSP Research Findings

## LSP Protocol Overview

**Source**: https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/

### Protocol Structure

The Language Server Protocol (LSP) is built on JSON-RPC 2.0 and consists of:

1. **Base Protocol**
   - Header Part (HTTP-like headers)
   - Content Part (JSON-RPC messages)
   - Separated by `\r\n`

2. **Message Types**
   - Request Message (requires response)
   - Response Message (result of request)
   - Notification Message (no response expected)

3. **Base Types**
   - `integer`: -2^31 to 2^31 - 1
   - `uinteger`: 0 to 2^31 - 1
   - `decimal`: floating point numbers
   - `LSPAny`: any LSP type
   - `LSPObject`: key-value pairs
   - `LSPArray`: arrays of LSPAny

### Key Protocol Features (3.17)

- Type hierarchy
- Inline values
- Inlay hints
- Notebook document support
- Meta model description

### Required Message Structure

**Request Message**:
```typescript
interface RequestMessage extends Message {
    id: integer | string;
    method: string;
    params?: array | object;
}
```

**Response Message**:
```typescript
interface ResponseMessage extends Message {
    id: integer | string | null;
    result?: LSPAny;
    error?: ResponseError;
}
```

**Notification Message**:
```typescript
interface NotificationMessage extends Message {
    method: string;
    params?: array | object;
}
```

## Missing Types in bolt-cppml LSP Implementation

Based on compilation errors, the following types are missing from `lsp_protocol.hpp`:

1. **TextDocumentItem** - Represents a text document
2. **VersionedTextDocumentIdentifier** - Document with version number
3. **TextDocumentContentChangeEvent** - Document change event
4. **TextDocumentIdentifier** - Basic document identifier
5. **TextDocumentPositionParams** - Position in a document

These are fundamental LSP types required for document synchronization and position-based operations.

## C++ LSP Implementation Reference

**GitHub**: https://github.com/leon-bckl/lsp-framework

This is a complete C++ implementation of LSP that can be used as reference for:
- Server and client implementation
- Message serialization/deserialization
- Protocol handling
