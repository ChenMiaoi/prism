#include "prism/driver/compiler.hpp"

#include "prism/ast/ast_context.hpp"
#include "prism/ast/decl.hpp"
#include "prism/ast/expr.hpp"
#include "prism/ast/stmt.hpp"
#include "prism/codegen/ast_lowering.hpp"
#include "prism/codegen/llvm/llvm_codegen.hpp"
#include "prism/codegen/x86_64/x86_64_codegen.hpp"
#include "prism/core/format/format.hpp"
#include "prism/frontend/lexer.hpp"
#include "prism/frontend/parser.hpp"
#include "prism/frontend/source_manager.hpp"
#include "prism/ir/context.hpp"
#include "prism/ir/ir_printer.hpp"
#include "prism/ir/module.hpp"
#include "prism/opt/pass_manager.hpp"
#include "prism/sema/semantic_analyzer.hpp"

#include <cstdio>
#include <cstring>

namespace prism {

/** @brief Default constructor for the Compiler. */
Compiler::Compiler() = default;

/** @brief Compiles a source file from disk.
 *  @param filename Path to the source file.
 *  @param backend The target backend for code generation.
 *  @return An Expected<void> indicating success or an error.
 */
Expected<void> Compiler::compile(const char* filename, BackendType backend) {
    FILE* file = ::fopen(filename, "r");
    if (!file) {
        return Unexpected<Error>(make_error(prism::format("cannot open file '{}'", filename).c_str()));
    }

    ::fseek(file, 0, SEEK_END);
    long size = ::ftell(file);
    if (size < 0) {
        ::fclose(file);
        return Unexpected<Error>(make_error(prism::format("cannot determine size of file '{}'", filename).c_str()));
    }
    ::fseek(file, 0, SEEK_SET);

    size_t buffer_size = static_cast<size_t>(size);
    char* buffer = new char[buffer_size + 1];
    size_t bytes_read = ::fread(buffer, 1, buffer_size, file);
    if (bytes_read != buffer_size) {
        delete[] buffer;
        ::fclose(file);
        return Unexpected<Error>(make_error(prism::format("cannot read file '{}'", filename).c_str()));
    }
    buffer[buffer_size] = '\0';
    ::fclose(file);

    auto result = compile_source(buffer, filename, backend);
    delete[] buffer;
    return result;
}

/** @brief Compiles source code through the full pipeline: lex, parse, sema, IR, optimize, codegen.
 *  @param source Null-terminated source code string.
 *  @param filename The filename used for diagnostics.
 *  @param backend The target backend for code generation.
 *  @return An Expected<void> indicating success or an error.
 */
Expected<void> Compiler::compile_source(const char* source, const char* filename, BackendType backend) {
    frontend::SourceManager source_manager;
    source_manager.set_main_buffer(filename, source);

    frontend::Lexer lexer(source_manager, diag_);
    auto tokens = lexer.tokenize();
    if (!tokens) return Unexpected<Error>(tokens.error());

    ast::ASTContext ast_context;
    frontend::Parser parser(ast_context, diag_, tokens.value());
    auto tu = parser.parse_translation_unit();
    if (!tu) return Unexpected<Error>(tu.error());

    sema::SemanticAnalyzer sema(diag_);
    auto sema_result = sema.analyze(tu.value());
    if (!sema_result) return Unexpected<Error>(sema_result.error());

    ir::Context ir_context;
    codegen::ASTLowering lowering(ir_context);
    auto module = lowering.lower(tu.value());
    if (!module) return Unexpected<Error>(module.error());

    opt::PassManager passes;
    passes.add_pass(UniquePtr<opt::ModulePass>(new opt::ConstantFoldingPass()));
    passes.add_pass(UniquePtr<opt::ModulePass>(new opt::DeadCodeEliminationPass()));
    auto pass_result = passes.run(*module.value());
    if (!pass_result) return Unexpected<Error>(pass_result.error());

    ir::IRPrinter printer;
    last_ir_ = printer.print_module(module.value().get());

    if (backend == BackendType::X86_64) {
        codegen::X86_64CodeGenerator codegen;
        last_output_ = codegen.generate(module.value().get());
    } else if (backend == BackendType::LLVM) {
        codegen::LLVMCodeGenerator codegen;
        last_output_ = codegen.generate(module.value().get());
    } else {
        last_output_ = last_ir_;
    }

    return {};
}

/** @brief Prints the generated IR to stdout.
 *  @return An Expected<void> indicating success.
 */
Expected<void> Compiler::emit_ir() {
    prism::print("{}\n", last_ir_.c_str());
    return {};
}

/** @brief Prints the generated code output to stdout.
 *  @return An Expected<void> indicating success.
 */
Expected<void> Compiler::emit_output() {
    prism::print("{}\n", last_output_.c_str());
    return {};
}

}  // namespace prism

/** @brief Entry point for the prism compiler CLI.
 *  @param argc Argument count.
 *  @param argv Argument vector.
 *  @return 0 on success, 1 on error.
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        prism::print(stderr, "Usage: {} <input.cc> [options]\n", argv[0]);
        prism::print(stderr, "Options:\n");
        prism::print(stderr, "  -o <output>      Output file\n");
        prism::print(stderr, "  --emit-ir        Emit IR\n");
        prism::print(stderr, "  --backend <x86_64|llvm> Select backend\n");
        prism::print(stderr, "  -h, --help       Show help\n");
        return 1;
    }

    const char* input_file = nullptr;
    const char* output_file = "a.out";
    bool emit_ir = false;
    prism::BackendType backend = prism::BackendType::X86_64;

    for (int i = 1; i < argc; ++i) {
        prism::String arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            prism::print("Usage: {} <input.cc> [options]\n", argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--emit-ir") {
            emit_ir = true;
        } else if (arg == "--backend" && i + 1 < argc) {
            prism::String backend_name = argv[++i];
            if (backend_name == "llvm")
                backend = prism::BackendType::LLVM;
            else if (backend_name == "x86_64")
                backend = prism::BackendType::X86_64;
            else {
                prism::print(stderr, "Error: unknown backend '{}'\n", backend_name.c_str());
                return 1;
            }
        } else if (arg[0] != '-') {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        prism::print(stderr, "Error: no input file\n");
        return 1;
    }

    (void)output_file;
    prism::Compiler compiler;
    auto result = compiler.compile(input_file, backend);

    if (!result) {
        prism::print(stderr, "Error: {}\n", result.error().message());
        return 1;
    }
    if (emit_ir) {
        auto ir_result = compiler.emit_ir();
        if (!ir_result) {
            prism::print(stderr, "Error: {}\n", ir_result.error().message());
            return 1;
        }
    } else {
        auto output_result = compiler.emit_output();
        if (!output_result) {
            prism::print(stderr, "Error: {}\n", output_result.error().message());
            return 1;
        }
    }
    return 0;
}
