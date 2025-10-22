#include "MIR/HIRToMIR.hpp"
#include "HIR/HIR.hpp"
#include <sstream>

namespace MIR {
    
MIR::Program HIRToMIR::lower(const HIR::HIRProgram& hirProgram) {
    // Declare volta_gc_malloc as extern function
    std::vector<Value> gcMallocParams;
    const Type::Type* i64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::I64);
    gcMallocParams.push_back(Value::makeParam("size", i64Type));
    const Type::Type* voidPtrType = typeRegistry.getPointer(typeRegistry.getPrimitive(Type::PrimitiveKind::Void));
    builder.createFunction("volta_gc_malloc", gcMallocParams, voidPtrType);
    builder.finishFunction();

    for (auto& topLvlStmt : hirProgram.statements) {
        // Handle function declarations
        if (auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(topLvlStmt.get())) {
            // Convert parameters
            std::vector<Value> params;
            for (auto& par : fnDecl->params) {
                // Structs and arrays are always passed by reference (pointer)
                const Type::Type* paramType = par.type;
                if (paramType->kind == Type::TypeKind::Struct ||
                    paramType->kind == Type::TypeKind::Array) {
                    paramType = typeRegistry.getPointer(paramType);
                }
                params.push_back(Value::makeParam(par.name, paramType));
            }

            // Convert return type - structs and arrays are returned by reference (pointer)
            const Type::Type* returnType = fnDecl->returnType;
            if (returnType->kind == Type::TypeKind::Struct ||
                returnType->kind == Type::TypeKind::Array) {
                returnType = typeRegistry.getPointer(returnType);
            }

            builder.createFunction(fnDecl->name, params, returnType);
            builder.createBlock("entry");
            varPointers.clear();

            // Set up parameters
            for (size_t i = 0; i < params.size(); i++) {
                auto& param = params[i];
                auto& paramDecl = fnDecl->params[i];

                // For mut ref parameters of primitive types, we need to allocate space
                // But for arrays/structs, they're already passed as pointers, so just use them directly
                if (paramDecl.isMutRef &&
                    paramDecl.type->kind != Type::TypeKind::Array &&
                    paramDecl.type->kind != Type::TypeKind::Struct) {
                    Value ptr = builder.createAlloca(param.type);
                    builder.createStore(param, ptr);
                    varPointers[param.name] = ptr;
                } else {
                    builder.setVariable(param.name, param);
                }
            }

            // Lower function body
            for (auto& stmt : fnDecl->body) {
                lowerHIRStmt(*stmt);
            }

            // Add return if missing
            if (!builder.currentBlockTerminated()) {
                if (fnDecl->returnType->kind == Type::TypeKind::Primitive) {
                    const auto* primType = static_cast<const Type::PrimitiveType*>(fnDecl->returnType);
                    if (primType->kind == Type::PrimitiveKind::Void) {
                        builder.createReturnVoid();
                    } else {
                        diag.error("Function must return a value", fnDecl->line, fnDecl->column);
                    }
                } else {
                    diag.error("Function must return a value", fnDecl->line, fnDecl->column);
                }
            }
            builder.finishFunction();
        }
        // Handle extern blocks
        else if (auto* externBlock = dynamic_cast<HIR::HIRExternBlock*>(topLvlStmt.get())) {
            // Add each extern function declaration to MIR
            for (auto& fnDecl : externBlock->declarations) {
                std::vector<Value> params;
                for (const auto& param : fnDecl->params) {
                    // Structs and arrays are passed by reference (pointer)
                    const Type::Type* paramType = param.type;
                    if (paramType->kind == Type::TypeKind::Struct ||
                        paramType->kind == Type::TypeKind::Array) {
                        paramType = typeRegistry.getPointer(paramType);
                    }
                    params.push_back(Value::makeParam(param.name, paramType));
                }

                // Structs and arrays are returned by reference (pointer)
                const Type::Type* returnType = fnDecl->returnType;
                if (returnType->kind == Type::TypeKind::Struct ||
                    returnType->kind == Type::TypeKind::Array) {
                    returnType = typeRegistry.getPointer(returnType);
                }

                // Create MIR function with no body (empty blocks = extern)
                builder.createFunction(fnDecl->name, params, returnType);
                builder.finishFunction();
            }
        }
        // Handle struct declarations with methods
        else if (auto* structDecl = dynamic_cast<HIR::HIRStructDecl*>(topLvlStmt.get())) {
            // Lower each method as a mangled function
            for (auto& method : structDecl->methods) {
                // Mangle the function name: StructName__methodName
                std::string mangledName = structDecl->name.lexeme + "__" + method->name;

                // Convert parameters
                std::vector<Value> params;
                for (auto& par : method->params) {
                    // Structs and arrays are always passed by reference (pointer)
                    // Note: self parameter is already a pointer type from semantic analysis
                    const Type::Type* paramType = par.type;
                    if (paramType->kind == Type::TypeKind::Struct ||
                        paramType->kind == Type::TypeKind::Array) {
                        paramType = typeRegistry.getPointer(paramType);
                    }
                    params.push_back(Value::makeParam(par.name, paramType));
                }

                // Convert return type - structs and arrays are returned by reference (pointer)
                const Type::Type* returnType = method->returnType;
                if (returnType->kind == Type::TypeKind::Struct ||
                    returnType->kind == Type::TypeKind::Array) {
                    returnType = typeRegistry.getPointer(returnType);
                }

                builder.createFunction(mangledName, params, returnType);
                builder.createBlock("entry");
                varPointers.clear();

                // Set up parameters
                for (size_t i = 0; i < params.size(); i++) {
                    auto& param = params[i];
                    auto& paramDecl = method->params[i];
                    if (paramDecl.isMutRef) {
                        Value ptr = builder.createAlloca(param.type);
                        builder.createStore(param, ptr);
                        varPointers[param.name] = ptr;
                    } else {
                        builder.setVariable(param.name, param);
                    }
                }

                // Lower function body
                for (auto& stmt : method->body) {
                    lowerHIRStmt(*stmt);
                }

                // Add return if missing
                if (!builder.currentBlockTerminated()) {
                    if (method->returnType->kind == Type::TypeKind::Primitive) {
                        const auto* primType = static_cast<const Type::PrimitiveType*>(method->returnType);
                        if (primType->kind == Type::PrimitiveKind::Void) {
                            builder.createReturnVoid();
                        } else {
                            diag.error("Function must return a value", method->line, method->column);
                        }
                    } else {
                        diag.error("Function must return a value", method->line, method->column);
                    }
                }
                builder.finishFunction();
            }
        }
        // Imports don't need MIR lowering
    }
    return builder.getProgram();
}

