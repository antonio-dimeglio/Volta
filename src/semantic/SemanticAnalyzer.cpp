#include "semantic/SemanticAnalyzer.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <unordered_set>

namespace volta::semantic {


SemanticAnalyzer::SemanticAnalyzer(volta::errors::ErrorReporter& reporter)
    : errorReporter_(reporter),
      symbolTable_(std::make_unique<SymbolTable>()) {}

void SemanticAnalyzer::registerBuiltins() {
    // print(int) -> void
    {
        std::vector<std::shared_ptr<Type>> params = {
            typeCache_.getInt()
        };
        auto returnType = typeCache_.getVoid();
        auto printType = typeCache_.getFunctionType(std::move(params), returnType);

        auto symbol = Symbol("print", printType, false, volta::errors::SourceLocation());
        symbolTable_->declare("print", symbol);
    }

    // print(float) -> void
    {
        std::vector<std::shared_ptr<Type>> params = {
            typeCache_.getFloat()
        };
        auto returnType = typeCache_.getVoid();
        auto printType = typeCache_.getFunctionType(std::move(params), returnType);

        auto symbol = Symbol("print", printType, false, volta::errors::SourceLocation());
        symbolTable_->declare("print", symbol);
    }

    // print(bool) -> void
    {
        std::vector<std::shared_ptr<Type>> params = {
            typeCache_.getBool()
        };
        auto returnType = typeCache_.getVoid();
        auto printType = typeCache_.getFunctionType(std::move(params), returnType);

        auto symbol = Symbol("print", printType, false, volta::errors::SourceLocation());
        symbolTable_->declare("print", symbol);
    }

    // print(str) -> void
    {
        std::vector<std::shared_ptr<Type>> params = {
            typeCache_.getString()
        };
        auto returnType = typeCache_.getVoid();
        auto printType = typeCache_.getFunctionType(std::move(params), returnType);

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

void SemanticAnalyzer::declareType(const std::string& name, std::shared_ptr<Type> type, volta::errors::SourceLocation loc) {
    auto symbol = Symbol(name, type, false, loc);
    if (!symbolTable_->declare(name, symbol)) {
        error("Redefinition of type '" + name + "'", loc);
    }
}

std::shared_ptr<Type> SemanticAnalyzer::lookupVariable(const std::string& name, volta::errors::SourceLocation loc) {
    auto symbol = symbolTable_->lookup(name);
    if (!symbol) {
        error("Undefined variable '" + name + "'", loc);
        return typeCache_.getUnknown();
    }

    return symbol->type;
}

std::shared_ptr<Type> SemanticAnalyzer::lookupFunction(const std::string& name, volta::errors::SourceLocation loc) {
    auto symbol = symbolTable_->lookup(name);
    if (!symbol) {
        error("Undefined function '" + name + "'", loc);
        return typeCache_.getUnknown();
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
        return typeCache_.getUnknown();
    }

    // Handle PrimitiveType (int, float, bool, str, void)
    if (auto* prim = dynamic_cast<const volta::ast::PrimitiveType*>(astType)) {
        switch (prim->kind) {
            case volta::ast::PrimitiveType::Kind::Int:
                return typeCache_.getInt();
            case volta::ast::PrimitiveType::Kind::Float:
                return typeCache_.getFloat();
            case volta::ast::PrimitiveType::Kind::Bool:
                return typeCache_.getBool();
            case volta::ast::PrimitiveType::Kind::Str:
                return typeCache_.getString();
            case volta::ast::PrimitiveType::Kind::Void:
                return typeCache_.getVoid();
        }
    }

    if (auto* compound = dynamic_cast<const volta::ast::CompoundType*>(astType)) {
        switch (compound->kind) {
            case volta::ast::CompoundType::Kind::Array: {
                if (compound->typeArgs.size() != 1) {
                    return typeCache_.getUnknown();
                }
                auto elemType = resolveTypeAnnotation(compound->typeArgs[0].get());
                return typeCache_.getArrayType(elemType);
            }


            case volta::ast::CompoundType::Kind::Option: {
                // Option is now a user-defined generic enum, not a builtin!
                // Treat it as a generic enum instantiation: Option[T]
                if (compound->typeArgs.size() != 1) {
                    return typeCache_.getUnknown();
                }

                // Look up the Option enum type
                auto symbol = symbolTable_->lookup("Option");
                if (!symbol || symbol->type->kind() != Type::Kind::Enum) {
                    // Fallback to legacy Option type if enum not found
                    auto innerType = resolveTypeAnnotation(compound->typeArgs[0].get());
                    return typeCache_.getOptionType(innerType);
                }

                // It's a user-defined enum! Instantiate it
                auto* enumType = dynamic_cast<const EnumType*>(symbol->type.get());
                auto innerType = resolveTypeAnnotation(compound->typeArgs[0].get());
                auto instantiated = enumType->instantiate({innerType});
                return instantiated ? instantiated : typeCache_.getUnknown();
            }

            case volta::ast::CompoundType::Kind::Tuple: {
                std::vector<std::shared_ptr<Type>> elementTypes;
                for (const auto& typeArg : compound->typeArgs) {
                    elementTypes.push_back(resolveTypeAnnotation(typeArg.get()));
                }
                return typeCache_.getTupleType(std::move(elementTypes));
            }

            case volta::ast::CompoundType::Kind::Matrix:
                // Matrix removed from language
                return typeCache_.getUnknown();
        }
    }

    // Handle FunctionType fn(T1, T2) -> R
    if (auto* fnType = dynamic_cast<const volta::ast::FunctionType*>(astType)) {
        std::vector<std::shared_ptr<Type>> paramTypes;
        for (const auto& paramType : fnType->paramTypes) {
            paramTypes.push_back(resolveTypeAnnotation(paramType.get()));
        }
        auto returnType = resolveTypeAnnotation(fnType->returnType.get());
        return typeCache_.getFunctionType(std::move(paramTypes), returnType);
    }

    // Handle GenericType (e.g., Pair[int], Option[String])
    if (auto* generic = dynamic_cast<const volta::ast::GenericType*>(astType)) {
        const std::string& typeName = generic->identifier->name;
        auto symbol = symbolTable_->lookup(typeName);

        if (!symbol) {
            return typeCache_.getUnknown();
        }

        // Resolve type arguments
        std::vector<std::shared_ptr<Type>> typeArgs;
        for (const auto& arg : generic->typeArgs) {
            typeArgs.push_back(resolveTypeAnnotation(arg.get()));
        }

        // Check if it's a generic enum
        if (symbol->type->kind() == Type::Kind::Enum) {
            auto* enumType = dynamic_cast<const EnumType*>(symbol->type.get());
            auto instantiated = enumType->instantiate(typeArgs);
            return instantiated ? instantiated : typeCache_.getUnknown();
        }

        // Check if it's a generic struct
        if (symbol->type->kind() == Type::Kind::Struct) {
            auto* structType = dynamic_cast<const StructType*>(symbol->type.get());
            auto instantiated = structType->instantiate(typeArgs);
            return instantiated ? instantiated : typeCache_.getUnknown();
        }

        return typeCache_.getUnknown();
    }

    // Handle NamedType (user-defined structs or type aliases)
    if (auto* named = dynamic_cast<const volta::ast::NamedType*>(astType)) {
        // Look up the type name in the symbol table
        const std::string& typeName = named->identifier->name;
        auto symbol = symbolTable_->lookup(typeName);

        if (!symbol) {
            // Type not found
            return typeCache_.getUnknown();
        }

        // Return the type associated with this symbol
        // For struct declarations, this will be a StructType
        return symbol->type;
    }

    // Unknown type
    return typeCache_.getUnknown();
}

void SemanticAnalyzer::collectDeclarations(const volta::ast::Program& program) {
    for (auto& stmt : program.statements) {
        if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt.get())) {
            collectFunction(*fnDecl);
        }
        if (auto* structDecl = dynamic_cast<const volta::ast::StructDeclaration*>(stmt.get())) {
            collectStruct(*structDecl);
        }
        if (auto* enumDecl = dynamic_cast<const volta::ast::EnumDeclaration*>(stmt.get())) {
            collectEnum(*enumDecl);
        }
    }
}

void SemanticAnalyzer::collectEnum(const volta::ast::EnumDeclaration& enumDecl) {
    std::string enumName = enumDecl.name;
    std::vector<std::string> typeParams = enumDecl.typeParams;
    std::vector<EnumType::Variant> variants;

    for (const auto& astVariant : enumDecl.variants) {
        std::string variantName = astVariant->name;
        std::vector<std::shared_ptr<Type>> semanticTypes;
        for (const auto& astType : astVariant->associatedTypes) {
            // Check if this is a type parameter (like T in Option[T])
            if (auto* named = dynamic_cast<const volta::ast::NamedType*>(astType.get())) {
                const std::string& typeName = named->identifier->name;

                // Is this one of the enum's type parameters?
                auto it = std::find(typeParams.begin(), typeParams.end(), typeName);
                if (it != typeParams.end()) {
                    // Yes! Create a TypeVariable instead of resolving
                    semanticTypes.push_back(std::make_shared<TypeVariable>(typeName));
                    continue;
                }
            }

            // Not a type parameter - resolve normally
            auto resolved = resolveTypeAnnotation(astType.get());
            semanticTypes.push_back(resolved);
        }

        variants.push_back(
            EnumType::Variant(variantName, semanticTypes)
        );
    }

    auto enumType = std::make_shared<EnumType>(
        enumName,
        typeParams,
        std::move(variants)
    );

    // Register only the enum type, NOT the individual variants
    // Variants are accessed via qualified access: Color.Red
    symbolTable_->declare(enumName, Symbol{
        enumName,
        enumType,
        false,  // not mutable (types aren't mutable)
        enumDecl.location
    });
}

void SemanticAnalyzer::collectFunction(const volta::ast::FnDeclaration& fn) {
    std::vector<std::shared_ptr<Type>> paramTypes;
    std::vector<bool> paramMutability;

    // If this is a method on a generic struct, we need to handle type parameters
    std::vector<std::string> structTypeParams;
    if (fn.isMethod && !fn.receiverType.empty()) {
        // Look up the receiver type (struct/enum)
        auto receiverSymbol = symbolTable_->lookup(fn.receiverType);
        if (receiverSymbol) {
            if (auto* structType = dynamic_cast<const StructType*>(receiverSymbol->type.get())) {
                structTypeParams = structType->typeParams();
            } else if (auto* enumType = dynamic_cast<const EnumType*>(receiverSymbol->type.get())) {
                structTypeParams = enumType->typeParams();
            }
        }
    }

    // Helper lambda to resolve type with awareness of struct's type parameters
    std::function<std::shared_ptr<Type>(const volta::ast::Type*)> resolveTypeWithParams;
    resolveTypeWithParams = [&](const volta::ast::Type* astType) -> std::shared_ptr<Type> {
        if (!structTypeParams.empty()) {
            // Check if it's a simple named type that's a type parameter
            if (auto* named = dynamic_cast<const volta::ast::NamedType*>(astType)) {
                const std::string& typeName = named->identifier->name;
                auto it = std::find(structTypeParams.begin(), structTypeParams.end(), typeName);
                if (it != structTypeParams.end()) {
                    // Yes! Create a TypeVariable instead of resolving
                    return std::make_shared<TypeVariable>(typeName);
                }
            }

            // Check if it's a generic type like Box[T]
            if (auto* generic = dynamic_cast<const volta::ast::GenericType*>(astType)) {
                const std::string& typeName = generic->identifier->name;

                // Recursively resolve type arguments
                std::vector<std::shared_ptr<Type>> resolvedTypeArgs;
                for (const auto& arg : generic->typeArgs) {
                    resolvedTypeArgs.push_back(resolveTypeWithParams(arg.get()));
                }

                // Look up the base type (e.g., Box)
                auto symbol = symbolTable_->lookup(typeName);
                if (symbol && symbol->type->kind() == Type::Kind::Struct) {
                    auto* structType = dynamic_cast<const StructType*>(symbol->type.get());
                    if (structType) {
                        auto instantiated = structType->instantiate(resolvedTypeArgs);
                        if (instantiated) {
                            return instantiated;
                        }
                    }
                }
            }
        }
        // Not a type parameter - resolve normally
        return resolveTypeAnnotation(astType);
    };

    for (auto& param : fn.parameters) {
        auto resolvedType = resolveTypeWithParams(param->type.get());
        paramTypes.push_back(resolvedType);
        paramMutability.push_back(param->isMutable);
    }

    auto returnTypeResolved = resolveTypeWithParams(fn.returnType.get());

    auto fnType = typeCache_.getFunctionType(std::move(paramTypes), returnTypeResolved, std::move(paramMutability));

    // For methods, use qualified name: "ReceiverType.methodName"
    std::string functionName = fn.identifier;
    if (fn.isMethod) {
        functionName = fn.receiverType + "." + fn.identifier;
    }

    declareFunction(functionName, fnType, fn.location);

}

void SemanticAnalyzer::collectStruct(const volta::ast::StructDeclaration& structDecl) {
    // For now, we'll create a non-generic struct type even if type parameters are present
    // Full generic struct support (instantiation, type checking with type parameters) is future work
    // This allows parsing to succeed without implementing full generics

    std::vector<StructType::Field> fields;
    std::unordered_set<std::string> seenFields;
    std::vector<std::string> typeParams = structDecl.typeParams;

    for (auto& field : structDecl.fields) {
        // Check for duplicate fields
        if (!seenFields.insert(field->identifier).second) {
            error(
                "Duplicate field '" + field->identifier + "' in struct '" + structDecl.identifier + "'",
                structDecl.location
            );
            continue;
        }

        // Resolve field type - handle type parameters similar to enum variants
        std::shared_ptr<Type> resolvedFieldType;
        if (auto* named = dynamic_cast<const volta::ast::NamedType*>(field->type.get())) {
            const std::string& typeName = named->identifier->name;

            // Check if this is one of the struct's type parameters
            auto it = std::find(typeParams.begin(), typeParams.end(), typeName);
            if (it != typeParams.end()) {
                // Create a TypeVariable for the type parameter
                resolvedFieldType = std::make_shared<TypeVariable>(typeName);
            } else {
                // Not a type parameter - resolve normally
                resolvedFieldType = resolveTypeAnnotation(field->type.get());
            }
        } else {
            // Not a named type - resolve normally
            resolvedFieldType = resolveTypeAnnotation(field->type.get());
        }

        auto structField = StructType::Field(field->identifier, resolvedFieldType);
        fields.push_back(structField);
    }

    auto structType = std::make_shared<StructType>(structDecl.identifier, typeParams, std::move(fields));
    declareType(structDecl.identifier, structType, structDecl.location);
}

void SemanticAnalyzer::resolveTypes(const volta::ast::Program& program) {
    // PURPOSE: Second pass - resolve forward type references
    //
    // CURRENTLY EMPTY because Volta doesn't support:
    // - Recursive types: struct Node { next: Option[Node] }
    // - Mutually recursive structs
    // - Type aliases referencing types defined later
    //
    // TODO (Phase 3): Implement type resolution for:
    // 1. Validate all struct field types exist
    // 2. Detect cycles in type definitions
    // 3. Resolve type aliases in order of dependency

    (void)program;  // Suppress unused parameter warning
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
    } else if (dynamic_cast<const volta::ast::BreakStatement*>(stmt)) {
        // Check if we're in a loop
        if (loopDepth_ == 0) {
            error("Break statement outside of loop", stmt->location);
        }
    } else if (dynamic_cast<const volta::ast::ContinueStatement*>(stmt)) {
        // Check if we're in a loop
        if (loopDepth_ == 0) {
            error("Continue statement outside of loop", stmt->location);
        }
    } else if (auto* exprStmt = dynamic_cast<const volta::ast::ExpressionStatement*>(stmt)) {
        analyzeExpression(exprStmt->expr.get());
    } else if (auto* block = dynamic_cast<const volta::ast::Block*>(stmt)) {
        for (auto& currStmt : block->statements) {
            analyzeStatement(currStmt.get());
        }
    } else if (dynamic_cast<const volta::ast::ImportStatement*>(stmt)) {
        // Nothing to do - imports handled elsewhere
    } else if (dynamic_cast<const volta::ast::StructDeclaration*>(stmt)) {
        // Nothing to do - structs already collected in Pass 1
    } else if (dynamic_cast<const volta::ast::EnumDeclaration*>(stmt)) {
        // Nothing to do - enums already collected in Pass 1
    } else {
        throw std::runtime_error("Unexpected node for type checking.");
    }
}

