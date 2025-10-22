#pragma once
#include "Token.hpp"
#include "Error/Error.hpp"
#include <vector>

class Lexer {
private:
    std::string source;
    size_t len;
    size_t idx;
    size_t line;
    size_t column;
    DiagnosticManager& diag;

public:
    Lexer(std::string const& source, DiagnosticManager& diag) :
        source(source), len(source.size()),
        idx(0), line(1), column(1), diag(diag) {}

    std::vector<Token> tokenize();

private:
    [[nodiscard]] bool isAtEnd() const { return idx >= len; }
    [[nodiscard]] char peek(size_t i = 0) const;
    char advance();
    void skipWhitespace();
    Token scanToken();
    Token scanNumber();
    Token scanRawString();
    Token scanIdentifier();
    Token scanStringLiteral();
    Token scanSymbol();
};