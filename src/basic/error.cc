#include "prism/basic/error.hpp"

#include "prism/core/format/format.hpp"

namespace prism {

/** @brief Formats the error as a human-readable string with optional source location.
 *  @return A formatted string of the form "filename:line: message" if a valid location exists,
 *          otherwise the bare message text. */
String Error::format() const {
    if (loc_.is_valid()) {
        return prism::format("{}:{}: {}", loc_.filename(), loc_.line(), message_.c_str());
    }
    return message_;
}

/** @brief Creates an Error with the given message and optional source location.
 *  @param message The error message text.
 *  @param loc     The source location where the error occurred.
 *  @return A new Error instance. */
Error make_error(const char* message, SourceLocation loc) {
    return Error(message, loc);
}

}  // namespace prism