void HIRToMIR::visitFnDecl(::FnDecl& fnDecl) {
    std::vector<Value> params;

    for (auto& par : fnDecl.params) {
        // Structs are always passed by reference (pointer)
        const Type::Type* paramType = par.type;
        if (paramType->kind == Type::TypeKind::Struct) {
            paramType = typeRegistry.getPointer(paramType);
        }
        params.push_back(Value::makeParam(par.name, paramType));
    }

    // Convert return type - structs are returned by reference (pointer)
    const Type::Type* returnType = fnDecl.returnType;
    if (returnType->kind == Type::TypeKind::Struct) {
        returnType = typeRegistry.getPointer(returnType);
    }

    builder.createFunction(fnDecl.name, params, returnType);
    builder.createBlock("entry");
    varPointers.clear();

    for (size_t i = 0; i < params.size(); i++) {
        auto& param = params[i];
        auto& paramDecl = fnDecl.params[i];

        if (paramDecl.isMutRef) {
            Value ptr = builder.createAlloca(param.type);
            builder.createStore(param, ptr);
            varPointers[param.name] = ptr;
        } else {
            builder.setVariable(param.name, param);
        }
    }

    for (auto& stmt : fnDecl.body) {
        lowerStmt(*stmt);
    }

    if (!builder.currentBlockTerminated()) {
        if (fnDecl.returnType->kind == Type::TypeKind::Primitive) {
            const auto* primType = static_cast<const Type::PrimitiveType*>(fnDecl.returnType);
            if (primType->kind == Type::PrimitiveKind::Void) {
                builder.createReturnVoid();
            } else {
                diag.error("Function must return a value", fnDecl.line, fnDecl.column);
            }
        } else {
            diag.error("Function must return a value", fnDecl.line, fnDecl.column);
        }
    }

    builder.finishFunction();
}

void HIRToMIR::visitStructDecl(::StructDecl& node) {
}

void HIRToMIR::visitVarDecl(::VarDecl& decl) {
    // Lower the initializer expression
    Value initValue = lowerExpr(*decl.initValue);

    // For structs and arrays, always use varPointers (even for immutable) to avoid auto-loading
    // Immutability is enforced at semantic analysis level
    if (decl.mutable_ ||
        decl.typeAnnotation->kind == Type::TypeKind::Struct ||
        decl.typeAnnotation->kind == Type::TypeKind::Array) {
        // For structs/arrays (heap pointers), allocate pointer-sized slot
        // For primitives, allocate value-sized slot
        const Type::Type* storageType = decl.typeAnnotation;
        if (decl.typeAnnotation->kind == Type::TypeKind::Struct ||
            decl.typeAnnotation->kind == Type::TypeKind::Array) {
            storageType = typeRegistry.getPointer(decl.typeAnnotation);
            // initValue is already a pointer from visitStructLiteral/visitArrayLiteral
        }

        Value ptr = builder.createAlloca(storageType);
        builder.createStore(initValue, ptr);
        varPointers[decl.name.lexeme] = ptr;
    } else {
        // Immutable primitive: track value directly using SSA
        builder.setVariable(decl.name.lexeme, initValue);
    }
}

void HIRToMIR::visitExprStmt(::ExprStmt& stmt) {
    // Lower the expression and discard result
    lowerExpr(*stmt.expr);
}

void HIRToMIR::visitReturnStmt(::ReturnStmt& stmt) {
    if (stmt.value) {
        Value retVal = lowerExpr(*stmt.value);

        // Structs and arrays are returned as pointers (by reference)

        builder.createReturn(retVal);
    } else {
        builder.createReturnVoid();
    }
}

void HIRToMIR::visitIfStmt(::IfStmt& stmt) {
    // Lower condition
    Value condVal = lowerExpr(*stmt.condition);

    // Generate labels
    std::string thenLabel = generateLabel("if.then");
    std::string elseLabel = stmt.elseBody.empty() ? "" : generateLabel("if.else");
    std::string mergeLabel = generateLabel("if.merge");

    // Emit conditional branch
    if (stmt.elseBody.empty()) {
        builder.createCondBranch(condVal, thenLabel, mergeLabel);
    } else {
        builder.createCondBranch(condVal, thenLabel, elseLabel);
    }

    // Then block
    builder.createBlock(thenLabel);
    for (auto& s : stmt.thenBody) {
        s->accept(*this);
    }
    if (!builder.currentBlockTerminated()) {
        builder.createBranch(mergeLabel);
    }

    // Else block (if exists)
    if (!stmt.elseBody.empty()) {
        builder.createBlock(elseLabel);
        for (auto& s : stmt.elseBody) {
            s->accept(*this);
        }
        if (!builder.currentBlockTerminated()) {
            builder.createBranch(mergeLabel);
        }
    }

    // Merge block
    builder.createBlock(mergeLabel);
}

