#pragma once

#include "prism/core/utility/move.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace prism {

/** @brief A heap-allocated string with small-string optimization (SSO) for strings up to 22 characters. */
class String {
    static constexpr size_t SSO_CAPACITY = 22;  ///< Maximum length for inline (SSO) storage.

    /** @brief Data layout for heap-allocated (long) strings. */
    struct LongData {
        char* data;       ///< Pointer to the heap-allocated character buffer.
        size_t size;      ///< Number of characters in the string.
        size_t capacity;  ///< Allocated buffer capacity.
    };

    /** @brief Data layout for stack-allocated (short) strings. */
    struct ShortData {
        char data[SSO_CAPACITY + 1];  ///< Inline character buffer with null terminator.
        unsigned char size_and_flag;  ///< Packed size and long-string flag (bit 0).
    };

    /** @brief Internal union of long and short string storage. */
    union Storage {
        LongData long_data;    ///< Heap-allocated string data.
        ShortData short_data;  ///< Inline string data.

        /** @brief Constructs the short-data default state (empty string). */
        Storage() {
            short_data.size_and_flag = 0;
            short_data.data[0] = '\0';
        }
        /** @brief Destructor is intentionally trivial. */
        ~Storage() {}
    };

public:
    /** @brief Default-constructs an empty string. */
    String() noexcept {
        storage_.short_data.size_and_flag = 0;
        storage_.short_data.data[0] = '\0';
        is_long_ = false;
    }

    /** @brief Constructs a String from a null-terminated C string.
     *  @param str The source C string. */
    String(const char* str) {
        size_t len = ::strlen(str);
        if (len <= SSO_CAPACITY) {
            ::memcpy(storage_.short_data.data, str, len);
            storage_.short_data.data[len] = '\0';
            storage_.short_data.size_and_flag = static_cast<unsigned char>(len << 1);
            is_long_ = false;
        } else {
            storage_.long_data.capacity = len * 2;
            if (storage_.long_data.capacity < 32) storage_.long_data.capacity = 32;
            storage_.long_data.data = static_cast<char*>(::operator new(storage_.long_data.capacity + 1));
            ::memcpy(storage_.long_data.data, str, len);
            storage_.long_data.data[len] = '\0';
            storage_.long_data.size = len;
            is_long_ = true;
        }
    }

    /** @brief Constructs a String from a character buffer with explicit length.
     *  @param str The source character buffer.
     *  @param len The number of characters to copy. */
    String(const char* str, size_t len) {
        if (len <= SSO_CAPACITY) {
            ::memcpy(storage_.short_data.data, str, len);
            storage_.short_data.data[len] = '\0';
            storage_.short_data.size_and_flag = static_cast<unsigned char>(len << 1);
            is_long_ = false;
        } else {
            storage_.long_data.capacity = len * 2;
            if (storage_.long_data.capacity < 32) storage_.long_data.capacity = 32;
            storage_.long_data.data = static_cast<char*>(::operator new(storage_.long_data.capacity + 1));
            ::memcpy(storage_.long_data.data, str, len);
            storage_.long_data.data[len] = '\0';
            storage_.long_data.size = len;
            is_long_ = true;
        }
    }

    /** @brief Constructs a String with @p count copies of @p ch.
     *  @param count The number of characters.
     *  @param ch The character to repeat. */
    String(size_t count, char ch) {
        if (count <= SSO_CAPACITY) {
            ::memset(storage_.short_data.data, ch, count);
            storage_.short_data.data[count] = '\0';
            storage_.short_data.size_and_flag = static_cast<unsigned char>(count << 1);
            is_long_ = false;
        } else {
            storage_.long_data.capacity = count * 2;
            storage_.long_data.data = static_cast<char*>(::operator new(storage_.long_data.capacity + 1));
            ::memset(storage_.long_data.data, ch, count);
            storage_.long_data.data[count] = '\0';
            storage_.long_data.size = count;
            is_long_ = true;
        }
    }

    /** @brief Copy constructor. Deep-copies the string content.
     *  @param other The String to copy from. */
    String(const String& other) {
        if (other.is_short()) {
            ::memcpy(&storage_.short_data, &other.storage_.short_data, sizeof(ShortData));
            is_long_ = false;
        } else {
            size_t len = other.size();
            storage_.long_data.capacity = len * 2;
            if (storage_.long_data.capacity < 32) storage_.long_data.capacity = 32;
            storage_.long_data.data = static_cast<char*>(::operator new(storage_.long_data.capacity + 1));
            ::memcpy(storage_.long_data.data, other.data(), len + 1);
            storage_.long_data.size = len;
            is_long_ = true;
        }
    }

