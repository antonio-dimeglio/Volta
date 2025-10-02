#include "semantic/SemanticAnalyzer.hpp"
#include <cassert>

namespace volta::semantic {


SemanticAnalyzer::SemanticAnalyzer(volta::errors::ErrorReporter& reporter)
    : errorReporter_(reporter),
      symbolTable_(std::make_unique<SymbolTable>()) {}

void SemanticAnalyzer::registerBuiltins() {
    // print(str) -> void
    {
        std::vector<std::shared_ptr<Type>> params = {
            std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String)
        };
        auto returnType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Void);
        auto printType = std::make_shared<FunctionType>(std::move(params), returnType);

        auto symbol = Symbol("print", printType, false, volta::errors::SourceLocation());
        symbolTable_->declare("print", symbol);
    }

    // TODO: Add more builtin functions here:
    // - println(str) -> void
    // - len(Array[T]) -> int
    // - assert(bool) -> void
    // etc.
}

bool SemanticAnalyzer::analyze(const volta::ast::Program& program) {
    registerBuiltins();
    collectDeclarations(program);
    resolveTypes(program);
    analyzeProgram(program);

    return !hasErrors_;
}

std::shared_ptr<Type> SemanticAnalyzer::getExpressionType(const volta::ast::Expression* expr) const {
    auto it = expressionTypes_.find(expr);

    if (it != expressionTypes_.end()) {
        return it->second;
    }

    return nullptr; 
}

void SemanticAnalyzer::enterScope() {
    symbolTable_->enterScope();
}

void SemanticAnalyzer::exitScope() {
    symbolTable_->exitScope();
}

void SemanticAnalyzer::declareVariable(const std::string& name, std::shared_ptr<Type> type, bool isMutable, volta::errors::SourceLocation loc) {
    auto symbol = Symbol(name, type, isMutable, loc);
    if (!symbolTable_->declare(name, symbol)) {
        error("Redefinition of variable " + name + ".", loc);
    }
}

void SemanticAnalyzer::declareFunction(const std::string& name, std::shared_ptr<Type> functionType, volta::errors::SourceLocation loc) {
    auto symbol = Symbol(name, functionType, false, loc);
    if (!symbolTable_->declare(name, symbol)) {
        error("Redefinition of function " + name + ".", loc);
    }
}

std::shared_ptr<Type> SemanticAnalyzer::lookupVariable(const std::string& name, volta::errors::SourceLocation loc) {
    auto* symbol = symbolTable_->lookup(name);
    if (symbol == nullptr) {
        error("Undefined variable '" + name + "'", loc);
        return std::make_shared<UnknownType>();
    }

    return symbol->type;
}

std::shared_ptr<Type> SemanticAnalyzer::lookupFunction(const std::string& name, volta::errors::SourceLocation loc) {
    auto* symbol = symbolTable_->lookup(name);
    if (symbol == nullptr) {
        error("Undefined function '" + name + "'", loc);
        return std::make_shared<UnknownType>();
    }

    return symbol->type;
}

bool SemanticAnalyzer::isVariableMutable(const std::string& name) {
    return symbolTable_->isMutable(name);
}

void SemanticAnalyzer::enterLoop() {
    loopDepth_++;
}

void SemanticAnalyzer::exitLoop() {
    loopDepth_--;
}

void SemanticAnalyzer::enterFunction(std::shared_ptr<Type> returnType) {
    currentReturnType_ = std::move(returnType);
}

void SemanticAnalyzer::exitFunction() {
    currentReturnType_ = nullptr;
}


void SemanticAnalyzer::error(const std::string& message, volta::errors::SourceLocation loc) {
    // TODO: Implement semantic error instead of using compiler error
    hasErrors_ = true; 
    errorReporter_.reportError(
        std::make_unique<volta::errors::CompilerError>(
            volta::errors::CompilerError::Level::Error,
            message,
            loc)
    );
}

void SemanticAnalyzer::typeError(const std::string& message, const Type* expected, const Type* actual, volta::errors::SourceLocation loc) {
    error(
        message + " Expected type: " + expected->toString() + " actual: " + actual->toString(),
        loc
    );
}

