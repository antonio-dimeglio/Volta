#include "Lexer/Token.hpp"
#include <unordered_map>


static const std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::Let},
    {"mut", TokenType::Mut},
    {"ref", TokenType::Ref},
    {"fn", TokenType::Function},
    {"return", TokenType::Return},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"in", TokenType::In},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"match", TokenType::Match},
    {"struct", TokenType::Struct},
    {"import", TokenType::Import},
    {"as", TokenType::As},
    {"type", TokenType::Type},
    {"enum", TokenType::Enum},
    {"extern", TokenType::Extern},
    {"opaque", TokenType::Opaque},
    {"addrof", TokenType::PtrKeyword},
    {"self", TokenType::Self_},
    {"impl", TokenType::Impl},
    {"varargs", TokenType::Varargs},
    {"true", TokenType::True_},
    {"false", TokenType::False_},
    {"and", TokenType::And},
    {"or", TokenType::Or},
    {"not", TokenType::Not},
};

Token fromKeyword(const std::string& word, size_t line, size_t column) {
    auto it = keywords.find(word);
    if (it != keywords.end())
        return Token(it->second, line, column, word);
    return Token(TokenType::Identifier, line, column, word);
}
