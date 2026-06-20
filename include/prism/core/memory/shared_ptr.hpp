#pragma once

#include "prism/core/utility/move.hpp"

#include <cstddef>

namespace prism {

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

namespace detail {

/** @brief Base class for reference-counting control blocks. Manages strong and weak reference counts. */
class SharedCount {
public:
    /** @brief Constructs a control block with a strong count of 1 and weak count of 0. */
    SharedCount() : strong_count_(1), weak_count_(0) {}

    /** @brief Virtual destructor. */
    virtual ~SharedCount() = default;

    /** @brief Increments the strong reference count. */
    void add_ref() { ++strong_count_; }
    /** @brief Decrements the strong reference count and disposes the managed object when it reaches zero. */
    void release() {
        --strong_count_;
        if (strong_count_ == 0) {
            dispose();
            if (weak_count_ == 0) {
                delete this;
            }
        }
    }

    /** @brief Increments the weak reference count. */
    void add_weak_ref() { ++weak_count_; }
    /** @brief Decrements the weak reference count and deletes this control block if both counts reach zero. */
    void release_weak() {
        --weak_count_;
        if (weak_count_ == 0) {
            if (strong_count_ == 0) {
                delete this;
            }
        }
    }

    /** @brief Pure virtual function to dispose of the managed object. */
    virtual void dispose() = 0;

    /** @brief Returns the current strong reference count. */
    long use_count() const noexcept { return strong_count_; }

private:
    long strong_count_;  ///< Number of shared owners.
    long weak_count_;    ///< Number of weak observers.
};

/** @brief Control block that manages a heap-allocated object via delete.
 *  @tparam T The managed object type. */
template <typename T>
class SharedCountImpl : public SharedCount {
public:
    /** @brief Constructs a control block managing @p ptr.
     *  @param ptr The raw pointer to manage. */
    explicit SharedCountImpl(T* ptr) : ptr_(ptr) {}
    /** @brief Deletes the managed object. */
    void dispose() override { delete ptr_; }

private:
    T* ptr_;  ///< The managed raw pointer.
};

/** @brief Control block that stores the object inline using aligned storage.
 *  @tparam T The managed object type.
 *  @tparam Args Constructor argument types. */
template <typename T, typename... Args>
class SharedCountAllocImpl : public SharedCount {
public:
    /** @brief Constructs the managed object in-place with the given arguments.
     *  @param args Arguments forwarded to the T constructor. */
    SharedCountAllocImpl(Args&&... args) { new (&storage_) T(forward<Args>(args)...); }

    /** @brief Destroys the managed object. */
    void dispose() override { get()->~T(); }

    /** @brief Returns a pointer to the in-place constructed object. */
    T* get() { return reinterpret_cast<T*>(&storage_); }

private:
    alignas(T) unsigned char storage_[sizeof(T)];  ///< Aligned storage for the in-place object.
};

}  // namespace detail

/** @brief A shared-ownership smart pointer that manages a dynamically allocated object via reference counting. */
template <typename T>
class SharedPtr {
public:
    /** @brief Default-constructs a null SharedPtr. */
    SharedPtr() noexcept : ptr_(nullptr), count_(nullptr) {}

    /** @brief Constructs a SharedPtr that owns @p ptr.
     *  @param ptr The raw pointer to manage. */
    explicit SharedPtr(T* ptr) : ptr_(ptr), count_(new detail::SharedCountImpl<T>(ptr)) {}

    /** @brief Copy constructor. Increments the reference count.
     *  @param other The SharedPtr to copy from. */
    SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
        if (count_) count_->add_ref();
    }

    /** @brief Move constructor. Transfers ownership without changing the reference count.
     *  @param other The SharedPtr to move from. */
    SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
        other.ptr_ = nullptr;
        other.count_ = nullptr;
    }

    /** @brief Converting copy constructor from SharedPtr<U>.
     *  @param other The SharedPtr to copy from. */
    template <typename U>
    SharedPtr(const SharedPtr<U>& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
        if (count_) count_->add_ref();
    }

    /** @brief Destructor. Releases the shared reference. */
    ~SharedPtr() {
        if (count_) count_->release();
    }

    /** @brief Copy assignment. Releases the current object and shares ownership with @p other.
     *  @param other The SharedPtr to copy from.
     *  @return Reference to this SharedPtr. */
    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this != &other) {
            if (count_) count_->release();
            ptr_ = other.ptr_;
            count_ = other.count_;
            if (count_) count_->add_ref();
        }
        return *this;
    }

    /** @brief Move assignment. Transfers ownership from @p other.
     *  @param other The SharedPtr to move from.
     *  @return Reference to this SharedPtr. */
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            if (count_) count_->release();
            ptr_ = other.ptr_;
            count_ = other.count_;
            other.ptr_ = nullptr;
            other.count_ = nullptr;
        }
        return *this;
    }

    /** @brief Dereference operator. Returns a reference to the managed object. */
    T& operator*() const { return *ptr_; }
    /** @brief Arrow operator. Returns a pointer to the managed object. */
    T* operator->() const { return ptr_; }
    /** @brief Returns the raw pointer without releasing ownership. */
    T* get() const noexcept { return ptr_; }

    /** @brief Converts to bool. Returns true if the pointer is non-null. */
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /** @brief Returns the number of shared owners. */
    long use_count() const noexcept { return count_ ? count_->use_count() : 0; }

    /** @brief Resets the managed pointer, releasing the current object.
     *  @param ptr The new pointer to manage (default: nullptr). */
    void reset(T* ptr = nullptr) noexcept {
        if (count_) count_->release();
        ptr_ = ptr;
        count_ = ptr ? new detail::SharedCountImpl<T>(ptr) : nullptr;
    }

    /** @brief Swaps the managed pointers with another SharedPtr.
     *  @param other The SharedPtr to swap with. */
    void swap(SharedPtr& other) noexcept {
        using prism::swap;
        swap(ptr_, other.ptr_);
        swap(count_, other.count_);
    }

private:
    template <typename U>
    friend class SharedPtr;
    template <typename U, typename... Args>
    friend SharedPtr<U> make_shared(Args&&...);

    T* ptr_;                      ///< The managed raw pointer.
    detail::SharedCount* count_;  ///< The reference-counting control block.
};

/** @brief Creates a SharedPtr with an in-place constructed object.
 *  @tparam T The managed object type.
 *  @param args Arguments forwarded to the T constructor.
 *  @return A SharedPtr<T> managing the new object. */
template <typename T, typename... Args>
SharedPtr<T> make_shared(Args&&... args) {
    auto* impl = new detail::SharedCountAllocImpl<T, Args...>(forward<Args>(args)...);
    SharedPtr<T> result;
    result.ptr_ = impl->get();
    result.count_ = impl;
    return result;
}

}  // namespace prism
