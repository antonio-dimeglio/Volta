#pragma once
#include "TokenType.hpp"
#include <cstddef>
#include <string>
#include <iomanip>

struct Token {
    TokenType tt;
    size_t line;
    size_t column;
    std::string lexeme;

    Token(TokenType tt, size_t line, size_t column, std::string lexeme) :
        tt(tt), line(line), column(column), lexeme(lexeme) {}

    Token(TokenType tt, size_t line, size_t column) :
        tt(tt), line(line), column(column), lexeme("") {}

    std::string toString() const {
    std::ostringstream oss;
        constexpr int width = 15; // fixed width for token type

        oss << '[' << std::left << std::setw(width) << tokenTypeToString(tt) << ']'
            << " line=" << line
            << " col=" << column;

        if (!lexeme.empty())
            oss << " lexeme=\"" << lexeme << '"';

        return oss.str();
    }
};

Token fromKeyword(const std::string& word, size_t line, size_t column);