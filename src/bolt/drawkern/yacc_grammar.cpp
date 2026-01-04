#include "bolt/drawkern/yacc_grammar.hpp"
#include "bolt/drawkern/drawkern.hpp"
#include "bolt/drawkern/dis_vm.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace bolt {
namespace drawkern {

// ASTNode implementation
std::string ASTNode::to_string() const {
    std::string result;
    switch (type) {
        case ASTNodeType::PROGRAM: result = "Program"; break;
        case ASTNodeType::VM_DECL: result = "VMDecl"; break;
        case ASTNodeType::GLYPH_DECL: result = "GlyphDecl"; break;
        case ASTNodeType::PROPERTY: result = "Property"; break;
        case ASTNodeType::BLOCK: result = "Block"; break;
        case ASTNodeType::EXPRESSION: result = "Expression"; break;
        case ASTNodeType::IDENTIFIER: result = "Identifier"; break;
        case ASTNodeType::LITERAL: result = "Literal"; break;
        case ASTNodeType::LIST: result = "List"; break;
        case ASTNodeType::CALL: result = "Call"; break;
    }
    
    if (!attributes.empty()) {
        result += " {";
        for (const auto& [key, value] : attributes) {
            result += " " + key + "=" + value;
        }
        result += " }";
    }
    
    return result;
}

// GlyphLexer implementation
GlyphLexer::GlyphLexer(const std::string& input) 
    : input_(input), pos_(0), line_(1), column_(1) {
}

GlyphLexer::~GlyphLexer() {
}

std::vector<Token> GlyphLexer::tokenize() {
    std::vector<Token> tokens;
    errors_.clear();
    
    while (pos_ < input_.length()) {
        Token token = next_token();
        if (token.type != TokenType::UNKNOWN) {
            tokens.push_back(token);
        }
        
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    
    return tokens;
}

Token GlyphLexer::next_token() {
    skip_whitespace();
    
    if (pos_ >= input_.length()) {
        return Token(TokenType::END_OF_FILE, "", line_, column_);
    }
    
    char ch = current_char();
    
    // Comments
    if (ch == '/' && peek_char() == '/') {
        skip_comment();
        return next_token();
    }
    
    // Single character tokens
    switch (ch) {
        case '=': advance(); return Token(TokenType::ASSIGN, "=", line_, column_ - 1);
        case ';': advance(); return Token(TokenType::SEMICOLON, ";", line_, column_ - 1);
        case ',': advance(); return Token(TokenType::COMMA, ",", line_, column_ - 1);
        case '{': advance(); return Token(TokenType::LBRACE, "{", line_, column_ - 1);
        case '}': advance(); return Token(TokenType::RBRACE, "}", line_, column_ - 1);
        case '(': advance(); return Token(TokenType::LPAREN, "(", line_, column_ - 1);
        case ')': advance(); return Token(TokenType::RPAREN, ")", line_, column_ - 1);
        case '[': advance(); return Token(TokenType::LBRACKET, "[", line_, column_ - 1);
        case ']': advance(); return Token(TokenType::RBRACKET, "]", line_, column_ - 1);
    }
    
    // String literals
    if (ch == '"') {
        return read_string();
    }
    
    // Numbers
    if (is_digit(ch)) {
        return read_number();
    }
    
    // Identifiers and keywords
    if (is_alpha(ch) || ch == '_') {
        return read_identifier();
    }
    
    add_error("Unexpected character: " + std::string(1, ch));
    advance();
    return Token(TokenType::UNKNOWN, std::string(1, ch), line_, column_ - 1);
}

char GlyphLexer::current_char() {
    if (pos_ >= input_.length()) return '\0';
    return input_[pos_];
}

char GlyphLexer::peek_char() {
    if (pos_ + 1 >= input_.length()) return '\0';
    return input_[pos_ + 1];
}

void GlyphLexer::advance() {
    if (pos_ < input_.length()) {
        if (input_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

void GlyphLexer::skip_whitespace() {
    while (pos_ < input_.length() && std::isspace(current_char())) {
        advance();
    }
}

void GlyphLexer::skip_comment() {
    while (pos_ < input_.length() && current_char() != '\n') {
        advance();
    }
}

Token GlyphLexer::read_identifier() {
    size_t start_line = line_;
    size_t start_column = column_;
    std::string value;
    
    while (pos_ < input_.length() && (is_alnum(current_char()) || current_char() == '_')) {
        value += current_char();
        advance();
    }
    
    // Check for keywords
    TokenType type = TokenType::IDENTIFIER;
    if (value == "vm") type = TokenType::VM;
    else if (value == "glyph") type = TokenType::GLYPH;
    else if (value == "workbench") type = TokenType::WORKBENCH;
    else if (value == "dis") type = TokenType::DIS;
    else if (value == "native") type = TokenType::NATIVE;
    else if (value == "wasm") type = TokenType::WASM;
    else if (value == "render") type = TokenType::RENDER;
    else if (value == "ai") type = TokenType::AI;
    else if (value == "model") type = TokenType::MODEL;
    else if (value == "tools") type = TokenType::TOOLS;
    else if (value == "namespace") type = TokenType::NAMESPACE;
    
    return Token(type, value, start_line, start_column);
}

Token GlyphLexer::read_string() {
    size_t start_line = line_;
    size_t start_column = column_;
    std::string value;
    
    advance(); // Skip opening quote
    
    while (pos_ < input_.length() && current_char() != '"') {
        if (current_char() == '\\' && peek_char() != '\0') {
            advance();
            char escaped = current_char();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                default: value += escaped; break;
            }
        } else {
            value += current_char();
        }
        advance();
    }
    
    if (current_char() == '"') {
        advance(); // Skip closing quote
    } else {
        add_error("Unterminated string literal");
    }
    
    return Token(TokenType::STRING, value, start_line, start_column);
}

Token GlyphLexer::read_number() {
    size_t start_line = line_;
    size_t start_column = column_;
    std::string value;
    
    while (pos_ < input_.length() && (is_digit(current_char()) || current_char() == '.')) {
        value += current_char();
        advance();
    }
    
    return Token(TokenType::NUMBER, value, start_line, start_column);
}

bool GlyphLexer::is_alpha(char c) {
    return std::isalpha(c);
}

bool GlyphLexer::is_digit(char c) {
    return std::isdigit(c);
}

bool GlyphLexer::is_alnum(char c) {
    return std::isalnum(c);
}

void GlyphLexer::add_error(const std::string& message) {
    errors_.push_back("Line " + std::to_string(line_) + ", Column " + std::to_string(column_) + ": " + message);
}

// GlyphParser implementation
GlyphParser::GlyphParser(const std::vector<Token>& tokens) 
    : tokens_(tokens), pos_(0) {
}

GlyphParser::~GlyphParser() {
}

std::unique_ptr<ProgramNode> GlyphParser::parse() {
    errors_.clear();
    pos_ = 0;
    
    auto program = std::make_unique<ProgramNode>();
    
    while (pos_ < tokens_.size() && current_token().type != TokenType::END_OF_FILE) {
        auto statement = parse_statement();
        if (statement) {
            program->add_child(std::move(statement));
        }
    }
    
    return program;
}

Token GlyphParser::current_token() {
    if (pos_ >= tokens_.size()) {
        return Token(TokenType::END_OF_FILE, "", 0, 0);
    }
    return tokens_[pos_];
}

Token GlyphParser::peek_token() {
    if (pos_ + 1 >= tokens_.size()) {
        return Token(TokenType::END_OF_FILE, "", 0, 0);
    }
    return tokens_[pos_ + 1];
}

void GlyphParser::advance() {
    if (pos_ < tokens_.size()) {
        pos_++;
    }
}

bool GlyphParser::match(TokenType type) {
    return current_token().type == type;
}

bool GlyphParser::consume(TokenType type) {
    if (match(type)) {
        advance();
        return true;
    }
    return false;
}

std::unique_ptr<ASTNode> GlyphParser::parse_statement() {
    if (match(TokenType::VM)) {
        return parse_vm_declaration();
    } else if (match(TokenType::GLYPH) || match(TokenType::WORKBENCH)) {
        return parse_glyph_declaration();
    } else if (match(TokenType::IDENTIFIER)) {
        return parse_property();
    } else {
        add_error("Expected statement, got " + current_token().value);
        advance();
        return nullptr;
    }
}

std::unique_ptr<VMDeclNode> GlyphParser::parse_vm_declaration() {
    if (!consume(TokenType::VM)) {
        return nullptr;
    }
    
    if (!match(TokenType::IDENTIFIER)) {
        add_error("Expected VM name");
        return nullptr;
    }
    
    std::string vm_id = current_token().value;
    advance();
    
    if (!consume(TokenType::ASSIGN)) {
        add_error("Expected ':' after VM name");
        return nullptr;
    }
    
    std::string vm_type;
    if (match(TokenType::DIS)) {
        vm_type = "dis";
        advance();
    } else if (match(TokenType::NATIVE)) {
        vm_type = "native";
        advance();
    } else if (match(TokenType::WASM)) {
        vm_type = "wasm";
        advance();
    } else {
        add_error("Expected VM type (dis, native, wasm)");
        return nullptr;
    }
    
    auto vm_node = std::make_unique<VMDeclNode>(vm_id, vm_type);
    
    if (consume(TokenType::LBRACE)) {
        auto block = parse_block();
        if (block) {
            vm_node->add_child(std::move(block));
        }
    }
    
    return vm_node;
}

std::unique_ptr<GlyphDeclNode> GlyphParser::parse_glyph_declaration() {
    bool is_workbench = match(TokenType::WORKBENCH);
    
    if (!(consume(TokenType::GLYPH) || consume(TokenType::WORKBENCH))) {
        return nullptr;
    }
    
    if (!match(TokenType::IDENTIFIER)) {
        add_error("Expected glyph name");
        return nullptr;
    }
    
    std::string glyph_id = current_token().value;
    advance();
    
    // Expect 'on' keyword (simplified - using identifier)
    if (!match(TokenType::IDENTIFIER) || current_token().value != "on") {
        add_error("Expected 'on' after glyph name");
        return nullptr;
    }
    advance();
    
    if (!match(TokenType::IDENTIFIER)) {
        add_error("Expected VM name after 'on'");
        return nullptr;
    }
    
    std::string base_vm = current_token().value;
    advance();
    
    auto glyph_node = std::make_unique<GlyphDeclNode>(glyph_id, base_vm);
    if (is_workbench) {
        glyph_node->set_attribute("type", "workbench");
    }
    
    if (consume(TokenType::LBRACE)) {
        auto block = parse_block();
        if (block) {
            glyph_node->add_child(std::move(block));
        }
    }
    
    return glyph_node;
}

std::unique_ptr<PropertyNode> GlyphParser::parse_property() {
    if (!match(TokenType::IDENTIFIER)) {
        return nullptr;
    }
    
    std::string name = current_token().value;
    advance();
    
    if (!consume(TokenType::ASSIGN)) {
        add_error("Expected '=' after property name");
        return nullptr;
    }
    
    std::string value;
    if (match(TokenType::STRING) || match(TokenType::NUMBER) || match(TokenType::IDENTIFIER)) {
        value = current_token().value;
        advance();
    } else if (match(TokenType::LBRACKET)) {
        // Handle list values (simplified)
        value = "[";
        advance();
        while (!match(TokenType::RBRACKET) && !match(TokenType::END_OF_FILE)) {
            value += current_token().value;
            advance();
            if (match(TokenType::COMMA)) {
                value += ",";
                advance();
            }
        }
        if (consume(TokenType::RBRACKET)) {
            value += "]";
        }
    } else {
        add_error("Expected property value");
        return nullptr;
    }
    
    consume(TokenType::SEMICOLON); // Optional semicolon
    
    return std::make_unique<PropertyNode>(name, value);
}

std::unique_ptr<ASTNode> GlyphParser::parse_block() {
    auto block = std::make_unique<ASTNode>(ASTNodeType::BLOCK);
    
    while (!match(TokenType::RBRACE) && !match(TokenType::END_OF_FILE)) {
        auto statement = parse_statement();
        if (statement) {
            block->add_child(std::move(statement));
        } else {
            advance(); // Skip problematic tokens
        }
    }
    
    consume(TokenType::RBRACE);
    return block;
}

std::unique_ptr<ASTNode> GlyphParser::parse_expression() {
    // Simplified expression parsing
    auto expr = std::make_unique<ASTNode>(ASTNodeType::EXPRESSION);
    
    if (match(TokenType::IDENTIFIER) || match(TokenType::STRING) || match(TokenType::NUMBER)) {
        expr->set_attribute("value", current_token().value);
        advance();
    }
    
    return expr;
}

std::unique_ptr<ASTNode> GlyphParser::parse_list() {
    auto list = std::make_unique<ASTNode>(ASTNodeType::LIST);
    
    if (!consume(TokenType::LBRACKET)) {
        return nullptr;
    }
    
    while (!match(TokenType::RBRACKET) && !match(TokenType::END_OF_FILE)) {
        auto expr = parse_expression();
        if (expr) {
            list->add_child(std::move(expr));
        }
        
        if (!consume(TokenType::COMMA)) {
            break;
        }
    }
    
    consume(TokenType::RBRACKET);
    return list;
}

void GlyphParser::add_error(const std::string& message) {
    Token token = current_token();
    errors_.push_back("Line " + std::to_string(token.line) + ", Column " + std::to_string(token.column) + ": " + message);
}

// GlyphCodeGenerator implementation
GlyphCodeGenerator::GlyphCodeGenerator() {
}

GlyphCodeGenerator::~GlyphCodeGenerator() {
}

VMGlyph GlyphCodeGenerator::generate_vm_glyph(const ASTNode& ast) {
    errors_.clear();
    VMGlyph glyph;
    
    // Set defaults
    glyph.vm_type = "dis";
    glyph.architecture = "portable";
    glyph.render.width = 800;
    glyph.render.height = 600;
    glyph.render.background_color = 0x1e1e1e;
    glyph.render.interactive = true;
    glyph.render.ai_enabled = false;
    
    // Process AST nodes
    for (const auto& child : ast.children) {
        if (child->type == ASTNodeType::VM_DECL) {
            const VMDeclNode* vm_decl = static_cast<const VMDeclNode*>(child.get());
            glyph.vm_type = vm_decl->vm_type;
        } else if (child->type == ASTNodeType::GLYPH_DECL) {
            // Process glyph properties
            for (const auto& grandchild : child->children) {
                if (grandchild->type == ASTNodeType::BLOCK) {
                    for (const auto& prop_child : grandchild->children) {
                        if (prop_child->type == ASTNodeType::PROPERTY) {
                            const PropertyNode* prop = static_cast<const PropertyNode*>(prop_child.get());
                            
                            if (prop->name == "width") {
                                glyph.render.width = std::stoi(prop->value);
                            } else if (prop->name == "height") {
                                glyph.render.height = std::stoi(prop->value);
                            } else if (prop->name == "architecture") {
                                glyph.architecture = prop->value;
                            } else if (prop->name == "interactive") {
                                glyph.render.interactive = (prop->value == "true");
                            }
                        }
                    }
                }
            }
        }
    }
    
    return glyph;
}

AIWorkbenchGlyph GlyphCodeGenerator::generate_ai_workbench(const ASTNode& ast) {
    AIWorkbenchGlyph workbench;
    
    workbench.workbench_id = "generated_workbench";
    workbench.ai_model = "ggml-rwkv";
    workbench.tools = {"completion", "chat"};
    workbench.styx_address = "tcp!*!9999";
    
    // Generate host VM
    workbench.host_vm = generate_vm_glyph(ast);
    workbench.host_vm.render.ai_enabled = true;
    
    return workbench;
}

DISProgram GlyphCodeGenerator::generate_dis_program(const ASTNode& ast) {
    DISProgram program;
    
    // Generate a simple AI workbench program
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("🤖 AI Workbench starting...\n")));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("ggml-rwkv")));
    program.add_instruction(DISInstruction(DISOpcode::AI_INIT));
    
