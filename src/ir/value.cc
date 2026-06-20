#include "prism/ir/value.hpp"

namespace prism::ir {

/** @brief Constructs a Value with a type and an optional name. */
Value::Value(Type* ty, const char* name) : type_(ty), name_(name ? name : "") {
}

/** @brief Returns the type of this value. */
Type* Value::get_type() const {
    return type_;
}

/** @brief Returns the name of this value. */
const String& Value::get_name() const {
    return name_;
}

/** @brief Sets the name of this value. */
void Value::set_name(const char* name) {
    name_ = String(name);
}

/** @brief Returns all uses of this value. */
const Vector<Use*>& Value::uses() const {
    return uses_;
}

/** @brief Adds a use reference pointing to this value. */
void Value::add_use(Use* use) {
    uses_.push_back(use);
}

/** @brief Removes a use reference from this value. */
void Value::remove_use(Use* use) {
    for (size_t i = 0; i < uses_.size(); ++i) {
        if (uses_[i] == use) {
            uses_[i] = uses_.back();
            uses_.pop_back();
            return;
        }
    }
}

/** @brief Returns true if this value has no uses. */
bool Value::use_empty() const {
    return uses_.empty();
}

/** @brief Prints the value as a string, using "%name" or "<unnamed>". */
String Value::print() const {
    if (name_.empty()) return "<unnamed>";
    return String("%") + name_;
}

}  // namespace prism::ir
