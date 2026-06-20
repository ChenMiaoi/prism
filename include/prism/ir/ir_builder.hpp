#pragma once

#include "prism/ir/constant.hpp"
#include "prism/ir/instruction.hpp"

namespace prism::ir {

class Context;

/**
 * @brief Helper class for constructing IR instructions at a given insertion point.
 *
 * The IRBuilder maintains a current insertion point (a BasicBlock and optionally
 * an instruction position) and provides convenience methods for creating all
 * types of IR instructions.
 */
class IRBuilder {
public:
    /**
     * @brief Constructs an IRBuilder.
     * @param ctx Reference to the IR Context.
     */
    explicit IRBuilder(Context& ctx);

    /**
     * @brief Sets the insertion point to the end of a basic block.
     * @param bb Pointer to the BasicBlock.
     */
    void set_insert_point(BasicBlock* bb);

    /**
     * @brief Sets the insertion point after a given instruction.
     * @param inst Pointer to the instruction after which to insert.
     */
    void set_insert_point_after(Instruction* inst);

    /**
     * @brief Gets the current insertion block.
     * @return Pointer to the current BasicBlock.
     */
    BasicBlock* get_insert_block() const;

    /**
     * @brief Creates an integer addition instruction.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_add(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer subtraction instruction.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_sub(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer multiplication instruction.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_mul(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates a signed integer division instruction.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_sdiv(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates a signed integer remainder instruction.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_srem(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer equal comparison.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_icmp_eq(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer not-equal comparison.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_icmp_ne(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer signed-less-than comparison.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_icmp_slt(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer signed-greater-than comparison.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_icmp_sgt(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer signed-less-or-equal comparison.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_icmp_sle(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates an integer signed-greater-or-equal comparison.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the result Value.
     */
    Value* create_icmp_sge(Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Creates a phi node.
     * @param ty The result type.
     * @param values The incoming values.
     * @param blocks The corresponding basic blocks.
     * @param name Optional name for the result.
     * @return Pointer to the new PHINode.
     */
    PHINode* create_phi(Type* ty, Vector<Value*> values, Vector<BasicBlock*> blocks, const char* name = "");

    /**
     * @brief Creates an unconditional branch.
     * @param target The target BasicBlock.
     * @return Pointer to the new BranchInst.
     */
    BranchInst* create_br(BasicBlock* target);

    /**
     * @brief Creates a conditional branch.
     * @param cond The condition Value.
     * @param then_bb The then BasicBlock.
     * @param else_bb The else BasicBlock.
     * @return Pointer to the new BranchInst.
     */
    BranchInst* create_cond_br(Value* cond, BasicBlock* then_bb, BasicBlock* else_bb);

    /**
     * @brief Creates a return instruction with a value.
     * @param val The value to return.
     * @return Pointer to the new ReturnInst.
     */
    ReturnInst* create_ret(Value* val);

    /**
     * @brief Creates a void return instruction.
     * @return Pointer to the new ReturnInst.
     */
    ReturnInst* create_ret_void();

    /**
     * @brief Creates an alloca instruction.
     * @param ty The type to allocate.
     * @param name Optional name for the result.
     * @return Pointer to the new AllocaInst.
     */
    AllocaInst* create_alloca(Type* ty, const char* name = "");

    /**
     * @brief Creates a load instruction.
     * @param ty The type being loaded.
     * @param ptr The source pointer.
     * @param name Optional name for the result.
     * @return Pointer to the new LoadInst.
     */
    LoadInst* create_load(Type* ty, Value* ptr, const char* name = "");

    /**
     * @brief Creates a store instruction.
     * @param val The value to store.
     * @param ptr The destination pointer.
     * @return Pointer to the new StoreInst.
     */
    StoreInst* create_store(Value* val, Value* ptr);

    /**
     * @brief Creates a function call instruction.
     * @param fn The function to call.
     * @param args The argument values.
     * @param name Optional name for the result.
     * @return Pointer to the new CallInst.
     */
    CallInst* create_call(Function* fn, Vector<Value*> args, const char* name = "");

    /**
     * @brief Creates a 32-bit integer constant.
     * @param val The integer value.
     * @return Pointer to the ConstantInt.
     */
    ConstantInt* get_int32(int32_t val);

    /**
     * @brief Creates a 64-bit integer constant.
     * @param val The integer value.
     * @return Pointer to the ConstantInt.
     */
    ConstantInt* get_int64(int64_t val);

    /**
     * @brief Creates a double-precision float constant.
     * @param val The floating-point value.
     * @return Pointer to the ConstantFP.
     */
    ConstantFP* get_float(double val);

private:
    Context& ctx_;
    BasicBlock* block_ = nullptr;
};

}  // namespace prism::ir
