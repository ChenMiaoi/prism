#pragma once

#include "prism/core/string/string.hpp"
#include "prism/ir/use.hpp"
#include "prism/ir/user.hpp"

namespace prism::ir {

class Context;
class Type;
class BasicBlock;
class Function;

/** @brief Enumeration of all instruction opcodes. */
enum class Opcode {
    Add,      ///< Integer addition.
    Sub,      ///< Integer subtraction.
    Mul,      ///< Integer multiplication.
    SDiv,     ///< Signed integer division.
    SRem,     ///< Signed integer remainder.
    FAdd,     ///< Floating-point addition.
    FSub,     ///< Floating-point subtraction.
    FMul,     ///< Floating-point multiplication.
    FDiv,     ///< Floating-point division.
    ICmpEQ,   ///< Integer comparison: equal.
    ICmpNE,   ///< Integer comparison: not equal.
    ICmpSLT,  ///< Integer comparison: signed less than.
    ICmpSGT,  ///< Integer comparison: signed greater than.
    ICmpSLE,  ///< Integer comparison: signed less than or equal.
    ICmpSGE,  ///< Integer comparison: signed greater than or equal.
    FCmpOEQ,  ///< Float comparison: ordered equal.
    FCmpONE,  ///< Float comparison: ordered not equal.
    FCmpOLT,  ///< Float comparison: ordered less than.
    FCmpOGT,  ///< Float comparison: ordered greater than.
    FCmpOLE,  ///< Float comparison: ordered less than or equal.
    FCmpOGE,  ///< Float comparison: ordered greater than or equal.
    And,      ///< Bitwise AND.
    Or,       ///< Bitwise OR.
    Xor,      ///< Bitwise XOR.
    Shl,      ///< Shift left.
    LShr,     ///< Logical shift right.
    AShr,     ///< Arithmetic shift right.
    Alloca,   ///< Stack allocation.
    Load,     ///< Memory load.
    Store,    ///< Memory store.
    Phi,      ///< SSA phi function.
    Br,       ///< Unconditional branch.
    CondBr,   ///< Conditional branch.
    Ret,      ///< Return from function.
    Call,     ///< Function call.
    Trunc,    ///< Truncate integer.
    ZExt,     ///< Zero-extend integer.
    SExt,     ///< Sign-extend integer.
    FPToSI,   ///< Float to signed integer conversion.
    SIToFP,   ///< Signed integer to float conversion.
};

/**
 * @brief Base class for all IR instructions.
 *
 * An instruction is a User that performs an operation within a BasicBlock.
 */
class Instruction : public User {
public:
    /**
     * @brief Gets the basic block containing this instruction.
     * @return Pointer to the parent BasicBlock.
     */
    BasicBlock* get_parent() const { return parent_; }

    /**
     * @brief Sets the basic block containing this instruction.
     * @param bb Pointer to the new parent BasicBlock.
     */
    void set_parent(BasicBlock* bb) { parent_ = bb; }

    /**
     * @brief Gets the opcode of this instruction.
     * @return The Opcode value.
     */
    Opcode get_opcode() const { return opcode_; }

    /**
     * @brief Checks if this instruction is a terminator (br, ret, etc.).
     * @return True if it is a terminator instruction.
     */
    bool is_terminator() const;

    /**
     * @brief Checks if this instruction is a binary operation (add, mul, etc.).
     * @return True if it is a binary operation.
     */
    bool is_binary_op() const;

    /**
     * @brief Checks if this instruction is a memory operation (load, store, alloca).
     * @return True if it is a memory operation.
     */
    bool is_memory_op() const;

    /**
     * @brief Checks if this instruction is a cast operation (trunc, zext, etc.).
     * @return True if it is a cast operation.
     */
    bool is_cast_op() const;

    /**
     * @brief Returns a string representation of this instruction.
     * @return String representation for printing/debugging.
     */
    String print() const override;

    /**
     * @brief Constructs an Instruction.
     * @param ty Pointer to the result type.
     * @param op The opcode for this instruction.
     * @param num_operands Number of operands.
     * @param parent Optional parent BasicBlock.
     */
    Instruction(Type* ty, Opcode op, unsigned num_operands, BasicBlock* parent = nullptr);

private:
    friend class BinaryOperator;
    friend class ICmpInst;
    friend class ReturnInst;
    friend class BranchInst;
    friend class CallInst;
    friend class AllocaInst;
    friend class StoreInst;
    friend class LoadInst;
    friend class PHINode;
    BasicBlock* parent_ = nullptr;
    Opcode opcode_;
};

/**
 * @brief Represents a binary arithmetic or logical instruction.
 */
class BinaryOperator final : public Instruction {
public:
    /**
     * @brief Gets the left-hand side operand.
     * @return Pointer to the left operand Value.
     */
    Value* get_lhs() const;

