#include "prism/frontend/parser.hpp"

#include "prism/core/format/format.hpp"

#include <cstdlib>

namespace prism::frontend {

/** @brief Constructs a Parser that parses a token stream into an AST.
 *  @param ast_context The AST context for allocating AST nodes.
 *  @param diag        The diagnostics engine for reporting parse errors.
 *  @param tokens      The token stream to parse. */
Parser::Parser(ast::ASTContext& ast_context, DiagnosticsEngine& diag, const Vector<Token>& tokens)
    : ast_context_(ast_context), diag_(diag), tokens_(tokens) {
}

/** @brief Parses the token stream into a translation unit declaration.
 *  @return The TranslationUnitDecl node, or an Error if parsing fails. */
Expected<ast::TranslationUnitDecl*, Error> Parser::parse_translation_unit() {
    auto* tu = ast_context_.create<ast::TranslationUnitDecl>();
    while (!is_at_end()) {
        auto decl = parse_external_decl();
        if (!decl) return Unexpected<Error>(decl.error());
        tu->add_decl(decl.value());
    }
    return tu;
}

/** @brief Returns the token at the given offset from the current position.
 *  @param offset The number of tokens to look ahead (default 0).
 *  @return A const reference to the token at the specified offset. */
const Token& Parser::peek(size_t offset) const {
    return tokens_[current_ + offset];
}

/** @brief Returns the most recently consumed token.
 *  @return A const reference to the previous token. */
const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

/** @brief Checks whether the parser has consumed all tokens.
 *  @return True if the current token is EndOfFile. */
bool Parser::is_at_end() const {
    return peek().kind() == TokenKind::EndOfFile;
}

/** @brief Checks if the current token matches the given kind without consuming it.
 *  @param kind The token kind to check against.
 *  @return True if the current token matches the given kind. */
bool Parser::check(TokenKind kind) const {
    return !is_at_end() && peek().kind() == kind;
}

/** @brief Consumes the current token if it matches the given kind.
 *  @param kind The token kind to match.
 *  @return True if the token was consumed, false otherwise. */
bool Parser::match(TokenKind kind) {
    if (!check(kind)) return false;
    ++current_;
    return true;
}

/** @brief Consumes the current token, producing an error if it does not match the expected kind.
 *  @param kind    The expected token kind.
 *  @param message The error message if the token does not match.
 *  @return The consumed token, or an Error. */
Expected<Token, Error> Parser::consume(TokenKind kind, const char* message) {
    if (check(kind)) return tokens_[current_++];
    auto loc = peek().loc();
    auto error = make_error(message, loc);
    diag_.report(DiagLevel::Error, error.message(), loc);
    return Unexpected<Error>(static_cast<Error&&>(error));
}

/** @brief Checks whether the given token kind represents a type keyword.
 *  @param kind The token kind to test.
 *  @return True if the kind is int, void, or bool. */
bool Parser::is_type_token(TokenKind kind) const {
    return kind == TokenKind::KeywordInt || kind == TokenKind::KeywordVoid || kind == TokenKind::KeywordBool;
}

/** @brief Returns the type name string for the given type token.
 *  @param token The token whose kind identifies the type.
 *  @return The type name string (e.g. "int", "void", "bool"), or "<invalid>". */
const char* Parser::type_name(const Token& token) const {
    switch (token.kind()) {
    case TokenKind::KeywordInt:
        return "int";
    case TokenKind::KeywordVoid:
        return "void";
    case TokenKind::KeywordBool:
        return "bool";
    default:
        return "<invalid>";
    }
}

/** @brief Parses a type name from the token stream, optionally allowing void.
 *  @param allow_void Whether 'void' is accepted as a valid type name.
 *  @return The type name string, or an Error if no type is found. */
Expected<String, Error> Parser::parse_type_name(bool allow_void) {
    if (!is_type_token(peek().kind()) || (!allow_void && peek().kind() == TokenKind::KeywordVoid)) {
        auto loc = peek().loc();
        auto error = make_error("expected type", loc);
        diag_.report(DiagLevel::Error, error.message(), loc);
        return Unexpected<Error>(static_cast<Error&&>(error));
    }
    Token type_token = tokens_[current_++];
    String result(type_name(type_token));
    if (match(TokenKind::Amp)) {
        if (type_token.kind() == TokenKind::KeywordVoid) {
            auto loc = previous().loc();
            auto error = make_error("reference to void is invalid", loc);
            diag_.report(DiagLevel::Error, error.message(), loc);
            return Unexpected<Error>(static_cast<Error&&>(error));
        }
        result.append("&");
    }
    return result;
}

/** @brief Parses an external (top-level) declaration.
 *  @return The parsed declaration, or an Error if parsing fails. */
Expected<ast::Decl*, Error> Parser::parse_external_decl() {
    if (check(TokenKind::KeywordTemplate)) {
        return parse_template_decl();
    }
    if (check(TokenKind::KeywordClass) || check(TokenKind::KeywordStruct)) {
        auto record = parse_record_decl();
        if (!record) return Unexpected<Error>(record.error());
        return static_cast<ast::Decl*>(record.value());
    }
    auto parsed_type = parse_type_name(true);
    if (!parsed_type) return Unexpected<Error>(parsed_type.error());
    auto name = consume(TokenKind::Identifier, "expected declaration name");
    if (!name) return Unexpected<Error>(name.error());
    if (check(TokenKind::LParen)) {
        auto fn = parse_function(parsed_type.value(), name.value());
        if (!fn) return Unexpected<Error>(fn.error());
        return static_cast<ast::Decl*>(fn.value());
    }
    ast::Expr* init = nullptr;
    if (match(TokenKind::Equal)) {
        auto expr = parse_expr();
        if (!expr) return Unexpected<Error>(expr.error());
        init = expr.value();
    }
    auto semi = consume(TokenKind::Semicolon, "expected ';' after variable declaration");
    if (!semi) return Unexpected<Error>(semi.error());
    return static_cast<ast::Decl*>(ast_context_.create<ast::VarDecl>(
        parsed_type.value().c_str(), name.value().spelling().c_str(), init, name.value().loc()));
}

/** @brief Parses a template declaration (currently recognized but not supported).
 *  @return An Error indicating that template declarations are not yet supported. */
Expected<ast::Decl*, Error> Parser::parse_template_decl() {
    auto template_token = consume(TokenKind::KeywordTemplate, "expected 'template'");
    if (!template_token) return Unexpected<Error>(template_token.error());
    auto less = consume(TokenKind::Less, "expected '<' after template");
    if (!less) return Unexpected<Error>(less.error());
    auto typename_token = consume(TokenKind::KeywordTypename, "expected 'typename' in template parameter");
    if (!typename_token) return Unexpected<Error>(typename_token.error());
    auto param_name = consume(TokenKind::Identifier, "expected template parameter name");
    if (!param_name) return Unexpected<Error>(param_name.error());
    auto greater = consume(TokenKind::Greater, "expected '>' after template parameter list");
    if (!greater) return Unexpected<Error>(greater.error());
    auto error = make_error("template declarations are recognized but not supported yet", template_token.value().loc());
    diag_.report(DiagLevel::Error, error.message(), template_token.value().loc());
    return Unexpected<Error>(static_cast<Error&&>(error));
}

/** @brief Parses a class or struct record declaration including fields, constructors, and destructors.
 *  @return The RecordDecl node, or an Error if parsing fails. */
Expected<ast::RecordDecl*, Error> Parser::parse_record_decl() {
    bool is_class = match(TokenKind::KeywordClass);

    if (!is_class) {
        auto struct_token = consume(TokenKind::KeywordStruct, "expected 'class' or 'struct'");
        if (!struct_token) return Unexpected<Error>(struct_token.error());
    }
    auto name = consume(TokenKind::Identifier, "expected record name");
    if (!name) return Unexpected<Error>(name.error());
    auto lbrace = consume(TokenKind::LBrace, "expected '{' after record name");
    if (!lbrace) return Unexpected<Error>(lbrace.error());
    auto* record = ast_context_.create<ast::RecordDecl>(is_class, name.value().spelling().c_str(), name.value().loc());
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        if (check(TokenKind::Identifier) && peek().spelling() == name.value().spelling() &&
            peek(1).kind() == TokenKind::LParen) {
            auto ctor_name = consume(TokenKind::Identifier, "expected constructor name");
            if (!ctor_name) return Unexpected<Error>(ctor_name.error());
            auto lparen = consume(TokenKind::LParen, "expected '(' after constructor name");
            if (!lparen) return Unexpected<Error>(lparen.error());
            auto rparen = consume(TokenKind::RParen, "only default constructors are supported");
            if (!rparen) return Unexpected<Error>(rparen.error());
            auto semi = consume(TokenKind::Semicolon, "expected ';' after constructor declaration");
            if (!semi) return Unexpected<Error>(semi.error());
            if (record->has_default_constructor()) {
                auto error = make_error("duplicate default constructor", ctor_name.value().loc());
                diag_.report(DiagLevel::Error, error.message(), ctor_name.value().loc());
                return Unexpected<Error>(static_cast<Error&&>(error));
            }
            record->set_default_constructor(ctor_name.value().loc());
            continue;
        }
        if (match(TokenKind::Tilde)) {
            auto dtor_name = consume(TokenKind::Identifier, "expected destructor name");
            if (!dtor_name) return Unexpected<Error>(dtor_name.error());
            if (dtor_name.value().spelling() != name.value().spelling()) {
                auto error = make_error("destructor name must match record name", dtor_name.value().loc());
                diag_.report(DiagLevel::Error, error.message(), dtor_name.value().loc());
                return Unexpected<Error>(static_cast<Error&&>(error));
            }
            auto lparen = consume(TokenKind::LParen, "expected '(' after destructor name");
            if (!lparen) return Unexpected<Error>(lparen.error());
            auto rparen = consume(TokenKind::RParen, "destructors cannot have parameters");
            if (!rparen) return Unexpected<Error>(rparen.error());
            auto semi = consume(TokenKind::Semicolon, "expected ';' after destructor declaration");
            if (!semi) return Unexpected<Error>(semi.error());
            if (record->has_destructor()) {
                auto error = make_error("duplicate destructor", dtor_name.value().loc());
                diag_.report(DiagLevel::Error, error.message(), dtor_name.value().loc());
                return Unexpected<Error>(static_cast<Error&&>(error));
            }
            record->set_destructor(dtor_name.value().loc());
            continue;
        }
        auto parsed_type = parse_type_name(false);
        if (!parsed_type) return Unexpected<Error>(parsed_type.error());
        auto field_name = consume(TokenKind::Identifier, "expected field name");
        if (!field_name) return Unexpected<Error>(field_name.error());
        auto semi = consume(TokenKind::Semicolon, "expected ';' after field declaration");
        if (!semi) return Unexpected<Error>(semi.error());
        record->add_field(ast_context_.create<ast::VarDecl>(
            parsed_type.value().c_str(), field_name.value().spelling().c_str(), nullptr, field_name.value().loc()));
    }
    auto rbrace = consume(TokenKind::RBrace, "expected '}' after record body");
    if (!rbrace) return Unexpected<Error>(rbrace.error());
    auto semi = consume(TokenKind::Semicolon, "expected ';' after record declaration");
    if (!semi) return Unexpected<Error>(semi.error());
    return record;
}

