#include "HIR/Lowering.hpp"
#include "Lexer/Token.hpp"

Program HIRLowering::lower(Program ast) {
    Program hir;

    for (auto& stmt : ast.statements) {
        hir.add_statement(lowerStmt(std::move(stmt)));
    }

    return hir;
}

std::unique_ptr<Stmt> HIRLowering::lowerStmt(std::unique_ptr<Stmt> stmt) {
    if (auto* varDecl = dynamic_cast<VarDecl*>(stmt.get())) {
        return lowerVarDecl(std::unique_ptr<VarDecl>(static_cast<VarDecl*>(stmt.release())));
    } else if (auto* fnDecl = dynamic_cast<FnDecl*>(stmt.get())) {
        return lowerFnDecl(std::unique_ptr<FnDecl>(static_cast<FnDecl*>(stmt.release())));
    } else if (auto* retStmt = dynamic_cast<ReturnStmt*>(stmt.get())) {
        return lowerReturnStmt(std::unique_ptr<ReturnStmt>(static_cast<ReturnStmt*>(stmt.release())));
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt.get())) {
        return lowerIfStmt(std::unique_ptr<IfStmt>(static_cast<IfStmt*>(stmt.release())));
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt.get())) {
        return lowerWhileStmt(std::unique_ptr<WhileStmt>(static_cast<WhileStmt*>(stmt.release())));
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt.get())) {
        return lowerExprStmt(std::unique_ptr<ExprStmt>(static_cast<ExprStmt*>(stmt.release())));
    } else if (auto* forStmt = dynamic_cast<ForStmt*>(stmt.get())) {
        return desugarForLoop(std::unique_ptr<ForStmt>(static_cast<ForStmt*>(stmt.release())));
    } else if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt.get())) {
        return lowerBlockStmt(std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(stmt.release())));
    } else if (auto* breakStmt = dynamic_cast<BreakStmt*>(stmt.get())) {
        auto* rawBreak = static_cast<BreakStmt*>(stmt.release());
        return std::make_unique<HIR::HIRBreakStmt>(rawBreak->line, rawBreak->column);
    } else if (auto* continueStmt = dynamic_cast<ContinueStmt*>(stmt.get())) {
        auto* rawContinue = static_cast<ContinueStmt*>(stmt.release());
        return std::make_unique<HIR::HIRContinueStmt>(rawContinue->line, rawContinue->column);
    }

    return stmt;
}

std::unique_ptr<Stmt> HIRLowering::lowerVarDecl(std::unique_ptr<VarDecl> node) {
    if (node->type_annotation) {
        auto info = analyzeArrayDimensions(node->type_annotation.get());

        if (info.dimensions.size() > 1) {
            node->array_dimensions = info.dimensions;
        }
    }

    if (node->init_value) {
        node->init_value = lowerExpr(std::move(node->init_value));
        if (auto* arrLit = dynamic_cast<ArrayLiteral*>(node->init_value.get())) {
            if (!arrLit->array_dimensions.empty() && node->array_dimensions.empty()) {
                node->array_dimensions = arrLit->array_dimensions;
            }
        }
    }

    return node;
}

std::unique_ptr<Stmt> HIRLowering::lowerFnDecl(std::unique_ptr<FnDecl> node) {
    std::vector<std::unique_ptr<Stmt>> loweredBody;
    for (auto& stmt : node->body) {
        loweredBody.push_back(lowerStmt(std::move(stmt)));
    }
    node->body = std::move(loweredBody);
    return node;
}

std::unique_ptr<Stmt> HIRLowering::lowerReturnStmt(std::unique_ptr<ReturnStmt> node) {
    auto loweredValue = node->value ? lowerExpr(std::move(node->value)) : nullptr;
    return std::make_unique<HIR::HIRReturnStmt>(
        std::move(loweredValue),
        node->line,
        node->column
    );
}

std::unique_ptr<Stmt> HIRLowering::lowerIfStmt(std::unique_ptr<IfStmt> node) {
    auto loweredCondition = lowerExpr(std::move(node->condition));

    std::vector<std::unique_ptr<Stmt>> loweredThen;
    for (auto& stmt : node->thenBody) {
        loweredThen.push_back(lowerStmt(std::move(stmt)));
    }

    std::vector<std::unique_ptr<Stmt>> loweredElse;
    for (auto& stmt : node->elseBody) {
        loweredElse.push_back(lowerStmt(std::move(stmt)));
    }

    return std::make_unique<HIR::HIRIfStmt>(
        std::move(loweredCondition),
        std::move(loweredThen),
        std::move(loweredElse),
        node->line,
        node->column
    );
}

