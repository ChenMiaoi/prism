#pragma once

#include "prism/basic/source_location.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

namespace prism {

/** @brief Severity level for diagnostic messages. */
enum class DiagLevel {
    /** @brief Informational note. */
    Note,
    /** @brief Warning-level diagnostic. */
    Warning,
    /** @brief Error-level diagnostic. */
    Error,
    /** @brief Fatal error diagnostic. */
    Fatal
};

/** @brief A single diagnostic message with severity, text, optional code, and source location. */
struct Diagnostic {
    DiagLevel level;          ///< Severity level of this diagnostic.
    String message;           ///< Human-readable diagnostic message.
    String code;              ///< Optional diagnostic code identifier (e.g. "err-name-undef").
    SourceLocation location;  ///< Source location that triggered this diagnostic.
};

/** @brief Collects and manages diagnostic messages emitted during compilation or processing. */
class DiagnosticsEngine {
public:
    /** @brief Report a diagnostic with only a message and optional location. */
    auto report(DiagLevel level, const char* message, SourceLocation loc = SourceLocation::invalid()) -> void;

    /** @brief Report a diagnostic with an associated code, message, location, and optional range. */
    auto report(DiagLevel level, const char* code, const char* message, SourceLocation loc,
                SourceRange range = SourceRange()) -> void;

    /** @brief Emit an informational note diagnostic. */
    auto note(const char* message, SourceLocation loc = SourceLocation::invalid()) -> void;

    /** @brief Emit a help hint diagnostic. */
    auto help(const char* message, SourceLocation loc = SourceLocation::invalid()) -> void;

    /** @brief Check whether any error-level (or higher) diagnostics have been reported. */
    bool has_errors() const;

    /** @brief Get the list of all recorded diagnostics. */
    const Vector<Diagnostic>& diagnostics() const;

    /** @brief Get the total number of error-level diagnostics. */
    size_t error_count() const;

    /** @brief Clear all recorded diagnostics. */
    void clear();

    /** @brief Enable or suppress diagnostic output. */
    void set_quiet(bool quiet) { quiet_ = quiet; }

    /** @brief Check whether quiet mode is enabled. */
    bool is_quiet() const { return quiet_; }

private:
    Vector<Diagnostic> diagnostics_;  ///< All recorded diagnostics.
    bool quiet_ = false;              ///< When true, diagnostic output is suppressed.
};

}  // namespace prism
