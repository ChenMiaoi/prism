#include "prism/ir/basic_block.hpp"

#include "prism/ir/context.hpp"
#include "prism/ir/function.hpp"

namespace prism::ir {

/** @brief Constructs a BasicBlock with a name and an optional parent function. */
BasicBlock::BasicBlock(Context& ctx, const char* name, Function* parent) : Value(nullptr, name), parent_(parent) {
    (void)ctx;
}

/** @brief Appends an instruction to the end of this basic block. */
void BasicBlock::append(prism::UniquePtr<Instruction> inst) {
    inst->set_parent(this);
    insts_.push_back(static_cast<prism::UniquePtr<Instruction>&&>(inst));
}

/** @brief Inserts an instruction after the given position instruction. */
void BasicBlock::insert_after(Instruction* pos, prism::UniquePtr<Instruction> inst) {
    inst->set_parent(this);
    for (size_t i = 0; i < insts_.size(); ++i) {
        if (insts_[i].get() == pos) {
            insts_.insert(insts_.begin() + i + 1, static_cast<prism::UniquePtr<Instruction>&&>(inst));
            return;
        }
    }
    append(static_cast<prism::UniquePtr<Instruction>&&>(inst));
}

/** @brief Returns true if this basic block ends with a terminator instruction. */
bool BasicBlock::is_terminated() const {
    return !insts_.empty() && insts_[insts_.size() - 1]->is_terminator();
}

/** @brief Returns the terminator instruction, or nullptr if none exists. */
Instruction* BasicBlock::get_terminator() const {
    if (!insts_.empty() && insts_[insts_.size() - 1]->is_terminator()) {
        return insts_[insts_.size() - 1].get();
    }
    return nullptr;
}

/** @brief Prints the basic block and all its instructions as a string. */
String BasicBlock::print() const {
    String result;
    if (!get_name().empty()) {
        result.append(get_name());
        result.append(":\n");
    }
    for (size_t i = 0; i < insts_.size(); ++i) {
        result.append("  ");
        result.append(insts_[i]->print());
        result.append("\n");
    }
    return result;
}

}  // namespace prism::ir