std::shared_ptr<Type> SemanticAnalyzer::resolveTypeAnnotation(const volta::ast::Type* astType) const {
    if (!astType) {
        return std::make_shared<UnknownType>();
    }

    // Handle PrimitiveType (int, float, bool, str)
    if (auto* prim = dynamic_cast<const volta::ast::PrimitiveType*>(astType)) {
        switch (prim->kind) {
            case volta::ast::PrimitiveType::Kind::Int:
                return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
            case volta::ast::PrimitiveType::Kind::Float:
                return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Float);
            case volta::ast::PrimitiveType::Kind::Bool:
                return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
            case volta::ast::PrimitiveType::Kind::Str:
                return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String);
        }
    }

    if (auto* compound = dynamic_cast<const volta::ast::CompoundType*>(astType)) {
        switch (compound->kind) {
            case volta::ast::CompoundType::Kind::Array: {
                if (compound->typeArgs.size() != 1) {
                    return std::make_shared<UnknownType>();
                }
                auto elemType = resolveTypeAnnotation(compound->typeArgs[0].get());
                return std::make_shared<ArrayType>(elemType);
            }
            
            case volta::ast::CompoundType::Kind::Matrix: {
                // Matrix must have exactly 1 type argument
                if (compound->typeArgs.size() != 1) {
                    return std::make_shared<UnknownType>();
                }
                auto elemType = resolveTypeAnnotation(compound->typeArgs[0].get());
                return std::make_shared<MatrixType>(elemType);
            }
            
            case volta::ast::CompoundType::Kind::Option: {
                if (compound->typeArgs.size() != 1) {
                    return std::make_shared<UnknownType>();
                }
                auto innerType = resolveTypeAnnotation(compound->typeArgs[0].get());
                return std::make_shared<OptionType>(innerType);
            }
            
            case volta::ast::CompoundType::Kind::Tuple: {
                std::vector<std::shared_ptr<Type>> elementTypes;
                for (const auto& typeArg : compound->typeArgs) {
                    elementTypes.push_back(resolveTypeAnnotation(typeArg.get()));
                }
                return std::make_shared<TupleType>(std::move(elementTypes));
            }
        }
    }

    // Handle FunctionType fn(T1, T2) -> R
    if (auto* fnType = dynamic_cast<const volta::ast::FunctionType*>(astType)) {
        std::vector<std::shared_ptr<Type>> paramTypes;
        for (const auto& paramType : fnType->paramTypes) {
            paramTypes.push_back(resolveTypeAnnotation(paramType.get()));
        }
        auto returnType = resolveTypeAnnotation(fnType->returnType.get());
        return std::make_shared<FunctionType>(std::move(paramTypes), returnType);
    }

    // Handle NamedType (user-defined structs or type aliases)
    if (auto* named = dynamic_cast<const volta::ast::NamedType*>(astType)) {
        // Look up the type name in the symbol table
        const std::string& typeName = named->identifier->name;
        auto* symbol = symbolTable_->lookup(typeName);

        if (!symbol) {
            // Type not found
            return std::make_shared<UnknownType>();
        }

        // Return the type associated with this symbol
        // For struct declarations, this will be a StructType
        return symbol->type;
    }

    // Unknown type
    return std::make_shared<UnknownType>();
}

void SemanticAnalyzer::collectDeclarations(const volta::ast::Program& program) {
    for (auto& stmt : program.statements) {
        if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt.get())) {
            collectFunction(*fnDecl);
        }
        if (auto* structDecl = dynamic_cast<const volta::ast::StructDeclaration*>(stmt.get())) {
            collectStruct(*structDecl);
        }
    }
}

void SemanticAnalyzer::collectFunction(const volta::ast::FnDeclaration& fn) {
    std::vector<std::shared_ptr<Type>> paramTypes;

    for (auto& param : fn.parameters) {
        auto resolvedType = resolveTypeAnnotation(param->type.get());
        paramTypes.push_back(resolvedType);
    }

    auto returnTypeResolved = resolveTypeAnnotation(fn.returnType.get());

    auto fnType = std::make_shared<FunctionType>(std::move(paramTypes), returnTypeResolved);

    // For methods, use qualified name: "ReceiverType.methodName"
    std::string functionName = fn.identifier;
    if (fn.isMethod) {
        functionName = fn.receiverType + "." + fn.identifier;
    }

    declareFunction(functionName, fnType, fn.location);

}

