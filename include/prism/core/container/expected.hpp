#pragma once

#include "prism/core/utility/move.hpp"

#include <cstddef>
#include <new>
#include <utility>

namespace prism {
class Error;

/** @brief A wrapper for holding an error value in an Expected. */
template <typename E>
class Unexpected {
public:
    /** @brief Constructs an Unexpected with a copy of @p error.
     *  @param error The error value to store. */
    explicit Unexpected(const E& error) : error_(error) {}
    /** @brief Constructs an Unexpected with a moved @p error.
     *  @param error The error value to move. */
    explicit Unexpected(E&& error) : error_(move(error)) {}

    /** @brief Returns a const reference to the error. */
    const E& error() const& { return error_; }
    /** @brief Returns a reference to the error. */
    E& error() & { return error_; }
    /** @brief Returns an rvalue reference to the error. */
    E&& error() && { return move(error_); }

private:
    E error_;  ///< The stored error value.
};

/** @brief A type that holds either a value of type @p T or an error of type @p E. */
template <typename T, typename E = Error>
class Expected {
    /** @brief Internal storage union for value or error. */
    union Storage {
        char dummy;  ///< Placeholder for the empty state.
        T value;     ///< Storage for the success value.
        E error;     ///< Storage for the error value.
        /** @brief Constructs the dummy byte. */
        Storage() : dummy(0) {}
        /** @brief Destructor is intentionally trivial. */
        ~Storage() {}
    };

public:
    /** @brief Constructs an Expected holding a copy of @p value.
     *  @param value The success value to store. */
    Expected(const T& value) : has_value_(true) { new (&storage_.value) T(value); }

    /** @brief Constructs an Expected holding a moved @p value.
     *  @param value The success value to move. */
    Expected(T&& value) : has_value_(true) { new (&storage_.value) T(move(value)); }

    /** @brief Constructs an Expected holding an error from an Unexpected.
     *  @param err The unexpected error. */
    Expected(const Unexpected<E>& err) : has_value_(false) { new (&storage_.error) E(err.error()); }

    /** @brief Constructs an Expected holding a moved error from an Unexpected.
     *  @param err The unexpected error to move. */
    Expected(Unexpected<E>&& err) : has_value_(false) { new (&storage_.error) E(move(err.error())); }

    /** @brief Copy constructor.
     *  @param other The Expected to copy from. */
    Expected(const Expected& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(other.storage_.value);
        } else {
            new (&storage_.error) E(other.storage_.error);
        }
    }

    /** @brief Move constructor.
     *  @param other The Expected to move from. */
    Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(move(other.storage_.value));
        } else {
            new (&storage_.error) E(move(other.storage_.error));
        }
    }

    /** @brief Destructor. Destroys the contained value or error. */
    ~Expected() {
        if (has_value_) {
            storage_.value.~T();
        } else {
            storage_.error.~E();
        }
    }

    /** @brief Copy assignment.
     *  @param other The Expected to copy from.
     *  @return Reference to this Expected. */
    Expected& operator=(const Expected& other) {
        if (this != &other) {
            if (has_value_)
                storage_.value.~T();
            else
                storage_.error.~E();
            has_value_ = other.has_value_;
            if (has_value_) {
                new (&storage_.value) T(other.storage_.value);
            } else {
                new (&storage_.error) E(other.storage_.error);
            }
        }
        return *this;
    }

    /** @brief Move assignment.
     *  @param other The Expected to move from.
     *  @return Reference to this Expected. */
    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            if (has_value_)
                storage_.value.~T();
            else
                storage_.error.~E();
            has_value_ = other.has_value_;
            if (has_value_) {
                new (&storage_.value) T(move(other.storage_.value));
            } else {
                new (&storage_.error) E(move(other.storage_.error));
            }
        }
        return *this;
    }

    /** @brief Returns true if the Expected holds a value (not an error). */
    bool has_value() const noexcept { return has_value_; }
    /** @brief Converts to bool, returning true if a value is present. */
    explicit operator bool() const noexcept { return has_value_; }

    /** @brief Returns a reference to the contained value (lvalue). */
    T& value() & { return storage_.value; }
    /** @brief Returns a const reference to the contained value. */
    const T& value() const& { return storage_.value; }
    /** @brief Returns an rvalue reference to the contained value. */
    T&& value() && { return move(storage_.value); }

    /** @brief Returns a reference to the contained error (lvalue). */
    E& error() & { return storage_.error; }
    /** @brief Returns a const reference to the contained error. */
    const E& error() const& { return storage_.error; }
    /** @brief Returns an rvalue reference to the contained error. */
    E&& error() && { return move(storage_.error); }

    /** @brief Dereference operator. Returns a reference to the contained value (lvalue). */
    T& operator*() & { return storage_.value; }
    /** @brief Const dereference operator. */
    const T& operator*() const& { return storage_.value; }
    /** @brief Rvalue dereference operator. */
    T&& operator*() && { return move(storage_.value); }

    /** @brief Arrow operator. Returns a pointer to the contained value. */
    T* operator->() { return &storage_.value; }
    /** @brief Const arrow operator. */
    const T* operator->() const { return &storage_.value; }

    /** @brief Returns the contained value or a constructed default from @p default_value.
     *  @param default_value The fallback value.
     *  @return The contained value or @p default_value. */
    template <typename U>
    T value_or(U&& default_value) const& {
        return has_value_ ? storage_.value : T(forward<U>(default_value));
    }

    /** @brief Swaps the contents with another Expected.
     *  @param other The Expected to swap with. */
    void swap(Expected& other) noexcept {
        if (has_value_ && other.has_value_) {
            using prism::swap;
            swap(storage_.value, other.storage_.value);
        } else if (!has_value_ && !other.has_value_) {
            using prism::swap;
            swap(storage_.error, other.storage_.error);
        }
    }