    program.add_instruction(DISInstruction(DISOpcode::NEWSTR, DISValue("Hello from generated workbench!")));
    program.add_instruction(DISInstruction(DISOpcode::AI_CHAT));
    program.add_instruction(DISInstruction(DISOpcode::PRINT));
    
    program.add_instruction(DISInstruction(DISOpcode::HALT));
    
    return program;
}

std::string GlyphCodeGenerator::generate_cpp_code(const ASTNode& ast) {
    std::ostringstream code;
    
    code << "#include \"bolt/drawkern/drawkern.hpp\"\n";
    code << "#include \"bolt/drawkern/dis_vm.hpp\"\n\n";
    code << "using namespace bolt::drawkern;\n\n";
    code << "AIWorkbenchGlyph create_generated_workbench() {\n";
    code << "    AIWorkbenchGlyph workbench;\n";
    code << "    workbench.workbench_id = \"generated_workbench\";\n";
    code << "    workbench.ai_model = \"ggml-rwkv\";\n";
    code << "    workbench.tools = {\"completion\", \"chat\"};\n";
    code << "    \n";
    code << "    // Configure VM\n";
    code << "    workbench.host_vm.vm_type = \"dis\";\n";
    code << "    workbench.host_vm.architecture = \"portable\";\n";
    code << "    workbench.host_vm.render.width = 1200;\n";
    code << "    workbench.host_vm.render.height = 800;\n";
    code << "    workbench.host_vm.render.ai_enabled = true;\n";
    code << "    \n";
    code << "    return workbench;\n";
    code << "}\n";
    
    return code.str();
}