void SemanticAnalyzer::collectStruct(const volta::ast::StructDeclaration& structDecl) {
    std::vector<StructType::Field> fields;

    for (auto& field : structDecl.fields) {
        auto resolvedFieldtype = resolveTypeAnnotation(field->type.get());
        auto structField = StructType::Field(field->identifier, resolvedFieldtype);
        fields.push_back(structField);
    }
    
    auto structType = std::make_shared<StructType>(structDecl.identifier, std::move(fields));
    declareFunction(structDecl.identifier, structType, structDecl.location);
}

void SemanticAnalyzer::resolveTypes(const volta::ast::Program& program) {
    // PURPOSE: Second pass - currently just a placeholder
    // In a more advanced compiler, this would resolve forward type references
    // For now, all types are resolved immediately in Pass 1
    
    // TODO: Leave empty for now
    // This pass is for languages with forward type declarations
    // Volta resolves types immediately, so nothing to do here
}

void SemanticAnalyzer::analyzeProgram(const volta::ast::Program& program) {
    // PURPOSE: Third pass - type check all statements and expressions
    // This is the main semantic analysis pass
    
    for (auto& stmt: program.statements) {
        analyzeStatement(stmt.get());
    }
}

void SemanticAnalyzer::analyzeStatement(const volta::ast::Statement* stmt) {
    // PURPOSE: Dispatch to the appropriate analyze method based on statement type
    // BEHAVIOR: Use dynamic_cast to determine statement type and call specific handler
    if (auto* varDecl = dynamic_cast<const volta::ast::VarDeclaration*>(stmt)) {
        analyzeVarDeclaration(varDecl);
    } else if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt)) {
        analyzeFnDeclaration(fnDecl);
    } else if (auto* ifStmt = dynamic_cast<const volta::ast::IfStatement*>(stmt)) {
        analyzeIfStatement(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const volta::ast::WhileStatement*>(stmt)) {
        analyzeWhileStatement(whileStmt);
    } else if (auto* forStmt = dynamic_cast<const volta::ast::ForStatement*>(stmt)) {
        analyzeForStatement(forStmt);
    } else if (auto* retStmt = dynamic_cast<const volta::ast::ReturnStatement*>(stmt)) {
        analyzeReturnStatement(retStmt);
    } else if (auto* exprStmt = dynamic_cast<const volta::ast::ExpressionStatement*>(stmt)) {
        analyzeExpression(exprStmt->expr.get());
    } else if (auto* block = dynamic_cast<const volta::ast::Block*>(stmt)) {
        for (auto& currStmt : block->statements) {
            analyzeStatement(currStmt.get());
        }
    } else if (auto* importStmt = dynamic_cast<const volta::ast::ImportStatement*>(stmt)) {
        // Nothing to do - imports handled elsewhere
    } else if (auto* structDecl = dynamic_cast<const volta::ast::StructDeclaration*>(stmt)) {
        // Nothing to do - structs already collected in Pass 1
    } else {
        throw std::runtime_error("Unexpected node for type checking.");
    }
}

void SemanticAnalyzer::analyzeVarDeclaration(const volta::ast::VarDeclaration* varDecl) {
    // PURPOSE: Type check variable declaration and add to symbol table
    // BEHAVIOR: Handle both explicit types (x: int = 5) and inference (x := 5)
    
    // TODO Step 1: Analyze the initializer expression
    //   - Call analyzeExpression(varDecl->initializer.get())
    //   - Store the result in initType
    //   - This gives us the type of the right-hand side
    auto initType = analyzeExpression(varDecl->initializer.get());
    std::shared_ptr<volta::semantic::Type> finalType;
    if (varDecl->typeAnnotation) {
        auto declaredType = resolveTypeAnnotation(varDecl->typeAnnotation->type.get());

        if (!areTypesCompatible(declaredType.get(), initType.get())) {
            typeError("Type mismatch in variable declaration", declaredType.get(), initType.get(), varDecl->location);
        }
        finalType = declaredType;
    } else {
        finalType = initType;
    }

    bool isMutable = varDecl->typeAnnotation ? varDecl->typeAnnotation->isMutable : false;
    declareVariable(varDecl->identifier, finalType, isMutable, varDecl->location);

}

