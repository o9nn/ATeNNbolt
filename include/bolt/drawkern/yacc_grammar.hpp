#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <variant>

namespace bolt {
namespace drawkern {

// Forward declarations
struct DISProgram;
struct VMGlyph;
struct AIWorkbenchGlyph;

// Yacc Grammar System for VM Glyph Description Language
// This provides a formal grammar for describing VM topologies and rendering

// AST Node types for the glyph description language
enum class ASTNodeType {
    PROGRAM,      // Root program node
    VM_DECL,      // VM declaration
    GLYPH_DECL,   // Glyph declaration  
    PROPERTY,     // Property assignment
    BLOCK,        // Block statement
    EXPRESSION,   // Expression
    IDENTIFIER,   // Identifier
    LITERAL,      // Literal value
    LIST,         // List of items
    CALL          // Function call
};

// Token types for lexical analysis
enum class TokenType {
    // Keywords
    VM, GLYPH, WORKBENCH, DIS, NATIVE, WASM,
    RENDER, AI, MODEL, TOOLS, NAMESPACE,
    
    // Operators and punctuation
    ASSIGN,       // =
    SEMICOLON,    // ;
    COMMA,        // ,
    LBRACE,       // {
    RBRACE,       // }
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    
    // Literals
    IDENTIFIER,
    STRING,
    NUMBER,
    
    // Special
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType t, const std::string& v, size_t l = 0, size_t c = 0) 
        : type(t), value(v), line(l), column(c) {}
};

// AST Node base class
class ASTNode {
public:
    ASTNodeType type;
    std::vector<std::unique_ptr<ASTNode>> children;
    std::map<std::string, std::string> attributes;
    
    ASTNode(ASTNodeType t) : type(t) {}
    virtual ~ASTNode() = default;
    
    void add_child(std::unique_ptr<ASTNode> child) {
        children.push_back(std::move(child));
    }
    
    void set_attribute(const std::string& key, const std::string& value) {
        attributes[key] = value;
    }
    
    std::string get_attribute(const std::string& key) const {
        auto it = attributes.find(key);
        return it != attributes.end() ? it->second : "";
    }
    
    virtual std::string to_string() const;
};

// Specialized AST node types
class ProgramNode : public ASTNode {
public:
    ProgramNode() : ASTNode(ASTNodeType::PROGRAM) {}
};

class VMDeclNode : public ASTNode {
public:
    std::string vm_id;
    std::string vm_type;
    
    VMDeclNode(const std::string& id, const std::string& type) 
        : ASTNode(ASTNodeType::VM_DECL), vm_id(id), vm_type(type) {}
};

class GlyphDeclNode : public ASTNode {
public:
    std::string glyph_id;
    std::string base_vm;
    
    GlyphDeclNode(const std::string& id, const std::string& vm) 
        : ASTNode(ASTNodeType::GLYPH_DECL), glyph_id(id), base_vm(vm) {}
};

class PropertyNode : public ASTNode {
public:
    std::string name;
    std::string value;
    
    PropertyNode(const std::string& n, const std::string& v) 
        : ASTNode(ASTNodeType::PROPERTY), name(n), value(v) {}
};

// Lexer for tokenizing glyph description language
class GlyphLexer {
public:
    GlyphLexer(const std::string& input);
    ~GlyphLexer();
    
    std::vector<Token> tokenize();
    Token next_token();
    
    bool has_errors() const { return !errors_.empty(); }
    std::vector<std::string> get_errors() const { return errors_; }
    
private:
    std::string input_;
    size_t pos_;
    size_t line_;
    size_t column_;
    std::vector<std::string> errors_;
    
    char current_char();
    char peek_char();
    void advance();
    void skip_whitespace();
    void skip_comment();
    
    Token read_identifier();
    Token read_string();
    Token read_number();
    
    bool is_alpha(char c);
    bool is_digit(char c);
    bool is_alnum(char c);
    
    void add_error(const std::string& message);
};

// Parser for glyph description language
class GlyphParser {
public:
    GlyphParser(const std::vector<Token>& tokens);
    ~GlyphParser();
    
    std::unique_ptr<ProgramNode> parse();
    
    bool has_errors() const { return !errors_.empty(); }
    std::vector<std::string> get_errors() const { return errors_; }
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    std::vector<std::string> errors_;
    
    Token current_token();
    Token peek_token();
    void advance();
    bool match(TokenType type);
    bool consume(TokenType type);
    
    std::unique_ptr<ASTNode> parse_statement();
    std::unique_ptr<VMDeclNode> parse_vm_declaration();
    std::unique_ptr<GlyphDeclNode> parse_glyph_declaration();
    std::unique_ptr<PropertyNode> parse_property();
    std::unique_ptr<ASTNode> parse_block();
    std::unique_ptr<ASTNode> parse_expression();
    std::unique_ptr<ASTNode> parse_list();
    
    void add_error(const std::string& message);
};

// Code generator - converts AST to executable VM glyphs
class GlyphCodeGenerator {
public:
    GlyphCodeGenerator();
    ~GlyphCodeGenerator();
    
