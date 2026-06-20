#pragma once

#include "prism/basic/source_location.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

namespace prism::ast {

class Expr;

/** @brief Enumerates the kinds of statement AST nodes. */
enum class StmtKind {
    Block,         ///< Block statement (curly-brace delimited)
    Expression,    ///< Expression statement
    Return,        ///< Return statement
    If,            ///< If/else statement
    While,         ///< While loop statement
    For,           ///< For loop statement
    VariableDecl,  ///< Variable declaration statement
    Break,         ///< Break statement
    Continue,      ///< Continue statement
};

/** @brief Base class for all statement AST nodes. */
class Stmt {
public:
    virtual ~Stmt() = default;

    /** @brief Returns the statement kind discriminator.
     *  @return The StmtKind for this node. */
    StmtKind kind() const { return kind_; }

    /** @brief Returns the source location of this statement.
     *  @return The SourceLocation where this statement appears. */
    SourceLocation loc() const { return loc_; }

protected:
    /** @brief Constructs a statement with a given kind and source location.
     *  @param kind The statement kind.
     *  @param loc  The source location. */
    Stmt(StmtKind kind, SourceLocation loc = SourceLocation::invalid()) : kind_(kind), loc_(loc) {}

private:
    StmtKind kind_;
    SourceLocation loc_;
};

/** @brief AST node for block statements (curly-brace delimited sequence of statements). */
class BlockStmt final : public Stmt {
public:
    /** @brief Constructs a block statement.
     *  @param loc The source location. */
    explicit BlockStmt(SourceLocation loc = SourceLocation::invalid()) : Stmt(StmtKind::Block, loc) {}

    /** @brief Returns the mutable list of statements in this block.
     *  @return Reference to the vector of statements. */
    prism::Vector<Stmt*>& stmts() { return stmts_; }

    /** @brief Returns the immutable list of statements in this block.
     *  @return Const reference to the vector of statements. */
    const prism::Vector<Stmt*>& stmts() const { return stmts_; }

    /** @brief Appends a statement to this block.
     *  @param stmt The statement to add. */
    void add_stmt(Stmt* stmt) { stmts_.push_back(stmt); }

private:
    prism::Vector<Stmt*> stmts_;
};

/** @brief AST node for expression statements. */
class ExprStmt final : public Stmt {
public:
    /** @brief Constructs an expression statement.
     *  @param expr The expression.
     *  @param loc  The source location. */
    explicit ExprStmt(Expr* expr, SourceLocation loc = SourceLocation::invalid())
        : Stmt(StmtKind::Expression, loc), expr_(expr) {}

    /** @brief Returns the expression.
     *  @return Pointer to the expression. */
    Expr* expr() const { return expr_; }

private:
    Expr* expr_;
};

/** @brief AST node for return statements. */
class ReturnStmt final : public Stmt {
public:
    /** @brief Constructs a return statement.
     *  @param value The return value expression, or nullptr for void returns.
     *  @param loc   The source location. */
    explicit ReturnStmt(Expr* value = nullptr, SourceLocation loc = SourceLocation::invalid())
        : Stmt(StmtKind::Return, loc), value_(value) {}

    /** @brief Returns the return value expression.
     *  @return Pointer to the value expression, or nullptr. */
    Expr* value() const { return value_; }

    /** @brief Returns whether this return statement has a value.
     *  @return True if a value is present. */
    bool has_value() const { return value_ != nullptr; }

private:
    Expr* value_;
};

/** @brief AST node for if/else statements. */
class IfStmt final : public Stmt {
public:
    /** @brief Constructs an if statement.
     *  @param cond      The condition expression.
     *  @param then_stmt The then-branch statement.
     *  @param else_stmt The else-branch statement, or nullptr.
     *  @param loc       The source location. */
    IfStmt(Expr* cond, Stmt* then_stmt, Stmt* else_stmt = nullptr, SourceLocation loc = SourceLocation::invalid())
        : Stmt(StmtKind::If, loc), cond_(cond), then_stmt_(then_stmt), else_stmt_(else_stmt) {}