/** @brief Parses a function declaration with the given return type and name.
 *  @param return_type The return type string.
 *  @param name_token  The token containing the function name.
 *  @return The FunctionDecl node, or an Error if parsing fails. */
Expected<ast::FunctionDecl*, Error> Parser::parse_function(const String& return_type, const Token& name_token) {
    auto lparen = consume(TokenKind::LParen, "expected '(' after function name");
    if (!lparen) return Unexpected<Error>(lparen.error());
    Vector<ast::ParamDecl*> params;
    if (!check(TokenKind::RParen)) {
        do {
            auto parsed_type = parse_type_name(false);
            if (!parsed_type) return Unexpected<Error>(parsed_type.error());
            auto param_name = consume(TokenKind::Identifier, "expected parameter name");
            if (!param_name) return Unexpected<Error>(param_name.error());
            params.push_back(ast_context_.create<ast::ParamDecl>(
                parsed_type.value().c_str(), param_name.value().spelling().c_str(), param_name.value().loc()));
        } while (match(TokenKind::Comma));
    }
    auto rparen = consume(TokenKind::RParen, "expected ')' after parameter list");
    if (!rparen) return Unexpected<Error>(rparen.error());
    auto body = parse_block();
    if (!body) return Unexpected<Error>(body.error());
    auto* fn = ast_context_.create<ast::FunctionDecl>(return_type.c_str(), name_token.spelling().c_str(),
                                                      static_cast<Vector<ast::ParamDecl*>&&>(params), name_token.loc());
    fn->set_body(body.value());
    return fn;
}