std::unique_ptr<Stmt> HIRLowering::lowerWhileStmt(std::unique_ptr<WhileStmt> node) {
    auto loweredCondition = lowerExpr(std::move(node->condition));

    std::vector<std::unique_ptr<Stmt>> loweredBody;
    for (auto& stmt : node->thenBody) {
        loweredBody.push_back(lowerStmt(std::move(stmt)));
    }

    return std::make_unique<HIR::HIRWhileStmt>(
        std::move(loweredCondition),
        std::move(loweredBody),
        nullptr, 
        node->line,
        node->column
    );
}

std::unique_ptr<Stmt> HIRLowering::lowerExprStmt(std::unique_ptr<ExprStmt> node) {
    auto loweredExpr = lowerExpr(std::move(node->expr));
    return std::make_unique<HIR::HIRExprStmt>(
        std::move(loweredExpr),
        node->line,
        node->column
    );
}

std::unique_ptr<Stmt> HIRLowering::lowerBlockStmt(std::unique_ptr<BlockStmt> node) {
    std::vector<std::unique_ptr<Stmt>> loweredStmts;
    for (auto& stmt : node->statements) {
        loweredStmts.push_back(lowerStmt(std::move(stmt)));
    }
    return std::make_unique<HIR::HIRBlockStmt>(
        std::move(loweredStmts),
        node->line,
        node->column
    );
}


std::unique_ptr<Expr> HIRLowering::lowerExpr(std::unique_ptr<Expr> expr) {
    if (auto* compAssign = dynamic_cast<CompoundAssign*>(expr.get())) {
        return desugarCompoundAssign(std::unique_ptr<CompoundAssign>(static_cast<CompoundAssign*>(expr.release())));
    } else if (auto* inc = dynamic_cast<Increment*>(expr.get())) {
        return desugarIncrement(std::unique_ptr<Increment>(static_cast<Increment*>(expr.release())));
    } else if (auto* dec = dynamic_cast<Decrement*>(expr.get())) {
        return desugarDecrement(std::unique_ptr<Decrement>(static_cast<Decrement*>(expr.release())));
    }

    if (auto* binExpr = dynamic_cast<BinaryExpr*>(expr.get())) {
        return lowerBinaryExpr(std::unique_ptr<BinaryExpr>(static_cast<BinaryExpr*>(expr.release())));
    } else if (auto* unExpr = dynamic_cast<UnaryExpr*>(expr.get())) {
        return lowerUnaryExpr(std::unique_ptr<UnaryExpr>(static_cast<UnaryExpr*>(expr.release())));
    } else if (auto* fnCall = dynamic_cast<FnCall*>(expr.get())) {
        return lowerFnCall(std::unique_ptr<FnCall>(static_cast<FnCall*>(expr.release())));
    } else if (auto* assign = dynamic_cast<Assignment*>(expr.get())) {
        return lowerAssignment(std::unique_ptr<Assignment>(static_cast<Assignment*>(expr.release())));
    } else if (auto* group = dynamic_cast<GroupingExpr*>(expr.get())) {
        return lowerGroupingExpr(std::unique_ptr<GroupingExpr>(static_cast<GroupingExpr*>(expr.release())));
    } else if (auto* arr = dynamic_cast<ArrayLiteral*>(expr.get())) {
        return lowerArrayLiteral(std::unique_ptr<ArrayLiteral>(static_cast<ArrayLiteral*>(expr.release())));
    } else if (auto* idx = dynamic_cast<IndexExpr*>(expr.get())) {
        return lowerIndexExpr(std::unique_ptr<IndexExpr>(static_cast<IndexExpr*>(expr.release())));
    }

    return expr;
}

std::unique_ptr<Expr> HIRLowering::lowerBinaryExpr(std::unique_ptr<BinaryExpr> node) {
    node->lhs = lowerExpr(std::move(node->lhs));
    node->rhs = lowerExpr(std::move(node->rhs));
    return node;
}

std::unique_ptr<Expr> HIRLowering::lowerUnaryExpr(std::unique_ptr<UnaryExpr> node) {
    node->operand = lowerExpr(std::move(node->operand));
    return node;
}

std::unique_ptr<Expr> HIRLowering::lowerFnCall(std::unique_ptr<FnCall> node) {
    for (auto& arg : node->args) {
        arg = lowerExpr(std::move(arg));
    }
    return node;
}

