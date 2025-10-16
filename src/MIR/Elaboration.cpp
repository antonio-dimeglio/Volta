#include "MIR/Elaboration.hpp"
#include "Semantic/SymbolTable.hpp"
#include "Error/Error.hpp"
#include <iostream>

MIRElaboration::MIRElaboration(Semantic::SymbolTable* symTable)
    : symbolTable(symTable) {}

Program MIRElaboration::elaborate(Program ast) {
    Program elaborated;

    for (auto& stmt : ast.statements) {
        auto elaboratedStmt = elaborateStmt(std::move(stmt));
        if (elaboratedStmt) {
            elaborated.statements.push_back(std::move(elaboratedStmt));
        }
    }

    return elaborated;
}


std::unique_ptr<Stmt> MIRElaboration::elaborateStmt(std::unique_ptr<Stmt> stmt) {
    if (auto* varDecl = dynamic_cast<VarDecl*>(stmt.get())) {
        return elaborateVarDecl(std::unique_ptr<VarDecl>(static_cast<VarDecl*>(stmt.release())));
    } else if (auto* fnDecl = dynamic_cast<FnDecl*>(stmt.get())) {
        return elaborateFnDecl(std::unique_ptr<FnDecl>(static_cast<FnDecl*>(stmt.release())));
    } else if (auto* retStmt = dynamic_cast<ReturnStmt*>(stmt.get())) {
        return elaborateReturnStmt(std::unique_ptr<ReturnStmt>(static_cast<ReturnStmt*>(stmt.release())));
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt.get())) {
        return elaborateIfStmt(std::unique_ptr<IfStmt>(static_cast<IfStmt*>(stmt.release())));
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt.get())) {
        return elaborateWhileStmt(std::unique_ptr<WhileStmt>(static_cast<WhileStmt*>(stmt.release())));
    } else if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt.get())) {
        return elaborateBlockStmt(std::unique_ptr<BlockStmt>(static_cast<BlockStmt*>(stmt.release())));
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt.get())) {
        return elaborateExprStmt(std::unique_ptr<ExprStmt>(static_cast<ExprStmt*>(stmt.release())));
    }

    return stmt;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateVarDecl(std::unique_ptr<VarDecl> node) {
    if (!node->array_dimensions.empty()) {
        variableDimensions[node->name.lexeme] = node->array_dimensions;

        if (node->type_annotation) {
            node->type_annotation = flattenArrayType(node->type_annotation.get());
        }
    }

    if (node->init_value) {
        node->init_value = elaborateExpr(std::move(node->init_value));
    }

    return node;
}

