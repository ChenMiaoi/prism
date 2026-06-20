#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"
#include "prism/ir/use.hpp"

namespace prism::ir {

class Type;

/**
 * @brief Base class for all SSA values in the IR.
 *
 * Represents a value produced by an instruction or constant. Each value has a type,
 * an optional name, and a list of uses that reference it.
 */
class Value {
public:
    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~Value() = default;

    /**
     * @brief Gets the type of this value.
     * @return Pointer to the Type of this value.
     */
    Type* get_type() const;

    /**
     * @brief Gets the name of this value.
     * @return Const reference to the name string.
     */
    const String& get_name() const;

    /**
     * @brief Sets the name of this value.
     * @param name The new name to assign.
     */
    void set_name(const char* name);

    /**
     * @brief Gets all uses of this value.
     * @return Const reference to the vector of Use pointers.
     */
    const Vector<Use*>& uses() const;

    /**
     * @brief Adds a use that references this value.
     * @param use Pointer to the Use to add.
     */
    void add_use(Use* use);

    /**
     * @brief Removes a use that references this value.
     * @param use Pointer to the Use to remove.
     */
    void remove_use(Use* use);

    /**
     * @brief Checks if this value has no uses.
     * @return True if the use list is empty, false otherwise.
     */
    bool use_empty() const;

    /**
     * @brief Returns a string representation of this value.
     * @return String representation for printing/debugging.
     */
    virtual String print() const;

    /**
     * @brief Equality comparison by identity.
     * @param other The other Value to compare.
     * @return True if both values are the same object.
     */
    bool operator==(const Value& other) const { return this == &other; }

protected:
    /**
     * @brief Constructs a Value with a given type and optional name.
     * @param ty Pointer to the Type of this value.
     * @param name Optional name for the value.
     */
    Value(Type* ty, const char* name = "");

private:
    Type* type_ = nullptr;
    String name_;
    Vector<Use*> uses_;
};

}  // namespace prism::ir
