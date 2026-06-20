#include "prism/ir/type.hpp"

namespace prism::ir {

/** @brief Prints the function type signature as a string, e.g. "i32 (i32, i32)". */
String FunctionType::print() const {
    String result = return_type_->print() + " (";
    for (size_t i = 0; i < param_types_.size(); ++i) {
        if (i > 0) result.append(", ");
        result.append(param_types_[i]->print());
    }
    if (is_variadic_) {
        if (!param_types_.empty()) result.append(", ");
        result.append("...");
    }
    result.append(")");
    return result;
}

}  // namespace prism::ir
