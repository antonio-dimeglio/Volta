#include "Lexer/Lexer.hpp"

char Lexer::peek(size_t i) const {
    if (idx + i >= len)
        return '\0';
    return source[idx + i];
}

char Lexer::advance() {
    char ch = peek();
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
        if (isAtEnd()) 
            break;
        tokens.push_back(scanToken());
    }

    tokens.push_back(Token(TokenType::EndOfFile, line, column));
    return tokens;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char ch = peek();
        if (isspace(ch)) 
            advance();
        else 
            break;
    }
}

Token Lexer::scanToken() {
    char ch = peek();

    if (isdigit(ch)) 
        return scanNumber();
    else if (ch == 'r' && peek(1) == '"')
        return scanRawString();
    else if (ch == '_' || isalpha(ch))
        return scanIdentifier();
    else if (ch == '"')
        return scanStringLiteral();
    else
        return scanSymbol();
}

Token Lexer::scanNumber() {
    size_t currLine = line;
    size_t currCol = column;
    bool dot = false;
    std::string lexeme = "";

    while (!isAtEnd()) {
        char ch = peek();
        if (isdigit(ch)) {
            lexeme += ch;
            advance();
        }
        else if (ch == '.') {
            if (isdigit(peek(1))) {
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
    TokenType type = dot ? TokenType::Float : TokenType::Integer;
    return Token(type, currLine, currCol, lexeme);
}

Token Lexer::scanRawString() {
    size_t currLine = line;
    size_t currCol = column;
    std::string lexeme = "";

    advance();
    advance();
    
    while (!isAtEnd()) {
        char ch = peek();
        if (ch == '"') {
            advance();
            return Token(TokenType::String, currLine, currCol, lexeme);
        }
        lexeme += ch;
        advance();
    }

    diag.error("Unterminated raw string", currLine, currCol);
    return Token(TokenType::Dummy, currLine, currCol);
} 


Token Lexer::scanIdentifier() {
    size_t currLine = line;
    size_t currCol = column;
    std::string lexeme = "";
    
    while (!isAtEnd()) {
        char ch = peek();
        if (isalnum(ch) || ch == '_') {
            lexeme += ch;
            advance();
        } else {
            break;
        }
    }

    return fromKeyword(lexeme, currLine, currCol);
}

Token Lexer::scanStringLiteral() {
    size_t currLine = line;
    size_t currCol = column;
    std::string lexeme = "";

    advance();

    while (!isAtEnd()) {
        char ch = peek();

        if (ch == '\\') {
            lexeme += ch; 
            advance();

            if (!isAtEnd()) {
                lexeme += peek();
                advance();
            }
        } else if (ch == '"') {
            advance();
            return Token(TokenType::String, currLine, currCol, lexeme);
        } else {
            lexeme += ch;
            advance();
        }
    }

    diag.error("Unterminated string", currLine, currCol);
    return Token(TokenType::Dummy, currLine, currCol);
}

Token Lexer::scanSymbol() {
    size_t tokLine = line;
    size_t tokColumn = column;

    char ch = advance();

    switch (ch) {
        case '+':
            if (peek() == '=') {
                advance();
                return Token(TokenType::PlusEqual, tokLine, tokColumn);
            } else if (peek() == '+') {
                advance();
                return Token(TokenType::Increment, tokLine, tokColumn);
            }
            return Token(TokenType::Plus, tokLine, tokColumn);

        case '-':
            if (peek() == '=') {
                advance();
                return Token(TokenType::MinusEqual, tokLine, tokColumn);
            } else if (peek() == '>') {
                advance();
                return Token(TokenType::Arrow, tokLine, tokColumn);
            } else if (peek() == '-') {
                advance();
                return Token(TokenType::Decrement, tokLine, tokColumn);
            }
            return Token(TokenType::Minus, tokLine, tokColumn);

        case '*':
            if (peek() == '=') {
                advance();
                return Token(TokenType::MultEqual, tokLine, tokColumn);
            }
            return Token(TokenType::Mult, tokLine, tokColumn);

        case '/':
            if (peek() == '/') {
                advance(); // consume second '/'
                while (!isAtEnd() && peek() != '\n') advance();
                skipWhitespace();
                if (isAtEnd()) return Token(TokenType::EndOfFile, line, column);
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
                if (isAtEnd()) return Token(TokenType::EndOfFile, line, column);
                return scanToken();
            } else if (peek() == '=') {
                advance();
                return Token(TokenType::DivEqual, tokLine, tokColumn);
            }
            return Token(TokenType::Div, tokLine, tokColumn);

        case '%':
            if (peek() == '=') {
                advance();
                return Token(TokenType::ModuloEqual, tokLine, tokColumn);
            }
            return Token(TokenType::Modulo, tokLine, tokColumn);

        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::NotEqual, tokLine, tokColumn);
            } else {
                diag.error("Unexpected symbol '!'", tokLine, tokColumn);
            }
            break;

        case '=':
            if (peek() == '=') {
                advance();
                return Token(TokenType::EqualEqual, tokLine, tokColumn);
            } else if (peek() == '>') {
                advance();
                return Token(TokenType::FatArrow, tokLine, tokColumn);
            }
            return Token(TokenType::Assign, tokLine, tokColumn);

        case ':':
            if (peek() == '=') {
                advance();
                return Token(TokenType::InferAssign, tokLine, tokColumn);
            }
            if (peek() == ':') {
                advance();
                return Token(TokenType::DoubleColon, tokLine, tokColumn);
            }
            return Token(TokenType::Colon, tokLine, tokColumn);

        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GreaterEqual, tokLine, tokColumn);
            }
            return Token(TokenType::GreaterThan, tokLine, tokColumn);

        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LessEqual, tokLine, tokColumn);
            }
            return Token(TokenType::LessThan, tokLine, tokColumn);

        case '.':
            if (peek() == '.') {
                if (peek(1) == '=') {
                    advance();
                    advance();
                    return Token(TokenType::InclusiveRange, tokLine, tokColumn);
                }
                advance();
                return Token(TokenType::Range, tokLine, tokColumn);
            }
            return Token(TokenType::Dot, tokLine, tokColumn);

        case ';': return Token(TokenType::Semicolon, tokLine, tokColumn);
        case ',': return Token(TokenType::Comma, tokLine, tokColumn);
        case '(': return Token(TokenType::LParen, tokLine, tokColumn);
        case ')': return Token(TokenType::RParen, tokLine, tokColumn);
        case '{': return Token(TokenType::LBrace, tokLine, tokColumn);
        case '}': return Token(TokenType::RBrace, tokLine, tokColumn);
        case '[': return Token(TokenType::LSquare, tokLine, tokColumn);
        case ']': return Token(TokenType::RSquare, tokLine, tokColumn);

        default:
            diag.error(std::string("Unexpected character: '") + ch + "'", tokLine, tokColumn);
            break;
    }

    return Token(TokenType::Dummy, -1, -1);
}
