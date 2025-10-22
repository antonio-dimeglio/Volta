#include "../../include/Semantic/SemanticAnalyzer.hpp"
#include "../../include/Parser/AST.hpp"
#include <iostream>
#include <climits>
#include <set>

namespace Semantic {

SemanticAnalyzer::SemanticAnalyzer(Type::TypeRegistry& types, DiagnosticManager& diag)
    : typeRegistry(types), symbolTable(), diag(diag) {
}

void SemanticAnalyzer::analyzeProgram(const HIR::HIRProgram& program) {
    // Phase 1: Register all struct types first (so they're available for function signatures)
    registerStructTypes(program);

    // Phase 2: Resolve all unresolved types in the program
    resolveUnresolvedTypes(const_cast<HIR::HIRProgram&>(program));

    // Phase 3: Collect function signatures
    collectFunctionSignatures(program);

    // Phase 4: Analyze all statements
    for (const auto& stmt : program.statements) {
        stmt->accept(*this);
    }
}

const Type::Type* SemanticAnalyzer::visitLiteral(Literal& node) {
    const Type::Type* type = nullptr;
    switch (node.token.tt) {
        case TokenType::Integer: {
            // Parse the integer literal and infer its type based on value range
            try {
                int64_t const value = std::stoll(node.token.lexeme);

                // Check if it fits in i32 range
                if (value >= INT32_MIN && value <= INT32_MAX) {
                    type = typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
                } else {
                    // Needs i64
                    type = typeRegistry.getPrimitive(Type::PrimitiveKind::I64);
                }
            } catch (const std::out_of_range&) {
                // Value too large for signed i64, try unsigned u64
                try {
                    uint64_t const value = std::stoull(node.token.lexeme);
                    type = typeRegistry.getPrimitive(Type::PrimitiveKind::U64);
                } catch (const std::out_of_range&) {
                    diag.error("Integer literal out of range: " + node.token.lexeme,
                               node.token.line, node.token.column);
                    type = typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
                }
            }
            break;
        }
        case TokenType::Float:
            type = typeRegistry.getPrimitive(Type::PrimitiveKind::F64);
            break;
        case TokenType::True_:
        case TokenType::False_:
            type = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
            break;
        case TokenType::String:
            // String literals are ptr<i8> for compatibility with C
            type = typeRegistry.getPointer(typeRegistry.getPrimitive(Type::PrimitiveKind::I8));
            break;
        case TokenType::Null:
            // null literal has type ptr<opaque> which can be assigned to any pointer type
            type = typeRegistry.getPointer(typeRegistry.getOpaque());
            break;
        default:
            diag.error("Unknown literal type", node.line, node.column);
            type = errorType();
            break;
    }
    exprTypes[&node] = type;
    return type;
}

const Type::Type* SemanticAnalyzer::visitGroupingExpr(GroupingExpr& node) {
    const Type::Type* type = node.expr->accept(*this);
    exprTypes[&node] = type;
    return type;
}

const Type::Type* SemanticAnalyzer::visitVariable(Variable& node) {
    const Semantic::Symbol* symbol = symbolTable.lookup(node.token.lexeme);
    const Type::Type* type = nullptr;
    if (symbol == nullptr) {
        diag.error("Undefined variable: " + node.token.lexeme, node.line, node.column);
        type = errorType();
    } else {
        type = symbol->type;
    }
    exprTypes[&node] = type;
    return type;
}

void SemanticAnalyzer::visitBreakStmt(HIR::HIRBreakStmt& node) {
    if (!inLoop) {
        diag.error("'break' outside of loop", node.line, node.column);
    }
}

void SemanticAnalyzer::visitContinueStmt(HIR::HIRContinueStmt& node) {
    if (!inLoop) {
        diag.error("'continue' outside of loop", node.line, node.column);
    }
}

void SemanticAnalyzer::visitExprStmt(HIR::HIRExprStmt& node) {
    node.expr->accept(*this);
}

const Type::Type* SemanticAnalyzer::errorType() {
    return typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
}

void SemanticAnalyzer::visitWhileStmt(HIR::HIRWhileStmt& node) {
    const Type::Type* exprType = node.condition->accept(*this);
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);

    if (exprType != boolType) {
        diag.error("While condition must be boolean type", node.line, node.column);
    }

    bool const oldInLoop = inLoop;
    inLoop = true;

    // While body gets its own scope
    symbolTable.enterScope();
    for (auto& stmt : node.body) {
        stmt->accept(*this);
    }

    // Check increment expression if it exists (for desugared for loops)
    if (node.increment) {
        node.increment->accept(*this);
    }
    symbolTable.exitScope();

    inLoop = oldInLoop;
}

void SemanticAnalyzer::visitBlockStmt(HIR::HIRBlockStmt& node) {
    symbolTable.enterScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    symbolTable.exitScope();
}

// Helper to check if a statement list contains a return statement (recursively)
bool SemanticAnalyzer::containsReturn(const std::vector<std::unique_ptr<HIR::HIRStmt>>& stmts) {
    for (const auto& stmt : stmts) {
        // Direct return statement
        if (dynamic_cast<HIR::HIRReturnStmt*>(stmt.get())) {
            return true;
        }

        // Check inside if statements
        if (auto* ifStmt = dynamic_cast<HIR::HIRIfStmt*>(stmt.get())) {
            if (containsReturn(ifStmt->thenBody) || containsReturn(ifStmt->elseBody)) {
                return true;
            }
        }

        // Check inside while loops
        if (auto* whileStmt = dynamic_cast<HIR::HIRWhileStmt*>(stmt.get())) {
            if (containsReturn(whileStmt->body)) {
                return true;
            }
        }

        // Check inside block statements
        if (auto* blockStmt = dynamic_cast<HIR::HIRBlockStmt*>(stmt.get())) {
            if (containsReturn(blockStmt->statements)) {
                return true;
            }
        }
    }
    return false;
}

