#include "prism/ir/context.hpp"

#include "prism/ir/constant.hpp"
#include "prism/ir/type.hpp"

namespace prism::ir {

/** @brief Constructs a Context with built-in scalar types (void, i1, i8, i32, i64, float, double). */
Context::Context()
    : void_ty_(0)
    , i1_ty_(1)
    , i8_ty_(8)
    , i32_ty_(32)
    , i64_ty_(64)
    , float_ty_(FloatType::FloatKind::Float)
    , double_ty_(FloatType::FloatKind::Double) {
}

/** @brief Destroys the Context and frees all owned IR objects. */
Context::~Context() {
    for (size_t i = 0; i < owned_ptrs_.size(); ++i) {
        owned_deleters_[i]->destroy(owned_ptrs_[i]);
        delete owned_deleters_[i];
    }
}

/** @brief Returns the void type. */
Type* Context::void_type() {
    return &void_ty_;
}

/** @brief Returns the 1-bit integer type (boolean). */
IntegerType* Context::i1_type() {
    return &i1_ty_;
}

/** @brief Returns the 8-bit integer type. */
IntegerType* Context::i8_type() {
    return &i8_ty_;
}

/** @brief Returns the 32-bit integer type. */
IntegerType* Context::i32_type() {
    return &i32_ty_;
}

/** @brief Returns the 64-bit integer type. */
IntegerType* Context::i64_type() {
    return &i64_ty_;
}

/** @brief Returns the 32-bit float type. */
FloatType* Context::float_type() {
    return &float_ty_;
}

/** @brief Returns the 64-bit double type. */
FloatType* Context::double_type() {
    return &double_ty_;
}

/** @brief Returns or creates a pointer type pointing to the given pointee type. */
PointerType* Context::get_pointer_type(Type* pointee) {
    for (size_t i = 0; i < pointer_types_.size(); ++i) {
        if (pointer_types_[i].pointee == pointee) return pointer_types_[i].ptrType;
    }
    auto* ptr = new PointerType(pointee);
    pointer_types_.push_back({pointee, ptr});
    owned_ptrs_.push_back(ptr);
    owned_deleters_.push_back(new Deleter<PointerType>());
    return ptr;
}

/** @brief Returns or creates a function type with the given return type, parameter types, and variadic flag. */
FunctionType* Context::get_function_type(Type* return_type, Vector<Type*> param_types, bool is_variadic) {
    for (size_t i = 0; i < function_types_.size(); ++i) {
        auto* existing = function_types_[i].fnType;
        if (function_types_[i].retType != return_type || existing->is_variadic() != is_variadic ||
            existing->param_count() != param_types.size()) {
            continue;
        }
        bool same_params = true;
        for (size_t param = 0; param < param_types.size(); ++param) {
            if (existing->param_types()[param] != param_types[param]) {
                same_params = false;
                break;
            }
        }
        if (same_params) return existing;
    }
    auto* fn_ty = new FunctionType(return_type, static_cast<Vector<Type*>&&>(param_types), is_variadic);
    function_types_.push_back({return_type, fn_ty});
    owned_ptrs_.push_back(fn_ty);
    owned_deleters_.push_back(new Deleter<FunctionType>());
    return fn_ty;
}

/** @brief Returns or creates a constant integer value. */
ConstantInt* Context::get_int_constant(int64_t value, IntegerType* ty) {
    if (!ty) ty = &i64_ty_;
    for (size_t i = 0; i < int_constants_.size(); ++i) {
        if (int_constants_[i].type == ty && int_constants_[i].value == value) return int_constants_[i].constant;
    }
    auto* c = new ConstantInt(ty, value);
    int_constants_.push_back({ty, value, c});
    owned_ptrs_.push_back(c);
    owned_deleters_.push_back(new Deleter<ConstantInt>());
    return c;
}

/** @brief Returns or creates a constant floating-point value. */
ConstantFP* Context::get_float_constant(double value, FloatType* ty) {
    if (!ty) ty = &double_ty_;
    for (size_t i = 0; i < float_constants_.size(); ++i) {
        if (float_constants_[i].type == ty && float_constants_[i].value == value) return float_constants_[i].constant;
    }
    auto* c = new ConstantFP(ty, value);
    float_constants_.push_back({ty, value, c});
    owned_ptrs_.push_back(c);
    owned_deleters_.push_back(new Deleter<ConstantFP>());
    return c;
}

}  // namespace prism::ir
