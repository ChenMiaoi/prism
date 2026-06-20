#pragma once

#include "prism/basic/source_location.hpp"
#include "prism/core/container/expected.hpp"
#include "prism/core/string/string.hpp"

namespace prism {

/** @brief Represents an error with an optional source location. */
class Error {
public:
    /** @brief Construct an error from a message and optional source location. */
    Error(const char* message, SourceLocation loc = SourceLocation::invalid()) : message_(message), loc_(loc) {}

    /** @brief Get the error message string. */
    const char* message() const { return message_.c_str(); }

    /** @brief Get the source location associated with this error. */
    SourceLocation location() const { return loc_; }

    /** @brief Format the error as a human-readable string. */
    String format() const;

private:
    String message_;      ///< The error message text.
    SourceLocation loc_;  ///< Source location where the error occurred.
};

/** @brief Create an Error from a message and optional source location. */
Error make_error(const char* message, SourceLocation loc = SourceLocation::invalid());

}  // namespace prism