void SemanticAnalyzer::analyzeFnDeclaration(const volta::ast::FnDeclaration* fnDecl) {
    // PURPOSE: Type check function body
    // BEHAVIOR: Function signature already collected in Pass 1, now check body
    enterScope();
   
    for (auto& param : fnDecl->parameters) {
        auto resType = resolveTypeAnnotation(param->type.get());
        declareVariable(param->identifier, resType, false, fnDecl->location);
    }
    
    // TODO Step 3: Set return type context
    //   - Resolve return type: resolveTypeAnnotation(fnDecl->returnType.get())
    //   - Call enterFunction(returnType)
    //   - This allows return statements to check against expected type
    auto retType = resolveTypeAnnotation(fnDecl->returnType.get());
    enterFunction(retType);
    // TODO Step 4: Analyze function body
    //   - If fnDecl->body exists (block body):
    //     * Loop through body->statements
    //     * Call analyzeStatement for each
    //   - If fnDecl->expressionBody exists (single expression):
    //     * Call analyzeExpression(fnDecl->expressionBody.get())
    //     * Check that expression type matches return type
    if (fnDecl->body) {
        for (auto& bodyStmt : fnDecl->body->statements) {
            analyzeStatement(bodyStmt.get());    
        }
    }
    if (fnDecl->expressionBody) {
        auto retVal = analyzeExpression(fnDecl->expressionBody.get());
        if (!areTypesCompatible(retType.get(), retVal.get())) {
            typeError("Mismatched return type and actual return type", retType.get(), retVal.get(), fnDecl->location);
        }
    }
    
    exitFunction();
    exitScope();
}

void SemanticAnalyzer::analyzeIfStatement(const volta::ast::IfStatement* ifStmt) {
    auto condType = analyzeExpression(ifStmt->condition.get());
    auto boolType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);

    if (!areTypesCompatible(boolType.get(), condType.get())) {
        typeError("If condition must be bool", boolType.get(), condType.get(), ifStmt->location);
    }

    enterScope();
    for (auto& stmt : ifStmt->thenBlock->statements) {
        analyzeStatement(stmt.get());
    }
    exitScope();

    for (auto& elseIfClause : ifStmt->elseIfClauses) {
        auto elseIfCondType = analyzeExpression(elseIfClause.first.get());
        if (!areTypesCompatible(boolType.get(), elseIfCondType.get())) {
            typeError("Else-if condition must be bool", boolType.get(), elseIfCondType.get(), ifStmt->location);
        }

        enterScope();
        for (auto& stmt : elseIfClause.second->statements) {
            analyzeStatement(stmt.get());
        }
        exitScope();
    }

    if (ifStmt->elseBlock) {
        enterScope();
        for (auto& stmt : ifStmt->elseBlock->statements) {
            analyzeStatement(stmt.get());
        }
        exitScope();
    }
}

void SemanticAnalyzer::analyzeWhileStatement(const volta::ast::WhileStatement* whileStmt) {
    auto condType = analyzeExpression(whileStmt->condition.get());
    auto boolType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);

    if (!areTypesCompatible(boolType.get(), condType.get())) {
        typeError("While condition must be bool", boolType.get(), condType.get(), whileStmt->location);
    }

    enterLoop();
    enterScope();

    for (auto& stmt : whileStmt->thenBlock->statements) {
        analyzeStatement(stmt.get());
    }

    exitScope();
    exitLoop();
}

void SemanticAnalyzer::analyzeForStatement(const volta::ast::ForStatement* forStmt) {
    auto exprType = analyzeExpression(forStmt->expression.get());

    std::shared_ptr<Type> elementType;

    // Check if it's an array type
    if (auto* arrayType = dynamic_cast<const ArrayType*>(exprType.get())) {
        elementType = arrayType->elementType();
    }
    // Check if it's a range (will have int elements)
    else if (exprType->kind() == Type::Kind::Int) {
        // Range expressions (0..10) produce integers
        elementType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    }
    else {
        error("Expression is not iterable", forStmt->location);
        elementType = std::make_shared<UnknownType>();
    }

    enterLoop();
    enterScope();

    declareVariable(forStmt->identifier, elementType, false, forStmt->location);

    for (auto& stmt : forStmt->thenBlock->statements) {
        analyzeStatement(stmt.get());
    }

    exitScope();
    exitLoop();
}

