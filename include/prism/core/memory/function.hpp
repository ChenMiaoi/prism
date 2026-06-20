#pragma once

#include "prism/core/utility/move.hpp"

#include <cstddef>
#include <new>
#include <utility>

namespace prism {

template <typename Signature>
class Function;

/** @brief A type-erased, move-only callable wrapper (similar to std::function).
 *  @tparam R The return type.
 *  @tparam Args The parameter types. */
template <typename R, typename... Args>
class Function<R(Args...)> {
    /** @brief Abstract base class for the type-erased callable. */
    struct Concept {
        virtual ~Concept() = default;
        /** @brief Invokes the stored callable.
         *  @param args The arguments to pass.
         *  @return The result of the invocation. */
        virtual R invoke(Args... args) = 0;
        /** @brief Creates a deep copy of this concept.
         *  @return Pointer to the cloned concept. */
        virtual Concept* clone() = 0;
    };

    /** @brief Concrete model that wraps a specific callable type F. */
    template <typename F>
    struct Model : Concept {
        F func;  ///< The stored callable.

        /** @brief Constructs the model with a callable.
         *  @param f The callable to store. */
        template <typename U>
        explicit Model(U&& f) : func(forward<U>(f)) {}

        /** @brief Invokes the stored callable with the given arguments.
         *  @param args The arguments to pass.
         *  @return The result of the invocation. */
        R invoke(Args... args) override { return func(forward<Args>(args)...); }

        /** @brief Creates a deep copy of this model.
         *  @return Pointer to the cloned model. */
        Concept* clone() override { return new Model(func); }
    };

public:
    /** @brief Default-constructs an empty Function (no callable). */
    Function() noexcept : heap_(nullptr), empty_(true) {}

    /** @brief Constructs an empty Function from nullptr.
     *  @param n Null pointer. */
    Function(decltype(nullptr)) noexcept : heap_(nullptr), empty_(true) {}

    /** @brief Constructs a Function wrapping the given callable.
     *  @tparam F The callable type.
     *  @param f The callable to wrap. */
    template <typename F, typename = enable_if_t<!is_same_v<decay_t<F>, Function>>>
    Function(F&& f) : empty_(false) {
        heap_ = new Model<decay_t<F>>(forward<F>(f));
    }

    /** @brief Copy constructor. Deep-copies the stored callable.
     *  @param other The Function to copy from. */
    Function(const Function& other) : empty_(other.empty_) {
        if (!empty_) {
            heap_ = other.heap_->clone();
        }
    }

    /** @brief Move constructor. Transfers ownership of the internal callable.
     *  @param other The Function to move from. */
    Function(Function&& other) noexcept : heap_(other.heap_), empty_(other.empty_) {
        other.heap_ = nullptr;
        other.empty_ = true;
    }

    /** @brief Destructor. Deletes the stored callable. */
    ~Function() {
        if (!empty_) {
            delete heap_;
        }
    }

    /** @brief Copy assignment.
     *  @param other The Function to copy from.
     *  @return Reference to this Function. */
    Function& operator=(const Function& other) {
        if (this != &other) {
            this->~Function();
            new (this) Function(other);
        }
        return *this;
    }

    /** @brief Move assignment.
     *  @param other The Function to move from.
     *  @return Reference to this Function. */
    Function& operator=(Function&& other) noexcept {
        if (this != &other) {
            this->~Function();
            new (this) Function(move(other));
        }
        return *this;
    }

    /** @brief Resets this Function to empty.
     *  @param n Null pointer. */
    Function& operator=(decltype(nullptr)) noexcept {
        this->~Function();
        new (this) Function();
        return *this;
    }

    /** @brief Invokes the stored callable.
     *  @param args The arguments to forward.
     *  @return The result of the invocation. */
    R operator()(Args... args) { return heap_->invoke(forward<Args>(args)...); }

    /** @brief Converts to bool. Returns true if a callable is stored. */
    explicit operator bool() const noexcept { return !empty_; }

private:
    Concept* heap_;  ///< Pointer to the type-erased callable on the heap.
    bool empty_;     ///< Whether this Function is empty.
};

}  // namespace prism