std::string GlyphCodeGenerator::generate_json(const ASTNode& ast) {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"workbench_id\": \"generated_workbench\",\n";
    json << "  \"ai_model\": \"ggml-rwkv\",\n";
    json << "  \"tools\": [\"completion\", \"chat\"],\n";
    json << "  \"host_vm\": {\n";
    json << "    \"vm_type\": \"dis\",\n";
    json << "    \"architecture\": \"portable\",\n";
    json << "    \"render\": {\n";
    json << "      \"width\": 1200,\n";
    json << "      \"height\": 800,\n";
    json << "      \"ai_enabled\": true\n";
    json << "    }\n";
    json << "  }\n";
    json << "}\n";
    
    return json.str();
}

std::string GlyphCodeGenerator::generate_yaml(const ASTNode& ast) {
    std::ostringstream yaml;
    
    yaml << "workbench_id: generated_workbench\n";
    yaml << "ai_model: ggml-rwkv\n";
    yaml << "tools:\n";
    yaml << "  - completion\n";
    yaml << "  - chat\n";
    yaml << "host_vm:\n";
    yaml << "  vm_type: dis\n";
    yaml << "  architecture: portable\n";
    yaml << "  render:\n";
    yaml << "    width: 1200\n";
    yaml << "    height: 800\n";
    yaml << "    ai_enabled: true\n";
    
    return yaml.str();
}

