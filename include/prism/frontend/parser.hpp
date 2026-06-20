#pragma once

#include "prism/ast/ast_context.hpp"
#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"
#include "prism/basic/diagnostics.hpp"
#include "prism/basic/error.hpp"
#include "prism/core/container/expected.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/frontend/lexer.hpp"

namespace prism::frontend {

/** @brief Recursive-descent parser that converts a token stream into an AST. */
class Parser {
public:
    /** @brief Constructs a Parser with the given AST context, diagnostics, and token stream. */
    Parser(ast::ASTContext& ast_context, DiagnosticsEngine& diag, const Vector<Token>& tokens);

    /** @brief Parses the token stream into a translation unit declaration. @return The TranslationUnitDecl or an error.
     */
    Expected<ast::TranslationUnitDecl*, Error> parse_translation_unit();

private:
    /** @brief Returns the token at the given offset from the current position. */
    const Token& peek(size_t offset = 0) const;

    /** @brief Returns the most recently consumed token. */
    const Token& previous() const;

    /** @brief Returns true if the parser has consumed all tokens. */
    bool is_at_end() const;

    /** @brief Returns true if the current token matches the given kind without consuming it. */
    bool check(TokenKind kind) const;

    /** @brief Consumes the current token if it matches the given kind. */
    bool match(TokenKind kind);

    /** @brief Consumes the current token, producing an error if it does not match the expected kind. */
    Expected<Token, Error> consume(TokenKind kind, const char* message);

    /** @brief Returns true if the given token kind represents a type. */
    bool is_type_token(TokenKind kind) const;

    /** @brief Returns the type name string for the given token. */
    const char* type_name(const Token& token) const;

    /** @brief Parses a type name from the token stream. @param allow_void Whether 'void' is allowed. @return The type
     * name string or an error. */
    Expected<String, Error> parse_type_name(bool allow_void);

    /** @brief Parses an external (top-level) declaration. @return The declaration or an error. */
    Expected<ast::Decl*, Error> parse_external_decl();

    /** @brief Parses a template declaration. @return The declaration or an error. */
    Expected<ast::Decl*, Error> parse_template_decl();

    /** @brief Parses a class or struct record declaration. @return The RecordDecl or an error. */
    Expected<ast::RecordDecl*, Error> parse_record_decl();

    /** @brief Parses a function declaration with the given return type and name. @return The FunctionDecl or an error.
     */
    Expected<ast::FunctionDecl*, Error> parse_function(const String& return_type, const Token& name_token);

    /** @brief Parses a statement. @return The statement or an error. */
    Expected<ast::Stmt*, Error> parse_stmt();

    /** @brief Parses a block (compound) statement. @return The BlockStmt or an error. */
    Expected<ast::BlockStmt*, Error> parse_block();

    /** @brief Parses an if-statement. @return The statement or an error. */
    Expected<ast::Stmt*, Error> parse_if_stmt();

    /** @brief Parses a while-statement. @return The statement or an error. */
    Expected<ast::Stmt*, Error> parse_while_stmt();

    /** @brief Parses a for-statement. @return The statement or an error. */
    Expected<ast::Stmt*, Error> parse_for_stmt();

    /** @brief Parses a return-statement. @return The statement or an error. */
    Expected<ast::Stmt*, Error> parse_return_stmt();

    /** @brief Parses a declaration statement. @return The statement or an error. */
    Expected<ast::Stmt*, Error> parse_decl_stmt();

    /** @brief Parses an expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_expr();

    /** @brief Parses an assignment expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_assignment();

    /** @brief Parses an equality expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_equality();

    /** @brief Parses a comparison expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_comparison();

    /** @brief Parses an additive (term) expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_term();

    /** @brief Parses a multiplicative (factor) expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_factor();

    /** @brief Parses a unary expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_unary();

    /** @brief Parses a function call expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_call();

    /** @brief Parses a primary (literal, identifier, parenthesized) expression. @return The expression or an error. */
    Expected<ast::Expr*, Error> parse_primary();

    ast::ASTContext& ast_context_;
    DiagnosticsEngine& diag_;
    const Vector<Token>& tokens_;
    size_t current_ = 0;
};

}  // namespace prism::frontend
