#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace prism {

/** @brief Default hash functor supporting integer types, C strings, and a generic byte-level fallback. */
struct DefaultHash {
    /** @brief Hashes an int key.
     *  @param key The int to hash.
     *  @return The hash value. */
    size_t operator()(int key) const noexcept { return static_cast<size_t>(key); }

    /** @brief Hashes a long key.
     *  @param key The long to hash.
     *  @return The hash value. */
    size_t operator()(long key) const noexcept { return static_cast<size_t>(key); }

    /** @brief Hashes a long long key.
     *  @param key The long long to hash.
     *  @return The hash value. */
    size_t operator()(long long key) const noexcept { return static_cast<size_t>(key); }

    /** @brief Hashes an unsigned int key.
     *  @param key The unsigned int to hash.
     *  @return The hash value. */
    size_t operator()(unsigned int key) const noexcept { return static_cast<size_t>(key); }

    /** @brief Hashes an unsigned long key.
     *  @param key The unsigned long to hash.
     *  @return The hash value. */
    size_t operator()(unsigned long key) const noexcept { return static_cast<size_t>(key); }

    /** @brief Hashes an unsigned long long key.
     *  @param key The unsigned long long to hash.
     *  @return The hash value. */
    size_t operator()(unsigned long long key) const noexcept { return static_cast<size_t>(key); }

    /** @brief Hashes a null-terminated C string using a polynomial hash (base 31).
     *  @param key The C string to hash.
     *  @return The hash value. */
    size_t operator()(const char* key) const noexcept {
        size_t hash = 0;
        while (*key) {
            hash = hash * 31 + static_cast<size_t>(*key);
            ++key;
        }
        return hash;
    }

    /** @brief Generic hash functor that hashes the raw bytes of any trivially copyable type.
     *  @tparam T The type of the key.
     *  @param key The value to hash.
     *  @return The hash value. */
    template <typename T>
    size_t operator()(const T& key) const noexcept {
        size_t hash = 0;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&key);
        for (size_t i = 0; i < sizeof(T); ++i) {
            hash = hash * 31 + bytes[i];
        }
        return hash;
    }
};

}  // namespace prism
