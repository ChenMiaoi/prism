#include "prism/frontend/lexer.hpp"

#include "prism/core/format/format.hpp"

#include <cctype>
#include <cstring>

namespace prism::frontend {

/** @brief Constructs a Lexer that tokenizes the source managed by the given SourceManager.
 *  @param source_manager The source manager providing the source text.
 *  @param diag           The diagnostics engine for reporting lexical errors. */
Lexer::Lexer(SourceManager& source_manager, DiagnosticsEngine& diag)
    : source_manager_(source_manager), diag_(diag), source_(source_manager.source().c_str()) {
}

/** @brief Tokenizes the entire source text into a sequence of tokens.
 *  @return A vector of Token objects, or an Error if an invalid token is encountered. */
Expected<Vector<Token>, Error> Lexer::tokenize() {
    Vector<Token> tokens;
    while (peek() != '\0') {
        skip_whitespace_and_comments();
        const char c = peek();
        if (c == '\0') break;

        const size_t begin_offset = offset_;
        const unsigned begin_line = line_;
        const unsigned begin_column = column_;

        if (c == '?' && peek(1) == '?') {
            auto rejected = reject_legacy_token(begin_offset, begin_line, begin_column,
                                                "legacy trigraph spelling is not accepted by Prism");
            return Unexpected<Error>(rejected.error());
        }
        if ((c == '<' && peek(1) == ':') || (c == ':' && peek(1) == '>') || (c == '<' && peek(1) == '%') ||
            (c == '%' && peek(1) == '>') || (c == '%' && peek(1) == ':')) {
            auto rejected = reject_legacy_token(begin_offset, begin_line, begin_column,
                                                "legacy digraph spelling is not accepted by Prism");
            return Unexpected<Error>(rejected.error());
        }

        if (::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            auto token = lex_identifier_or_keyword(begin_offset, begin_line, begin_column);
            if (!token) return Unexpected<Error>(token.error());
            tokens.push_back(static_cast<Token&&>(token.value()));
            continue;
        }
        if (::isdigit(static_cast<unsigned char>(c))) {
            auto token = lex_number(begin_offset, begin_line, begin_column);
            if (!token) return Unexpected<Error>(token.error());
            tokens.push_back(static_cast<Token&&>(token.value()));
            continue;
        }

        advance();
        switch (c) {
        case '(':
            tokens.push_back(make_token(TokenKind::LParen, begin_offset, begin_line, begin_column));
            break;
        case ')':
            tokens.push_back(make_token(TokenKind::RParen, begin_offset, begin_line, begin_column));
            break;
        case '{':
            tokens.push_back(make_token(TokenKind::LBrace, begin_offset, begin_line, begin_column));
            break;
        case '}':
            tokens.push_back(make_token(TokenKind::RBrace, begin_offset, begin_line, begin_column));
            break;
        case ',':
            tokens.push_back(make_token(TokenKind::Comma, begin_offset, begin_line, begin_column));
            break;
        case ';':
            tokens.push_back(make_token(TokenKind::Semicolon, begin_offset, begin_line, begin_column));
            break;
        case '+':
            tokens.push_back(make_token(TokenKind::Plus, begin_offset, begin_line, begin_column));
            break;
        case '-':
            tokens.push_back(make_token(TokenKind::Minus, begin_offset, begin_line, begin_column));
            break;
        case '*':
            tokens.push_back(make_token(TokenKind::Star, begin_offset, begin_line, begin_column));
            break;
        case '/':
            tokens.push_back(make_token(TokenKind::Slash, begin_offset, begin_line, begin_column));
            break;
        case '%':
            tokens.push_back(make_token(TokenKind::Percent, begin_offset, begin_line, begin_column));
            break;
        case '&':
            tokens.push_back(make_token(TokenKind::Amp, begin_offset, begin_line, begin_column));
            break;
        case '=':
            tokens.push_back(make_token(match('=') ? TokenKind::EqualEqual : TokenKind::Equal, begin_offset, begin_line,
                                        begin_column));
            break;
        case '~':
            tokens.push_back(make_token(TokenKind::Tilde, begin_offset, begin_line, begin_column));
            break;
        case '!':
            tokens.push_back(make_token(match('=') ? TokenKind::BangEqual : TokenKind::Bang, begin_offset, begin_line,
                                        begin_column));
            break;
        case '<':
            tokens.push_back(make_token(match('=') ? TokenKind::LessEqual : TokenKind::Less, begin_offset, begin_line,
                                        begin_column));
            break;
        case '>':
            tokens.push_back(make_token(match('=') ? TokenKind::GreaterEqual : TokenKind::Greater, begin_offset,
                                        begin_line, begin_column));
            break;
        case '"': {
            auto token = lex_string(begin_offset, begin_line, begin_column);
            if (!token) return Unexpected<Error>(token.error());
            tokens.push_back(static_cast<Token&&>(token.value()));
            break;
        }
        case '\'': {
            auto token = lex_char(begin_offset, begin_line, begin_column);
            if (!token) return Unexpected<Error>(token.error());
            tokens.push_back(static_cast<Token&&>(token.value()));
            break;
        }
        default: {
            auto loc = source_manager_.location(begin_line, begin_column);
            auto error = make_error(prism::format("unexpected character '{}'", c).c_str(), loc);
            diag_.report(DiagLevel::Error, error.message(), loc);
            return Unexpected<Error>(static_cast<Error&&>(error));
        }
        }
    }

    tokens.push_back(Token(TokenKind::EndOfFile, "", source_manager_.range(line_, column_, line_, column_)));
    return tokens;
}

/** @brief Returns the character at the given offset from the current position without consuming it.
 *  @param offset The number of characters to look ahead (default 0 for current character).
 *  @return The character at the specified offset, or '\\0' at end of source. */
char Lexer::peek(size_t offset) const {
    return source_[offset_ + offset];
}

/** @brief Consumes and returns the current character, advancing the position.
 *  @return The consumed character. */
char Lexer::advance() {
    const char c = source_[offset_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

/** @brief Matches the current character against an expected value, advancing if it matches.
 *  @param expected The character to match against.
 *  @return True if the current character matched and was consumed, false otherwise. */
bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

/** @brief Skips whitespace and line/block comments in the source text. */
void Lexer::skip_whitespace_and_comments() {
    for (;;) {
        const char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
            continue;
        }
        if (c == '/' && peek(1) == '/') {
            while (peek() != '\0' && peek() != '\n')
                advance();
            continue;
        }
        if (c == '/' && peek(1) == '*') {
            advance();
            advance();
            while (peek() != '\0' && !(peek() == '*' && peek(1) == '/'))
                advance();
            if (peek() == '*') {
                advance();
                advance();
            }
            continue;
        }
        return;
    }
}

/** @brief Constructs a Token spanning from the given start position to the current lexer position.
 *  @param kind         The token kind.
 *  @param begin_offset The source offset where the token begins.
 *  @param begin_line   The line number where the token begins.
 *  @param begin_column The column number where the token begins.
 *  @return The constructed Token with the appropriate source range. */
Token Lexer::make_token(TokenKind kind, size_t begin_offset, unsigned begin_line, unsigned begin_column) const {
    return Token(kind, String(source_ + begin_offset, offset_ - begin_offset),
                 source_manager_.range(begin_line, begin_column, line_, column_));
}

/** @brief Lexes an identifier or keyword token starting at the given position.
 *  @param begin_offset The source offset where the identifier begins.
 *  @param begin_line   The line number where the identifier begins.
 *  @param begin_column The column number where the identifier begins.
 *  @return The token with TokenKind::Identifier or the matching keyword kind, or an Error. */
Expected<Token, Error> Lexer::lex_identifier_or_keyword(size_t begin_offset, unsigned begin_line,
                                                        unsigned begin_column) {
    while (::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        advance();
    String spelling(source_ + begin_offset, offset_ - begin_offset);
    TokenKind kind = TokenKind::Identifier;
    if (spelling == "int")
        kind = TokenKind::KeywordInt;
    else if (spelling == "void")
        kind = TokenKind::KeywordVoid;
    else if (spelling == "return")
        kind = TokenKind::KeywordReturn;
    else if (spelling == "if")
        kind = TokenKind::KeywordIf;
    else if (spelling == "else")
        kind = TokenKind::KeywordElse;
    else if (spelling == "while")
        kind = TokenKind::KeywordWhile;
    else if (spelling == "for")
        kind = TokenKind::KeywordFor;
    else if (spelling == "bool")
        kind = TokenKind::KeywordBool;
    else if (spelling == "true")
        kind = TokenKind::KeywordTrue;
    else if (spelling == "false")
        kind = TokenKind::KeywordFalse;
    else if (spelling == "class")
        kind = TokenKind::KeywordClass;
    else if (spelling == "struct")
        kind = TokenKind::KeywordStruct;
    else if (spelling == "template")
        kind = TokenKind::KeywordTemplate;
    else if (spelling == "typename")
        kind = TokenKind::KeywordTypename;
    return Token(kind, static_cast<String&&>(spelling),
                 source_manager_.range(begin_line, begin_column, line_, column_));
}

/** @brief Lexes a numeric (integer) literal token starting at the given position.
 *  @param begin_offset The source offset where the number begins.
 *  @param begin_line   The line number where the number begins.
 *  @param begin_column The column number where the number begins.
 *  @return The integer literal token. */
Expected<Token, Error> Lexer::lex_number(size_t begin_offset, unsigned begin_line, unsigned begin_column) {
    while (::isdigit(static_cast<unsigned char>(peek())))
        advance();
    return make_token(TokenKind::IntegerLiteral, begin_offset, begin_line, begin_column);
}

/** @brief Lexes a string literal token starting at the given position.
 *  @param begin_offset The source offset where the opening quote is located.
 *  @param begin_line   The line number where the string begins.
 *  @param begin_column The column number where the string begins.
 *  @return The string literal token, or an Error if the string is unterminated. */
Expected<Token, Error> Lexer::lex_string(size_t begin_offset, unsigned begin_line, unsigned begin_column) {
    while (peek() != '\0' && peek() != '"' && peek() != '\n')
        advance();
    if (peek() != '"') {
        auto loc = source_manager_.location(begin_line, begin_column);
        auto error = make_error("unterminated string literal", loc);
        diag_.report(DiagLevel::Error, error.message(), loc);
        return Unexpected<Error>(static_cast<Error&&>(error));
    }
    advance();
    return make_token(TokenKind::StringLiteral, begin_offset, begin_line, begin_column);
}

/** @brief Lexes a character literal token starting at the given position.
 *  @param begin_offset The source offset where the opening quote is located.
 *  @param begin_line   The line number where the character literal begins.
 *  @param begin_column The column number where the character literal begins.
 *  @return The character literal token, or an Error if the literal is malformed. */
Expected<Token, Error> Lexer::lex_char(size_t begin_offset, unsigned begin_line, unsigned begin_column) {
    if (peek() == '\0' || peek() == '\n') {
        auto loc = source_manager_.location(begin_line, begin_column);
        auto error = make_error("unterminated character literal", loc);
        diag_.report(DiagLevel::Error, error.message(), loc);
        return Unexpected<Error>(static_cast<Error&&>(error));
    }
    advance();
    if (peek() != '\'') {
        auto loc = source_manager_.location(begin_line, begin_column);
        auto error = make_error("expected closing quote for character literal", loc);
        diag_.report(DiagLevel::Error, error.message(), loc);
        return Unexpected<Error>(static_cast<Error&&>(error));
    }
    advance();
    return make_token(TokenKind::CharLiteral, begin_offset, begin_line, begin_column);
}

/** @brief Rejects a legacy or unsupported token (trigraph/digraph) and reports an error.
 *  @param begin_offset The source offset where the token begins.
 *  @param begin_line   The line number where the token begins.
 *  @param begin_column The column number where the token begins.
 *  @param message      The error message to report.
 *  @return An Error wrapped in Unexpected indicating the rejection. */
Expected<Token, Error> Lexer::reject_legacy_token(size_t, unsigned begin_line, unsigned begin_column,
                                                  const char* message) {
    auto loc = source_manager_.location(begin_line, begin_column);
    auto error = make_error(message, loc);
    diag_.report(DiagLevel::Error, error.message(), loc);
    return Unexpected<Error>(static_cast<Error&&>(error));
}

/** @brief Returns the human-readable name of the given token kind.
 *  @param kind The token kind to look up.
 *  @return A C-string representing the name of the token kind. */
const char* token_kind_name(TokenKind kind) {
    switch (kind) {
    case TokenKind::EndOfFile:
        return "end of file";
    case TokenKind::Identifier:
        return "identifier";
    case TokenKind::IntegerLiteral:
        return "integer literal";
    case TokenKind::StringLiteral:
        return "string literal";
    case TokenKind::CharLiteral:
        return "character literal";
    case TokenKind::KeywordInt:
        return "int";
    case TokenKind::KeywordVoid:
        return "void";
    case TokenKind::KeywordReturn:
        return "return";
    case TokenKind::KeywordIf:
        return "if";
    case TokenKind::KeywordElse:
        return "else";
    case TokenKind::KeywordWhile:
        return "while";
    case TokenKind::KeywordFor:
        return "for";
    case TokenKind::KeywordBool:
        return "bool";
    case TokenKind::KeywordTrue:
        return "true";
    case TokenKind::KeywordFalse:
        return "false";
    case TokenKind::KeywordClass:
        return "class";
    case TokenKind::KeywordStruct:
        return "struct";
    case TokenKind::KeywordTemplate:
        return "template";
    case TokenKind::KeywordTypename:
        return "typename";
    case TokenKind::LParen:
        return "(";
    case TokenKind::RParen:
        return ")";
    case TokenKind::LBrace:
        return "{";
    case TokenKind::RBrace:
        return "}";
    case TokenKind::Comma:
        return ",";
    case TokenKind::Semicolon:
        return ";";
    case TokenKind::Plus:
        return "+";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "/";
    case TokenKind::Percent:
        return "%";
    case TokenKind::Amp:
        return "&";
    case TokenKind::Equal:
        return "=";
    case TokenKind::EqualEqual:
        return "==";
    case TokenKind::Tilde:
        return "~";
    case TokenKind::Bang:
        return "!";
    case TokenKind::BangEqual:
        return "!=";
    case TokenKind::Less:
        return "<";
    case TokenKind::LessEqual:
        return "<=";
    case TokenKind::Greater:
        return ">";
    case TokenKind::GreaterEqual:
        return ">=";
    }
    return "token";
}

}  // namespace prism::frontend
