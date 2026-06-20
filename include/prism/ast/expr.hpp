#pragma once

#include "prism/basic/source_location.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

namespace prism::ast {

/** @brief Enumerates the kinds of expression AST nodes. */
enum class ExprKind {
    IntegerLiteral,  ///< Integer literal expression
    FloatLiteral,    ///< Floating-point literal expression
    StringLiteral,   ///< String literal expression
    BoolLiteral,     ///< Boolean literal expression
    CharLiteral,     ///< Character literal expression
    Identifier,      ///< Identifier (name reference) expression
    BinaryOp,        ///< Binary operator expression
    UnaryOp,         ///< Unary operator expression
    Assignment,      ///< Assignment expression
    Call,            ///< Function call expression
    MemberAccess,    ///< Member access (dot/arrow) expression
    ArrayAccess,     ///< Array subscript expression
    Ternary,         ///< Ternary conditional expression
    Cast,            ///< Type cast expression
};

/** @brief Base class for all expression AST nodes. */
class Expr {
public:
    virtual ~Expr() = default;

    /** @brief Returns the expression kind discriminator.
     *  @return The ExprKind for this node. */
    ExprKind kind() const { return kind_; }

    /** @brief Returns the source location of this expression.
     *  @return The SourceLocation where this expression appears. */
    SourceLocation loc() const { return loc_; }

    /** @brief Returns the resolved type name of this expression.
     *  @return The type name string. */
    const prism::String& type_name() const { return type_name_; }

    /** @brief Sets the resolved type name of this expression.
     *  @param name The type name to assign. */
    void set_type_name(const char* name) { type_name_ = name; }

protected:
    /** @brief Constructs an expression with a given kind and source location.
     *  @param kind The expression kind.
     *  @param loc  The source location. */
    Expr(ExprKind kind, SourceLocation loc = SourceLocation::invalid()) : kind_(kind), loc_(loc) {}

    /** @brief Constructs an expression with a kind, type name, and source location.
     *  @param kind      The expression kind.
     *  @param type_name The type name string.
     *  @param loc       The source location. */
    Expr(ExprKind kind, const char* type_name, SourceLocation loc = SourceLocation::invalid())
        : kind_(kind), loc_(loc), type_name_(type_name) {}

private:
    ExprKind kind_;
    SourceLocation loc_;
    prism::String type_name_;
};

/** @brief AST node for integer literal expressions. */
class IntegerLiteralExpr final : public Expr {
public:
    /** @brief Constructs an integer literal expression.
     *  @param value The integer value.
     *  @param loc   The source location. */
    explicit IntegerLiteralExpr(int64_t value, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::IntegerLiteral, "int", loc), value_(value) {}

    /** @brief Returns the integer value.
     *  @return The int64_t value. */
    int64_t value() const { return value_; }

private:
    int64_t value_;
};

/** @brief AST node for floating-point literal expressions. */
class FloatLiteralExpr final : public Expr {
public:
    /** @brief Constructs a floating-point literal expression.
     *  @param value The double value.
     *  @param loc   The source location. */
    explicit FloatLiteralExpr(double value, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::FloatLiteral, "double", loc), value_(value) {}

    /** @brief Returns the floating-point value.
     *  @return The double value. */
    double value() const { return value_; }

private:
    double value_;
};

/** @brief AST node for string literal expressions. */
class StringLiteralExpr final : public Expr {
public:
    /** @brief Constructs a string literal expression.
     *  @param value The string value.
     *  @param loc   The source location. */
    explicit StringLiteralExpr(const char* value, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::StringLiteral, "string", loc), value_(value) {}

    /** @brief Returns the string value.
     *  @return A reference to the string. */
    const prism::String& value() const { return value_; }

private:
    prism::String value_;
};

/** @brief AST node for boolean literal expressions. */
class BoolLiteralExpr final : public Expr {
public:
    /** @brief Constructs a boolean literal expression.
     *  @param value The boolean value.
     *  @param loc   The source location. */
    explicit BoolLiteralExpr(bool value, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::BoolLiteral, "bool", loc), value_(value) {}

    /** @brief Returns the boolean value.
     *  @return The bool value. */
    bool value() const { return value_; }

private:
    bool value_;
};

/** @brief AST node for character literal expressions. */
class CharLiteralExpr final : public Expr {
public:
    /** @brief Constructs a character literal expression.
     *  @param value The character value.
     *  @param loc   The source location. */
    explicit CharLiteralExpr(char value, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::CharLiteral, "char", loc), value_(value) {}

