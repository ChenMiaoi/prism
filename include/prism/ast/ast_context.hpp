#pragma once

#include "prism/basic/source_location.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/memory/unique_ptr.hpp"
#include "prism/core/string/string.hpp"

namespace prism::ast {

class Decl;
class Stmt;
class Expr;

/** @brief Owns and manages all AST nodes created during compilation. */
class ASTContext {
public:
    /** @brief Constructs an AST context. */
    ASTContext();

    /** @brief Destructs an AST context and frees all owned AST nodes. */
    ~ASTContext();

    ASTContext(const ASTContext&) = delete;
    ASTContext& operator=(const ASTContext&) = delete;

    /** @brief Creates a new declaration node and registers ownership.
     *  @tparam T    The declaration type to create (must derive from Decl).
     *  @tparam Args Constructor argument types.
     *  @param args  Arguments forwarded to T's constructor.
     *  @return A raw pointer to the newly created declaration. */
    template <typename T, typename... Args>
    T* create(Args&&... args) {
        auto ptr = prism::make_unique<T>(prism::forward<Args>(args)...);
        T* raw = ptr.get();
        decls_raw_.push_back(raw);
        decls_owned_.push_back(prism::UniquePtr<Decl>(ptr.release()));
        return raw;
    }

    /** @brief Creates a new statement node and registers ownership.
     *  @tparam T    The statement type to create (must derive from Stmt).
     *  @tparam Args Constructor argument types.
     *  @param args  Arguments forwarded to T's constructor.
     *  @return A raw pointer to the newly created statement. */
    template <typename T, typename... Args>
    T* create_stmt(Args&&... args) {
        auto ptr = prism::make_unique<T>(prism::forward<Args>(args)...);
        T* raw = ptr.get();
        stmts_raw_.push_back(raw);
        stmts_owned_.push_back(prism::UniquePtr<Stmt>(ptr.release()));
        return raw;
    }

    /** @brief Creates a new expression node and registers ownership.
     *  @tparam T    The expression type to create (must derive from Expr).
     *  @tparam Args Constructor argument types.
     *  @param args  Arguments forwarded to T's constructor.
     *  @return A raw pointer to the newly created expression. */
    template <typename T, typename... Args>
    T* create_expr(Args&&... args) {
        auto ptr = prism::make_unique<T>(prism::forward<Args>(args)...);
        T* raw = ptr.get();
        exprs_raw_.push_back(raw);
        exprs_owned_.push_back(prism::UniquePtr<Expr>(ptr.release()));
        return raw;
    }

    /** @brief Returns the mutable list of all declarations.
     *  @return Reference to the declaration vector. */
    prism::Vector<Decl*>& decls() { return decls_raw_; }

    /** @brief Returns the mutable list of all statements.
     *  @return Reference to the statement vector. */
    prism::Vector<Stmt*>& stmts() { return stmts_raw_; }

    /** @brief Returns the mutable list of all expressions.
     *  @return Reference to the expression vector. */
    prism::Vector<Expr*>& exprs() { return exprs_raw_; }

private:
    prism::Vector<Decl*> decls_raw_;
    prism::Vector<prism::UniquePtr<Decl>> decls_owned_;
    prism::Vector<Stmt*> stmts_raw_;
    prism::Vector<prism::UniquePtr<Stmt>> stmts_owned_;
    prism::Vector<Expr*> exprs_raw_;
    prism::Vector<prism::UniquePtr<Expr>> exprs_owned_;
};

}  // namespace prism::ast
