#pragma once

#include "prism/core/string/string.hpp"

namespace prism::ir {

class Module;
class Function;
class BasicBlock;
class Instruction;
class Value;

/**
 * @brief Utility class for printing IR entities as human-readable strings.
 *
 * Provides methods to convert modules, functions, basic blocks, instructions,
 * and values into their textual IR representation.
 */
class IRPrinter {
public:
    /**
     * @brief Prints a module to a string.
     * @param module Pointer to the Module to print.
     * @return String representation of the module.
     */
    String print_module(const Module* module);

    /**
     * @brief Prints a function to a string.
     * @param fn Pointer to the Function to print.
     * @return String representation of the function.
     */
    String print_function(const Function* fn);

    /**
     * @brief Prints a basic block to a string.
     * @param bb Pointer to the BasicBlock to print.
     * @return String representation of the basic block.
     */
    String print_basic_block(const BasicBlock* bb);

    /**
     * @brief Prints an instruction to a string.
     * @param inst Pointer to the Instruction to print.
     * @return String representation of the instruction.
     */
    String print_instruction(const Instruction* inst);

    /**
     * @brief Prints a value to a string.
     * @param val Pointer to the Value to print.
     * @return String representation of the value.
     */
    String print_value(const Value* val);
};

}  // namespace prism::ir
