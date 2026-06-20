#pragma once

#include "prism/core/string/string.hpp"

#include <cstddef>
#include <cstdio>

namespace prism {

/** @brief Abstract base class for raw output streams providing formatted output via operator<<. */
class raw_ostream {
public:
    /** @brief Virtual destructor. */
    virtual ~raw_ostream() = default;

    /** @brief Pure virtual function to write raw bytes to the stream.
     *  @param data Pointer to the bytes to write.
     *  @param len Number of bytes to write. */
    virtual void write(const char* data, size_t len) = 0;

    /** @brief Writes a null-terminated C string to the stream.
     *  @param str The C string to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(const char* str) {
        write(str, ::strlen(str));
        return *this;
    }

    /** @brief Writes a String to the stream.
     *  @param str The String to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(const String& str) {
        write(str.data(), str.size());
        return *this;
    }

    /** @brief Writes a single character to the stream.
     *  @param ch The character to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(char ch) {
        write(&ch, 1);
        return *this;
    }

    /** @brief Writes an int value to the stream.
     *  @param value The int to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(int value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%d", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes a long value to the stream.
     *  @param value The long to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(long value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%ld", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes a long long value to the stream.
     *  @param value The long long to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(long long value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%lld", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes an unsigned int value to the stream.
     *  @param value The unsigned int to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(unsigned int value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%u", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes an unsigned long value to the stream.
     *  @param value The unsigned long to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(unsigned long value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%lu", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes an unsigned long long value to the stream.
     *  @param value The unsigned long long to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(unsigned long long value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%llu", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes a float value to the stream.
     *  @param value The float to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(float value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%f", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes a double value to the stream.
     *  @param value The double to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(double value) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%f", value);
        write(buf, static_cast<size_t>(len));
        return *this;
    }

    /** @brief Writes a bool value to the stream ("true" or "false").
     *  @param value The bool to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(bool value) {
        const char* str = value ? "true" : "false";
        write(str, value ? 4 : 5);
        return *this;
    }

    /** @brief Writes a pointer address to the stream in hexadecimal format.
     *  @param ptr The pointer to write.
     *  @return Reference to this stream. */
    raw_ostream& operator<<(void* ptr) {
        char buf[32];
        int len = ::snprintf(buf, sizeof(buf), "%p", ptr);
        write(buf, static_cast<size_t>(len));
        return *this;
    }
};

}  // namespace prism