void SemanticAnalyzer::analyzeReturnStatement(const volta::ast::ReturnStatement* returnStmt) {
    if (currentFunctionReturnType() == nullptr) {
        error("Return statement outside of function definition", returnStmt->location);
        return;
    }

    std::shared_ptr<Type> returnType;

    if (returnStmt->expression) {
        returnType = analyzeExpression(returnStmt->expression.get());
    } else {
        // Empty return statement
        returnType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Void);
    }

    auto expectedType = currentFunctionReturnType();

    if (!areTypesCompatible(expectedType.get(), returnType.get())) {
        typeError("Return type mismatch", expectedType.get(), returnType.get(), returnStmt->location);
    }
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeExpression(const volta::ast::Expression* expr) {
    std::shared_ptr<Type> resultType;

    if (dynamic_cast<const volta::ast::IntegerLiteral*>(expr) ||
        dynamic_cast<const volta::ast::FloatLiteral*>(expr) ||
        dynamic_cast<const volta::ast::StringLiteral*>(expr) ||
        dynamic_cast<const volta::ast::BooleanLiteral*>(expr)) {
        resultType = analyzeLiteral(expr);
    }
    else if (auto* binExpr = dynamic_cast<const volta::ast::BinaryExpression*>(expr)) {
        resultType = analyzeBinaryExpression(binExpr);
    }
    else if (auto* unaryExpr = dynamic_cast<const volta::ast::UnaryExpression*>(expr)) {
        resultType = analyzeUnaryExpression(unaryExpr);
    }
    else if (auto* identifier = dynamic_cast<const volta::ast::IdentifierExpression*>(expr)) {
        resultType = analyzeIdentifier(identifier);
    }
    else if (auto* callExpr = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        resultType = analyzeCallExpression(callExpr);
    }
    else if (auto* matchExpr = dynamic_cast<const volta::ast::MatchExpression*>(expr)) {
        resultType = analyzeMatchExpression(matchExpr);
    }
    else if (auto* memberExpr = dynamic_cast<const volta::ast::MemberExpression*>(expr)) {
        // Analyze member access: object.field
        auto objectType = analyzeExpression(memberExpr->object.get());

        // Object must be a struct type
        auto* structType = dynamic_cast<const StructType*>(objectType.get());
        if (!structType) {
            error("Member access on non-struct type", memberExpr->location);
            resultType = std::make_shared<UnknownType>();
        } else {
            const std::string& fieldName = memberExpr->member->name;
            const auto* field = structType->getField(fieldName);

            if (!field) {
                error("Struct '" + structType->name() + "' has no field '" + fieldName + "'",
                      memberExpr->location);
                resultType = std::make_shared<UnknownType>();
            } else {
                resultType = field->type;
            }
        }
    }
    else if (auto* methodCall = dynamic_cast<const volta::ast::MethodCallExpression*>(expr)) {
        // Analyze method call: object.method(args)
        auto objectType = analyzeExpression(methodCall->object.get());

        // Object must be a struct type
        auto* structType = dynamic_cast<const StructType*>(objectType.get());
        if (!structType) {
            error("Method call on non-struct type", methodCall->location);
            resultType = std::make_shared<UnknownType>();
        } else {
            // Look up method by qualified name: "StructName.methodName"
            const std::string& methodName = methodCall->method->name;
            const std::string qualifiedName = structType->name() + "." + methodName;

            auto* methodSymbol = symbolTable_->lookup(qualifiedName);
            if (!methodSymbol) {
                error("Struct '" + structType->name() + "' has no method '" + methodName + "'",
                      methodCall->location);
                resultType = std::make_shared<UnknownType>();
            } else {
                // Check if it's actually a function type
                auto* fnType = dynamic_cast<const FunctionType*>(methodSymbol->type.get());
                if (!fnType) {
                    error("'" + qualifiedName + "' is not a method", methodCall->location);
                    resultType = std::make_shared<UnknownType>();
                } else {
                    // Type check arguments
                    // Note: First parameter is self, so we check args against parameters[1...]
                    size_t expectedArgCount = fnType->paramTypes().size() - 1;
                    if (methodCall->arguments.size() != expectedArgCount) {
                        error("Wrong number of arguments to method '" + methodName + "'",
                              methodCall->location);
                    } else {
                        for (size_t i = 0; i < methodCall->arguments.size(); ++i) {
                            auto argType = analyzeExpression(methodCall->arguments[i].get());
                            auto expectedType = fnType->paramTypes()[i + 1]; // Skip self parameter
                            if (!areTypesCompatible(expectedType.get(), argType.get())) {
                                typeError("Argument type mismatch in method call",
                                         expectedType.get(), argType.get(), methodCall->location);
                            }
                        }
                    }
                    resultType = fnType->returnType();
                }
            }
        }
    }
    else if (auto* arrayLit = dynamic_cast<const volta::ast::ArrayLiteral*>(expr)) {
        if (arrayLit->elements.empty()) {
            resultType = std::make_shared<ArrayType>(std::make_shared<UnknownType>());
        } else {
            auto elemType = analyzeExpression(arrayLit->elements[0].get());
            resultType = std::make_shared<ArrayType>(elemType);
        }
    }
    else if (auto* structLit = dynamic_cast<const volta::ast::StructLiteral*>(expr)) {
        // Look up the struct type by name
        const std::string& structName = structLit->structName->name;
        auto* symbol = symbolTable_->lookup(structName);

        if (!symbol) {
            error("Unknown struct type '" + structName + "'", structLit->location);
            resultType = std::make_shared<UnknownType>();
        } else if (symbol->type->kind() != Type::Kind::Struct) {
            error("'" + structName + "' is not a struct type", structLit->location);
            resultType = std::make_shared<UnknownType>();
        } else {
            resultType = symbol->type;

            // Type check each field initializer
            auto structType = std::dynamic_pointer_cast<StructType>(symbol->type);
            for (const auto& fieldInit : structLit->fields) {
                const std::string& fieldName = fieldInit->identifier->name;
                const auto* field = structType->getField(fieldName);

                if (!field) {
                    error("Struct '" + structName + "' has no field '" + fieldName + "'",
                          structLit->location);
                    continue;
                }

                // Analyze field value and check type compatibility
                auto valueType = analyzeExpression(fieldInit->value.get());
                if (!areTypesCompatible(field->type.get(), valueType.get())) {
                    typeError("Field '" + fieldName + "' type mismatch",
                             field->type.get(), valueType.get(), structLit->location);
                }
            }
        }
    }
    else {
        resultType = std::make_shared<UnknownType>();
    }

    expressionTypes_[expr] = resultType;
    return resultType;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeLiteral(const volta::ast::Expression* literal) {
    if (dynamic_cast<const volta::ast::IntegerLiteral*>(literal)) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    }
    else if (dynamic_cast<const volta::ast::FloatLiteral*>(literal)) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Float);
    }
    else if (dynamic_cast<const volta::ast::BooleanLiteral*>(literal)) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
    }
    else if (dynamic_cast<const volta::ast::StringLiteral*>(literal)) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String);
    }
    return std::make_shared<UnknownType>();
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeIdentifier(const volta::ast::IdentifierExpression* identifier) {
    return lookupVariable(identifier->name, identifier->location);
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeBinaryExpression(const volta::ast::BinaryExpression* binExpr) {
    auto leftType = analyzeExpression(binExpr->left.get());
    auto rightType = analyzeExpression(binExpr->right.get());

    // Handle assignment operators
    using Op = volta::ast::BinaryExpression::Operator;
    if (binExpr->op == Op::Assign || binExpr->op == Op::AddAssign ||
        binExpr->op == Op::SubtractAssign || binExpr->op == Op::MultiplyAssign ||
        binExpr->op == Op::DivideAssign) {

        auto* ident = dynamic_cast<const volta::ast::IdentifierExpression*>(binExpr->left.get());
        if (!ident) {
            error("Left side of assignment must be a variable", binExpr->location);
            return leftType;
        }

        if (!isVariableMutable(ident->name)) {
            error("Cannot assign to immutable variable '" + ident->name + "'", binExpr->location);
        }

        if (!areTypesCompatible(leftType.get(), rightType.get())) {
            typeError("Type mismatch in assignment", leftType.get(), rightType.get(), binExpr->location);
        }

        return leftType;
    }

    return inferBinaryOpType(binExpr->op, leftType.get(), rightType.get());
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeUnaryExpression(const volta::ast::UnaryExpression* unaryExpr) {
    auto operandType = analyzeExpression(unaryExpr->operand.get());
    return inferUnaryOpType(unaryExpr->op, operandType.get());
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeCallExpression(const volta::ast::CallExpression* callExpr) {
    auto calleeType = analyzeExpression(callExpr->callee.get());

    auto* fnType = dynamic_cast<const FunctionType*>(calleeType.get());
    if (!fnType) {
        error("Expression is not callable", callExpr->location);
        return std::make_shared<UnknownType>();
    }

    if (callExpr->arguments.size() != fnType->paramTypes().size()) {
        error("Wrong number of arguments", callExpr->location);
        return fnType->returnType();
    }

    for (size_t i = 0; i < callExpr->arguments.size(); ++i) {
        auto argType = analyzeExpression(callExpr->arguments[i].get());
        auto expectedType = fnType->paramTypes()[i];
        if (!areTypesCompatible(expectedType.get(), argType.get())) {
            typeError("Argument type mismatch", expectedType.get(), argType.get(), callExpr->location);
        }
    }

    return fnType->returnType();
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeMatchExpression(const volta::ast::MatchExpression* matchExpr) {
    // TODO: Instead of just analyzing the value this should check if the match is exhaustive
    analyzeExpression(matchExpr->value.get());

    std::shared_ptr<Type> resultType;
    for (const auto& arm : matchExpr->arms) {
        if (arm->guard) {
            analyzeExpression(arm->guard.get());
        }
        auto armType = analyzeExpression(arm->body.get());
        if (!resultType) {
            resultType = armType;
        }
    }

    return resultType ? resultType : std::make_shared<UnknownType>();
}

bool SemanticAnalyzer::areTypesCompatible(const Type* expected, const Type* actual) {
    // Handle UnknownType - compatible with anything (error recovery)
    if (actual->kind() == Type::Kind::Unknown) {
        return true;
    }

    // For array types, check element compatibility
    if (expected->kind() == Type::Kind::Array && actual->kind() == Type::Kind::Array) {
        auto* expectedArray = dynamic_cast<const ArrayType*>(expected);
        auto* actualArray = dynamic_cast<const ArrayType*>(actual);

        // Empty arrays (Array[Unknown]) are compatible with any array type
        if (actualArray->elementType()->kind() == Type::Kind::Unknown) {
            return true;
        }

        return areTypesCompatible(expectedArray->elementType().get(),
                                  actualArray->elementType().get());
    }

    return expected->equals(actual);
}

std::shared_ptr<Type> SemanticAnalyzer::inferBinaryOpType(
    volta::ast::BinaryExpression::Operator op,
    const Type* left,
    const Type* right
) {
    using Op = volta::ast::BinaryExpression::Operator;

    // Arithmetic operators
    if (op == Op::Add || op == Op::Subtract || op == Op::Multiply ||
        op == Op::Divide || op == Op::Modulo || op == Op::Power) {

        if (left->kind() == Type::Kind::Int && right->kind() == Type::Kind::Int) {
            return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
        }
        if ((left->kind() == Type::Kind::Float || left->kind() == Type::Kind::Int) &&
            (right->kind() == Type::Kind::Float || right->kind() == Type::Kind::Int)) {
            return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Float);
        }
        return std::make_shared<UnknownType>();
    }

    // Comparison operators
    if (op == Op::Equal || op == Op::NotEqual || op == Op::Less ||
        op == Op::Greater || op == Op::LessEqual || op == Op::GreaterEqual) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
    }

    // Logical operators
    if (op == Op::LogicalAnd || op == Op::LogicalOr) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
    }

    // Range operators
    if (op == Op::Range || op == Op::RangeInclusive) {
        return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    }

    return std::make_shared<UnknownType>();
}

std::shared_ptr<Type> SemanticAnalyzer::inferUnaryOpType(
    volta::ast::UnaryExpression::Operator op,
    const Type* operand
) {
    using Op = volta::ast::UnaryExpression::Operator;

    if (op == Op::Negate) {
        if (operand->kind() == Type::Kind::Int || operand->kind() == Type::Kind::Float) {
            return std::shared_ptr<Type>(const_cast<Type*>(operand), [](Type*){});
        }
    }

    if (op == Op::Not) {
        if (operand->kind() == Type::Kind::Bool) {
            return std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
        }
    }

    return std::make_shared<UnknownType>();
}

}