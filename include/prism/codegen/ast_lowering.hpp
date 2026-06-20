#pragma once

#include "prism/basic/error.hpp"
#include "prism/core/container/hash_map.hpp"
#include "prism/core/string/string.hpp"
#include "prism/ir/context.hpp"
#include "prism/ir/ir_builder.hpp"
#include "prism/ir/module.hpp"

namespace prism::ast {
class TranslationUnitDecl;
class FunctionDecl;
class Stmt;
class Expr;
class BlockStmt;
}  // namespace prism::ast

namespace prism::codegen {

/** @brief Lowers an AST into the IR representation for code generation. */
class ASTLowering {
public:
    /** @brief Constructs an ASTLowering instance with the given IR context. */
    ASTLowering(prism::ir::Context& ctx);

    /** @brief Lowers a translation unit into an IR module. @return The resulting IR module, or an error. */
    prism::Expected<prism::UniquePtr<prism::ir::Module>> lower(ast::TranslationUnitDecl* tu);

private:
    /** @brief Lowers a function declaration into IR. @return Success or an error. */
    prism::Expected<void> lower_function(ast::FunctionDecl* fn);

    /** @brief Lowers a single statement into IR. @return Success or an error. */
    prism::Expected<void> lower_stmt(ast::Stmt* stmt);

    /** @brief Lowers a block statement into IR. @return Success or an error. */
    prism::Expected<void> lower_block_stmt(ast::BlockStmt* block);

    /** @brief Lowers an expression into an IR value. @return The resulting IR Value, or an error. */
    prism::Expected<prism::ir::Value*> lower_expr(ast::Expr* expr);

    prism::ir::Context& ctx_;
    prism::UniquePtr<prism::ir::Module> module_;
    prism::ir::IRBuilder builder_;
    prism::HashMap<prism::String, prism::ir::Value*> named_values_;
    prism::ir::Function* current_function_ = nullptr;
};

}  // namespace prism::codegen
