#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

namespace prism {

/**
 * @brief A vector with a fixed-size inline buffer for small element counts.
 *
 * Stores up to N elements inline without heap allocation. When the element
 * count exceeds N, automatically transitions to heap allocation.
 *
 * @tparam T The element type.
 * @tparam N The inline buffer capacity.
 */
template <typename T, unsigned N>
class SmallVector {
    static constexpr unsigned InlineCapacity = N;

    using StorageType = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::array<StorageType, InlineCapacity> inline_buffer_;  ///< Aligned storage for inline elements.
    T* data_ = nullptr;                                      ///< Pointer to the active element storage.
    unsigned size_ = 0;                                      ///< Number of elements in the vector.
    unsigned capacity_ = 0;                                  ///< Current storage capacity.

    /** @brief Get a pointer to the inline buffer (mutable). */
    auto inline_buffer_ptr() -> T* { return reinterpret_cast<T*>(inline_buffer_.data()); }

    /** @brief Get a pointer to the inline buffer (const). */
    auto inline_buffer_ptr() const -> const T* { return reinterpret_cast<const T*>(inline_buffer_.data()); }

public:
    /** @brief Mutable iterator type. */
    using iterator = T*;

    /** @brief Constant iterator type. */
    using const_iterator = const T*;

    /** @brief Default-construct an empty SmallVector pointing to the inline buffer. */
    SmallVector() : data_(inline_buffer_ptr()), size_(0), capacity_(InlineCapacity) {}

    /** @brief Construct with count copies of the given value. */
    explicit SmallVector(unsigned count, const T& value = T()) : SmallVector() {
        reserve(count);
        for (unsigned i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    /** @brief Construct from an initializer list. */
    SmallVector(std::initializer_list<T> init) : SmallVector() {
        reserve(init.size());
        for (auto&& item : init) {
            push_back(std::move(item));
        }
    }

    /** @brief Copy constructor. */
    SmallVector(const SmallVector& other) : SmallVector() {
        reserve(other.size());
        for (const auto& item : other) {
            push_back(item);
        }
    }

    /** @brief Move constructor. Moves elements from inline buffer or steals the heap pointer. */
    SmallVector(SmallVector&& other) noexcept : SmallVector() {
        if (other.data_ == other.inline_buffer_ptr()) {
            for (unsigned i = 0; i < other.size_; ++i) {
                push_back(std::move(other[i]));
            }
        } else {
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = other.inline_buffer_ptr();
            other.size_ = 0;
            other.capacity_ = InlineCapacity;
        }
    }

    /** @brief Destructor. Destroys all elements and frees heap memory if used. */
    ~SmallVector() {
        clear();
        if (data_ != inline_buffer_ptr()) {
            ::operator delete(data_);
        }
    }

    /** @brief Copy assignment. */
    auto operator=(const SmallVector& other) -> SmallVector& {
        if (this != &other) {
            clear();
            reserve(other.size());
            for (const auto& item : other) {
                push_back(item);
            }
        }
        return *this;
    }

    /** @brief Move assignment. */
    auto operator=(SmallVector&& other) noexcept -> SmallVector& {
        if (this != &other) {
            clear();
            if (data_ != inline_buffer_ptr()) {
                ::operator delete(data_);
            }
            if (other.data_ == other.inline_buffer_ptr()) {
                data_ = inline_buffer_ptr();
                capacity_ = InlineCapacity;
                for (unsigned i = 0; i < other.size_; ++i) {
                    push_back(std::move(other[i]));
                }
            } else {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = other.inline_buffer_ptr();
                other.size_ = 0;
                other.capacity_ = InlineCapacity;
            }
        }
        return *this;
    }

    /** @brief Access element at the given index (mutable). */
    auto operator[](unsigned index) -> T& {
        assert(index < size_ && "Index out of bounds");
        return data_[index];
    }

    /** @brief Access element at the given index (const). */
    auto operator[](unsigned index) const -> const T& {
        assert(index < size_ && "Index out of bounds");
        return data_[index];
    }

    /** @brief Get a reference to the first element. */
    auto front() -> T& { return data_[0]; }

    /** @brief Get a const reference to the first element. */
    auto front() const -> const T& { return data_[0]; }

    /** @brief Get a reference to the last element. */
    auto back() -> T& { return data_[size_ - 1]; }

    /** @brief Get a const reference to the last element. */
    auto back() const -> const T& { return data_[size_ - 1]; }

    /** @brief Get a pointer to the underlying data. */
    auto data() -> T* { return data_; }

    /** @brief Get a const pointer to the underlying data. */
    auto data() const -> const T* { return data_; }

    /** @brief Get the number of elements. */
    auto size() const -> unsigned { return size_; }

    /** @brief Get the current storage capacity. */
    auto capacity() const -> unsigned { return capacity_; }

    /** @brief Check whether the vector is empty. */
    auto empty() const -> bool { return size_ == 0; }

    /** @brief Iterator to the beginning. */
    auto begin() -> iterator { return data_; }

    /** @brief Iterator past the end. */
    auto end() -> iterator { return data_ + size_; }

    /** @brief Const iterator to the beginning. */
    auto begin() const -> const_iterator { return data_; }

    /** @brief Const iterator past the end. */
    auto end() const -> const_iterator { return data_ + size_; }

    /** @brief Append a copy of the given value. */
    auto push_back(const T& value) -> void {
        if (size_ == capacity_) {
            grow(capacity_ * 2);
        }
        new (data_ + size_) T(value);
        ++size_;
    }

    /** @brief Append by moving the given value. */
    auto push_back(T&& value) -> void {
        if (size_ == capacity_) {
            grow(capacity_ * 2);
        }
        new (data_ + size_) T(std::move(value));
        ++size_;
    }

    /**
     * @brief Construct an element in-place at the end of the vector.
     *
     * @param args Arguments forwarded to the element constructor.
     * @return Reference to the newly constructed element.
     */
    template <typename... Args>
    auto emplace_back(Args&&... args) -> T& {
        if (size_ == capacity_) {
            grow(capacity_ * 2);
        }
        new (data_ + size_) T(std::forward<Args>(args)...);
        return data_[size_++];
    }

    /** @brief Remove the last element. */
    auto pop_back() -> void {
        assert(size_ > 0 && "Pop from empty vector");
        --size_;
        data_[size_].~T();
    }

    /** @brief Destroy all elements and set size to zero. */
    auto clear() -> void {
        for (unsigned i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }

    /** @brief Ensure the vector has capacity for at least the given number of elements. */
    auto reserve(unsigned new_capacity) -> void {
        if (new_capacity > capacity_) {
            grow(new_capacity);
        }
    }

    /** @brief Return a span over the elements (mutable). */
    auto as_span() -> std::span<T> { return {data_, size_}; }

    /** @brief Return a span over the elements (const). */
    auto as_span() const -> std::span<const T> { return {data_, size_}; }

private:
    /** @brief Grow the storage to the given capacity, moving elements to the new buffer. */
    auto grow(unsigned new_capacity) -> void {
        T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_capacity));
        for (unsigned i = 0; i < size_; ++i) {
            new (new_data + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        if (data_ != inline_buffer_ptr()) {
            ::operator delete(data_);
        }
        data_ = new_data;
        capacity_ = new_capacity;
    }
};

}  // namespace prism
