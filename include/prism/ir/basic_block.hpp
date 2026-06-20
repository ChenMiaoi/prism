#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/memory/unique_ptr.hpp"
#include "prism/core/string/string.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/value.hpp"

namespace prism::ir {

class Function;

/**
 * @brief Represents a basic block: a sequence of instructions with a single entry and exit.
 *
 * A BasicBlock contains a list of Instructions, ends with a terminator instruction,
 * and belongs to a Function.
 */
class BasicBlock final : public Value {
public:
    /**
     * @brief Constructs a BasicBlock.
     * @param ctx Reference to the IR Context.
     * @param name Optional name for the block.
     * @param parent Optional parent Function.
     */
    BasicBlock(prism::ir::Context& ctx, const char* name = "", Function* parent = nullptr);

    /**
     * @brief Gets the parent function.
     * @return Pointer to the parent Function.
     */
    Function* get_parent() const { return parent_; }

    /**
     * @brief Sets the parent function.
     * @param fn Pointer to the new parent Function.
     */
    void set_parent(Function* fn) { parent_ = fn; }

    /**
     * @brief Gets the list of instructions (mutable).
     * @return Reference to the instruction vector.
     */
    Vector<prism::UniquePtr<Instruction>>& instructions() { return insts_; }

    /**
     * @brief Gets the list of instructions (const).
     * @return Const reference to the instruction vector.
     */
    const Vector<prism::UniquePtr<Instruction>>& instructions() const { return insts_; }

    /**
     * @brief Checks if the block has no instructions.
     * @return True if empty.
     */
    bool empty() const { return insts_.empty(); }

    /**
     * @brief Gets the number of instructions.
     * @return The instruction count.
     */
    size_t size() const { return insts_.size(); }

    /**
     * @brief Gets the first instruction.
     * @return Reference to the front Instruction.
     */
    Instruction& front() { return *insts_[0]; }

    /**
     * @brief Gets the last instruction.
     * @return Reference to the back Instruction.
     */
    Instruction& back() { return *insts_[insts_.size() - 1]; }

    /**
     * @brief Appends an instruction to the end of the block.
     * @param inst The instruction to append (takes ownership).
     */
    void append(prism::UniquePtr<Instruction> inst);

    /**
     * @brief Inserts an instruction after a given position.
     * @param pos Pointer to the instruction after which to insert.
     * @param inst The instruction to insert (takes ownership).
     */
    void insert_after(Instruction* pos, prism::UniquePtr<Instruction> inst);

    /**
     * @brief Checks if this block ends with a terminator.
     * @return True if the last instruction is a terminator.
     */
    bool is_terminated() const;

    /**
     * @brief Gets the terminator instruction.
     * @return Pointer to the terminator Instruction, or nullptr if none.
     */
    Instruction* get_terminator() const;

    /**
     * @brief Returns a string representation of this basic block.
     * @return String representation for printing/debugging.
     */
    String print() const override;

private:
    Function* parent_ = nullptr;
    Vector<prism::UniquePtr<Instruction>> insts_;
};

}  // namespace prism::ir
