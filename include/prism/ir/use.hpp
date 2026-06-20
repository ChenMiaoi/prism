#pragma once

namespace prism::ir {

class Value;
class User;

/**
 * @brief Represents a use of a Value by a User.
 *
 * Tracks which operand position a User refers to a Value at. Uses are
 * maintained in a doubly-linked list on each Value to efficiently track
 * all references to that value.
 */
class Use {
public:
    /** @brief Default constructor. */
    Use() = default;

    /**
     * @brief Constructs a Use linking a user to a value at a given operand position.
     * @param operand_no The operand index in the user's operand list.
     * @param user Pointer to the User that uses the value.
     * @param value Pointer to the Value being used.
     */
    Use(unsigned operand_no, User* user, Value* value);

    /** @brief Destructor that removes this use from the value's use list. */
    ~Use();

    /**
     * @brief Gets the value being used.
     * @return Pointer to the used Value.
     */
    auto get() const -> Value*;

    /**
     * @brief Gets the user that owns this use.
     * @return Pointer to the User.
     */
    auto get_user() const -> User*;

    /**
     * @brief Gets the operand index of this use within its user.
     * @return The operand number.
     */
    auto get_operand_no() const -> unsigned;

    /**
     * @brief Updates this use to point to a different value.
     * @param val Pointer to the new Value to use.
     */
    auto set(Value* val) -> void;

private:
    Value* value_ = nullptr;
    User* user_ = nullptr;
    unsigned operand_no_ = 0;
};

}  // namespace prism::ir
