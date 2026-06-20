#include "prism/ir/use.hpp"

#include "prism/ir/value.hpp"

namespace prism::ir {

/** @brief Constructs a Use linking a user to a value at a given operand index. */
Use::Use(unsigned operand_no, User* user, Value* value) : value_(value), user_(user), operand_no_(operand_no) {
    if (value_) {
        value_->add_use(this);
    }
}

/** @brief Destroys the Use and removes itself from the referenced value's use list. */
Use::~Use() {
    if (value_) {
        value_->remove_use(this);
    }
}

/** @brief Returns the value being used. */
Value* Use::get() const {
    return value_;
}

/** @brief Returns the user that owns this use. */
User* Use::get_user() const {
    return user_;
}

/** @brief Returns the operand index within the user. */
unsigned Use::get_operand_no() const {
    return operand_no_;
}

/** @brief Updates this use to reference a new value, updating use lists accordingly. */
void Use::set(Value* val) {
    if (value_) {
        value_->remove_use(this);
    }
    value_ = val;
    if (value_) {
        value_->add_use(this);
    }
}

}  // namespace prism::ir
