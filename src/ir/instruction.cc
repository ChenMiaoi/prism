#include "prism/ir/instruction.hpp"

#include "prism/ir/basic_block.hpp"
#include "prism/ir/context.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/type.hpp"

namespace prism::ir {

/** @brief Constructs an Instruction with a type, opcode, operand count, and parent block. */
Instruction::Instruction(Type* ty, Opcode op, unsigned num_operands, BasicBlock* parent)
    : User(ty, num_operands), parent_(parent), opcode_(op) {
}

/** @brief Returns true if this instruction is a terminator (br, condbr, ret). */
bool Instruction::is_terminator() const {
    return opcode_ == Opcode::Br || opcode_ == Opcode::CondBr || opcode_ == Opcode::Ret;
}

/** @brief Returns true if this instruction is a binary operation (add, sub, mul, sdiv, srem). */
bool Instruction::is_binary_op() const {
    return opcode_ == Opcode::Add || opcode_ == Opcode::Sub || opcode_ == Opcode::Mul || opcode_ == Opcode::SDiv ||
           opcode_ == Opcode::SRem;
}

/** @brief Returns true if this instruction is a memory operation (alloca, load, store). */
bool Instruction::is_memory_op() const {
    return opcode_ == Opcode::Alloca || opcode_ == Opcode::Load || opcode_ == Opcode::Store;
}

/** @brief Returns true if this instruction is a cast operation (trunc, zext, sext, fptosi, sitofp). */
bool Instruction::is_cast_op() const {
    return opcode_ == Opcode::Trunc || opcode_ == Opcode::ZExt || opcode_ == Opcode::SExt ||
           opcode_ == Opcode::FPToSI || opcode_ == Opcode::SIToFP;
}

// BinaryOperator

/** @brief Constructs a BinaryOperator with the given opcode, left and right operands, and name. */
BinaryOperator::BinaryOperator(Type* ty, Opcode op, Value* lhs, Value* rhs, const char* name) : Instruction(ty, op, 2) {
    set_operand(0, lhs);
    set_operand(1, rhs);
    set_name(name);
}

/** @brief Creates a BinaryOperator in the given context. */
BinaryOperator* BinaryOperator::create(Context& ctx, Opcode op, Value* lhs, Value* rhs, const char* name) {
    return ctx.create<BinaryOperator>(lhs->get_type(), op, lhs, rhs, name);
}

/** @brief Returns the left-hand side operand. */
Value* BinaryOperator::get_lhs() const {
    return const_cast<Value*>(operand(0));
}

/** @brief Returns the right-hand side operand. */
Value* BinaryOperator::get_rhs() const {
    return const_cast<Value*>(operand(1));
}

// ICmpInst

/** @brief Constructs an ICmpInst with the given comparison opcode, operands, and name. */
ICmpInst::ICmpInst(Opcode op, Value* lhs, Value* rhs, const char* name) : Instruction(nullptr, Opcode::ICmpEQ, 2) {
    opcode_ = op;
    set_operand(0, lhs);
    set_operand(1, rhs);
    set_name(name);
}

/** @brief Creates an ICmpInst in the given context. */
ICmpInst* ICmpInst::create(Context& ctx, Opcode op, Value* lhs, Value* rhs, const char* name) {
    return ctx.create<ICmpInst>(op, lhs, rhs, name);
}

/** @brief Returns the left-hand side operand of the comparison. */
Value* ICmpInst::get_lhs() const {
    return const_cast<Value*>(operand(0));
}

/** @brief Returns the right-hand side operand of the comparison. */
Value* ICmpInst::get_rhs() const {
    return const_cast<Value*>(operand(1));
}

// PHINode

/** @brief Constructs a PHINode with a type, incoming values, incoming blocks, and name. */
PHINode::PHINode(Type* ty, Vector<Value*> values, Vector<BasicBlock*> blocks, const char* name)
    : Instruction(ty, Opcode::Phi, static_cast<unsigned>(values.size()))
    , incoming_blocks_(static_cast<Vector<BasicBlock*>&&>(blocks)) {
    for (unsigned i = 0; i < static_cast<unsigned>(values.size()); ++i) {
        set_operand(i, values[i]);
    }
    set_name(name);
}

/** @brief Creates a PHINode in the given context. */
PHINode* PHINode::create(Context& ctx, Type* ty, Vector<Value*> values, Vector<BasicBlock*> blocks, const char* name) {
    return ctx.create<PHINode>(ty, static_cast<Vector<Value*>&&>(values), static_cast<Vector<BasicBlock*>&&>(blocks),
                               name);
}

// ReturnInst

/** @brief Constructs a ReturnInst with an optional return value. */
ReturnInst::ReturnInst(Value* ret_val)
    : Instruction(ret_val ? ret_val->get_type() : nullptr, Opcode::Ret, ret_val ? 1 : 0) {
    if (ret_val) {
        set_operand(0, ret_val);
    }
}

/** @brief Returns the return value, or nullptr for void returns. */
Value* ReturnInst::get_return_value() const {
    return operand_count() > 0 ? const_cast<Value*>(operand(0)) : nullptr;
}

/** @brief Creates a ReturnInst with the given value in the given context. */
ReturnInst* ReturnInst::create(Context& ctx, Value* ret_val) {
    return ctx.create<ReturnInst>(ret_val);
}

/** @brief Creates a void ReturnInst in the given context. */
ReturnInst* ReturnInst::create_empty(Context& ctx) {
    return ctx.create<ReturnInst>(nullptr);
}

// BranchInst

/** @brief Constructs an unconditional BranchInst targeting the given block. */
BranchInst::BranchInst(BasicBlock* target) : Instruction(nullptr, Opcode::Br, 1) {
    set_operand(0, target);
}

/** @brief Constructs a conditional BranchInst with a condition and then/else targets. */
BranchInst::BranchInst(Value* cond, BasicBlock* then_bb, BasicBlock* else_bb)
    : Instruction(nullptr, Opcode::CondBr, 3) {
    set_operand(0, cond);
    set_operand(1, then_bb);
    set_operand(2, else_bb);
}

/** @brief Returns the i-th successor basic block. */
BasicBlock* BranchInst::get_successor(unsigned i) const {
    return static_cast<BasicBlock*>(const_cast<Value*>(operand(opcode_ == Opcode::CondBr ? i + 1 : i)));
}

/** @brief Returns the number of successor blocks (1 for unconditional, 2 for conditional). */
unsigned BranchInst::get_num_successors() const {
    return opcode_ == Opcode::CondBr ? 2 : 1;
}

/** @brief Creates an unconditional BranchInst in the given context. */
BranchInst* BranchInst::create(Context& ctx, BasicBlock* target) {
    return ctx.create<BranchInst>(target);
}

/** @brief Creates a conditional BranchInst in the given context. */
BranchInst* BranchInst::create(Context& ctx, Value* cond, BasicBlock* then_bb, BasicBlock* else_bb) {
    return ctx.create<BranchInst>(cond, then_bb, else_bb);
}

// CallInst

/** @brief Constructs a CallInst calling the given function with the provided arguments. */
CallInst::CallInst(Function* fn, Vector<Value*> args, const char* name)
    : Instruction(fn->get_return_type(), Opcode::Call, static_cast<unsigned>(args.size() + 1)) {
    set_operand(0, fn);
    for (unsigned i = 0; i < static_cast<unsigned>(args.size()); ++i) {
        set_operand(i + 1, args[i]);
    }
    set_name(name);
}

/** @brief Returns the called function. */
Function* CallInst::get_called_function() const {
    return static_cast<Function*>(const_cast<Value*>(operand(0)));
}

/** @brief Returns the number of arguments passed to the call. */
unsigned CallInst::arg_count() const {
    return operand_count() - 1;
}

/** @brief Creates a CallInst in the given context. */
CallInst* CallInst::create(Context& ctx, Function* fn, Vector<Value*> args, const char* name) {
    return ctx.create<CallInst>(fn, static_cast<Vector<Value*>&&>(args), name);
}

// AllocaInst

/** @brief Constructs an AllocaInst that allocates space for the given type. */
AllocaInst::AllocaInst(Type* ty, const char* name) : Instruction(ty, Opcode::Alloca, 0) {
    set_name(name);
}

/** @brief Returns the type being allocated. */
Type* AllocaInst::get_allocated_type() const {
    return get_type();
}

/** @brief Creates an AllocaInst in the given context. */
AllocaInst* AllocaInst::create(Context& ctx, Type* ty, const char* name) {
    return ctx.create<AllocaInst>(ty, name);
}

// StoreInst

/** @brief Constructs a StoreInst that stores a value to a pointer. */
StoreInst::StoreInst(Value* val, Value* ptr) : Instruction(nullptr, Opcode::Store, 2) {
    set_operand(0, val);
    set_operand(1, ptr);
}

/** @brief Returns the pointer operand. */
Value* StoreInst::get_pointer_operand() const {
    return const_cast<Value*>(operand(1));
}

/** @brief Returns the value operand being stored. */
Value* StoreInst::get_value_operand() const {
    return const_cast<Value*>(operand(0));
}

/** @brief Creates a StoreInst in the given context. */
StoreInst* StoreInst::create(Context& ctx, Value* val, Value* ptr) {
    return ctx.create<StoreInst>(val, ptr);
}

// LoadInst

/** @brief Constructs a LoadInst that loads a value from a pointer. */
LoadInst::LoadInst(Type* ty, Value* ptr, const char* name) : Instruction(ty, Opcode::Load, 1) {
    set_operand(0, ptr);
    set_name(name);
}

/** @brief Returns the pointer operand. */
Value* LoadInst::get_pointer_operand() const {
    return const_cast<Value*>(operand(0));
}

/** @brief Creates a LoadInst in the given context. */
LoadInst* LoadInst::create(Context& ctx, Type* ty, Value* ptr, const char* name) {
    return ctx.create<LoadInst>(ty, ptr, name);
}

/** @brief Prints the instruction as a placeholder string. */
String Instruction::print() const {
    return "<instruction>";
}

}  // namespace prism::ir
