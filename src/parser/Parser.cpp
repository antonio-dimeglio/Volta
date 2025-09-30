#include "parser/Parser.hpp"
#include <stdexcept>

using namespace volta::ast;
using namespace volta::lexer;

namespace volta::parser {

Parser::Parser(std::vector<volta::lexer::Token> tokens)
    : tokens_(std::move(tokens)), current_(0) {}

// Main parsing entry point
std::unique_ptr<volta::ast::Program> Parser::parseProgram() {
    std::vector<std::unique_ptr<Statement>> statements; 
    while (!isAtEnd()) {
        statements.push_back(parseStatement());
    }

    return std::make_unique<Program>(std::move(statements), currentLocation());
}

// Statement parsing
std::unique_ptr<volta::ast::Statement> Parser::parseStatement() {
    if (match(TokenType::IF)) return parseIfStatement();
    if (match(TokenType::WHILE)) return parseWhileStatement();
    if (match(TokenType::FOR)) return parseForStatement();
    if (match(TokenType::RETURN)) return parseReturnStatement();
    if (match(TokenType::IMPORT)) return parseImportStatement();
    if (match(TokenType::FUNCTION)) return parseFnDeclaration();
    if (match(TokenType::STRUCT)) return parseStructDeclaration();
    if (check(TokenType::IDENTIFIER)) {
        size_t saved = current_;
        advance();

        if (check(TokenType::INFER_ASSIGN) || 
            check(TokenType::COLON)) {
            current_ = saved;
            return parseVarDeclaration();
        }

        current_ = saved; 
    }

    return parseExpressionStatement();
}

std::unique_ptr<volta::ast::Block> Parser::parseBlock() {
    auto startLoc = currentLocation();
    consume(TokenType::LBRACE, "Missing '{' for block definition");
    
    std::vector<std::unique_ptr<Statement>> statements; 

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }

    consume(TokenType::RBRACE, "Missing '}' at end of block");
    
    return std::make_unique<Block>(std::move(statements), startLoc);

}

std::unique_ptr<volta::ast::ExpressionStatement> Parser::parseExpressionStatement() {
    auto startLoc = currentLocation();
    auto expr = parseExpression();
    return std::make_unique<ExpressionStatement>(std::move(expr), startLoc);
}

std::unique_ptr<volta::ast::IfStatement> Parser::parseIfStatement() {
    auto startLoc = currentLocation();

    auto condition = parseExpression();

    auto thenBlock = parseBlock();

    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Block>>> elseIfClauses;
    while (check(TokenType::ELSE)) {
        auto elsePos = current_;
        advance();  // consume 'else'

        if (match(TokenType::IF)) {
            auto elseIfCond = parseExpression();
            auto elseIfBlock = parseBlock();
            elseIfClauses.push_back({std::move(elseIfCond), std::move(elseIfBlock)});
        } else {
            // Put back the 'else' and break to handle it below
            current_ = elsePos;
            break;
        }
    }

    std::unique_ptr<Block> elseBlock = nullptr;
    if (match(TokenType::ELSE)) {
        elseBlock = parseBlock();
    }

    return std::make_unique<IfStatement>(
        std::move(condition),
        std::move(thenBlock),
        std::move(elseIfClauses),
        std::move(elseBlock),
        startLoc
    );
}

std::unique_ptr<volta::ast::WhileStatement> Parser::parseWhileStatement() {
    auto startLoc = currentLocation();

    auto condition = parseExpression();

    auto thenBlock = parseBlock();

    return std::make_unique<WhileStatement>(
        std::move(condition),
        std::move(thenBlock),
        startLoc
    );
}

std::unique_ptr<volta::ast::ForStatement> Parser::parseForStatement() {
    auto startLoc = currentLocation();

    auto identToken = consume(TokenType::IDENTIFIER, "Expected identifier in for loop");
    std::string loopVar = identToken.lexeme;

    consume(TokenType::IN, "Expected 'in' after loop variable");

    auto iterable = parseExpression();

    auto body = parseBlock();

    return std::make_unique<ForStatement>(
        loopVar,
        std::move(iterable),
        std::move(body),
        startLoc
    );
}