/** @brief Parses a single statement from the token stream.
 *  @return The parsed statement, or an Error if parsing fails. */
Expected<ast::Stmt*, Error> Parser::parse_stmt() {
    if (check(TokenKind::LBrace)) {
        auto block = parse_block();
        if (!block) return Unexpected<Error>(block.error());
        return static_cast<ast::Stmt*>(block.value());
    }
    if (match(TokenKind::KeywordIf)) return parse_if_stmt();
    if (match(TokenKind::KeywordWhile)) return parse_while_stmt();
    if (match(TokenKind::KeywordFor)) return parse_for_stmt();
    if (match(TokenKind::KeywordReturn)) return parse_return_stmt();
    if (is_type_token(peek().kind()) && peek().kind() != TokenKind::KeywordVoid) return parse_decl_stmt();
    auto expr = parse_expr();
    if (!expr) return Unexpected<Error>(expr.error());
    auto semi = consume(TokenKind::Semicolon, "expected ';' after expression");
    if (!semi) return Unexpected<Error>(semi.error());
    return static_cast<ast::Stmt*>(ast_context_.create_stmt<ast::ExprStmt>(expr.value(), expr.value()->loc()));
}

/** @brief Parses a block (compound) statement enclosed in curly braces.
 *  @return The BlockStmt node, or an Error if parsing fails. */
