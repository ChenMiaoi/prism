#include "prism/ast/ast_context.hpp"

#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"

namespace prism::ast {

/** @brief Default-constructs an ASTContext with empty owning/non-owning vectors. */
ASTContext::ASTContext() = default;

/** @brief Destroys the ASTContext and frees all owned AST nodes. */
ASTContext::~ASTContext() = default;

}  // namespace prism::ast
