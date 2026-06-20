#include "prism/sema/semantic_analyzer.hpp"

#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"
#include "prism/core/format/format.hpp"

namespace prism::sema {
namespace {

/** @brief Checks whether a type name denotes a reference type (ends with '&'). */
bool is_reference_type_name(const String& type_name) {
    return !type_name.empty() && type_name.back() == '&';
}

/** @brief Strips the trailing '&' from a reference type name to obtain the base type. */
String base_type_name(const String& type_name) {
    if (!is_reference_type_name(type_name)) return String(type_name.c_str());
    return String(type_name.c_str(), type_name.size() - 1);
}

/** @brief Returns @c true if @p lhs and @p rhs share the same base (non-reference) type name. */
bool same_value_type(const TypeInfo& lhs, const TypeInfo& rhs) {
    return base_type_name(lhs.name) == base_type_name(rhs.name);
}

/** @brief Returns @c true if @p expr is an lvalue (currently only identifier expressions). */
bool is_lvalue_expr(ast::Expr* expr) {
    return dynamic_cast<ast::IdentifierExpr*>(expr) != nullptr;
}

}  // namespace

/** @brief Constructs a SemanticAnalyzer with the given diagnostics engine for error reporting. */
SemanticAnalyzer::SemanticAnalyzer(DiagnosticsEngine& diag) : diag_(diag) {
}

/** @brief Analyzes the entire translation unit @p tu, performing name binding and type checking.
 *  @param tu The translation unit declaration to analyze.
 *  @return An Expected<void> indicating success or the first semantic error encountered.
 */
prism::Expected<void, prism::Error> SemanticAnalyzer::analyze(ast::TranslationUnitDecl* tu) {
    scopes_.push_scope();
    for (size_t i = 0; i < tu->decls().size(); ++i) {
        if (tu->decls()[i]->kind() == ast::DeclKind::Class || tu->decls()[i]->kind() == ast::DeclKind::Struct) {
            auto* record = static_cast<ast::RecordDecl*>(tu->decls()[i]);
            TypeInfo record_type{String(record->name().c_str())};
            if (!scopes_.insert(record->name().c_str(), record_type)) {
                auto error = make_error(prism::format("redefinition of type '{}'", record->name().c_str()).c_str(),
                                        record->loc());
                diag_.report(DiagLevel::Error, error.message(), record->loc());
                scopes_.pop_scope();
                return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
            }
            continue;
        }
        if (tu->decls()[i]->kind() != ast::DeclKind::Function) continue;
        auto* fn = static_cast<ast::FunctionDecl*>(tu->decls()[i]);
        auto return_type = resolve_type(fn->return_type().c_str());
        if (!return_type) {
            scopes_.pop_scope();
            return prism::Unexpected<prism::Error>(return_type.error());
        }
        TypeInfo fn_type = return_type.value();
        fn_type.is_function = true;
        fn_type.overload_count = 1;
        fn_type.overload_return_types[0] = String(fn_type.name.c_str());
        for (size_t param = 0; param < fn->params().size() && param < 16; ++param) {
            fn_type.param_types[param] = String(fn->params()[param]->type_name().c_str());
            fn_type.overload_param_types[0][param] = String(fn->params()[param]->type_name().c_str());
            ++fn_type.param_count;
            ++fn_type.overload_param_counts[0];
        }
        if (auto* existing = scopes_.lookup(fn->name().c_str())) {
            if (!existing->is_function || existing->overload_count >= 8) {
                auto error = make_error(prism::format("redefinition of '{}'", fn->name().c_str()).c_str(), fn->loc());
                diag_.report(DiagLevel::Error, error.message(), fn->loc());
                scopes_.pop_scope();
                return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
            }
            for (size_t overload = 0; overload < existing->overload_count; ++overload) {
                bool same_signature = existing->overload_param_counts[overload] == fn_type.param_count;
                for (size_t param = 0; same_signature && param < fn_type.param_count; ++param) {
                    same_signature = existing->overload_param_types[overload][param] == fn_type.param_types[param];
                }
                if (same_signature) {
                    auto error = make_error(prism::format("redefinition of function '{}'", fn->name().c_str()).c_str(),
                                            fn->loc());
                    diag_.report(DiagLevel::Error, error.message(), fn->loc());
                    scopes_.pop_scope();
                    return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
                }
            }
            size_t slot = existing->overload_count++;
            existing->overload_return_types[slot] = String(fn_type.name.c_str());
            existing->overload_param_counts[slot] = fn_type.param_count;
            for (size_t param = 0; param < fn_type.param_count; ++param) {
                existing->overload_param_types[slot][param] = fn_type.param_types[param];
            }
            continue;
        }
        if (!scopes_.insert(fn->name().c_str(), fn_type)) {
            auto error =
                make_error(prism::format("redefinition of function '{}'", fn->name().c_str()).c_str(), fn->loc());
            diag_.report(DiagLevel::Error, error.message(), fn->loc());
            scopes_.pop_scope();
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
    }
    for (size_t i = 0; i < tu->decls().size(); ++i) {
        if (tu->decls()[i]->kind() == ast::DeclKind::Function) {
            auto result = analyze_function(static_cast<ast::FunctionDecl*>(tu->decls()[i]));
            if (!result) {
                scopes_.pop_scope();
                return prism::Unexpected<prism::Error>(result.error());
            }
        }
    }
    scopes_.pop_scope();
    return {};
}

/** @brief Analyzes a single function declaration, including parameter binding and body validation.
 *  @param fn The function declaration to analyze.
 *  @return An Expected<void> indicating success or a semantic error.
 */
prism::Expected<void, prism::Error> SemanticAnalyzer::analyze_function(ast::FunctionDecl* fn) {
    scopes_.push_scope();

    for (size_t i = 0; i < fn->params().size(); ++i) {
        auto type_result = resolve_type(fn->params()[i]->type_name().c_str());
        if (!type_result) return prism::Unexpected<prism::Error>(type_result.error());
        if (!scopes_.insert(fn->params()[i]->name().c_str(), type_result.value())) {
            auto error = make_error(prism::format("redefinition of '{}'", fn->params()[i]->name().c_str()).c_str(),
                                    fn->params()[i]->loc());
            diag_.report(DiagLevel::Error, error.message(), fn->params()[i]->loc());
            scopes_.pop_scope();
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
    }

    if (fn->has_body()) {
        auto result = analyze_stmt(fn->body());
        if (!result) {
            scopes_.pop_scope();
            return prism::Unexpected<prism::Error>(result.error());
        }
        auto function_return_type = resolve_type(fn->return_type().c_str());
        if (!function_return_type) {
            scopes_.pop_scope();
            return prism::Unexpected<prism::Error>(function_return_type.error());
        }
        auto ret_check = validate_return_type(fn->body(), function_return_type.value());
        if (!ret_check) {
            scopes_.pop_scope();
            return prism::Unexpected<prism::Error>(ret_check.error());
        }
    }

    scopes_.pop_scope();
    return {};
}

/** @brief Analyzes a statement, recursively checking sub-statements and expressions.
 *  @param stmt The statement to analyze.
 *  @return An Expected<void> indicating success or a semantic error.
 */
prism::Expected<void, prism::Error> SemanticAnalyzer::analyze_stmt(ast::Stmt* stmt) {
    switch (stmt->kind()) {
    case ast::StmtKind::Block: {
        auto* block = static_cast<ast::BlockStmt*>(stmt);
        for (size_t i = 0; i < block->stmts().size(); ++i) {
            auto result = analyze_stmt(block->stmts()[i]);
            if (!result) return prism::Unexpected<prism::Error>(result.error());
        }
        return {};
    }
    case ast::StmtKind::Return: {
        auto* ret = static_cast<ast::ReturnStmt*>(stmt);
        if (ret->has_value()) {
            auto result = analyze_expr(ret->value());
            if (!result) return prism::Unexpected<prism::Error>(result.error());
        }
        return {};
    }
    case ast::StmtKind::If: {
        auto* if_stmt = static_cast<ast::IfStmt*>(stmt);
        auto cond_result = analyze_expr(if_stmt->condition());
        if (!cond_result) return prism::Unexpected<prism::Error>(cond_result.error());
        auto then_result = analyze_stmt(if_stmt->then_stmt());
        if (!then_result) return prism::Unexpected<prism::Error>(then_result.error());
        if (if_stmt->has_else()) {
            auto else_result = analyze_stmt(if_stmt->else_stmt());
            if (!else_result) return prism::Unexpected<prism::Error>(else_result.error());
        }
        return {};
    }
    case ast::StmtKind::While: {
        auto* while_stmt = static_cast<ast::WhileStmt*>(stmt);
        auto cond_result = analyze_expr(while_stmt->condition());
        if (!cond_result) return prism::Unexpected<prism::Error>(cond_result.error());
        return analyze_stmt(while_stmt->body());
    }
    case ast::StmtKind::For: {
        auto* for_stmt = static_cast<ast::ForStmt*>(stmt);
        scopes_.push_scope();
        if (for_stmt->init()) {
            auto init_result = analyze_stmt(for_stmt->init());
            if (!init_result) return prism::Unexpected<prism::Error>(init_result.error());
        }
        if (for_stmt->condition()) {
            auto cond_result = analyze_expr(for_stmt->condition());
            if (!cond_result) return prism::Unexpected<prism::Error>(cond_result.error());
        }
        if (for_stmt->increment()) {
            auto inc_result = analyze_expr(for_stmt->increment());
            if (!inc_result) return prism::Unexpected<prism::Error>(inc_result.error());
        }
        auto body_result = analyze_stmt(for_stmt->body());
        scopes_.pop_scope();
        return body_result;
    }
    case ast::StmtKind::VariableDecl: {
        auto* var = static_cast<ast::VarDeclStmt*>(stmt);
        auto type_result = resolve_type(var->type_name().c_str());
        if (!type_result) return prism::Unexpected<prism::Error>(type_result.error());

        if (type_result.value().is_reference && !var->init_expr()) {
            auto error =
                make_error(prism::format("reference '{}' must be initialized", var->var_name().c_str()).c_str(),
                           stmt->loc());
            diag_.report(DiagLevel::Error, error.message(), stmt->loc());
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
        if (type_result.value().is_reference && var->init_expr() && !is_lvalue_expr(var->init_expr())) {
            auto error =
                make_error(prism::format("reference '{}' must bind to an lvalue", var->var_name().c_str()).c_str(),
                           var->init_expr()->loc());
            diag_.report(DiagLevel::Error, error.message(), var->init_expr()->loc());
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
        if (!scopes_.insert(var->var_name().c_str(), type_result.value())) {
            return prism::Unexpected<prism::Error>(
                make_error(prism::format("redefinition of '{}'", var->var_name().c_str()).c_str(), stmt->loc()));
        }

        if (var->init_expr()) {
            auto init_result = analyze_expr(var->init_expr());
            if (!init_result) return prism::Unexpected<prism::Error>(init_result.error());
            if (!same_value_type(type_result.value(), init_result.value())) {
                auto error =
                    make_error(prism::format("initializer type '{}' does not match variable type '{}'",
                                             init_result.value().name.c_str(), type_result.value().name.c_str())
                                   .c_str(),
                               var->init_expr()->loc());
                diag_.report(DiagLevel::Error, error.message(), var->init_expr()->loc());
                return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
            }
        }
        return {};
    }
    case ast::StmtKind::Expression: {
        auto* expr_stmt = static_cast<ast::ExprStmt*>(stmt);
        auto result = analyze_expr(expr_stmt->expr());
        if (!result) return prism::Unexpected<prism::Error>(result.error());
        return {};
    }
    default:
        return {};
    }
}

/** @brief Analyzes an expression and returns its resolved type.
 *  @param expr The expression to analyze.
 *  @return An Expected<TypeInfo> containing the expression's type, or a semantic error.
 */
prism::Expected<TypeInfo, prism::Error> SemanticAnalyzer::analyze_expr(ast::Expr* expr) {
    switch (expr->kind()) {
    case ast::ExprKind::IntegerLiteral:
        return TypeInfo{String("int")};
    case ast::ExprKind::FloatLiteral:
        return TypeInfo{String("double")};
    case ast::ExprKind::BoolLiteral:
        return TypeInfo{String("bool")};
    case ast::ExprKind::CharLiteral:
        return TypeInfo{String("char")};
    case ast::ExprKind::StringLiteral:
        return TypeInfo{String("string")};
    case ast::ExprKind::Identifier: {
        auto* id = static_cast<ast::IdentifierExpr*>(expr);
        auto type = scopes_.lookup(id->name().c_str());
        if (!type) {
            return prism::Unexpected<prism::Error>(
                make_error(prism::format("undeclared identifier '{}'", id->name().c_str()).c_str(), expr->loc()));
        }
        return *type;
    }
    case ast::ExprKind::BinaryOp: {
        auto* bin = static_cast<ast::BinaryExpr*>(expr);
        auto lhs_result = analyze_expr(bin->lhs());
        if (!lhs_result) return prism::Unexpected<prism::Error>(lhs_result.error());
        auto rhs_result = analyze_expr(bin->rhs());
        if (!rhs_result) return prism::Unexpected<prism::Error>(rhs_result.error());
        return lhs_result.value();
    }
    case ast::ExprKind::UnaryOp: {
        auto* unary = static_cast<ast::UnaryExpr*>(expr);
        return analyze_expr(unary->operand());
    }
    case ast::ExprKind::Assignment: {
        auto* assign = static_cast<ast::AssignmentExpr*>(expr);
        auto target_result = analyze_expr(assign->target());
        if (!target_result) return prism::Unexpected<prism::Error>(target_result.error());
        auto value_result = analyze_expr(assign->value());
        if (!value_result) return prism::Unexpected<prism::Error>(value_result.error());
        if (!same_value_type(target_result.value(), value_result.value())) {
            auto error = make_error(prism::format("cannot assign '{}' to '{}'", value_result.value().name.c_str(),
                                                  target_result.value().name.c_str())
                                        .c_str(),
                                    assign->value()->loc());
            diag_.report(DiagLevel::Error, error.message(), assign->value()->loc());
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
        return target_result.value();
    }
    case ast::ExprKind::Call: {
        auto* call = static_cast<ast::CallExpr*>(expr);
        auto callee_result = analyze_expr(call->callee());
        if (!callee_result) return prism::Unexpected<prism::Error>(callee_result.error());
        if (!callee_result.value().is_function) {
            auto error = make_error("called object is not a function", expr->loc());
            diag_.report(DiagLevel::Error, error.message(), expr->loc());
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
        int matching_overload = -1;
        bool saw_arity_match = false;
        for (size_t overload = 0; overload < callee_result.value().overload_count; ++overload) {
            if (callee_result.value().overload_param_counts[overload] != call->args().size()) continue;
            saw_arity_match = true;
            bool matches = true;
            for (size_t i = 0; i < call->args().size(); ++i) {
                auto arg_result = analyze_expr(call->args()[i]);
                if (!arg_result) return prism::Unexpected<prism::Error>(arg_result.error());
                auto expected_type = resolve_type(callee_result.value().overload_param_types[overload][i].c_str());
                if (!expected_type) return prism::Unexpected<prism::Error>(expected_type.error());
                if (expected_type.value().is_reference && !is_lvalue_expr(call->args()[i])) {
                    matches = false;
                    break;
                }
                if (!same_value_type(arg_result.value(), expected_type.value())) {
                    matches = false;
                    break;
                }
            }
            if (!matches) continue;
            if (matching_overload != -1) {
                auto error = make_error("ambiguous overloaded function call", expr->loc());
                diag_.report(DiagLevel::Error, error.message(), expr->loc());
                return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
            }
            matching_overload = static_cast<int>(overload);
        }
        if (matching_overload == -1) {
            auto error =
                make_error(saw_arity_match ? "no matching overload for function call"
                                           : prism::format("function call expects different argument count, got {}",
                                                           static_cast<int>(call->args().size()))
                                                 .c_str(),
                           expr->loc());
            diag_.report(DiagLevel::Error, error.message(), expr->loc());
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
        return TypeInfo{String(callee_result.value().overload_return_types[matching_overload].c_str())};
    }
    default:
        return TypeInfo{String("void")};
    }
}

/** @brief Validates that all return statements in @p stmt are compatible with the expected function return type.
 *  @param stmt The statement tree to validate.
 *  @param type The expected return type of the enclosing function.
 *  @return An Expected<void> indicating success or a type mismatch error.
 */
prism::Expected<void, prism::Error> SemanticAnalyzer::validate_return_type(ast::Stmt* stmt, const TypeInfo& type) {
    if (stmt->kind() == ast::StmtKind::Return) {
        auto* ret = static_cast<ast::ReturnStmt*>(stmt);
        if (!ret->has_value() && type.name != "void") {
            auto error =
                make_error(prism::format("non-void function must return '{}'", type.name.c_str()).c_str(), stmt->loc());
            diag_.report(DiagLevel::Error, error.message(), stmt->loc());
            return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
        }
        if (ret->has_value()) {
            auto value_type = analyze_expr(ret->value());
            if (!value_type) return prism::Unexpected<prism::Error>(value_type.error());
            if (type.name == "void") {
                auto error = make_error("void function cannot return a value", stmt->loc());
                diag_.report(DiagLevel::Error, error.message(), stmt->loc());
                return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
            }
            if (!same_value_type(value_type.value(), type)) {
                auto error = make_error(prism::format("return type '{}' does not match function return type '{}'",
                                                      value_type.value().name.c_str(), type.name.c_str())
                                            .c_str(),
                                        ret->value()->loc());
                diag_.report(DiagLevel::Error, error.message(), ret->value()->loc());
                return prism::Unexpected<prism::Error>(static_cast<Error&&>(error));
            }
        }
        return {};
    }
    if (stmt->kind() == ast::StmtKind::Block) {
        auto* block = static_cast<ast::BlockStmt*>(stmt);
        for (size_t i = 0; i < block->stmts().size(); ++i) {
            auto result = validate_return_type(block->stmts()[i], type);
            if (!result) return prism::Unexpected<prism::Error>(result.error());
        }
    }
    if (stmt->kind() == ast::StmtKind::If) {
        auto* if_stmt = static_cast<ast::IfStmt*>(stmt);
        auto then_result = validate_return_type(if_stmt->then_stmt(), type);
        if (!then_result) return prism::Unexpected<prism::Error>(then_result.error());
        if (if_stmt->has_else()) return validate_return_type(if_stmt->else_stmt(), type);
    }
    return {};
}

/** @brief Resolves a type name string into a TypeInfo, handling builtins and user-defined types.
 *  @param type_name The type name string to resolve (may include trailing '&' for references).
 *  @return An Expected<TypeInfo> containing the resolved type, or an error if the type is unknown.
 */
prism::Expected<TypeInfo, prism::Error> SemanticAnalyzer::resolve_type(const char* type_name) {
    String requested(type_name);
    bool is_reference = is_reference_type_name(requested);
    String base_name = base_type_name(requested);
    static const char* builtin_types[] = {"int",    "long", "short", "unsigned", "signed", "float",
                                          "double", "char", "bool",  "void",     "string"};

    for (int i = 0; i < 11; ++i) {
        if (base_name == builtin_types[i]) {
            if (is_reference && base_name == "void") {
                return prism::Unexpected<prism::Error>(make_error("reference to void is invalid"));
            }
            TypeInfo type{String(base_name.c_str())};
            type.is_reference = is_reference;
            if (is_reference) type.name.append("&");
            return type;
        }
    }

    auto type = scopes_.lookup(base_name.c_str());
    if (type) {
        TypeInfo resolved = *type;
        resolved.is_reference = is_reference;
        resolved.name = String(base_name.c_str());
        if (is_reference) resolved.name.append("&");
        return resolved;
    }

    return prism::Unexpected<prism::Error>(make_error(prism::format("unknown type '{}'", type_name).c_str()));
}

}  // namespace prism::sema