void HIRToMIR::visitWhileStmt(::WhileStmt& stmt) {
    // Generate labels
    std::string condLabel = generateLabel("while.cond");
    std::string bodyLabel = generateLabel("while.body");
    std::string endLabel = generateLabel("while.end");

    // Branch to condition check
    builder.createBranch(condLabel);

    // Condition block
    builder.createBlock(condLabel);
    Value condVal = lowerExpr(*stmt.condition);
    builder.createCondBranch(condVal, bodyLabel, endLabel);

    // Push loop context for break/continue
    loopStack.push_back({endLabel, condLabel});

    // Body block
    builder.createBlock(bodyLabel);
    for (auto& s : stmt.thenBody) {
        s->accept(*this);
    }
    if (!builder.currentBlockTerminated()) {
        builder.createBranch(condLabel);  // Loop back
    }

    // Pop loop context
    loopStack.pop_back();

    // End block
    builder.createBlock(endLabel);
}

void HIRToMIR::visitForStmt(::ForStmt& stmt) {
    diag.error("For loops should be lowered to while loops in HIR", stmt.line, stmt.column);
}

void HIRToMIR::visitBlockStmt(::BlockStmt& stmt) {
    for (auto& s : stmt.statements) {
        s->accept(*this);
    }
}

void HIRToMIR::visitBreakStmt(::BreakStmt& stmt) {
    if (loopStack.empty()) {
        diag.error("break statement outside loop", stmt.line, stmt.column);
        return;
    }
    builder.createBranch(loopStack.back().breakLabel);
}

void HIRToMIR::visitContinueStmt(::ContinueStmt& stmt) {
    if (loopStack.empty()) {
        diag.error("continue statement outside loop", stmt.line, stmt.column);
        return;
    }
    builder.createBranch(loopStack.back().continueLabel);
}

void HIRToMIR::visitExternBlock(::ExternBlock& node) {
    // This is for visiting AST ExternBlock nodes from within statements
    // Top-level extern blocks are handled in the lower() method
    // This should not be called in normal flow
}

void HIRToMIR::visitImportStmt(::ImportStmt& node) {
    // Skip - imports are resolved in semantic analysis
}

