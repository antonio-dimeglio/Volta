#include "Parser/Parser.hpp"
#include <sstream>

// ==================== PUBLIC API ====================

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();

    while (!isAtEnd()) {
        Token t = peek();
        if (t.tt == TokenType::Function) {
            auto stmt = parseFnDef();
            if (stmt) {
                program->add_statement(std::move(stmt));
            }
        } else if (t.tt == TokenType::Extern) {
            auto stmt = parseExternBlock();
            if (stmt) {
                program->add_statement(std::move(stmt));
            }
        } else {
            diag.error("Unrecognized top lvl statement", t.line, t.column);
            advance();
        }
    }

    return program;
}

Token Parser::peek(size_t offset) const {
    size_t at = offset + idx;
    if (at >= tokens.size())
        return tokens[tokens.size() - 1];
    else
        return tokens[at];
}

bool Parser::isAtEnd() const {
    return peek().tt == TokenType::EndOfFile;
}

Token Parser::advance() {
    Token t = peek();
    idx++;
    return t;
}

bool Parser::check(TokenType type, size_t offset) const {
    return peek(offset).tt == type;
}

Token Parser::expect(TokenType expected) {
    Token t = peek();
    if (t.tt != expected) {
        std::ostringstream oss;
        oss << "Parsing error: expected " << tokenTypeToString(expected)
            << ", got " << tokenTypeToString(t.tt);
        diag.error(oss.str(), t.line, t.column);
    }
    return advance();
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType tt : types) {
        if (check(tt)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::isLiteralExpr() const {
    TokenType tt = peek().tt;
    return tt == TokenType::Integer ||
           tt == TokenType::Float ||
           tt == TokenType::String ||
           tt == TokenType::True_ ||
           tt == TokenType::False_;
}

std::unique_ptr<Type> Parser::parseType() {
    if (check(TokenType::LSquare)) {
        advance(); 

        auto elementType = parseType();

        expect(TokenType::Semicolon);

        bool isNegative = false;
        if (match({TokenType::Minus})) {
            isNegative = true;
        }

        Token sizeToken = expect(TokenType::Integer);
        int size = std::stoi(sizeToken.lexeme);

        if (isNegative) {
            size = -size;
            diag.error("Array size cannot be negative", sizeToken.line, sizeToken.column);
            size = 1;
        }

        if (size <= 0) {
            diag.error("Array size must be positive", sizeToken.line, sizeToken.column);
            size = 1;
        }

        expect(TokenType::RSquare);

        return std::make_unique<ArrayType>(std::move(elementType), size);
    }

    // Handle 'opaque' keyword
    if (check(TokenType::Opaque)) {
        advance();
        return std::make_unique<OpaqueType>();
    }

    if (check(TokenType::Identifier)) {
        std::string typeStr = advance().lexeme;

        if (check(TokenType::LessThan)) {
            advance();

            std::vector<std::unique_ptr<Type>> typeParams;
            typeParams.push_back(parseType());

            while (match({TokenType::Comma})) {
                typeParams.push_back(parseType());
            }

            expect(TokenType::GreaterThan);
            return std::make_unique<GenericType>(typeStr, std::move(typeParams));
        }

        auto maybeType = stringToPrimitiveTypeKind(typeStr);
        if (maybeType.has_value()) {
            return std::make_unique<PrimitiveType>(maybeType.value());
        }
        diag.error("Unknown type: " + typeStr, peek().line, peek().column);
        return std::make_unique<PrimitiveType>(PrimitiveTypeKind::I32);
    }

    diag.error("Expected type", peek().line, peek().column);
    return std::make_unique<PrimitiveType>(PrimitiveTypeKind::I32); // Default fallback
}


std::unique_ptr<FnDecl> Parser::parseFnSignature() {
    Token fnToken = expect(TokenType::Function);
    Token name = expect(TokenType::Identifier);

    expect(TokenType::LParen);
    std::vector<Param> params;

    while (!check(TokenType::RParen) && !isAtEnd()) {
        bool isRef = false;
        bool isMutRef = false;

        if (check(TokenType::Mut)) {
            advance();
            if (check(TokenType::Ref)) {
                advance();
                isMutRef = true;
            }
        } else if (check(TokenType::Ref)) {
            advance();
            isRef = true;
        }

        Token paramName = expect(TokenType::Identifier);
        expect(TokenType::Colon);

        if (check(TokenType::Mut)) {
            advance();
            if (check(TokenType::Ref)) {
                advance();
                isMutRef = true;
            } else {
                isMutRef = true;
            }
        } else if (check(TokenType::Ref)) {
            advance();
            isRef = true;
        }

        auto paramType = parseType();

        params.push_back(Param(paramName.lexeme, std::move(paramType), isRef, isMutRef));

        if (!check(TokenType::RParen)) {
            expect(TokenType::Comma);
        }
    }

    expect(TokenType::RParen);

    std::unique_ptr<Type> returnType = std::make_unique<PrimitiveType>(PrimitiveTypeKind::Void);
    if (match({TokenType::Arrow})) {
        returnType = parseType();
    }

    // Create FnDecl without body (empty vector indicates no body)
    return std::make_unique<FnDecl>(name.lexeme, std::move(params), std::move(returnType),
                                    std::vector<std::unique_ptr<Stmt>>(), false, fnToken.line, fnToken.column);
}

std::unique_ptr<Stmt> Parser::parseFnDef() {
    auto fn = parseFnSignature();

    // Parse body and set it
    auto body = parseBody();
    fn->body = std::move(body);

    return fn;
}

std::unique_ptr<Stmt> Parser::parseExternBlock() {
    Token externToken = expect(TokenType::Extern);

    // Expect ABI string (e.g., "C")
    Token abiToken = expect(TokenType::String);
    std::string abi = abiToken.lexeme;

    // Remove quotes from string literal
    if (abi.length() >= 2 && abi.front() == '"' && abi.back() == '"') {
        abi = abi.substr(1, abi.length() - 2);
    }

    expect(TokenType::LBrace);

    std::vector<std::unique_ptr<FnDecl>> declarations;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        auto fn = parseFnSignature();
        fn->isExtern = true;  // Mark as external declaration
        expect(TokenType::Semicolon);  // Extern functions end with semicolon
        declarations.push_back(std::move(fn));
    }

    expect(TokenType::RBrace);

    return std::make_unique<ExternBlock>(abi, std::move(declarations), externToken.line, externToken.column);
}

std::vector<std::unique_ptr<Stmt>> Parser::parseBody() {
    expect(TokenType::LBrace);
    std::vector<std::unique_ptr<Stmt>> stmts;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        Token t = peek();

        if (t.tt == TokenType::Let) {
            stmts.push_back(parseVarDecl());
            expect(TokenType::Semicolon);
        } else if (t.tt == TokenType::Return) {
            stmts.push_back(parseReturnStmt());
            expect(TokenType::Semicolon);
        } else if (t.tt == TokenType::If) {
            stmts.push_back(parseIfStmt());
        } else if (t.tt == TokenType::While) {
            stmts.push_back(parseWhileStmt());
        } else if (t.tt == TokenType::Break) {
            stmts.push_back(parseBreakStmt());
            expect(TokenType::Semicolon);
        } else if (t.tt == TokenType::Continue) {
            stmts.push_back(parseContinueStmt());
            expect(TokenType::Semicolon);
        } else if (t.tt == TokenType::For) {
            stmts.push_back(parseForStmt());
        } else {
            stmts.push_back(parseExprStmt());
            expect(TokenType::Semicolon);
        }
    }

    expect(TokenType::RBrace);
    return stmts;
}