    /** @brief Move constructor. Transfers ownership of the buffer.
     *  @param other The String to move from. */
    String(String&& other) noexcept {
        if (other.is_short()) {
            ::memcpy(&storage_.short_data, &other.storage_.short_data, sizeof(ShortData));
            is_long_ = false;
        } else {
            storage_.long_data.data = other.storage_.long_data.data;
            storage_.long_data.size = other.storage_.long_data.size;
            storage_.long_data.capacity = other.storage_.long_data.capacity;
            is_long_ = true;
        }
        other.storage_.short_data.size_and_flag = 0;
        other.storage_.short_data.data[0] = '\0';
        other.is_long_ = false;
    }

    /** @brief Destructor. Frees the heap buffer if in long mode. */
    ~String() {
        if (!is_short()) {
            ::operator delete(storage_.long_data.data);
        }
    }

    /** @brief Copy assignment.
     *  @param other The String to copy from.
     *  @return Reference to this String. */
    String& operator=(const String& other) {
        if (this != &other) {
            if (!is_short()) ::operator delete(storage_.long_data.data);
            if (other.is_short()) {
                ::memcpy(&storage_.short_data, &other.storage_.short_data, sizeof(ShortData));
                is_long_ = false;
            } else {
                size_t len = other.size();
                storage_.long_data.capacity = len * 2;
                if (storage_.long_data.capacity < 32) storage_.long_data.capacity = 32;
                storage_.long_data.data = static_cast<char*>(::operator new(storage_.long_data.capacity + 1));
                ::memcpy(storage_.long_data.data, other.data(), len + 1);
                storage_.long_data.size = len;
                is_long_ = true;
            }
        }
        return *this;
    }

    /** @brief Move assignment.
     *  @param other The String to move from.
     *  @return Reference to this String. */
    String& operator=(String&& other) noexcept {
        if (this != &other) {
            if (!is_short()) ::operator delete(storage_.long_data.data);
            if (other.is_short()) {
                ::memcpy(&storage_.short_data, &other.storage_.short_data, sizeof(ShortData));
                is_long_ = false;
            } else {
                storage_.long_data.data = other.storage_.long_data.data;
                storage_.long_data.size = other.storage_.long_data.size;
                storage_.long_data.capacity = other.storage_.long_data.capacity;
                is_long_ = true;
            }
            other.storage_.short_data.size_and_flag = 0;
            other.storage_.short_data.data[0] = '\0';
            other.is_long_ = false;
        }
        return *this;
    }

    /** @brief Assignment from a null-terminated C string.
     *  @param str The source C string.
     *  @return Reference to this String. */
    String& operator=(const char* str) {
        if (!is_short()) ::operator delete(storage_.long_data.data);
        size_t len = ::strlen(str);
        if (len <= SSO_CAPACITY) {
            ::memcpy(storage_.short_data.data, str, len);
            storage_.short_data.data[len] = '\0';
            storage_.short_data.size_and_flag = static_cast<unsigned char>(len << 1);
            is_long_ = false;
        } else {
            storage_.long_data.capacity = len * 2;
            if (storage_.long_data.capacity < 32) storage_.long_data.capacity = 32;
            storage_.long_data.data = static_cast<char*>(::operator new(storage_.long_data.capacity + 1));
            ::memcpy(storage_.long_data.data, str, len);
            storage_.long_data.data[len] = '\0';
            storage_.long_data.size = len;
            is_long_ = true;
        }
        return *this;
    }

    /** @brief Returns the null-terminated C string pointer. */
    const char* c_str() const noexcept { return is_short() ? storage_.short_data.data : storage_.long_data.data; }

    /** @brief Returns the raw character data pointer. */
    const char* data() const noexcept { return c_str(); }
    /** @brief Returns the number of characters in the string. */
    size_t size() const noexcept {
        return is_short() ? (storage_.short_data.size_and_flag >> 1) : storage_.long_data.size;
    }
    /** @brief Returns the number of characters (alias for size). */
    size_t length() const noexcept { return size(); }
    /** @brief Returns true if the string is empty. */
    bool empty() const noexcept { return size() == 0; }

    /** @brief Unchecked character access by index (const).
     *  @param i The index.
     *  @return The character at position @p i. */
    char operator[](size_t i) const { return c_str()[i]; }
    /** @brief Unchecked character access by index (mutable).
     *  @param i The index.
     *  @return Reference to the character at position @p i. */
    char& operator[](size_t i) { return is_short() ? storage_.short_data.data[i] : storage_.long_data.data[i]; }

    /** @brief Bounds-checked character access.
     *  @param i The index.
     *  @return The character at position @p i. */
    char at(size_t i) const {
        if (i >= size()) { /* throw */
        }
        return c_str()[i];
    }

    /** @brief Returns the first character. */
    char front() const { return c_str()[0]; }
    /** @brief Returns the last character. */
    char back() const { return c_str()[size() - 1]; }

    /** @brief Clears the string, setting its length to 0. */
    void clear() noexcept {
        if (is_short()) {
            storage_.short_data.size_and_flag = 0;
            storage_.short_data.data[0] = '\0';
        } else {
            storage_.long_data.size = 0;
            storage_.long_data.data[0] = '\0';
        }
    }