std::unique_ptr<Expr> HIRLowering::lowerAssignment(std::unique_ptr<Assignment> node) {
    node->lhs = lowerExpr(std::move(node->lhs));
    node->value = lowerExpr(std::move(node->value));
    return node;
}

std::unique_ptr<Expr> HIRLowering::lowerGroupingExpr(std::unique_ptr<GroupingExpr> node) {
    node->expr = lowerExpr(std::move(node->expr));
    return node;
}

std::unique_ptr<Expr> HIRLowering::lowerArrayLiteral(std::unique_ptr<ArrayLiteral> node) {
    bool isNestedArray = false;

    if (!node->elements.empty()) {
        if (dynamic_cast<ArrayLiteral*>(node->elements[0].get())) {
            isNestedArray = true;
        }
    }

    if (isNestedArray) {
        std::vector<int> dimensions;
        dimensions.push_back(node->elements.size());

        if (auto* firstInner = dynamic_cast<ArrayLiteral*>(node->elements[0].get())) {
            if (!firstInner->array_dimensions.empty()) {
                dimensions.insert(dimensions.end(),
                                firstInner->array_dimensions.begin(),
                                firstInner->array_dimensions.end());
            } else if (!firstInner->elements.empty()) {
                dimensions.push_back(firstInner->elements.size());
            }
        }

        node->array_dimensions = dimensions;

        for (auto& elem : node->elements) {
            elem = lowerExpr(std::move(elem));
        }

        return node;
    }

    if (node->repeat_value) {
        node->repeat_value = lowerExpr(std::move(node->repeat_value));
    } else {
        for (auto& elem : node->elements) {
            elem = lowerExpr(std::move(elem));
        }
    }

    return node;
}


std::unique_ptr<Expr> HIRLowering::lowerIndexExpr(std::unique_ptr<IndexExpr> node) {
    node->array = lowerExpr(std::move(node->array));
    node->index = lowerExpr(std::move(node->index));
    return node;
}


std::unique_ptr<Expr> HIRLowering::desugarCompoundAssign(std::unique_ptr<CompoundAssign> node) {
    TokenType binOp;
    switch (node->op) {
        case TokenType::PlusEqual:   binOp = TokenType::Plus; break;
        case TokenType::MinusEqual:  binOp = TokenType::Minus; break;
        case TokenType::MultEqual:   binOp = TokenType::Mult; break;
        case TokenType::DivEqual:    binOp = TokenType::Div; break;
        case TokenType::ModuloEqual: binOp = TokenType::Modulo; break;
        default: binOp = TokenType::Plus; break; // Should never happen
    }

    auto lhs = cloneVariable(node->var.get());
    auto binaryExpr = std::make_unique<BinaryExpr>(
        std::move(lhs),
        lowerExpr(std::move(node->value)),
        binOp
    );

    return std::make_unique<Assignment>(
        std::move(node->var),
        std::move(binaryExpr)
    );
}

std::unique_ptr<Expr> HIRLowering::desugarIncrement(std::unique_ptr<Increment> node) {
    Token oneToken(TokenType::Integer, node->var->token.line, node->var->token.column, "1");
    auto one = std::make_unique<Literal>(oneToken);

    auto lhs = cloneVariable(node->var.get());
    auto binaryExpr = std::make_unique<BinaryExpr>(
        std::move(lhs),
        std::move(one),
        TokenType::Plus
    );

    return std::make_unique<Assignment>(
        std::move(node->var),
        std::move(binaryExpr)
    );
}

std::unique_ptr<Expr> HIRLowering::desugarDecrement(std::unique_ptr<Decrement> node) {
    Token oneToken(TokenType::Integer, node->var->token.line, node->var->token.column, "1");
    auto one = std::make_unique<Literal>(oneToken);

    auto lhs = cloneVariable(node->var.get());
    auto binaryExpr = std::make_unique<BinaryExpr>(
        std::move(lhs),
        std::move(one),
        TokenType::Minus
    );

    return std::make_unique<Assignment>(
        std::move(node->var),
        std::move(binaryExpr)
    );
}

