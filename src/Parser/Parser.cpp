#include "Parser/Parser.hpp"
#include <sstream>

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();

    while (!isAtEnd()) {
        Token t = peek();
        if (t.tt == TokenType::Function || (t.tt == TokenType::Pub && peek(1).tt == TokenType::Function)) {
            auto stmt = parseFnDef();
            if (stmt) {
                program->add_statement(std::move(stmt));
            }
        } else if (t.tt == TokenType::Extern) {
            auto stmt = parseExternBlock();
            if (stmt) {
                program->add_statement(std::move(stmt));
            }
        } else if(t.tt == TokenType::Import) {
            auto stmt = parseImportStmt();
            if (stmt) {
                program->add_statement(std::move(stmt));
            }
        } else if (t.tt == TokenType::Struct || (t.tt == TokenType::Pub && peek(1).tt == TokenType::Struct)) {
            auto stmt = parseStructDecl();
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
           tt == TokenType::False_ ||
           tt == TokenType::Null;
}

const Type::Type* Parser::parseType() {
    if (check(TokenType::LSquare)) {
        advance(); 

        const Type::Type* elementType = parseType();

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

        return types.getArray(elementType, size);
    }

    if (check(TokenType::Opaque)) {
        advance();
        return types.getOpaque();
    }

    if (check(TokenType::Identifier)) {
        std::string typeStr = advance().lexeme;

        if (check(TokenType::LessThan)) {
            advance();

            std::vector<const Type::Type*> typeParams;
            typeParams.push_back(parseType());

            while (match({TokenType::Comma})) {
                typeParams.push_back(parseType());
            }

            expect(TokenType::GreaterThan);

            // Special case: ptr<T> is a pointer type, not a generic
            if (typeStr == "ptr") {
                if (typeParams.size() != 1) {
                    diag.error("ptr requires exactly one type parameter", peek().line, peek().column);
                    return types.getPointer(types.getPrimitive(Type::PrimitiveKind::I32));
                }
                return types.getPointer(typeParams[0]);
            }

            return types.getGeneric(typeStr, typeParams);
        }

        const Type::Type* parsedType = types.parseTypeName(typeStr);
        if (parsedType) {
            return parsedType;
        }

        // If not a known primitive/struct, create an unresolved type
        // This will be resolved during semantic analysis
        return types.getUnresolved(typeStr);
    }

    diag.error("Expected type", peek().line, peek().column);
    return types.getPrimitive(Type::PrimitiveKind::I32); // Default fallback type
}


std::unique_ptr<FnDecl> Parser::parseFnSignature() {
    bool isPub = false;
    if (check(TokenType::Pub))  {
        isPub = true;
        advance();
    }

    Token fnToken = expect(TokenType::Function);
    Token name = expect(TokenType::Identifier);

    expect(TokenType::LParen);
    std::vector<Param> params;

    while (!check(TokenType::RParen) && !isAtEnd()) {
        bool isRef = false;
        bool isMutRef = false;

        // Check for 'mut self' or just 'self'
        if (check(TokenType::Mut) && peek(1).tt == TokenType::Self_) {
            advance();  // consume 'mut'
            Token selfToken = expect(TokenType::Self_);
            // 'mut self' - will be resolved to struct pointer type during semantic analysis
            params.push_back(Param(selfToken.lexeme, nullptr, true, true));

            if (!check(TokenType::RParen)) {
                expect(TokenType::Comma);
            }
            continue;
        } else if (check(TokenType::Self_)) {
            Token selfToken = advance();
            // 'self' - will be resolved to struct pointer type during semantic analysis
            params.push_back(Param(selfToken.lexeme, nullptr, true, false));

            if (!check(TokenType::RParen)) {
                expect(TokenType::Comma);
            }
            continue;
        }

        if (check(TokenType::Mut)) {
            advance();
            if (check(TokenType::Ref)) {
                advance();
                isRef = true;
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
                isRef = true;
                isMutRef = true;
            } else {
                isMutRef = true;
            }
        } else if (check(TokenType::Ref)) {
            advance();
            isRef = true;
        }

        const Type::Type* paramType = parseType();

        params.push_back(Param(paramName.lexeme, paramType, isRef, isMutRef));

        if (!check(TokenType::RParen)) {
            expect(TokenType::Comma);
        }
    }

    expect(TokenType::RParen);

    const Type::Type* returnType = types.getPrimitive(Type::PrimitiveKind::Void);
    if (match({TokenType::Arrow})) {
        returnType = parseType();
    }

    return std::make_unique<FnDecl>(name.lexeme, std::move(params), returnType,
                                    std::vector<std::unique_ptr<Stmt>>(), false, isPub, fnToken.line, fnToken.column);
}

std::unique_ptr<Stmt> Parser::parseFnDef() {
    auto fn = parseFnSignature();

    auto body = parseBody();
    fn->body = std::move(body);

    return fn;
}

std::unique_ptr<StructDecl> Parser::parseStructDecl() {
    bool isPublic = match({TokenType::Pub});
    expect(TokenType::Struct);  // Consume 'struct' keyword
    Token structName = expect(TokenType::Identifier);
    expect(TokenType::LBrace);

    std::vector<StructField> fields;
    std::vector<std::unique_ptr<FnDecl>> methods;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        bool memberIsPublic = false;
        if (match({TokenType::Pub})) {
            memberIsPublic = true;
        }

        // Check if this is a method (fn keyword) or a field
        if (check(TokenType::Function)) {
            // Parse method (parseFnSignature expects to see 'fn' token)
            auto method = parseFnSignature();
            method->isPublic = memberIsPublic;

            // Parse method body
            method->body = parseBody();

            methods.push_back(std::move(method));
        } else {
            // Parse field
            Token fieldName = expect(TokenType::Identifier);
            expect(TokenType::Colon);
            const Type::Type* fieldType = parseType();

            fields.push_back(StructField(memberIsPublic, fieldName, fieldType));

            if (check(TokenType::Comma)) {
                advance();
            }
        }
    }

    expect(TokenType::RBrace);

    return std::make_unique<StructDecl>(isPublic, structName, std::move(fields), std::move(methods));
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

std::unique_ptr<ImportStmt> Parser::parseImportStmt() {
    Token importToken = expect(TokenType::Import);
    std::string modulePath = expect(TokenType::Identifier).lexeme;
    std::vector<std::string> importedItems;

    // Parse module path: std.io
    while (check(TokenType::Dot) && !isAtEnd()) {
        // Check if the dot is followed by a brace (end of module path)
        if (check(TokenType::LBrace, 1)) {
            advance();  // consume the final dot before the brace
            break;
        }

        advance();  // consume dot
        modulePath += ".";
        modulePath += expect(TokenType::Identifier).lexeme;
    }

    // Expect opening brace
    expect(TokenType::LBrace);

    // Parse imported items
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        importedItems.push_back(expect(TokenType::Identifier).lexeme);
        if (!check(TokenType::RBrace)) {
            expect(TokenType::Comma);
        }
    }

    expect(TokenType::RBrace);
    expect(TokenType::Semicolon);

    return std::make_unique<ImportStmt>(modulePath, importedItems, importToken.line, importToken.column);
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
        } else if (t.tt == TokenType::LBrace) {
            stmts.push_back(parseBlockStmt());
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

    const Type::Type* typeAnnotation = nullptr;
    if (match({TokenType::Colon})) {
        typeAnnotation = parseType();
    }

    std::unique_ptr<Expr> initValue = nullptr;
    if (match({TokenType::Assign})) {
        initValue = parseExpression();
    } else if (match({TokenType::InferAssign})) {
        initValue = parseExpression();
    }

    return std::make_unique<VarDecl>(mutable_, name, typeAnnotation, std::move(initValue));
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
        // Check if this is "else if" or just "else"
        if (check(TokenType::If)) {
            // Recursively parse the "else if" as a nested if-statement
            elseBody.push_back(parseIfStmt());
        } else {
            // Parse regular else block
            elseBody = parseBody();
        }
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

std::unique_ptr<Stmt> Parser::parseBlockStmt() {
    Token lbrace = expect(TokenType::LBrace);
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
        } else if (t.tt == TokenType::LBrace) {
            stmts.push_back(parseBlockStmt());  // Recursive!
        } else {
            stmts.push_back(parseExprStmt());
            expect(TokenType::Semicolon);
        }
    }

    expect(TokenType::RBrace);
    return std::make_unique<BlockStmt>(std::move(stmts), lbrace.line, lbrace.column);
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
        } else if (auto* fieldAccess = dynamic_cast<FieldAccess*>(expr.get())) {
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

    while (true) {
        if (match({TokenType::LSquare})) {
            // Array indexing: expr[index]
            Token lsqToken = tokens[idx - 1];
            auto index = parseExpression();
            expect(TokenType::RSquare);
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), lsqToken.line, lsqToken.column);
        } else if (match({TokenType::Dot})) {
            Token dotToken = tokens[idx - 1];
            Token memberName = expect(TokenType::Identifier);

            // Check if this is a method call (followed by '(') or field access
            if (check(TokenType::LParen)) {
                // Instance method call: expr.method(args)
                expect(TokenType::LParen);
                std::vector<std::unique_ptr<Expr>> args;
                if (!check(TokenType::RParen)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match({TokenType::Comma}));
                }
                expect(TokenType::RParen);
                expr = std::make_unique<InstanceMethodCall>(std::move(expr), memberName, std::move(args), dotToken.line, dotToken.column);
            } else {
                // Field access: expr.field
                expr = std::make_unique<FieldAccess>(std::move(expr), memberName, memberName.line, memberName.column);
            }
        } else {
            break;
        }
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

    // Handle 'self' keyword as a variable
    if (check(TokenType::Self_)) {
        Token token = advance();
        return std::make_unique<Variable>(token);
    }

    if (check(TokenType::Identifier)) {
        // Check for static method call: Type::method(...)
        if (check(TokenType::DoubleColon, 1)) {
            Token typeName = advance();
            expect(TokenType::DoubleColon);
            Token methodName = expect(TokenType::Identifier);
            expect(TokenType::LParen);

            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(parseExpression());
                } while (match({TokenType::Comma}));
            }

            expect(TokenType::RParen);
            return std::make_unique<StaticMethodCall>(typeName, methodName, std::move(args), typeName.line, typeName.column);
        }

        if (check(TokenType::LParen, 1)) {
            return parseFunctionCall();
        }

        // Check for struct literal: StructName { ... }
        // Use heuristic: treat as struct literal if identifier starts with uppercase
        // This prevents false positives like "variable and something {" being parsed as struct literal
        if (check(TokenType::LBrace, 1)) {
            Token identToken = peek();
            if (!identToken.lexeme.empty() && std::isupper(identToken.lexeme[0])) {
                Token structName = advance();
                return parseStructLiteral(structName);
            }
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

std::unique_ptr<StructLiteral> Parser::parseStructLiteral(Token structName) {
    // Already consumed the struct name, now consume the opening '{'
    expect(TokenType::LBrace);

    std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        Token fieldName = expect(TokenType::Identifier);
        expect(TokenType::Colon);
        auto fieldValue = parseExpression();

        fields.push_back({fieldName, std::move(fieldValue)});

        if (!match({TokenType::Comma})) {
            break;
        }
    }

    expect(TokenType::RBrace);

    return std::make_unique<StructLiteral>(structName, std::move(fields),
                                            structName.line, structName.column);
}