// Lower HIR statements
void HIRToMIR::lowerHIRStmt(HIR::HIRStmt& stmt) {
    if (auto* hirRet = dynamic_cast<HIR::HIRReturnStmt*>(&stmt)) {
        if (hirRet->value) {
            Value retVal = lowerExpr(*hirRet->value);

            // Structs are returned as pointers (heap-allocated, GC-managed)
            // No need to load - just return the pointer

            builder.createReturn(retVal);
        } else {
            builder.createReturnVoid();
        }
        return;
    } else if (auto* hirWhile = dynamic_cast<HIR::HIRWhileStmt*>(&stmt)) {
        // Lower while loop
        std::string condLabel = generateLabel("while.cond");
        std::string bodyLabel = generateLabel("while.body");
        std::string endLabel = generateLabel("while.end");

        builder.createBranch(condLabel);

        builder.createBlock(condLabel);
        Value condVal = lowerExpr(*hirWhile->condition);
        builder.createCondBranch(condVal, bodyLabel, endLabel);

        loopStack.push_back({endLabel, condLabel});

        builder.createBlock(bodyLabel);
        for (auto& s : hirWhile->body) {
            lowerHIRStmt(*s);
        }

        if (hirWhile->increment) {
            lowerExpr(*hirWhile->increment);
        }

        if (!builder.currentBlockTerminated()) {
            builder.createBranch(condLabel);
        }

        loopStack.pop_back();

        builder.createBlock(endLabel);
        return;
    } else if (auto* hirIf = dynamic_cast<HIR::HIRIfStmt*>(&stmt)) {
        // Lower if statement
        std::string thenLabel = generateLabel("if.then");
        std::string elseLabel = generateLabel("if.else");
        std::string mergeLabel = generateLabel("if.merge");

        Value condVal = lowerExpr(*hirIf->condition);
        if (!hirIf->elseBody.empty()) {
            builder.createCondBranch(condVal, thenLabel, elseLabel);
        } else {
            builder.createCondBranch(condVal, thenLabel, mergeLabel);
        }

        builder.createBlock(thenLabel);
        for (auto& s : hirIf->thenBody) {
            lowerHIRStmt(*s);
        }
        bool thenTerminated = builder.currentBlockTerminated();
        if (!thenTerminated) {
            builder.createBranch(mergeLabel);
        }

        bool elseTerminated = false;
        if (!hirIf->elseBody.empty()) {
            builder.createBlock(elseLabel);
            for (auto& s : hirIf->elseBody) {
                lowerHIRStmt(*s);
            }
            elseTerminated = builder.currentBlockTerminated();
            if (!elseTerminated) {
                builder.createBranch(mergeLabel);
            }
        }

        // Only create merge block if at least one branch can reach it
        if (!thenTerminated || !elseTerminated) {
            builder.createBlock(mergeLabel);
        }
        return;
    } else if (auto* hirBlock = dynamic_cast<HIR::HIRBlockStmt*>(&stmt)) {
        for (auto& s : hirBlock->statements) {
            lowerHIRStmt(*s);
        }
        return;
    } else if (auto* hirBreak = dynamic_cast<HIR::HIRBreakStmt*>(&stmt)) {
        if (loopStack.empty()) {
            diag.error("break statement outside loop", hirBreak->line, hirBreak->column);
            return;
        }
        builder.createBranch(loopStack.back().breakLabel);
        return;
    } else if (auto* hirContinue = dynamic_cast<HIR::HIRContinueStmt*>(&stmt)) {
        if (loopStack.empty()) {
            diag.error("continue statement outside loop", hirContinue->line, hirContinue->column);
            return;
        }
        builder.createBranch(loopStack.back().continueLabel);
        return;
    } else if (auto* hirExpr = dynamic_cast<HIR::HIRExprStmt*>(&stmt)) {
        lowerExpr(*hirExpr->expr);
        return;
    } else if (auto* hirVarDecl = dynamic_cast<HIR::HIRVarDecl*>(&stmt)) {
        // Handle variable declaration
        Value initVal = lowerExpr(*hirVarDecl->initValue);

        // Use the declared type if available, otherwise infer from initializer
        const Type::Type* varType = hirVarDecl->typeAnnotation
            ? hirVarDecl->typeAnnotation
            : getExprType(hirVarDecl->initValue.get());

        // For structs, always use varPointers (even for immutable) to avoid auto-loading
        // Immutability is enforced at semantic analysis level
        if (hirVarDecl->mutable_ || varType->kind == Type::TypeKind::Struct) {
            // For structs (which are heap pointers), allocate a pointer-sized slot
            // For primitives, allocate value-sized slot
            const Type::Type* storageType = varType;
            if (varType->kind == Type::TypeKind::Struct) {
                // Struct variables store pointers to heap-allocated structs
                storageType = typeRegistry.getPointer(varType);
                // initVal is already a pointer from visitStructLiteral
            }

            Value ptr = builder.createAlloca(storageType);

            // Convert initVal to match storage type if needed
            if (initVal.type != storageType) {
                initVal = convertValue(initVal, storageType);
            }

            builder.createStore(initVal, ptr);
            varPointers[hirVarDecl->name.lexeme] = ptr;
        } else {
            // Immutable primitive: track value directly using SSA
            builder.setVariable(hirVarDecl->name.lexeme, initVal);
        }
        return;
    } else if (auto* hirFnDecl = dynamic_cast<HIR::HIRFnDecl*>(&stmt)) {
        // Function declarations should be handled at the top level
        // This shouldn't be called during statement lowering
        return;
    } else if (auto* hirExternBlock = dynamic_cast<HIR::HIRExternBlock*>(&stmt)) {
        // Extern blocks don't need lowering - they're handled elsewhere
        return;
    } else if (auto* hirImport = dynamic_cast<HIR::HIRImportStmt*>(&stmt)) {
        // Import statements don't need lowering - they're handled elsewhere
        return;
    }

    diag.error("Unknown HIR statement type", stmt.line, stmt.column);
}

// Lower AST statements (old compatibility)
void HIRToMIR::lowerStmt(Stmt& stmt) {
    // Fall back to regular visitor pattern for AST nodes
    stmt.accept(*this);
}


Value HIRToMIR::lowerExpr(Expr& expr) {
    // Dispatch based on expression type
    if (auto* lit = dynamic_cast<Literal*>(&expr)) {
        return visitLiteral(*lit);
    } else if (auto* var = dynamic_cast<Variable*>(&expr)) {
        return visitVariable(*var);
    } else if (auto* bin = dynamic_cast<BinaryExpr*>(&expr)) {
        return visitBinaryExpr(*bin);
    } else if (auto* un = dynamic_cast<UnaryExpr*>(&expr)) {
        return visitUnaryExpr(*un);
    } else if (auto* call = dynamic_cast<FnCall*>(&expr)) {
        return visitFnCall(*call);
    } else if (auto* assign = dynamic_cast<Assignment*>(&expr)) {
        return visitAssignment(*assign);
    } else if (auto* group = dynamic_cast<GroupingExpr*>(&expr)) {
        return visitGroupingExpr(*group);
    } else if (auto* arr = dynamic_cast<ArrayLiteral*>(&expr)) {
        return visitArrayLiteral(*arr);
    } else if (auto* idx = dynamic_cast<IndexExpr*>(&expr)) {
        return visitIndexExpr(*idx);
    } else if (auto* addr = dynamic_cast<AddrOf*>(&expr)) {
        return visitAddrOf(*addr);
    } else if (auto* compound = dynamic_cast<CompoundAssign*>(&expr)) {
        return visitCompoundAssign(*compound);
    } else if (auto* inc = dynamic_cast<Increment*>(&expr)) {
        return visitIncrement(*inc);
    } else if (auto* dec = dynamic_cast<Decrement*>(&expr)) {
        return visitDecrement(*dec);
    } else if (auto* range = dynamic_cast<Range*>(&expr)) {
        return visitRange(*range);
    } else if (auto* structLit = dynamic_cast<StructLiteral*>(&expr)) {
        return visitStructLiteral(*structLit);
    } else if (auto* fieldAccess = dynamic_cast<FieldAccess*>(&expr)) {
        return visitFieldAccess(*fieldAccess);
    } else if (auto* staticCall = dynamic_cast<StaticMethodCall*>(&expr)) {
        return visitStaticMethodCall(*staticCall);
    } else if (auto* instanceCall = dynamic_cast<InstanceMethodCall*>(&expr)) {
        return visitInstanceMethodCall(*instanceCall);
    }

    diag.error("Unknown expression type in HIR lowering", expr.line, expr.column);
    return Value();
}

