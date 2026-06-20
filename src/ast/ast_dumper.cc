#include "prism/ast/ast_dumper.hpp"

#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"

#include <cstdio>

namespace prism::ast {
namespace {

size_t indent_width(int indent) {
    return indent <= 0 ? 0 : static_cast<size_t>(indent) * 2;
}

}  // namespace

/** @brief Dumps a translation unit to a human-readable string representation.
 *  @param tu The translation unit declaration to dump.
 *  @return A string containing the textual representation of the translation unit. */
String ASTDumper::dump(const TranslationUnitDecl* tu) {
    String result;
    result.append("TranslationUnitDecl {\n");
    for (size_t i = 0; i < tu->decls().size(); ++i) {
        result.append(dump_decl(tu->decls()[i], 1));
    }
    result.append("}\n");
    return result;
}

/** @brief Dumps a single declaration to a human-readable string at the given indentation level.
 *  @param decl   The declaration to dump.
 *  @param indent The current indentation level (multiplied by 2 for spacing).
 *  @return A string containing the textual representation of the declaration. */
String ASTDumper::dump_decl(const Decl* decl, int indent) {
    String result;
    String pad(indent_width(indent), ' ');

    switch (decl->kind()) {
    case DeclKind::Function: {
        auto* fn = static_cast<const FunctionDecl*>(decl);
        result.append(pad);
        result.append("FunctionDecl '");
        result.append(fn->name());
        result.append("' : ");
        result.append(fn->return_type());
        result.append("\n");
        if (fn->has_body()) {
            result.append(dump_stmt(fn->body(), indent + 1));
        }
        break;
    }
    case DeclKind::Variable: {
        auto* var = static_cast<const VarDecl*>(decl);
        result.append(pad);
        result.append("VarDecl '");
        result.append(var->name());
        result.append("' : ");
        result.append(var->type_name());
        result.append("\n");
        break;
    }
    default:
        result.append(pad);
        result.append("Decl '");
        result.append(decl->name());
        result.append("'\n");
        break;
    }

    return result;
}

/** @brief Dumps a single statement to a human-readable string at the given indentation level.
 *  @param stmt   The statement to dump.
 *  @param indent The current indentation level (multiplied by 2 for spacing).
 *  @return A string containing the textual representation of the statement. */
String ASTDumper::dump_stmt(const Stmt* stmt, int indent) {
    String result;
    String pad(indent_width(indent), ' ');

    switch (stmt->kind()) {
    case StmtKind::Block: {
        auto* block = static_cast<const BlockStmt*>(stmt);
        result.append(pad);
        result.append("BlockStmt {\n");
        for (size_t i = 0; i < block->stmts().size(); ++i) {
            result.append(dump_stmt(block->stmts()[i], indent + 1));
        }
        result.append(pad);
        result.append("}\n");
        break;
    }
    case StmtKind::Return: {
        auto* ret = static_cast<const ReturnStmt*>(stmt);
        result.append(pad);
        result.append("ReturnStmt\n");
        if (ret->has_value()) {
            result.append(dump_expr(ret->value(), indent + 1));
        }
        break;
    }
    case StmtKind::If: {
        auto* if_stmt = static_cast<const IfStmt*>(stmt);
        result.append(pad);
        result.append("IfStmt {\n");
        result.append(dump_expr(if_stmt->condition(), indent + 1));
        result.append(dump_stmt(if_stmt->then_stmt(), indent + 1));
        if (if_stmt->has_else()) {
            result.append(dump_stmt(if_stmt->else_stmt(), indent + 1));
        }
        result.append(pad);
        result.append("}\n");
        break;
    }
    default:
        result.append(pad);
        result.append("Stmt\n");
        break;
    }

    return result;
}

/** @brief Dumps a single expression to a human-readable string at the given indentation level.
 *  @param expr   The expression to dump.
 *  @param indent The current indentation level (multiplied by 2 for spacing).
 *  @return A string containing the textual representation of the expression. */
String ASTDumper::dump_expr(const Expr* expr, int indent) {
    String pad(indent_width(indent), ' ');

    switch (expr->kind()) {
    case ExprKind::IntegerLiteral: {
        auto* lit = static_cast<const IntegerLiteralExpr*>(expr);
        String result = pad;
        result.append("IntegerLiteral(");
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(lit->value()));
        result.append(buf, static_cast<size_t>(len));
        result.append(")\n");
        return result;
    }
    case ExprKind::Identifier: {
        auto* id = static_cast<const IdentifierExpr*>(expr);
        String result = pad;
        result.append("Identifier('");
        result.append(id->name());
        result.append("')\n");
        return result;
    }
    case ExprKind::BinaryOp: {
        auto* bin = static_cast<const BinaryExpr*>(expr);
        String result = pad;
        result.append("BinaryExpr('");
        result.append(bin->op());
        result.append("') {\n");
        result.append(dump_expr(bin->lhs(), indent + 1));
        result.append(dump_expr(bin->rhs(), indent + 1));
        result.append(pad);
        result.append("}\n");
        return result;
    }
    default:
        return pad + "Expr\n";
    }
}

/** @brief Returns a string of spaces for the given indentation level.
 *  @param level The indentation level (each level produces 2 spaces).
 *  @return A string of spaces for visual indentation. */
String ASTDumper::indent_str(int level) const {
    return String(indent_width(level), ' ');
}

}  // namespace prism::ast
