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
                int64_t value = std::stoll(node.token.lexeme);

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
                    uint64_t value = std::stoull(node.token.lexeme);
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
    if (!symbol) {
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

    bool oldInLoop = inLoop;
    inLoop = true;

    for (auto& stmt : node.body) {
        stmt->accept(*this);
    }

    // Check increment expression if it exists (for desugared for loops)
    if (node.increment) {
        node.increment->accept(*this);
    }

    inLoop = oldInLoop;
}

void SemanticAnalyzer::visitBlockStmt(HIR::HIRBlockStmt& node) {
    symbolTable.enterScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    symbolTable.exitScope();
}

void SemanticAnalyzer::visitReturnStmt(HIR::HIRReturnStmt& node) {
    if (currentReturnType == nullptr) {
        diag.error("'return' outside of function definition", node.line, node.column);
        return;
    }

    if (node.value) {
        const Type::Type* retValueType = node.value->accept(*this);
        if (!isTypeCompatible(retValueType, currentReturnType)) {
            diag.error("Return type mismatch", node.line, node.column);
        }
    } else{
        const Type::Type* voidType = typeRegistry.getPrimitive(Type::PrimitiveKind::Void);
        if (currentReturnType != voidType) {
            diag.error("Missing return value in non-void function", node.line, node.column);
        }
    }
}