std::unique_ptr<Stmt> HIRLowering::desugarForLoop(std::unique_ptr<ForStmt> node) {
    auto rangeLhs = lowerExpr(std::move(node->range->from));
    auto rangeRhs = lowerExpr(std::move(node->range->to));
    bool inclusive = node->range->inclusive;

    int line = node->line;
    int column = node->column;
    Token loopVar = node->var->token;

    auto varDecl = std::make_unique<VarDecl>(
        true,  // mutable
        loopVar,
        nullptr,  // no type annotation
        std::move(rangeLhs)
    );

    auto conditionVar = std::make_unique<Variable>(loopVar);
    TokenType comparisonOp = inclusive ? TokenType::LessEqual : TokenType::LessThan;
    auto condition = std::make_unique<BinaryExpr>(
        std::move(conditionVar),
        std::move(rangeRhs),
        comparisonOp,
        line,
        column
    );

    Token oneToken(TokenType::Integer, loopVar.line, loopVar.column, "1");
    auto one = std::make_unique<Literal>(oneToken);
    auto incrementLhs = std::make_unique<Variable>(loopVar);
    auto incrementValue = std::make_unique<BinaryExpr>(
        std::move(incrementLhs),
        std::move(one),
        TokenType::Plus,
        loopVar.line,
        loopVar.column
    );
    auto incrementVar = std::make_unique<Variable>(loopVar);
    auto incrementExpr = std::make_unique<Assignment>(
        std::move(incrementVar),
        std::move(incrementValue),
        loopVar.line,
        loopVar.column
    );

    std::vector<std::unique_ptr<Stmt>> whileBody;
    for (auto& stmt : node->body) {
        whileBody.push_back(lowerStmt(std::move(stmt)));
    }

    auto whileStmt = std::make_unique<HIR::HIRWhileStmt>(
        std::move(condition),
        std::move(whileBody),
        std::move(incrementExpr),  
        line,
        column
    );

    std::vector<std::unique_ptr<Stmt>> blockStmts;
    blockStmts.push_back(std::move(varDecl));
    blockStmts.push_back(std::move(whileStmt));

    return std::make_unique<HIR::HIRBlockStmt>(
        std::move(blockStmts),
        line,
        column
    );
}

std::unique_ptr<Variable> HIRLowering::cloneVariable(const Variable* var) {
    return std::make_unique<Variable>(var->token);
}


HIRLowering::ArrayDimensionInfo HIRLowering::analyzeArrayDimensions(const Type* type) {
    ArrayDimensionInfo info;
    std::vector<int> dims;

    const Type* current = type;

    while (auto* arrType = dynamic_cast<const ArrayType*>(current)) {
        dims.push_back(arrType->size);
        current = arrType->element_type.get();
    }

    info.dimensions = dims;
    info.element_type = const_cast<Type*>(current);

    return info;
}

std::unique_ptr<Type> HIRLowering::flattenArrayType(const Type* type) {
    auto info = analyzeArrayDimensions(type);

    if (info.dimensions.size() <= 1) {
        if (auto* arrType = dynamic_cast<const ArrayType*>(type)) {
            if (auto* primType = dynamic_cast<const PrimitiveType*>(info.element_type)) {
                return std::make_unique<ArrayType>(
                    std::make_unique<PrimitiveType>(primType->kind),
                    arrType->size
                );
            }
        }
        return nullptr;
    }

    int totalSize = 1;
    for (int dim : info.dimensions) {
        totalSize *= dim;
    }

    if (auto* primType = dynamic_cast<const PrimitiveType*>(info.element_type)) {
        return std::make_unique<ArrayType>(
            std::make_unique<PrimitiveType>(primType->kind),
            totalSize
        );
    }

    return nullptr;
}

std::unique_ptr<Expr> HIRLowering::calculateFlatIndex(
    std::vector<std::unique_ptr<Expr>>& indices,
    const std::vector<int>& dimensions) {

    if (indices.size() != dimensions.size()) {
        return std::move(indices[0]);
    }

    if (indices.size() == 1) {
        return std::move(indices[0]);
    }

    std::vector<int> strides(dimensions.size());
    strides[dimensions.size() - 1] = 1;
    for (int i = dimensions.size() - 2; i >= 0; i--) {
        strides[i] = strides[i + 1] * dimensions[i + 1];
    }

    std::unique_ptr<Expr> result = nullptr;

    for (size_t i = 0; i < indices.size(); i++) {
        std::unique_ptr<Expr> term;

        if (strides[i] == 1) {
            term = std::move(indices[i]);
        } else {
            Token strideToken(TokenType::Integer, 0, 0, std::to_string(strides[i]));
            auto strideLiteral = std::make_unique<Literal>(strideToken);

            term = std::make_unique<BinaryExpr>(
                std::move(indices[i]),
                std::move(strideLiteral),
                TokenType::Mult
            );
        }

        if (result == nullptr) {
            result = std::move(term);
        } else {
            result = std::make_unique<BinaryExpr>(
                std::move(result),
                std::move(term),
                TokenType::Plus
            );
        }
    }

    return result;
}