std::unique_ptr<Stmt> Parser::parseVarDecl() {
    expect(TokenType::Let);

    bool mutable_ = false;
    if (match({TokenType::Mut})) {
        mutable_ = true;
    }

    Token name = expect(TokenType::Identifier);

    std::unique_ptr<Type> type_annotation = nullptr;
    if (match({TokenType::Colon})) {
        type_annotation = parseType();
    }

    std::unique_ptr<Expr> init_value = nullptr;
    if (match({TokenType::Assign})) {
        init_value = parseExpression();
    } else if (match({TokenType::InferAssign})) {
        init_value = parseExpression();
    }

    return std::make_unique<VarDecl>(mutable_, name, std::move(type_annotation), std::move(init_value));
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    Token retToken = expect(TokenType::Return);

    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::Semicolon)) {
        value = parseExpression();
    }

    return std::make_unique<ReturnStmt>(std::move(value), retToken.line, retToken.column);
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
    Token ifToken = expect(TokenType::If);
    auto condition = parseExpression();
    auto thenBody = parseBody();

    std::vector<std::unique_ptr<Stmt>> elseBody;
    if (match({TokenType::Else})) {
        elseBody = parseBody();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBody), std::move(elseBody), ifToken.line, ifToken.column);
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    Token whileToken = expect(TokenType::While);
    auto condition = parseExpression();
    auto body = parseBody();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), whileToken.line, whileToken.column);
}