void SemanticAnalyzer::analyzeVarDeclaration(const volta::ast::VarDeclaration* varDecl) {
    // PURPOSE: Type check variable declaration and add to symbol table
    // BEHAVIOR: Handle both explicit types (x: int = 5) and inference (x := 5)

    // If we have a type annotation, resolve it first to use as expected type
    std::shared_ptr<volta::semantic::Type> declaredType;
    if (varDecl->typeAnnotation) {
        declaredType = resolveTypeAnnotation(varDecl->typeAnnotation->type.get());
    }

    // Analyze initializer with expected type if available
    auto initType = analyzeExpression(varDecl->initializer.get(), declaredType);

    std::shared_ptr<volta::semantic::Type> finalType;
    if (declaredType) {
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

    // Look up the already-collected function type (which has correct type parameter handling)
    std::string functionName = fnDecl->identifier;
    if (fnDecl->isMethod) {
        functionName = fnDecl->receiverType + "." + fnDecl->identifier;
    }

    auto fnSymbol = symbolTable_->lookup(functionName);
    if (!fnSymbol || fnSymbol->type->kind() != Type::Kind::Function) {
        // Shouldn't happen if collectFunction worked correctly
        error("Function '" + functionName + "' not found in symbol table", fnDecl->location);
        return;
    }

    auto* fnType = dynamic_cast<const FunctionType*>(fnSymbol->type.get());
    if (!fnType) {
        error("Symbol '" + functionName + "' is not a function", fnDecl->location);
        return;
    }

    enterScope();

    // Use the parameter types from the collected function type
    for (size_t i = 0; i < fnDecl->parameters.size(); ++i) {
        auto paramType = fnType->paramTypes()[i];
        bool isMutable = i < fnType->paramMutability().size() ? fnType->paramMutability()[i] : false;
        declareVariable(fnDecl->parameters[i]->identifier, paramType, isMutable, fnDecl->location);
    }

    // Use the return type from the collected function type
    auto retType = fnType->returnType();
    enterFunction(retType);

    
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
    auto boolType = typeCache_.getBool();

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
    auto boolType = typeCache_.getBool();

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
        elementType = typeCache_.getInt();
    }
    else {
        error("Expression is not iterable", forStmt->location);
        elementType = typeCache_.getUnknown();
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
    auto expectedType = currentFunctionReturnType();

    if (returnStmt->expression) {
        // Pass expected type for contextual inference!
        returnType = analyzeExpression(returnStmt->expression.get(), expectedType);
    } else {
        // Empty return statement
        returnType = typeCache_.getVoid();
    }

    if (!areTypesCompatible(expectedType.get(), returnType.get())) {
        typeError("Return type mismatch", expectedType.get(), returnType.get(), returnStmt->location);
    }
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeExpression(const volta::ast::Expression* expr,
                                                          std::shared_ptr<Type> expectedType) {
    std::shared_ptr<Type> resultType;

    if (dynamic_cast<const volta::ast::IntegerLiteral*>(expr) ||
        dynamic_cast<const volta::ast::FloatLiteral*>(expr) ||
        dynamic_cast<const volta::ast::StringLiteral*>(expr) ||
        dynamic_cast<const volta::ast::BooleanLiteral*>(expr)) {
        resultType = analyzeLiteral(expr);
    }
    else if (auto* binExpr = dynamic_cast<const volta::ast::BinaryExpression*>(expr)) {
        resultType = analyzeBinaryExpression(binExpr, expectedType);
    }
    else if (auto* unaryExpr = dynamic_cast<const volta::ast::UnaryExpression*>(expr)) {
        resultType = analyzeUnaryExpression(unaryExpr, expectedType);
    }
    else if (auto* identifier = dynamic_cast<const volta::ast::IdentifierExpression*>(expr)) {
        resultType = analyzeIdentifier(identifier);
    }
    else if (auto* callExpr = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        resultType = analyzeCallExpression(callExpr, expectedType);
    }
    else if (auto* matchExpr = dynamic_cast<const volta::ast::MatchExpression*>(expr)) {
        resultType = analyzeMatchExpression(matchExpr);
    }
    else if (auto* memberExpr = dynamic_cast<const volta::ast::MemberExpression*>(expr)) {
        resultType = analyzeMemberExpression(memberExpr, expectedType);
    }
    else if (auto* methodCall = dynamic_cast<const volta::ast::MethodCallExpression*>(expr)) {
        resultType = analyzeMethodCallExpression(methodCall, expectedType);
    }
    else if (auto* arrayLit = dynamic_cast<const volta::ast::ArrayLiteral*>(expr)) {
        resultType = analyzeArrayLiteral(arrayLit, expectedType);
    }
    else if (auto* indexExpr = dynamic_cast<const volta::ast::IndexExpression*>(expr)) {
        resultType = analyzeIndexExpression(indexExpr);
    }
    else if (auto* structLit = dynamic_cast<const volta::ast::StructLiteral*>(expr)) {
        resultType = analyzeStructLiteral(structLit, expectedType);
    } else if (auto* castExpr = dynamic_cast<const volta::ast::CastExpression*>(expr)) {
        resultType = analyzeCastExpression(castExpr);
    }
    else {
        resultType = typeCache_.getUnknown();
    }

    expressionTypes_[expr] = resultType;
    return resultType;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeLiteral(const volta::ast::Expression* literal) {
    if (dynamic_cast<const volta::ast::IntegerLiteral*>(literal)) {
        return typeCache_.getInt();
    }
    else if (dynamic_cast<const volta::ast::FloatLiteral*>(literal)) {
        return typeCache_.getFloat();
    }
    else if (dynamic_cast<const volta::ast::BooleanLiteral*>(literal)) {
        return typeCache_.getBool();
    }
    else if (dynamic_cast<const volta::ast::StringLiteral*>(literal)) {
        return typeCache_.getString();
    }
    return typeCache_.getUnknown();
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeIdentifier(const volta::ast::IdentifierExpression* identifier) {
    return lookupVariable(identifier->name, identifier->location);
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeBinaryExpression(const volta::ast::BinaryExpression* binExpr,
                                                                std::shared_ptr<Type> expectedType) {
    // expectedType not used for binary expressions (can't infer operand types from result)
    (void)expectedType;
    auto leftType = analyzeExpression(binExpr->left.get());
    auto rightType = analyzeExpression(binExpr->right.get());

    // Handle assignment operators
    using Op = volta::ast::BinaryExpression::Operator;
    if (binExpr->op == Op::Assign || binExpr->op == Op::AddAssign ||
        binExpr->op == Op::SubtractAssign || binExpr->op == Op::MultiplyAssign ||
        binExpr->op == Op::DivideAssign) {

        // Check if left side is a valid l-value
        bool isValidLValue = false;

        // 1. Simple identifier (variable)
        if (auto* ident = dynamic_cast<const volta::ast::IdentifierExpression*>(binExpr->left.get())) {
            // Only check mutability if the variable exists (undefined variables already reported by analyzeExpression above)
            if (symbolTable_->lookup(ident->name)) {
                if (!isVariableMutable(ident->name)) {
                    error("Cannot assign to immutable variable '" + ident->name + "'", binExpr->location);
                }
            }
            isValidLValue = true;
        }
        // 2. Member access (struct.field)
        else if (auto* memberExpr = dynamic_cast<const volta::ast::MemberExpression*>(binExpr->left.get())) {
            if (!isExpressionMutable(memberExpr->object.get())) {
                error("Cannot assign to field of immutable object", binExpr->location);
            }
            isValidLValue = true;
        }
        // 3. Array indexing (array[index])
        else if (auto* indexExpr = dynamic_cast<const volta::ast::IndexExpression*>(binExpr->left.get())) {
            if (!isExpressionMutable(indexExpr->object.get())) {
                error("Cannot assign to element of immutable array", binExpr->location);
            }
            isValidLValue = true;
        }

        if (!isValidLValue) {
            error("Left side of assignment must be a variable, struct field, or array element", binExpr->location);
            return leftType;
        }

        if (!areTypesCompatible(leftType.get(), rightType.get())) {
            typeError("Type mismatch in assignment", leftType.get(), rightType.get(), binExpr->location);
        }

        return leftType;
    }

    return inferBinaryOpType(binExpr->op, leftType.get(), rightType.get());
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeUnaryExpression(const volta::ast::UnaryExpression* unaryExpr,
                                                               std::shared_ptr<Type> expectedType) {
    (void)expectedType;  // Not used
    auto operandType = analyzeExpression(unaryExpr->operand.get());
    return inferUnaryOpType(unaryExpr->op, operandType.get());
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeCallExpression(const volta::ast::CallExpression* callExpr,
                                                              std::shared_ptr<Type> expectedType) {
    (void)expectedType;  // Could be used for function overload resolution in future
    // First, analyze all argument types
    std::vector<std::shared_ptr<Type>> argTypes;
    for (const auto& arg : callExpr->arguments) {
        argTypes.push_back(analyzeExpression(arg.get()));
    }

    // Check if callee is an identifier (for overload resolution)
    if (auto* identExpr = dynamic_cast<const volta::ast::IdentifierExpression*>(callExpr->callee.get())) {
        // Try to resolve overload based on argument types
        auto symbol = symbolTable_->lookupOverload(identExpr->name, argTypes);

        if (symbol.has_value()) {
            auto* fnType = dynamic_cast<const FunctionType*>(symbol->type.get());
            if (fnType) {
                // Check mutability for each argument
                const auto& mutability = fnType->paramMutability();
                for (size_t i = 0; i < callExpr->arguments.size(); ++i) {
                    if (!mutability.empty() && i < mutability.size() && mutability[i]) {
                        if (!isExpressionMutable(callExpr->arguments[i].get())) {
                            error("Cannot pass immutable expression to mutable parameter", callExpr->location);
                        }
                    }
                }
                return fnType->returnType();
            }
        }

        // No matching overload found - provide helpful error
        auto allOverloads = symbolTable_->lookupAllOverloads(identExpr->name);
        if (!allOverloads.empty()) {
            error("No matching overload for function '" + identExpr->name + "' with given argument types", callExpr->location);
            return typeCache_.getUnknown();
        }

        // Function doesn't exist at all
        error("Undefined function '" + identExpr->name + "'", callExpr->location);
        return typeCache_.getUnknown();
    }

    // Not an identifier - analyze as a regular expression
    auto calleeType = analyzeExpression(callExpr->callee.get());

    auto* fnType = dynamic_cast<const FunctionType*>(calleeType.get());
    if (!fnType) {
        error("Expression is not callable", callExpr->location);
        return typeCache_.getUnknown();
    }

    if (callExpr->arguments.size() != fnType->paramTypes().size()) {
        error("Wrong number of arguments", callExpr->location);
        return fnType->returnType();
    }

    for (size_t i = 0; i < callExpr->arguments.size(); ++i) {
        auto expectedType = fnType->paramTypes()[i];
        if (!areTypesCompatible(expectedType.get(), argTypes[i].get())) {
            typeError("Argument type mismatch", expectedType.get(), argTypes[i].get(), callExpr->location);
        }

        // Check mutability: if parameter is mutable, argument must be mutable
        const auto& mutability = fnType->paramMutability();
        if (!mutability.empty() && i < mutability.size() && mutability[i]) {
            if (!isExpressionMutable(callExpr->arguments[i].get())) {
                error("Cannot pass immutable expression to mutable parameter", callExpr->location);
            }
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

    return resultType ? resultType : typeCache_.getUnknown();
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeMemberExpression(const volta::ast::MemberExpression* memberExpr,
                                                                std::shared_ptr<Type> expectedType) {
    // For member access like `Color.Red`, the object is an identifier expression
    // We need to check if it's an enum type
    // expectedType could be used for enum variants without data (like Option.None)
    auto* identExpr = dynamic_cast<const volta::ast::IdentifierExpression*>(memberExpr->object.get());

    if (identExpr) {
        // Check if this identifier is an enum type
        auto symbol = symbolTable_->lookup(identExpr->name);
        if (symbol && symbol->type->kind() == Type::Kind::Enum) {
            auto* enumType = dynamic_cast<const EnumType*>(symbol->type.get());
            const std::string& variantName = memberExpr->member->name;

            // Look up the variant in the enum
            const auto* variant = enumType->getVariant(variantName);
            if (!variant) {
                error("Enum '" + enumType->name() + "' has no variant '" + variantName + "'",
                      memberExpr->location);
                return typeCache_.getUnknown();
            }

            // For generic enums with variants that have NO data (like Option.None),
            // use expected type for inference!
            if (!enumType->typeParams().empty() && variant->associatedTypes.empty()) {
                if (expectedType) {
                    if (auto* expectedEnum = dynamic_cast<const EnumType*>(expectedType.get())) {
                        if (expectedEnum->name() == enumType->name() && expectedEnum->isInstantiated()) {
                            // Return the instantiated type from context!
                            return expectedType;
                        }
                    }
                }
                // No expected type - can't infer
                error("Cannot infer type parameters for '" + enumType->name() + "." + variantName +
                      "' - use in a context with known type or provide explicit type arguments",
                      memberExpr->location);
                return typeCache_.getUnknown();  // Return Unknown instead of template
            }

            // Return the enum type (the variant constructs an instance of the enum)
            return symbol->type;  // Return the shared_ptr to the enum type
        }
    }

    // If not an enum, try struct member access
    auto objectType = analyzeExpression(memberExpr->object.get());

    auto* structType = dynamic_cast<const StructType*>(objectType.get());
    if (!structType) {
        error("Member access on non-struct type", memberExpr->location);
        return typeCache_.getUnknown();
    }

    const std::string& fieldName = memberExpr->member->name;
    const auto* field = structType->getField(fieldName);

    if (!field) {
        error("Struct '" + structType->name() + "' has no field '" + fieldName + "'",
              memberExpr->location);
        return typeCache_.getUnknown();
    }

    return field->type;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeMethodCallExpression(const volta::ast::MethodCallExpression* methodCall,
                                                                   std::shared_ptr<Type> expectedType) {
    // Check if the object is a type name (for static methods or enum variants)
    auto* identExpr = dynamic_cast<const volta::ast::IdentifierExpression*>(methodCall->object.get());
    if (identExpr) {
        auto symbol = symbolTable_->lookup(identExpr->name);

        // Case 1: Enum variant construction (Option.Some(42))
        if (symbol && symbol->type->kind() == Type::Kind::Enum) {
            auto* enumType = dynamic_cast<const EnumType*>(symbol->type.get());
            const std::string& variantName = methodCall->method->name;

            // Look up the variant
            const auto* variant = enumType->getVariant(variantName);
            if (!variant) {
                error("Enum '" + enumType->name() + "' has no variant '" + variantName + "'",
                      methodCall->location);
                return typeCache_.getUnknown();
            }

            // Check argument count
            if (methodCall->arguments.size() != variant->associatedTypes.size()) {
                error("Variant '" + variantName + "' expects " +
                      std::to_string(variant->associatedTypes.size()) + " arguments, got " +
                      std::to_string(methodCall->arguments.size()),
                      methodCall->location);
                return typeCache_.getUnknown();
            }

            // Analyze argument types first
            std::vector<std::shared_ptr<Type>> argumentTypes;
            for (const auto& arg : methodCall->arguments) {
                argumentTypes.push_back(analyzeExpression(arg.get()));
            }

            // If the enum is generic (has type parameters), infer them from arguments OR expected type
            if (!enumType->typeParams().empty()) {
                std::vector<std::shared_ptr<Type>> inferredTypeArgs;

                // STRATEGY 1: Infer from arguments (if variant has data)
                if (!variant->associatedTypes.empty()) {
                    for (size_t i = 0; i < variant->associatedTypes.size(); ++i) {
                        auto* typeVar = dynamic_cast<const TypeVariable*>(variant->associatedTypes[i].get());
                        if (typeVar) {
                            // This is a type variable - use the argument's type
                            inferredTypeArgs.push_back(argumentTypes[i]);
                        }
                    }
                }

                // STRATEGY 2: If no inference from args (e.g., Option.None), use expected type!
                if (inferredTypeArgs.empty() && expectedType) {
                    // Check if expected type is an instantiation of this enum
                    if (auto* expectedEnum = dynamic_cast<const EnumType*>(expectedType.get())) {
                        if (expectedEnum->name() == enumType->name() && expectedEnum->isInstantiated()) {
                            // Use the type arguments from expected type!
                            inferredTypeArgs = expectedEnum->typeArgs();
                        }
                    }
                }

                // Create instantiated enum type: Option[T] → Option[int]
                if (!inferredTypeArgs.empty()) {
                    auto instantiated = enumType->instantiate(inferredTypeArgs);
                    if (instantiated) {
                        // Type-check with the instantiated variant
                        auto* instVariant = instantiated->getVariant(variantName);
                        if (instVariant) {
                            for (size_t i = 0; i < argumentTypes.size(); ++i) {
                                if (!areTypesCompatible(instVariant->associatedTypes[i].get(), argumentTypes[i].get())) {
                                    error("Argument " + std::to_string(i + 1) + " to variant '" + variantName +
                                          "' has incompatible type",
                                          methodCall->location);
                                }
                            }
                        }
                        return instantiated;
                    }
                }
            }

            // For non-generic enums, check types normally
            if (enumType->typeParams().empty()) {
                for (size_t i = 0; i < argumentTypes.size(); ++i) {
                    if (!areTypesCompatible(variant->associatedTypes[i].get(), argumentTypes[i].get())) {
                        error("Argument " + std::to_string(i + 1) + " to variant '" + variantName +
                              "' has incompatible type",
                              methodCall->location);
                    }
                }
                // Non-generic enum, return the enum type directly
                return symbol->type;
            }

            // If we reach here, it's a generic enum with NO arguments and NO expected type
            // We can't infer - this is an error
            error("Cannot infer type parameters for '" + enumType->name() + "." + variantName +
                  "' - provide explicit type or use in a context with known type",
                  methodCall->location);
            return symbol->type;  // Return template type (will fail type checking)
        }

        // Case 2: Static method call on struct type (Point.new(1.0, 1.0))
        // Check if it's a type name (not a variable)
        if (symbol && symbol->type->kind() == Type::Kind::Struct) {
            auto* structType = dynamic_cast<const StructType*>(symbol->type.get());

            // Distinguish between type name and variable:
            // - Type name: identExpr->name == structType->name() (e.g., "Point" == "Point")
            // - Variable: identExpr->name != structType->name() (e.g., "p" != "Point")
            if (identExpr->name == structType->name()) {
                // It's a static method call on the type itself
                const std::string& methodName = methodCall->method->name;
                const std::string qualifiedName = structType->name() + "." + methodName;

                // Lookup the static method
                auto methodSymbol = symbolTable_->lookup(qualifiedName);
                if (!methodSymbol) {
                    error("Struct '" + structType->name() + "' has no static method '" + methodName + "'",
                          methodCall->location);
                    return typeCache_.getUnknown();
                }

                auto* fnType = dynamic_cast<const FunctionType*>(methodSymbol->type.get());
                if (!fnType) {
                    error("'" + qualifiedName + "' is not a function", methodCall->location);
                    return typeCache_.getUnknown();
                }

                // Static methods don't have 'self' parameter, so arg count should match exactly
                if (methodCall->arguments.size() != fnType->paramTypes().size()) {
                    error("Wrong number of arguments to method '" + methodName + "'",
                          methodCall->location);
                    return typeCache_.getUnknown();
                }

                // Analyze all arguments first
                std::vector<std::shared_ptr<Type>> argTypes;
                for (const auto& arg : methodCall->arguments) {
                    argTypes.push_back(analyzeExpression(arg.get()));
                }

                // For generic structs, infer type parameters from argument types
                std::vector<std::shared_ptr<Type>> inferredTypeArgs;
                if (!structType->typeParams().empty()) {
                    // Initialize with nullptrs
                    inferredTypeArgs.resize(structType->typeParams().size(), nullptr);

                    // Infer type arguments by matching argument types with parameter types
                    for (size_t i = 0; i < argTypes.size(); ++i) {
                        auto paramType = fnType->paramTypes()[i];

                        // If param is a type variable, use the arg type
                        if (paramType->kind() == Type::Kind::TypeVariable) {
                            auto* typeVar = dynamic_cast<const TypeVariable*>(paramType.get());
                            // Find which type parameter this is
                            for (size_t j = 0; j < structType->typeParams().size(); ++j) {
                                if (structType->typeParams()[j] == typeVar->name()) {
                                    if (!inferredTypeArgs[j]) {
                                        inferredTypeArgs[j] = argTypes[i];
                                    }
                                }
                            }
                        }
                    }
                }

                // Type-check arguments (with substituted types if generic)
                const auto& mutability = fnType->paramMutability();
                for (size_t i = 0; i < argTypes.size(); ++i) {
                    auto expectedType = fnType->paramTypes()[i];

                    // Substitute type variables if we inferred them
                    if (!inferredTypeArgs.empty() && !structType->typeParams().empty()) {
                        expectedType = substituteTypeVariables(expectedType, structType->typeParams(), inferredTypeArgs);
                    }

                    if (!areTypesCompatible(expectedType.get(), argTypes[i].get())) {
                        typeError("Argument type mismatch in method call",
                                 expectedType.get(), argTypes[i].get(), methodCall->location);
                    }

                    // Check mutability
                    if (!mutability.empty() && i < mutability.size() && mutability[i]) {
                        if (!isExpressionMutable(methodCall->arguments[i].get())) {
                            error("Cannot pass immutable expression to mutable parameter", methodCall->location);
                        }
                    }
                }

                // Substitute type variables in return type
                auto returnType = fnType->returnType();
                if (!inferredTypeArgs.empty() && !structType->typeParams().empty()) {
                    returnType = substituteTypeVariables(returnType, structType->typeParams(), inferredTypeArgs);
                }

                return returnType;
            }
        }
    }

    // Otherwise, it's a regular instance method call on a struct
    auto objectType = analyzeExpression(methodCall->object.get());

    auto* structType = dynamic_cast<const StructType*>(objectType.get());
    if (!structType) {
        error("Method call on non-struct type", methodCall->location);
        return typeCache_.getUnknown();
    }

    const std::string& methodName = methodCall->method->name;
    // Use base name without type arguments for method lookup
    // E.g., "Box[int].getValue" -> "Box.getValue"
    std::string baseName = structType->name();
    size_t bracketPos = baseName.find('[');
    if (bracketPos != std::string::npos) {
        baseName = baseName.substr(0, bracketPos);
    }
    const std::string qualifiedName = baseName + "." + methodName;

    auto methodSymbol = symbolTable_->lookup(qualifiedName);
    if (!methodSymbol) {
        error("Struct '" + baseName + "' has no method '" + methodName + "'",
              methodCall->location);
        return typeCache_.getUnknown();
    }

    auto baseFnType = dynamic_cast<const FunctionType*>(methodSymbol->type.get());
    if (!baseFnType) {
        error("'" + qualifiedName + "' is not a method", methodCall->location);
        return typeCache_.getUnknown();
    }

    // If the struct is a generic instantiation, we need to substitute type variables
    // in the method's function type
    std::shared_ptr<FunctionType> fnType = std::const_pointer_cast<FunctionType>(
        std::shared_ptr<const FunctionType>(methodSymbol->type, baseFnType)
    );

    if (structType->isInstantiated() && !structType->typeArgs().empty()) {
        // Substitute type parameters in the function type
        std::vector<std::shared_ptr<Type>> instantiatedParamTypes;
        for (const auto& paramType : baseFnType->paramTypes()) {
            instantiatedParamTypes.push_back(
                substituteTypeVariables(paramType, structType->typeParams(), structType->typeArgs())
            );
        }

        auto instantiatedReturnType = substituteTypeVariables(
            baseFnType->returnType(),
            structType->typeParams(),
            structType->typeArgs()
        );

        fnType = std::make_shared<FunctionType>(
            std::move(instantiatedParamTypes),
            instantiatedReturnType,
            baseFnType->paramMutability()
        );
    }

    // Check mutability of 'self' parameter (first parameter)
    const auto& mutability = fnType->paramMutability();
    if (!mutability.empty() && mutability.size() > 0 && mutability[0]) {
        // Method expects mutable self
        if (!isExpressionMutable(methodCall->object.get())) {
            error("Cannot call method with mutable 'self' on immutable object", methodCall->location);
        }
    }

    size_t expectedArgCount = fnType->paramTypes().size() - 1;
    if (methodCall->arguments.size() != expectedArgCount) {
        error("Wrong number of arguments to method '" + methodName + "'",
              methodCall->location);
        return typeCache_.getUnknown();  // Return early to avoid further errors
    } else {
        for (size_t i = 0; i < methodCall->arguments.size(); ++i) {
            auto argType = analyzeExpression(methodCall->arguments[i].get());
            auto expectedType = fnType->paramTypes()[i + 1];
            if (!areTypesCompatible(expectedType.get(), argType.get())) {
                typeError("Argument type mismatch in method call",
                         expectedType.get(), argType.get(), methodCall->location);
            }

            // Check mutability of additional parameters (skip self, which is at index 0)
            size_t paramIndex = i + 1;
            if (!mutability.empty() && paramIndex < mutability.size() && mutability[paramIndex]) {
                if (!isExpressionMutable(methodCall->arguments[i].get())) {
                    error("Cannot pass immutable expression to mutable parameter", methodCall->location);
                }
            }
        }
    }

    return fnType->returnType();
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeArrayLiteral(const volta::ast::ArrayLiteral* arrayLit,
                                                            std::shared_ptr<Type> expectedType) {
    (void)expectedType;  // Could use to infer element type in empty arrays
    if (arrayLit->elements.empty()) {
        return typeCache_.getArrayType(typeCache_.getUnknown());
    }

    auto elemType = analyzeExpression(arrayLit->elements[0].get());
    return typeCache_.getArrayType(elemType);
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeIndexExpression(const volta::ast::IndexExpression* indexExpr) {
    auto arrayType = analyzeExpression(indexExpr->object.get());
    auto indexType = analyzeExpression(indexExpr->index.get());

    if (indexType->kind() != Type::Kind::Int) {
        typeError("Array index must be an integer",
                 typeCache_.getInt().get(),
                 indexType.get(), indexExpr->location);
    }

    if (!arrayType->isIndexable()) {
        error("Index operator can only be used on indexable types (arrays, matrices)", indexExpr->location);
        return typeCache_.getUnknown();
    }

    if (auto* arrType = dynamic_cast<const ArrayType*>(arrayType.get())) {
        return arrType->elementType();
    }

    return typeCache_.getUnknown();
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeStructLiteral(const volta::ast::StructLiteral* structLit,
                                                             std::shared_ptr<Type> expectedType) {
    const std::string& structName = structLit->structName->name;
    auto symbol = symbolTable_->lookup(structName);

    if (!symbol) {
        error("Unknown struct type '" + structName + "'", structLit->location);
        return typeCache_.getUnknown();
    }

    if (symbol->type->kind() != Type::Kind::Struct) {
        error("'" + structName + "' is not a struct type", structLit->location);
        return typeCache_.getUnknown();
    }

    auto baseStructType = std::dynamic_pointer_cast<StructType>(symbol->type);
    std::shared_ptr<StructType> structType = baseStructType;

    // If expectedType is a generic struct instantiation (e.g., Pair[int]),
    // use it to validate and return the correct type
    if (expectedType && expectedType->kind() == Type::Kind::Struct) {
        auto* expectedStruct = dynamic_cast<const StructType*>(expectedType.get());
        if (expectedStruct && expectedStruct->name() == structName && expectedStruct->isInstantiated()) {
            // Use the expected type's instantiation
            structType = std::static_pointer_cast<StructType>(expectedType);
        }
    }

    // If the struct is generic but not yet instantiated, we need to infer type arguments from field values
    if (baseStructType->typeParams().size() > 0 && !structType->isInstantiated()) {
        // Infer type arguments from field initializers
        std::vector<std::shared_ptr<Type>> inferredTypeArgs;

        // For simple case: all fields have the same type parameter (like Pair[T])
        // Analyze the first field to infer T
        if (!structLit->fields.empty()) {
            auto firstFieldValue = analyzeExpression(structLit->fields[0]->value.get());
            inferredTypeArgs.push_back(firstFieldValue);

            // Instantiate the struct with inferred types
            auto instantiated = baseStructType->instantiate(inferredTypeArgs);
            if (instantiated) {
                structType = instantiated;
            }
        }
    }

    auto finalStructType = structType;

    // Validate and type-check all fields
    for (const auto& fieldInit : structLit->fields) {
        const std::string& fieldName = fieldInit->identifier->name;
        const auto* field = finalStructType->getField(fieldName);

        if (!field) {
            error("Struct '" + structName + "' has no field '" + fieldName + "'",
                  structLit->location);
            continue;
        }

        auto valueType = analyzeExpression(fieldInit->value.get());
        if (!areTypesCompatible(field->type.get(), valueType.get())) {
            typeError("Field '" + fieldName + "' type mismatch",
                     field->type.get(), valueType.get(), structLit->location);
        }
    }

    // Sort fields to match struct definition order for easier IR generation
    auto& fields = const_cast<volta::ast::StructLiteral*>(structLit)->fields;
    std::sort(fields.begin(), fields.end(),
        [&finalStructType](const auto& a, const auto& b) {
            int indexA = finalStructType->getFieldIndex(a->identifier->name);
            int indexB = finalStructType->getFieldIndex(b->identifier->name);
            return indexA < indexB;
        });

    return finalStructType;
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
            return typeCache_.getInt();
        }
        if (left->isNumeric() && right->isNumeric()) {
            return typeCache_.getFloat();
        }
        return typeCache_.getUnknown();
    }

    // Comparison operators
    if (op == Op::Equal || op == Op::NotEqual || op == Op::Less ||
        op == Op::Greater || op == Op::LessEqual || op == Op::GreaterEqual) {
        return typeCache_.getBool();
    }

    // Logical operators
    if (op == Op::LogicalAnd || op == Op::LogicalOr) {
        return typeCache_.getBool();
    }

    // Range operators
    if (op == Op::Range || op == Op::RangeInclusive) {
        return typeCache_.getInt();
    }

    return typeCache_.getUnknown();
}

std::shared_ptr<Type> SemanticAnalyzer::inferUnaryOpType(
    volta::ast::UnaryExpression::Operator op,
    const Type* operand
) {
    using Op = volta::ast::UnaryExpression::Operator;

    if (op == Op::Negate) {
        if (operand->isNumeric()) {
            return operand->kind() == Type::Kind::Int ? typeCache_.getInt() : typeCache_.getFloat();
        }
    }

    if (op == Op::Not) {
        if (operand->kind() == Type::Kind::Bool) {
            return typeCache_.getBool();
        }
    }

    return typeCache_.getUnknown();
}

bool SemanticAnalyzer::isExpressionMutable(const volta::ast::Expression* expr) {
    // 1. Simple identifier - check symbol table
    if (auto* ident = dynamic_cast<const volta::ast::IdentifierExpression*>(expr)) {
        return isVariableMutable(ident->name);
    }

    // 2. Member expression - check base object mutability
    if (auto* member = dynamic_cast<const volta::ast::MemberExpression*>(expr)) {
        return isExpressionMutable(member->object.get());
    }

    // 3. Index expression - check base array mutability
    if (auto* index = dynamic_cast<const volta::ast::IndexExpression*>(expr)) {
        return isExpressionMutable(index->object.get());
    }

    // All other expressions (literals, calls, etc.) are not mutable
    return false;
}

std::shared_ptr<Type> SemanticAnalyzer::analyzeCastExpression(const volta::ast::CastExpression* castExpr) {
    // 1. Analyze the expression being cast
    auto sourceType = analyzeExpression(castExpr->expression.get());
    
    // 2. Get the target type
    auto targetType = resolveTypeAnnotation(castExpr->targetType.get());
    
    // 3. Check if the cast is valid
    bool validCast = false;
    
    if (sourceType->kind() == Type::Kind::Int && targetType->kind() == Type::Kind::Float) {
        validCast = true;  // int → float
    } else if (sourceType->kind() == Type::Kind::Float && targetType->kind() == Type::Kind::Int) {
        validCast = true;  // float → int
    }
    
    if (!validCast) {
        error("Invalid cast from " + sourceType->toString() + " to " + targetType->toString(), 
              castExpr->location);
    }
    
    // 4. Return the target type
    return targetType;
}

std::shared_ptr<Type> SemanticAnalyzer::substituteTypeVariables(
    const std::shared_ptr<Type>& type,
    const std::vector<std::string>& typeParams,
    const std::vector<std::shared_ptr<Type>>& typeArgs
) {
    // If it's a type variable, substitute it
    if (auto* typeVar = dynamic_cast<const TypeVariable*>(type.get())) {
        for (size_t i = 0; i < typeParams.size(); ++i) {
            if (typeParams[i] == typeVar->name()) {
                // Make sure we have a valid type arg
                if (i < typeArgs.size() && typeArgs[i]) {
                    return typeArgs[i];
                }
            }
        }
        // Couldn't substitute, return as-is
        return type;
    }

    // If it's a struct type with type parameters (like Box[T])
    if (auto* structType = dynamic_cast<const StructType*>(type.get())) {
        // Check if this struct has type parameters that need substitution
        if (!structType->typeParams().empty()) {
            // Get the base struct template by name
            std::string baseName = structType->name();
            size_t bracketPos = baseName.find('[');
            if (bracketPos != std::string::npos) {
                baseName = baseName.substr(0, bracketPos);
            }

            // Build new type args by substituting type variables
            std::vector<std::shared_ptr<Type>> newTypeArgs;

            if (structType->isInstantiated()) {
                // Already instantiated with concrete or variable types
                // Substitute variables in the type args
                for (const auto& arg : structType->typeArgs()) {
                    newTypeArgs.push_back(substituteTypeVariables(arg, typeParams, typeArgs));
                }
            } else {
                // Not instantiated - struct uses type parameters like T
                // Match struct's type params with our substitution map
                for (const auto& structTypeParam : structType->typeParams()) {
                    bool found = false;
                    for (size_t i = 0; i < typeParams.size(); ++i) {
                        if (typeParams[i] == structTypeParam) {
                            if (i < typeArgs.size() && typeArgs[i]) {
                                newTypeArgs.push_back(typeArgs[i]);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        // Couldn't find substitution - use Unknown
                        newTypeArgs.push_back(typeCache_.getUnknown());
                    }
                }
            }

            // Instantiate the struct with the new type args
            auto baseStruct = symbolTable_->lookup(baseName);
            if (baseStruct && baseStruct->type->kind() == Type::Kind::Struct) {
                auto* base = dynamic_cast<const StructType*>(baseStruct->type.get());
                if (base) {
                    auto instantiated = base->instantiate(newTypeArgs);
                    if (instantiated) {
                        return instantiated;
                    }
                }
            }
        }
    }

    // For other types (primitives, already concrete types, etc.), return as-is
    return type;
}

}