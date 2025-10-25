#include "../../include/Semantic/SemanticAnalyzer.hpp"
#include "../../include/Parser/AST.hpp"
#include "../../include/Semantic/TypeSubstitution.hpp"
#include "../../include/HIR/Lowering.hpp"
#include <iostream>
#include <climits>
#include <set>

namespace Semantic {

SemanticAnalyzer::SemanticAnalyzer(Type::TypeRegistry& types, DiagnosticManager& diag, Volta::GenericRegistry* genRegistry)
    : typeRegistry(types),  diag(diag), externalGenericRegistry(genRegistry) {
}

void SemanticAnalyzer::registerGenericTemplates(const Program& ast) {
    // Scan AST for generic functions and structs and register them
    for (const auto& stmt : ast.statements) {
        if (auto* fnDecl = dynamic_cast<FnDecl*>(stmt.get())) {
            if (!fnDecl->typeParamaters.empty()) {
                // This is a generic function - register it
                registerGenericFunction(fnDecl);
            }
        } else if (auto* structDecl = dynamic_cast<StructDecl*>(stmt.get())) {
            if (!structDecl->typeParamaters.empty()) {
                // This is a generic struct - register it
                registerGenericStruct(structDecl);
            }
        }
    }
}

void SemanticAnalyzer::analyzeProgram(const HIR::HIRProgram& program) {
    // Phase 1: Register all struct types first (so they're available for function signatures)
    registerStructTypes(program);

    // Phase 2: Resolve all unresolved types in the program
    resolveUnresolvedTypes(const_cast<HIR::HIRProgram&>(program));

    // Phase 3: Collect function signatures
    collectFunctionSignatures(program);

    // Phase 4: Analyze all statements (skip generic templates)
    for (const auto& stmt : program.statements) {
        // Skip generic function/struct templates - they will be analyzed when monomorphized
        if (auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(stmt.get())) {
            bool isGeneric = false;
            if (fnDecl->returnType->kind == Type::TypeKind::Unresolved ||
                fnDecl->returnType->kind == Type::TypeKind::Generic) {
                isGeneric = true;
            }
            for (const auto& param : fnDecl->params) {
                if (param.type->kind == Type::TypeKind::Unresolved ||
                    param.type->kind == Type::TypeKind::Generic) {
                    isGeneric = true;
                    break;
                }
            }
            if (isGeneric) {
                continue;  // Skip generic templates
            }
        }
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
        if (dynamic_cast<HIR::HIRReturnStmt*>(stmt.get()) != nullptr) {
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
            // Set expected type for top-down type inference
            expectedType = varType;
            const Type::Type* initType = node.initValue->accept(*this);
            expectedType = nullptr; // Reset after use

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

    // Set the type annotation on the node so later passes can use it
    node.typeAnnotation = varType;

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
        // Parameter is mutable if it's either mut ref or mut (by-value)
        bool isMut = param.isMutRef || param.isMutable;
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
        // Check mutability of the base array variable
        if (auto* baseVar = dynamic_cast<Variable*>(indexNode->array.get())) {
            const Symbol* sym = symbolTable.lookup(baseVar->token.lexeme);
            if (sym && !sym->is_mut) {
                diag.error("Cannot modify immutable array: " + baseVar->token.lexeme,
                          node.line, node.column);
            }
        }

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
        // Check mutability of the base struct variable
        if (auto* baseVar = dynamic_cast<Variable*>(fieldAccess->object.get())) {
            const Symbol* sym = symbolTable.lookup(baseVar->token.lexeme);
            if (sym && !sym->is_mut) {
                diag.error("Cannot modify immutable struct: " + baseVar->token.lexeme,
                          node.line, node.column);
            }
        }

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
    // Check if this is the sizeof<T>() compiler builtin
    if (node.name == "sizeof" && node.typeArgs.size() == 1 && node.args.empty()) {
        // This is sizeof<T>() - replace with a constant
        const Type::Type* sizeType = resolveType(node.typeArgs[0]);
        size_t size = getTypeSize(sizeType);

        // Store the result type
        const Type::Type* u64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::U64);
        exprTypes[&node] = u64Type;

        // Store the size value for MIR lowering to use
        sizeofValues[&node] = size;

        return u64Type;
    }

    // Check if this is a generic function call
    if (!node.typeArgs.empty()) {
        // Resolve type arguments (they may be unresolved from parsing)
        std::vector<const Type::Type*> resolvedTypeArgs;
        for (const auto* typeArg : node.typeArgs) {
            resolvedTypeArgs.push_back(resolveType(typeArg));
        }
        node.typeArgs = resolvedTypeArgs;

        // Check if it's a registered generic function
        if (getGenericRegistry().isGenericFunction(node.name)) {
            // Store original name before monomorphization
            std::string originalName = node.name;

            // Monomorphize the function
            auto monomorphed = monomorphizeFunction(originalName, node.typeArgs, node.line, node.column);
            if (!monomorphed) {
                // Already monomorphized - look up the existing name
                std::string monomorphName = getGenericRegistry().findMonomorphName(originalName, node.typeArgs, true);
                if (monomorphName.empty()) {
                    // This shouldn't happen - monomorphizeFunction should have created it
                    diag.error("Failed to find or create monomorphized function", node.line, node.column);
                    const Type::Type* type = errorType();
                    exprTypes[&node] = type;
                    return type;
                }
                // Update to use existing monomorph
                node.name = monomorphName;
            } else {
                // New monomorph created - update name
                node.name = monomorphed->name;
            }

            // Analyze the monomorphized function now if not already done
            const FunctionSignature* monomorphSig = symbolTable.lookupFunction(node.name);
            if (monomorphSig == nullptr) {
                // Register the function signature so it can be called
                // Note: Full body analysis will happen when the monomorphized function is lowered to HIR
                std::vector<FunctionParameter> params;
                for (const auto& p : monomorphed->params) {
                    params.emplace_back(p.name, p.type, p.isRef, p.isMutRef);
                }

                FunctionSignature fnSig(params, monomorphed->returnType, monomorphed->isExtern);
                symbolTable.addFunction(node.name, fnSig);
            }
        } else {
            diag.error("Function '" + node.name + "' is not a generic function but was called with type arguments",
                      node.line, node.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }
    }

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
    // Handle repeat syntax: [value; count]
    if (node.repeat_value && node.repeat_count) {
        const Type::Type* elementType = node.repeat_value->accept(*this);
        const Type::Type* arrayType = typeRegistry.getArray(elementType, {*node.repeat_count});
        exprTypes[&node] = arrayType;
        return arrayType;
    }

    // Handle regular array literal: [e1, e2, e3, ...]
    if (node.elements.empty()) {
        diag.error("Cannot infer type of empty array", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // Special case: Fill initialization [value] with expected type
    // If we have exactly 1 element and an expected array type, treat as fill
    if (node.elements.size() == 1 && expectedType != nullptr && expectedType->kind == Type::TypeKind::Array) {
        const auto* expectedArrayType = dynamic_cast<const Type::ArrayType*>(expectedType);

        // Calculate total size from expected dimensions
        int totalSize = 1;
        for (int dim : expectedArrayType->dimensions) {
            totalSize *= dim;
        }

        // Type check the single value
        const Type::Type* valueType = node.elements[0]->accept(*this);

        // Verify value type matches expected element type
        if (!isImplicitlyConvertible(valueType, expectedArrayType->elementType)) {
            diag.error("Fill value type " + valueType->toString() +
                      " does not match expected element type " + expectedArrayType->elementType->toString(),
                      node.line, node.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }

        // Convert to repeat syntax: [value; totalSize]
        node.repeat_value = std::move(node.elements[0]);
        node.repeat_count = totalSize;
        node.elements.clear();

        // Return the expected array type
        exprTypes[&node] = expectedType;
        return expectedType;
    }

    // Type check first element to determine element type
    const Type::Type* elementType = node.elements[0]->accept(*this);

    // Check all elements have the same type
    for (size_t i = 1; i < node.elements.size(); i++) {
        const Type::Type* elemType = node.elements[i]->accept(*this);
        if (elemType != elementType) {
            diag.error("Array elements must have uniform type", node.line, node.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }
    }

    // Build dimensions vector
    std::vector<int> dimensions;
    dimensions.push_back(static_cast<int>(node.elements.size())); // Outermost dimension

    // If elements are arrays themselves, extract their dimensions (nested array)
    if (elementType->kind == Type::TypeKind::Array) {
        const auto* innerArrayType = dynamic_cast<const Type::ArrayType*>(elementType);
        // Append inner dimensions: [outer, inner_dim0, inner_dim1, ...]
        dimensions.insert(dimensions.end(),
                         innerArrayType->dimensions.begin(),
                         innerArrayType->dimensions.end());

        // The actual element type is the innermost element type
        elementType = innerArrayType->elementType;
    }

    const Type::Type* arrayType = typeRegistry.getArray(elementType, dimensions);
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

    const auto* arrType = dynamic_cast<const Type::ArrayType*>(arrayType);

    // Type check all indices - must be integers
    for (size_t i = 0; i < node.index.size(); i++) {
        const Type::Type* indexType = node.index[i]->accept(*this);
        if (!isIntegerType(indexType)) {
            diag.error("Array index must be integer type", node.line, node.column);
        }
    }

    // Check if we have too many indices
    if (node.index.size() > arrType->dimensions.size()) {
        diag.error("Too many indices for array (got " + std::to_string(node.index.size()) +
                  ", expected at most " + std::to_string(arrType->dimensions.size()) + ")",
                  node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // Compile-time bounds checking for literal indices
    for (size_t i = 0; i < node.index.size(); i++) {
        if (auto* lit = dynamic_cast<Literal*>(node.index[i].get())) {
            if (lit->token.tt == TokenType::Integer) {
                try {
                    int64_t literalIndex = std::stoll(lit->token.lexeme);
                    if (literalIndex < 0 || literalIndex >= static_cast<int64_t>(arrType->dimensions[i])) {
                        diag.error("Array index " + std::to_string(literalIndex) +
                                  " at dimension " + std::to_string(i) +
                                  " is out of bounds for array dimension of size " + std::to_string(arrType->dimensions[i]),
                                  node.line, node.column);
                    }
                } catch (const std::out_of_range&) {
                    diag.error("Array index literal is out of range", node.line, node.column);
                }
            }
        }
    }

    // Determine result type based on number of indices provided
    const Type::Type* resultType = nullptr;

    if (node.index.size() == arrType->dimensions.size()) {
        // Full indexing - returns element type
        resultType = arrType->elementType;
    } else {
        // Partial indexing - returns sub-array with remaining dimensions
        std::vector<int> remainingDims(arrType->dimensions.begin() + node.index.size(),
                                       arrType->dimensions.end());
        resultType = typeRegistry.getArray(arrType->elementType, remainingDims);
    }

    exprTypes[&node] = resultType;
    return resultType;
}

const Type::Type* SemanticAnalyzer::visitAddrOf(AddrOf& node) {
    // Check if operand is addressable (must be a variable, field access, or index expression)
    // Cannot take address of literals or temporary values
    bool isAddressable = dynamic_cast<Variable*>(node.operand.get()) != nullptr ||
                         dynamic_cast<FieldAccess*>(node.operand.get()) != nullptr ||
                         dynamic_cast<IndexExpr*>(node.operand.get()) != nullptr;

    if (!isAddressable) {
        diag.error("Cannot take address of temporary value or literal", node.line, node.column);
        const Type::Type* errorType = typeRegistry.getPrimitive(Type::PrimitiveKind::Void);
        exprTypes[&node] = typeRegistry.getPointer(errorType);
        return typeRegistry.getPointer(errorType);
    }

    const Type::Type* operandType = node.operand->accept(*this);
    const Type::Type* ptrType = typeRegistry.getPointer(operandType);
    exprTypes[&node] = ptrType;
    return ptrType;
}

const Type::Type* SemanticAnalyzer::visitSizeOf(SizeOf& node) {
    // sizeof<T>() is a compiler builtin that is replaced with a constant
    // The type has already been parsed and stored in node.type
    // We just return u64 as the type - the actual replacement with a constant
    // happens during MIR lowering or we can do it here

    const Type::Type* u64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::U64);
    exprTypes[&node] = u64Type;
    return u64Type;
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
    // Check if this is a generic struct instantiation
    std::string structName = node.structName.lexeme;
    if (!node.typeArgs.empty()) {
        // Resolve type arguments (they may be unresolved from parsing)
        std::vector<const Type::Type*> resolvedTypeArgs;
        for (const auto* typeArg : node.typeArgs) {
            resolvedTypeArgs.push_back(resolveType(typeArg));
        }
        node.typeArgs = resolvedTypeArgs;

        // Check if it's a registered generic struct
        if (getGenericRegistry().isGenericStruct(structName)) {
            // Monomorphize the struct
            auto* monomorphedType = monomorphizeStruct(structName, node.typeArgs,
                                                       node.structName.line, node.structName.column);
            if (monomorphedType == nullptr) {
                // Error was already reported
                const Type::Type* type = errorType();
                exprTypes[&node] = type;
                return type;
            }

            // Update the struct name to use the monomorphized version
            structName = monomorphedType->name;
        } else {
            diag.error("Struct '" + structName + "' is not a generic struct but was instantiated with type arguments",
                      node.structName.line, node.structName.column);
            const Type::Type* type = errorType();
            exprTypes[&node] = type;
            return type;
        }
    }

    // 1. Look up the struct type by name (either original or monomorphized)
    auto* structType = typeRegistry.getStruct(structName);
    if (structType == nullptr) {
        diag.error("Unknown struct type: " + structName,
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
            diag.error("Unknown field '" + providedField + "' in struct '" + structName + "'",
                       node.structName.line, node.structName.column);
        }
    }

    // 3. Type-check each field value
    for (auto& [fieldName, fieldValue] : node.fields) {
        const Type::Type* expectedType = structType->getFieldType(fieldName.lexeme);
        if (expectedType != nullptr) {
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
            structType = dynamic_cast<const Type::StructType*>(ptrType->pointeeType);
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
            structType = dynamic_cast<const Type::StructType*>(ptrType->pointeeType);
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
            // Skip generic function templates (they have unresolved types)
            bool isGeneric = false;
            if (fnDecl->returnType->kind == Type::TypeKind::Unresolved) {
                isGeneric = true;
            }
            for (const auto& param : fnDecl->params) {
                if (param.type->kind == Type::TypeKind::Unresolved) {
                    isGeneric = true;
                    break;
                }
            }
            if (!isGeneric) {
                registerFunction(*fnDecl);
            }
        } else if (auto* externBlock = dynamic_cast<HIR::HIRExternBlock*>(stmt.get())) {
            for (const auto& fn : externBlock->declarations) {
                registerFunction(*fn);
            }
        }
    }
}

void SemanticAnalyzer::registerFunction(HIR::HIRFnDecl& node) {
    std::vector<FunctionParameter> params;
    params.reserve(node.params.size());
for (const auto& p : node.params) {
        params.emplace_back(p.name, p.type, p.isRef, p.isMutRef);
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

bool SemanticAnalyzer::containsGenericType(const Type::Type* type) {
    if (!type) { return false; }

    // In HIR, type parameters might be Unresolved (like "T") or Generic (like "Box<T>")
    if (type->kind == Type::TypeKind::Unresolved) {
        const auto* unresolvedType = dynamic_cast<const Type::UnresolvedType*>(type);
        // Simple heuristic: single capital letter is likely a type parameter
        if (unresolvedType->name.length() == 1 && std::isupper(unresolvedType->name[0])) {
            return true;
        }
    }

    // Check if this type itself is a generic type parameter
    if (type->kind == Type::TypeKind::Generic) {
        const auto* genType = dynamic_cast<const Type::GenericType*>(type);
        // If it has no type parameters, it's a bare type parameter like T
        if (genType->typeParams.empty()) {
            return true;
        }
        // Otherwise check the type parameters recursively (e.g., Box<T>)
        for (const auto* param : genType->typeParams) {
            if (containsGenericType(param)) {
                return true;
            }
        }
    }

    return false;
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

size_t SemanticAnalyzer::getTypeSize(const Type::Type* type) {
    if (type == nullptr) { return 0; }

    switch (type->kind) {
        case Type::TypeKind::Primitive: {
            const auto* primType = dynamic_cast<const Type::PrimitiveType*>(type);
            switch (primType->kind) {
                case Type::PrimitiveKind::I8:
                case Type::PrimitiveKind::U8:
                case Type::PrimitiveKind::Bool:
                    return 1;
                case Type::PrimitiveKind::I16:
                case Type::PrimitiveKind::U16:
                    return 2;
                case Type::PrimitiveKind::I32:
                case Type::PrimitiveKind::U32:
                case Type::PrimitiveKind::F32:
                    return 4;
                case Type::PrimitiveKind::I64:
                case Type::PrimitiveKind::U64:
                case Type::PrimitiveKind::F64:
                    return 8;
                case Type::PrimitiveKind::String:
                    return 8;  // Pointer to string data
                case Type::PrimitiveKind::Void:
                    return 0;
            }
            break;
        }
        case Type::TypeKind::Pointer:
            return 8;  // 64-bit pointers

        case Type::TypeKind::Struct: {
            const auto* structType = dynamic_cast<const Type::StructType*>(type);
            size_t total = 0;
            for (const auto& field : structType->fields) {
                // TODO: Add proper alignment/padding calculation
                total += getTypeSize(field.type);
            }
            return total;
        }

        case Type::TypeKind::Array: {
            const auto* arrayType = dynamic_cast<const Type::ArrayType*>(type);
            // Calculate total number of elements: product of all dimensions
            size_t totalElements = 1;
            for (int dim : arrayType->dimensions) {
                totalElements *= dim;
            }
            return getTypeSize(arrayType->elementType) * totalElements;
        }

        case Type::TypeKind::Generic:
        case Type::TypeKind::Opaque:
        case Type::TypeKind::Unresolved:
            return 0;  // Unknown size
    }

    return 0;
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
            // Skip generic struct templates (they have unresolved or generic type parameters)
            bool isGeneric = false;
            for (const auto& field : structDecl->fields) {
                if (field.type->kind == Type::TypeKind::Unresolved ||
                    field.type->kind == Type::TypeKind::Generic) {
                    isGeneric = true;
                    break;
                }
            }
            if (isGeneric) {
                // Generic structs are handled by monomorphization, skip HIR registration
                continue;
            }

            // Check if already registered as a COMPLETE struct
            // A struct is complete if it has been registered with its full definition
            // (not just as a stub from pre-registration phase)
            auto* existingStruct = typeRegistry.getStruct(structDecl->name.lexeme);
            if (existingStruct != nullptr) {
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
            fields.reserve(structDecl->fields.size());
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
            // Skip generic function templates (they have unresolved or generic types that are type parameters)
            bool isGeneric = false;
            if (fnDecl->returnType->kind == Type::TypeKind::Unresolved ||
                fnDecl->returnType->kind == Type::TypeKind::Generic) {
                isGeneric = true;
            }
            for (const auto& param : fnDecl->params) {
                if (param.type->kind == Type::TypeKind::Unresolved ||
                    param.type->kind == Type::TypeKind::Generic) {
                    isGeneric = true;
                    break;
                }
            }
            if (isGeneric) {
                continue;  // Skip generic templates
            }

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
            // Skip generic struct templates (they have type parameters that appear as Generic types in fields)
            // Do NOT skip structs with Unresolved field types - those just need resolution
            bool isGeneric = false;
            for (const auto& field : structDecl->fields) {
                if (containsGenericType(field.type)) {
                    // This field contains a type parameter like T, U - skip this struct
                    isGeneric = true;
                    break;
                }
            }
            if (isGeneric) {
                continue;  // Skip generic templates
            }

            // Resolve field types in struct declaration
            for (auto& field : structDecl->fields) {
                field.type = resolveType(field.type);
            }

            // Also update the registered struct type with resolved field types
            // IMPORTANT: Struct-typed fields are stored as pointers (all structs are heap-allocated)
            std::vector<Type::FieldInfo> resolvedFields;
            resolvedFields.reserve(structDecl->fields.size());
            for (const auto& field : structDecl->fields) {
                const Type::Type* fieldType = field.type;
                // Convert struct types to pointers (all structs are heap-allocated in Volta)
                if (fieldType->kind == Type::TypeKind::Struct) {
                    fieldType = typeRegistry.getPointer(fieldType);
                }
                resolvedFields.emplace_back(field.name.lexeme, fieldType, field.isPublic);
            }

            // Update the struct in the registry
            auto* structType = typeRegistry.getStruct(structDecl->name.lexeme);
            if (structType != nullptr) {
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
            if (structType != nullptr) {
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

    // If it's a generic type, monomorphize it
    if (type->kind == Type::TypeKind::Generic) {
        const auto* genericType = dynamic_cast<const Type::GenericType*>(type);

        // Resolve type parameters first (they might be unresolved)
        std::vector<const Type::Type*> resolvedTypeParams;
        for (const auto* typeParam : genericType->typeParams) {
            resolvedTypeParams.push_back(resolveType(typeParam));
        }

        // Check if this is a generic struct
        if (getGenericRegistry().isGenericStruct(genericType->name)) {
            // Monomorphize the struct
            auto* monomorphedType = monomorphizeStruct(genericType->name, resolvedTypeParams, 0, 0);
            if (monomorphedType) {
                return monomorphedType;
            }
            return errorType();
        }

        // Generic function types aren't supported in type positions
        diag.error("Generic functions cannot be used as types: " + genericType->name, 0, 0);
        return errorType();
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
            return typeRegistry.getArray(resolvedElement, arrayType->dimensions);
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

// ============================================================================
// Generic Support
// ============================================================================

void SemanticAnalyzer::validateTypeParameters(const std::vector<Token>& typeParams, int line, int column) {
    std::set<std::string> seenParams;

    for (const auto& param : typeParams) {
        // Check for duplicates
        if (seenParams.count(param.lexeme) > 0) {
            diag.error("Duplicate type parameter '" + param.lexeme + "'", param.line, param.column);
        }
        seenParams.insert(param.lexeme);

        // TODO: Check for shadowing of local variables (requires scope context)
        // For now, we'll just validate uniqueness
    }
}

void SemanticAnalyzer::registerGenericFunction(FnDecl* fn) {
    if (!fn->isGeneric()) {
        return;  // Not a generic function
    }

    validateTypeParameters(fn->typeParamaters, fn->line, fn->column);
    getGenericRegistry().registerGenericFunction(fn->name, fn->typeParamaters, fn);
}

void SemanticAnalyzer::registerGenericStruct(StructDecl* structDecl) {
    if (!structDecl->isGeneric()) {
        return;  // Not a generic struct
    }

    validateTypeParameters(structDecl->typeParamaters, structDecl->name.line, structDecl->name.column);
    getGenericRegistry().registerGenericStruct(structDecl->name.lexeme, structDecl->typeParamaters, structDecl);
}

FnDecl* SemanticAnalyzer::monomorphizeFunction(
    const std::string& genericName,
    const std::vector<const Type::Type*>& typeArgs,
    int callSiteLine,
    int callSiteColumn
) {
    // Get generic definition
    auto* genericDef = getGenericRegistry().getGenericFunction(genericName);
    if (!genericDef) {
        diag.error("Unknown generic function '" + genericName + "'", callSiteLine, callSiteColumn);
        return nullptr;
    }

    // Validate type argument count
    if (typeArgs.size() != genericDef->typeParams.size()) {
        diag.error(
            "Generic function '" + genericName + "' expects " +
            std::to_string(genericDef->typeParams.size()) + " type argument(s), got " +
            std::to_string(typeArgs.size()),
            callSiteLine,
            callSiteColumn
        );
        return nullptr;
    }

    // Check if already monomorphized
    if (getGenericRegistry().hasMonomorphInstance(genericName, typeArgs, true)) {
        // Already exists, don't create again
        return nullptr;
    }

    // Generate monomorph name
    std::string monomorphName = getGenericRegistry().getOrCreateMonomorphName(genericName, typeArgs, true);

    // DEBUG: Print monomorphization info
    std::cout << "    [Monomorphizing function '" << genericName << "' with types: ";
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        std::cout << typeArgs[i]->toString();
        if (i < typeArgs.size() - 1) std::cout << ", ";
    }
    std::cout << " -> " << monomorphName << "]\n";

    // Prevent infinite recursion
    if (currentlyAnalyzing.count(monomorphName) > 0) {
        diag.error("Recursive generic instantiation detected for '" + monomorphName + "'", callSiteLine, callSiteColumn);
        return nullptr;
    }
    currentlyAnalyzing.insert(monomorphName);

    // Perform type substitution
    Volta::TypeSubstitution substitution(genericDef->typeParams, typeArgs);
    auto monomorphed = substitution.substituteFnDecl(genericDef->astNode);

    // Update name to monomorphized version
    monomorphed->name = monomorphName;

    // Resolve GenericTypes in the function signature and body to their monomorphized struct types
    // This ensures that return types like Pair<i64, i32> become Pair$i64$i32
    monomorphed->returnType = resolveType(monomorphed->returnType);
    for (auto& param : monomorphed->params) {
        param.type = resolveType(param.type);
    }

    // Resolve types in the function body (recursively handle all statements)
    // This is needed because the cloned AST may have GenericTypes that need to be resolved
    for (auto& stmt : monomorphed->body) {
        // Resolve types in variable declarations
        if (auto* varDecl = dynamic_cast<VarDecl*>(stmt.get())) {
            if (varDecl->typeAnnotation) {
                varDecl->typeAnnotation = resolveType(varDecl->typeAnnotation);
            }
        }
        // Could add more statement types here if needed (IfStmt, WhileStmt, etc.)
    }

    // Store the monomorphized function for later MIR lowering
    FnDecl* result = monomorphed.get();
    monomorphizedFunctions.push_back(std::move(monomorphed));
    std::cout << "    [Stored monomorphized function '" << monomorphName << "', total count: " << monomorphizedFunctions.size() << "]\n";

    // Remove recursion guard
    currentlyAnalyzing.erase(monomorphName);

    // Return raw pointer (ownership is in monomorphizedFunctions)
    return result;
}

const Type::StructType* SemanticAnalyzer::monomorphizeStruct(
    const std::string& genericName,
    const std::vector<const Type::Type*>& typeArgs,
    int useSiteLine,
    int useSiteColumn
) {
    // Get generic definition
    auto* genericDef = getGenericRegistry().getGenericStruct(genericName);
    if (!genericDef) {
        diag.error("Unknown generic struct '" + genericName + "'", useSiteLine, useSiteColumn);
        return nullptr;
    }

    // Validate type argument count
    if (typeArgs.size() != genericDef->typeParams.size()) {
        diag.error(
            "Generic struct '" + genericName + "' expects " +
            std::to_string(genericDef->typeParams.size()) + " type argument(s), got " +
            std::to_string(typeArgs.size()),
            useSiteLine,
            useSiteColumn
        );
        return nullptr;
    }

    // Generate monomorph name
    std::string monomorphName = getGenericRegistry().getOrCreateMonomorphName(genericName, typeArgs, false);

    // Check if already monomorphized
    const auto* existingType = typeRegistry.getStruct(monomorphName);
    if (existingType) {
        return existingType;  // Already exists
    }

    // Prevent infinite recursion
    if (currentlyAnalyzing.count(monomorphName) > 0) {
        diag.error("Recursive generic instantiation detected for '" + monomorphName + "'", useSiteLine, useSiteColumn);
        return nullptr;
    }
    currentlyAnalyzing.insert(monomorphName);

    // Perform type substitution
    Volta::TypeSubstitution substitution(genericDef->typeParams, typeArgs);
    auto monomorphed = substitution.substituteStructDecl(genericDef->astNode);

    // Update name to monomorphized version
    monomorphed->name = Token{TokenType::Identifier, static_cast<size_t>(useSiteLine), static_cast<size_t>(useSiteColumn), monomorphName};

    // Convert StructField to FieldInfo for registration
    // IMPORTANT: Struct-typed fields are stored as pointers (all structs are heap-allocated)
    std::vector<Type::FieldInfo> fields;
    fields.reserve(monomorphed->fields.size());
    for (const auto& field : monomorphed->fields) {
        // Resolve GenericTypes in field types to their monomorphized struct types
        const Type::Type* resolvedFieldType = resolveType(field.type);
        // Convert struct types to pointers (all structs are heap-allocated in Volta)
        if (resolvedFieldType->kind == Type::TypeKind::Struct) {
            resolvedFieldType = typeRegistry.getPointer(resolvedFieldType);
        }
        fields.emplace_back(field.name.lexeme, resolvedFieldType, field.isPublic);
    }

    // Register the struct type (without analyzing methods yet - they'll be analyzed on-demand)
    typeRegistry.registerStruct(monomorphName, fields);

    // Store the monomorphized AST for later
    monomorphizedStructs.push_back(std::move(monomorphed));

    // Remove recursion guard
    currentlyAnalyzing.erase(monomorphName);

    return typeRegistry.getStruct(monomorphName);
}

} // namespace Semantic