    /**
     * @brief Gets the right-hand side operand.
     * @return Pointer to the right operand Value.
     */
    Value* get_rhs() const;

    /**
     * @brief Creates a binary operator instruction.
     * @param ctx Reference to the Context.
     * @param op The binary opcode (Add, Sub, Mul, etc.).
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the new BinaryOperator.
     */
    static BinaryOperator* create(Context& ctx, Opcode op, Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Constructs a BinaryOperator directly.
     * @param ty Pointer to the result type.
     * @param op The binary opcode.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Name for the result.
     */
    BinaryOperator(Type* ty, Opcode op, Value* lhs, Value* rhs, const char* name);

private:
    friend class Context;
};

/**
 * @brief Represents an integer comparison instruction.
 */
class ICmpInst final : public Instruction {
public:
    /**
     * @brief Gets the left-hand side operand.
     * @return Pointer to the left operand Value.
     */
    Value* get_lhs() const;

    /**
     * @brief Gets the right-hand side operand.
     * @return Pointer to the right operand Value.
     */
    Value* get_rhs() const;

    /**
     * @brief Creates an integer comparison instruction.
     * @param ctx Reference to the Context.
     * @param op The comparison opcode (ICmpEQ, ICmpNE, etc.).
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Optional name for the result.
     * @return Pointer to the new ICmpInst.
     */
    static ICmpInst* create(Context& ctx, Opcode op, Value* lhs, Value* rhs, const char* name = "");

    /**
     * @brief Constructs an ICmpInst directly.
     * @param op The comparison opcode.
     * @param lhs Left-hand side operand.
     * @param rhs Right-hand side operand.
     * @param name Name for the result.
     */
    ICmpInst(Opcode op, Value* lhs, Value* rhs, const char* name);

private:
    friend class Context;
};

/**
 * @brief Represents an SSA phi node for merging values from different control flow paths.
 */
class PHINode final : public Instruction {
public:
    /**
     * @brief Constructs a PHINode.
     * @param ty The type of the phi result.
     * @param values The incoming values.
     * @param blocks The corresponding basic blocks.
     * @param name Name for the result.
     */
    PHINode(Type* ty, Vector<Value*> values, Vector<BasicBlock*> blocks, const char* name);

    /**
     * @brief Gets the number of incoming values.
     * @return The incoming count.
     */
    unsigned incoming_count() const { return static_cast<unsigned>(incoming_blocks_.size()); }

    /**
     * @brief Gets the i-th incoming value.
     * @param i The incoming index.
     * @return Pointer to the incoming Value.
     */
    Value* incoming_value(unsigned i) const { return const_cast<Value*>(operand(i)); }

    /**
     * @brief Gets the i-th incoming basic block.
     * @param i The incoming index.
     * @return Pointer to the incoming BasicBlock.
     */
    BasicBlock* incoming_block(unsigned i) const { return incoming_blocks_[i]; }

    /**
     * @brief Creates a PHINode.
     * @param ctx Reference to the Context.
     * @param ty The type of the phi result.
     * @param values The incoming values.
     * @param blocks The corresponding basic blocks.
     * @param name Optional name for the result.
     * @return Pointer to the new PHINode.
     */
    static PHINode* create(Context& ctx, Type* ty, Vector<Value*> values, Vector<BasicBlock*> blocks,
                           const char* name = "");

private:
    friend class Context;
    Vector<BasicBlock*> incoming_blocks_;
};

/**
 * @brief Represents a return instruction that terminates a function.
 */
class ReturnInst final : public Instruction {
public:
    /**
     * @brief Gets the return value.
     * @return Pointer to the return Value, or nullptr for void returns.
     */
    Value* get_return_value() const;

    /**
     * @brief Creates a return instruction with a value.
     * @param ctx Reference to the Context.
     * @param ret_val The value to return.
     * @return Pointer to the new ReturnInst.
     */
    static ReturnInst* create(Context& ctx, Value* ret_val);

    /**
     * @brief Creates a void return instruction.
     * @param ctx Reference to the Context.
     * @return Pointer to the new ReturnInst.
     */
    static ReturnInst* create_empty(Context& ctx);

    /**
     * @brief Constructs a ReturnInst with a return value.
     * @param ret_val The value to return.
     */
    explicit ReturnInst(Value* ret_val);

private:
    friend class Context;
};

/**
 * @brief Represents a branch instruction (unconditional or conditional).
 */
class BranchInst final : public Instruction {
public:
    /**
     * @brief Gets the i-th successor basic block.
     * @param i The successor index.
     * @return Pointer to the successor BasicBlock.
     */
    BasicBlock* get_successor(unsigned i) const;

