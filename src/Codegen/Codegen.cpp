#include "Codegen/Codegen.hpp"
#include <llvm/IR/DerivedTypes.h>

llvm::Module* Codegen::generate() {
    context = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>("volta_module", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    for (const auto& stmt : hirProgram.statements) {
        generateStmt(stmt.get());
    }

    return module_.get();
}

void Codegen::generateStmt(const Stmt* stmt) {

    if (auto* fnDecl = dynamic_cast<const FnDecl*>(stmt)) {
        generateFnDecl(fnDecl);
    } else if (auto* varDecl = dynamic_cast<const VarDecl*>(stmt)) {
        generateVarDecl(varDecl);
    } else if (auto* hirReturnStmt = dynamic_cast<const HIR::HIRReturnStmt*>(stmt)) {
        generateHIRReturn(hirReturnStmt);
    } else if (auto* hirIfStmt = dynamic_cast<const HIR::HIRIfStmt*>(stmt)) {
        generateHIRIfStmt(hirIfStmt);
    } else if (auto* hirWhileStmt = dynamic_cast<const HIR::HIRWhileStmt*>(stmt)) {
        generateHIRWhileStmt(hirWhileStmt);
    } else if (auto* hirBlockStmt = dynamic_cast<const HIR::HIRBlockStmt*>(stmt)) {
        generateHIRBlockStmt(hirBlockStmt);
    } else if (auto* hirBreakStmt = dynamic_cast<const HIR::HIRBreakStmt*>(stmt)) {
        generateHIRBreak(hirBreakStmt);
    } else if (auto* hirContinueStmt = dynamic_cast<const HIR::HIRContinueStmt*>(stmt)) {
        generateHIRContinue(hirContinueStmt);
    } else if (auto* hirExprStmt = dynamic_cast<const HIR::HIRExprStmt*>(stmt)) {
        generateHIRExprStmt(hirExprStmt);
    } else {
        diag.error("Unknown statement type in codegen", 0, 0);
    }
}

void Codegen::generateFnDecl(const FnDecl* stmt) {
    functionDecls[stmt->name] = stmt;

    llvm::Type* retType = typeRegistry.toLLVMType(stmt->returnType.get(), *context);

    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : stmt->params) {
        if (param.isRef || param.isMutRef) {
            paramTypes.push_back(llvm::PointerType::get(*context, 0));
        } else {
            llvm::Type* baseType = typeRegistry.toLLVMType(param.type.get(), *context);
            paramTypes.push_back(baseType);
        }
    }

    llvm::FunctionType* fnType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function* func = llvm::Function::Create(
        fnType,
        llvm::Function::ExternalLinkage,
        stmt->name,
        *module_
    );

    llvm::BasicBlock* block = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(block);

    currentFunctionReturnType = stmt->returnType.get();

    for (size_t i = 0; i < stmt->params.size(); ++i) {
        const Param& param = stmt->params[i];
        func->getArg(i)->setName(param.name);

        llvm::Type* paramType = typeRegistry.toLLVMType(param.type.get(), *context);

        if (param.isRef || param.isMutRef) {
            variables[param.name] = {func->getArg(i), paramType};
        } else {
            llvm::AllocaInst* paramPtr = builder->CreateAlloca(paramType, nullptr, param.name);
            builder->CreateStore(func->getArg(i), paramPtr);
            variables[param.name] = {paramPtr, paramType};
        }
    }

    for (const auto& bodyStmt : stmt->body) {
        generateStmt(bodyStmt.get());
    }

    variables.clear();
    currentFunctionReturnType = nullptr;
}


