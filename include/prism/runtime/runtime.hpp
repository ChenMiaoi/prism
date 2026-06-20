#pragma once

extern "C" {

/** @brief Aborts the program with the given error message. @param message The error message to display. */
void prism_runtime_abort(const char* message);

/** @brief Prints a 64-bit integer value to standard output. @param value The integer to print. */
void prism_runtime_print_i64(long long value);

/** @brief Reads a 64-bit integer from standard input. @return The integer read from the user. */
long long prism_runtime_read_i64();
}