private:
    Storage storage_;  ///< Internal union storage.
    bool has_value_;   ///< Whether the Expected currently holds a value.
};

/** @brief Specialization of Expected<void, E> for operations that return no value on success. */
template <typename E>
class Expected<void, E> {
public:
    /** @brief Default constructor. Constructs a successful (value-holding) Expected. */
    Expected() : has_value_(true) {}

    /** @brief Constructs an Expected holding an error from an Unexpected.
     *  @param err The unexpected error. */
    Expected(const Unexpected<E>& err) : has_value_(false) { new (&storage_.error) E(err.error()); }

    /** @brief Constructs an Expected holding a moved error from an Unexpected.
     *  @param err The unexpected error to move. */
    Expected(Unexpected<E>&& err) : has_value_(false) { new (&storage_.error) E(move(err.error())); }

    /** @brief Copy constructor.
     *  @param other The Expected to copy from. */
    Expected(const Expected& other) : has_value_(other.has_value_) {
        if (!has_value_) {
            new (&storage_.error) E(other.storage_.error);
        }
    }

    /** @brief Move constructor.
     *  @param other The Expected to move from. */
    Expected(Expected&& other) noexcept : has_value_(other.has_value_) {
        if (!has_value_) {
            new (&storage_.error) E(move(other.storage_.error));
        }
    }

    /** @brief Destructor. Destroys the error if present. */
    ~Expected() {
        if (!has_value_) {
            storage_.error.~E();
        }
    }

    /** @brief Copy assignment.
     *  @param other The Expected to copy from.
     *  @return Reference to this Expected. */
    Expected& operator=(const Expected& other) {
        if (this != &other) {
            if (!has_value_) storage_.error.~E();
            has_value_ = other.has_value_;
            if (!has_value_) {
                new (&storage_.error) E(other.storage_.error);
            }
        }
        return *this;
    }

    /** @brief Move assignment.
     *  @param other The Expected to move from.
     *  @return Reference to this Expected. */
    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            if (!has_value_) storage_.error.~E();
            has_value_ = other.has_value_;
            if (!has_value_) {
                new (&storage_.error) E(move(other.storage_.error));
            }
        }
        return *this;
    }

    /** @brief Returns true if the Expected holds a successful (void) state. */
    bool has_value() const noexcept { return has_value_; }
    /** @brief Converts to bool, returning true if the state is successful. */
    explicit operator bool() const noexcept { return has_value_; }

    /** @brief Returns a reference to the contained error (lvalue). */
    E& error() & { return storage_.error; }
    /** @brief Returns a const reference to the contained error. */
    const E& error() const& { return storage_.error; }
    /** @brief Returns an rvalue reference to the contained error. */
    E&& error() && { return move(storage_.error); }

    /** @brief Swaps the contents with another Expected.
     *  @param other The Expected to swap with. */
    void swap(Expected& other) noexcept {
        if (!has_value_ && !other.has_value_) {
            using prism::swap;
            swap(storage_.error, other.storage_.error);
        }
    }

private:
    /** @brief Internal storage union for the error. */
    union ErrorStorage {
        char dummy;  ///< Placeholder for the success state.
        E error;     ///< Storage for the error value.
        /** @brief Constructs the dummy byte. */
        ErrorStorage() : dummy(0) {}
        /** @brief Destructor is intentionally trivial. */
        ~ErrorStorage() {}
    };

    ErrorStorage storage_;  ///< Internal union storage.
    bool has_value_;        ///< Whether the Expected holds a successful state.
};

}  // namespace prism