Expected<ast::BlockStmt*, Error> Parser::parse_block() {
    auto lbrace = consume(TokenKind::LBrace, "expected '{' to begin block");
    if (!lbrace) return Unexpected<Error>(lbrace.error());
    auto* block = ast_context_.create_stmt<ast::BlockStmt>(lbrace.value().loc());
    while (!check(TokenKind::RBrace) && !is_at_end()) {
        auto stmt = parse_stmt();
        if (!stmt) return Unexpected<Error>(stmt.error());
        block->add_stmt(stmt.value());
    }
    auto rbrace = consume(TokenKind::RBrace, "expected '}' after block");
    if (!rbrace) return Unexpected<Error>(rbrace.error());
    return block;
}

/** @brief Parses an if/else statement.
 *  @return The IfStmt node, or an Error if parsing fails. */
Expected<ast::Stmt*, Error> Parser::parse_if_stmt() {
    auto lparen = consume(TokenKind::LParen, "expected '(' after if");
    if (!lparen) return Unexpected<Error>(lparen.error());
    auto cond = parse_expr();
    if (!cond) return Unexpected<Error>(cond.error());
    auto rparen = consume(TokenKind::RParen, "expected ')' after if condition");
    if (!rparen) return Unexpected<Error>(rparen.error());
    auto then_stmt = parse_stmt();
    if (!then_stmt) return Unexpected<Error>(then_stmt.error());
    ast::Stmt* else_stmt = nullptr;
    if (match(TokenKind::KeywordElse)) {
        auto parsed_else = parse_stmt();
        if (!parsed_else) return Unexpected<Error>(parsed_else.error());
        else_stmt = parsed_else.value();
    }
    return static_cast<ast::Stmt*>(
        ast_context_.create_stmt<ast::IfStmt>(cond.value(), then_stmt.value(), else_stmt, cond.value()->loc()));
}

/** @brief Parses a while loop statement.
 *  @return The WhileStmt node, or an Error if parsing fails. */
Expected<ast::Stmt*, Error> Parser::parse_while_stmt() {
    auto lparen = consume(TokenKind::LParen, "expected '(' after while");
    if (!lparen) return Unexpected<Error>(lparen.error());
    auto cond = parse_expr();
    if (!cond) return Unexpected<Error>(cond.error());
    auto rparen = consume(TokenKind::RParen, "expected ')' after while condition");
    if (!rparen) return Unexpected<Error>(rparen.error());
    auto body = parse_stmt();
    if (!body) return Unexpected<Error>(body.error());
    return static_cast<ast::Stmt*>(
        ast_context_.create_stmt<ast::WhileStmt>(cond.value(), body.value(), cond.value()->loc()));
}

/** @brief Parses a for loop statement.
 *  @return The ForStmt node, or an Error if parsing fails. */
