#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace prism::ir {

/** @brief Enumeration of all type kinds in the IR. */
enum class TypeID {
    Void,      ///< Void type (no return value).
    Integer,   ///< Integer type with a specified bit width.
    Float,     ///< Floating-point type (float or double).
    Pointer,   ///< Pointer to another type.
    Function,  ///< Function type with return and parameter types.
};

/**
 * @brief Base class for all types in the IR.
 *
 * Provides a common interface for querying type properties such as size
 * and category. Concrete subclasses represent specific type kinds.
 */
class Type {
public:
    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~Type() = default;

    /**
     * @brief Gets the type identifier.
     * @return The TypeID for this type.
     */
    TypeID type_id() const { return type_id_; }

    /**
     * @brief Checks if this is a void type.
     * @return True if this is a VoidType, false otherwise.
     */
    virtual bool is_void_type() const { return false; }

    /**
     * @brief Checks if this is an integer type.
     * @return True if this is an IntegerType, false otherwise.
     */
    virtual bool is_integer_type() const { return false; }

    /**
     * @brief Checks if this is a float type.
     * @return True if this is a FloatType, false otherwise.
     */
    virtual bool is_float_type() const { return false; }

    /**
     * @brief Checks if this is a pointer type.
     * @return True if this is a PointerType, false otherwise.
     */
    virtual bool is_pointer_type() const { return false; }

    /**
     * @brief Checks if this is a function type.
     * @return True if this is a FunctionType, false otherwise.
     */
    virtual bool is_function_type() const { return false; }

    /**
     * @brief Gets the size of the type in bits.
     * @return Size in bits.
     */
    virtual unsigned get_size_in_bits() const = 0;

    /**
     * @brief Returns a string representation of this type.
     * @return String representation for printing/debugging.
     */
    virtual String print() const = 0;

    /**
     * @brief Equality comparison by identity.
     * @param other The other Type to compare.
     * @return True if both types are the same object.
     */
    bool operator==(const Type& other) const { return this == &other; }

protected:
    /**
     * @brief Constructs a Type with the given identifier.
     * @param id The TypeID for this type.
     */
    explicit Type(TypeID id) : type_id_(id) {}

private:
    TypeID type_id_;
};

/**
 * @brief Represents the void type (no value).
 */
class VoidType final : public Type {
public:
    /** @brief Constructs a VoidType. */
    VoidType() : Type(TypeID::Void) {}

    /** @brief Returns true since this is a void type. */
    bool is_void_type() const override { return true; }

    /** @brief Returns 0 since void has no size. */
    unsigned get_size_in_bits() const override { return 0; }

    /** @brief Returns "void". */
    String print() const override { return "void"; }
};

/**
 * @brief Represents an integer type with a specific bit width.
 */
class IntegerType final : public Type {
public:
    /**
     * @brief Constructs an IntegerType with the given bit width.
     * @param width The bit width of the integer.
     */
    explicit IntegerType(unsigned width) : Type(TypeID::Integer), bit_width_(width) {}

    /** @brief Returns true since this is an integer type. */
    bool is_integer_type() const override { return true; }

    /**
     * @brief Gets the bit width of this integer type.
     * @return The bit width (e.g., 1, 8, 16, 32, 64).
     */
    unsigned bit_width() const { return bit_width_; }

    /**
     * @brief Gets the size of the type in bits.
     * @return The bit width.
     */
    unsigned get_size_in_bits() const override { return bit_width_; }

    /**
     * @brief Returns a string like "i32" or "i64".
     * @return String representation.
     */
    String print() const override {
        char buf[16];
        int len = ::snprintf(buf, sizeof(buf), "i%u", bit_width_);
        return String(buf, static_cast<size_t>(len));
    }

private:
    unsigned bit_width_;
};

/**
 * @brief Represents a floating-point type (float or double).
 */
class FloatType final : public Type {
public:
    /** @brief Enumeration of floating-point kinds. */
    enum class FloatKind {
        Float,  ///< 32-bit single-precision float.
        Double  ///< 64-bit double-precision float.
    };

    /**
     * @brief Constructs a FloatType with the given kind.
     * @param kind The floating-point kind (Float or Double).
     */
    explicit FloatType(FloatKind kind) : Type(TypeID::Float), kind_(kind) {}

    /** @brief Returns true since this is a float type. */
    bool is_float_type() const override { return true; }

    /**
     * @brief Gets the floating-point kind.
     * @return The FloatKind (Float or Double).
     */
    FloatKind float_kind() const { return kind_; }

    /**
     * @brief Gets the size of the type in bits.
     * @return 32 for Float, 64 for Double.
     */
    unsigned get_size_in_bits() const override { return kind_ == FloatKind::Float ? 32 : 64; }

    /**
     * @brief Returns "float" or "double".
     * @return String representation.
     */
    String print() const override { return kind_ == FloatKind::Float ? "float" : "double"; }

private:
    FloatKind kind_;
};

/**
 * @brief Represents a pointer type pointing to another type.
 */
class PointerType final : public Type {
public:
    /**
     * @brief Constructs a PointerType pointing to the given type.
     * @param pointee Pointer to the pointed-to type.
     */
    explicit PointerType(Type* pointee) : Type(TypeID::Pointer), pointee_(pointee) {}

    /** @brief Returns true since this is a pointer type. */
    bool is_pointer_type() const override { return true; }

    /**
     * @brief Gets the type being pointed to.
     * @return Pointer to the pointee type.
     */
    Type* pointee_type() const { return pointee_; }

    /**
     * @brief Gets the size of the type in bits.
     * @return 64 bits (pointer size).
     */
    unsigned get_size_in_bits() const override { return 64; }

    /**
     * @brief Returns the pointee type name followed by "*".
     * @return String representation.
     */
    String print() const override { return pointee_->print() + "*"; }

private:
    Type* pointee_;
};

/**
 * @brief Represents a function type with return type and parameter types.
 */
class FunctionType final : public Type {
public:
    /**
     * @brief Constructs a FunctionType.
     * @param return_type Pointer to the return type.
     * @param param_types Vector of parameter types.
     * @param is_variadic Whether the function accepts variadic arguments.
     */
    FunctionType(Type* return_type, Vector<Type*> param_types, bool is_variadic = false)
        : Type(TypeID::Function)
        , return_type_(return_type)
        , param_types_(static_cast<Vector<Type*>&&>(param_types))
        , is_variadic_(is_variadic) {}

    /** @brief Returns true since this is a function type. */
    bool is_function_type() const override { return true; }

    /**
     * @brief Gets the return type of this function.
     * @return Pointer to the return type.
     */
    Type* return_type() const { return return_type_; }

    /**
     * @brief Gets the parameter types of this function.
     * @return Const reference to the vector of parameter types.
     */
    const Vector<Type*>& param_types() const { return param_types_; }

    /**
     * @brief Gets the number of parameters.
     * @return The parameter count.
     */
    size_t param_count() const { return param_types_.size(); }

    /**
     * @brief Checks if this function is variadic.
     * @return True if variadic, false otherwise.
     */
    bool is_variadic() const { return is_variadic_; }

    /**
     * @brief Gets the size of the type in bits.
     * @return 0 (function types have no direct size).
     */
    unsigned get_size_in_bits() const override { return 0; }

    /**
     * @brief Returns a string representation like "i32 (i32, float)".
     * @return String representation.
     */
    String print() const override;

private:
    Type* return_type_;
    Vector<Type*> param_types_;
    bool is_variadic_;
};

}  // namespace prism::ir
