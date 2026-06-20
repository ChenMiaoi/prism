#include "prism/ir/user.hpp"

namespace prism::ir {

/** @brief Constructs a User with a type, a fixed number of operands, and an optional name. */
User::User(Type* ty, unsigned num_operands, const char* name) : Value(ty, name) {
    init_operands(num_operands);
}

/** @brief Initializes the operand list to the given count. */
void User::init_operands(unsigned count) {
    operands_.resize(count);
}

/** @brief Returns the i-th operand value. */
Value* User::operand(unsigned i) {
    return operands_[i].get();
}

/** @brief Returns the i-th operand value (const version). */
const Value* User::operand(unsigned i) const {
    return operands_[i].get();
}

/** @brief Returns the number of operands. */
unsigned User::operand_count() const {
    return static_cast<unsigned>(operands_.size());
}

/** @brief Sets the i-th operand to the given value. */
void User::set_operand(unsigned i, Value* val) {
    operands_[i].set(val);
}

/** @brief Returns a pointer to the beginning of the operand use list. */
Use* User::operand_begin() {
    return operands_.data();
}

/** @brief Returns a pointer past the end of the operand use list. */
Use* User::operand_end() {
    return operands_.data() + operands_.size();
}

/** @brief Returns a const pointer to the beginning of the operand use list. */
const Use* User::operand_begin() const {
    return operands_.data();
}

/** @brief Returns a const pointer past the end of the operand use list. */
const Use* User::operand_end() const {
    return operands_.data() + operands_.size();
}

}  // namespace prism::ir