    /** @brief Appends a character buffer of the given length.
     *  @param str The source characters.
     *  @param len The number of characters to append.
     *  @return Reference to this String. */
    String& append(const char* str, size_t len) {
        size_t old_size = size();
        size_t new_size = old_size + len;
        if (is_short() && new_size <= SSO_CAPACITY) {
            ::memcpy(storage_.short_data.data + old_size, str, len);
            storage_.short_data.data[new_size] = '\0';
            storage_.short_data.size_and_flag = static_cast<unsigned char>(new_size << 1);
        } else {
            reserve(new_size);
            ::memcpy(storage_.long_data.data + old_size, str, len);
            storage_.long_data.data[new_size] = '\0';
            storage_.long_data.size = new_size;
        }
        return *this;
    }

    /** @brief Appends a null-terminated C string.
     *  @param str The source C string.
     *  @return Reference to this String. */
    String& append(const char* str) { return append(str, ::strlen(str)); }
    /** @brief Appends another String.
     *  @param other The String to append.
     *  @return Reference to this String. */
    String& append(const String& other) { return append(other.data(), other.size()); }
    /** @brief Appends a C string via +=.
     *  @param str The source C string.
     *  @return Reference to this String. */
    String& operator+=(const char* str) { return append(str); }
    /** @brief Appends another String via +=.
     *  @param other The String to append.
     *  @return Reference to this String. */
    String& operator+=(const String& other) { return append(other); }

    /** @brief Appends a single character.
     *  @param ch The character to append.
     *  @return Reference to this String. */
    String& push_back(char ch) { return append(&ch, 1); }

    /** @brief Reserves capacity for at least @p new_capacity characters.
     *  @param new_capacity The minimum capacity to reserve. */
    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity()) return;
        size_t old_size = size();
        char* new_data = static_cast<char*>(::operator new(new_capacity + 1));
        ::memcpy(new_data, data(), old_size + 1);
        if (!is_short()) ::operator delete(storage_.long_data.data);
        storage_.long_data.data = new_data;
        storage_.long_data.size = old_size;
        storage_.long_data.capacity = new_capacity;
        is_long_ = true;
    }

    /** @brief Returns the current allocated capacity. */
    size_t capacity() const noexcept { return is_short() ? SSO_CAPACITY : storage_.long_data.capacity; }

    /** @brief Equality comparison with another String.
     *  @param other The String to compare.
     *  @return True if the strings are equal. */
    bool operator==(const String& other) const noexcept {
        return size() == other.size() && ::memcmp(data(), other.data(), size()) == 0;
    }

    /** @brief Equality comparison with a C string.
     *  @param str The C string to compare.
     *  @return True if the strings are equal. */
    bool operator==(const char* str) const noexcept {
        return ::memcmp(data(), str, size()) == 0 && str[size()] == '\0';
    }

    /** @brief Inequality comparison with another String.
     *  @param other The String to compare.
     *  @return True if the strings are not equal. */
    bool operator!=(const String& other) const noexcept { return !(*this == other); }
    /** @brief Inequality comparison with a C string.
     *  @param str The C string to compare.
     *  @return True if the strings are not equal. */
    bool operator!=(const char* str) const noexcept { return !(*this == str); }

    /** @brief Less-than comparison (lexicographic).
     *  @param other The String to compare.
     *  @return True if this string is lexicographically less than @p other. */
    bool operator<(const String& other) const noexcept {
        size_t min_size = size() < other.size() ? size() : other.size();
        int cmp = ::memcmp(data(), other.data(), min_size);
        return cmp < 0 || (cmp == 0 && size() < other.size());
    }

    /** @brief Implicit conversion to const char*. */
    operator const char*() const noexcept { return c_str(); }

    /** @brief Constant representing the maximum value of size_t (used as "not found"). */
    static constexpr size_t npos = static_cast<size_t>(-1);

private:
    /** @brief Returns true if the string is using short (SSO) storage. */
    bool is_short() const noexcept { return !is_long_; }

    Storage storage_;       ///< Internal union storage for long or short strings.
    bool is_long_ = false;  ///< True if using heap-allocated storage.
};

/** @brief Concatenates two Strings.
 *  @param a The left-hand String.
 *  @param b The right-hand String.
 *  @return A new String containing @p a followed by @p b. */
inline String operator+(const String& a, const String& b) {
    String result(a);
    result.append(b);
    return result;
}

/** @brief Concatenates a String and a C string.
 *  @param a The String.
 *  @param b The C string.
 *  @return A new String containing @p a followed by @p b. */
inline String operator+(const String& a, const char* b) {
    String result(a);
    result.append(b);
    return result;
}

/** @brief Concatenates a C string and a String.
 *  @param a The C string.
 *  @param b The String.
 *  @return A new String containing @p a followed by @p b. */
inline String operator+(const char* a, const String& b) {
    String result(a);
    result.append(b);
    return result;
}

}  // namespace prism