Value HIRToMIR::visitLiteral(::Literal& lit) {
    const Type::Type* type = getExprType(&lit);

    switch (lit.token.tt) {
        case TokenType::Integer: {
            int64_t value = std::stoll(lit.token.lexeme);
            return builder.createConstantInt(value, type);
        }
        case TokenType::Float: {
            double value = std::stod(lit.token.lexeme);
            return builder.createConstantFloat(value, type);
        }
        case TokenType::True_:
            return builder.createConstantBool(true, type);
        case TokenType::False_:
            return builder.createConstantBool(false, type);
        case TokenType::String:
            return builder.createConstantString(lit.token.lexeme, type);
        case TokenType::Null:
            return builder.createConstantNull(type);
        default:
            diag.error("Unknown literal type", lit.line, lit.column);
            return Value();
    }
}

Value HIRToMIR::visitVariable(::Variable& var) {
    // Check if variable is mutable (has pointer in varPointers)
    if (varPointers.count(var.token.lexeme)) {
        Value ptr = varPointers[var.token.lexeme];

        // Get the variable's semantic type to check if we should load
        const Type::Type* varType = getExprType(&var);

        // For structs: load the heap pointer from the stack slot
        // (varPointers stores ptr-to-ptr for heap-allocated structs)
        if (varType->kind == Type::TypeKind::Struct) {
            return builder.createLoad(ptr);  // Load the struct pointer
        }

        // For pointers to structs: also need to load (e.g., mut self parameter)
        // varPointers stores ptr-to-ptr, we need to load to get the single ptr
        if (varType->kind == Type::TypeKind::Pointer) {
            const auto* ptrType = static_cast<const Type::PointerType*>(varType);
            if (ptrType->pointeeType->kind == Type::TypeKind::Struct) {
                return builder.createLoad(ptr);  // Load the struct pointer
            }
            // For other pointers (like ptr<i32>), don't load
            return ptr;
        }

        // For arrays, don't load - they are manipulated by reference
        if (varType->kind == Type::TypeKind::Array) {
            return ptr;  // Return the pointer, not the loaded value
        }

        return builder.createLoad(ptr);
    }

    // Otherwise, get immutable value
    return builder.getVariable(var.token.lexeme);
}

Value HIRToMIR::visitBinaryExpr(::BinaryExpr& expr) {
    // Lower operands
    Value lhs = lowerExpr(*expr.lhs);
    Value rhs = lowerExpr(*expr.rhs);

    // Check type of operands
    const Type::Type* lhsType = getExprType(expr.lhs.get());

    bool isFloat = false;
    bool isUnsigned = false;

    if (lhsType->kind == Type::TypeKind::Primitive) {
        const auto* primType = static_cast<const Type::PrimitiveType*>(lhsType);
        isFloat = (primType->kind == Type::PrimitiveKind::F32 ||
                   primType->kind == Type::PrimitiveKind::F64);
        isUnsigned = primType->isUnsigned();
    }

    // Map operator to MIR instruction
    switch (expr.op) {
        case TokenType::Plus:
            return isFloat ? builder.createFAdd(lhs, rhs) : builder.createIAdd(lhs, rhs);
        case TokenType::Minus:
            return isFloat ? builder.createFSub(lhs, rhs) : builder.createISub(lhs, rhs);
        case TokenType::Mult:
            return isFloat ? builder.createFMul(lhs, rhs) : builder.createIMul(lhs, rhs);

        case TokenType::Div:
            if (isFloat) return builder.createFDiv(lhs, rhs);
            return isUnsigned ? builder.createUDiv(lhs, rhs) : builder.createIDiv(lhs, rhs);

        case TokenType::Modulo:
            return isUnsigned ? builder.createURem(lhs, rhs) : builder.createIRem(lhs, rhs);

        // Comparisons
        case TokenType::EqualEqual:
            return isFloat ? builder.createFCmpEQ(lhs, rhs) : builder.createICmpEQ(lhs, rhs);
        case TokenType::NotEqual:
            return isFloat ? builder.createFCmpNE(lhs, rhs) : builder.createICmpNE(lhs, rhs);

        case TokenType::LessThan:
            if (isFloat) return builder.createFCmpLT(lhs, rhs);
            return isUnsigned ? builder.createICmpULT(lhs, rhs) : builder.createICmpLT(lhs, rhs);

        case TokenType::LessEqual:
            if (isFloat) return builder.createFCmpLE(lhs, rhs);
            return isUnsigned ? builder.createICmpULE(lhs, rhs) : builder.createICmpLE(lhs, rhs);

        case TokenType::GreaterThan:
            if (isFloat) return builder.createFCmpGT(lhs, rhs);
            return isUnsigned ? builder.createICmpUGT(lhs, rhs) : builder.createICmpGT(lhs, rhs);

        case TokenType::GreaterEqual:
            if (isFloat) return builder.createFCmpGE(lhs, rhs);
            return isUnsigned ? builder.createICmpUGE(lhs, rhs) : builder.createICmpGE(lhs, rhs);

        // Logical operators
        case TokenType::And:
            return builder.createAnd(lhs, rhs);
        case TokenType::Or:
            return builder.createOr(lhs, rhs);

        default:
            diag.error("Unknown binary operator", expr.line, expr.column);
            return Value();
    }
}

