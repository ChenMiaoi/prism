#pragma once

#include "prism/core/string/string.hpp"

namespace prism::ast {

class TranslationUnitDecl;
class Decl;
class Stmt;
class Expr;

/** @brief Utility for dumping an AST to a human-readable string representation. */
class ASTDumper {
public:
    /** @brief Dumps the entire translation unit to a string.
     *  @param tu The translation unit declaration to dump.
     *  @return The string representation of the AST. */
    prism::String dump(const TranslationUnitDecl* tu);

private:
    /** @brief Dumps a single declaration to a string.
     *  @param decl   The declaration to dump.
     *  @param indent The current indentation level.
     *  @return The string representation of the declaration. */
    prism::String dump_decl(const Decl* decl, int indent = 0);

    /** @brief Dumps a single statement to a string.
     *  @param stmt   The statement to dump.
     *  @param indent The current indentation level.
     *  @return The string representation of the statement. */
    prism::String dump_stmt(const Stmt* stmt, int indent = 0);

    /** @brief Dumps a single expression to a string.
     *  @param expr   The expression to dump.
     *  @param indent The current indentation level.
     *  @return The string representation of the expression. */
    prism::String dump_expr(const Expr* expr, int indent = 0);

    /** @brief Generates an indentation string for the given level.
     *  @param level The indentation level.
     *  @return A string of indentation characters. */
    prism::String indent_str(int level) const;
};

}  // namespace prism::ast