std::unique_ptr<volta::ast::ReturnStatement> Parser::parseReturnStatement() {
    auto startLoc = currentLocation();

    if (check(TokenType::RBRACE)) {
        return std::make_unique<ReturnStatement>(
            nullptr,
            startLoc
        );
    } else {
        auto returnExpr = parseExpression();
        return std::make_unique<ReturnStatement>(
            std::move(returnExpr),
            startLoc
        );
    }
}

std::unique_ptr<volta::ast::ImportStatement> Parser::parseImportStatement() {
    auto startLoc = currentLocation();
    auto identifierToken = consume(TokenType::IDENTIFIER, "Expected identifier after import statement");
    return std::make_unique<ImportStatement>(
        identifierToken.lexeme,
        startLoc
    );
}

// Declaration parsing
std::unique_ptr<volta::ast::VarDeclaration> Parser::parseVarDeclaration() {
    auto startLoc = currentLocation();
    auto varName = consume(TokenType::IDENTIFIER, "Expected identifier for variable declaration").lexeme;

    if (peek().type == TokenType::INFER_ASSIGN) {
        advance();
        auto initializer = parseExpression();

        return std::make_unique<VarDeclaration>(
            varName,
            nullptr,
            std::move(initializer),
            startLoc
        );
    } else {
        consume(TokenType::COLON, "Expected either infer-assign operator or type definition after variable.");
        auto typeAnnotation = parseTypeAnnotation();
        consume(TokenType::ASSIGN, "Expected '=' after type");
        auto initializer = parseExpression();

        return std::make_unique<VarDeclaration>(
            varName,
            std::move(typeAnnotation),
            std::move(initializer),
            startLoc
        );
    }
}

std::unique_ptr<volta::ast::FnDeclaration> Parser::parseFnDeclaration() {
    auto startLoc = currentLocation();
    auto funcName = consume(TokenType::IDENTIFIER, "Expected identifier for function declaration").lexeme;
    
    std::vector<std::string> typeParams;
    
    std::vector<std::unique_ptr<Parameter>> params;
    consume(TokenType::LPAREN, "Expected '(' for function definition.");

    while (!check(TokenType::RPAREN)) {
        auto var = consume(TokenType::IDENTIFIER, "Expected identifier.").lexeme;
        consume(TokenType::COLON, "Expected ':' for parameter type.");
        auto type = parseType();
        
        // Wrap in unique_ptr
        params.push_back(std::make_unique<Parameter>(var, std::move(type)));
        
        if (!check(TokenType::RPAREN)) {
            consume(TokenType::COMMA, "Expected ',' separator in parameter list.");
        }
    }

    consume(TokenType::RPAREN, "Expected ')' after parameters.");
    consume(TokenType::ARROW, "Expected '->' after function parameters.");
    auto returnType = parseType();

    std::unique_ptr<Block> body = nullptr;
    std::unique_ptr<Expression> expressionBody = nullptr;

    // Check for expression body: fn foo() -> int = expr
    if (match(TokenType::ASSIGN)) {
        expressionBody = parseExpression();
    } else {
        // Regular block body: fn foo() -> int { ... }
        body = parseBlock();
    }

    return std::make_unique<FnDeclaration>(
        funcName,
        std::move(typeParams),
        std::move(params),
        std::move(returnType),
        std::move(body),
        std::move(expressionBody),
        startLoc
    );
}

std::unique_ptr<volta::ast::StructDeclaration> Parser::parseStructDeclaration() {
    auto startLoc = currentLocation();
    auto structName = consume(TokenType::IDENTIFIER, "Expected struct name after struct keyword.").lexeme;

    consume(TokenType::LBRACE, "Expected '}' after struct name.");
    std::vector<std::unique_ptr<StructField>> structFields;

    while (!check(TokenType::RBRACE)) {
        auto fieldIdentifier = consume(TokenType::IDENTIFIER, "Expected identifier for struct field definition");
        consume(TokenType::COLON, "Expected ':' after variable name.");
        auto fieldType = parseType();
        structFields.push_back(
            std::make_unique<StructField>(fieldIdentifier.lexeme, std::move(fieldType))
        );
        if (!check(TokenType::RBRACE)) {
            consume(TokenType::COMMA, "Expected ',' between fields");
        }
    }
    consume(TokenType::RBRACE, "Expected '}' after struct fields");

    return std::make_unique<StructDeclaration>(
        structName,
        std::move(structFields),
        startLoc
    );
}

