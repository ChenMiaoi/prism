#pragma once

#include "prism/core/utility/type_traits.hpp"

namespace prism {

/**
 * @brief Check whether a pointer is of a specific derived type.
 *
 * Uses a classof() static member if available; otherwise returns true for non-null pointers.
 *
 * @tparam To The target type to test against.
 * @tparam From The source pointer type.
 * @param val The pointer to test.
 * @return true if the pointer is an instance of type To.
 */
template <typename To, typename From>
[[nodiscard]] constexpr bool isa(const From* val) {
    if constexpr (is_same_v<To, From>) {
        return true;
    } else if constexpr (requires { To::classof(val); }) {
        return val != nullptr && To::classof(val);
    } else {
        return val != nullptr;
    }
}

/**
 * @brief Unchecked static cast from a derived type to a base type.
 *
 * Both From and To must be related by inheritance. Use dyn_cast for checked casting.
 *
 * @tparam To The target base type.
 * @tparam From The source derived type.
 * @param val The pointer to cast.
 * @return The cast pointer.
 */
template <typename To, typename From>
    requires is_base_of_v<To, From>
[[nodiscard]] constexpr To* cast(From* val) {
    return static_cast<To*>(val);
}

/**
 * @brief Checked dynamic cast from a derived type to a base type.
 *
 * Returns nullptr if the pointer does not actually point to the target type.
 *
 * @tparam To The target base type.
 * @tparam From The source derived type.
 * @param val The pointer to cast.
 * @return The cast pointer, or nullptr if the type does not match.
 */
template <typename To, typename From>
    requires is_base_of_v<To, From>
[[nodiscard]] To* dyn_cast(From* val) {
    if (isa<To>(val)) {
        return cast<To>(val);
    }
    return nullptr;
}

}  // namespace prism