void SemanticAnalyzer::visitVarDecl(HIR::HIRVarDecl& node) {
    const Type::Type* varType;

    if (node.typeAnnotation) {
        varType = node.typeAnnotation;

        if (node.initValue) {
            const Type::Type* initType = node.initValue->accept(*this);
            if (!isTypeCompatible(initType, varType)) {
                diag.error("Type mismatch: variable type doesn't match initializer",
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

    for (auto& stmt : node.thenBody) {
        stmt->accept(*this);
    }
    for (auto& stmt : node.elseBody) {
        stmt->accept(*this);
    }
}

const Type::Type* SemanticAnalyzer::visitBinaryExpr(BinaryExpr& node) {
    const Type::Type* lhsType = node.lhs->accept(*this);
    const Type::Type* rhsType = node.rhs->accept(*this);

    const Type::Type* resultType = nullptr;

    // Handle numeric type coercion for literals
    // If types are compatible but not equal, coerce literals to the non-literal type
    if (lhsType != rhsType && isNumericType(lhsType) && isNumericType(rhsType)) {
        // Check if one side is a literal that can be coerced
        bool lhsIsLiteral = dynamic_cast<Literal*>(node.lhs.get()) != nullptr;
        bool rhsIsLiteral = dynamic_cast<Literal*>(node.rhs.get()) != nullptr;

        if (lhsIsLiteral && isTypeCompatible(lhsType, rhsType)) {
            // Coerce LHS literal to RHS type
            exprTypes[node.lhs.get()] = rhsType;
            lhsType = rhsType;
        } else if (rhsIsLiteral && isTypeCompatible(rhsType, lhsType)) {
            // Coerce RHS literal to LHS type
            exprTypes[node.rhs.get()] = lhsType;
            rhsType = lhsType;
        }
    }

    if (lhsType != rhsType) {
        diag.error("Type mismatch in binary expression", node.line, node.column);
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
        if (!sym) {
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

        if (!isTypeCompatible(valueType, lhsType)) {
            diag.error("Type mismatch in field assignment", node.line, node.column);
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
    if (!sig && functionRegistry) {
        const Semantic::GlobalFunctionSignature* globalSig = functionRegistry->getFunction(node.name);
        if (globalSig) {
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

                if (!isTypeCompatible(argType, paramType)) {
                    diag.error("Argument " + std::to_string(i+1) + " type mismatch in call to '" +
                              node.name + "'", node.line, node.column);
                }
            }

            exprTypes[&node] = globalSig->return_type;
            return globalSig->return_type;
        }
    }

    if (!sig) {
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

        if (!isTypeCompatible(argType, paramType)) {
            diag.error("Argument " + std::to_string(i+1) + " type mismatch in call to '" +
                      node.name + "'", node.line, node.column);
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

    auto* arrType = static_cast<const Type::ArrayType*>(arrayType);
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
    if (!structType) {
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
    for (const auto& [fieldName, fieldType] : structType->fields) {
        requiredFields.insert(fieldName);
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
            if (!isTypeCompatible(actualType, expectedType)) {
                diag.error("Type mismatch for field '" + fieldName.lexeme + "'",
                           fieldName.line, fieldName.column);
            }
            // CRITICAL FIX: Override the inferred type with the expected field type
            // This ensures that numeric literals get the correct type (e.g., f32 instead of default f64)
            exprTypes[fieldValue.get()] = expectedType;
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
        structType = static_cast<const Type::StructType*>(objectType);
    } else if (objectType->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = static_cast<const Type::PointerType*>(objectType);
        if (ptrType->pointeeType->kind == Type::TypeKind::Struct) {
            structType = static_cast<const Type::StructType*>(ptrType->pointeeType);
        }
    }

    if (!structType) {
        diag.error("Field access on non-struct type", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 3. Look up the field in the struct
    const Type::Type* fieldType = structType->getFieldType(node.fieldName.lexeme);

    if (!fieldType) {
        diag.error("Unknown field '" + node.fieldName.lexeme + "' in struct '" + structType->name + "'",
                   node.fieldName.line, node.fieldName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 4. Store resolved info for MIR lowering
    node.resolvedStructType = structType;

    // Find field index for efficient codegen
    for (size_t i = 0; i < structType->fields.size(); ++i) {
        if (structType->fields[i].first == node.fieldName.lexeme) {
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
    if (!structType) {
        diag.error("Unknown type '" + node.typeName.lexeme + "'",
                   node.typeName.line, node.typeName.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 2. Look up the method in the struct type
    const auto* methodSig = structType->getMethod(node.methodName.lexeme);
    if (!methodSig) {
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
        if (!isTypeCompatible(argType, methodSig->paramTypes[i])) {
            diag.error("Argument " + std::to_string(i + 1) + " type mismatch in method call",
                       node.args[i]->line, node.args[i]->column);
        }
    }

    exprTypes[&node] = methodSig->returnType;
    return methodSig->returnType;
}

const Type::Type* SemanticAnalyzer::visitInstanceMethodCall(InstanceMethodCall& node) {
    // 1. Analyze the object expression to get its type
    const Type::Type* objectType = node.object->accept(*this);

    // 2. Handle both struct values and pointers to structs
    const Type::StructType* structType = nullptr;
    if (objectType->kind == Type::TypeKind::Struct) {
        structType = static_cast<const Type::StructType*>(objectType);
    } else if (objectType->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = static_cast<const Type::PointerType*>(objectType);
        if (ptrType->pointeeType->kind == Type::TypeKind::Struct) {
            structType = static_cast<const Type::StructType*>(ptrType->pointeeType);
        }
    }

    if (!structType) {
        diag.error("Method call on non-struct type", node.line, node.column);
        const Type::Type* type = errorType();
        exprTypes[&node] = type;
        return type;
    }

    // 3. Look up the method in the struct type
    const auto* methodSig = structType->getMethod(node.methodName.lexeme);
    if (!methodSig) {
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
        if (!isTypeCompatible(argType, expectedType)) {
            diag.error("Argument " + std::to_string(i + 1) + " type mismatch in method call",
                       node.args[i]->line, node.args[i]->column);
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

    FunctionSignature sig(params, node.returnType);

    if (!symbolTable.addFunction(node.name, sig)) {
        diag.error("Function '" + node.name + "' already defined", node.line, node.column);
    }
}

bool SemanticAnalyzer::isNumericType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) return false;
    auto* prim = static_cast<const Type::PrimitiveType*>(type);
    return prim->kind == Type::PrimitiveKind::I32 ||
           prim->kind == Type::PrimitiveKind::I64 ||
           prim->kind == Type::PrimitiveKind::U32 ||
           prim->kind == Type::PrimitiveKind::U64 ||
           prim->kind == Type::PrimitiveKind::F32 ||
           prim->kind == Type::PrimitiveKind::F64;
}

bool SemanticAnalyzer::isIntegerType(const Type::Type* type) {
    if (type->kind != Type::TypeKind::Primitive) return false;
    auto* prim = static_cast<const Type::PrimitiveType*>(type);
    return prim->kind == Type::PrimitiveKind::I32 ||
           prim->kind == Type::PrimitiveKind::I64 ||
           prim->kind == Type::PrimitiveKind::U32 ||
           prim->kind == Type::PrimitiveKind::U64 ||
           prim->kind == Type::PrimitiveKind::I8 ||
           prim->kind == Type::PrimitiveKind::I16 ||
           prim->kind == Type::PrimitiveKind::U8 ||
           prim->kind == Type::PrimitiveKind::U16;
}

bool SemanticAnalyzer::isTypeCompatible(const Type::Type* from, const Type::Type* to) {
    // Same type
    if (from == to) return true;

    // Special case: integer literals (i32) are compatible with any integer type
    if (from->kind == Type::TypeKind::Primitive && to->kind == Type::TypeKind::Primitive) {
        const auto* primFrom = static_cast<const Type::PrimitiveType*>(from);
        const auto* primTo = static_cast<const Type::PrimitiveType*>(to);

        // Integer literals can be assigned to any integer type
        // (i32, i64, u64 depending on literal value range)
        if ((primFrom->kind == Type::PrimitiveKind::I32 ||
             primFrom->kind == Type::PrimitiveKind::I64 ||
             primFrom->kind == Type::PrimitiveKind::U64) && isIntegerType(to)) {
            return true;
        }

        // f64 literals can be assigned to any float type
        // f32 can also be assigned to f64
        if ((primFrom->kind == Type::PrimitiveKind::F64 || primFrom->kind == Type::PrimitiveKind::F32) &&
            (primTo->kind == Type::PrimitiveKind::F32 || primTo->kind == Type::PrimitiveKind::F64)) {
            return true;
        }
    }

    // Special case: str is compatible with ptr<i8>
    if (from->kind == Type::TypeKind::Primitive && to->kind == Type::TypeKind::Pointer) {
        const auto* primFrom = static_cast<const Type::PrimitiveType*>(from);
        const auto* ptrTo = static_cast<const Type::PointerType*>(to);

        if (primFrom->kind == Type::PrimitiveKind::String &&
            ptrTo->pointeeType->kind == Type::TypeKind::Primitive) {
            const auto* pointeePrim = static_cast<const Type::PrimitiveType*>(ptrTo->pointeeType);
            if (pointeePrim->kind == Type::PrimitiveKind::I8) {
                return true;  // str -> ptr<i8> is allowed
            }
        }
    }

    // Special case: pointer to struct is compatible with struct type
    // (since all structs are heap-allocated, ptr<Point> is compatible with Point)
    if (from->kind == Type::TypeKind::Pointer && to->kind == Type::TypeKind::Struct) {
        const auto* ptrFrom = static_cast<const Type::PointerType*>(from);
        if (ptrFrom->pointeeType == to) {
            return true;  // ptr<Point> -> Point is allowed
        }
    }

    // And vice versa: struct type is compatible with pointer to struct
    if (from->kind == Type::TypeKind::Struct && to->kind == Type::TypeKind::Pointer) {
        const auto* ptrTo = static_cast<const Type::PointerType*>(to);
        if (ptrTo->pointeeType == from) {
            return true;  // Point -> ptr<Point> is allowed
        }
    }

    return false;
}

void SemanticAnalyzer::registerStructTypes(const HIR::HIRProgram& program) {
    // First pass: register all struct types in the type registry
    for (const auto& stmt : program.statements) {
        if (auto* structDecl = dynamic_cast<HIR::HIRStructDecl*>(stmt.get())) {
            // Check if already registered (can happen if analyzeProgram is called multiple times)
            if (typeRegistry.hasStruct(structDecl->name.lexeme)) {
                // Already registered, skip silently
                continue;
            }

            // Collect fields (types might be unresolved at this point)
            std::vector<std::pair<std::string, const Type::Type*>> fields;
            for (const auto& field : structDecl->fields) {
                fields.push_back({field.name.lexeme, field.type});
            }

            // Register the struct type
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
            std::vector<std::pair<std::string, const Type::Type*>> resolvedFields;
            for (const auto& field : structDecl->fields) {
                resolvedFields.push_back({field.name.lexeme, field.type});
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
    if (!stmt) return;

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
    if (!type) return nullptr;

    // If it's an unresolved type, look it up in the type registry
    if (type->kind == Type::TypeKind::Unresolved) {
        const auto* unresolvedType = static_cast<const Type::UnresolvedType*>(type);

        // Try to find the struct type
        auto* structType = typeRegistry.getStruct(unresolvedType->name);
        if (structType) {
            return structType;
        }

        // If not found, report an error
        diag.error("Unknown type: " + unresolvedType->name, 0, 0);
        return errorType();
    }

    // For array types, resolve the element type
    if (type->kind == Type::TypeKind::Array) {
        const auto* arrayType = static_cast<const Type::ArrayType*>(type);
        const Type::Type* resolvedElement = resolveType(arrayType->elementType);
        if (resolvedElement != arrayType->elementType) {
            return typeRegistry.getArray(resolvedElement, arrayType->size);
        }
    }

    // For pointer types, resolve the pointee type
    if (type->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = static_cast<const Type::PointerType*>(type);
        const Type::Type* resolvedPointee = resolveType(ptrType->pointeeType);
        if (resolvedPointee != ptrType->pointeeType) {
            return typeRegistry.getPointer(resolvedPointee);
        }
    }

    // For generic types, resolve all type parameters
    if (type->kind == Type::TypeKind::Generic) {
        const auto* genType = static_cast<const Type::GenericType*>(type);
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
