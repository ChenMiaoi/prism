#include "prism/codegen/ast_lowering.hpp"

#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"
#include "prism/core/format/format.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/type.hpp"

namespace prism::codegen {
namespace {

/** @brief Checks whether a type name denotes a reference type (ends with '&'). */
bool is_reference_type(const String& type_name) {
    return !type_name.empty() && type_name.back() == '&';
}

}  // namespace

/** @brief Constructs an ASTLowering instance with the given IR context.
 *  @param ctx The IR context used to create types and constants.
 */
ASTLowering::ASTLowering(ir::Context& ctx)
    : ctx_(ctx), module_(prism::make_unique<ir::Module>("main", ctx)), builder_(ctx) {
}

/** @brief Lowers an entire translation unit into an IR module.
 *  @param tu The translation unit declaration to lower.
 *  @return An Expected<UniquePtr<ir::Module>> containing the generated module, or an error.
 */
Expected<UniquePtr<ir::Module>> ASTLowering::lower(ast::TranslationUnitDecl* tu) {
    for (size_t i = 0; i < tu->decls().size(); ++i) {
        if (tu->decls()[i]->kind() == ast::DeclKind::Function) {
            auto* fn = static_cast<ast::FunctionDecl*>(tu->decls()[i]);
            auto result = lower_function(fn);
            if (!result) return Unexpected<Error>(result.error());
        }
    }
    return Expected<UniquePtr<ir::Module>>(static_cast<UniquePtr<ir::Module>&&>(module_));
}

/** @brief Lowers a single function declaration into IR, including parameter allocation and body.
 *  @param fn The function declaration to lower.
 *  @return An Expected<void> indicating success or an error.
 */
Expected<void> ASTLowering::lower_function(ast::FunctionDecl* fn) {
    ir::Type* ret_type = ctx_.i64_type();
    if (fn->return_type() == "void")
        ret_type = ctx_.void_type();
    else if (fn->return_type() == "int" || fn->return_type() == "long")
        ret_type = ctx_.i64_type();
    else if (fn->return_type() == "float")
        ret_type = ctx_.float_type();
    else if (fn->return_type() == "double")
        ret_type = ctx_.double_type();

    Vector<ir::Type*> param_types;
    for (size_t i = 0; i < fn->params().size(); ++i) {
        if (fn->params()[i]->type_name() == "int" || fn->params()[i]->type_name() == "long")
            param_types.push_back(ctx_.i64_type());
        else if (fn->params()[i]->type_name() == "float")
            param_types.push_back(ctx_.float_type());
        else if (fn->params()[i]->type_name() == "double")
            param_types.push_back(ctx_.double_type());
        else
            param_types.push_back(ctx_.i64_type());
    }

    auto* fn_type = ctx_.get_function_type(ret_type, static_cast<Vector<ir::Type*>&&>(param_types));
    auto* func = module_->get_or_insert_function(fn->name().c_str(), fn_type);
    current_function_ = func;
    named_values_.clear();

    if (fn->has_body()) {
        auto* entry = func->create_block("entry");
        builder_.set_insert_point(entry);

        for (size_t i = 0; i < func->args().size() && i < fn->params().size(); ++i) {
            func->args()[i]->set_name(fn->params()[i]->name().c_str());
            auto* alloca = builder_.create_alloca(func->args()[i]->get_type(), fn->params()[i]->name().c_str());
            builder_.create_store(func->args()[i].get(), alloca);
            named_values_[fn->params()[i]->name()] = alloca;
        }

        auto result = lower_stmt(fn->body());
        if (!result) return Unexpected<Error>(result.error());

        if (!builder_.get_insert_block()->is_terminated()) {
            if (ret_type->is_void_type())
                builder_.create_ret_void();
            else
                builder_.create_ret(ctx_.get_int_constant(0));
        }
    }
    return {};
}

/** @brief Lowers a statement to IR instructions.
 *  @param stmt The statement to lower.
 *  @return An Expected<void> indicating success or an error.
 */
Expected<void> ASTLowering::lower_stmt(ast::Stmt* stmt) {
    switch (stmt->kind()) {
    case ast::StmtKind::Block:
        return lower_block_stmt(static_cast<ast::BlockStmt*>(stmt));
    case ast::StmtKind::Return: {
        auto* ret = static_cast<ast::ReturnStmt*>(stmt);
        if (ret->has_value()) {
            auto val = lower_expr(ret->value());
            if (!val) return Unexpected<Error>(val.error());
            builder_.create_ret(val.value());
        } else {
            builder_.create_ret_void();
        }
        return {};
    }
    case ast::StmtKind::If: {
        auto* if_stmt = static_cast<ast::IfStmt*>(stmt);
        auto cond = lower_expr(if_stmt->condition());
        if (!cond) return Unexpected<Error>(cond.error());
        auto* then_bb = current_function_->create_block("if.then");
        auto* merge_bb = current_function_->create_block("if.end");
        if (if_stmt->has_else()) {
            auto* else_bb = current_function_->create_block("if.else");
            builder_.create_cond_br(cond.value(), then_bb, else_bb);
            builder_.set_insert_point(then_bb);
            auto then_result = lower_stmt(if_stmt->then_stmt());
            if (!then_result) return Unexpected<Error>(then_result.error());
            if (!builder_.get_insert_block()->is_terminated()) builder_.create_br(merge_bb);
            builder_.set_insert_point(else_bb);
            auto else_result = lower_stmt(if_stmt->else_stmt());
            if (!else_result) return Unexpected<Error>(else_result.error());
            if (!builder_.get_insert_block()->is_terminated()) builder_.create_br(merge_bb);
        } else {
            builder_.create_cond_br(cond.value(), then_bb, merge_bb);
            builder_.set_insert_point(then_bb);
            auto then_result = lower_stmt(if_stmt->then_stmt());
            if (!then_result) return Unexpected<Error>(then_result.error());
            if (!builder_.get_insert_block()->is_terminated()) builder_.create_br(merge_bb);
        }
        builder_.set_insert_point(merge_bb);
        return {};
    }
    case ast::StmtKind::While: {
        auto* while_stmt = static_cast<ast::WhileStmt*>(stmt);
        auto* cond_bb = current_function_->create_block("while.cond");
        auto* body_bb = current_function_->create_block("while.body");
        auto* end_bb = current_function_->create_block("while.end");
        builder_.create_br(cond_bb);
        builder_.set_insert_point(cond_bb);
        auto cond = lower_expr(while_stmt->condition());
        if (!cond) return Unexpected<Error>(cond.error());
        builder_.create_cond_br(cond.value(), body_bb, end_bb);
        builder_.set_insert_point(body_bb);
        auto body_result = lower_stmt(while_stmt->body());
        if (!body_result) return Unexpected<Error>(body_result.error());
        if (!builder_.get_insert_block()->is_terminated()) builder_.create_br(cond_bb);
        builder_.set_insert_point(end_bb);
        return {};
    }
    case ast::StmtKind::For: {
        auto* for_stmt = static_cast<ast::ForStmt*>(stmt);
        auto* cond_bb = current_function_->create_block("for.cond");
        auto* body_bb = current_function_->create_block("for.body");
        auto* inc_bb = current_function_->create_block("for.inc");
        auto* end_bb = current_function_->create_block("for.end");
        if (for_stmt->init()) {
            auto r = lower_stmt(for_stmt->init());
            if (!r) return Unexpected<Error>(r.error());
        }
        builder_.create_br(cond_bb);
        builder_.set_insert_point(cond_bb);
        if (for_stmt->condition()) {
            auto cond = lower_expr(for_stmt->condition());
            if (!cond) return Unexpected<Error>(cond.error());
            builder_.create_cond_br(cond.value(), body_bb, end_bb);
        } else {
            builder_.create_br(body_bb);
        }
        builder_.set_insert_point(body_bb);
        auto body_result = lower_stmt(for_stmt->body());
        if (!body_result) return Unexpected<Error>(body_result.error());
        if (!builder_.get_insert_block()->is_terminated()) builder_.create_br(inc_bb);
        builder_.set_insert_point(inc_bb);
        if (for_stmt->increment()) {
            auto inc = lower_expr(for_stmt->increment());
            if (!inc) return Unexpected<Error>(inc.error());
        }
        builder_.create_br(cond_bb);
        builder_.set_insert_point(end_bb);
        return {};
    }
    case ast::StmtKind::VariableDecl: {
        auto* var = static_cast<ast::VarDeclStmt*>(stmt);
        if (is_reference_type(var->type_name())) {
            if (!var->init_expr() || var->init_expr()->kind() != ast::ExprKind::Identifier) {
                return Unexpected<Error>(make_error("reference initializer must be an identifier", stmt->loc()));
            }
            auto* id = static_cast<ast::IdentifierExpr*>(var->init_expr());
            auto target = named_values_.find(id->name());
            if (!target)
                return Unexpected<Error>(make_error(
                    prism::format("undeclared variable '{}'", id->name().c_str()).c_str(), var->init_expr()->loc()));
            named_values_[var->var_name()] = target->value;
            return {};
        }
        auto* alloca = builder_.create_alloca(ctx_.i64_type(), var->var_name().c_str());
        named_values_[var->var_name()] = alloca;
        if (var->init_expr()) {
            auto init_val = lower_expr(var->init_expr());
            if (!init_val) return Unexpected<Error>(init_val.error());
            builder_.create_store(init_val.value(), alloca);
        }
        return {};
    }
    case ast::StmtKind::Expression: {
        auto* expr_stmt = static_cast<ast::ExprStmt*>(stmt);
        auto val = lower_expr(expr_stmt->expr());
        if (!val) return Unexpected<Error>(val.error());
        return {};
    }
    default:
        return {};
    }
}

/** @brief Lowers a block statement, processing each sub-statement sequentially.
 *  @param block The block statement to lower.
 *  @return An Expected<void> indicating success or an error.
 */
Expected<void> ASTLowering::lower_block_stmt(ast::BlockStmt* block) {
    for (size_t i = 0; i < block->stmts().size(); ++i) {
        auto result = lower_stmt(block->stmts()[i]);
        if (!result) return Unexpected<Error>(result.error());
        if (builder_.get_insert_block()->is_terminated()) break;
    }
    return {};
}

/** @brief Lowers an expression to an IR value.
 *  @param expr The expression to lower.
 *  @return An Expected<ir::Value*> containing the resulting IR value, or an error.
 */
Expected<ir::Value*> ASTLowering::lower_expr(ast::Expr* expr) {
    switch (expr->kind()) {
    case ast::ExprKind::IntegerLiteral: {
        auto* lit = static_cast<ast::IntegerLiteralExpr*>(expr);
        return ctx_.get_int_constant(lit->value());
    }
    case ast::ExprKind::FloatLiteral: {
        auto* lit = static_cast<ast::FloatLiteralExpr*>(expr);
        return ctx_.get_float_constant(lit->value());
    }
    case ast::ExprKind::BoolLiteral: {
        auto* lit = static_cast<ast::BoolLiteralExpr*>(expr);
        return ctx_.get_int_constant(lit->value() ? 1 : 0, ctx_.i1_type());
    }
    case ast::ExprKind::Identifier: {
        auto* id = static_cast<ast::IdentifierExpr*>(expr);
        auto it = named_values_.find(id->name());
        if (!it)
            return Unexpected<Error>(
                make_error(prism::format("undeclared variable '{}'", id->name().c_str()).c_str(), expr->loc()));
        return builder_.create_load(ctx_.i64_type(), it->value, id->name().c_str());
    }
    case ast::ExprKind::BinaryOp: {
        auto* bin = static_cast<ast::BinaryExpr*>(expr);
        auto lhs_result = lower_expr(bin->lhs());
        if (!lhs_result) return Unexpected<Error>(lhs_result.error());
        auto rhs_result = lower_expr(bin->rhs());
        if (!rhs_result) return Unexpected<Error>(rhs_result.error());
        auto* lhs = lhs_result.value();
        auto* rhs = rhs_result.value();
        if (bin->op() == "+") return builder_.create_add(lhs, rhs, "add");
        if (bin->op() == "-") return builder_.create_sub(lhs, rhs, "sub");
        if (bin->op() == "*") return builder_.create_mul(lhs, rhs, "mul");
        if (bin->op() == "/") return builder_.create_sdiv(lhs, rhs, "sdiv");
        if (bin->op() == "%") return builder_.create_srem(lhs, rhs, "srem");
        if (bin->op() == "==") return builder_.create_icmp_eq(lhs, rhs, "cmp");
        if (bin->op() == "!=") return builder_.create_icmp_ne(lhs, rhs, "cmp");
        if (bin->op() == "<") return builder_.create_icmp_slt(lhs, rhs, "cmp");
        if (bin->op() == ">") return builder_.create_icmp_sgt(lhs, rhs, "cmp");
        if (bin->op() == "<=") return builder_.create_icmp_sle(lhs, rhs, "cmp");
        if (bin->op() == ">=") return builder_.create_icmp_sge(lhs, rhs, "cmp");
        return Unexpected<Error>(
            make_error(prism::format("unsupported binary operator '{}'", bin->op().c_str()).c_str(), expr->loc()));
    }
    case ast::ExprKind::UnaryOp: {
        auto* unary = static_cast<ast::UnaryExpr*>(expr);
        auto operand = lower_expr(unary->operand());
        if (!operand) return Unexpected<Error>(operand.error());
        if (unary->op() == "-" && unary->is_prefix()) {
            auto* zero = ctx_.get_int_constant(0);
            return builder_.create_sub(zero, operand.value(), "neg");
        }
        return operand;
    }
    case ast::ExprKind::Assignment: {
        auto* assign = static_cast<ast::AssignmentExpr*>(expr);
        if (assign->target()->kind() != ast::ExprKind::Identifier) {
            return Unexpected<Error>(make_error("assignment target must be an identifier", expr->loc()));
        }
        auto* id = static_cast<ast::IdentifierExpr*>(assign->target());
        auto slot = named_values_.find(id->name());
        if (!slot)
            return Unexpected<Error>(
                make_error(prism::format("undeclared variable '{}'", id->name().c_str()).c_str(), expr->loc()));
        auto value_result = lower_expr(assign->value());
        if (!value_result) return Unexpected<Error>(value_result.error());
        builder_.create_store(value_result.value(), slot->value);
        return value_result.value();
    }
    case ast::ExprKind::Call: {
        auto* call = static_cast<ast::CallExpr*>(expr);
        auto* callee_expr = call->callee();
        if (callee_expr->kind() != ast::ExprKind::Identifier) {
            return Unexpected<Error>(make_error("invalid call target", expr->loc()));
        }
        auto* id = static_cast<ast::IdentifierExpr*>(callee_expr);
        auto* func = module_->get_function(id->name().c_str());
        if (!func)
            return Unexpected<Error>(
                make_error(prism::format("undeclared function '{}'", id->name().c_str()).c_str(), expr->loc()));
        Vector<ir::Value*> args;
        for (size_t i = 0; i < call->args().size(); ++i) {
            auto val = lower_expr(call->args()[i]);
            if (!val) return Unexpected<Error>(val.error());
            args.push_back(val.value());
        }
        return builder_.create_call(func, static_cast<Vector<ir::Value*>&&>(args), "call");
    }
    default:
        return Unexpected<Error>(make_error("unsupported expression", expr->loc()));
    }
}

}  // namespace prism::codegen