Expected<ast::Stmt*, Error> Parser::parse_for_stmt() {
    auto lparen = consume(TokenKind::LParen, "expected '(' after for");
    if (!lparen) return Unexpected<Error>(lparen.error());
    ast::Stmt* init = nullptr;
    if (!check(TokenKind::Semicolon)) {
        if (is_type_token(peek().kind()) && peek().kind() != TokenKind::KeywordVoid) {
            auto parsed_init = parse_decl_stmt();
            if (!parsed_init) return Unexpected<Error>(parsed_init.error());
            init = parsed_init.value();
        } else {
            auto expr = parse_expr();
            if (!expr) return Unexpected<Error>(expr.error());
            auto semi = consume(TokenKind::Semicolon, "expected ';' after for initializer");
            if (!semi) return Unexpected<Error>(semi.error());
            init = ast_context_.create_stmt<ast::ExprStmt>(expr.value(), expr.value()->loc());
        }
    } else {
        ++current_;
    }
    ast::Expr* cond = nullptr;
    if (!check(TokenKind::Semicolon)) {
        auto parsed_cond = parse_expr();
        if (!parsed_cond) return Unexpected<Error>(parsed_cond.error());
        cond = parsed_cond.value();
    }
    auto semi = consume(TokenKind::Semicolon, "expected ';' after for condition");
    if (!semi) return Unexpected<Error>(semi.error());
    ast::Expr* inc = nullptr;
    if (!check(TokenKind::RParen)) {
        auto parsed_inc = parse_expr();
        if (!parsed_inc) return Unexpected<Error>(parsed_inc.error());
        inc = parsed_inc.value();
    }
    auto rparen = consume(TokenKind::RParen, "expected ')' after for clauses");
    if (!rparen) return Unexpected<Error>(rparen.error());
    auto body = parse_stmt();
    if (!body) return Unexpected<Error>(body.error());
    return static_cast<ast::Stmt*>(
        ast_context_.create_stmt<ast::ForStmt>(init, cond, inc, body.value(), lparen.value().loc()));
}

/** @brief Parses a return statement.
 *  @return The ReturnStmt node, or an Error if parsing fails. */
Expected<ast::Stmt*, Error> Parser::parse_return_stmt() {
    SourceLocation loc = previous().loc();
    ast::Expr* value = nullptr;
    if (!check(TokenKind::Semicolon)) {
        auto expr = parse_expr();
        if (!expr) return Unexpected<Error>(expr.error());
        value = expr.value();
    }
    auto semi = consume(TokenKind::Semicolon, "expected ';' after return value");
    if (!semi) return Unexpected<Error>(semi.error());
    return static_cast<ast::Stmt*>(ast_context_.create_stmt<ast::ReturnStmt>(value, loc));
}

/** @brief Parses a variable declaration statement.
 *  @return The VarDeclStmt node, or an Error if parsing fails. */
Expected<ast::Stmt*, Error> Parser::parse_decl_stmt() {
    auto parsed_type = parse_type_name(false);
    if (!parsed_type) return Unexpected<Error>(parsed_type.error());
    auto name = consume(TokenKind::Identifier, "expected variable name");
    if (!name) return Unexpected<Error>(name.error());
    ast::Expr* init = nullptr;
    if (match(TokenKind::Equal)) {
        auto expr = parse_expr();
        if (!expr) return Unexpected<Error>(expr.error());
        init = expr.value();
    }
    auto semi = consume(TokenKind::Semicolon, "expected ';' after variable declaration");
    if (!semi) return Unexpected<Error>(semi.error());
    return static_cast<ast::Stmt*>(ast_context_.create_stmt<ast::VarDeclStmt>(
        parsed_type.value().c_str(), name.value().spelling().c_str(), init, name.value().loc()));
}

