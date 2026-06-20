#pragma once

#include "prism/ast/stmt.hpp"
#include "prism/basic/ADT/scoped_hash_map.hpp"
#include "prism/basic/diagnostics.hpp"
#include "prism/basic/error.hpp"
#include "prism/core/container/expected.hpp"
#include "prism/core/string/string.hpp"

namespace prism::ast {
class TranslationUnitDecl;
class FunctionDecl;
class VarDecl;
class Expr;
}  // namespace prism::ast

namespace prism::sema {

/** @brief Holds resolved type information for a symbol during semantic analysis. */
struct TypeInfo {
    /** @brief Default constructor. */
    TypeInfo() = default;

    /** @brief Constructs TypeInfo with the given type name. */
    explicit TypeInfo(prism::String type_name) : name(static_cast<prism::String&&>(type_name)) {}

    prism::String name;                        /**< The resolved type name. */
    bool is_const = false;                     /**< True if the type is const-qualified. */
    bool is_pointer = false;                   /**< True if the type is a pointer. */
    bool is_reference = false;                 /**< True if the type is a reference. */
    bool is_function = false;                  /**< True if the type represents a function. */
    prism::String param_types[16];             /**< Parameter types for function types. */
    size_t param_count = 0;                    /**< Number of parameters in the function type. */
    prism::String overload_return_types[8];    /**< Return types for overloaded functions. */
    prism::String overload_param_types[8][16]; /**< Parameter types for each overload. */
    size_t overload_param_counts[8] = {};      /**< Parameter count for each overload. */
    size_t overload_count = 0;                 /**< Number of overloads. */
};

/** @brief Performs semantic analysis on an AST, checking types and symbol resolution. */
class SemanticAnalyzer {
public:
    /** @brief Constructs a SemanticAnalyzer with the given diagnostics engine. */
    explicit SemanticAnalyzer(prism::DiagnosticsEngine& diag);

    /** @brief Analyzes the given translation unit. @return Success or an error. */
    prism::Expected<void, prism::Error> analyze(ast::TranslationUnitDecl* tu);

private:
    /** @brief Analyzes a single function declaration. @return Success or an error. */
    prism::Expected<void, prism::Error> analyze_function(ast::FunctionDecl* fn);

    /** @brief Analyzes a single statement. @return Success or an error. */
    prism::Expected<void, prism::Error> analyze_stmt(ast::Stmt* stmt);

    /** @brief Analyzes a block (compound) statement. @return Success or an error. */
    prism::Expected<void, prism::Error> analyze_block_stmt(ast::BlockStmt* block);

    /** @brief Analyzes an expression and returns its resolved type. @return The resolved TypeInfo or an error. */
    prism::Expected<TypeInfo, prism::Error> analyze_expr(ast::Expr* expr);

    /** @brief Validates that a return statement matches the expected return type. @return Success or an error. */
    prism::Expected<void, prism::Error> validate_return_type(ast::Stmt* stmt, const TypeInfo& type);

    /** @brief Resolves a type name string into a TypeInfo. @return The resolved TypeInfo or an error. */
    prism::Expected<TypeInfo, prism::Error> resolve_type(const char* type_name);

    prism::DiagnosticsEngine& diag_;
    prism::ScopedHashTable<TypeInfo> scopes_;
};

}  // namespace prism::sema