void Codegen::generateHIRWhileStmt(const HIR::HIRWhileStmt* stmt) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "while.cond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "while.body", func);
    llvm::BasicBlock* incBB = nullptr;  
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context, "while.end", func);

    if (stmt->increment) {
        incBB = llvm::BasicBlock::Create(*context, "while.inc", func);
    }

    builder->CreateBr(condBB);

    builder->SetInsertPoint(condBB);
    llvm::Value* cond = generateExpr(stmt->condition.get());
    if (!cond) {
        return;
    }
    builder->CreateCondBr(cond, bodyBB, endBB);

    loopStack.push_back({
        .continueTarget = incBB ? incBB : condBB,
        .breakTarget = endBB
    });

    builder->SetInsertPoint(bodyBB);
    for (const auto& bodyStmt : stmt->thenBody) {
        generateStmt(bodyStmt.get());
    }

    if (!builder->GetInsertBlock()->getTerminator()) {
        if (incBB) {
            builder->CreateBr(incBB);
        } else {
            builder->CreateBr(condBB);
        }
    }

    if (incBB) {
        builder->SetInsertPoint(incBB);
        generateExpr(stmt->increment.get());
        builder->CreateBr(condBB);  
    }

    loopStack.pop_back();

    builder->SetInsertPoint(endBB);
}


void Codegen::generateHIRReturn(const HIR::HIRReturnStmt* stmt) {
    if (stmt->value) {
        auto val = generateExpr(stmt->value.get(), currentFunctionReturnType);
        builder->CreateRet(val);
    } else {
        builder->CreateRetVoid();
    }
}

void Codegen::generateHIRIfStmt(const HIR::HIRIfStmt* stmt) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "if.then", func);
    llvm::BasicBlock* elseBB = stmt->elseBody.empty() ? nullptr : llvm::BasicBlock::Create(*context, "if.else", func);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context, "if.end", func);

    llvm::Value* cond = generateExpr(stmt->condition.get());
    if (!cond) {
        return;
    }

    if (elseBB) {
        builder->CreateCondBr(cond, thenBB, elseBB);
    } else {
        builder->CreateCondBr(cond, thenBB, mergeBB);
    }

    builder->SetInsertPoint(thenBB);
    for (const auto& thenStmt : stmt->thenBody) {
        generateStmt(thenStmt.get());
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }

    if (elseBB) {
        builder->SetInsertPoint(elseBB);
        for (const auto& elseStmt : stmt->elseBody) {
            generateStmt(elseStmt.get());
        }
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }
    }

    if (mergeBB->hasNPredecessors(0)) {
        mergeBB->eraseFromParent();
    } else {
        builder->SetInsertPoint(mergeBB);
    }
}

void Codegen::generateHIRBlockStmt(const HIR::HIRBlockStmt* stmt) {
    for (const auto& blockStmt : stmt->statements) {
        generateStmt(blockStmt.get());
    }
}

void Codegen::generateHIRBreak(const HIR::HIRBreakStmt* stmt) {
    (void)stmt;  
    if (loopStack.empty()) {
        diag.error("'break' statement outside of loop", 0, 0);
        return;
    }
    builder->CreateBr(loopStack.back().breakTarget);
}

void Codegen::generateHIRContinue(const HIR::HIRContinueStmt* stmt) {
    (void)stmt;  
    if (loopStack.empty()) {
        diag.error("'continue' statement outside of loop", 0, 0);
        return;
    }
    builder->CreateBr(loopStack.back().continueTarget);
}

void Codegen::generateHIRExprStmt(const HIR::HIRExprStmt* stmt) {
    generateExpr(stmt->expr.get());
}

void Codegen::generateVarDecl(const VarDecl* stmt) {
    std::string name = stmt->name.lexeme;
    Type* expectedType = stmt->type_annotation ? stmt->type_annotation.get() : nullptr;
    llvm::Type* type;

    if (stmt->init_value) {
        if (auto* arrayLit = dynamic_cast<const ArrayLiteral*>(stmt->init_value.get())) {
            type = typeRegistry.toLLVMType(expectedType, *context);
            llvm::AllocaInst* alloca = builder->CreateAlloca(type, nullptr, name);
            fillArrayLiteral(alloca, type, arrayLit);

            variables[name] = {alloca, type};
        } else {
            auto initValue = generateExpr(stmt->init_value.get(), expectedType);

            if (expectedType) {
                type = typeRegistry.toLLVMType(expectedType, *context);
            } else {
                type = initValue->getType();
            }

            llvm::AllocaInst* alloca = builder->CreateAlloca(type, nullptr, name);
            builder->CreateStore(initValue, alloca);
            variables[name] = {alloca, type};
        }
    } else {
        type = typeRegistry.toLLVMType(expectedType, *context);
        llvm::AllocaInst* alloca = builder->CreateAlloca(type, nullptr, name);
        variables[name] = {alloca, type};
    }
}


