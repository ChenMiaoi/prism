#include "prism/ast/ast_context.hpp"
#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"
#include "prism/basic/diagnostics.hpp"
#include "prism/basic/error.hpp"
#include "prism/codegen/ast_lowering.hpp"
#include "prism/codegen/x86_64/x86_64_codegen.hpp"
#include "prism/core/format/format.hpp"
#include "prism/frontend/lexer.hpp"
#include "prism/frontend/parser.hpp"
#include "prism/frontend/source_manager.hpp"
#include "prism/ir/context.hpp"
#include "prism/ir/ir_builder.hpp"
#include "prism/ir/ir_printer.hpp"
#include "prism/ir/module.hpp"
#include "prism/ir/verifier.hpp"
#include "prism/opt/pass_manager.hpp"
#include "prism/sema/semantic_analyzer.hpp"

#include <cstdio>
#include <cstring>

auto test_basic_types() -> bool {
    std::fprintf(stdout, "  test_basic_types...");
    prism::ir::Context ctx;
    auto* i32 = ctx.i32_type();
    auto* i64 = ctx.i64_type();
    auto* f32 = ctx.float_type();
    auto* f64 = ctx.double_type();
    if (i32->bit_width() != 32 || i64->bit_width() != 64) return false;
    if (f32->get_size_in_bits() != 32 || f64->get_size_in_bits() != 64) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_ir_context() -> bool {
    std::fprintf(stdout, "  test_ir_context...");
    prism::ir::Context ctx;
    auto* c1 = ctx.get_int_constant(42);
    auto* c2 = ctx.get_int_constant(42);
    if (c1 != c2) return false;
    auto* c3 = ctx.get_int_constant(100);
    if (c1 == c3) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_ir_builder() -> bool {
    std::fprintf(stdout, "  test_ir_builder...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {ctx.i64_type(), ctx.i64_type()});
    auto* fn = module.get_or_insert_function("add", fn_type);
    auto* entry = fn->create_block("entry");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    auto* arg0 = fn->args()[0].get();
    auto* arg1 = fn->args()[1].get();
    auto* add = builder.create_add(arg0, arg1, "result");
    builder.create_ret(add);
    if (fn->is_declaration()) return false;
    if (entry->empty()) return false;
    if (!entry->is_terminated()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_ir_printer() -> bool {
    std::fprintf(stdout, "  test_ir_printer...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {ctx.i64_type()});
    auto* fn = module.get_or_insert_function("double_it", fn_type);
    auto* entry = fn->create_block("entry");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    auto* arg0 = fn->args()[0].get();
    auto* two = ctx.get_int_constant(2);
    auto* mul = builder.create_mul(arg0, two, "doubled");
    builder.create_ret(mul);
    prism::ir::IRPrinter printer;
    auto printed = printer.print_module(&module);
    if (printed.empty()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_ir_verifier() -> bool {
    std::fprintf(stdout, "  test_ir_verifier...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {});
    auto* fn = module.get_or_insert_function("main", fn_type);
    auto* entry = fn->create_block("entry");
    prism::ir::Verifier verifier;
    if (verifier.verify_module(module)) return false;
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    builder.create_ret(ctx.get_int_constant(0));
    if (!verifier.verify_module(module)) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_pass_manager() -> bool {
    std::fprintf(stdout, "  test_pass_manager...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {});
    auto* fn = module.get_or_insert_function("main", fn_type);
    auto* entry = fn->create_block("entry");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    auto* sum = builder.create_add(ctx.get_int_constant(40), ctx.get_int_constant(2), "sum");
    builder.create_ret(sum);
    prism::opt::PassManager passes;
    passes.add_pass(prism::UniquePtr<prism::opt::ModulePass>(new prism::opt::ConstantFoldingPass()));
    passes.add_pass(prism::UniquePtr<prism::opt::ModulePass>(new prism::opt::DeadCodeEliminationPass()));
    auto result = passes.run(module);
    if (!result || !result.value()) return false;
    prism::ir::IRPrinter printer;
    auto printed = printer.print_module(&module);
    if (::strstr(printed.c_str(), "ret i64, 42") == nullptr) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_phi_nodes() -> bool {
    std::fprintf(stdout, "  test_phi_nodes...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {});
    auto* fn = module.get_or_insert_function("choose", fn_type);
    auto* entry = fn->create_block("entry");
    auto* then_bb = fn->create_block("then");
    auto* else_bb = fn->create_block("else");
    auto* merge_bb = fn->create_block("merge");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    builder.create_cond_br(ctx.get_int_constant(1, ctx.i1_type()), then_bb, else_bb);
    builder.set_insert_point(then_bb);
    builder.create_br(merge_bb);
    builder.set_insert_point(else_bb);
    builder.create_br(merge_bb);
    builder.set_insert_point(merge_bb);
    prism::Vector<prism::ir::Value*> values;
    values.push_back(ctx.get_int_constant(1));
    values.push_back(ctx.get_int_constant(2));
    prism::Vector<prism::ir::BasicBlock*> blocks;
    blocks.push_back(then_bb);
    blocks.push_back(else_bb);
    auto* phi = builder.create_phi(ctx.i64_type(), static_cast<prism::Vector<prism::ir::Value*>&&>(values),
                                   static_cast<prism::Vector<prism::ir::BasicBlock*>&&>(blocks), "selected");
    builder.create_ret(phi);
    prism::ir::Verifier verifier;
    if (!verifier.verify_module(module)) return false;
    prism::ir::IRPrinter printer;
    auto printed = printer.print_module(&module);
    if (::strstr(printed.c_str(), "phi i64, [ 1, %then ], [ 2, %else ]") == nullptr) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_mem2reg_pass() -> bool {
    std::fprintf(stdout, "  test_mem2reg_pass...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {});
    auto* fn = module.get_or_insert_function("main", fn_type);
    auto* entry = fn->create_block("entry");
    auto* then_bb = fn->create_block("then");
    auto* else_bb = fn->create_block("else");
    auto* merge_bb = fn->create_block("merge");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    auto* slot = builder.create_alloca(ctx.i64_type(), "x");
    builder.create_store(ctx.get_int_constant(0), slot);
    builder.create_cond_br(ctx.get_int_constant(1, ctx.i1_type()), then_bb, else_bb);
    builder.set_insert_point(then_bb);
    builder.create_store(ctx.get_int_constant(1), slot);
    builder.create_br(merge_bb);
    builder.set_insert_point(else_bb);
    builder.create_store(ctx.get_int_constant(2), slot);
    builder.create_br(merge_bb);
    builder.set_insert_point(merge_bb);
    auto* value = builder.create_load(ctx.i64_type(), slot, "x");
    builder.create_ret(value);

    prism::opt::PassManager passes;
    passes.add_pass(prism::UniquePtr<prism::opt::ModulePass>(new prism::opt::Mem2RegPass()));
    auto result = passes.run(module);
    if (!result || !result.value()) return false;
    prism::ir::Verifier verifier;
    if (!verifier.verify_module(module)) return false;
    prism::ir::IRPrinter printer;
    auto printed = printer.print_module(&module);
    if (::strstr(printed.c_str(), "phi i64, [ 1, %then ], [ 2, %else ]") == nullptr) return false;
    if (::strstr(printed.c_str(), "alloca") != nullptr) return false;
    if (::strstr(printed.c_str(), "load") != nullptr) return false;
    if (::strstr(printed.c_str(), "store") != nullptr) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_code_generation() -> bool {
    std::fprintf(stdout, "  test_code_generation...");
    prism::ir::Context ctx;
    prism::ir::Module module("test", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {ctx.i64_type(), ctx.i64_type()});
    auto* fn = module.get_or_insert_function("add", fn_type);
    auto* entry = fn->create_block("entry");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    auto* a = fn->args()[0].get();
    auto* b = fn->args()[1].get();
    auto* sum = builder.create_add(a, b, "sum");
    builder.create_ret(sum);
    prism::codegen::X86_64CodeGenerator codegen;
    auto assembly = codegen.generate(&module);
    if (::strstr(assembly.c_str(), "_add:") == nullptr) return false;
    if (::strstr(assembly.c_str(), "addq") == nullptr) return false;
    if (::strstr(assembly.c_str(), "# unhandled opcode") != nullptr) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_sema() -> bool {
    std::fprintf(stdout, "  test_sema...");
    prism::DiagnosticsEngine diag;
    prism::sema::SemanticAnalyzer sema(diag);
    prism::ast::ASTContext ast_ctx;
    auto* ret_expr = ast_ctx.create_expr<prism::ast::IntegerLiteralExpr>(42);
    auto* ret_stmt = ast_ctx.create_stmt<prism::ast::ReturnStmt>(ret_expr);
    auto* body = ast_ctx.create_stmt<prism::ast::BlockStmt>();
    body->add_stmt(ret_stmt);
    auto* main_fn = ast_ctx.create<prism::ast::FunctionDecl>("int", "main", prism::Vector<prism::ast::ParamDecl*>{});
    main_fn->set_body(body);
    auto* tu = ast_ctx.create<prism::ast::TranslationUnitDecl>();
    tu->add_decl(main_fn);
    auto result = sema.analyze(tu);
    if (!result) return false;
    if (diag.has_errors()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_full_pipeline() -> bool {
    std::fprintf(stdout, "  test_full_pipeline...");
    prism::ir::Context ctx;
    prism::ir::Module module("factorial", ctx);
    auto* fn_type = ctx.get_function_type(ctx.i64_type(), {ctx.i64_type()});
    auto* fn = module.get_or_insert_function("factorial", fn_type);
    auto* entry = fn->create_block("entry");
    prism::ir::IRBuilder builder(ctx);
    builder.set_insert_point(entry);
    auto* n = fn->args()[0].get();
    builder.create_ret(n);
    prism::ir::IRPrinter printer;
    auto printed = printer.print_module(&module);
    if (printed.empty()) return false;
    prism::codegen::X86_64CodeGenerator codegen;
    auto assembly = codegen.generate(&module);
    if (assembly.empty()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_frontend_lexer_parser() -> bool {
    std::fprintf(stdout, "  test_frontend_lexer_parser...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("unit.cc", "int main(){return 0;}");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    if (tokens.value().size() < 8) return false;
    if (tokens.value()[0].kind() != prism::frontend::TokenKind::KeywordInt) return false;
    if (tokens.value()[0].range().begin().line() != 1) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    if (tu.value()->decls().size() != 1) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_record_declarations() -> bool {
    std::fprintf(stdout, "  test_record_declarations...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer(
        "records.cc",
        "struct Point { Point(); ~Point(); int x; int y; }; class Box { Box(); bool full; }; int main(){return 0;}");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    if (tu.value()->decls().size() != 3) return false;
    if (tu.value()->decls()[0]->kind() != prism::ast::DeclKind::Struct) return false;
    auto* point = static_cast<prism::ast::RecordDecl*>(tu.value()->decls()[0]);
    if (point->fields().size() != 2) return false;
    if (!point->has_default_constructor() || !point->has_destructor()) return false;
    prism::sema::SemanticAnalyzer sema(diag);
    auto result = sema.analyze(tu.value());
    if (!result || diag.has_errors()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_duplicate_record_rejected() -> bool {
    std::fprintf(stdout, "  test_duplicate_record_rejected...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("records.cc",
                                   "struct Point { int x; }; class Point { int y; }; int main(){return 0;}");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    prism::sema::SemanticAnalyzer sema(diag);
    auto result = sema.analyze(tu.value());
    if (result) return false;
    if (!diag.has_errors()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_frontend_rejects_trigraph() -> bool {
    std::fprintf(stdout, "  test_frontend_rejects_trigraph...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("legacy.cc", "int main() ?"
                                                "?< return 0; ?"
                                                "?>");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (tokens) return false;
    if (!diag.has_errors()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_template_subset_rejected() -> bool {
    std::fprintf(stdout, "  test_template_subset_rejected...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("template.cc", "template <typename T> T id(T x){ return x; }");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (tu) return false;
    if (!diag.has_errors()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_source_pipeline_ir() -> bool {
    std::fprintf(stdout, "  test_source_pipeline_ir...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("unit.cc", "int main() { return 42; }");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    prism::sema::SemanticAnalyzer sema(diag);
    auto sema_result = sema.analyze(tu.value());
    if (!sema_result) return false;
    prism::ir::Context ir_context;
    prism::codegen::ASTLowering lowering(ir_context);
    auto module = lowering.lower(tu.value());
    if (!module) return false;
    prism::ir::IRPrinter printer;
    auto ir = printer.print_module(module.value().get());
    if (::strstr(ir.c_str(), "42") == nullptr) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_sema_failures() -> bool {
    std::fprintf(stdout, "  test_sema_failures...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("bad.cc", "int add(int a, int b){return a;} int main(){return add(1);}");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    prism::sema::SemanticAnalyzer sema(diag);
    auto result = sema.analyze(tu.value());
    if (result) return false;
    if (!diag.has_errors()) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_reference_semantics() -> bool {
    std::fprintf(stdout, "  test_reference_semantics...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("refs.cc", "int main(){ int x = 1; int& r = x; r = 2; return x; }");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

auto test_overload_resolution() -> bool {
    std::fprintf(stdout, "  test_overload_resolution...");
    prism::DiagnosticsEngine diag;
    prism::frontend::SourceManager source_manager;
    source_manager.set_main_buffer("overload.cc",
                                   "int f(int x){return x;} int f(bool x){return 1;} int main(){ return f(true); }");
    prism::frontend::Lexer lexer(source_manager, diag);
    auto tokens = lexer.tokenize();
    if (!tokens) return false;
    prism::ast::ASTContext ast_context;
    prism::frontend::Parser parser(ast_context, diag, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return false;
    prism::sema::SemanticAnalyzer sema(diag);
    if (!sema.analyze(tu.value())) return false;

    prism::DiagnosticsEngine bad_diag;
    prism::frontend::SourceManager bad_source;
    bad_source.set_main_buffer("bad_overload.cc",
                               "int f(int x){return x;} int f(int y){return y;} int main(){ return f(1); }");
    prism::frontend::Lexer bad_lexer(bad_source, bad_diag);
    auto bad_tokens = bad_lexer.tokenize();
    if (!bad_tokens) return false;
    prism::ast::ASTContext bad_context;
    prism::frontend::Parser bad_parser(bad_context, bad_diag, bad_tokens.value());
    auto bad_tu = bad_parser.parse_translation_unit();
    if (!bad_tu) return false;
    prism::sema::SemanticAnalyzer bad_sema(bad_diag);
    if (bad_sema.analyze(bad_tu.value())) return false;
    std::fprintf(stdout, " PASS\n");
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    std::fprintf(stdout, "Running Prism compiler tests:\n\n");
    std::fprintf(stdout, "IR Layer tests:\n");
    if (test_basic_types())
        passed++;
    else
        failed++;
    if (test_ir_context())
        passed++;
    else
        failed++;
    if (test_ir_builder())
        passed++;
    else
        failed++;
    if (test_ir_printer())
        passed++;
    else
        failed++;
    if (test_ir_verifier())
        passed++;
    else
        failed++;
    if (test_pass_manager())
        passed++;
    else
        failed++;
    if (test_phi_nodes())
        passed++;
    else
        failed++;
    if (test_mem2reg_pass())
        passed++;
    else
        failed++;

    std::fprintf(stdout, "\nCode Generation tests:\n");
    if (test_code_generation())
        passed++;
    else
        failed++;

    std::fprintf(stdout, "\nSemantic Analysis tests:\n");
    if (test_sema())
        passed++;
    else
        failed++;

    std::fprintf(stdout, "\nFrontend tests:\n");
    if (test_frontend_lexer_parser())
        passed++;
    else
        failed++;
    if (test_record_declarations())
        passed++;
    else
        failed++;
    if (test_duplicate_record_rejected())
        passed++;
    else
        failed++;
    if (test_frontend_rejects_trigraph())
        passed++;
    else
        failed++;
    if (test_template_subset_rejected())
        passed++;
    else
        failed++;
    if (test_sema_failures())
        passed++;
    else
        failed++;
    if (test_reference_semantics())
        passed++;
    else
        failed++;
    if (test_overload_resolution())
        passed++;
    else
        failed++;

    std::fprintf(stdout, "\nIntegration tests:\n");
    if (test_full_pipeline())
        passed++;
    else
        failed++;
    if (test_source_pipeline_ir())
        passed++;
    else
        failed++;

    std::fprintf(stdout, "\nResults: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
