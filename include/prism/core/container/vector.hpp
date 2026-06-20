#pragma once

#include "prism/core/utility/move.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <new>

namespace prism {

/** @brief Alias for std::initializer_list. */
template <typename T>
using InitializerList = std ::initializer_list<T>;

/** @brief A dynamically-resizing contiguous container similar to std::vector. */
template <typename T>
class Vector {
public:
    /** @brief The element type stored in the vector. */
    using value_type = T;
    /** @brief Mutable iterator type (pointer to T). */
    using iterator = T*;
    /** @brief Const iterator type (pointer to const T). */
    using const_iterator = const T*;

    /** @brief Default-constructs an empty vector. */
    Vector() noexcept : data_(nullptr), size_(0), capacity_(0) {}

    /** @brief Constructs a vector with @p count default-initialized elements.
     *  @param count Number of elements to create. */
    explicit Vector(size_t count) : size_(count), capacity_(count) {
        data_ = static_cast<T*>(::operator new(sizeof(T) * count));
        for (size_t i = 0; i < count; ++i) {
            new (data_ + i) T();
        }
    }

    /** @brief Constructs a vector with @p count copies of @p value.
     *  @param count Number of elements.
     *  @param value The value each element is copy-constructed from. */
    Vector(size_t count, const T& value) : size_(count), capacity_(count) {
        data_ = static_cast<T*>(::operator new(sizeof(T) * count));
        for (size_t i = 0; i < count; ++i) {
            new (data_ + i) T(value);
        }
    }

    /** @brief Constructs a vector from an array of @p count items.
     *  @param items Pointer to the source array.
     *  @param count Number of elements to copy. */
    Vector(const T* items, size_t count) : size_(count), capacity_(count) {
        if (capacity_ > 0) {
            data_ = static_cast<T*>(::operator new(sizeof(T) * count));
            for (size_t i = 0; i < count; ++i) {
                new (data_ + i) T(items[i]);
            }
        }
    }

    /** @brief Constructs a vector from an initializer list.
     *  @param items The initializer list of elements. */
    Vector(InitializerList<T> items) : size_(items.size()), capacity_(items.size()) {
        if (capacity_ > 0) {
            data_ = static_cast<T*>(::operator new(sizeof(T) * capacity_));
            size_t i = 0;
            for (const auto& item : items) {
                new (data_ + i) T(item);
                ++i;
            }
        }
    }

    /** @brief Copy constructor. Deep-copies all elements.
     *  @param other The vector to copy from. */
    Vector(const Vector& other) : size_(other.size_), capacity_(other.capacity_) {
        if (capacity_ > 0) {
            data_ = static_cast<T*>(::operator new(sizeof(T) * capacity_));
            for (size_t i = 0; i < size_; ++i) {
                new (data_ + i) T(other.data_[i]);
            }
        }
    }

    /** @brief Move constructor. Transfers ownership of the buffer.
     *  @param other The vector to move from. */
    Vector(Vector&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    /** @brief Destructor. Destroys all elements and frees the buffer. */
    ~Vector() {
        clear();
        if (data_) ::operator delete(data_);
    }

    /** @brief Copy assignment. Replaces contents with copies of @p other.
     *  @param other The vector to copy from.
     *  @return Reference to this vector. */
    Vector& operator=(const Vector& other) {
        if (this != &other) {
            clear();
            if (capacity_ < other.size_) {
                if (data_) ::operator delete(data_);
                capacity_ = other.size_ * 2;
                if (capacity_ < 4) capacity_ = 4;
                data_ = static_cast<T*>(::operator new(sizeof(T) * capacity_));
            }
            size_ = other.size_;
            for (size_t i = 0; i < size_; ++i) {
                new (data_ + i) T(other.data_[i]);
            }
        }
        return *this;
    }

    /** @brief Move assignment. Transfers ownership of the buffer.
     *  @param other The vector to move from.
     *  @return Reference to this vector. */
    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clear();
            if (data_) ::operator delete(data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    /** @brief Unchecked element access by index.
     *  @param i Index of the element.
     *  @return Reference to the element at index @p i. */
    T& operator[](size_t i) { return data_[i]; }
    /** @brief Const unchecked element access by index.
     *  @param i Index of the element.
     *  @return Const reference to the element at index @p i. */
    const T& operator[](size_t i) const { return data_[i]; }

    /** @brief Bounds-checked element access.
     *  @param i Index of the element.
     *  @return Reference to the element at index @p i. */
    T& at(size_t i) {
        assert(i < size_ && "Vector::at index out of bounds");
        return data_[i];
    }

    /** @brief Const bounds-checked element access.
     *  @param i Index of the element.
     *  @return Const reference to the element at index @p i. */
    const T& at(size_t i) const {
        assert(i < size_ && "Vector::at index out of bounds");
        return data_[i];
    }

    /** @brief Returns a reference to the first element. */
    T& front() { return data_[0]; }
    /** @brief Returns a const reference to the first element. */
    const T& front() const { return data_[0]; }
    /** @brief Returns a reference to the last element. */
    T& back() { return data_[size_ - 1]; }
    /** @brief Returns a const reference to the last element. */
    const T& back() const { return data_[size_ - 1]; }

    /** @brief Returns a pointer to the underlying data buffer. */
    T* data() noexcept { return data_; }
    /** @brief Returns a const pointer to the underlying data buffer. */
    const T* data() const noexcept { return data_; }

    /** @brief Returns the number of elements in the vector. */
    size_t size() const noexcept { return size_; }
    /** @brief Returns the total capacity of the vector. */
    size_t capacity() const noexcept { return capacity_; }
    /** @brief Returns true if the vector is empty. */
    bool empty() const noexcept { return size_ == 0; }

    /** @brief Destroys all elements and sets size to 0. */
    void clear() noexcept {
        for (size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }

    /** @brief Appends a copy of @p value to the end.
     *  @param value The element to append. */
    void push_back(const T& value) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(value);
        ++size_;
    }

    /** @brief Appends a moved @p value to the end.
     *  @param value The element to move-append. */
    void push_back(T&& value) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(move(value));
        ++size_;
    }

    /** @brief Constructs an element in-place at the end.
     *  @param args Arguments forwarded to the element constructor.
     *  @return Reference to the newly constructed element. */
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) grow();
        new (data_ + size_) T(forward<Args>(args)...);
        return data_[size_++];
    }