llvm::Value* Codegen::generateExpr(const Expr* expr, const Type* expectedType) {
    if (auto* literal = dynamic_cast<const Literal*>(expr)) {
        return generateLiteral(literal, expectedType);
    } else if (auto* variable = dynamic_cast<const Variable*>(expr)) {
        return generateVariable(variable);
    } else if (auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        return generateFnCall(fnCall);
    } else if (auto* binaryExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        return generateBinaryExpr(binaryExpr, expectedType);
    } else if (auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        return generateUnaryExpr(unaryExpr, expectedType);
    } else if (auto* assignment = dynamic_cast<const Assignment*>(expr)) {
        return generateAssignment(assignment);
    } else if (auto* arrayLit = dynamic_cast<const ArrayLiteral*>(expr)) {
        return generateArrayLiteral(arrayLit);
    } else if (auto* indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        return generateIndexExpr(indexExpr);
    } else if (auto* grouping = dynamic_cast<const GroupingExpr*>(expr)) {
        return generateExpr(grouping->expr.get(), expectedType);
    } else {
        diag.error("Unknown expression type in codegen", 0, 0);
        return nullptr;
    }
}

llvm::Value* Codegen::generateLiteral(const Literal* expr, const Type* expectedType) {
    std::string lexeme = expr->token.lexeme;
    TokenType tt = expr->token.tt;

    if (tt == TokenType::Integer) {
        // TODO: Add support for full i128 casting.
        int64_t value = std::stoll(expr->token.lexeme);
        llvm::Type* llvmType;
        if (expectedType) {
            llvmType = typeRegistry.toLLVMType(expectedType, *context);
        } else {
            llvmType = llvm::Type::getInt32Ty(*context);
        }
        return llvm::ConstantInt::get(llvmType, value);
    }
    else if (tt == TokenType::Float) {
        double value = std::stod(expr->token.lexeme);
        llvm::Type* llvmType;
        if (expectedType) {
            llvmType = typeRegistry.toLLVMType(expectedType, *context);
        } else {
            llvmType = llvm::Type::getFloatTy(*context);
        }
        return llvm::ConstantFP::get(llvmType, value);
    }
    else if (tt == TokenType::True_) {
        llvm::Type* llvmType =  llvm::Type::getInt1Ty(*context);
        return llvm::ConstantInt::get(llvmType, 1);
    } else if (tt == TokenType::False_) {
        llvm::Type* llvmType =  llvm::Type::getInt1Ty(*context);
        return llvm::ConstantInt::get(llvmType, 0);
    } else {
        diag.error("Unsupported type for code gen.", -1, -1);
        return nullptr;
    }
}

llvm::Value* Codegen::generateVariable(const Variable* expr) {
    std::string name = expr->token.lexeme;

    auto it = variables.find(name);
    if (it == variables.end()) {
        diag.error("Undefined variable: " + name, expr->token.line, expr->token.column);
        return nullptr;
    }

    llvm::Value* ptr = it->second.first;
    llvm::Type* elemType = it->second.second;
    return builder->CreateLoad(elemType, ptr, name);
}

llvm::Value* Codegen::generateFnCall(const FnCall* expr) {
    llvm::Function* fn = module_->getFunction(expr->name);

    if (!fn) {
        diag.error("Function '" + expr->name + "' not found", 0, 0);
        return nullptr;
    }

    auto declIt = functionDecls.find(expr->name);
    if (declIt == functionDecls.end()) {
        diag.error("Function declaration for '" + expr->name + "' not found", 0, 0);
        return nullptr;
    }
    const FnDecl* fnDecl = declIt->second;
    
    std::vector<llvm::Value*> args;
    for (size_t i = 0; i < expr->args.size(); ++i) {
        const Param& param = fnDecl->params[i];
        
        if (param.isRef || param.isMutRef) {
            if (auto* varExpr = dynamic_cast<const Variable*>(expr->args[i].get())) {
                std::string varName = varExpr->token.lexeme;
                auto varIt = variables.find(varName);
                if (varIt == variables.end()) {
                    diag.error("Variable '" + varName + "' not found for reference argument", 0, 0);
                    return nullptr;
                }
                args.push_back(varIt->second.first);
            } else {
                diag.error("Non-variable passed to reference parameter", 0, 0);
                return nullptr;
            }
        } else {
            llvm::Value* argVal = generateExpr(expr->args[i].get());
            if (!argVal) {
                return nullptr;
            }
            args.push_back(argVal);
        }
    }
    
    return builder->CreateCall(fn, args, expr->name + "_result");
}

