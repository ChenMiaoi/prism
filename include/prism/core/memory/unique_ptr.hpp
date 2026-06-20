#pragma once

#include "prism/core/utility/move.hpp"

#include <cstddef>
#include <utility>

namespace prism {

/** @brief Default deleter that calls delete on the managed pointer. */
template <typename T>
struct DefaultDelete {
    void operator()(T* ptr) const { delete ptr; }
};

/** @brief A smart pointer that owns and manages a dynamically allocated object, ensuring single ownership. */
template <typename T, typename Deleter = DefaultDelete<T>>
class UniquePtr {
public:
    /** @brief Default-constructs a null UniquePtr. */
    UniquePtr() noexcept : ptr_(nullptr) {}
    /** @brief Constructs a UniquePtr that owns @p ptr.
     *  @param ptr The raw pointer to manage. */
    explicit UniquePtr(T* ptr) noexcept : ptr_(ptr) {}

    /** @brief Move constructor. Transfers ownership from @p other.
     *  @param other The UniquePtr to move from. */
    UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    /** @brief Destructor. Deletes the managed object if non-null. */
    ~UniquePtr() {
        if (ptr_) Deleter{}(ptr_);
    }

    /** @brief Move assignment. Releases the current object and takes ownership from @p other.
     *  @param other The UniquePtr to move from.
     *  @return Reference to this UniquePtr. */
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            if (ptr_) Deleter{}(ptr_);
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    /** @brief Copy construction is deleted to enforce single ownership. */
    UniquePtr(const UniquePtr&) = delete;
    /** @brief Copy assignment is deleted to enforce single ownership. */
    UniquePtr& operator=(const UniquePtr&) = delete;

    /** @brief Dereference operator. Returns a reference to the managed object. */
    T& operator*() const { return *ptr_; }
    /** @brief Arrow operator. Returns a pointer to the managed object. */
    T* operator->() const { return ptr_; }
    /** @brief Returns the raw pointer without releasing ownership. */
    T* get() const noexcept { return ptr_; }

    /** @brief Converts to bool. Returns true if the pointer is non-null. */
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /** @brief Releases ownership and returns the raw pointer.
     *  @return The raw pointer that was managed. */
    T* release() noexcept {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }

    /** @brief Resets the managed pointer, deleting the old one.
     *  @param ptr The new pointer to manage (default: nullptr). */
    void reset(T* ptr = nullptr) noexcept {
        if (ptr_) Deleter{}(ptr_);
        ptr_ = ptr;
    }

    /** @brief Swaps the managed pointers with another UniquePtr.
     *  @param other The UniquePtr to swap with. */
    void swap(UniquePtr& other) noexcept {
        using prism::swap;
        swap(ptr_, other.ptr_);
    }

private:
    T* ptr_;  ///< The managed raw pointer.
};

/** @brief Creates a UniquePtr managing a new object constructed with the given arguments.
 *  @param args Arguments forwarded to the T constructor.
 *  @return A UniquePtr<T> owning the new object. */
template <typename T, typename... Args>
UniquePtr<T> make_unique(Args&&... args) {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

}  // namespace prism