    /** @brief Returns the character value.
     *  @return The char value. */
    char value() const { return value_; }

private:
    char value_;
};

/** @brief AST node for identifier (name reference) expressions. */
class IdentifierExpr final : public Expr {
public:
    /** @brief Constructs an identifier expression.
     *  @param name The identifier name.
     *  @param loc  The source location. */
    explicit IdentifierExpr(const char* name, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::Identifier, loc), name_(name) {}

    /** @brief Returns the identifier name.
     *  @return A reference to the name string. */
    const prism::String& name() const { return name_; }

private:
    prism::String name_;
};

/** @brief AST node for binary operator expressions (e.g. +, -, *, /). */
class BinaryExpr final : public Expr {
public:
    /** @brief Constructs a binary expression.
     *  @param op  The operator string.
     *  @param lhs The left-hand side operand.
     *  @param rhs The right-hand side operand.
     *  @param loc The source location. */
    BinaryExpr(const char* op, Expr* lhs, Expr* rhs, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::BinaryOp, loc), op_(op), lhs_(lhs), rhs_(rhs) {}

    /** @brief Returns the operator string.
     *  @return A reference to the operator string. */
    const prism::String& op() const { return op_; }

    /** @brief Returns the left-hand side operand.
     *  @return Pointer to the LHS expression. */
    Expr* lhs() const { return lhs_; }

    /** @brief Returns the right-hand side operand.
     *  @return Pointer to the RHS expression. */
    Expr* rhs() const { return rhs_; }

private:
    prism::String op_;
    Expr* lhs_;
    Expr* rhs_;
};

/** @brief AST node for unary operator expressions (e.g. -, !, ~). */
class UnaryExpr final : public Expr {
public:
    /** @brief Constructs a unary expression.
     *  @param op       The operator string.
     *  @param operand  The operand expression.
     *  @param is_prefix True if the operator is a prefix operator.
     *  @param loc      The source location. */
    UnaryExpr(const char* op, Expr* operand, bool is_prefix, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::UnaryOp, loc), op_(op), operand_(operand), is_prefix_(is_prefix) {}

    /** @brief Returns the operator string.
     *  @return A reference to the operator string. */
    const prism::String& op() const { return op_; }

    /** @brief Returns the operand expression.
     *  @return Pointer to the operand expression. */
    Expr* operand() const { return operand_; }

    /** @brief Returns whether the operator is a prefix operator.
     *  @return True if prefix, false if postfix. */
    bool is_prefix() const { return is_prefix_; }

private:
    prism::String op_;
    Expr* operand_;
    bool is_prefix_;
};

/** @brief AST node for assignment expressions (e.g. =, +=, -=). */
class AssignmentExpr final : public Expr {
public:
    /** @brief Constructs an assignment expression.
     *  @param target The assignment target expression.
     *  @param op     The assignment operator string.
     *  @param value  The value expression.
     *  @param loc    The source location. */
    AssignmentExpr(Expr* target, const char* op, Expr* value, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::Assignment, loc), target_(target), op_(op), value_(value) {}

    /** @brief Returns the assignment target expression.
     *  @return Pointer to the target expression. */
    Expr* target() const { return target_; }

    /** @brief Returns the assignment operator string.
     *  @return A reference to the operator string. */
    const prism::String& op() const { return op_; }

    /** @brief Returns the value being assigned.
     *  @return Pointer to the value expression. */
    Expr* value() const { return value_; }

private:
    Expr* target_;
    prism::String op_;
    Expr* value_;
};

/** @brief AST node for function call expressions. */
class CallExpr final : public Expr {
public:
    /** @brief Constructs a call expression.
     *  @param callee The callee expression (function being called).
     *  @param args   The argument expressions.
     *  @param loc    The source location. */
    CallExpr(Expr* callee, prism::Vector<Expr*> args, SourceLocation loc = SourceLocation::invalid())
        : Expr(ExprKind::Call, loc), callee_(callee), args_(static_cast<prism::Vector<Expr*>&&>(args)) {}

    /** @brief Returns the callee expression.
     *  @return Pointer to the callee expression. */
    Expr* callee() const { return callee_; }

    /** @brief Returns the argument expressions.
     *  @return A reference to the vector of argument expressions. */
    const prism::Vector<Expr*>& args() const { return args_; }

private:
    Expr* callee_;
    prism::Vector<Expr*> args_;
};

}  // namespace prism::ast
