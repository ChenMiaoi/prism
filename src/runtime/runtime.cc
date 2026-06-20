#include "prism/runtime/runtime.hpp"

#include <cstdio>
#include <cstdlib>

extern "C" {

/** @brief Aborts the program with an optional error message.
 *  @param message The error message to print to stderr before aborting, or nullptr for no message.
 */
void prism_runtime_abort(const char* message) {
    if (message) {
        ::fputs(message, stderr);
        ::fputc('\n', stderr);
    }
    ::abort();
}

/** @brief Prints a 64-bit integer to stdout followed by a newline.
 *  @param value The integer value to print.
 */
void prism_runtime_print_i64(long long value) {
    ::fprintf(stdout, "%lld\n", value);
}

/** @brief Reads a 64-bit integer from stdin.
 *  @return The parsed integer, or 0 if parsing fails.
 */
long long prism_runtime_read_i64() {
    long long value = 0;
    if (::scanf("%lld", &value) != 1) return 0;
    return value;
}
}