// Helper to validate and coerce numeric literals (integer and float, including negative via unary minus)
// Returns true if the expr was a literal that was successfully coerced, false otherwise
bool SemanticAnalyzer::tryCoerceIntegerLiteral(Expr* expr, const Type::Type* targetType) {
    // Direct literal (integer or float)
    if (auto* literal = dynamic_cast<Literal*>(expr)) {
        // Integer literal
        if (literal->token.tt == TokenType::Integer) {
            try {
                int64_t const value = std::stoll(literal->token.lexeme);
                if (doesLiteralFitInType(value, targetType)) {
                    exprTypes[literal] = targetType;
                    return true;
                }
            } catch (const std::out_of_range&) {
                // Fall through to return false
            }
        }
        // Float literal - can be coerced to any float type
        else if (literal->token.tt == TokenType::Float) {
            if (isFloatType(targetType)) {
                exprTypes[literal] = targetType;
                return true;
            }
        }
    }
    // Negative literal (unary minus applied to literal)
    else if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        if (unary->op == TokenType::Minus) {
            if (auto* literal = dynamic_cast<Literal*>(unary->operand.get())) {
                // Negative integer literal
                if (literal->token.tt == TokenType::Integer) {
                    try {
                        int64_t value = -std::stoll(literal->token.lexeme);
                        if (doesLiteralFitInType(value, targetType)) {
                            exprTypes[unary] = targetType;
                            return true;
                        }
                    } catch (const std::out_of_range&) {
                        // Fall through to return false
                    }
                }
                // Negative float literal
                else if (literal->token.tt == TokenType::Float) {
                    if (isFloatType(targetType)) {
                        exprTypes[unary] = targetType;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void SemanticAnalyzer::visitReturnStmt(HIR::HIRReturnStmt& node) {
    if (currentReturnType == nullptr) {
        diag.error("'return' outside of function definition", node.line, node.column);
        return;
    }

    if (node.value) {
        const Type::Type* retValueType = node.value->accept(*this);

        // Try to coerce integer literals to the return type
        if (tryCoerceIntegerLiteral(node.value.get(), currentReturnType)) {
            retValueType = currentReturnType;
        }

        if (!isImplicitlyConvertible(retValueType, currentReturnType)) {
            diag.error("Return type mismatch: cannot implicitly convert " +
                      retValueType->toString() + " to " + currentReturnType->toString(),
                      node.line, node.column);
        }
    } else{
        const Type::Type* voidType = typeRegistry.getPrimitive(Type::PrimitiveKind::Void);
        if (currentReturnType != voidType) {
            diag.error("Missing return value in non-void function", node.line, node.column);
        }
    }
}

void SemanticAnalyzer::visitVarDecl(HIR::HIRVarDecl& node) {
    const Type::Type* varType = nullptr;

    if (node.typeAnnotation != nullptr) {
        varType = node.typeAnnotation;

        if (node.initValue) {
            const Type::Type* initType = node.initValue->accept(*this);

            // Try to coerce integer literals to the variable type
            if (tryCoerceIntegerLiteral(node.initValue.get(), varType)) {
                initType = varType;
            }

            if (!isImplicitlyConvertible(initType, varType)) {
                diag.error("Type mismatch: cannot implicitly convert " + initType->toString() +
                          " to " + varType->toString(),
                          node.line, node.column);
                return;
            }
        }
    } else if (node.initValue) {
        varType = node.initValue->accept(*this);
    } else {
        diag.error("Variable must have type annotation or initializer", node.line, node.column);
        return;
    }

    if (!symbolTable.define(node.name.lexeme, varType, node.mutable_)) {
        diag.error("Variable '" + node.name.lexeme + "' already defined", node.line, node.column);
    }
}

void SemanticAnalyzer::visitFnDecl(HIR::HIRFnDecl& node) {
    if (node.isExtern) {
        return;
    }

    symbolTable.enterFunction();
    // Convert struct return types to pointers (all structs are heap-allocated)
    currentReturnType = node.returnType;
    if (currentReturnType->kind == Type::TypeKind::Struct) {
        currentReturnType = typeRegistry.getPointer(currentReturnType);
    }
    currentFunctionName = node.name;

    for (const auto& param : node.params) {
        bool isMut = param.isMutRef;
        if (!symbolTable.define(param.name, param.type, isMut)) {
            diag.error("Duplicate parameter name: " + param.name, node.line, node.column);
        }
    }

    for (const auto& stmt : node.body) {
        stmt->accept(*this);
    }

    // Check if non-void function is missing a return statement (recursively check nested blocks)
    const Type::Type* voidType = typeRegistry.getPrimitive(Type::PrimitiveKind::Void);
    if (currentReturnType != voidType && !containsReturn(node.body)) {
        diag.error("Function '" + node.name + "' must return a value of type " +
                  currentReturnType->toString(), node.line, node.column);
    }

    currentReturnType = nullptr;
    currentFunctionName = "";
    symbolTable.exitFunction();
}

void SemanticAnalyzer::visitIfStmt(HIR::HIRIfStmt& node) {
    const Type::Type* condType = node.condition->accept(*this);
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);

    if (condType != boolType) {
        diag.error("If condition must be boolean", node.line, node.column);
    }

    // Then body gets its own scope
    symbolTable.enterScope();
    for (auto& stmt : node.thenBody) {
        stmt->accept(*this);
    }
    symbolTable.exitScope();

    // Else body gets its own scope
    if (!node.elseBody.empty()) {
        symbolTable.enterScope();
        for (auto& stmt : node.elseBody) {
            stmt->accept(*this);
        }
        symbolTable.exitScope();
    }
}

const Type::Type* SemanticAnalyzer::visitBinaryExpr(BinaryExpr& node) {
    const Type::Type* lhsType = node.lhs->accept(*this);
    const Type::Type* rhsType = node.rhs->accept(*this);

    const Type::Type* resultType = nullptr;

    // Handle literal coercion for type matching
    if (lhsType != rhsType) {
        bool lhsIsLiteral = dynamic_cast<Literal*>(node.lhs.get()) != nullptr;
        bool rhsIsLiteral = dynamic_cast<Literal*>(node.rhs.get()) != nullptr;

        // Try to coerce integer literals to match the other operand type
        if (lhsIsLiteral) {
            if (auto* literal = dynamic_cast<Literal*>(node.lhs.get())) {
                if (literal->token.tt == TokenType::Integer && isIntegerType(rhsType)) {
                    try {
                        int64_t value = std::stoll(literal->token.lexeme);
                        if (doesLiteralFitInType(value, rhsType)) {
                            exprTypes[literal] = rhsType;
                            lhsType = rhsType;
                        }
                    } catch (const std::out_of_range&) {
                        // Fall through to type mismatch error
                    }
                }
            }
        } else if (rhsIsLiteral) {
            if (auto* literal = dynamic_cast<Literal*>(node.rhs.get())) {
                if (literal->token.tt == TokenType::Integer && isIntegerType(lhsType)) {
                    try {
                        int64_t value = std::stoll(literal->token.lexeme);
                        if (doesLiteralFitInType(value, lhsType)) {
                            exprTypes[literal] = lhsType;
                            rhsType = lhsType;
                        }
                    } catch (const std::out_of_range&) {
                        // Fall through to type mismatch error
                    }
                }
            }
        }
    }

    if (lhsType != rhsType) {
        diag.error("Type mismatch in binary expression: " + lhsType->toString() +
                  " and " + rhsType->toString() + " are not compatible",
                  node.line, node.column);
        resultType = errorType();
    } else {
        switch (node.op) {
            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Mult:
            case TokenType::Div:
            case TokenType::Modulo:
                if (!isNumericType(lhsType)) {
                    diag.error("Arithmetic requires numeric types", node.line, node.column);
                    resultType = errorType();
                } else {
                    resultType = lhsType;
                }
                break;

            case TokenType::LessThan:
            case TokenType::GreaterThan:
            case TokenType::LessEqual:
            case TokenType::GreaterEqual:
            case TokenType::EqualEqual:
            case TokenType::NotEqual:
                resultType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
                break;

            case TokenType::And:
            case TokenType::Or:
                if (lhsType != typeRegistry.getPrimitive(Type::PrimitiveKind::Bool)) {
                    diag.error("Logical operators require boolean types", node.line, node.column);
                    resultType = errorType();
                } else {
                    resultType = lhsType;
                }
                break;

            default:
                diag.error("Unknown binary operator", node.line, node.column);
                resultType = errorType();
                break;
        }
    }

    exprTypes[&node] = resultType;
    return resultType;
}

const Type::Type* SemanticAnalyzer::visitUnaryExpr(UnaryExpr& node) {
    const Type::Type* operandType = node.operand->accept(*this);

    const Type::Type* resultType = nullptr;

    switch (node.op) {
        case TokenType::Minus:
            if (!isNumericType(operandType)) {
                diag.error("Unary '-' requires numeric type", node.line, node.column);
                resultType = errorType();
            } else {
                resultType = operandType;
            }
            break;

        case TokenType::Not: {
            const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
            if (operandType != boolType) {
                diag.error("Logical NOT requires boolean type", node.line, node.column);
                resultType = errorType();
            } else {
                resultType = boolType;
            }
            break;
        }

        default:
            diag.error("Unknown unary operator", node.line, node.column);
            resultType = errorType();
            break;
    }

    exprTypes[&node] = resultType;
    return resultType;
}

const Type::Type* SemanticAnalyzer::visitAssignment(Assignment& node) {
    // Check if LHS is a variable
    if (auto* varNode = dynamic_cast<Variable*>(node.lhs.get())) {
        const Symbol* sym = symbolTable.lookup(varNode->token.lexeme);
        if (sym == nullptr) {
            diag.error("Undefined variable: " + varNode->token.lexeme, node.line, node.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }

        if (!sym->is_mut) {
            diag.error("Cannot assign to immutable variable: " + varNode->token.lexeme,
                      node.line, node.column);
        }

        const Type::Type* valueType = node.value->accept(*this);
        if (sym->type != valueType) {
            diag.error("Type mismatch in assignment", node.line, node.column);
        }

        exprTypes[&node] = valueType;
        return valueType;
    }
    // Check if LHS is an array index expression
    else if (auto* indexNode = dynamic_cast<IndexExpr*>(node.lhs.get())) {
        // Type check the index expression
        const Type::Type* lhsType = indexNode->accept(*this);
        const Type::Type* valueType = node.value->accept(*this);

        if (lhsType != valueType) {
            diag.error("Type mismatch in assignment", node.line, node.column);
        }

        exprTypes[&node] = valueType;
        return valueType;
    }
    // Check if LHS is a struct field access expression
    else if (auto* fieldAccess = dynamic_cast<FieldAccess*>(node.lhs.get())) {
        // Type check the field access
        const Type::Type* lhsType = fieldAccess->accept(*this);
        const Type::Type* valueType = node.value->accept(*this);

        // Check for integer literal and validate range
        if (auto* literal = dynamic_cast<Literal*>(node.value.get())) {
            if (literal->token.tt == TokenType::Integer) {
                try {
                    int64_t value = std::stoll(literal->token.lexeme);
                    if (doesLiteralFitInType(value, lhsType)) {
                        exprTypes[literal] = lhsType;
                        valueType = lhsType;
                    } else {
                        diag.error("Literal value " + literal->token.lexeme +
                                 " out of range for field type " + lhsType->toString(),
                                 node.line, node.column);
                        exprTypes[&node] = valueType;
                        return valueType;
                    }
                } catch (const std::out_of_range&) {
                    diag.error("Integer literal out of range", node.line, node.column);
                }
            }
        }

        if (!isImplicitlyConvertible(valueType, lhsType)) {
            diag.error("Type mismatch in field assignment: cannot convert " +
                      valueType->toString() + " to " + lhsType->toString(),
                      node.line, node.column);
        }

        exprTypes[&node] = valueType;
        return valueType;
    }
    else {
        diag.error("Left side of assignment must be a variable, array element, or struct field", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }
}

const Type::Type* SemanticAnalyzer::visitFnCall(FnCall& node) {
    const FunctionSignature* sig = symbolTable.lookupFunction(node.name);

    // If not found locally, check the function registry (for cross-module calls)
    if ((sig == nullptr) && (functionRegistry != nullptr)) {
        const Semantic::GlobalFunctionSignature* globalSig = functionRegistry->getFunction(node.name);
        if (globalSig != nullptr) {
            // Convert global function signature to local format for validation
            // Note: We just check the types here; the actual function is in another module
            if (node.args.size() != globalSig->parameters.size()) {
                diag.error("Function '" + node.name + "' expects " +
                          std::to_string(globalSig->parameters.size()) + " arguments, got " +
                          std::to_string(node.args.size()), node.line, node.column);
                const Type::Type* type = errorType();
                exprTypes[&node] = type;
                return type;
            }

            for (size_t i = 0; i < node.args.size(); i++) {
                const Type::Type* argType = node.args[i]->accept(*this);
                const Type::Type* paramType = globalSig->parameters[i].type;

                // Check for integer literal and validate range
                if (auto* literal = dynamic_cast<Literal*>(node.args[i].get())) {
                    if (literal->token.tt == TokenType::Integer) {
                        try {
                            int64_t value = std::stoll(literal->token.lexeme);
                            if (doesLiteralFitInType(value, paramType)) {
                                exprTypes[literal] = paramType;
                                argType = paramType;
                            } else {
                                diag.error("Argument " + std::to_string(i+1) + " literal value " +
                                         literal->token.lexeme + " out of range for parameter type " +
                                         paramType->toString(), node.line, node.column);
                                continue;
                            }
                        } catch (const std::out_of_range&) {
                            diag.error("Argument " + std::to_string(i+1) + " literal out of range",
                                     node.line, node.column);
                            continue;
                        }
                    }
                }

                // Check type compatibility
                bool canConvert = isImplicitlyConvertible(argType, paramType);

                // Special case for extern functions: allow struct -> ptr<opaque> conversion
                // This enables C FFI where structs are passed as opaque pointers
                if (!canConvert && globalSig->isExtern) {
                    if (argType->kind == Type::TypeKind::Struct &&
                        paramType->kind == Type::TypeKind::Pointer) {
                        const auto* ptrType = dynamic_cast<const Type::PointerType*>(paramType);
                        if (ptrType->pointeeType->kind == Type::TypeKind::Opaque) {
                            canConvert = true;  // Struct can be passed as ptr<opaque> to extern functions
                        }
                    }
                }

                if (!canConvert) {
                    diag.error("Argument " + std::to_string(i+1) + " type mismatch: cannot convert " +
                              argType->toString() + " to " + paramType->toString() +
                              " in call to '" + node.name + "'", node.line, node.column);
                }
            }

            exprTypes[&node] = globalSig->return_type;
            return globalSig->return_type;
        }
    }

    if (sig == nullptr) {
        diag.error("Undefined function: " + node.name, node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    if (node.args.size() != sig->parameters.size()) {
        diag.error("Function '" + node.name + "' expects " +
                  std::to_string(sig->parameters.size()) + " arguments, got " +
                  std::to_string(node.args.size()), node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    for (size_t i = 0; i < node.args.size(); i++) {
        const Type::Type* argType = node.args[i]->accept(*this);
        const Type::Type* paramType = sig->parameters[i].type;

        // Check if parameter requires a mutable reference
        if (sig->parameters[i].is_mut_ref) {
            // Argument must be a mutable variable
            if (auto* varExpr = dynamic_cast<Variable*>(node.args[i].get())) {
                const Symbol* sym = symbolTable.lookup(varExpr->token.lexeme);
                if ((sym != nullptr) && !sym->is_mut) {
                    diag.error("Cannot pass immutable variable '" + varExpr->token.lexeme +
                              "' as mutable reference to parameter " + std::to_string(i+1),
                              node.args[i]->line, node.args[i]->column);
                }
            } else {
                diag.error("Argument " + std::to_string(i+1) +
                          " must be a mutable variable for mut ref parameter",
                          node.args[i]->line, node.args[i]->column);
            }
        }

        // Check for integer literal and validate range
        if (auto* literal = dynamic_cast<Literal*>(node.args[i].get())) {
            if (literal->token.tt == TokenType::Integer) {
                try {
                    int64_t value = std::stoll(literal->token.lexeme);
                    if (doesLiteralFitInType(value, paramType)) {
                        exprTypes[literal] = paramType;
                        argType = paramType;
                    } else {
                        diag.error("Argument " + std::to_string(i+1) + " literal value " +
                                 literal->token.lexeme + " out of range for parameter type " +
                                 paramType->toString(), node.line, node.column);
                        continue;
                    }
                } catch (const std::out_of_range&) {
                    diag.error("Argument " + std::to_string(i+1) + " literal out of range",
                             node.line, node.column);
                    continue;
                }
            }
        }

        // Check type compatibility
        bool canConvert = isImplicitlyConvertible(argType, paramType);

        // Special case for extern functions: allow struct -> ptr<opaque> conversion
        // This enables C FFI where structs are passed as opaque pointers
        if (!canConvert && sig->isExtern) {
            if (argType->kind == Type::TypeKind::Struct &&
                paramType->kind == Type::TypeKind::Pointer) {
                const auto* ptrType = dynamic_cast<const Type::PointerType*>(paramType);
                if (ptrType->pointeeType->kind == Type::TypeKind::Opaque) {
                    canConvert = true;  // Struct can be passed as ptr<opaque> to extern functions
                }
            }
        }

        if (!canConvert) {
            diag.error("Argument " + std::to_string(i+1) + " type mismatch: cannot convert " +
                      argType->toString() + " to " + paramType->toString() +
                      " in call to '" + node.name + "'", node.line, node.column);
        }
    }

    exprTypes[&node] = sig->return_type;
    return sig->return_type;
}

const Type::Type* SemanticAnalyzer::visitArrayLiteral(ArrayLiteral& node) {
    if (node.elements.empty()) {
        diag.error("Cannot infer type of empty array", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    const Type::Type* elementType = node.elements[0]->accept(*this);

    for (size_t i = 1; i < node.elements.size(); i++) {
        const Type::Type* elemType = node.elements[i]->accept(*this);
        if (elemType != elementType) {
            diag.error("Array elements must have uniform type", node.line, node.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }
    }

    const Type::Type* arrayType = typeRegistry.getArray(elementType, node.elements.size());
    exprTypes[&node] = arrayType;
    return arrayType;
}

const Type::Type* SemanticAnalyzer::visitIndexExpr(IndexExpr& node) {
    const Type::Type* arrayType = node.array->accept(*this);

    if (arrayType->kind != Type::TypeKind::Array) {
        diag.error("Cannot index non-array type", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    const Type::Type* indexType = node.index->accept(*this);
    if (!isIntegerType(indexType)) {
        diag.error("Array index must be integer type", node.line, node.column);
    }

    auto* arrType = dynamic_cast<const Type::ArrayType*>(arrayType);

    // Compile-time bounds checking for literal indices
    int64_t literalIndex = -1;
    bool isLiteral = false;

    // Check for direct integer literal
    if (auto* lit = dynamic_cast<Literal*>(node.index.get())) {
        if (lit->token.tt == TokenType::Integer) {
            try {
                literalIndex = std::stoll(lit->token.lexeme);
                isLiteral = true;
            } catch (const std::out_of_range&) {
                // Literal is too large to fit in int64_t, will definitely be out of bounds
                diag.error("Array index literal is out of range", node.line, node.column);
                const Type::Type* type = errorType();
                exprTypes[&node] = type;
                return type;
            }
        }
    }

    // Perform bounds check if we have a literal index
    if (isLiteral) {
        if (literalIndex < 0 || literalIndex >= static_cast<int64_t>(arrType->size)) {
            diag.error("Array index " + std::to_string(literalIndex) +
                      " is out of bounds for array of size " + std::to_string(arrType->size),
                      node.line, node.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }
    }

    exprTypes[&node] = arrType->elementType;
    return arrType->elementType;
}

const Type::Type* SemanticAnalyzer::visitAddrOf(AddrOf& node) {
    const Type::Type* operandType = node.operand->accept(*this);
    const Type::Type* ptrType = typeRegistry.getPointer(operandType);
    exprTypes[&node] = ptrType;
    return ptrType;
}

void SemanticAnalyzer::visitExternBlock(HIR::HIRExternBlock& node) {
    // No-op: functions already registered in first pass
}

void SemanticAnalyzer::visitImportStmt(HIR::HIRImportStmt& node) {
    // No-op: imports handled elsewhere
}

void SemanticAnalyzer::visitStructDecl(HIR::HIRStructDecl& node) {
    // Structs are registered in registerStructTypes() during first pass
    // Now we need to analyze the method bodies
    for (auto& method : node.methods) {
        visitFnDecl(*method);
    }
}

const Type::Type* SemanticAnalyzer::visitCompoundAssign(CompoundAssign& node) {
    diag.error("Compound assignment should be desugared by HIR lowering", node.line, node.column);
    const Type::Type* type = errorType();
    exprTypes[&node] = type;
    return type;
}

const Type::Type* SemanticAnalyzer::visitIncrement(Increment& node) {
    diag.error("Increment should be desugared by HIR lowering", node.line, node.column);
    const Type::Type* type = errorType();
    exprTypes[&node] = type;
    return type;
}

const Type::Type* SemanticAnalyzer::visitDecrement(Decrement& node) {
    diag.error("Decrement should be desugared by HIR lowering", node.line, node.column);
    const Type::Type* type = errorType();
    exprTypes[&node] = type;
    return type;
}

const Type::Type* SemanticAnalyzer::visitRange(Range& node) {
    const Type::Type* startType = node.from->accept(*this);
    const Type::Type* endType = node.to->accept(*this);

    if (!isIntegerType(startType) || !isIntegerType(endType)) {
        diag.error("Range bounds must be integer types", node.line, node.column);
    }

    const Type::Type* rangeType = typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
    exprTypes[&node] = rangeType;
    return rangeType;
}

const Type::Type* SemanticAnalyzer::visitStructLiteral(StructLiteral& node) {
    // 1. Look up the struct type by name
    auto* structType = typeRegistry.getStruct(node.structName.lexeme);
    if (structType == nullptr) {
        diag.error("Unknown struct type: " + node.structName.lexeme,
                   node.structName.line, node.structName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 2. Check that all required fields are provided
    std::set<std::string> providedFields;
    for (const auto& [fieldName, fieldValue] : node.fields) {
        providedFields.insert(fieldName.lexeme);
    }

    std::set<std::string> requiredFields;
    for (const auto& field : structType->fields) {
        requiredFields.insert(field.name);
    }

    // Check for missing fields
    for (const auto& requiredField : requiredFields) {
        if (providedFields.find(requiredField) == providedFields.end()) {
            diag.error("Missing field '" + requiredField + "' in struct literal",
                       node.structName.line, node.structName.column);
        }
    }

    // Check for extra fields
    for (const auto& providedField : providedFields) {
        if (requiredFields.find(providedField) == requiredFields.end()) {
            diag.error("Unknown field '" + providedField + "' in struct '" + node.structName.lexeme + "'",
                       node.structName.line, node.structName.column);
        }
    }

    // 3. Type-check each field value
    for (auto& [fieldName, fieldValue] : node.fields) {
        const Type::Type* expectedType = structType->getFieldType(fieldName.lexeme);
        if (expectedType) {
            const Type::Type* actualType = fieldValue->accept(*this);

            // Try to coerce numeric literals (both int and float) to the target type
            if (tryCoerceIntegerLiteral(fieldValue.get(), expectedType)) {
                actualType = expectedType;
            }

            if (!isImplicitlyConvertible(actualType, expectedType)) {
                diag.error("Type mismatch for field '" + fieldName.lexeme + "': cannot convert " +
                          actualType->toString() + " to " + expectedType->toString(),
                           fieldName.line, fieldName.column);
            }
        }
    }

    // 4. Store resolved type
    node.resolvedType = structType;
    exprTypes[&node] = structType;
    return structType;
}

const Type::Type* SemanticAnalyzer::visitFieldAccess(FieldAccess& node) {
    // 1. Analyze the object expression to get its type
    const Type::Type* objectType = node.object->accept(*this);

    // 2. Handle both struct values and pointers to structs
    const Type::StructType* structType = nullptr;
    if (objectType->kind == Type::TypeKind::Struct) {
        structType = dynamic_cast<const Type::StructType*>(objectType);
    } else if (objectType->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = dynamic_cast<const Type::PointerType*>(objectType);
        if (ptrType->pointeeType->kind == Type::TypeKind::Struct) {
            structType = static_cast<const Type::StructType*>(ptrType->pointeeType);
        }
    }

    if (structType == nullptr) {
        diag.error("Field access on non-struct type", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 3. Look up the field in the struct
    const Type::Type* fieldType = structType->getFieldType(node.fieldName.lexeme);

    if (fieldType == nullptr) {
        diag.error("Unknown field '" + node.fieldName.lexeme + "' in struct '" + structType->name + "'",
                   node.fieldName.line, node.fieldName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 4. Check field visibility
    if (!structType->isFieldPublic(node.fieldName.lexeme)) {
        diag.error("Cannot access private field '" + node.fieldName.lexeme + "' in struct '" + structType->name + "'",
                   node.fieldName.line, node.fieldName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 5. Store resolved info for MIR lowering
    node.resolvedStructType = structType;

    // Find field index for efficient codegen
    for (size_t i = 0; i < structType->fields.size(); ++i) {
        if (structType->fields[i].name == node.fieldName.lexeme) {
            node.fieldIndex = static_cast<int>(i);
            break;
        }
    }

    exprTypes[&node] = fieldType;
    return fieldType;
}

const Type::Type* SemanticAnalyzer::visitStaticMethodCall(StaticMethodCall& node) {
    // 1. Look up the struct type
    const auto* structType = typeRegistry.getStruct(node.typeName.lexeme);
    if (structType == nullptr) {
        diag.error("Unknown type '" + node.typeName.lexeme + "'",
                   node.typeName.line, node.typeName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 2. Look up the method in the struct type
    const auto* methodSig = structType->getMethod(node.methodName.lexeme);
    if (methodSig == nullptr) {
        diag.error("Unknown method '" + node.methodName.lexeme + "' in struct '" + structType->name + "'",
                   node.methodName.line, node.methodName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 3. Verify it's a static method (no self parameter)
    if (methodSig->hasSelf) {
        diag.error("Cannot call instance method '" + node.methodName.lexeme + "' as static method",
                   node.methodName.line, node.methodName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 4. Check argument count
    if (node.args.size() != methodSig->paramTypes.size()) {
        diag.error("Expected " + std::to_string(methodSig->paramTypes.size()) +
                   " arguments but got " + std::to_string(node.args.size()),
                   node.methodName.line, node.methodName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 5. Type check each argument
    for (size_t i = 0; i < node.args.size(); ++i) {
        const Type::Type* argType = node.args[i]->accept(*this);
        const Type::Type* paramType = methodSig->paramTypes[i];

        // Check for integer literal and validate range
        if (auto* literal = dynamic_cast<Literal*>(node.args[i].get())) {
            if (literal->token.tt == TokenType::Integer) {
                try {
                    int64_t value = std::stoll(literal->token.lexeme);
                    if (doesLiteralFitInType(value, paramType)) {
                        exprTypes[literal] = paramType;
                        argType = paramType;
                    } else {
                        diag.error("Argument " + std::to_string(i+1) + " literal value " +
                                 literal->token.lexeme + " out of range for parameter type " +
                                 paramType->toString(), node.args[i]->line, node.args[i]->column);
                        continue;
                    }
                } catch (const std::out_of_range&) {
                    diag.error("Argument " + std::to_string(i+1) + " literal out of range",
                             node.args[i]->line, node.args[i]->column);
                    continue;
                }
            }
        }

        if (!isImplicitlyConvertible(argType, paramType)) {
            diag.error("Argument " + std::to_string(i + 1) + " type mismatch: cannot convert " +
                      argType->toString() + " to " + paramType->toString() +
                      " in method call", node.args[i]->line, node.args[i]->column);
        }
    }

    exprTypes[&node] = methodSig->returnType;
    return methodSig->returnType;
}

const Type::Type* SemanticAnalyzer::visitInstanceMethodCall(InstanceMethodCall& node) {
    // Note: Point.new() style calls are desugared to StaticMethodCall during HIR lowering,
    // so by the time we reach here, this is truly an instance method call (e.g., p.distance())

    // 1. Analyze the object expression to get its type
    const Type::Type* objectType = node.object->accept(*this);

    // 2. Handle both struct values and pointers to structs
    const Type::StructType* structType = nullptr;
    if (objectType->kind == Type::TypeKind::Struct) {
        structType = dynamic_cast<const Type::StructType*>(objectType);
    } else if (objectType->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = dynamic_cast<const Type::PointerType*>(objectType);
        if (ptrType->pointeeType->kind == Type::TypeKind::Struct) {
            structType = static_cast<const Type::StructType*>(ptrType->pointeeType);
        }
    }

    if (structType == nullptr) {
        diag.error("Method call on non-struct type", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 3. Look up the method in the struct type
    const auto* methodSig = structType->getMethod(node.methodName.lexeme);
    if (methodSig == nullptr) {
        diag.error("Unknown method '" + node.methodName.lexeme + "' in struct '" + structType->name + "'",
                   node.methodName.line, node.methodName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 4. Verify it's an instance method (has self parameter)
    if (!methodSig->hasSelf) {
        diag.error("Cannot call static method '" + node.methodName.lexeme + "' as instance method",
                   node.methodName.line, node.methodName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 5. Check argument count (excluding self)
    size_t expectedArgCount = methodSig->paramTypes.size() - 1; // Subtract self parameter
    if (node.args.size() != expectedArgCount) {
        diag.error("Expected " + std::to_string(expectedArgCount) +
                   " arguments but got " + std::to_string(node.args.size()),
                   node.methodName.line, node.methodName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 6. Type check each argument (skip first param which is self)
    for (size_t i = 0; i < node.args.size(); ++i) {
        const Type::Type* argType = node.args[i]->accept(*this);
        const Type::Type* expectedType = methodSig->paramTypes[i + 1]; // +1 to skip self

        // Check for integer literal and validate range
        if (auto* literal = dynamic_cast<Literal*>(node.args[i].get())) {
            if (literal->token.tt == TokenType::Integer) {
                try {
                    int64_t value = std::stoll(literal->token.lexeme);
                    if (doesLiteralFitInType(value, expectedType)) {
                        exprTypes[literal] = expectedType;
                        argType = expectedType;
                    } else {
                        diag.error("Argument " + std::to_string(i+1) + " literal value " +
                                 literal->token.lexeme + " out of range for parameter type " +
                                 expectedType->toString(), node.args[i]->line, node.args[i]->column);
                        continue;
                    }
                } catch (const std::out_of_range&) {
                    diag.error("Argument " + std::to_string(i+1) + " literal out of range",
                             node.args[i]->line, node.args[i]->column);
                    continue;
                }
            }
        }

        if (!isImplicitlyConvertible(argType, expectedType)) {
            diag.error("Argument " + std::to_string(i + 1) + " type mismatch: cannot convert " +
                      argType->toString() + " to " + expectedType->toString() +
                      " in method call", node.args[i]->line, node.args[i]->column);
        }
    }

    exprTypes[&node] = methodSig->returnType;
    return methodSig->returnType;
}

void SemanticAnalyzer::collectFunctionSignatures(const HIR::HIRProgram& program) {
    for (const auto& stmt : program.statements) {
        if (auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(stmt.get())) {
            registerFunction(*fnDecl);
        } else if (auto* externBlock = dynamic_cast<HIR::HIRExternBlock*>(stmt.get())) {
            for (const auto& fn : externBlock->declarations) {
                registerFunction(*fn);
            }
        }
    }
}

void SemanticAnalyzer::registerFunction(HIR::HIRFnDecl& node) {
    std::vector<FunctionParameter> params;
    for (const auto& p : node.params) {
        params.push_back(FunctionParameter(p.name, p.type, p.isRef, p.isMutRef));
    }

    FunctionSignature sig(params, node.returnType, node.isExtern);

    if (!symbolTable.addFunction(node.name, sig)) {
        diag.error("Function '" + node.name + "' already defined", node.line, node.column);
    }
}

bool SemanticAnalyzer::isNumericType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) { return false;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(type);
    return prim->kind == Type::PrimitiveKind::I32 ||
           prim->kind == Type::PrimitiveKind::I64 ||
           prim->kind == Type::PrimitiveKind::U32 ||
           prim->kind == Type::PrimitiveKind::U64 ||
           prim->kind == Type::PrimitiveKind::F32 ||
           prim->kind == Type::PrimitiveKind::F64;
}

bool SemanticAnalyzer::isIntegerType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) { return false;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(type);
    return prim->kind == Type::PrimitiveKind::I32 ||
           prim->kind == Type::PrimitiveKind::I64 ||
           prim->kind == Type::PrimitiveKind::U32 ||
           prim->kind == Type::PrimitiveKind::U64 ||
           prim->kind == Type::PrimitiveKind::I8 ||
           prim->kind == Type::PrimitiveKind::I16 ||
           prim->kind == Type::PrimitiveKind::U8 ||
           prim->kind == Type::PrimitiveKind::U16;
}

// ============================================================================
// Type Utility Functions
// ============================================================================

bool SemanticAnalyzer::isSignedIntegerType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) { return false;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(type);
    return prim->isSigned();
}

bool SemanticAnalyzer::isUnsignedIntegerType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) { return false;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(type);
    return prim->isUnsigned();
}

bool SemanticAnalyzer::isFloatType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) { return false;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(type);
    return prim->kind == Type::PrimitiveKind::F32 ||
           prim->kind == Type::PrimitiveKind::F64;
}

int SemanticAnalyzer::getTypeBitWidth(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) { return 0;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(type);

    switch (prim->kind) {
        case Type::PrimitiveKind::I8:
        case Type::PrimitiveKind::U8:
        case Type::PrimitiveKind::Bool:
            return 8;
        case Type::PrimitiveKind::I16:
        case Type::PrimitiveKind::U16:
            return 16;
        case Type::PrimitiveKind::I32:
        case Type::PrimitiveKind::U32:
        case Type::PrimitiveKind::F32:
            return 32;
        case Type::PrimitiveKind::I64:
        case Type::PrimitiveKind::U64:
        case Type::PrimitiveKind::F64:
            return 64;
        default:
            return 0;
    }
}

bool SemanticAnalyzer::areTypesEqual(const Type::Type* a, const Type::Type* b) {
    // Pointer equality is sufficient since TypeRegistry ensures type uniqueness
    return a == b;
}

bool SemanticAnalyzer::haveSameSignedness(const Type::Type* from, const Type::Type* to) {
    // Non-integer types are considered to have compatible signedness
    if (!isIntegerType(from) || !isIntegerType(to)) { return true;
}

    return (isSignedIntegerType(from) && isSignedIntegerType(to)) ||
           (isUnsignedIntegerType(from) && isUnsignedIntegerType(to));
}

bool SemanticAnalyzer::isWideningConversion(const Type::Type* from, const Type::Type* to) {
    if (from->kind != Type::TypeKind::Primitive || to->kind != Type::TypeKind::Primitive) {
        return false;
    }

    int const fromWidth = getTypeBitWidth(from);
    int const toWidth = getTypeBitWidth(to);

    return fromWidth > 0 && toWidth > fromWidth;
}

bool SemanticAnalyzer::isNarrowingConversion(const Type::Type* from, const Type::Type* to) {
    if (from->kind != Type::TypeKind::Primitive || to->kind != Type::TypeKind::Primitive) {
        return false;
    }

    int const fromWidth = getTypeBitWidth(from);
    int const toWidth = getTypeBitWidth(to);

    return fromWidth > 0 && toWidth > 0 && toWidth < fromWidth;
}

bool SemanticAnalyzer::doesLiteralFitInType(int64_t value, const Type::Type* targetType) {
    if (targetType->kind != Type::TypeKind::Primitive) { return false;
}
    const auto* prim = dynamic_cast<const Type::PrimitiveType*>(targetType);

    switch (prim->kind) {
        case Type::PrimitiveKind::I8:
            return value >= INT8_MIN && value <= INT8_MAX;
        case Type::PrimitiveKind::U8:
            return value >= 0 && value <= UINT8_MAX;
        case Type::PrimitiveKind::I16:
            return value >= INT16_MIN && value <= INT16_MAX;
        case Type::PrimitiveKind::U16:
            return value >= 0 && value <= UINT16_MAX;
        case Type::PrimitiveKind::I32:
            return value >= INT32_MIN && value <= INT32_MAX;
        case Type::PrimitiveKind::U32:
            return value >= 0 && value <= UINT32_MAX;
        case Type::PrimitiveKind::I64:
            return true; // int64_t can hold any int64_t value
        case Type::PrimitiveKind::U64:
            return value >= 0; // unsigned requires non-negative
        default:
            return false;
    }
}

bool SemanticAnalyzer::isImplicitlyConvertible(const Type::Type* from, const Type::Type* to) {
    // Exact type match - always allowed
    if (areTypesEqual(from, to)) { return true;
}

    // Special case: ptr<opaque> conversions (bi-directional)
    if (from->kind == Type::TypeKind::Pointer && to->kind == Type::TypeKind::Pointer) {
        const auto* ptrFrom = dynamic_cast<const Type::PointerType*>(from);
        const auto* ptrTo = dynamic_cast<const Type::PointerType*>(to);

        // ptr<opaque> can convert to any pointer type (for null)
        if (ptrFrom->pointeeType->kind == Type::TypeKind::Opaque) {
            return true;
        }

        // Any pointer type can convert to ptr<opaque> (for C interop)
        if (ptrTo->pointeeType->kind == Type::TypeKind::Opaque) {
            return true;
        }
    }

    // Special case: str -> ptr<i8>
    if (from->kind == Type::TypeKind::Primitive && to->kind == Type::TypeKind::Pointer) {
        const auto* primFrom = dynamic_cast<const Type::PrimitiveType*>(from);
        const auto* ptrTo = dynamic_cast<const Type::PointerType*>(to);

        if (primFrom->kind == Type::PrimitiveKind::String &&
            ptrTo->pointeeType->kind == Type::TypeKind::Primitive) {
            const auto* pointeePrim = dynamic_cast<const Type::PrimitiveType*>(ptrTo->pointeeType);
            if (pointeePrim->kind == Type::PrimitiveKind::I8) {
                return true;  // str -> ptr<i8> is allowed
            }
        }
    }

    // Special case: struct <-> ptr<struct> (bi-directional)
    if (from->kind == Type::TypeKind::Pointer && to->kind == Type::TypeKind::Struct) {
        const auto* ptrFrom = dynamic_cast<const Type::PointerType*>(from);
        if (ptrFrom->pointeeType == to) {
            return true;  // ptr<Point> -> Point is allowed
        }
    }

    if (from->kind == Type::TypeKind::Struct && to->kind == Type::TypeKind::Pointer) {
        const auto* ptrTo = dynamic_cast<const Type::PointerType*>(to);
        if (ptrTo->pointeeType == from) {
            return true;  // Point -> ptr<Point> is allowed
        }
    }

    // Numeric type conversions
    if (from->kind == Type::TypeKind::Primitive && to->kind == Type::TypeKind::Primitive) {
        const auto* primFrom = dynamic_cast<const Type::PrimitiveType*>(from);
        const auto* primTo = dynamic_cast<const Type::PrimitiveType*>(to);

        // Integer to integer: only allow widening with same signedness
        if (primFrom->isInteger() && primTo->isInteger()) {
            return haveSameSignedness(from, to) && isWideningConversion(from, to);
        }

        // Float to float: only allow f32 -> f64 (widening)
        if (isFloatType(from) && isFloatType(to)) {
            return isWideningConversion(from, to);
        }

        // NO implicit conversions between int and float
        // NO implicit conversions between signed and unsigned
    }

    return false;
}

bool SemanticAnalyzer::isExplicitlyConvertible(const Type::Type* from, const Type::Type* to) {
    // Exact type match
    if (areTypesEqual(from, to)) { return true;
}

    // All implicit conversions are also explicit
    if (isImplicitlyConvertible(from, to)) { return true;
}

    // Additional conversions allowed with explicit cast:
    if (from->kind == Type::TypeKind::Primitive && to->kind == Type::TypeKind::Primitive) {
        const auto* primFrom = dynamic_cast<const Type::PrimitiveType*>(from);
        const auto* primTo = dynamic_cast<const Type::PrimitiveType*>(to);

        // Any numeric to any numeric conversion is allowed with explicit cast
        if (isNumericType(from) && isNumericType(to)) {
            return true;
        }

        // Bool to integer and vice versa
        if (primFrom->kind == Type::PrimitiveKind::Bool && primTo->isInteger()) {
            return true;
        }
        if (primFrom->isInteger() && primTo->kind == Type::PrimitiveKind::Bool) {
            return true;
        }
    }

    // Pointer casts could be added here in the future

    return false;
}

void SemanticAnalyzer::registerStructTypes(const HIR::HIRProgram& program) {
    // First pass: register all struct types in the type registry
    for (const auto& stmt : program.statements) {
        if (auto* structDecl = dynamic_cast<HIR::HIRStructDecl*>(stmt.get())) {
            // Check if already registered as a COMPLETE struct
            // A struct is complete if it has been registered with its full definition
            // (not just as a stub from pre-registration phase)
            auto* existingStruct = typeRegistry.getStruct(structDecl->name.lexeme);
            if (existingStruct) {
                // Check if it's a stub: stubs have no fields AND no methods
                // If it has either fields or methods, it's already been fully registered
                bool isStub = existingStruct->fields.empty() && existingStruct->methods.empty();
                if (!isStub) {
                    // Already fully registered, skip silently
                    continue;
                }
                // If it's a stub, continue to register it fully below
            }

            // Collect fields (types might be unresolved at this point)
            std::vector<Type::FieldInfo> fields;
            for (const auto& field : structDecl->fields) {
                fields.emplace_back(field.name.lexeme, field.type, field.isPublic);
            }

            // Register the struct type (will update stub if it exists)
            typeRegistry.registerStruct(structDecl->name.lexeme, fields);

            // Register methods after type is created
            auto* structType = typeRegistry.getStruct(structDecl->name.lexeme);
            for (auto& method : structDecl->methods) {
                // Resolve self parameter type
                bool hasSelf = false;
                bool hasMutSelf = false;
                std::vector<const Type::Type*> paramTypes;

                for (auto& param : method->params) {
                    if (param.name == "self") {
                        // Replace null type with pointer to this struct
                        param.type = typeRegistry.getPointer(structType);
                        hasSelf = true;
                        hasMutSelf = param.isMutRef;
                        paramTypes.push_back(param.type);
                    } else {
                        paramTypes.push_back(param.type);
                    }
                }

                // Convert struct return types to pointers (all structs are heap-allocated)
                const Type::Type* returnType = method->returnType;
                if (returnType->kind == Type::TypeKind::Struct) {
                    returnType = typeRegistry.getPointer(returnType);
                }

                // Add method to struct type
                Type::MethodSignature methodSig(
                    method->name,
                    paramTypes,
                    returnType,
                    hasSelf,
                    hasMutSelf,
                    method->isPublic
                );
                structType->addMethod(methodSig);
            }
        }
    }
}

void SemanticAnalyzer::resolveUnresolvedTypes(HIR::HIRProgram& program) {
    // Second pass: resolve all unresolved types to their actual types
    for (auto& stmt : program.statements) {
        if (auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(stmt.get())) {
            // Resolve return type
            fnDecl->returnType = resolveType(fnDecl->returnType);

            // Resolve parameter types
            for (auto& param : fnDecl->params) {
                param.type = resolveType(param.type);
            }

            // IMPORTANT: Also resolve types in function body
            resolveTypesInStmts(fnDecl->body);
        } else if (auto* varDecl = dynamic_cast<HIR::HIRVarDecl*>(stmt.get())) {
            // Resolve variable type
            varDecl->typeAnnotation = resolveType(varDecl->typeAnnotation);
        } else if (auto* structDecl = dynamic_cast<HIR::HIRStructDecl*>(stmt.get())) {
            // Resolve field types in struct declaration
            for (auto& field : structDecl->fields) {
                field.type = resolveType(field.type);
            }

            // Also update the registered struct type with resolved field types
            std::vector<Type::FieldInfo> resolvedFields;
            for (const auto& field : structDecl->fields) {
                resolvedFields.emplace_back(field.name.lexeme, field.type, field.isPublic);
            }

            // Update the struct in the registry
            auto* structType = typeRegistry.getStruct(structDecl->name.lexeme);
            if (structType) {
                structType->fields = resolvedFields;
            }

            // Resolve method return types and parameter types
            for (auto& method : structDecl->methods) {
                // Resolve return type
                method->returnType = resolveType(method->returnType);

                // Resolve parameter types
                for (auto& param : method->params) {
                    param.type = resolveType(param.type);
                }

                // Also resolve types in method body
                resolveTypesInStmts(method->body);
            }

            // Re-register method signatures with resolved types
            if (structType) {
                structType->methods.clear();  // Clear old signatures
                for (auto& method : structDecl->methods) {
                    // Build resolved parameter types
                    bool hasSelf = false;
                    bool hasMutSelf = false;
                    std::vector<const Type::Type*> paramTypes;
                    for (auto& param : method->params) {
                        if (param.name == "self") {
                            hasSelf = true;
                            hasMutSelf = param.isMutRef;
                        }
                        paramTypes.push_back(param.type);
                    }

                    // Convert struct return types to pointers
                    const Type::Type* returnType = method->returnType;
                    if (returnType->kind == Type::TypeKind::Struct) {
                        returnType = typeRegistry.getPointer(returnType);
                    }

                    // Add resolved method signature
                    Type::MethodSignature methodSig(
                        method->name,
                        paramTypes,
                        returnType,
                        hasSelf,
                        hasMutSelf,
                        method->isPublic
                    );
                    structType->addMethod(methodSig);
                }
            }
        }
    }
}

void SemanticAnalyzer::resolveTypesInStmts(std::vector<std::unique_ptr<HIR::HIRStmt>>& stmts) {
    for (auto& stmt : stmts) {
        resolveTypesInStmt(stmt.get());
    }
}

void SemanticAnalyzer::resolveTypesInStmt(HIR::HIRStmt* stmt) {
    if (stmt == nullptr) { return;
}

    // Resolve types in variable declarations
    if (auto* varDecl = dynamic_cast<HIR::HIRVarDecl*>(stmt)) {
        varDecl->typeAnnotation = resolveType(varDecl->typeAnnotation);
    }
    // Resolve types in if statements
    else if (auto* ifStmt = dynamic_cast<HIR::HIRIfStmt*>(stmt)) {
        resolveTypesInStmts(ifStmt->thenBody);
        resolveTypesInStmts(ifStmt->elseBody);
    }
    // Resolve types in while statements
    else if (auto* whileStmt = dynamic_cast<HIR::HIRWhileStmt*>(stmt)) {
        resolveTypesInStmts(whileStmt->body);
    }
    // Resolve types in block statements
    else if (auto* blockStmt = dynamic_cast<HIR::HIRBlockStmt*>(stmt)) {
        resolveTypesInStmts(blockStmt->statements);
    }
}

const Type::Type* SemanticAnalyzer::resolveType(const Type::Type* type) {
    if (type == nullptr) { return nullptr;
}

    // If it's an unresolved type, look it up in the type registry
    if (type->kind == Type::TypeKind::Unresolved) {
        const auto* unresolvedType = dynamic_cast<const Type::UnresolvedType*>(type);

        // Try to find the struct type
        auto* structType = typeRegistry.getStruct(unresolvedType->name);
        if (structType != nullptr) {
            return structType;
        }

        // If not found, report an error
        diag.error("Unknown type: " + unresolvedType->name, 0, 0);
        return errorType();
    }

    // For array types, resolve the element type
    if (type->kind == Type::TypeKind::Array) {
        const auto* arrayType = dynamic_cast<const Type::ArrayType*>(type);
        const Type::Type* resolvedElement = resolveType(arrayType->elementType);
        if (resolvedElement != arrayType->elementType) {
            return typeRegistry.getArray(resolvedElement, arrayType->size);
        }
    }

    // For pointer types, resolve the pointee type
    if (type->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = dynamic_cast<const Type::PointerType*>(type);
        const Type::Type* resolvedPointee = resolveType(ptrType->pointeeType);
        if (resolvedPointee != ptrType->pointeeType) {
            return typeRegistry.getPointer(resolvedPointee);
        }
    }

    // For generic types, resolve all type parameters
    if (type->kind == Type::TypeKind::Generic) {
        const auto* genType = dynamic_cast<const Type::GenericType*>(type);
        std::vector<const Type::Type*> resolvedParams;
        bool changed = false;
        for (const auto* param : genType->typeParams) {
            const Type::Type* resolvedParam = resolveType(param);
            resolvedParams.push_back(resolvedParam);
            if (resolvedParam != param) {
                changed = true;
            }
        }
        if (changed) {
            return typeRegistry.getGeneric(genType->name, resolvedParams);
        }
    }

    // Already resolved or primitive type
    return type;
}

} // namespace Semantic