void GlyphCodeGenerator::add_error(const std::string& message) {
    errors_.push_back(message);
}

// YaccGrammarSystem implementation
YaccGrammarSystem::YaccGrammarSystem() {
}

YaccGrammarSystem::~YaccGrammarSystem() {
}

VMGlyph YaccGrammarSystem::parse_vm_glyph(const std::string& description) {
    clear_errors();
    
    lexer_ = std::make_unique<GlyphLexer>(description);
    auto tokens = lexer_->tokenize();
    
    parser_ = std::make_unique<GlyphParser>(tokens);
    auto ast = parser_->parse();
    
    generator_ = std::make_unique<GlyphCodeGenerator>();
    VMGlyph glyph = generator_->generate_vm_glyph(*ast);
    
    collect_errors();
    return glyph;
}

AIWorkbenchGlyph YaccGrammarSystem::parse_ai_workbench(const std::string& description) {
    clear_errors();
    
    lexer_ = std::make_unique<GlyphLexer>(description);
    auto tokens = lexer_->tokenize();
    
    parser_ = std::make_unique<GlyphParser>(tokens);
    auto ast = parser_->parse();
    
    generator_ = std::make_unique<GlyphCodeGenerator>();
    AIWorkbenchGlyph workbench = generator_->generate_ai_workbench(*ast);
    
    collect_errors();
    return workbench;
}

