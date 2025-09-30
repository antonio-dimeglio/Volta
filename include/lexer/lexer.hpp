#ifndef VOLTA_LEXER_HPP
#define VOLTA_LEXER_HPP

#include "token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace volta {

class Lexer {
public:
    explicit Lexer(std::string source);

    // Tokenize the entire source
    std::vector<Token> tokenize();

    // Scan a single token
    Token scanToken();

private:
    std::string source_;
    size_t idx_;
    int line_;
    int column_;

    // Helper methods
    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    std::optional<char> currentChar() const;
    std::optional<char> advance();

    void skipWhitespace();
    void skipComment();

    // Token scanning methods
    Token scanNumber();
    Token scanString();
    Token scanIdentifierOrKeyword();
    Token scanSymbol();

    // Character classification
    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphanumeric(char c);
};

} // namespace volta

#endif // VOLTA_LEXER_HPP