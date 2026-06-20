#pragma once

#include "prism/basic/diagnostics.hpp"
#include "prism/basic/error.hpp"
#include "prism/core/string/string.hpp"

namespace prism {

/** @brief Supported code generation backends. */
enum class BackendType {
    X86_64, /**< x86-64 assembly backend. */
    LLVM,   /**< LLVM IR backend. */
    RISC_V  /**< RISC-V backend (not yet implemented). */
};

/** @brief Top-level compiler driver that orchestrates parsing, analysis, and code generation. */
class Compiler {
public:
    /** @brief Constructs a Compiler with default settings. */
    Compiler();

    /** @brief Compiles the named source file with the given backend. @param filename Path to the source file. @param
     * backend The target backend. @return Success or an error. */
    Expected<void> compile(const char* filename, BackendType backend = BackendType::X86_64);

    /** @brief Compiles the given source string with an optional filename for diagnostics. @param source The source code
     * text. @param filename Filename for error reporting. @param backend The target backend. @return Success or an
     * error. */
    Expected<void> compile_source(const char* source, const char* filename = "<input>",
                                  BackendType backend = BackendType::X86_64);

    /** @brief Emits the IR produced by the last compilation. @return Success or an error. */
    Expected<void> emit_ir();

    /** @brief Emits the final output (assembly or object) from the last compilation. @return Success or an error. */
    Expected<void> emit_output();

private:
    DiagnosticsEngine diag_;
    String last_ir_;
    String last_output_;
};

}  // namespace prism