std::unique_ptr<Stmt> Parser::parseForStmt() {
    Token forToken = expect(TokenType::For);
    Token var = expect(TokenType::Identifier);
    auto varNode = std::make_unique<Variable>(var);
    expect(TokenType::In);
    auto rangeExpr = parseRangeExpr();
    auto body = parseBody();

    // Cast Expr to Range (parseRangeExpr returns unique_ptr<Expr>)
    auto range = std::unique_ptr<Range>(static_cast<Range*>(rangeExpr.release()));

    return std::make_unique<ForStmt>(std::move(varNode), std::move(range), std::move(body), forToken.line, forToken.column);
}

std::unique_ptr<Stmt> Parser::parseBreakStmt() {
    expect(TokenType::Break);
    return std::make_unique<BreakStmt>();
}

std::unique_ptr<Stmt> Parser::parseContinueStmt() {
    expect(TokenType::Continue);
    return std::make_unique<ContinueStmt>();
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
    auto expr = parseExpression();
    int exprLine = expr->line;
    int exprColumn = expr->column;

    if (match({TokenType::Assign})) {
        Token assignToken = tokens[idx - 1];
        auto value = parseExpression();

        std::unique_ptr<Expr> lhs;
        if (auto* var = dynamic_cast<Variable*>(expr.get())) {
            lhs = std::make_unique<Variable>(var->token);
        } else if (auto* idx = dynamic_cast<IndexExpr*>(expr.get())) {
            lhs = std::move(expr);
        }

        return std::make_unique<ExprStmt>(
            std::make_unique<Assignment>(std::move(lhs), std::move(value), assignToken.line, assignToken.column),
            exprLine, exprColumn
        );
    }

    if (auto* var = dynamic_cast<Variable*>(expr.get())) {
        if (check(TokenType::PlusEqual) || check(TokenType::MinusEqual) ||
                   check(TokenType::MultEqual) || check(TokenType::DivEqual) ||
                   check(TokenType::ModuloEqual)) {
            Token opToken = advance();
            auto value = parseExpression();
            auto varCopy = std::make_unique<Variable>(var->token);
            return std::make_unique<ExprStmt>(
                std::make_unique<CompoundAssign>(std::move(varCopy), std::move(value), opToken.tt, opToken.line, opToken.column),
                exprLine, exprColumn
            );
        } else if (match({TokenType::Increment})) {
            Token incToken = tokens[idx - 1];
            auto varCopy = std::make_unique<Variable>(var->token);
            return std::make_unique<ExprStmt>(
                std::make_unique<Increment>(std::move(varCopy), incToken.line, incToken.column),
                exprLine, exprColumn
            );
        } else if (match({TokenType::Decrement})) {
            Token decToken = tokens[idx - 1];
            auto varCopy = std::make_unique<Variable>(var->token);
            return std::make_unique<ExprStmt>(
                std::make_unique<Decrement>(std::move(varCopy), decToken.line, decToken.column),
                exprLine, exprColumn
            );
        }
    }

    return std::make_unique<ExprStmt>(std::move(expr), exprLine, exprColumn);
}


