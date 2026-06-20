#pragma once

#include "prism/core/string/string.hpp"
#include "prism/ir/type.hpp"
#include "prism/ir/value.hpp"

#include <cstdint>

namespace prism::ir {

class Context;

/**
 * @brief Base class for all constant values in the IR.
 *
 * Constants are immutable values that do not change during program execution.
 */
class Constant : public Value {
protected:
    /**
     * @brief Constructs a Constant with a given type and name.
     * @param ty Pointer to the Type of this constant.
     * @param name The constant's name.
     */
    Constant(Type* ty, const char* name) : Value(ty, name) {}
};

/**
 * @brief Represents an integer constant value.
 */
class ConstantInt final : public Constant {
public:
    /**
     * @brief Gets the integer value.
     * @return The int64_t value.
     */
    int64_t get_value() const { return value_; }

    /**
     * @brief Gets the type of this constant as an IntegerType.
     * @return Pointer to the IntegerType.
     */
    IntegerType* get_type() const;

    /**
     * @brief Returns a string representation like "42".
     * @return String representation.
     */
    String print() const override;

    /**
     * @brief Creates or retrieves a ConstantInt from the context.
     * @param ctx Reference to the Context for uniquing.
     * @param value The integer value.
     * @param ty Optional type; defaults to i64 if null.
     * @return Pointer to the ConstantInt.
     */
    static ConstantInt* get(Context& ctx, int64_t value, IntegerType* ty = nullptr);

    /**
     * @brief Constructs a ConstantInt directly.
     * @param ty Pointer to the IntegerType.
     * @param value The integer value.
     */
    ConstantInt(IntegerType* ty, int64_t value);

    friend class Context;

private:
    int64_t value_;
};

/**
 * @brief Represents a floating-point constant value.
 */
class ConstantFP final : public Constant {
public:
    /**
     * @brief Gets the floating-point value.
     * @return The double value.
     */
    double get_value() const { return value_; }

    /**
     * @brief Gets the type of this constant as a FloatType.
     * @return Pointer to the FloatType.
     */
    FloatType* get_type() const;

    /**
     * @brief Returns a string representation like "3.14".
     * @return String representation.
     */
    String print() const override;

    /**
     * @brief Creates or retrieves a ConstantFP from the context.
     * @param ctx Reference to the Context for uniquing.
     * @param value The floating-point value.
     * @param ty Optional type; defaults to double if null.
     * @return Pointer to the ConstantFP.
     */
    static ConstantFP* get(Context& ctx, double value, FloatType* ty = nullptr);

    /**
     * @brief Constructs a ConstantFP directly.
     * @param ty Pointer to the FloatType.
     * @param value The floating-point value.
     */
    ConstantFP(FloatType* ty, double value);

    friend class Context;

private:
    double value_;
};

}  // namespace prism::ir