    /** @brief Returns the condition expression.
     *  @return Pointer to the condition expression. */
    Expr* condition() const { return cond_; }

    /** @brief Returns the then-branch statement.
     *  @return Pointer to the then statement. */
    Stmt* then_stmt() const { return then_stmt_; }

    /** @brief Returns the else-branch statement.
     *  @return Pointer to the else statement, or nullptr. */
    Stmt* else_stmt() const { return else_stmt_; }

    /** @brief Returns whether this if statement has an else branch.
     *  @return True if an else branch is present. */
    bool has_else() const { return else_stmt_ != nullptr; }

private:
    Expr* cond_;
    Stmt* then_stmt_;
    Stmt* else_stmt_;
};

/** @brief AST node for while loop statements. */
class WhileStmt final : public Stmt {
public:
    /** @brief Constructs a while loop statement.
     *  @param cond The loop condition expression.
     *  @param body The loop body statement.
     *  @param loc  The source location. */
    WhileStmt(Expr* cond, Stmt* body, SourceLocation loc = SourceLocation::invalid())
        : Stmt(StmtKind::While, loc), cond_(cond), body_(body) {}

    /** @brief Returns the loop condition expression.
     *  @return Pointer to the condition expression. */
    Expr* condition() const { return cond_; }

    /** @brief Returns the loop body statement.
     *  @return Pointer to the body statement. */
    Stmt* body() const { return body_; }

private:
    Expr* cond_;
    Stmt* body_;
};

/** @brief AST node for for loop statements. */
class ForStmt final : public Stmt {
public:
    /** @brief Constructs a for loop statement.
     *  @param init The initializer statement, or nullptr.
     *  @param cond The loop condition expression, or nullptr.
     *  @param inc  The increment expression, or nullptr.
     *  @param body The loop body statement.
     *  @param loc  The source location. */
    ForStmt(Stmt* init, Expr* cond, Expr* inc, Stmt* body, SourceLocation loc = SourceLocation::invalid())
        : Stmt(StmtKind::For, loc), init_(init), cond_(cond), inc_(inc), body_(body) {}

    /** @brief Returns the initializer statement.
     *  @return Pointer to the init statement, or nullptr. */
    Stmt* init() const { return init_; }

    /** @brief Returns the loop condition expression.
     *  @return Pointer to the condition expression, or nullptr. */
    Expr* condition() const { return cond_; }

    /** @brief Returns the increment expression.
     *  @return Pointer to the increment expression, or nullptr. */
    Expr* increment() const { return inc_; }

    /** @brief Returns the loop body statement.
     *  @return Pointer to the body statement. */
    Stmt* body() const { return body_; }

private:
    Stmt* init_;
    Expr* cond_;
    Expr* inc_;
    Stmt* body_;
};

/** @brief AST node for variable declaration statements. */
class VarDeclStmt final : public Stmt {
public:
    /** @brief Constructs a variable declaration statement.
     *  @param type_name The type name string.
     *  @param var_name  The variable name string.
     *  @param init      The initializer expression, or nullptr.
     *  @param loc       The source location. */
    VarDeclStmt(const char* type_name, const char* var_name, Expr* init = nullptr,
                SourceLocation loc = SourceLocation::invalid())
        : Stmt(StmtKind::VariableDecl, loc), type_name_(type_name), var_name_(var_name), init_(init) {}

    /** @brief Returns the type name.
     *  @return A reference to the type name string. */
    const prism::String& type_name() const { return type_name_; }

    /** @brief Returns the variable name.
     *  @return A reference to the variable name string. */
    const prism::String& var_name() const { return var_name_; }

    /** @brief Returns the initializer expression.
     *  @return Pointer to the init expression, or nullptr. */
    Expr* init_expr() const { return init_; }

private:
    prism::String type_name_;
    prism::String var_name_;
    Expr* init_;
};

}  // namespace prism::ast
