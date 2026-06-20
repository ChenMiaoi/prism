#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/memory/unique_ptr.hpp"
#include "prism/ir/type.hpp"

#include <cstdint>
#include <cstring>
#include <functional>

namespace prism::ir {

class ConstantInt;
class ConstantFP;
class Value;

/** @brief Cache entry for uniqued pointer types. */
struct PtrCacheEntry {
    Type* pointee;         ///< The pointed-to type.
    PointerType* ptrType;  ///< The cached pointer type.
};

/** @brief Cache entry for uniqued function types. */
struct FnCacheEntry {
    Type* retType;         ///< The return type.
    FunctionType* fnType;  ///< The cached function type.
};

/** @brief Cache entry for uniqued integer constants. */
struct IntCacheEntry {
    Type* type;             ///< The integer type.
    int64_t value;          ///< The integer value.
    ConstantInt* constant;  ///< The cached constant.
};

/** @brief Cache entry for uniqued floating-point constants. */
struct FloatCacheEntry {
    Type* type;            ///< The float type.
    double value;          ///< The floating-point value.
    ConstantFP* constant;  ///< The cached constant.
};

/**
 * @brief Owner and factory for all IR types and constants.
 *
 * The Context manages the lifetime of allocated IR objects and provides
 * caching/uniquing for types and constants. It is the central object
 * needed to create any IR entity.
 */
class Context {
public:
    /** @brief Constructs a Context. */
    Context();

    /** @brief Destroys the Context and all owned objects. */
    ~Context();

    /** @brief Non-copyable. */
    Context(const Context&) = delete;

    /** @brief Non-copyable. */
    Context& operator=(const Context&) = delete;

    /**
     * @brief Creates and owns a new IR object.
     * @tparam T The type of object to create.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the constructor.
     * @return Pointer to the newly created object.
     */
    template <typename T, typename... Args>
    T* create(Args&&... args) {
        T* raw = new T(forward<Args>(args)...);
        owned_ptrs_.push_back(raw);
        owned_deleters_.push_back(new Deleter<T>());
        return raw;
    }

    /**
     * @brief Gets the void type.
     * @return Pointer to the void Type.
     */
    Type* void_type();

    /**
     * @brief Gets the 1-bit integer type (boolean).
     * @return Pointer to the i1 IntegerType.
     */
    IntegerType* i1_type();

    /**
     * @brief Gets the 8-bit integer type.
     * @return Pointer to the i8 IntegerType.
     */
    IntegerType* i8_type();

    /**
     * @brief Gets the 32-bit integer type.
     * @return Pointer to the i32 IntegerType.
     */
    IntegerType* i32_type();

    /**
     * @brief Gets the 64-bit integer type.
     * @return Pointer to the i64 IntegerType.
     */
    IntegerType* i64_type();

    /**
     * @brief Gets the 32-bit float type.
     * @return Pointer to the float FloatType.
     */
    FloatType* float_type();

    /**
     * @brief Gets the 64-bit double type.
     * @return Pointer to the double FloatType.
     */
    FloatType* double_type();

    /**
     * @brief Gets or creates a pointer type.
     * @param pointee The type being pointed to.
     * @return Pointer to the PointerType.
     */
    PointerType* get_pointer_type(Type* pointee);

    /**
     * @brief Gets or creates a function type.
     * @param return_type The return type.
     * @param param_types The parameter types.
     * @param is_variadic Whether the function is variadic.
     * @return Pointer to the FunctionType.
     */
    FunctionType* get_function_type(Type* return_type, Vector<Type*> param_types, bool is_variadic = false);

    /**
     * @brief Gets or creates an integer constant.
     * @param value The integer value.
     * @param ty Optional type; defaults to i64 if null.
     * @return Pointer to the ConstantInt.
     */
    ConstantInt* get_int_constant(int64_t value, IntegerType* ty = nullptr);

    /**
     * @brief Gets or creates a floating-point constant.
     * @param value The floating-point value.
     * @param ty Optional type; defaults to double if null.
     * @return Pointer to the ConstantFP.
     */
    ConstantFP* get_float_constant(double value, FloatType* ty = nullptr);

private:
    struct DeleterBase {
        virtual ~DeleterBase() = default;
        virtual void destroy(void* ptr) = 0;
    };

    template <typename T>
    struct Deleter final : DeleterBase {
        void destroy(void* ptr) override { delete static_cast<T*>(ptr); }
    };

    IntegerType void_ty_;
    IntegerType i1_ty_;
    IntegerType i8_ty_;
    IntegerType i32_ty_;
    IntegerType i64_ty_;
    FloatType float_ty_;
    FloatType double_ty_;

    Vector<PtrCacheEntry> pointer_types_;
    Vector<FnCacheEntry> function_types_;
    Vector<IntCacheEntry> int_constants_;
    Vector<FloatCacheEntry> float_constants_;

    Vector<void*> owned_ptrs_;
    Vector<DeleterBase*> owned_deleters_;
};

}  // namespace prism::ir