llvm::Value* Codegen::generateBinaryExpr(const BinaryExpr* expr, const Type* expectedType) {
 llvm::Value* lhs = generateExpr(expr->lhs.get());
    llvm::Value* rhs = generateExpr(expr->rhs.get());
    
    if (!lhs || !rhs) {
        return nullptr;
    }
    
    llvm::Type* type = lhs->getType();
    bool isFloat = type->isFloatingPointTy();
    bool isInt = type->isIntegerTy();
    
    switch (expr->op) {
        case TokenType::Plus:
            return isFloat ? builder->CreateFAdd(lhs, rhs, "fadd") 
                          : builder->CreateAdd(lhs, rhs, "add");
        
        case TokenType::Minus:
            return isFloat ? builder->CreateFSub(lhs, rhs, "fsub")
                          : builder->CreateSub(lhs, rhs, "sub");
        
        case TokenType::Mult:
            return isFloat ? builder->CreateFMul(lhs, rhs, "fmul")
                          : builder->CreateMul(lhs, rhs, "mul");
        
        case TokenType::Div:
            return isFloat ? builder->CreateFDiv(lhs, rhs, "fdiv")
                          : builder->CreateSDiv(lhs, rhs, "sdiv");  
        
        case TokenType::Modulo:
            if (isFloat) {
                return builder->CreateFRem(lhs, rhs, "frem");
            } else {
                return builder->CreateSRem(lhs, rhs, "srem");  
            }
        
        case TokenType::EqualEqual:
            return isFloat ? builder->CreateFCmpOEQ(lhs, rhs, "feq")
                          : builder->CreateICmpEQ(lhs, rhs, "eq");
        
        case TokenType::NotEqual:
            return isFloat ? builder->CreateFCmpONE(lhs, rhs, "fne")
                          : builder->CreateICmpNE(lhs, rhs, "ne");
        
        case TokenType::LessThan:
            return isFloat ? builder->CreateFCmpOLT(lhs, rhs, "flt")
                          : builder->CreateICmpSLT(lhs, rhs, "slt");
        
        case TokenType::LessEqual:
            return isFloat ? builder->CreateFCmpOLE(lhs, rhs, "fle")
                          : builder->CreateICmpSLE(lhs, rhs, "sle");
        
        case TokenType::GreaterThan:
            return isFloat ? builder->CreateFCmpOGT(lhs, rhs, "fgt")
                          : builder->CreateICmpSGT(lhs, rhs, "sgt");
        
        case TokenType::GreaterEqual:
            return isFloat ? builder->CreateFCmpOGE(lhs, rhs, "fge")
                          : builder->CreateICmpSGE(lhs, rhs, "sge");
        
        case TokenType::And:
            return builder->CreateAnd(lhs, rhs, "and");
        
        case TokenType::Or:
            return builder->CreateOr(lhs, rhs, "or");
        
        default:
            diag.error("Unknown binary operator", 0, 0);
            return nullptr;
    }
}

llvm::Value* Codegen::generateUnaryExpr(const UnaryExpr* expr, const Type* expectedType) {
    llvm::Value* operand = generateExpr(expr->operand.get(), expectedType);

    if (!operand) {
        return nullptr;
    }

    if (expr->op == TokenType::Minus) {
        if (operand->getType()->isIntegerTy()) {
            return builder->CreateNeg(operand, "neg");
        } else if (operand->getType()->isFloatingPointTy()) {
            return builder->CreateFNeg(operand, "fneg");
        }
    }
    if (expr->op == TokenType::Plus) {
        return operand;
    }
    if (expr->op == TokenType::Not) {
        return builder->CreateNot(operand, "not");
    }

    return nullptr;
}

