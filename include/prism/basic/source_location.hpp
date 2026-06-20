#pragma once

#include <cstddef>
#include <cstdint>

namespace prism {

/** @brief Represents a position in a source file (file, line, column). */
class SourceLocation {
public:
    /** @brief Default constructor; creates an invalid (zero) location. */
    constexpr SourceLocation() = default;

    /** @brief Construct a source location from file, line, and column. */
    constexpr SourceLocation(const char* file, unsigned line, unsigned col)
        : filename_(file), line_(line), column_(col) {}

    /** @brief Get the source filename. */
    constexpr const char* filename() const { return filename_; }

    /** @brief Get the 1-based line number. */
    constexpr unsigned line() const { return line_; }

    /** @brief Get the 1-based column number. */
    constexpr unsigned column() const { return column_; }

    /** @brief Check whether this location points to a valid source position. */
    constexpr bool is_valid() const { return line_ > 0; }

    /** @brief Equality comparison; two locations are equal if they have the same line and column. */
    constexpr bool operator==(const SourceLocation& other) const {
        return line_ == other.line_ && column_ == other.column_;
    }

    /** @brief Return an invalid (sentinel) source location. */
    static constexpr SourceLocation invalid() { return {}; }

private:
    const char* filename_ = "";  ///< Pointer to the source filename string.
    unsigned line_ = 0;          ///< 1-based line number; 0 means invalid.
    unsigned column_ = 0;        ///< 1-based column number.
};

/** @brief Represents a half-open range [begin, end) in a source file. */
class SourceRange {
public:
    /** @brief Default constructor; creates an invalid range. */
    constexpr SourceRange() = default;

    /** @brief Construct a range from begin and end source locations. */
    constexpr SourceRange(SourceLocation begin, SourceLocation end) : begin_(begin), end_(end) {}

    /** @brief Get the start location of the range. */
    constexpr auto begin() const -> SourceLocation { return begin_; }

    /** @brief Get the end location of the range. */
    constexpr auto end() const -> SourceLocation { return end_; }

    /** @brief Check whether this range has a valid start location. */
    constexpr auto is_valid() const -> bool { return begin_.is_valid(); }

private:
    SourceLocation begin_;  ///< Start of the range.
    SourceLocation end_;    ///< End of the range (exclusive).
};

}  // namespace prism
