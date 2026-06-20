#pragma once

#include "prism/basic/error.hpp"
#include "prism/basic/source_location.hpp"
#include "prism/core/container/expected.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"

#include <cstdio>

namespace prism::frontend {

/** @brief Manages source file metadata including filename and source content. */
class SourceManager {
public:
    /** @brief Default constructor. */
    SourceManager() = default;

    /** @brief Creates a SourceLocation at the given line and column in the current file. @return The constructed
     * SourceLocation. */
    SourceLocation location(unsigned line, unsigned column) const {
        return SourceLocation(filename_.c_str(), line, column);
    }

    /** @brief Creates a SourceRange spanning from the given start to end positions. @return The constructed
     * SourceRange. */
    SourceRange range(unsigned begin_line, unsigned begin_column, unsigned end_line, unsigned end_column) const {
        return SourceRange(location(begin_line, begin_column), location(end_line, end_column));
    }

    /** @brief Sets the main source buffer with the given filename and source text. */
    void set_main_buffer(const char* filename, const char* source) {
        filename_ = filename;
        source_ = source;
    }

    /** @brief Returns the filename of the current source. @return Reference to the filename string. */
    const String& filename() const { return filename_; }

    /** @brief Returns the source text content. @return Reference to the source string. */
    const String& source() const { return source_; }

private:
    String filename_ = "<input>";
    String source_;
};

/** @brief Handles file I/O operations for reading source files from disk. */
class FileManager {
public:
    /** @brief Reads the entire contents of the named file. @param filename Path to the file to read. @return The file
     * contents as a string, or an error. */
    Expected<String, Error> read_file(const char* filename) const {
        FILE* file = ::fopen(filename, "rb");
        if (!file) return Unexpected<Error>(make_error("cannot open input file"));
        ::fseek(file, 0, SEEK_END);
        long size = ::ftell(file);
        ::fseek(file, 0, SEEK_SET);
        if (size < 0) {
            ::fclose(file);
            return Unexpected<Error>(make_error("cannot determine input file size"));
        }
        Vector<char> bytes;
        bytes.resize(static_cast<size_t>(size) + 1);
        size_t read = ::fread(bytes.data(), 1, static_cast<size_t>(size), file);
        ::fclose(file);
        bytes[read] = '\0';
        return String(bytes.data(), read);
    }
};

}  // namespace prism::frontend