    // Generate VM glyph from AST
    VMGlyph generate_vm_glyph(const ASTNode& ast);
    AIWorkbenchGlyph generate_ai_workbench(const ASTNode& ast);
    DISProgram generate_dis_program(const ASTNode& ast);
    
    // Generate different output formats
    std::string generate_cpp_code(const ASTNode& ast);
    std::string generate_json(const ASTNode& ast);
    std::string generate_yaml(const ASTNode& ast);
    
    bool has_errors() const { return !errors_.empty(); }
    std::vector<std::string> get_errors() const { return errors_; }
    
private:
    std::vector<std::string> errors_;
    std::map<std::string, VMGlyph> vm_registry_;
    
    void add_error(const std::string& message);
    void process_vm_declaration(const VMDeclNode& node);
    void process_glyph_declaration(const GlyphDeclNode& node);
    void process_properties(const ASTNode& node, std::map<std::string, std::string>& props);
};

// Grammar specification (BNF-like notation in comments)
/*
Grammar for VM Glyph Description Language:

program      ::= declaration*

declaration  ::= vm_decl | glyph_decl

vm_decl      ::= 'vm' IDENTIFIER ':' vm_type '{' property* '}'
vm_type      ::= 'dis' | 'native' | 'wasm'

glyph_decl   ::= 'glyph' IDENTIFIER 'on' IDENTIFIER '{' glyph_body '}'
glyph_body   ::= property* render_spec? ai_spec?

property     ::= IDENTIFIER '=' value ';'
value        ::= STRING | NUMBER | IDENTIFIER | list

list         ::= '[' (value (',' value)*)? ']'

render_spec  ::= 'render' '{' render_prop* '}'
render_prop  ::= ('width' | 'height' | 'background' | 'font' | 'interactive') '=' value ';'

ai_spec      ::= 'ai' '{' ai_prop* '}'
ai_prop      ::= ('model' | 'tools' | 'context') '=' value ';'

workbench    ::= 'workbench' IDENTIFIER 'on' IDENTIFIER '{' workbench_body '}'
workbench_body ::= property* render_spec? ai_spec? namespace_spec?

namespace_spec ::= 'namespace' '{' mount* '}'
mount        ::= 'mount' STRING 'as' STRING ';'
*/

// High-level interface for the grammar system
class YaccGrammarSystem {
public:
    YaccGrammarSystem();
    ~YaccGrammarSystem();
    
    // Parse glyph description and generate VM
    VMGlyph parse_vm_glyph(const std::string& description);
    AIWorkbenchGlyph parse_ai_workbench(const std::string& description);
    
    // Validate glyph description
    bool validate_description(const std::string& description);
    std::vector<std::string> get_validation_errors() const;
    
    // Generate code from description
    std::string generate_cpp_code(const std::string& description);
    std::string generate_deployment_script(const std::string& description);
    
    // Built-in templates
    std::string get_ai_workbench_template();
    std::string get_file_server_template();
    std::string get_echo_server_template();
    
    // Grammar introspection
    std::vector<std::string> get_supported_vm_types();
    std::vector<std::string> get_supported_properties();
    std::string get_grammar_documentation();
    
private:
    std::unique_ptr<GlyphLexer> lexer_;
    std::unique_ptr<GlyphParser> parser_;
    std::unique_ptr<GlyphCodeGenerator> generator_;
    std::vector<std::string> errors_;
    
    void clear_errors();
    void collect_errors();
};

// Example glyph descriptions as string constants
namespace GlyphTemplates {
    
    constexpr const char* AI_WORKBENCH = R"(
vm bolt_dis : dis {
    architecture = "portable";
    capabilities = ["ai-inference", "code-parsing", "file-io"];
}

workbench bolt_ai on bolt_dis {
    model = "ggml-rwkv";
    tools = ["completion", "chat", "analysis", "refactoring"];
    
    render {
        width = 1200;
        height = 800;
        background = "#1e1e1e";
        font = "Fira Code";
        interactive = true;
    }
    
    ai {
        model = "rwkv-14b";
        context = 8192;
        temperature = 0.7;
    }
    
    namespace {
        mount "/ai/models" as "/srv/models";
        mount "/ai/cache" as "/tmp/ai_cache";
    }
}
)";

    constexpr const char* FILE_SERVER = R"(
vm file_server_vm : dis {
    architecture = "arm64";
    capabilities = ["file-io", "network"];
}

glyph file_server on file_server_vm {
    root_path = "/srv/files";
    port = 9999;
    
    render {
        width = 600;
        height = 400;
        background = "#2d2d2d";
    }
}
)";

    constexpr const char* ECHO_SERVER = R"(
vm echo_vm : native {
    architecture = "x86_64";
    capabilities = ["network"];
}

glyph echo_server on echo_vm {
    port = 8080;
    buffer_size = 4096;
    
    render {
        width = 400;
        height = 300;
    }
}
)";

} // namespace GlyphTemplates

} // namespace drawkern
} // namespace bolt