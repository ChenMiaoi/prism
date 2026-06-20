#include "prism/ir/module.hpp"

#include "prism/ir/context.hpp"

namespace prism::ir {

/** @brief Constructs a Module with a name and a reference to its owning context. */
Module::Module(const char* name, Context& ctx) : name_(name), ctx_(ctx) {
}

/** @brief Returns an existing function with the given name, or creates a new one if not found. */
Function* Module::get_or_insert_function(const char* name, FunctionType* ty) {
    Function* existing = get_function(name);
    if (existing) return existing;
    auto fn = prism::make_unique<Function>(ty, name, this);
    Function* ptr = fn.get();
    functions_.push_back(static_cast<UniquePtr<Function>&&>(fn));
    return ptr;
}

/** @brief Returns the function with the given name, or nullptr if not found. */
Function* Module::get_function(const char* name) {
    String target(name);
    for (size_t i = 0; i < functions_.size(); ++i) {
        if (functions_[i]->get_name() == target) return functions_[i].get();
    }
    return nullptr;
}

/** @brief Prints the entire module including all functions as a string. */
String Module::print() const {
    String result;
    result.append("module \"");
    result.append(name_);
    result.append("\" {\n");
    for (size_t i = 0; i < functions_.size(); ++i) {
        result.append(functions_[i]->print());
        result.append("\n\n");
    }
    result.append("}\n");
    return result;
}

}  // namespace prism::ir
