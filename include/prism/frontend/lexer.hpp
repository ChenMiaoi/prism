#pragma once

#include "prism/basic/diagnostics.hpp"
#include "prism/basic/error.hpp"
#include "prism/basic/source_location.hpp"
#include "prism/core/container/expected.hpp"
#include "prism/core/container/vector.hpp"
#include "prism/core/string/string.hpp"
#include "prism/frontend/source_manager.hpp"

#include <cstddef>

namespace prism::frontend {

/** @brief Token kind enumeration for the Prism lexer. */
enum class TokenKind {
    EndOfFile,       /**< End of file marker. */
    Identifier,      /**< Identifier token. */
    IntegerLiteral,  /**< Integer literal token. */
    StringLiteral,   /**< String literal token. */
    CharLiteral,     /**< Character literal token. */
    KeywordInt,      /**< 'int' keyword. */
    KeywordVoid,     /**< 'void' keyword. */
    KeywordReturn,   /**< 'return' keyword. */
    KeywordIf,       /**< 'if' keyword. */
    KeywordElse,     /**< 'else' keyword. */
    KeywordWhile,    /**< 'while' keyword. */
    KeywordFor,      /**< 'for' keyword. */
    KeywordBool,     /**< 'bool' keyword. */
    KeywordTrue,     /**< 'true' keyword. */
    KeywordFalse,    /**< 'false' keyword. */
    KeywordClass,    /**< 'class' keyword. */
    KeywordStruct,   /**< 'struct' keyword. */
    KeywordTemplate, /**< 'template' keyword. */
    KeywordTypename, /**< 'typename' keyword. */
    LParen,          /**< Left parenthesis '('. */
    RParen,          /**< Right parenthesis ')'. */
    LBrace,          /**< Left brace '{'. */
    RBrace,          /**< Right brace '}'. */
    Comma,           /**< Comma ','. */
    Semicolon,       /**< Semicolon ';'. */
    Plus,            /**< Plus operator '+'. */
    Minus,           /**< Minus operator '-'. */
    Star,            /**< Star operator '*'. */
    Slash,           /**< Slash operator '/'. */
    Percent,         /**< Percent operator '%'. */
    Amp,             /**< Ampersand operator '&'. */
    Equal,           /**< Assignment operator '=' or equality in other contexts. */
    EqualEqual,      /**< Equality operator '=='. */
    Bang,            /**< Logical NOT operator '!'. */
    Tilde,           /**< Bitwise NOT operator '~'. */
    BangEqual,       /**< Not-equal operator '!='. */
    Less,            /**< Less-than operator '<'. */
    LessEqual,       /**< Less-than-or-equal operator '<='. */
    Greater,         /**< Greater-than operator '>'. */
    GreaterEqual,    /**< Greater-than-or-equal operator '>='. */
};

/** @brief Represents a single lexical token produced by the Lexer. */
class Token {
public:
    /** @brief Default constructor. */
    Token() = default;

    /** @brief Constructs a Token with the given kind, spelling, and source range. */
    Token(TokenKind kind, String spelling, SourceRange range)
        : kind_(kind), spelling_(static_cast<String&&>(spelling)), range_(range) {}

    /** @brief Returns the kind of this token. */
    TokenKind kind() const { return kind_; }

    /** @brief Returns the raw source text of this token. */
    const String& spelling() const { return spelling_; }

    /** @brief Returns the source range of this token. */
    SourceRange range() const { return range_; }

    /** @brief Returns the beginning source location of this token. */
    SourceLocation loc() const { return range_.begin(); }

private:
    TokenKind kind_ = TokenKind::EndOfFile;
    String spelling_;
    SourceRange range_;
};

/** @brief Lexical analyzer that converts source text into a stream of tokens. */
class Lexer {
public:
    /** @brief Constructs a Lexer for the given source file. */
    Lexer(SourceManager& source_manager, DiagnosticsEngine& diag);

    /** @brief Tokenizes the entire source and returns all tokens. @return Vector of tokens or an error. */
    Expected<Vector<Token>, Error> tokenize();

private:
    /** @brief Peeks at the current character without consuming it. */
    char peek(size_t offset = 0) const;

    /** @brief Consumes and returns the current character. */
    char advance();

    /** @brief Matches the current character against expected; advances if matched. */
    bool match(char expected);

    /** @brief Skips whitespace and comments in the source. */
    void skip_whitespace_and_comments();

    /** @brief Creates a token spanning from the given offset to the current position. */
    Token make_token(TokenKind kind, size_t begin_offset, unsigned begin_line, unsigned begin_column) const;

    /** @brief Lexes an identifier or keyword token. */
    Expected<Token, Error> lex_identifier_or_keyword(size_t begin_offset, unsigned begin_line, unsigned begin_column);

    /** @brief Lexes a numeric literal token. */
    Expected<Token, Error> lex_number(size_t begin_offset, unsigned begin_line, unsigned begin_column);

    /** @brief Lexes a string literal token. */
    Expected<Token, Error> lex_string(size_t begin_offset, unsigned begin_line, unsigned begin_column);

    /** @brief Lexes a character literal token. */
    Expected<Token, Error> lex_char(size_t begin_offset, unsigned begin_line, unsigned begin_column);

    /** @brief Rejects a legacy or unsupported token with the given error message. */
    Expected<Token, Error> reject_legacy_token(size_t begin_offset, unsigned begin_line, unsigned begin_column,
                                               const char* message);

    SourceManager& source_manager_;
    DiagnosticsEngine& diag_;
    const char* source_ = "";
    size_t offset_ = 0;
    unsigned line_ = 1;
    unsigned column_ = 1;
};

/** @brief Returns the human-readable name of the given token kind. @return C-string name. */
const char* token_kind_name(TokenKind kind);

}  // namespace prism::frontend
