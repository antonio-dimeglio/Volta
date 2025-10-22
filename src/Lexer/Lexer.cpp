#include "Lexer/Lexer.hpp"

char Lexer::peek(size_t i) const {
    if (idx + i >= len) {
        return '\0';
    }
    return source[idx + i];
}

char Lexer::advance() {
    char const ch = peek();
    if (ch == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    idx++;
    return ch;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) { 
            break;
        }
        tokens.push_back(scanToken());
    }

    tokens.emplace_back(TokenType::EndOfFile, line, column);
    return tokens;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char const ch = peek();
        if (isspace(ch) != 0) { 
            advance();
        } else { 
            break;
        }
    }
}

Token Lexer::scanToken() {
    char const ch = peek();

    if (isdigit(ch) != 0) { 
        return scanNumber();
    } else if (ch == 'r' && peek(1) == '"') {
        return scanRawString();
    } else if (ch == '_' || (isalpha(ch) != 0)) {
        return scanIdentifier();
    } else if (ch == '"') {
        return scanStringLiteral();
    } else {
        return scanSymbol();
    }
}

Token Lexer::scanNumber() {
    size_t const currLine = line;
    size_t const currCol = column;
    bool dot = false;
    std::string lexeme;

    while (!isAtEnd()) {
        char const ch = peek();
        if (isdigit(ch) != 0) {
            lexeme += ch;
            advance();
        }
        else if (ch == '.') {
            if (isdigit(peek(1)) != 0) {
                if (dot) {
                    diag.error("Multiple decimal points in number literal", line, column);
                    // Continue parsing to recover
                }
                dot = true;
                lexeme += ch;
                advance();
            } else {
                break;
            }
        } else {
            break;
        }
    }

    // Determine token type based on presence of decimal point
    TokenType const type = dot ? TokenType::Float : TokenType::Integer;
    return {type, currLine, currCol, lexeme};
}

Token Lexer::scanRawString() {
    size_t const currLine = line;
    size_t const currCol = column;
    std::string lexeme;

    advance();
    advance();
    
    while (!isAtEnd()) {
        char const ch = peek();
        if (ch == '"') {
            advance();
            return {TokenType::String, currLine, currCol, lexeme};
        }
        lexeme += ch;
        advance();
    }

    diag.error("Unterminated raw string", currLine, currCol);
    return {TokenType::Dummy, currLine, currCol};
} 


Token Lexer::scanIdentifier() {
    size_t const currLine = line;
    size_t const currCol = column;
    std::string lexeme;
    
    while (!isAtEnd()) {
        char const ch = peek();
        if ((isalnum(ch) != 0) || ch == '_') {
            lexeme += ch;
            advance();
        } else {
            break;
        }
    }

    return fromKeyword(lexeme, currLine, currCol);
}

Token Lexer::scanStringLiteral() {
    size_t const currLine = line;
    size_t const currCol = column;
    std::string lexeme;

    advance();

    while (!isAtEnd()) {
        char const ch = peek();

        if (ch == '\\') {
            lexeme += ch; 
            advance();

            if (!isAtEnd()) {
                lexeme += peek();
                advance();
            }
        } else if (ch == '"') {
            advance();
            return {TokenType::String, currLine, currCol, lexeme};
        } else {
            lexeme += ch;
            advance();
        }
    }

    diag.error("Unterminated string", currLine, currCol);
    return {TokenType::Dummy, currLine, currCol};
}

