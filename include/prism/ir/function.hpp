#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/memory/unique_ptr.hpp"
#include "prism/core/string/string.hpp"
#include "prism/ir/basic_block.hpp"
#include "prism/ir/type.hpp"
#include "prism/ir/user.hpp"

namespace prism::ir {

class Context;
class Module;

/**
 * @brief Represents a function argument in the IR.
 */
class Argument : public Value {
public:
    /**
     * @brief Constructs an Argument.
     * @param ty Pointer to the argument's type.
     * @param name The argument's name.
     * @param parent Optional parent Function.
     */
    Argument(Type* ty, const char* name, Function* parent = nullptr);

    /**
     * @brief Gets the parent function.
     * @return Pointer to the parent Function.
     */
    Function* get_parent() const { return parent_; }

    /**
     * @brief Gets the argument index (0-based).
     * @return The argument number.
     */
    unsigned get_arg_no() const { return arg_no_; }

    /**
     * @brief Sets the argument index.
     * @param no The new argument number.
     */
    void set_arg_no(unsigned no) { arg_no_ = no; }

private:
    Function* parent_ = nullptr;
    unsigned arg_no_ = 0;
};

/**
 * @brief Represents a function in the IR.
 *
 * A Function contains a list of BasicBlocks and Arguments. It may be a
 * declaration (no body) or a definition with basic blocks.
 */
class Function final : public User {
public:
    /**
     * @brief Constructs a Function.
     * @param ty The function type.
     * @param name The function name.
     * @param module The parent Module.
     */
    Function(FunctionType* ty, const char* name, Module* module);

    /**
     * @brief Gets the function type.
     * @return Pointer to the FunctionType.
     */
    FunctionType* get_function_type() const { return func_type_; }

    /**
     * @brief Gets the return type of this function.
     * @return Pointer to the return Type.
     */
    Type* get_return_type() const { return func_type_->return_type(); }

    /**
     * @brief Checks if this function is a declaration (no body).
     * @return True if this is a declaration, false if it has a body.
     */
    bool is_declaration() const { return is_declaration_; }

    /**
     * @brief Creates a new basic block in this function.
     * @param name Optional name for the block.
     * @return Pointer to the new BasicBlock.
     */
    BasicBlock* create_block(const char* name = "");

    /**
     * @brief Gets the list of basic blocks (mutable).
     * @return Reference to the block vector.
     */
    Vector<prism::UniquePtr<BasicBlock>>& blocks() { return blocks_; }

    /**
     * @brief Gets the list of basic blocks (const).
     * @return Const reference to the block vector.
     */
    const Vector<prism::UniquePtr<BasicBlock>>& blocks() const { return blocks_; }

    /**
     * @brief Gets the list of arguments (mutable).
     * @return Reference to the argument vector.
     */
    Vector<prism::UniquePtr<Argument>>& args() { return args_; }

    /**
     * @brief Gets the list of arguments (const).
     * @return Const reference to the argument vector.
     */
    const Vector<prism::UniquePtr<Argument>>& args() const { return args_; }

    /**
     * @brief Gets the number of arguments.
     * @return The argument count.
     */
    size_t arg_count() const { return args_.size(); }

    /**
     * @brief Gets the number of basic blocks.
     * @return The block count.
     */
    size_t size() const { return blocks_.size(); }

    /**
     * @brief Checks if the function has no basic blocks.
     * @return True if the function has no blocks.
     */
    bool empty() const { return blocks_.empty(); }

    /**
     * @brief Gets the parent module.
     * @return Pointer to the parent Module.
     */
    Module* get_parent() const { return module_; }

    /**
     * @brief Returns a string representation of this function.
     * @return String representation for printing/debugging.
     */
    String print() const override;

private:
    FunctionType* func_type_;
    Module* module_;
    bool is_declaration_;
    Vector<prism::UniquePtr<BasicBlock>> blocks_;
    Vector<prism::UniquePtr<Argument>> args_;
};

}  // namespace prism::ir
