#include "prism/ir/ir_builder.hpp"

#include "prism/ir/context.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/instruction.hpp"
#include "prism/ir/type.hpp"

namespace prism::ir {

/** @brief Constructs an IRBuilder associated with the given context. */
IRBuilder::IRBuilder(Context& ctx) : ctx_(ctx) {
}

/** @brief Sets the insertion point to the given basic block. */
void IRBuilder::set_insert_point(BasicBlock* bb) {
    block_ = bb;
}

/** @brief Sets the insertion point after the given instruction (not yet implemented). */
void IRBuilder::set_insert_point_after(Instruction* inst) {
    (void)inst;
}

/** @brief Returns the current insertion block. */
BasicBlock* IRBuilder::get_insert_block() const {
    return block_;
}

/** @brief Creates an integer addition instruction at the current insertion point. */
Value* IRBuilder::create_add(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new BinaryOperator(lhs->get_type(), Opcode::Add, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates an integer subtraction instruction at the current insertion point. */
Value* IRBuilder::create_sub(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new BinaryOperator(lhs->get_type(), Opcode::Sub, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates an integer multiplication instruction at the current insertion point. */
Value* IRBuilder::create_mul(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new BinaryOperator(lhs->get_type(), Opcode::Mul, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a signed integer division instruction at the current insertion point. */
Value* IRBuilder::create_sdiv(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new BinaryOperator(lhs->get_type(), Opcode::SDiv, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a signed integer remainder instruction at the current insertion point. */
Value* IRBuilder::create_srem(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new BinaryOperator(lhs->get_type(), Opcode::SRem, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates an integer equality comparison instruction at the current insertion point. */
Value* IRBuilder::create_icmp_eq(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new ICmpInst(Opcode::ICmpEQ, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates an integer not-equal comparison instruction at the current insertion point. */
Value* IRBuilder::create_icmp_ne(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new ICmpInst(Opcode::ICmpNE, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a signed less-than comparison instruction at the current insertion point. */
Value* IRBuilder::create_icmp_slt(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new ICmpInst(Opcode::ICmpSLT, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a signed greater-than comparison instruction at the current insertion point. */
Value* IRBuilder::create_icmp_sgt(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new ICmpInst(Opcode::ICmpSGT, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a signed less-than-or-equal comparison instruction at the current insertion point. */
Value* IRBuilder::create_icmp_sle(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new ICmpInst(Opcode::ICmpSLE, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a signed greater-than-or-equal comparison instruction at the current insertion point. */
Value* IRBuilder::create_icmp_sge(Value* lhs, Value* rhs, const char* name) {
    auto* inst = new ICmpInst(Opcode::ICmpSGE, lhs, rhs, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a phi node with the given type, incoming values, and blocks. */
PHINode* IRBuilder::create_phi(Type* ty, Vector<Value*> values, Vector<BasicBlock*> blocks, const char* name) {
    auto* inst =
        new PHINode(ty, static_cast<Vector<Value*>&&>(values), static_cast<Vector<BasicBlock*>&&>(blocks), name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates an unconditional branch to the target block. */
BranchInst* IRBuilder::create_br(BasicBlock* target) {
    auto* inst = new BranchInst(target);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a conditional branch based on a condition, with then and else targets. */
BranchInst* IRBuilder::create_cond_br(Value* cond, BasicBlock* then_bb, BasicBlock* else_bb) {
    auto* inst = new BranchInst(cond, then_bb, else_bb);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a return instruction with the given value. */
ReturnInst* IRBuilder::create_ret(Value* val) {
    auto* inst = new ReturnInst(val);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a void return instruction. */
ReturnInst* IRBuilder::create_ret_void() {
    auto* inst = new ReturnInst(nullptr);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates an alloca instruction for the given type. */
AllocaInst* IRBuilder::create_alloca(Type* ty, const char* name) {
    auto* inst = new AllocaInst(ty, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a load instruction from the given pointer. */
LoadInst* IRBuilder::create_load(Type* ty, Value* ptr, const char* name) {
    auto* inst = new LoadInst(ty, ptr, name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a store instruction writing a value to the given pointer. */
StoreInst* IRBuilder::create_store(Value* val, Value* ptr) {
    auto* inst = new StoreInst(val, ptr);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Creates a call instruction to the given function with the provided arguments. */
CallInst* IRBuilder::create_call(Function* fn, Vector<Value*> args, const char* name) {
    auto* inst = new CallInst(fn, static_cast<Vector<Value*>&&>(args), name);
    block_->append(prism::UniquePtr<Instruction>(inst));
    return inst;
}

/** @brief Returns a constant 32-bit integer. */
ConstantInt* IRBuilder::get_int32(int32_t val) {
    return ctx_.get_int_constant(val, ctx_.i32_type());
}

/** @brief Returns a constant 64-bit integer. */
ConstantInt* IRBuilder::get_int64(int64_t val) {
    return ctx_.get_int_constant(val, ctx_.i64_type());
}

/** @brief Returns a constant floating-point value. */
ConstantFP* IRBuilder::get_float(double val) {
    return ctx_.get_float_constant(val);
}

}  // namespace prism::ir
