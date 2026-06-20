#pragma once

#include "prism/core/container/hash_map.hpp"
#include "prism/core/string/string.hpp"

namespace prism::ir {
class Module;
class Function;
class BasicBlock;
class Instruction;
class Value;
}  // namespace prism::ir

namespace prism::codegen {

/** @brief Generates x86-64 assembly code from an IR module. */
class X86_64CodeGenerator {
public:
    /** @brief Generates x86-64 assembly for the given IR module. @return The assembly source code as a string. */
    prism::String generate(prism::ir::Module* module);

private:
    /** @brief Emits assembly for a single function. */
    void emit_function(prism::ir::Function* fn);

    /** @brief Emits assembly for a single basic block. */
    void emit_basic_block(prism::ir::BasicBlock* bb);

    /** @brief Emits assembly for a single instruction. */
    void emit_instruction(prism::ir::Instruction* inst);

    /** @brief Emits the function prologue (stack setup). */
    void emit_prologue(prism::ir::Function* fn);

    /** @brief Emits the function epilogue (stack teardown and return). */
    void emit_epilogue(prism::ir::Function* fn);

    /** @brief Returns the assembly label for the given function. @return The label string. */
    prism::String function_label(prism::ir::Function* fn) const;

    /** @brief Returns the assembly label for the given basic block. @return The label string. */
    prism::String block_label(prism::ir::BasicBlock* bb) const;

    /** @brief Returns the operand representation for the given IR value. @return The operand string (register or
     * immediate). */
    prism::String value_operand(prism::ir::Value* value);

    /** @brief Returns the stack slot operand for the given variable name. @return The stack slot string. */
    prism::String stack_slot(const prism::String& name);

    /** @brief Returns the stack offset for the given variable name. @return The offset from the base pointer. */
    int get_stack_offset(const prism::String& name);

    /** @brief Allocates a new stack slot for the given variable name. @return The assigned stack offset. */
    int allocate_stack_slot(const prism::String& name);

    prism::String output_;
    prism::HashMap<prism::String, int> stack_offsets_;
    int stack_size_ = 0;
    int label_count_ = 0;
    prism::String current_function_label_;
};

}  // namespace prism::codegen