Value HIRToMIR::visitUnaryExpr(::UnaryExpr& expr) {
    Value operand = lowerExpr(*expr.operand);

    switch (expr.op) {
        case TokenType::Not:
            return builder.createNot(operand);
        case TokenType::Minus: {
            // Negate: 0 - operand
            const Type::Type* operandType = getExprType(expr.operand.get());
            bool isFloat = operandType->kind == Type::TypeKind::Primitive &&
                           (static_cast<const Type::PrimitiveType*>(operandType)->kind == Type::PrimitiveKind::F32 ||
                            static_cast<const Type::PrimitiveType*>(operandType)->kind == Type::PrimitiveKind::F64);

            if (isFloat) {
                Value zero = builder.createConstantFloat(0.0, operandType);
                return builder.createISub(zero, operand);  // TODO: use FSub
            } else {
                Value zero = builder.createConstantInt(0, operandType);
                return builder.createISub(zero, operand);
            }
        }
        default:
            diag.error("Unknown unary operator", expr.line, expr.column);
            return Value();
    }
}

Value HIRToMIR::visitFnCall(::FnCall& call) {
    // Try to look up the function to get parameter types
    Function* func = builder.getProgram().getFunction(call.name);

    // Lower all arguments
    std::vector<Value> args;
    for (size_t i = 0; i < call.args.size(); i++) {
        Value arg = lowerExpr(*call.args[i]);

        // If we found the function, convert arguments to match parameter types
        if (func && i < func->params.size()) {
            const Type::Type* paramType = func->params[i].type;

            // Structs and arrays are passed as pointers
            // - Structs: already correct from visitStructLiteral (returns heap pointer)
            // - Arrays: already correct from visitArrayLiteral (returns stack pointer)
            // - Array/struct variables: already pointers from visitVariable
            // Just convert primitive types if needed
            if (arg.type != paramType) {
                arg = convertValue(arg, paramType);
            }
        }

        args.push_back(arg);
    }

    // Get return type from type map
    const Type::Type* returnType = getExprType(&call);

    // If the return type is a struct or array, it's returned as a pointer
    if (returnType && (returnType->kind == Type::TypeKind::Struct ||
                       returnType->kind == Type::TypeKind::Array)) {
        returnType = typeRegistry.getPointer(returnType);
    }

    // Emit call
    return builder.createCall(call.name, args, returnType);
}

Value HIRToMIR::visitAssignment(::Assignment& assign) {
    // Lower the right-hand side
    Value value = lowerExpr(*assign.value);

    // Check if LHS is a variable
    if (auto* varExpr = dynamic_cast<Variable*>(assign.lhs.get())) {
        std::string varName = varExpr->token.lexeme;

        // Get the pointer to assign to (must be mutable)
        if (!varPointers.count(varName)) {
            diag.error("Cannot assign to immutable variable: " + varName, assign.line, assign.column);
            return value;
        }

        Value ptr = varPointers[varName];
        builder.createStore(value, ptr);

        return value;
    }
    // Check if LHS is an array index expression
    else if (auto* indexExpr = dynamic_cast<IndexExpr*>(assign.lhs.get())) {
        // Get pointer to the array element (without loading)
        Value elementPtr = getIndexExprPtr(*indexExpr);

        // Store the value
        builder.createStore(value, elementPtr);

        return value;
    }
    // Check if LHS is a struct field access
    else if (auto* fieldAccess = dynamic_cast<FieldAccess*>(assign.lhs.get())) {
        // Get pointer to the field (without loading)
        Value fieldPtr = getFieldAccessPtr(*fieldAccess);

        // Get the field type from the pointer
        const auto* ptrType = static_cast<const Type::PointerType*>(fieldPtr.type);
        const Type::Type* fieldType = ptrType->pointeeType;

        // Convert value to match field type if needed
        if (value.type != fieldType) {
            value = convertValue(value, fieldType);
        }

        // Store the value
        builder.createStore(value, fieldPtr);

        return value;
    }
    else {
        diag.error("Assignment target must be a variable, array element, or struct field", assign.line, assign.column);
        return value;
    }
}

Value HIRToMIR::visitGroupingExpr(::GroupingExpr& expr) {
    // Just forward to the inner expression
    return lowerExpr(*expr.expr);
}

Value HIRToMIR::visitArrayLiteral(::ArrayLiteral& lit) {
    // Get array type
    const Type::Type* arrayType = getExprType(&lit);

    // Allocate space on the GC heap for the array (like structs)
    size_t arrayByteSize = getTypeSize(arrayType);
    const Type::Type* i64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::I64);
    Value sizeVal = builder.createConstantInt(static_cast<int64_t>(arrayByteSize), i64Type);

    // Call volta_gc_malloc(size)
    const Type::Type* voidPtrType = typeRegistry.getPointer(typeRegistry.getPrimitive(Type::PrimitiveKind::Void));
    Value heapPtr = builder.createCall("volta_gc_malloc", {sizeVal}, voidPtrType);

    // Cast void* to typed array pointer
    Value arrayPtr = heapPtr;  // MIR doesn't need explicit casting, types are tracked separately
    arrayPtr.type = typeRegistry.getPointer(arrayType);

    // Store each element
    for (size_t i = 0; i < lit.elements.size(); i++) {
        Value elemValue = lowerExpr(*lit.elements[i]);

        // For structs (heap pointers), store the pointers directly
        // No need to load - arrays of structs are arrays of pointers

        Value indexVal = builder.createConstantInt(static_cast<int64_t>(i),
                                                     typeRegistry.getPrimitive(Type::PrimitiveKind::I64));
        Value elemPtr = builder.createGetElementPtr(arrayPtr, indexVal);
        builder.createStore(elemValue, elemPtr);
    }

    return arrayPtr;
}

