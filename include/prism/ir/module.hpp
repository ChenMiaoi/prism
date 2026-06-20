#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/memory/unique_ptr.hpp"
#include "prism/core/string/string.hpp"
#include "prism/ir/function.hpp"
#include "prism/ir/type.hpp"
#include "prism/ir/value.hpp"

namespace prism::ir {

class Context;

/**
 * @brief Top-level container for a collection of functions in the IR.
 *
 * A Module owns a set of Functions and provides lookup/insertion by name.
 */
class Module {
public:
    /**
     * @brief Constructs a Module.
     * @param name The module name.
     * @param ctx Reference to the IR Context.
     */
    Module(const char* name, Context& ctx);

    /**
     * @brief Gets the module name.
     * @return Const reference to the name string.
     */
    const String& get_name() const { return name_; }

    /**
     * @brief Gets the IR context.
     * @return Reference to the Context.
     */
    Context& get_context() { return ctx_; }

    /**
     * @brief Gets a function by name, or inserts a new declaration if not found.
     * @param name The function name.
     * @param ty The function type.
     * @return Pointer to the existing or newly created Function.
     */
    Function* get_or_insert_function(const char* name, FunctionType* ty);

    /**
     * @brief Looks up a function by name.
     * @param name The function name.
     * @return Pointer to the Function if found, nullptr otherwise.
     */
    Function* get_function(const char* name);

    /**
     * @brief Gets the list of functions (mutable).
     * @return Reference to the function vector.
     */
    Vector<prism::UniquePtr<Function>>& functions() { return functions_; }

    /**
     * @brief Gets the list of functions (const).
     * @return Const reference to the function vector.
     */
    const Vector<prism::UniquePtr<Function>>& functions() const { return functions_; }

    /**
     * @brief Returns a string representation of this module.
     * @return String representation for printing/debugging.
     */
    String print() const;

private:
    String name_;
    Context& ctx_;
    Vector<prism::UniquePtr<Function>> functions_;
};

}  // namespace prism::ir
