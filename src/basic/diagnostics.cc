#include "prism/basic/diagnostics.hpp"

#include "prism/core/format/format.hpp"

namespace prism {

/** @brief Reports a diagnostic with a message and optional source location.
 *  @param level   The severity level of the diagnostic.
 *  @param message The diagnostic message text.
 *  @param loc     The source location where the diagnostic was triggered. */
void DiagnosticsEngine::report(DiagLevel level, const char* message, SourceLocation loc) {
    report(level, "E0000", message, loc, SourceRange(loc, loc));
}

/** @brief Reports a diagnostic with an associated code, message, source location, and optional range.
 *  @param level   The severity level of the diagnostic.
 *  @param code    The diagnostic code identifier.
 *  @param message The diagnostic message text.
 *  @param loc     The source location where the diagnostic was triggered.
 *  @param range   The source range associated with the diagnostic. */
void DiagnosticsEngine::report(DiagLevel level, const char* code, const char* message, SourceLocation loc,
                               SourceRange range) {
    (void)range;
    if (quiet_ && level != DiagLevel::Fatal) return;

    Diagnostic diag{level, String(message), String(code), loc};
    diagnostics_.push_back(diag);

    const char* prefix = "";
    const char* label = "";
    switch (level) {
    case DiagLevel::Note:
        prefix = "note";
        break;
    case DiagLevel::Warning:
        prefix = "warning";
        break;
    case DiagLevel::Error:
        prefix = "error";
        label = "error starts here";
        break;
    case DiagLevel::Fatal:
        prefix = "fatal";
        label = "fatal error starts here";
        break;
    }

    if (loc.is_valid()) {
        prism::print(stderr, "{}[{}]: {}\n", prefix, code, message);
        prism::print(stderr, " --> {}:{}:{}\n", loc.filename(), loc.line(), loc.column());
        prism::print(stderr, "  |\n");
        prism::print(stderr, "{} | {}\n", loc.line(), label);
        prism::print(stderr, "  | ^\n");
    } else {
        prism::print(stderr, "{}[{}]: {}\n", prefix, code, message);
    }
}

/** @brief Emits an informational note diagnostic.
 *  @param message The note message text.
 *  @param loc     The source location associated with the note. */
void DiagnosticsEngine::note(const char* message, SourceLocation loc) {
    report(DiagLevel::Note, "N0000", message, loc, SourceRange(loc, loc));
}

/** @brief Emits a help hint diagnostic.
 *  @param message The help message text.
 *  @param loc     The source location associated with the help hint. */
void DiagnosticsEngine::help(const char* message, SourceLocation loc) {
    report(DiagLevel::Note, "H0000", message, loc, SourceRange(loc, loc));
}

/** @brief Checks whether any error-level or fatal-level diagnostics have been recorded.
 *  @return True if at least one error or fatal diagnostic exists. */
bool DiagnosticsEngine::has_errors() const {
    for (size_t i = 0; i < diagnostics_.size(); ++i) {
        if (diagnostics_[i].level == DiagLevel::Error || diagnostics_[i].level == DiagLevel::Fatal) {
            return true;
        }
    }
    return false;
}

/** @brief Returns the list of all recorded diagnostics.
 *  @return A const reference to the vector of Diagnostic objects. */
const Vector<Diagnostic>& DiagnosticsEngine::diagnostics() const {
    return diagnostics_;
}

/** @brief Returns the total number of error-level and fatal-level diagnostics.
 *  @return The count of error and fatal diagnostics. */
size_t DiagnosticsEngine::error_count() const {
    size_t count = 0;
    for (size_t i = 0; i < diagnostics_.size(); ++i) {
        if (diagnostics_[i].level == DiagLevel::Error || diagnostics_[i].level == DiagLevel::Fatal) {
            ++count;
        }
    }
    return count;
}

/** @brief Clears all recorded diagnostics from the engine. */
void DiagnosticsEngine::clear() {
    diagnostics_.clear();
}

}  // namespace prism