// Expression parsing
std::unique_ptr<volta::ast::Expression> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<volta::ast::Expression> Parser::parseAssignment() {
    auto expr = parseLogicalOr();

    if (match({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN,
               TokenType::MULT_ASSIGN, TokenType::DIV_ASSIGN})) {
        auto op = previous();
        auto value = parseAssignment(); // Right-associative

        // Convert token type to operator
        BinaryExpression::Operator binOp;
        switch (op.type) {
            case TokenType::ASSIGN: binOp = BinaryExpression::Operator::Assign; break;
            case TokenType::PLUS_ASSIGN: binOp = BinaryExpression::Operator::AddAssign; break;
            case TokenType::MINUS_ASSIGN: binOp = BinaryExpression::Operator::SubtractAssign; break;
            case TokenType::MULT_ASSIGN: binOp = BinaryExpression::Operator::MultiplyAssign; break;
            case TokenType::DIV_ASSIGN: binOp = BinaryExpression::Operator::DivideAssign; break;
            default: throw std::runtime_error("Invalid assignment operator");
        }

        return std::make_unique<BinaryExpression>(
            std::move(expr),
            binOp,
            std::move(value),
            currentLocation()
        );
    }

    return expr;
}

std::unique_ptr<volta::ast::Expression> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (match(TokenType::OR)) {
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpression>(
            std::move(left),
            BinaryExpression::Operator::LogicalOr,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseLogicalAnd() {
    auto left = parseEquality();

    while (match(TokenType::AND)) {
        auto right = parseEquality();
        left = std::make_unique<BinaryExpression>(
            std::move(left),
            BinaryExpression::Operator::LogicalAnd,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseEquality() {
    auto left = parseComparison();

    while (match({TokenType::EQUALS, TokenType::NOT_EQUALS})) {
        auto op = previous();
        auto right = parseComparison();

        BinaryExpression::Operator binOp =
            (op.type == TokenType::EQUALS) ?
            BinaryExpression::Operator::Equal :
            BinaryExpression::Operator::NotEqual;

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            binOp,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseComparison() {
    auto left = parseRange();

    while (match({TokenType::LT, TokenType::GT, TokenType::LEQ, TokenType::GEQ})) {
        auto op = previous();
        auto right = parseRange();

        BinaryExpression::Operator binOp;
        switch (op.type) {
            case TokenType::LT: binOp = BinaryExpression::Operator::Less; break;
            case TokenType::GT: binOp = BinaryExpression::Operator::Greater; break;
            case TokenType::LEQ: binOp = BinaryExpression::Operator::LessEqual; break;
            case TokenType::GEQ: binOp = BinaryExpression::Operator::GreaterEqual; break;
            default: throw std::runtime_error("Invalid comparison operator");
        }

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            binOp,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseRange() {
    auto left = parseTerm();

    if (match({TokenType::RANGE, TokenType::INCLUSIVE_RANGE})) {
        auto op = previous();
        auto right = parseTerm();

        BinaryExpression::Operator binOp =
            (op.type == TokenType::RANGE) ?
            BinaryExpression::Operator::Range :
            BinaryExpression::Operator::RangeInclusive;

        return std::make_unique<BinaryExpression>(
            std::move(left),
            binOp,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseTerm() {
    auto left = parseFactor();

    while (match({TokenType::PLUS, TokenType::MINUS})) {
        auto op = previous();
        auto right = parseFactor();

        BinaryExpression::Operator binOp =
            (op.type == TokenType::PLUS) ?
            BinaryExpression::Operator::Add :
            BinaryExpression::Operator::Subtract;

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            binOp,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseFactor() {
    auto left = parseUnary();

    while (match({TokenType::MULT, TokenType::DIV, TokenType::MODULO})) {
        auto op = previous();
        auto right = parseUnary();

        BinaryExpression::Operator binOp;
        switch (op.type) {
            case TokenType::MULT: binOp = BinaryExpression::Operator::Multiply; break;
            case TokenType::DIV: binOp = BinaryExpression::Operator::Divide; break;
            case TokenType::MODULO: binOp = BinaryExpression::Operator::Modulo; break;
            default: throw std::runtime_error("Invalid factor operator");
        }

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            binOp,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseUnary() {
    if (match({TokenType::NOT, TokenType::MINUS})) {
        auto op = previous();
        auto operand = parseUnary(); // Recursive for multiple unary operators

        UnaryExpression::Operator unaryOp =
            (op.type == TokenType::NOT) ?
            UnaryExpression::Operator::Not :
            UnaryExpression::Operator::Negate;

        return std::make_unique<UnaryExpression>(
            unaryOp,
            std::move(operand),
            currentLocation()
        );
    }

    return parsePower();
}

std::unique_ptr<volta::ast::Expression> Parser::parsePower() {
    auto left = parseCall();

    if (match(TokenType::POWER)) {
        auto right = parsePower(); // Right-associative
        return std::make_unique<BinaryExpression>(
            std::move(left),
            BinaryExpression::Operator::Power,
            std::move(right),
            currentLocation()
        );
    }

    return left;
}

std::unique_ptr<volta::ast::Expression> Parser::parseCall() {
    auto expr = parsePrimary();

    while (true) {
        if (match(TokenType::LPAREN)) {
            // Function call
            std::vector<std::unique_ptr<Expression>> args;

            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpression>(
                std::move(expr),
                std::move(args),
                currentLocation()
            );
        } else if (match(TokenType::DOT)) {
            // Member access or method call
            auto member = consume(TokenType::IDENTIFIER, "Expected member name after '.'");
            auto memberExpr = std::make_unique<IdentifierExpression>(member.lexeme, currentLocation());

            if (match(TokenType::LPAREN)) {
                // Method call
                std::vector<std::unique_ptr<Expression>> args;

                if (!check(TokenType::RPAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }

                consume(TokenType::RPAREN, "Expected ')' after arguments");
                expr = std::make_unique<MethodCallExpression>(
                    std::move(expr),
                    std::move(memberExpr),
                    std::move(args),
                    currentLocation()
                );
            } else {
                // Member access
                expr = std::make_unique<MemberExpression>(
                    std::move(expr),
                    std::move(memberExpr),
                    currentLocation()
                );
            }
        } else if (match(TokenType::LSQUARE)) {
            // Index or slice
            auto startLoc = currentLocation();

            if (check(TokenType::COLON)) {
                // [:end] slice
                advance();
                auto end = parseExpression();
                consume(TokenType::RSQUARE, "Expected ']' after slice");
                expr = std::make_unique<SliceExpression>(
                    std::move(expr),
                    nullptr,
                    std::move(end),
                    startLoc
                );
            } else {
                auto first = parseExpression();

                if (match(TokenType::COLON)) {
                    // [start:end] or [start:] slice
                    std::unique_ptr<Expression> end = nullptr;
                    if (!check(TokenType::RSQUARE)) {
                        end = parseExpression();
                    }
                    consume(TokenType::RSQUARE, "Expected ']' after slice");
                    expr = std::make_unique<SliceExpression>(
                        std::move(expr),
                        std::move(first),
                        std::move(end),
                        startLoc
                    );
                } else {
                    // [index] indexing
                    consume(TokenType::RSQUARE, "Expected ']' after index");
                    expr = std::make_unique<IndexExpression>(
                        std::move(expr),
                        std::move(first),
                        startLoc
                    );
                }
            }
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<volta::ast::Expression> Parser::parsePrimary() {
    auto loc = currentLocation();

    // Boolean literals
    if (match(TokenType::BOOLEAN_LITERAL)) {
        bool value = (previous().lexeme == "true");
        return std::make_unique<BooleanLiteral>(value, loc);
    }

    // None literal
    if (match(TokenType::NONE_KEYWORD)) {
        return std::make_unique<NoneLiteral>(loc);
    }

    // Some literal
    if (match(TokenType::SOME_KEYWORD)) {
        consume(TokenType::LPAREN, "Expected '(' after 'Some'");
        auto value = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after Some value");
        return std::make_unique<SomeLiteral>(std::move(value), loc);
    }

    // Number literals
    if (match(TokenType::INTEGER)) {
        int64_t value = std::stoll(previous().lexeme);
        return std::make_unique<IntegerLiteral>(value, loc);
    }

    if (match(TokenType::REAL)) {
        double value = std::stod(previous().lexeme);
        return std::make_unique<FloatLiteral>(value, loc);
    }

    // String literal
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<StringLiteral>(previous().lexeme, loc);
    }

    // Identifier
    if (match(TokenType::IDENTIFIER)) {
        auto name = previous().lexeme;

        // Check for struct literal: Point { x: 1, y: 2 }
        // Must be followed by { identifier : to disambiguate from blocks
        if (check(TokenType::LBRACE)) {
            // Lookahead to see if this is really a struct literal
            size_t saved = current_;
            advance(); // consume {

            bool isStructLiteral = false;
            if (check(TokenType::IDENTIFIER)) {
                advance(); // consume identifier
                if (check(TokenType::COLON)) {
                    isStructLiteral = true;
                }
            } else if (check(TokenType::RBRACE)) {
                // Empty braces - could be empty struct
                isStructLiteral = true;
            }

            current_ = saved; // restore position

            if (isStructLiteral) {
                return parseStructLiteral();
            }
        }

        return std::make_unique<IdentifierExpression>(name, loc);
    }

    // Grouping
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();

        // Check for tuple: (a, b, c)
        if (match(TokenType::COMMA)) {
            std::vector<std::unique_ptr<Expression>> elements;
            elements.push_back(std::move(expr));

            do {
                elements.push_back(parseExpression());
            } while (match(TokenType::COMMA));

            consume(TokenType::RPAREN, "Expected ')' after tuple elements");
            return std::make_unique<TupleLiteral>(std::move(elements), loc);
        }

        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    // Array literal
    if (match(TokenType::LSQUARE)) {
        return parseArrayLiteral();
    }

    // If expression
    if (match(TokenType::IF)) {
        return parseIfExpression();
    }

    // Match expression
    if (match(TokenType::MATCH)) {
        return parseMatchExpression();
    }

    throw std::runtime_error("Expected expression at line " + std::to_string(peek().line));
}

// Literal parsing
std::unique_ptr<volta::ast::Expression> Parser::parseArrayLiteral() {
    // '[' already consumed
    auto loc = currentLocation();
    std::vector<std::unique_ptr<Expression>> elements;

    if (!check(TokenType::RSQUARE)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RSQUARE, "Expected ']' after array elements");
    return std::make_unique<ArrayLiteral>(std::move(elements), loc);
}

std::unique_ptr<volta::ast::Expression> Parser::parseTupleLiteral() {
    // Already handled in parsePrimary() for consistency
    throw std::runtime_error("parseTupleLiteral should be handled in parsePrimary");
}

std::unique_ptr<volta::ast::Expression> Parser::parseStructLiteral() {
    // Struct name already consumed as identifier in parsePrimary
    auto loc = currentLocation();
    auto structName = std::make_unique<IdentifierExpression>(previous().lexeme, loc);

    consume(TokenType::LBRACE, "Expected '{' after struct name");

    std::vector<std::unique_ptr<FieldInit>> fields;

    if (!check(TokenType::RBRACE)) {
        do {
            auto fieldName = consume(TokenType::IDENTIFIER, "Expected field name");
            auto fieldIdent = std::make_unique<IdentifierExpression>(fieldName.lexeme, currentLocation());

            consume(TokenType::COLON, "Expected ':' after field name");

            auto fieldValue = parseExpression();

            fields.push_back(std::make_unique<FieldInit>(
                std::move(fieldIdent),
                std::move(fieldValue)
            ));
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RBRACE, "Expected '}' after struct fields");

    return std::make_unique<StructLiteral>(
        std::move(structName),
        std::move(fields),
        loc
    );
}

std::unique_ptr<volta::ast::Expression> Parser::parseIfExpression() {
    // 'if' already consumed
    auto loc = currentLocation();

    auto condition = parseExpression();

    consume(TokenType::LBRACE, "Expected '{' after if condition");
    auto thenExpr = parseExpression();
    consume(TokenType::RBRACE, "Expected '}' after then expression");

    consume(TokenType::ELSE, "If expression must have else clause");

    consume(TokenType::LBRACE, "Expected '{' after else");
    auto elseExpr = parseExpression();
    consume(TokenType::RBRACE, "Expected '}' after else expression");

    return std::make_unique<IfExpression>(
        std::move(condition),
        std::move(thenExpr),
        std::move(elseExpr),
        loc
    );
}

std::unique_ptr<volta::ast::Expression> Parser::parseMatchExpression() {
    // 'match' already consumed
    auto loc = currentLocation();

    auto value = parseExpression();

    consume(TokenType::LBRACE, "Expected '{' after match value");

    std::vector<std::unique_ptr<MatchArm>> arms;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto pattern = parsePattern();

        // Optional guard: "if condition"
        std::unique_ptr<Expression> guard = nullptr;
        if (match(TokenType::IF)) {
            guard = parseExpression();
        }

        consume(TokenType::MATCH_ARROW, "Expected '=>' after pattern");

        auto body = parseExpression();

        arms.push_back(std::make_unique<MatchArm>(
            std::move(pattern),
            std::move(guard),
            std::move(body)
        ));

        // Optional comma between arms
        match(TokenType::COMMA);
    }

    consume(TokenType::RBRACE, "Expected '}' after match arms");

    return std::make_unique<MatchExpression>(
        std::move(value),
        std::move(arms),
        loc
    );
}

std::unique_ptr<volta::ast::Expression> Parser::parseLambdaExpression() {
    // Lambda syntax: |params| -> type { body } or |params| -> type expression
    auto loc = currentLocation();

    // Simplified: not implemented yet
    throw std::runtime_error("parseLambdaExpression not fully implemented");
}

// Type parsing
std::unique_ptr<volta::ast::Type> Parser::parseType() {
    // Check for function type: fn(T1, T2) -> T
    if (match(TokenType::FUNCTION)) {
        return parseFunctionType();
    }

    // Check for tuple type: (T1, T2, ...)
    if (check(TokenType::LPAREN)) {
        advance();

        // Empty tuple or single type?
        if (check(TokenType::RPAREN)) {
            advance();
            std::vector<std::unique_ptr<Type>> types;
            return std::make_unique<CompoundType>(CompoundType::Kind::Tuple, std::move(types));
        }

        auto firstType = parseType();

        if (match(TokenType::COMMA)) {
            // Tuple type
            std::vector<std::unique_ptr<Type>> types;
            types.push_back(std::move(firstType));

            do {
                types.push_back(parseType());
            } while (match(TokenType::COMMA));

            consume(TokenType::RPAREN, "Expected ')' after tuple types");
            return std::make_unique<CompoundType>(CompoundType::Kind::Tuple, std::move(types));
        } else if (match(TokenType::RPAREN)) {
            // Single type in parens
            return firstType;
        }

        // Error: expected comma or rparen
        throw std::runtime_error("Expected ',' or ')' in type");
    }

    // Check for identifier (named type or compound with generic args)
    if (match(TokenType::IDENTIFIER)) {
        auto name = previous().lexeme;
        auto nameExpr = std::make_unique<IdentifierExpression>(name, currentLocation());

        // Check for generic args: Identifier[T1, T2]
        if (match(TokenType::LSQUARE)) {
            std::vector<std::unique_ptr<Type>> typeArgs;

            if (!check(TokenType::RSQUARE)) {
                do {
                    typeArgs.push_back(parseType());
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RSQUARE, "Expected ']' after type arguments");

            // Check if it's a known compound type
            if (name == "Array") {
                return std::make_unique<CompoundType>(CompoundType::Kind::Array, std::move(typeArgs));
            } else if (name == "Matrix") {
                return std::make_unique<CompoundType>(CompoundType::Kind::Matrix, std::move(typeArgs));
            } else if (name == "Option") {
                return std::make_unique<CompoundType>(CompoundType::Kind::Option, std::move(typeArgs));
            } else {
                // Generic user-defined type
                return std::make_unique<GenericType>(std::move(nameExpr), std::move(typeArgs));
            }
        }

        // Check if it's a primitive type
        if (name == "int") {
            return std::make_unique<PrimitiveType>(PrimitiveType::Kind::Int);
        } else if (name == "float") {
            return std::make_unique<PrimitiveType>(PrimitiveType::Kind::Float);
        } else if (name == "bool") {
            return std::make_unique<PrimitiveType>(PrimitiveType::Kind::Bool);
        } else if (name == "str") {
            return std::make_unique<PrimitiveType>(PrimitiveType::Kind::Str);
        }

        // Named type (user-defined struct, etc.)
        return std::make_unique<NamedType>(std::move(nameExpr));
    }

    throw std::runtime_error("Expected type");
}

std::unique_ptr<volta::ast::TypeAnnotation> Parser::parseTypeAnnotation() {
    bool isMutable = match(TokenType::MUT);
    auto type = parseType();
    return std::make_unique<TypeAnnotation>(isMutable, std::move(type));
}

std::unique_ptr<volta::ast::Type> Parser::parsePrimitiveType() {
    // Handled in parseType()
    throw std::runtime_error("parsePrimitiveType should be called through parseType");
}

std::unique_ptr<volta::ast::Type> Parser::parseCompoundType() {
    // Handled in parseType()
    throw std::runtime_error("parseCompoundType should be called through parseType");
}

std::unique_ptr<volta::ast::Type> Parser::parseFunctionType() {
    // 'fn' already consumed
    consume(TokenType::LPAREN, "Expected '(' after 'fn'");

    std::vector<std::unique_ptr<Type>> paramTypes;

    if (!check(TokenType::RPAREN)) {
        do {
            paramTypes.push_back(parseType());
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "Expected ')' after parameter types");
    consume(TokenType::ARROW, "Expected '->' in function type");

    auto returnType = parseType();

    return std::make_unique<FunctionType>(std::move(paramTypes), std::move(returnType));
}

// Pattern parsing
std::unique_ptr<volta::ast::Pattern> Parser::parsePattern() {
    auto loc = currentLocation();

    // Literal pattern
    if (check(TokenType::INTEGER) || check(TokenType::REAL) ||
        check(TokenType::STRING_LITERAL) || check(TokenType::BOOLEAN_LITERAL)) {
        auto literal = parsePrimary(); // Reuse primary parsing for literals
        return std::make_unique<LiteralPattern>(std::move(literal), loc);
    }

    // Identifier, wildcard, or constructor pattern
    if (check(TokenType::IDENTIFIER)) {
        auto name = advance().lexeme;
        auto ident = std::make_unique<IdentifierExpression>(name, loc);

        // Wildcard pattern: _
        if (name == "_") {
            return std::make_unique<WildcardPattern>(loc);
        }

        // Constructor pattern: Some(x), Point(a, b)
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<Pattern>> args;

            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parsePattern());
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RPAREN, "Expected ')' after constructor arguments");
            return std::make_unique<ConstructorPattern>(std::move(ident), std::move(args), loc);
        }

        // Simple identifier pattern
        return std::make_unique<IdentifierPattern>(std::move(ident), loc);
    }

    // Tuple pattern: (a, b, c)
    if (match(TokenType::LPAREN)) {
        std::vector<std::unique_ptr<Pattern>> elements;

        if (!check(TokenType::RPAREN)) {
            do {
                elements.push_back(parsePattern());
            } while (match(TokenType::COMMA));
        }

        consume(TokenType::RPAREN, "Expected ')' after tuple pattern");
        return std::make_unique<TuplePattern>(std::move(elements), loc);
    }

    throw std::runtime_error("Expected pattern");
}

// Token navigation
const volta::lexer::Token& Parser::peek() const {
    return tokens_[current_];
}

const volta::lexer::Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const volta::lexer::Token& Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

const volta::lexer::Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();

    auto loc = currentLocation();
    errorReporter.reportSyntaxError(message, loc);

    throw std::runtime_error(message);
}

// Helper methods
volta::errors::SourceLocation Parser::currentLocation() const {
    const auto& token = peek();
    return volta::errors::SourceLocation(
        "",  // filename will be set by caller if needed
        token.line,
        token.column,
        token.lexeme.length()
    );
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) {
            case TokenType::FUNCTION:
            case TokenType::STRUCT:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }

        advance();
    }
}

} // namespace volta::parser