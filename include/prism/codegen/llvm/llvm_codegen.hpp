#pragma once

#include "prism/core/string/string.hpp"

namespace prism::ir {
class Module;
}

namespace prism::codegen {

/** @brief Generates LLVM IR from an IR module for compilation. */
class LLVMCodeGenerator {
public:
    /** @brief Generates LLVM IR for the given IR module. @return The LLVM IR text as a string. */
    String generate(ir::Module* module);
};

}  // namespace prism::codegen