    /** @brief Inserts a moved element at the given position.
     *  @param pos Iterator to the insertion position.
     *  @param value The element to insert.
     *  @return Iterator to the inserted element. */
    iterator insert(iterator pos, T&& value) {
        size_t index = static_cast<size_t>(pos - data_);
        if (size_ == capacity_) grow();
        for (size_t i = size_; i > index; --i) {
            new (data_ + i) T(move(data_[i - 1]));
            data_[i - 1].~T();
        }
        new (data_ + index) T(move(value));
        ++size_;
        return data_ + index;
    }

    /** @brief Removes the element at the given index.
     *  @param index Index of the element to remove. */
    void erase_at(size_t index) {
        if (index >= size_) return;
        data_[index].~T();
        for (size_t i = index; i + 1 < size_; ++i) {
            new (data_ + i) T(move(data_[i + 1]));
            data_[i + 1].~T();
        }
        --size_;
    }

    /** @brief Removes the last element. */
    void pop_back() {
        if (size_ > 0) {
            --size_;
            data_[size_].~T();
        }
    }

    /** @brief Reserves capacity for at least @p new_capacity elements.
     *  @param new_capacity The minimum capacity to reserve. */
    void reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_capacity));
            for (size_t i = 0; i < size_; ++i) {
                new (new_data + i) T(move(data_[i]));
                data_[i].~T();
            }
            if (data_) ::operator delete(data_);
            data_ = new_data;
            capacity_ = new_capacity;
        }
    }

    /** @brief Resizes the vector to @p new_size elements.
     *  @param new_size The desired new size. */
    void resize(size_t new_size) {
        if (new_size > size_) {
            if (new_size > capacity_) reserve(new_size);
            for (size_t i = size_; i < new_size; ++i) {
                new (data_ + i) T();
            }
        } else {
            for (size_t i = new_size; i < size_; ++i) {
                data_[i].~T();
            }
        }
        size_ = new_size;
    }

    /** @brief Returns an iterator to the beginning. */
    iterator begin() noexcept { return data_; }
    /** @brief Returns a const iterator to the beginning. */
    const_iterator begin() const noexcept { return data_; }
    /** @brief Returns an iterator past the end. */
    iterator end() noexcept { return data_ + size_; }
    /** @brief Returns a const iterator past the end. */
    const_iterator end() const noexcept { return data_ + size_; }

private:
    /** @brief Doubles the capacity (or sets to 4 if zero) and reallocates. */
    void grow() {
        size_t new_capacity = capacity_ == 0 ? 4 : capacity_ * 2;
        reserve(new_capacity);
    }

    T* data_;          ///< Pointer to the dynamically allocated element buffer.
    size_t size_;      ///< Number of elements currently stored.
    size_t capacity_;  ///< Total number of elements that can be stored without reallocation.
};

}  // namespace prism