llvm::Value* Codegen::generateAssignment(const Assignment* expr) {
    llvm::Value* value = generateExpr(expr->value.get());
    if (!value) {
        return nullptr;
    }

    if (auto* variable = dynamic_cast<const Variable*>(expr->lhs.get())) {
        std::string name = variable->token.lexeme;
        
        auto it = variables.find(name);
        if (it == variables.end()) {
            diag.error("Undefined variable: " + name, variable->token.line, variable->token.column);
            return nullptr;
        }
        
        llvm::Value* ptr = it->second.first;
        builder->CreateStore(value, ptr);
        return value; 
        
    } else if (auto* indexExpr = dynamic_cast<const IndexExpr*>(expr->lhs.get())) {
        llvm::Value* idx = generateExpr(indexExpr->index.get());
        if (!idx) {
            return nullptr;
        }

        if (auto* variable = dynamic_cast<const Variable*>(indexExpr->array.get())) {
            auto arrRef = variables[variable->token.lexeme].first;
            auto type = variables[variable->token.lexeme].second;

            llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
            if (!arrayType) {
                diag.error("Cannot index non-array type", 0, 0);
                return nullptr;
            }

            llvm::Value* elementPtr = builder->CreateGEP(
                type,
                arrRef,
                {builder->getInt32(0), idx},
                "element_ptr"
            );

            builder->CreateStore(value, elementPtr);
            return value;
        } else {
            diag.error("Array assignment only supported for variables currently", 0, 0);
            return nullptr;
        }
        
    } else {
        diag.error("Invalid assignment target", 0, 0);
        return nullptr;
    }
}

llvm::Value* Codegen::generateArrayLiteral(const ArrayLiteral* expr) {
    std::vector<llvm::Value*> elementValues;
    for (const auto& elem : expr->elements) {
        llvm::Value* val = generateExpr(elem.get());
        if (!val) return nullptr;
        elementValues.push_back(val);
    }
    
    llvm::Type* elementType = elementValues[0]->getType();
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, elementValues.size());
    llvm::Value* arrayPtr = builder->CreateAlloca(arrayType, nullptr, "array_literal");
    
    for (size_t i = 0; i < elementValues.size(); i++) {
        auto ptr = builder->CreateGEP(
            arrayType, 
            arrayPtr,
        {builder->getInt32(0), builder->getInt32(i)}, 
        "element_ptr");

        builder->CreateStore(elementValues[i], ptr);
    }
    
    return arrayPtr;
}

llvm::Value* Codegen::generateIndexExpr(const IndexExpr* expr) {
    llvm::Value* arr = generateExpr(expr->array.get());
    llvm::Value* idx = generateExpr(expr->index.get());


    if (auto* variable = dynamic_cast<const Variable*>(expr->array.get())) {
        auto arrRef = variables[variable->token.lexeme].first;  
        auto type = variables[variable->token.lexeme].second; 
        
        llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
        if (!arrayType) {
            diag.error("Cannot index non-array type", 0, 0);
            return nullptr;
        }
        llvm::Type* elementType = arrayType->getElementType();
        llvm::Value* elementPtr = builder->CreateGEP(
            type,
            arrRef,
            {builder->getInt32(0), idx},
            "element_ptr"
        );
        
        return builder->CreateLoad(elementType, elementPtr, "element");
    }
    
    diag.error("Array indexing only supported for variables currently", 0, 0);
    return nullptr;
}

void Codegen::fillArrayLiteral(llvm::Value* arrayPtr, llvm::Type* arrayType, const ArrayLiteral* expr) {
    for (size_t i = 0; i < expr->elements.size(); i++) {
        llvm::Value* elemValue = generateExpr(expr->elements[i].get());
        if (!elemValue) continue;

        llvm::Value* elemPtr = builder->CreateGEP(
            arrayType, arrayPtr,
            {builder->getInt32(0), builder->getInt32(i)},
            "element_ptr"
        );
        builder->CreateStore(elemValue, elemPtr);
    }
}
