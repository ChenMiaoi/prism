#pragma once

#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"
#include "prism/core/string/string_view.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace prism {

namespace detail {

/** @brief Appends the string representation of @p value to @p out.
 *  @tparam T The type of the value to format.
 *  @param out The output string to append to.
 *  @param value The value to format. */
template <typename T>
void format_value(String& out, const T& value) {
    if constexpr (is_integral_v<T>) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
        out.append(buf, static_cast<size_t>(len));
    } else if constexpr (is_floating_point_v<T>) {
        char buf[64];
        int len = ::snprintf(buf, sizeof(buf), "%f", static_cast<double>(value));
        out.append(buf, static_cast<size_t>(len));
    } else if constexpr (is_same_v<T, bool>) {
        out.append(value ? "true" : "false");
    } else if constexpr (is_same_v<T, const char*>) {
        out.append(value);
    } else if constexpr (is_same_v<T, String>) {
        out.append(value);
    } else if constexpr (is_same_v<T, StringView>) {
        out.append(value.data(), value.size());
    } else {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%p", static_cast<const void*>(&value));
        out.append(buf, static_cast<size_t>(len));
    }
}

/** @brief Base case for variadic format recursion — appends the remaining format string.
 *  @param out The output string to append to.
 *  @param fmt The remaining format string. */
inline void formatImpl(String& out, const char* fmt) {
    out.append(fmt);
}

/** @brief Recursive format implementation that substitutes '{}' placeholders with arguments.
 *  @param out The output string to append to.
 *  @param fmt The format string containing '{}' placeholders.
 *  @param value The current argument to substitute.
 *  @param rest Remaining arguments for subsequent placeholders. */
template <typename T, typename... Rest>
void formatImpl(String& out, const char* fmt, const T& value, const Rest&... rest) {
    while (*fmt) {
        if (*fmt == '{' && *(fmt + 1) == '}') {
            format_value(out, value);
            fmt += 2;
            formatImpl(out, fmt, rest...);
            return;
        }
        out.push_back(*fmt);
        ++fmt;
    }
}

}  // namespace detail

/** @brief Formats a string by replacing '{}' placeholders with the provided arguments.
 *  @tparam Args The types of the arguments to substitute.
 *  @param fmt The format string with '{}' placeholders.
 *  @param args The values to substitute into the format string.
 *  @return The formatted String. */
template <typename... Args>
String format(const char* fmt, const Args&... args) {
    String result;
    detail::formatImpl(result, fmt, args...);
    return result;
}

/** @brief Prints a formatted string to stdout.
 *  @tparam Args The types of the arguments to substitute.
 *  @param fmt The format string.
 *  @param args The values to substitute. */
template <typename... Args>
void print(const char* fmt, const Args&... args) {
    String result = format(fmt, args...);
    ::fputs(result.c_str(), stdout);
}

/** @brief Prints a formatted string to a specific FILE stream.
 *  @tparam Args The types of the arguments to substitute.
 *  @param file The output FILE stream.
 *  @param fmt The format string.
 *  @param args The values to substitute. */
template <typename... Args>
void print(FILE* file, const char* fmt, const Args&... args) {
    String result = format(fmt, args...);
    ::fputs(result.c_str(), file);
}

/** @brief Prints a formatted string followed by a newline to stdout.
 *  @tparam Args The types of the arguments to substitute.
 *  @param fmt The format string.
 *  @param args The values to substitute. */
template <typename... Args>
void println(const char* fmt, const Args&... args) {
    String result = format(fmt, args...);
    result.push_back('\n');
    ::fputs(result.c_str(), stdout);
}

/** @brief Prints a formatted string followed by a newline to a specific FILE stream.
 *  @tparam Args The types of the arguments to substitute.
 *  @param file The output FILE stream.
 *  @param fmt The format string.
 *  @param args The values to substitute. */
template <typename... Args>
void println(FILE* file, const char* fmt, const Args&... args) {
    String result = format(fmt, args...);
    result.push_back('\n');
    ::fputs(result.c_str(), file);
}

}  // namespace prism