Value HIRToMIR::visitIndexExpr(::IndexExpr& expr) {
    // Lower the array expression and index
    Value arrayVal = lowerExpr(*expr.array);
    Value indexVal = lowerExpr(*expr.index);

    // Get element pointer and load
    Value elemPtr = builder.createGetElementPtr(arrayVal, indexVal);
    return builder.createLoad(elemPtr);
}

// Helper: Get pointer to array element without loading
Value HIRToMIR::getIndexExprPtr(::IndexExpr& expr) {
    Value arrayVal = lowerExpr(*expr.array);
    Value indexVal = lowerExpr(*expr.index);
    return builder.createGetElementPtr(arrayVal, indexVal);
}

Value HIRToMIR::getFieldAccessPtr(::FieldAccess& expr) {
    Value structPtr = lowerExpr(*expr.object);
    int fieldIndex = expr.fieldIndex;
    return builder.createGetFieldPtr(structPtr, fieldIndex);
}


Value HIRToMIR::visitAddrOf(::AddrOf& node) {
    // Get the pointer to a variable
    if (auto* var = dynamic_cast<Variable*>(node.operand.get())) {
        if (varPointers.count(var->token.lexeme)) {
            // Variable is mutable, return its pointer
            return varPointers[var->token.lexeme];
        } else {
            diag.error("Cannot take address of immutable variable", node.line, node.column);
            return Value();
        }
    }

    diag.error("Address-of operator only supported for variables", node.line, node.column);
    return Value();
}

Value HIRToMIR::visitCompoundAssign(::CompoundAssign& node) {
    diag.error("Compound assignments should be lowered in HIR", node.line, node.column);
    return Value();
}

Value HIRToMIR::visitIncrement(::Increment& node) {
    diag.error("Increment should be lowered in HIR", node.line, node.column);
    return Value();
}

Value HIRToMIR::visitDecrement(::Decrement& node) {
    diag.error("Decrement should be lowered in HIR", node.line, node.column);
    return Value();
}

Value HIRToMIR::visitRange(::Range& node) {
    diag.error("Ranges should be lowered in HIR", node.line, node.column);
    return Value();
}

Value HIRToMIR::visitStructLiteral(::StructLiteral& node) {
    // Get the struct type
    const Type::Type* structType = getExprType(&node);

    // Allocate space on the GC heap for the struct
    size_t structSize = getTypeSize(structType);
    const Type::Type* i64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::I64);
    Value sizeVal = builder.createConstantInt(static_cast<int64_t>(structSize), i64Type);

    // Call volta_gc_malloc(size)
    const Type::Type* voidPtrType = typeRegistry.getPointer(typeRegistry.getPrimitive(Type::PrimitiveKind::Void));
    Value heapPtr = builder.createCall("volta_gc_malloc", {sizeVal}, voidPtrType);

    // Cast void* to typed struct pointer
    Value structPtr = heapPtr;  // MIR doesn't need explicit casting, types are tracked separately
    structPtr.type = typeRegistry.getPointer(structType);

    // Initialize each field
    for (size_t i = 0; i < node.fields.size(); i++) {
        auto& [fieldName, fieldExpr] = node.fields[i];

        // Lower the field value expression
        Value fieldValue = lowerExpr(*fieldExpr);

        // Get pointer to this field
        Value fieldPtr = builder.createGetFieldPtr(structPtr, static_cast<int>(i));

        // Store the value
        builder.createStore(fieldValue, fieldPtr);
    }

    // Return the pointer to the struct
    return structPtr;
}

Value HIRToMIR::visitFieldAccess(::FieldAccess& node) {
    Value structVal = lowerExpr(*node.object);
    int fieldIndex = node.fieldIndex;

    // Check if structVal is a pointer or a value
    Value structPtr;
    if (structVal.type->kind == Type::TypeKind::Pointer) {
        // Already a pointer, use it directly
        structPtr = structVal;
    } else {
        // It's a struct value, we need to allocate it to memory first
        structPtr = builder.createAlloca(structVal.type);
        builder.createStore(structVal, structPtr);
    }

    Value fieldPtr = builder.createGetFieldPtr(structPtr, fieldIndex);
    return builder.createLoad(fieldPtr);
}

Value HIRToMIR::visitStaticMethodCall(::StaticMethodCall& node) {
    // Static method calls are converted to regular function calls with mangled names
    // The mangled name format is: StructName__methodName
    std::string mangledName = node.typeName.lexeme + "__" + node.methodName.lexeme;

    // Lower the arguments
    std::vector<Value> args;
    for (auto& arg : node.args) {
        args.push_back(lowerExpr(*arg));
    }

    // Get the return type from the type map
    const Type::Type* returnType = getExprType(&node);

    // Create the function call
    return builder.createCall(mangledName, args, returnType);
}

