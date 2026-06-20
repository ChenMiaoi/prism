#pragma once

#include "prism/core/utility/move.hpp"

#include <cstddef>
#include <new>

namespace prism {

/** @brief Tag type used to indicate an empty Optional. */
struct nullopt_t {
    explicit constexpr nullopt_t(int) {}
};
/** @brief Global nullopt constant for constructing empty Optionals. */
inline constexpr nullopt_t nullopt{0};

/** @brief A type-safe container that may or may not hold a value of type @p T. */
template <typename T>
class Optional {
    /** @brief Internal storage union for the value or a dummy byte. */
    union Storage {
        char dummy;  ///< Placeholder for the empty state.
        T value;     ///< Storage for the contained value.
        /** @brief Constructs the dummy byte. */
        Storage() : dummy(0) {}
        /** @brief Destructor is intentionally trivial for the dummy. */
        ~Storage() {}
    };

public:
    /** @brief Default-constructs an empty Optional. */
    Optional() noexcept : has_value_(false) {}
    /** @brief Constructs an empty Optional.
     *  @param nullopt_tag Must be nullopt. */
    Optional(nullopt_t) noexcept : has_value_(false) {}

    /** @brief Constructs an Optional holding a copy of @p value.
     *  @param value The value to store. */
    Optional(const T& value) : has_value_(true) { new (&storage_.value) T(value); }

    /** @brief Constructs an Optional holding a moved @p value.
     *  @param value The value to move. */
    Optional(T&& value) : has_value_(true) { new (&storage_.value) T(move(value)); }

    /** @brief Copy constructor.
     *  @param other The Optional to copy from. */
    Optional(const Optional& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(other.storage_.value);
        }
    }

    /** @brief Move constructor.
     *  @param other The Optional to move from. */
    Optional(Optional&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(move(other.storage_.value));
            other.storage_.value.~T();
            other.has_value_ = false;
        }
    }

    /** @brief Destructor. Destroys the contained value if present. */
    ~Optional() {
        if (has_value_) {
            storage_.value.~T();
        }
    }

    /** @brief Resets the Optional to empty.
     *  @param nullopt_tag Must be nullopt.
     *  @return Reference to this Optional. */
    Optional& operator=(nullopt_t) {
        if (has_value_) {
            storage_.value.~T();
            has_value_ = false;
        }
        return *this;
    }

    /** @brief Copy assignment.
     *  @param other The Optional to copy from.
     *  @return Reference to this Optional. */
    Optional& operator=(const Optional& other) {
        if (this != &other) {
            if (has_value_) storage_.value.~T();
            has_value_ = other.has_value_;
            if (has_value_) {
                new (&storage_.value) T(other.storage_.value);
            }
        }
        return *this;
    }

    /** @brief Move assignment.
     *  @param other The Optional to move from.
     *  @return Reference to this Optional. */
    Optional& operator=(Optional&& other) noexcept {
        if (this != &other) {
            if (has_value_) storage_.value.~T();
            has_value_ = other.has_value_;
            if (has_value_) {
                new (&storage_.value) T(move(other.storage_.value));
                other.storage_.value.~T();
                other.has_value_ = false;
            }
        }
        return *this;
    }

    /** @brief Returns true if the Optional contains a value. */
    bool has_value() const noexcept { return has_value_; }
    /** @brief Converts to bool, returning true if a value is present. */
    explicit operator bool() const noexcept { return has_value_; }

    /** @brief Returns a reference to the contained value (lvalue). */
    T& value() & { return storage_.value; }
    /** @brief Returns a const reference to the contained value. */
    const T& value() const& { return storage_.value; }
    /** @brief Returns an rvalue reference to the contained value. */
    T&& value() && { return move(storage_.value); }

    /** @brief Dereference operator. Returns a reference to the contained value. */
    T& operator*() { return storage_.value; }
    /** @brief Const dereference operator. */
    const T& operator*() const { return storage_.value; }

    /** @brief Arrow operator. Returns a pointer to the contained value. */
    T* operator->() { return &storage_.value; }
    /** @brief Const arrow operator. */
    const T* operator->() const { return &storage_.value; }

    /** @brief Returns the contained value or @p default_value if empty.
     *  @param default_value The fallback value.
     *  @return The contained value or @p default_value. */
    T value_or(const T& default_value) const& { return has_value_ ? storage_.value : default_value; }

    /** @brief Returns the contained value or a moved @p default_value if empty.
     *  @param default_value The fallback value.
     *  @return The contained value or the moved default. */
    T value_or(T&& default_value) && { return has_value_ ? move(storage_.value) : move(default_value); }

    /** @brief Destroys the contained value, making the Optional empty. */
    void reset() {
        if (has_value_) {
            storage_.value.~T();
            has_value_ = false;
        }
    }

    /** @brief Constructs a value in-place from the given arguments.
     *  @param args Arguments forwarded to the T constructor.
     *  @return Reference to the emplaced value. */
    template <typename... Args>
    T& emplace(Args&&... args) {
        if (has_value_) storage_.value.~T();
        new (&storage_.value) T(forward<Args>(args)...);
        has_value_ = true;
        return storage_.value;
    }

    /** @brief Swaps the contents with another Optional.
     *  @param other The Optional to swap with. */
    void swap(Optional& other) noexcept {
        if (has_value_ && other.has_value_) {
            using prism::swap;
            swap(storage_.value, other.storage_.value);
        } else if (has_value_) {
            new (&other.storage_.value) T(move(storage_.value));
            storage_.value.~T();
            other.has_value_ = true;
            has_value_ = false;
        } else if (other.has_value_) {
            new (&storage_.value) T(move(other.storage_.value));
            other.storage_.value.~T();
            has_value_ = true;
            other.has_value_ = false;
        }
    }

private:
    Storage storage_;  ///< Internal union storage.
    bool has_value_;   ///< Whether the Optional currently holds a value.
};

}  // namespace prism