bool YaccGrammarSystem::validate_description(const std::string& description) {
    clear_errors();
    
    lexer_ = std::make_unique<GlyphLexer>(description);
    auto tokens = lexer_->tokenize();
    
    parser_ = std::make_unique<GlyphParser>(tokens);
    auto ast = parser_->parse();
    
    collect_errors();
    return errors_.empty();
}

std::vector<std::string> YaccGrammarSystem::get_validation_errors() const {
    return errors_;
}

std::string YaccGrammarSystem::generate_cpp_code(const std::string& description) {
    clear_errors();
    
    lexer_ = std::make_unique<GlyphLexer>(description);
    auto tokens = lexer_->tokenize();
    
    parser_ = std::make_unique<GlyphParser>(tokens);
    auto ast = parser_->parse();
    
    generator_ = std::make_unique<GlyphCodeGenerator>();
    std::string code = generator_->generate_cpp_code(*ast);
    
    collect_errors();
    return code;
}

std::string YaccGrammarSystem::generate_deployment_script(const std::string& description) {
    std::ostringstream script;
    
    script << "#!/bin/bash\n";
    script << "# Generated DrawKern deployment script\n\n";
    script << "echo \"Deploying DrawKern workbench...\"\n";
    script << "./drawkern_server --port 9999 &\n";
    script << "sleep 2\n";
    script << "./drawkern_client localhost:9999\n";
    
    return script.str();
}

