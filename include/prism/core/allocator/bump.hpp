#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace prism {

/** @brief A bump allocator that allocates memory by advancing a pointer within a contiguous buffer. */
class BumpAllocator {
public:
    /** @brief Constructs a BumpAllocator with the given initial capacity.
     *  @param initial_capacity The initial buffer size in bytes (default: 4096). */
    explicit BumpAllocator(size_t initial_capacity = 4096)
        : buffer_(static_cast<char*>(::operator new(initial_capacity))), offset_(0), capacity_(initial_capacity) {}

    /** @brief Destructor. Frees the internal buffer. */
    ~BumpAllocator() { ::operator delete(buffer_); }

    /** @brief Copy construction is deleted. */
    BumpAllocator(const BumpAllocator&) = delete;
    /** @brief Copy assignment is deleted. */
    BumpAllocator& operator=(const BumpAllocator&) = delete;

    /** @brief Move constructor. Transfers ownership of the buffer.
     *  @param other The BumpAllocator to move from. */
    BumpAllocator(BumpAllocator&& other) noexcept
        : buffer_(other.buffer_), offset_(other.offset_), capacity_(other.capacity_) {
        other.buffer_ = nullptr;
        other.offset_ = 0;
        other.capacity_ = 0;
    }

    /** @brief Move assignment. Transfers ownership of the buffer.
     *  @param other The BumpAllocator to move from.
     *  @return Reference to this BumpAllocator. */
    BumpAllocator& operator=(BumpAllocator&& other) noexcept {
        if (this != &other) {
            ::operator delete(buffer_);
            buffer_ = other.buffer_;
            offset_ = other.offset_;
            capacity_ = other.capacity_;
            other.buffer_ = nullptr;
            other.offset_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    /** @brief Allocates a block of memory with the given size and alignment.
     *  @param size The number of bytes to allocate.
     *  @param alignment The required alignment (default: alignof(void*)).
     *  @return Pointer to the allocated memory. */
    void* allocate(size_t size, size_t alignment = alignof(void*)) {
        size_t aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);
        if (aligned_offset + size > capacity_) {
            grow(aligned_offset + size);
            aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);
        }
        void* ptr = buffer_ + aligned_offset;
        offset_ = aligned_offset + size;
        return ptr;
    }

    /** @brief Constructs an object of type @p T in the allocated memory.
     *  @tparam T The object type to construct.
     *  @tparam Args Constructor argument types.
     *  @param args Arguments forwarded to the T constructor.
     *  @return Pointer to the constructed object. */
    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(forward<Args>(args)...);
    }

    /** @brief Resets the allocator, making all previously allocated memory available again. */
    void reset() { offset_ = 0; }

    /** @brief Returns the number of bytes currently allocated. */
    size_t bytes_used() const { return offset_; }
    /** @brief Returns the total buffer capacity in bytes. */
    size_t capacity() const { return capacity_; }

private:
    /** @brief Grows the buffer to accommodate the required size.
     *  @param required The minimum required capacity in bytes. */
    void grow(size_t required) {
        size_t new_capacity = capacity_ * 2;
        while (new_capacity < required) {
            new_capacity *= 2;
        }
        char* new_buffer = static_cast<char*>(::operator new(new_capacity));
        if (buffer_) {
            ::memcpy(new_buffer, buffer_, offset_);
            ::operator delete(buffer_);
        }
        buffer_ = new_buffer;
        capacity_ = new_capacity;
    }

    char* buffer_;     ///< Pointer to the allocated buffer.
    size_t offset_;    ///< Current byte offset (next allocation position).
    size_t capacity_;  ///< Total buffer capacity in bytes.
};

}  // namespace prism
