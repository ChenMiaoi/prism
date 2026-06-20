#include "prism/ir/constant.hpp"

#include "prism/ir/context.hpp"
#include "prism/ir/type.hpp"

#include <cstdio>

namespace prism::ir {

/** @brief Constructs a ConstantInt with the given integer type and value. */
ConstantInt::ConstantInt(IntegerType* ty, int64_t value) : Constant(ty, ""), value_(value) {
    char buf[32];
    ::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(value));
    set_name(buf);
}

/** @brief Returns the integer type of this constant. */
IntegerType* ConstantInt::get_type() const {
    return static_cast<IntegerType*>(Value::get_type());
}

/** @brief Prints the constant value as a decimal string. */
String ConstantInt::print() const {
    char buf[32];
    int len = ::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(value_));
    return String(buf, static_cast<size_t>(len));
}

/** @brief Returns or creates a ConstantInt in the given context. */
ConstantInt* ConstantInt::get(Context& ctx, int64_t value, IntegerType* ty) {
    if (!ty) ty = ctx.i64_type();
    return ctx.create<ConstantInt>(ty, value);
}

/** @brief Constructs a ConstantFP with the given float type and value. */
ConstantFP::ConstantFP(FloatType* ty, double value) : Constant(ty, ""), value_(value) {
    char buf[32];
    ::snprintf(buf, sizeof(buf), "%f", value);
    set_name(buf);
}

/** @brief Returns the float type of this constant. */
FloatType* ConstantFP::get_type() const {
    return static_cast<FloatType*>(Value::get_type());
}

/** @brief Prints the constant value as a decimal string. */
String ConstantFP::print() const {
    char buf[32];
    int len = ::snprintf(buf, sizeof(buf), "%f", value_);
    return String(buf, static_cast<size_t>(len));
}

/** @brief Returns or creates a ConstantFP in the given context. */
ConstantFP* ConstantFP::get(Context& ctx, double value, FloatType* ty) {
    if (!ty) ty = ctx.double_type();
    return ctx.create<ConstantFP>(ty, value);
}

}  // namespace prism::ir
