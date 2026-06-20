#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

namespace prism {

/** @brief A non-owning reference to a contiguous sequence of characters. */
class StringRef {
public:
    /** @brief The element type (char). */
    using value_type = char;

    /** @brief Unsigned integer type for sizes and indices. */
    using size_type = std::size_t;

    /** @brief Constant iterator type. */
    using iterator = const char*;

    /** @brief Default-construct an empty StringRef. */
    constexpr StringRef() noexcept : data_(nullptr), length_(0) {}

    /** @brief Construct from a null-terminated C string. */
    constexpr StringRef(const char* str) noexcept : data_(str), length_(str ? std::strlen(str) : 0) {}

    /** @brief Construct from a pointer and explicit length. */
    constexpr StringRef(const char* str, size_type length) noexcept : data_(str), length_(length) {}

    /** @brief Construct from a std::string_view. */
    constexpr StringRef(std::string_view sv) noexcept : data_(sv.data()), length_(sv.size()) {}

    /** @brief Construct from a std::string (non-owning reference to its data). */
    constexpr StringRef(const std::string& str) noexcept : data_(str.data()), length_(str.size()) {}

    /** @brief Get the underlying character pointer. */
    constexpr auto data() const -> const char* { return data_; }

    /** @brief Get the number of characters in the string. */
    constexpr auto size() const -> size_type { return length_; }

    /** @brief Get the number of characters in the string. */
    constexpr auto length() const -> size_type { return length_; }

    /** @brief Check whether the string is empty. */
    constexpr auto empty() const -> bool { return length_ == 0; }

    /** @brief Access the character at the given index. */
    constexpr auto operator[](size_type index) const -> char {
        assert(index < length_ && "Index out of bounds");
        return data_[index];
    }

    /** @brief Get the first character. */
    constexpr auto front() const -> char { return data_[0]; }

    /** @brief Get the last character. */
    constexpr auto back() const -> char { return data_[length_ - 1]; }

    /** @brief Return an iterator to the beginning. */
    constexpr auto begin() const -> iterator { return data_; }

    /** @brief Return an iterator past the end. */
    constexpr auto end() const -> iterator { return data_ + length_; }

    /**
     * @brief Return a substring starting at the given position.
     *
     * @param start The starting index.
     * @param count Maximum number of characters to include (defaults to npos = all remaining).
     * @return A new StringRef pointing into the substring.
     */
    constexpr auto substr(size_type start, size_type count = npos) const -> StringRef {
        assert(start <= length_ && "Start out of bounds");
        size_type actual = std::min(count, length_ - start);
        return {data_ + start, actual};
    }

    /** @brief Convert this StringRef to a std::string (allocating copy). */
    auto str() const -> std::string { return {data_, length_}; }

    /** @brief Check for exact equality with another StringRef. */
    constexpr auto operator==(const StringRef& other) const -> bool {
        if (length_ != other.length_) return false;
        return std::memcmp(data_, other.data_, length_) == 0;
    }

    /** @brief Three-way comparison (lexicographic ordering). */
    constexpr auto operator<=>(const StringRef& other) const -> std::strong_ordering {
        int result = std::memcmp(data_, other.data_, std::min(length_, other.length_));
        if (result != 0) return result < 0 ? std::strong_ordering::less : std::strong_ordering::greater;
        if (length_ < other.length_) return std::strong_ordering::less;
        if (length_ > other.length_) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    /** @brief Check whether this string starts with the given prefix. */
    auto starts_with(StringRef prefix) const -> bool {
        return length_ >= prefix.length_ && std::memcmp(data_, prefix.data_, prefix.length_) == 0;
    }

    /** @brief Check whether this string ends with the given suffix. */
    auto ends_with(StringRef suffix) const -> bool {
        return length_ >= suffix.length_ &&
               std::memcmp(data_ + length_ - suffix.length_, suffix.data_, suffix.length_) == 0;
    }

    /**
     * @brief Find the first occurrence of a character.
     *
     * @param c The character to search for.
     * @param pos The position to start searching from.
     * @return The index of the first occurrence, or npos if not found.
     */
    auto find(char c, size_type pos = 0) const -> size_type {
        for (size_type i = pos; i < length_; ++i) {
            if (data_[i] == c) return i;
        }
        return npos;
    }

    /**
     * @brief Find the first occurrence of a substring.
     *
     * @param str The substring to search for.
     * @param pos The position to start searching from.
     * @return The index of the first occurrence, or npos if not found.
     */
    auto find(StringRef str, size_type pos = 0) const -> size_type {
        if (str.length_ == 0) return pos;
        if (str.length_ > length_) return npos;
        for (size_type i = pos; i <= length_ - str.length_; ++i) {
            if (std::memcmp(data_ + i, str.data_, str.length_) == 0) return i;
        }
        return npos;
    }

    /** @brief Sentinel value indicating "no position found" (equivalent to string::npos). */
    static constexpr size_type npos = static_cast<size_type>(-1);

private:
    const char* data_;  ///< Pointer to the character data (not owned).
    size_type length_;  ///< Number of characters in the reference.
};

}  // namespace prism