    /**
     * @brief Gets the number of successors.
     * @return 1 for unconditional branch, 2 for conditional.
     */
    unsigned get_num_successors() const;

    /**
     * @brief Creates an unconditional branch.
     * @param ctx Reference to the Context.
     * @param target The target BasicBlock.
     * @return Pointer to the new BranchInst.
     */
    static BranchInst* create(Context& ctx, BasicBlock* target);

    /**
     * @brief Creates a conditional branch.
     * @param ctx Reference to the Context.
     * @param cond The condition Value.
     * @param then_bb The then BasicBlock.
     * @param else_bb The else BasicBlock.
     * @return Pointer to the new BranchInst.
     */
    static BranchInst* create(Context& ctx, Value* cond, BasicBlock* then_bb, BasicBlock* else_bb);

    /**
     * @brief Constructs an unconditional BranchInst.
     * @param target The target BasicBlock.
     */
    explicit BranchInst(BasicBlock* target);

    /**
     * @brief Constructs a conditional BranchInst.
     * @param cond The condition Value.
     * @param then_bb The then BasicBlock.
     * @param else_bb The else BasicBlock.
     */
    BranchInst(Value* cond, BasicBlock* then_bb, BasicBlock* else_bb);

private:
    friend class Context;
};

/**
 * @brief Represents a function call instruction.
 */
class CallInst final : public Instruction {
public:
    /**
     * @brief Gets the function being called.
     * @return Pointer to the called Function.
     */
    Function* get_called_function() const;

    /**
     * @brief Gets the number of arguments.
     * @return The argument count.
     */
    unsigned arg_count() const;

    /**
     * @brief Creates a call instruction.
     * @param ctx Reference to the Context.
     * @param fn The function to call.
     * @param args The argument values.
     * @param name Optional name for the result.
     * @return Pointer to the new CallInst.
     */
    static CallInst* create(Context& ctx, Function* fn, Vector<Value*> args, const char* name = "");

    /**
     * @brief Constructs a CallInst.
     * @param fn The function to call.
     * @param args The argument values.
     * @param name Name for the result.
     */
    CallInst(Function* fn, Vector<Value*> args, const char* name);

private:
    friend class Context;
};

/**
 * @brief Represents a stack allocation instruction.
 */
class AllocaInst final : public Instruction {
public:
    /**
     * @brief Gets the type being allocated.
     * @return Pointer to the allocated Type.
     */
    Type* get_allocated_type() const;

    /**
     * @brief Creates an alloca instruction.
     * @param ctx Reference to the Context.
     * @param ty The type to allocate.
     * @param name Optional name for the pointer result.
     * @return Pointer to the new AllocaInst.
     */
    static AllocaInst* create(Context& ctx, Type* ty, const char* name = "");

    /**
     * @brief Constructs an AllocaInst.
     * @param ty The type to allocate.
     * @param name Name for the pointer result.
     */
    AllocaInst(Type* ty, const char* name);

private:
    friend class Context;
};

/**
 * @brief Represents a store instruction that writes a value to memory.
 */
class StoreInst final : public Instruction {
public:
    /**
     * @brief Gets the pointer operand (destination).
     * @return Pointer to the pointer Value.
     */
    Value* get_pointer_operand() const;

    /**
     * @brief Gets the value operand (source).
     * @return Pointer to the value Value.
     */
    Value* get_value_operand() const;

    /**
     * @brief Creates a store instruction.
     * @param ctx Reference to the Context.
     * @param val The value to store.
     * @param ptr The destination pointer.
     * @return Pointer to the new StoreInst.
     */
    static StoreInst* create(Context& ctx, Value* val, Value* ptr);

    /**
     * @brief Constructs a StoreInst.
     * @param val The value to store.
     * @param ptr The destination pointer.
     */
    StoreInst(Value* val, Value* ptr);

private:
    friend class Context;
};

/**
 * @brief Represents a load instruction that reads a value from memory.
 */
class LoadInst final : public Instruction {
public:
    /**
     * @brief Gets the pointer operand (source).
     * @return Pointer to the pointer Value.
     */
    Value* get_pointer_operand() const;

    /**
     * @brief Creates a load instruction.
     * @param ctx Reference to the Context.
     * @param ty The type being loaded.
     * @param ptr The source pointer.
     * @param name Optional name for the loaded value.
     * @return Pointer to the new LoadInst.
     */
    static LoadInst* create(Context& ctx, Type* ty, Value* ptr, const char* name = "");

    /**
     * @brief Constructs a LoadInst.
     * @param ty The type being loaded.
     * @param ptr The source pointer.
     * @param name Name for the loaded value.
     */
    LoadInst(Type* ty, Value* ptr, const char* name);

private:
    friend class Context;
};

}  // namespace prism::ir