Value HIRToMIR::visitInstanceMethodCall(::InstanceMethodCall& node) {
    // Instance method calls are converted to regular function calls with:
    // 1. Mangled name: StructName__methodName
    // 2. First argument is the object (self parameter)

    // Get the object's type to determine the struct name
    const Type::Type* objectType = getExprType(node.object.get());
    std::string structName;

    if (objectType->kind == Type::TypeKind::Struct) {
        const auto* structType = static_cast<const Type::StructType*>(objectType);
        structName = structType->name;
    } else if (objectType->kind == Type::TypeKind::Pointer) {
        const auto* ptrType = static_cast<const Type::PointerType*>(objectType);
        if (ptrType->pointeeType->kind == Type::TypeKind::Struct) {
            const auto* structType = static_cast<const Type::StructType*>(ptrType->pointeeType);
            structName = structType->name;
        } else {
            diag.error("Instance method call on non-struct type", node.line, node.column);
            return Value();
        }
    } else {
        // This should not happen if semantic analysis succeeded
        diag.error("Instance method call on non-struct type", node.line, node.column);
        return Value();
    }

    // Mangle the function name
    std::string mangledName = structName + "__" + node.methodName.lexeme;

    // Lower the object expression and get a pointer to it
    Value objectVal = lowerExpr(*node.object);

    // Ensure we have a pointer to the struct
    Value objectPtr;
    if (objectVal.type->kind == Type::TypeKind::Pointer) {
        objectPtr = objectVal;
    } else {
        // Need to create a temporary allocation
        objectPtr = builder.createAlloca(objectVal.type);
        builder.createStore(objectVal, objectPtr);
    }

    // Build argument list: self is first, then the explicit arguments
    std::vector<Value> args;
    args.push_back(objectPtr);

    for (auto& arg : node.args) {
        args.push_back(lowerExpr(*arg));
    }

    // Get the return type from the type map
    const Type::Type* returnType = getExprType(&node);

    // Create the function call
    return builder.createCall(mangledName, args, returnType);
}


const Type::Type* HIRToMIR::getExprType(const Expr* expr) const {
    auto it = exprTypes.find(expr);
    if (it == exprTypes.end()) {
        // Type not found - this shouldn't happen if semantic analysis completed
        // Return a default type to avoid crashes
        return typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
    }
    return it->second;
}

const Type::Type* HIRToMIR::getMIRType(const Type::Type* hirType) {
    return hirType;
}

std::string HIRToMIR::generateLabel(const std::string& hint) {
    static int counter = 0;
    std::stringstream ss;
    ss << hint << "." << counter++;
    return ss.str();
}

Value HIRToMIR::convertValue(const Value& value, const Type::Type* targetType) {
    // If types match, no conversion needed
    if (value.type == targetType) {
        return value;
    }

    // Both must be primitives for numeric conversion
    if (!value.type || !targetType ||
        value.type->kind != Type::TypeKind::Primitive ||
        targetType->kind != Type::TypeKind::Primitive) {
        return value;  // Can't convert, return as-is
    }

    const auto* srcPrim = static_cast<const Type::PrimitiveType*>(value.type);
    const auto* tgtPrim = static_cast<const Type::PrimitiveType*>(targetType);

    // Helper lambdas for type classification
    auto isFloat = [](const Type::PrimitiveType* p) {
        return p->kind == Type::PrimitiveKind::F32 || p->kind == Type::PrimitiveKind::F64;
    };

    auto getBitWidth = [](const Type::PrimitiveType* p) {
        switch (p->kind) {
            case Type::PrimitiveKind::I8:
            case Type::PrimitiveKind::U8:
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
    };

    // Integer to integer conversion
    if (srcPrim->isInteger() && tgtPrim->isInteger()) {
        int srcBits = getBitWidth(srcPrim);
        int tgtBits = getBitWidth(tgtPrim);

        if (srcBits < tgtBits) {
            Opcode op = srcPrim->isSigned() ? Opcode::SExt : Opcode::ZExt;
            return builder.createConversion(op, value, targetType);
        } else if (srcBits > tgtBits) {
            return builder.createConversion(Opcode::Trunc, value, targetType);
        }
        // Same bit width but different signedness - use bitcast
        return builder.createConversion(Opcode::Bitcast, value, targetType);
    }

    // Float to float conversion
    if (isFloat(srcPrim) && isFloat(tgtPrim)) {
        int srcBits = getBitWidth(srcPrim);
        int tgtBits = getBitWidth(tgtPrim);

        if (srcBits < tgtBits) {
            return builder.createConversion(Opcode::FPExt, value, targetType);
        } else if (srcBits > tgtBits) {
            return builder.createConversion(Opcode::FPTrunc, value, targetType);
        }
        return value;
    }

    // Integer to float
    if (srcPrim->isInteger() && isFloat(tgtPrim)) {
        Opcode op = srcPrim->isSigned() ? Opcode::SIToFP : Opcode::UIToFP;
        return builder.createConversion(op, value, targetType);
    }

    // Float to integer
    if (isFloat(srcPrim) && tgtPrim->isInteger()) {
        Opcode op = tgtPrim->isSigned() ? Opcode::FPToSI : Opcode::FPToUI;
        return builder.createConversion(op, value, targetType);
    }

    return value;
}

size_t HIRToMIR::getTypeSize(const Type::Type* type) {
    if (!type) return 0;

    switch (type->kind) {
        case Type::TypeKind::Primitive: {
            const auto* primType = static_cast<const Type::PrimitiveType*>(type);
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
            const auto* structType = static_cast<const Type::StructType*>(type);
            size_t total = 0;
            for (const auto& field : structType->fields) {
                // TODO: Add proper alignment/padding calculation
                total += getTypeSize(field.type);
            }
            return total;
        }

        case Type::TypeKind::Array: {
            const auto* arrayType = static_cast<const Type::ArrayType*>(type);
            return getTypeSize(arrayType->elementType) * arrayType->size;
        }

        case Type::TypeKind::Generic:
        case Type::TypeKind::Opaque:
        case Type::TypeKind::Unresolved:
            return 0;  // Unknown size
    }

    return 0;
}


} // namespace MIR