std::unique_ptr<Expr> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();

    while (match({TokenType::Or})) {
        Token opToken = tokens[idx - 1];
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), opToken.tt, opToken.line, opToken.column);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseComparison();

    while (match({TokenType::And})) {
        Token opToken = tokens[idx - 1];
        auto right = parseComparison();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), opToken.tt, opToken.line, opToken.column);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseAddition();

    while (match({TokenType::EqualEqual, TokenType::NotEqual,
                  TokenType::LessThan, TokenType::LessEqual,
                  TokenType::GreaterThan, TokenType::GreaterEqual})) {
        Token opToken = tokens[idx - 1];
        auto right = parseAddition();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), opToken.tt, opToken.line, opToken.column);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseAddition() {
    auto left = parseTerm();

    while (match({TokenType::Plus, TokenType::Minus})) {
        Token opToken = tokens[idx - 1];
        auto right = parseTerm();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), opToken.tt, opToken.line, opToken.column);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto left = parseUnary();

    while (match({TokenType::Mult, TokenType::Div, TokenType::Modulo})) {
        Token opToken = tokens[idx - 1];
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(right), opToken.tt, opToken.line, opToken.column);
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match({TokenType::Minus, TokenType::Not, TokenType::Plus})) {
        Token opToken = tokens[idx - 1];
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(std::move(operand), opToken.tt, opToken.line, opToken.column);
    }

    if (check(TokenType::PtrKeyword)) {
        Token t = advance();
        auto expr = parseUnary();
        return std::make_unique<AddrOf>(std::move(expr), t.line, t.column);
    }

    return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();

    while (match({TokenType::LSquare})) {
        Token lsqToken = tokens[idx - 1];
        auto index = parseExpression();
        expect(TokenType::RSquare);
        expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), lsqToken.line, lsqToken.column);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseFunctionCall() {
    Token name = expect(TokenType::Identifier);
    expect(TokenType::LParen);

    std::vector<std::unique_ptr<Expr>> args;
    if (!check(TokenType::RParen)) {
        do {
            args.push_back(parseExpression());
        } while (match({TokenType::Comma}));
    }

    expect(TokenType::RParen);

    return std::make_unique<FnCall>(name.lexeme, std::move(args), name.line, name.column);
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    // Literals
    if (isLiteralExpr()) {
        Token token = advance();
        return std::make_unique<Literal>(token);
    }

    if (check(TokenType::Identifier)) {
        if (check(TokenType::LParen, 1)) {
            return parseFunctionCall();
        }
        Token token = advance();
        return std::make_unique<Variable>(token);
    }

    // Parenthesized expression
    if (match({TokenType::LParen})) {
        Token lparenToken = tokens[idx - 1];
        auto expr = parseExpression();
        expect(TokenType::RParen);
        return std::make_unique<GroupingExpr>(std::move(expr), lparenToken.line, lparenToken.column);
    }

    // Array literal
    if (check(TokenType::LSquare)) {
        return parseArrayLiteral();
    }

    Token t = peek();
    diag.error("Expected expression", t.line, t.column);
    return std::make_unique<Literal>(Token(TokenType::Integer, t.line, t.column, "0"));
}

std::unique_ptr<Expr> Parser::parseArrayLiteral() {
    Token lsqToken = expect(TokenType::LSquare);

    if (check(TokenType::RSquare)) {
        advance();
        return std::make_unique<ArrayLiteral>(std::vector<std::unique_ptr<Expr>>(), lsqToken.line, lsqToken.column);
    }

    auto firstExpr = parseExpression();

    if (match({TokenType::Semicolon})) {
        bool isNegative = false;
        if (match({TokenType::Minus})) {
            isNegative = true;
        }

        Token countToken = expect(TokenType::Integer);
        int count = std::stoi(countToken.lexeme);

        if (isNegative) {
            count = -count;
            diag.error("Array repeat count cannot be negative", countToken.line, countToken.column);
            count = 1; 
        }

        if (count <= 0) {
            diag.error("Array repeat count must be positive", countToken.line, countToken.column);
            count = 1;
        }

        expect(TokenType::RSquare);
        return std::make_unique<ArrayLiteral>(std::move(firstExpr), count, lsqToken.line, lsqToken.column);
    }

    std::vector<std::unique_ptr<Expr>> elements;
    elements.push_back(std::move(firstExpr));

    while (match({TokenType::Comma})) {
        if (check(TokenType::RSquare)) break;
        elements.push_back(parseExpression());
    }

    expect(TokenType::RSquare);
    return std::make_unique<ArrayLiteral>(std::move(elements), lsqToken.line, lsqToken.column);
}


std::unique_ptr<Expr> Parser::parseRangeExpr() {
    auto lhs = parseExpression();
    if (!match({TokenType::Range, TokenType::InclusiveRange})) {
        diag.error("Expected range operator", peek().line, peek().column);
    }
    Token rangeToken = tokens[idx - 1];
    bool inclusive = (rangeToken.tt == TokenType::InclusiveRange);
    auto rhs = parseExpression();
    return std::make_unique<Range>(std::move(lhs), std::move(rhs), inclusive, rangeToken.line, rangeToken.column);
}