std::string YaccGrammarSystem::get_ai_workbench_template() {
    return GlyphTemplates::AI_WORKBENCH;
}

std::string YaccGrammarSystem::get_file_server_template() {
    return GlyphTemplates::FILE_SERVER;
}

std::string YaccGrammarSystem::get_echo_server_template() {
    return GlyphTemplates::ECHO_SERVER;
}

std::vector<std::string> YaccGrammarSystem::get_supported_vm_types() {
    return {"dis", "native", "wasm"};
}

std::vector<std::string> YaccGrammarSystem::get_supported_properties() {
    return {"width", "height", "background", "font", "interactive", "ai_enabled", 
            "model", "tools", "architecture", "capabilities"};
}

std::string YaccGrammarSystem::get_grammar_documentation() {
    return R"(
DrawKern VM Glyph Description Language Grammar:

Keywords:
  vm, glyph, workbench, dis, native, wasm, render, ai, model, tools, namespace

Basic Structure:
  vm <name> : <type> { properties... }
  glyph <name> on <vm> { properties... }
  workbench <name> on <vm> { properties... render { ... } ai { ... } }

Properties:
  width = <number>;
  height = <number>;
  background = "<color>";
  font = "<font_name>";
  interactive = true|false;
  ai_enabled = true|false;
  model = "<model_name>";
  tools = ["tool1", "tool2"];
  architecture = "<arch>";

Example:
  vm my_vm : dis {
    architecture = "portable";
  }
  
  workbench my_workbench on my_vm {
    render { width = 1200; height = 800; }
    ai { model = "ggml-rwkv"; }
  }
)";
}

void YaccGrammarSystem::clear_errors() {
    errors_.clear();
}

void YaccGrammarSystem::collect_errors() {
    if (lexer_ && lexer_->has_errors()) {
        auto lexer_errors = lexer_->get_errors();
        errors_.insert(errors_.end(), lexer_errors.begin(), lexer_errors.end());
    }
    
    if (parser_ && parser_->has_errors()) {
        auto parser_errors = parser_->get_errors();
        errors_.insert(errors_.end(), parser_errors.begin(), parser_errors.end());
    }
    
    if (generator_ && generator_->has_errors()) {
        auto generator_errors = generator_->get_errors();
        errors_.insert(errors_.end(), generator_errors.begin(), generator_errors.end());
    }
}

} // namespace drawkern
} // namespace bolt