std::unique_ptr<Type> MIRElaboration::flattenArrayType(const Type* type) {
    std::vector<int> dims;
    const Type* current = type;

    while (auto* arrType = dynamic_cast<const ArrayType*>(current)) {
        dims.push_back(arrType->size);
        current = arrType->element_type.get();
    }

    int totalSize = 1;
    for (int dim : dims) {
        totalSize *= dim;
    }

    if (auto* primType = dynamic_cast<const PrimitiveType*>(current)) {
        return std::make_unique<ArrayType>(
            std::make_unique<PrimitiveType>(primType->kind),
            totalSize
        );
    }

    return nullptr;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateFnDecl(std::unique_ptr<FnDecl> node) {
    for (auto& param : node->params) {
        if (param.type) {
            std::vector<int> dims;
            const Type* current = param.type.get();

            while (auto* arrType = dynamic_cast<const ArrayType*>(current)) {
                dims.push_back(arrType->size);
                current = arrType->element_type.get();
            }

            if (dims.size() > 1) {
                variableDimensions[param.name] = dims;

                param.type = flattenArrayType(param.type.get());
            }
        }
    }

    std::vector<std::unique_ptr<Stmt>> elaboratedBody;

    for (auto& stmt : node->body) {
        auto elaboratedStmt = elaborateStmt(std::move(stmt));
        if (elaboratedStmt) {
            elaboratedBody.push_back(std::move(elaboratedStmt));
        }
    }

    node->body = std::move(elaboratedBody);
    return node;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateReturnStmt(std::unique_ptr<ReturnStmt> node) {
    if (node->value) {
        node->value = elaborateExpr(std::move(node->value));
    }
    return node;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateIfStmt(std::unique_ptr<IfStmt> node) {
    node->condition = elaborateExpr(std::move(node->condition));

    std::vector<std::unique_ptr<Stmt>> elaboratedThen;
    for (auto& stmt : node->thenBody) {
        elaboratedThen.push_back(elaborateStmt(std::move(stmt)));
    }
    node->thenBody = std::move(elaboratedThen);

    if (!node->elseBody.empty()) {
        std::vector<std::unique_ptr<Stmt>> elaboratedElse;
        for (auto& stmt : node->elseBody) {
            elaboratedElse.push_back(elaborateStmt(std::move(stmt)));
        }
        node->elseBody = std::move(elaboratedElse);
    }

    return node;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateWhileStmt(std::unique_ptr<WhileStmt> node) {
    node->condition = elaborateExpr(std::move(node->condition));

    std::vector<std::unique_ptr<Stmt>> elaboratedBody;
    for (auto& stmt : node->thenBody) {
        elaboratedBody.push_back(elaborateStmt(std::move(stmt)));
    }
    node->thenBody = std::move(elaboratedBody);

    return node;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateBlockStmt(std::unique_ptr<BlockStmt> node) {
    std::vector<std::unique_ptr<Stmt>> elaboratedStmts;
    for (auto& stmt : node->statements) {
        elaboratedStmts.push_back(elaborateStmt(std::move(stmt)));
    }
    node->statements = std::move(elaboratedStmts);
    return node;
}

std::unique_ptr<Stmt> MIRElaboration::elaborateExprStmt(std::unique_ptr<ExprStmt> node) {
    node->expr = elaborateExpr(std::move(node->expr));
    return node;
}



std::unique_ptr<Expr> MIRElaboration::elaborateExpr(std::unique_ptr<Expr> expr) {
    if (auto* binExpr = dynamic_cast<BinaryExpr*>(expr.get())) {
        return elaborateBinaryExpr(std::unique_ptr<BinaryExpr>(static_cast<BinaryExpr*>(expr.release())));
    } else if (auto* unExpr = dynamic_cast<UnaryExpr*>(expr.get())) {
        return elaborateUnaryExpr(std::unique_ptr<UnaryExpr>(static_cast<UnaryExpr*>(expr.release())));
    } else if (auto* fnCall = dynamic_cast<FnCall*>(expr.get())) {
        return elaborateFnCall(std::unique_ptr<FnCall>(static_cast<FnCall*>(expr.release())));
    } else if (auto* assign = dynamic_cast<Assignment*>(expr.get())) {
        return elaborateAssignment(std::unique_ptr<Assignment>(static_cast<Assignment*>(expr.release())));
    } else if (auto* group = dynamic_cast<GroupingExpr*>(expr.get())) {
        return elaborateGroupingExpr(std::unique_ptr<GroupingExpr>(static_cast<GroupingExpr*>(expr.release())));
    } else if (auto* arr = dynamic_cast<ArrayLiteral*>(expr.get())) {
        return elaborateArrayLiteral(std::unique_ptr<ArrayLiteral>(static_cast<ArrayLiteral*>(expr.release())));
    } else if (auto* idx = dynamic_cast<IndexExpr*>(expr.get())) {
        return elaborateIndexExpr(std::unique_ptr<IndexExpr>(static_cast<IndexExpr*>(expr.release())));
    }
\
    return expr;
}

std::unique_ptr<Expr> MIRElaboration::elaborateBinaryExpr(std::unique_ptr<BinaryExpr> node) {
    node->lhs = elaborateExpr(std::move(node->lhs));
    node->rhs = elaborateExpr(std::move(node->rhs));
    return node;
}

std::unique_ptr<Expr> MIRElaboration::elaborateUnaryExpr(std::unique_ptr<UnaryExpr> node) {
    node->operand = elaborateExpr(std::move(node->operand));
    return node;
}

std::unique_ptr<Expr> MIRElaboration::elaborateFnCall(std::unique_ptr<FnCall> node) {
    for (auto& arg : node->args) {
        arg = elaborateExpr(std::move(arg));
    }
    return node;
}

std::unique_ptr<Expr> MIRElaboration::elaborateAssignment(std::unique_ptr<Assignment> node) {
    node->lhs = elaborateExpr(std::move(node->lhs));
    node->value = elaborateExpr(std::move(node->value));
    return node;
}

std::unique_ptr<Expr> MIRElaboration::elaborateGroupingExpr(std::unique_ptr<GroupingExpr> node) {
    node->expr = elaborateExpr(std::move(node->expr));
    return node;
}

std::unique_ptr<Expr> MIRElaboration::elaborateArrayLiteral(std::unique_ptr<ArrayLiteral> node) {
    bool isNestedArray = false;
    if (!node->elements.empty()) {
        if (dynamic_cast<ArrayLiteral*>(node->elements[0].get())) {
            isNestedArray = true;
        }
    }

    if (isNestedArray) {
        std::vector<std::unique_ptr<Expr>> flatElements;

        for (auto& elem : node->elements) {
            auto* innerArray = dynamic_cast<ArrayLiteral*>(elem.get());
            if (!innerArray) {
                flatElements.push_back(elaborateExpr(std::move(elem)));
                continue;
            }

            auto elaboratedInner = elaborateArrayLiteral(
                std::unique_ptr<ArrayLiteral>(static_cast<ArrayLiteral*>(elem.release()))
            );

            auto* elaboratedInnerArray = dynamic_cast<ArrayLiteral*>(elaboratedInner.get());
            if (!elaboratedInnerArray) {
                continue;
            }

            for (auto& innerElem : elaboratedInnerArray->elements) {
                flatElements.push_back(std::move(innerElem));
            }
        }

        auto flatArray = std::make_unique<ArrayLiteral>(std::move(flatElements));
        flatArray->array_dimensions = node->array_dimensions;
        return flatArray;

    } else if (node->repeat_value) {
        node->repeat_value = elaborateExpr(std::move(node->repeat_value));
    } else {
        for (auto& elem : node->elements) {
            elem = elaborateExpr(std::move(elem));
        }
    }

    return node;
}

std::unique_ptr<Expr> MIRElaboration::elaborateIndexExpr(std::unique_ptr<IndexExpr> node) {
    if (isMultiDimensionalIndexing(node.get())) {
        return transformMultiDimIndexing(std::move(node));
    }

    node->array = elaborateExpr(std::move(node->array));
    node->index = elaborateExpr(std::move(node->index));
    return node;
}

bool MIRElaboration::isMultiDimensionalIndexing(const IndexExpr* expr) {
    if (auto nextIdx = dynamic_cast<const IndexExpr*>(expr->array.get())) {
        return true;
    }
    return false;
}

MIRElaboration::IndexChain MIRElaboration::collectIndices(std::unique_ptr<IndexExpr> expr) {
    IndexChain chain;
    IndexExpr* currIdxExpr = expr.get();
    
    while (currIdxExpr) {
        chain.indices.insert(chain.indices.begin(), currIdxExpr->index.get());
        
        if (auto var = dynamic_cast<const Variable*>(currIdxExpr->array.get())) {
            chain.baseVariable = var->token.lexeme;
            break;
        }
        currIdxExpr = dynamic_cast<IndexExpr*>(currIdxExpr->array.get());
    }
    
    return chain;
}

std::unique_ptr<Expr> MIRElaboration::calculateFlatIndex(
    std::vector<std::unique_ptr<Expr>>& indices,
    const std::vector<int>& dimensions) {
    
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

std::unique_ptr<IndexExpr> MIRElaboration::transformMultiDimIndexing(std::unique_ptr<IndexExpr> node) {
    if (!isMultiDimensionalIndexing(node.get())) {
        return node;
    }
    
    IndexChain chain;
    std::vector<Expr*> rawIndices;
    IndexExpr* currIdxExpr = node.get();
    
    while (currIdxExpr) {
        rawIndices.insert(rawIndices.begin(), currIdxExpr->index.get());
        
        if (auto* var = dynamic_cast<const Variable*>(currIdxExpr->array.get())) {
            chain.baseVariable = var->token.lexeme;
            break;
        }
        currIdxExpr = dynamic_cast<IndexExpr*>(currIdxExpr->array.get());
    }
    
    
    auto dimIt = variableDimensions.find(chain.baseVariable);
    if (dimIt == variableDimensions.end()) {
        return node;
    }
    
    const std::vector<int>& dimensions = dimIt->second;
    
    if (rawIndices.size() != dimensions.size()) {
        return node;
    }
    
    std::vector<std::unique_ptr<Expr>> ownedIndices;
    for (Expr* rawIndex : rawIndices) {
        auto cloned = cloneExpr(rawIndex);
        if (!cloned) {

            return node;
        }
        ownedIndices.push_back(std::move(cloned));
    }
    
    auto flatIndex = calculateFlatIndex(ownedIndices, dimensions);
    if (!flatIndex) {
        return node;
    }
    
    Token baseToken(TokenType::Identifier, 0, 0, chain.baseVariable);
    auto baseVar = std::make_unique<Variable>(baseToken);
    
    auto flatIndexExpr = std::make_unique<IndexExpr>(
        std::move(baseVar),
        std::move(flatIndex)
    );
    
    return flatIndexExpr;
}

std::unique_ptr<Expr> MIRElaboration::cloneExpr(const Expr* expr) {
    if (auto* var = dynamic_cast<const Variable*>(expr)) {
        return std::make_unique<Variable>(var->token);
    }

    if (auto* lit = dynamic_cast<const Literal*>(expr)) {
        return std::make_unique<Literal>(lit->token);
    }

    if (auto* bin = dynamic_cast<const BinaryExpr*>(expr)) {
        return std::make_unique<BinaryExpr>(
            cloneExpr(bin->lhs.get()),
            cloneExpr(bin->rhs.get()),
            bin->op
        );
    }

    if (auto* un = dynamic_cast<const UnaryExpr*>(expr)) {
        return std::make_unique<UnaryExpr>(
            cloneExpr(un->operand.get()),
            un->op
        );
    }

    return nullptr;
}