/** @brief Parses an expression starting from the lowest precedence (assignment).
 *  @return The parsed expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_expr() {
    return parse_assignment();
}

/** @brief Parses an assignment expression (right-associative).
 *  @return The parsed assignment expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_assignment() {
    auto expr = parse_equality();
    if (!expr) return expr;
    if (match(TokenKind::Equal)) {
        auto value = parse_assignment();
        if (!value) return Unexpected<Error>(value.error());
        return static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::AssignmentExpr>(expr.value(), "=", value.value(), expr.value()->loc()));
    }
    return expr;
}

/** @brief Parses an equality expression (== and !=).
 *  @return The parsed equality expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_equality() {
    auto expr = parse_comparison();
    if (!expr) return expr;
    while (match(TokenKind::EqualEqual) || match(TokenKind::BangEqual)) {
        Token op = previous();
        auto rhs = parse_comparison();
        if (!rhs) return Unexpected<Error>(rhs.error());
        expr = static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::BinaryExpr>(op.spelling().c_str(), expr.value(), rhs.value(), op.loc()));
    }
    return expr;
}

/** @brief Parses a comparison expression (<, <=, >, >=).
 *  @return The parsed comparison expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_comparison() {
    auto expr = parse_term();
    if (!expr) return expr;
    while (match(TokenKind::Less) || match(TokenKind::LessEqual) || match(TokenKind::Greater) ||
           match(TokenKind::GreaterEqual)) {
        Token op = previous();
        auto rhs = parse_term();
        if (!rhs) return Unexpected<Error>(rhs.error());
        expr = static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::BinaryExpr>(op.spelling().c_str(), expr.value(), rhs.value(), op.loc()));
    }
    return expr;
}

/** @brief Parses an additive (term) expression (+ and -).
 *  @return The parsed term expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_term() {
    auto expr = parse_factor();
    if (!expr) return expr;
    while (match(TokenKind::Plus) || match(TokenKind::Minus)) {
        Token op = previous();
        auto rhs = parse_factor();
        if (!rhs) return Unexpected<Error>(rhs.error());
        expr = static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::BinaryExpr>(op.spelling().c_str(), expr.value(), rhs.value(), op.loc()));
    }
    return expr;
}

/** @brief Parses a multiplicative (factor) expression (*, /, and %).
 *  @return The parsed factor expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_factor() {
    auto expr = parse_unary();
    if (!expr) return expr;
    while (match(TokenKind::Star) || match(TokenKind::Slash) || match(TokenKind::Percent)) {
        Token op = previous();
        auto rhs = parse_unary();
        if (!rhs) return Unexpected<Error>(rhs.error());
        expr = static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::BinaryExpr>(op.spelling().c_str(), expr.value(), rhs.value(), op.loc()));
    }
    return expr;
}

/** @brief Parses a unary expression (! and -).
 *  @return The parsed unary expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_unary() {
    if (match(TokenKind::Bang) || match(TokenKind::Minus)) {
        Token op = previous();
        auto rhs = parse_unary();
        if (!rhs) return Unexpected<Error>(rhs.error());
        return static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::UnaryExpr>(op.spelling().c_str(), rhs.value(), true, op.loc()));
    }
    return parse_call();
}

/** @brief Parses a function call expression, including chained calls.
 *  @return The parsed call expression, or an Error if parsing fails. */
Expected<ast::Expr*, Error> Parser::parse_call() {
    auto expr = parse_primary();
    if (!expr) return expr;
    while (match(TokenKind::LParen)) {
        Vector<ast::Expr*> args;
        if (!check(TokenKind::RParen)) {
            do {
                auto arg = parse_expr();
                if (!arg) return Unexpected<Error>(arg.error());
                args.push_back(arg.value());
            } while (match(TokenKind::Comma));
        }
        auto rparen = consume(TokenKind::RParen, "expected ')' after call arguments");
        if (!rparen) return Unexpected<Error>(rparen.error());
        expr = static_cast<ast::Expr*>(ast_context_.create_expr<ast::CallExpr>(
            expr.value(), static_cast<Vector<ast::Expr*>&&>(args), expr.value()->loc()));
    }
    return expr;
}

/** @brief Parses a primary expression (literal, identifier, or parenthesized expression).
 *  @return The parsed primary expression, or an Error if no valid expression is found. */
Expected<ast::Expr*, Error> Parser::parse_primary() {
    if (match(TokenKind::IntegerLiteral)) {
        return static_cast<ast::Expr*>(ast_context_.create_expr<ast::IntegerLiteralExpr>(
            ::strtoll(previous().spelling().c_str(), nullptr, 10), previous().loc()));
    }
    if (match(TokenKind::KeywordTrue))
        return static_cast<ast::Expr*>(ast_context_.create_expr<ast::BoolLiteralExpr>(true, previous().loc()));
    if (match(TokenKind::KeywordFalse))
        return static_cast<ast::Expr*>(ast_context_.create_expr<ast::BoolLiteralExpr>(false, previous().loc()));
    if (match(TokenKind::Identifier))
        return static_cast<ast::Expr*>(
            ast_context_.create_expr<ast::IdentifierExpr>(previous().spelling().c_str(), previous().loc()));
    if (match(TokenKind::LParen)) {
        auto expr = parse_expr();
        if (!expr) return expr;
        auto rparen = consume(TokenKind::RParen, "expected ')' after expression");
        if (!rparen) return Unexpected<Error>(rparen.error());
        return expr;
    }
    auto loc = peek().loc();
    auto error =
        make_error(prism::format("expected expression before {}", token_kind_name(peek().kind())).c_str(), loc);
    diag_.report(DiagLevel::Error, error.message(), loc);
    return Unexpected<Error>(static_cast<Error&&>(error));
}

}  // namespace prism::frontend
