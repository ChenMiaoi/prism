#pragma once

#include <cstddef>
#include <cstring>

namespace prism {

/** @brief A non-owning, read-only view of a contiguous character sequence. */
class StringView {
public:
    /** @brief The character type. */
    using value_type = char;
    /** @brief The size type. */
    using size_type = size_t;
    /** @brief The iterator type (const char pointer). */
    using iterator = const char*;

    /** @brief Default-constructs an empty StringView. */
    constexpr StringView() noexcept : data_(nullptr), size_(0) {}
    /** @brief Constructs a StringView from a null-terminated C string.
     *  @param str The source C string. */
    StringView(const char* str) noexcept : data_(str), size_(str ? ::strlen(str) : 0) {}
    /** @brief Constructs a StringView from a pointer and length.
     *  @param str The start of the character range.
     *  @param len The number of characters in the view. */
    constexpr StringView(const char* str, size_t len) noexcept : data_(str), size_(len) {}
    /** @brief Copy constructor. */
    constexpr StringView(const StringView&) noexcept = default;
    /** @brief Copy assignment operator. */
    constexpr StringView& operator=(const StringView&) noexcept = default;

    /** @brief Returns a pointer to the underlying character array. */
    constexpr const char* data() const noexcept { return data_; }
    /** @brief Returns the number of characters in the view. */
    constexpr size_t size() const noexcept { return size_; }
    /** @brief Returns the number of characters (alias for size). */
    constexpr size_t length() const noexcept { return size_; }
    /** @brief Returns true if the view is empty. */
    constexpr bool empty() const noexcept { return size_ == 0; }

    /** @brief Unchecked character access by index.
     *  @param i The index.
     *  @return Const reference to the character at position @p i. */
    constexpr const char& operator[](size_t i) const { return data_[i]; }
    /** @brief Returns a reference to the first character. */
    constexpr const char& front() const { return data_[0]; }
    /** @brief Returns a reference to the last character. */
    constexpr const char& back() const { return data_[size_ - 1]; }

    /** @brief Returns an iterator to the beginning. */
    constexpr iterator begin() const noexcept { return data_; }
    /** @brief Returns an iterator past the end. */
    constexpr iterator end() const noexcept { return data_ + size_; }

    /** @brief Returns a sub-view starting at @p pos for @p count characters.
     *  @param pos Starting position.
     *  @param count Maximum number of characters (default: npos).
     *  @return A new StringView over the requested range. */
    constexpr StringView substr(size_t pos, size_t count = npos) const {
        if (pos > size_) return {};
        size_t actual = count < size_ - pos ? count : size_ - pos;
        return {data_ + pos, actual};
    }

    /** @brief Lexicographically compares this view with @p other.
     *  @param other The StringView to compare against.
     *  @return Negative if less, zero if equal, positive if greater. */
    int compare(StringView other) const noexcept {
        size_t min_size = size_ < other.size_ ? size_ : other.size_;
        int cmp = ::memcmp(data_, other.data_, min_size);
        if (cmp != 0) return cmp;
        if (size_ < other.size_) return -1;
        if (size_ > other.size_) return 1;
        return 0;
    }

    /** @brief Equality comparison.
     *  @param other The StringView to compare.
     *  @return True if both views have the same size and content. */
    bool operator==(StringView other) const noexcept {
        return size_ == other.size_ && ::memcmp(data_, other.data_, size_) == 0;
    }

    /** @brief Inequality comparison.
     *  @param other The StringView to compare.
     *  @return True if the views are not equal. */
    bool operator!=(StringView other) const noexcept { return !(*this == other); }
    /** @brief Less-than comparison.
     *  @param other The StringView to compare.
     *  @return True if this view is lexicographically less than @p other. */
    bool operator<(StringView other) const noexcept { return compare(other) < 0; }
    /** @brief Less-than-or-equal comparison.
     *  @param other The StringView to compare.
     *  @return True if this view is lexicographically less than or equal to @p other. */
    bool operator<=(StringView other) const noexcept { return compare(other) <= 0; }
    /** @brief Greater-than comparison.
     *  @param other The StringView to compare.
     *  @return True if this view is lexicographically greater than @p other. */
    bool operator>(StringView other) const noexcept { return compare(other) > 0; }
    /** @brief Greater-than-or-equal comparison.
     *  @param other The StringView to compare.
     *  @return True if this view is lexicographically greater than or equal to @p other. */
    bool operator>=(StringView other) const noexcept { return compare(other) >= 0; }

    /** @brief Finds the first occurrence of a character.
     *  @param c The character to find.
     *  @param pos The position to start searching from.
     *  @return The index of the first occurrence, or npos if not found. */
    size_t find(char c, size_t pos = 0) const {
        if (pos >= size_) return npos;
        for (size_t i = pos; i < size_; ++i) {
            if (data_[i] == c) return i;
        }
        return npos;
    }

    /** @brief Finds the first occurrence of a substring.
     *  @param str The substring to find.
     *  @param pos The position to start searching from.
     *  @return The index of the first occurrence, or npos if not found. */
    size_t find(StringView str, size_t pos = 0) const {
        if (str.size_ == 0) return pos;
        if (str.size_ > size_) return npos;
        for (size_t i = pos; i <= size_ - str.size_; ++i) {
            if (::memcmp(data_ + i, str.data_, str.size_) == 0) return i;
        }
        return npos;
    }

    /** @brief Checks if the view starts with the given prefix.
     *  @param prefix The prefix to check.
     *  @return True if the view starts with @p prefix. */
    bool starts_with(StringView prefix) const noexcept {
        return size_ >= prefix.size_ && ::memcmp(data_, prefix.data_, prefix.size_) == 0;
    }

    /** @brief Checks if the view ends with the given suffix.
     *  @param suffix The suffix to check.
     *  @return True if the view ends with @p suffix. */
    bool ends_with(StringView suffix) const noexcept {
        return size_ >= suffix.size_ && ::memcmp(data_ + size_ - suffix.size_, suffix.data_, suffix.size_) == 0;
    }

    /** @brief Constant representing the maximum value of size_t (used as "not found"). */
    static constexpr size_t npos = static_cast<size_t>(-1);

private:
    const char* data_;  ///< Pointer to the beginning of the character sequence.
    size_t size_;       ///< Number of characters in the view.
};

}  // namespace prism
