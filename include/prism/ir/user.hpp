#pragma once

#include "prism/ir/use.hpp"
#include "prism/ir/value.hpp"

namespace prism::ir {

/**
 * @brief Base class for Values that use other Values (instructions, constants, etc.).
 *
 * A User holds a list of operands (each a Use) that reference other Values.
 * This forms the use-def chain in the SSA IR.
 */
class User : public Value {
public:
    /**
     * @brief Gets the i-th operand as a mutable pointer.
     * @param i The operand index.
     * @return Pointer to the operand Value.
     */
    Value* operand(unsigned i);

    /**
     * @brief Gets the i-th operand as a const pointer.
     * @param i The operand index.
     * @return Const pointer to the operand Value.
     */
    const Value* operand(unsigned i) const;

    /**
     * @brief Gets the number of operands.
     * @return The operand count.
     */
    unsigned operand_count() const;

    /**
     * @brief Replaces the i-th operand with a new value.
     * @param i The operand index.
     * @param val Pointer to the new Value.
     */
    void set_operand(unsigned i, Value* val);

    /**
     * @brief Gets an iterator to the first operand (mutable).
     * @return Pointer to the first Use.
     */
    Use* operand_begin();

    /**
     * @brief Gets an iterator past the last operand (mutable).
     * @return Pointer past the last Use.
     */
    Use* operand_end();

    /**
     * @brief Gets an iterator to the first operand (const).
     * @return Const pointer to the first Use.
     */
    const Use* operand_begin() const;

    /**
     * @brief Gets an iterator past the last operand (const).
     * @return Const pointer past the last Use.
     */
    const Use* operand_end() const;

protected:
    /**
     * @brief Constructs a User with a given type and operand count.
     * @param ty Pointer to the Type of this value.
     * @param num_operands Number of operands this user will hold.
     * @param name Optional name for the value.
     */
    User(Type* ty, unsigned num_operands, const char* name = "");

    /** @brief Default virtual destructor. */
    ~User() override = default;

    /**
     * @brief Initializes the operand list with the given count.
     * @param count Number of operands to allocate.
     */
    void init_operands(unsigned count);

private:
    Vector<Use> operands_;
};

}  // namespace prism::ir