Token Lexer::scanSymbol() {
    size_t const tokLine = line;
    size_t const tokColumn = column;

    char const ch = advance();

    switch (ch) {
        case '+':
            if (peek() == '=') {
                advance();
                return {TokenType::PlusEqual, tokLine, tokColumn};
            } else if (peek() == '+') {
                advance();
                return {TokenType::Increment, tokLine, tokColumn};
            }
            return {TokenType::Plus, tokLine, tokColumn};

        case '-':
            if (peek() == '=') {
                advance();
                return {TokenType::MinusEqual, tokLine, tokColumn};
            } else if (peek() == '>') {
                advance();
                return {TokenType::Arrow, tokLine, tokColumn};
            } else if (peek() == '-') {
                advance();
                return {TokenType::Decrement, tokLine, tokColumn};
            }
            return {TokenType::Minus, tokLine, tokColumn};

        case '*':
            if (peek() == '=') {
                advance();
                return {TokenType::MultEqual, tokLine, tokColumn};
            }
            return {TokenType::Mult, tokLine, tokColumn};

        case '/':
            if (peek() == '/') {
                advance(); // consume second '/'
                while (!isAtEnd() && peek() != '\n') { advance();
}
                skipWhitespace();
                if (isAtEnd()) { return {TokenType::EndOfFile, line, column};
}
                return scanToken();
            } else if (peek() == '*') {
                advance(); // consume '*'
                while (!isAtEnd()) {
                    if (peek() == '*' && peek(1) == '/') {
                        advance();
                        advance();
                        break;
                    }
                    advance();
                }
                skipWhitespace();
                if (isAtEnd()) { return {TokenType::EndOfFile, line, column};
}
                return scanToken();
            } else if (peek() == '=') {
                advance();
                return {TokenType::DivEqual, tokLine, tokColumn};
            }
            return {TokenType::Div, tokLine, tokColumn};

        case '%':
            if (peek() == '=') {
                advance();
                return {TokenType::ModuloEqual, tokLine, tokColumn};
            }
            return {TokenType::Modulo, tokLine, tokColumn};

        case '!':
            if (peek() == '=') {
                advance();
                return {TokenType::NotEqual, tokLine, tokColumn};
            } else {
                diag.error("Unexpected symbol '!'", tokLine, tokColumn);
            }
            break;

        case '=':
            if (peek() == '=') {
                advance();
                return {TokenType::EqualEqual, tokLine, tokColumn};
            } else if (peek() == '>') {
                advance();
                return {TokenType::FatArrow, tokLine, tokColumn};
            }
            return {TokenType::Assign, tokLine, tokColumn};

        case ':':
            if (peek() == '=') {
                advance();
                return {TokenType::InferAssign, tokLine, tokColumn};
            }
            if (peek() == ':') {
                advance();
                return {TokenType::DoubleColon, tokLine, tokColumn};
            }
            return {TokenType::Colon, tokLine, tokColumn};

        case '>':
            if (peek() == '=') {
                advance();
                return {TokenType::GreaterEqual, tokLine, tokColumn};
            }
            return {TokenType::GreaterThan, tokLine, tokColumn};

        case '<':
            if (peek() == '=') {
                advance();
                return {TokenType::LessEqual, tokLine, tokColumn};
            }
            return {TokenType::LessThan, tokLine, tokColumn};

        case '.':
            if (peek() == '.') {
                if (peek(1) == '=') {
                    advance();
                    advance();
                    return {TokenType::InclusiveRange, tokLine, tokColumn};
                }
                advance();
                return {TokenType::Range, tokLine, tokColumn};
            }
            return {TokenType::Dot, tokLine, tokColumn};

        case ';': return {TokenType::Semicolon, tokLine, tokColumn};
        case ',': return {TokenType::Comma, tokLine, tokColumn};
        case '(': return {TokenType::LParen, tokLine, tokColumn};
        case ')': return {TokenType::RParen, tokLine, tokColumn};
        case '{': return {TokenType::LBrace, tokLine, tokColumn};
        case '}': return {TokenType::RBrace, tokLine, tokColumn};
        case '[': return {TokenType::LSquare, tokLine, tokColumn};
        case ']': return {TokenType::RSquare, tokLine, tokColumn};

        default:
            diag.error(std::string("Unexpected character: '") + ch + "'", tokLine, tokColumn);
            break;
    }

    return {TokenType::Dummy, 0, 0};
}
