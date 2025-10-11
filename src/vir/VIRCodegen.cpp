#include "vir/VIRCodegen.hpp"

namespace volta::vir {

// ============================================================================
// RuntimeFunctions
// ============================================================================

RuntimeFunctions::RuntimeFunctions(llvm::Module* module, llvm::LLVMContext* context)
    : module_(module), context_(context) {
    declareRuntimeFunctions();
}

void RuntimeFunctions::declareRuntimeFunctions() {
    // TODO: Implement in Phase 3 - Declare all volta_* runtime functions
}

// ============================================================================
// VIRCodegen - Constructor & Destructor
// ============================================================================

VIRCodegen::VIRCodegen(llvm::LLVMContext* context,
                        llvm::Module* module,
                        const volta::semantic::SemanticAnalyzer* analyzer)
    : context_(context),
      module_(module),
      builder_(std::make_unique<llvm::IRBuilder<>>(*context)),
      allocaBuilder_(std::make_unique<llvm::IRBuilder<>>(*context)),
      runtime_(std::make_unique<RuntimeFunctions>(module, context)),
      analyzer_(analyzer),
      currentFunction_(nullptr),
      hasErrors_(false) {
        // Global scope
        pushScope();
    }

VIRCodegen::~VIRCodegen() = default;

// ============================================================================
// Public API
// ============================================================================

bool VIRCodegen::generate(const VIRModule* virModule) {
    generateFunctions(virModule);
    return true;
}

// ============================================================================
// Top-Level Generation
// ============================================================================

void VIRCodegen::declareStructs(const VIRModule* module) {
    // TODO: Implement in Phase 8
}

void VIRCodegen::declareStruct(const VIRStructDecl* structDecl) {
    // TODO: Implement in Phase 8
}

void VIRCodegen::declareFunctions(const VIRModule* module) {
    // TODO: Implement in Phase 2
}

void VIRCodegen::declareFunction(const VIRFunction* func) {
    // TODO: Implement in Phase 2
}

void VIRCodegen::generateFunctions(const VIRModule* module) {
    for (auto& fn : module->getFunctions()) {
        generateFunction(fn.get());
    }
}

void VIRCodegen::generateFunction(const VIRFunction* func) {
    // Clear variables from previous function
    popScope();
    pushScope();

    llvm::FunctionType* fnType = lowerFunctionType(func);

    llvm::Function* llvmFn = llvm::Function::Create(
        fnType,
        llvm::Function::ExternalLinkage,
        func->getName(),
        module_
    );

    currentFunction_ = llvmFn;

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(*context_, "entry", llvmFn);
    builder_->SetInsertPoint(entryBB);
    allocaBuilder_->SetInsertPoint(entryBB);

    size_t idx = 0;
    for (auto& arg: llvmFn->args()) {
        const VIRParam& param = func->getParams()[idx];
        auto* paramType = lowerType(param.type);
        auto* alloca = allocaBuilder_->CreateAlloca(paramType, nullptr, param.name);
        builder_->CreateStore(&arg, alloca);
        declareVariable(param.name, alloca);
        idx++;
    }

    codegen(func->getBody());
}

// ============================================================================
// Statement Codegen
// ============================================================================

void VIRCodegen::codegen(const VIRStmt* stmt) {
    if (auto* ret = dynamic_cast<const VIRReturn*>(stmt)) {
        return codegen(ret);
    }
    if (auto* block = dynamic_cast<const VIRBlock*>(stmt)) {
        return codegen(block);
    }
    if (auto* varDecl = dynamic_cast<const VIRVarDecl*>(stmt)) {
        return codegen(varDecl);
    }
    if (auto* ifStmt = dynamic_cast<const VIRIfStmt*>(stmt)) {
        return codegen(ifStmt);
    }
    if (auto* whileLoop = dynamic_cast<const VIRWhile*>(stmt)) {
        return codegen(whileLoop);
    }
    if (auto* forRangeLoop = dynamic_cast<const VIRForRange*>(stmt)) {
        return codegen(forRangeLoop);
    }
    if (auto* forLoop = dynamic_cast<const VIRFor*>(stmt)) {
        return codegen(forLoop);
    }
    if (auto* breakStmt = dynamic_cast<const VIRBreak*>(stmt)) {
        return codegen(breakStmt);
    }
    if (auto* continueStmt = dynamic_cast<const VIRContinue*>(stmt)) {
        return codegen(continueStmt);
    }
    if (auto* exprStmt = dynamic_cast<const VIRExprStmt*>(stmt)) {
        return codegen(exprStmt);
    }
}

void VIRCodegen::codegen(const VIRBlock* block) {
    pushScope();
    for (auto& stmt : block->getStatements()) {
        codegen(stmt.get());
    }
    popScope();
}

void VIRCodegen::codegen(const VIRVarDecl* varDecl) {
    auto* llvmType = lowerType(varDecl->getType());
    auto* llvmValue = codegen(varDecl->getInitializer());
    auto* ptr = allocaBuilder_->CreateAlloca(llvmType, nullptr, varDecl->getName());
    builder_->CreateStore(llvmValue, ptr);
    declareVariable(varDecl->getName(), ptr);
}

void VIRCodegen::codegen(const VIRReturn* ret) {
    if (ret->getValue()) {
        llvm::Value* retVal = codegen(ret->getValue());
        builder_->CreateRet(retVal);
    } else {
        builder_->CreateRetVoid();
    }
}

void VIRCodegen::codegen(const VIRIfStmt* ifStmt) {
    auto* cond = codegen(ifStmt->getCondition());

    auto* thenBB = llvm::BasicBlock::Create(*context_, "thenBlock", currentFunction_);
    auto* elseBB = llvm::BasicBlock::Create(*context_, "elseBlock", currentFunction_);
    auto* mergeBB = llvm::BasicBlock::Create(*context_, "mergeBlock", currentFunction_);

    builder_->CreateCondBr(cond, thenBB, elseBB);

    // Generate then block
    builder_->SetInsertPoint(thenBB);
    codegen(ifStmt->getThenBlock());
    bool thenTerminated = builder_->GetInsertBlock()->getTerminator() != nullptr;
    if (!thenTerminated) {
        builder_->CreateBr(mergeBB);
    }

    // Generate else block
    builder_->SetInsertPoint(elseBB);
    if (ifStmt->getElseBlock()) {
        codegen(ifStmt->getElseBlock());
    }
    bool elseTerminated = builder_->GetInsertBlock()->getTerminator() != nullptr;
    if (!elseTerminated) {
        builder_->CreateBr(mergeBB);
    }

    // Only set insert point to merge if at least one branch reaches it
    if (!thenTerminated || !elseTerminated) {
        builder_->SetInsertPoint(mergeBB);
    } else {
        // Both branches terminated - merge block is unreachable, delete it
        mergeBB->eraseFromParent();
    }
}

void VIRCodegen::codegen(const VIRWhile* whileLoop) {
    auto* condBB = llvm::BasicBlock::Create(*context_, "while.cond", currentFunction_);
    auto* bodyBB = llvm::BasicBlock::Create(*context_, "while.body", currentFunction_);
    auto* mergeBB = llvm::BasicBlock::Create(*context_, "while.merge", currentFunction_);

    loopStack_.push_back(LoopState(condBB, mergeBB));

    builder_->CreateBr(condBB);
    builder_->SetInsertPoint(condBB);
    auto* cond = codegen(whileLoop->getCondition());
    builder_->CreateCondBr(cond, bodyBB, mergeBB);

    builder_->SetInsertPoint(bodyBB);
    codegen(whileLoop->getBody());

    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(condBB);
    }

    loopStack_.pop_back();

    builder_->SetInsertPoint(mergeBB);
}

void VIRCodegen::codegen(const VIRForRange* forLoop) {
    // Generate for-loop with proper increment block for continue support
    // Structure:
    //   init: i = start; end_temp = end
    //   cond: if i < end goto body else goto merge
    //   body: <user code>; goto increment
    //   increment: i = i + 1; goto cond
    //   merge: ...

    auto* condBB = llvm::BasicBlock::Create(*context_, "for.cond", currentFunction_);
    auto* bodyBB = llvm::BasicBlock::Create(*context_, "for.body", currentFunction_);
    auto* incrementBB = llvm::BasicBlock::Create(*context_, "for.increment", currentFunction_);
    auto* mergeBB = llvm::BasicBlock::Create(*context_, "for.merge", currentFunction_);

    // Push loop state: continue goes to increment, break goes to merge
    loopStack_.push_back(LoopState(incrementBB, mergeBB));

    // Initialize loop variable: let mut i = start
    auto* loopVarType = lowerType(forLoop->getVarType());
    auto* startValue = codegen(forLoop->getStart());

    // Ensure allocaBuilder points to the start of entry block (before any terminators)
    llvm::BasicBlock& entryBlock = currentFunction_->getEntryBlock();
    if (entryBlock.empty()) {
        allocaBuilder_->SetInsertPoint(&entryBlock);
    } else {
        allocaBuilder_->SetInsertPoint(&entryBlock, entryBlock.begin());
    }

    auto* loopVarPtr = allocaBuilder_->CreateAlloca(loopVarType, nullptr, forLoop->getLoopVar());
    builder_->CreateStore(startValue, loopVarPtr);
    declareVariable(forLoop->getLoopVar(), loopVarPtr);

    // Cache end value: let end_temp = end
    std::string endTempName = "__end_" + forLoop->getLoopVar();
    auto* endValue = codegen(forLoop->getEnd());

    // Again ensure allocaBuilder is at the start
    if (entryBlock.empty()) {
        allocaBuilder_->SetInsertPoint(&entryBlock);
    } else {
        allocaBuilder_->SetInsertPoint(&entryBlock, entryBlock.begin());
    }

    auto* endTempPtr = allocaBuilder_->CreateAlloca(loopVarType, nullptr, endTempName);
    builder_->CreateStore(endValue, endTempPtr);

    // Jump to condition
    builder_->CreateBr(condBB);

    // Generate condition block
    builder_->SetInsertPoint(condBB);
    auto* loopVarValue = builder_->CreateLoad(loopVarType, loopVarPtr, "i");
    auto* endTempValue = builder_->CreateLoad(loopVarType, endTempPtr, "end");

    // Compare: i < end (or i <= end for inclusive)
    llvm::Value* cond;
    if (forLoop->isInclusive()) {
        // Check if signed or unsigned
        if (forLoop->getVarType()->isSignedInteger()) {
            cond = builder_->CreateICmpSLE(loopVarValue, endTempValue);
        } else {
            cond = builder_->CreateICmpULE(loopVarValue, endTempValue);
        }
    } else {
        if (forLoop->getVarType()->isSignedInteger()) {
            cond = builder_->CreateICmpSLT(loopVarValue, endTempValue);
        } else {
            cond = builder_->CreateICmpULT(loopVarValue, endTempValue);
        }
    }
    builder_->CreateCondBr(cond, bodyBB, mergeBB);

    // Generate body block
    builder_->SetInsertPoint(bodyBB);
    codegen(forLoop->getBody());

    // Branch to increment (if not already terminated by break/return)
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(incrementBB);
    }

    // Generate increment block
    builder_->SetInsertPoint(incrementBB);
    auto* currentValue = builder_->CreateLoad(loopVarType, loopVarPtr, "i");
    auto* one = llvm::ConstantInt::get(loopVarType, 1);
    auto* nextValue = builder_->CreateAdd(currentValue, one, "i.next");
    builder_->CreateStore(nextValue, loopVarPtr);
    builder_->CreateBr(condBB);

    // Pop loop state
    loopStack_.pop_back();

    // Continue at merge block
    builder_->SetInsertPoint(mergeBB);
}

void VIRCodegen::codegen(const VIRFor* forLoop) {
    // TODO: Implement in Phase 5 (for general iterables)
}

void VIRCodegen::codegen(const VIRBreak* breakStmt) {
    if (loopStack_.empty()) {
        reportError("break statement outside of loop at " + 
            breakStmt->getLocation().toString());
        return;
    }
    auto& currLoop = loopStack_.back();
    builder_->CreateBr(currLoop.exitBlock);
}

void VIRCodegen::codegen(const VIRContinue* continueStmt) {
    if (loopStack_.empty()) {
        reportError("break statement outside of loop at " + 
            continueStmt->getLocation().toString());
        return;
    }
    auto& currLoop = loopStack_.back();
    builder_->CreateBr(currLoop.conditionBlock);
}

void VIRCodegen::codegen(const VIRExprStmt* exprStmt) {
    codegen(exprStmt->getExpr());
}

// ============================================================================
// Expression Codegen
// ============================================================================

llvm::Value* VIRCodegen::codegen(const VIRExpr* expr) {
    if (auto* intLit = dynamic_cast<const VIRIntLiteral*>(expr)) {
        return codegen(intLit);
    }
    if (auto* floatLit = dynamic_cast<const VIRFloatLiteral*>(expr)) {
        return codegen(floatLit);
    }
    if (auto* boolLit = dynamic_cast<const VIRBoolLiteral*>(expr)) {
        return codegen(boolLit);
    }
    if (auto* stringLit = dynamic_cast<const VIRStringLiteral*>(expr)) {
        return codegen(stringLit);
    }
    if (auto* localRef = dynamic_cast<const VIRLocalRef*>(expr)) {
        return codegen(localRef);
    }
    if (auto* paramRef = dynamic_cast<const VIRParamRef*>(expr)) {
        return codegen(paramRef);
    }
    if (auto* globalRef = dynamic_cast<const VIRGlobalRef*>(expr)) {
        return codegen(globalRef);
    }
    if (auto* binaryOp = dynamic_cast<const VIRBinaryOp*>(expr)) {
        return codegen(binaryOp);
    }
    if (auto* unaryOp = dynamic_cast<const VIRUnaryOp*>(expr)) {
        return codegen(unaryOp);
    }
    if (auto* box = dynamic_cast<const VIRBox*>(expr)) {
        return codegen(box);
    }
    if (auto* unbox = dynamic_cast<const VIRUnbox*>(expr)) {
        return codegen(unbox);
    }
    if (auto* cast = dynamic_cast<const VIRCast*>(expr)) {
        return codegen(cast);
    }
    if (auto* implicitCast = dynamic_cast<const VIRImplicitCast*>(expr)) {
        return codegen(implicitCast);
    }
    if (auto* call = dynamic_cast<const VIRCall*>(expr)) {
        return codegen(call);
    }
    if (auto* callRuntime = dynamic_cast<const VIRCallRuntime*>(expr)) {
        return codegen(callRuntime);
    }
    if (auto* callIndirect = dynamic_cast<const VIRCallIndirect*>(expr)) {
        return codegen(callIndirect);
    }
    if (auto* alloca = dynamic_cast<const VIRAlloca*>(expr)) {
        return codegen(alloca);
    }
    if (auto* load = dynamic_cast<const VIRLoad*>(expr)) {
        return codegen(load);
    }
    if (auto* store = dynamic_cast<const VIRStore*>(expr)) {
        return codegen(store);
    }
    if (auto* ifExpr = dynamic_cast<const VIRIfExpr*>(expr)) {
        return codegen(ifExpr);
    }
    if (auto* structNew = dynamic_cast<const VIRStructNew*>(expr)) {
        return codegen(structNew);
    }
    if (auto* structGet = dynamic_cast<const VIRStructGet*>(expr)) {
        return codegen(structGet);
    }
    if (auto* structSet = dynamic_cast<const VIRStructSet*>(expr)) {
        return codegen(structSet);
    }
    if (auto* arrayNew = dynamic_cast<const VIRArrayNew*>(expr)) {
        return codegen(arrayNew);
    }
    if (auto* arrayGet = dynamic_cast<const VIRArrayGet*>(expr)) {
        return codegen(arrayGet);
    }
    if (auto* arraySet = dynamic_cast<const VIRArraySet*>(expr)) {
        return codegen(arraySet);
    }
    if (auto* boundsCheck = dynamic_cast<const VIRBoundsCheck*>(expr)) {
        return codegen(boundsCheck);
    }

    return nullptr;
}

// Literals

llvm::Value* VIRCodegen::codegen(const VIRIntLiteral* lit) {
    llvm::Type* type = lowerType(lit->getType());
    bool isSigned = lit->getType()->isSignedInteger();
    return llvm::ConstantInt::get(type, lit->getValue(), isSigned);
}

llvm::Value* VIRCodegen::codegen(const VIRFloatLiteral* lit) {
    llvm::Type* type = lowerType(lit->getType());
    return llvm::ConstantFP::get(type, lit->getValue());
}

llvm::Value* VIRCodegen::codegen(const VIRBoolLiteral* lit) {
    llvm::Type* type = lowerType(lit->getType());
    return llvm::ConstantInt::get(type, lit->getValue(), false);
}

llvm::Value* VIRCodegen::codegen(const VIRStringLiteral* lit) {
    // TODO: Implement in Phase 3
    return nullptr;
}

// References

llvm::Value* VIRCodegen::codegen(const VIRLocalRef* ref) {
    // Return the pointer to the variable, not the loaded value
    // The caller will wrap in VIRLoad if they want the value
    auto* ptr = lookupVariable(ref->getName());
    return ptr;
}

llvm::Value* VIRCodegen::codegen(const VIRParamRef* ref) {
    // TODO: Implement in Phase 2
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRGlobalRef* ref) {
    // TODO: Implement in Phase 3
    return nullptr;
}

// Operations

llvm::Value* VIRCodegen::codegen(const VIRBinaryOp* op) {
    auto* lhs = codegen(op->getLeft());
    auto* rhs = codegen(op->getRight());
    auto typeKind = op->getType()->kind();
    
    bool isFloat = (typeKind == semantic::Type::Kind::F32 || 
                    typeKind == semantic::Type::Kind::F64 ||
                    typeKind == semantic::Type::Kind::F8  || 
                    typeKind == semantic::Type::Kind::F16);

    bool isSigned = (typeKind == semantic::Type::Kind::I8  || 
                    typeKind == semantic::Type::Kind::I16 ||
                    typeKind == semantic::Type::Kind::I32 || 
                    typeKind == semantic::Type::Kind::I64);

    bool isUnsigned = (typeKind == semantic::Type::Kind::U8  || 
                    typeKind == semantic::Type::Kind::U16 ||
                    typeKind == semantic::Type::Kind::U32 || 
                    typeKind == semantic::Type::Kind::U64);
    
    switch (op->getOp()) {
        case VIRBinaryOpKind::Add:
            if (isFloat) return builder_->CreateFAdd(lhs, rhs);
            else return builder_->CreateAdd(lhs, rhs);
        case VIRBinaryOpKind::Sub:
            if (isFloat) return builder_->CreateFSub(lhs, rhs);
            else return builder_->CreateSub(lhs, rhs);

        case VIRBinaryOpKind::Mul:
            if (isFloat) return builder_->CreateFMul(lhs, rhs);
            else return builder_->CreateMul(lhs, rhs);

        case VIRBinaryOpKind::Div:
            if (isFloat) return builder_->CreateFDiv(lhs, rhs);
            else if (isSigned) return builder_->CreateSDiv(lhs, rhs);
            else return builder_->CreateUDiv(lhs, rhs);

        case VIRBinaryOpKind::Mod:
            if (isFloat) return builder_->CreateFRem(lhs, rhs);
            else if (isSigned) return builder_->CreateSRem(lhs, rhs);
            else return builder_->CreateURem(lhs, rhs);

        case VIRBinaryOpKind::Eq:
            if (isFloat) return builder_->CreateFCmpOEQ(lhs, rhs);
            else return builder_->CreateICmpEQ(lhs, rhs);

        case VIRBinaryOpKind::Ne:
            if (isFloat) return builder_->CreateFCmpONE(lhs, rhs);
            else return builder_->CreateICmpNE(lhs, rhs);

        case VIRBinaryOpKind::Lt:
            if (isFloat) return builder_->CreateFCmpOLT(lhs, rhs);
            else if (isSigned) return builder_->CreateICmpSLT(lhs, rhs);
            else return builder_->CreateICmpULT(lhs, rhs);

        case VIRBinaryOpKind::Le:
            if (isFloat) return builder_->CreateFCmpOLE(lhs, rhs);
            else if (isSigned) return builder_->CreateICmpSLE(lhs, rhs);
            else return builder_->CreateICmpULE(lhs, rhs);

        case VIRBinaryOpKind::Gt:
            if (isFloat) return builder_->CreateFCmpOGT(lhs, rhs);
            else if (isSigned) return builder_->CreateICmpSGT(lhs, rhs);
            else return builder_->CreateICmpUGT(lhs, rhs);

        case VIRBinaryOpKind::Ge:
            if (isFloat) return builder_->CreateFCmpOGE(lhs, rhs);
            else if (isSigned) return builder_->CreateICmpSGE(lhs, rhs);
            else return builder_->CreateICmpUGE(lhs, rhs);

        case VIRBinaryOpKind::And:
            return builder_->CreateAnd(lhs, rhs);

        case VIRBinaryOpKind::Or:
            return builder_->CreateOr(lhs, rhs);
    } 
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRUnaryOp* unaryOp) {
    auto* operand = codegen(unaryOp->getOperand());
    auto opKind = unaryOp->getOp();
    auto typeKind = unaryOp->getType()->kind();

    // Determine if float
    bool isFloat = (typeKind == semantic::Type::Kind::F32 ||
                    typeKind == semantic::Type::Kind::F64 ||
                    typeKind == semantic::Type::Kind::F8 ||
                    typeKind == semantic::Type::Kind::F16);

    switch (opKind) {
        case VIRUnaryOpKind::Neg:
            if (isFloat) return builder_->CreateFNeg(operand);
            else return builder_->CreateNeg(operand);

        case VIRUnaryOpKind::Not:
            return builder_->CreateNot(operand);
    }

    return nullptr;
}

// Type operations

llvm::Value* VIRCodegen::codegen(const VIRBox* box) {
    // TODO: Implement in Phase 6
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRUnbox* unbox) {
    // TODO: Implement in Phase 6
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRCast* cast) {
    // TODO: Implement in Phase 2
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRImplicitCast* cast) {
    // TODO: Implement in Phase 2
    return nullptr;
}

// Calls

llvm::Value* VIRCodegen::codegen(const VIRCall* call) {
    // Look up the function in the module
    llvm::Function* calleeFunc = module_->getFunction(call->getFunctionName());

    if (!calleeFunc) {
        // Function not found - should have been caught by semantic analyzer
        return nullptr;
    }

    // Codegen all arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : call->getArgs()) {
        args.push_back(codegen(arg.get()));
    }

    // Create the call instruction
    return builder_->CreateCall(calleeFunc, args);
}

llvm::Value* VIRCodegen::codegen(const VIRCallRuntime* call) {
    // TODO: Implement in Phase 3
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRCallIndirect* call) {
    // TODO: Implement in Phase 7
    return nullptr;
}

// Wrapper generation

llvm::Function* VIRCodegen::codegen(const VIRWrapFunction* wrap) {
    // TODO: Implement in Phase 7
    return nullptr;
}

// Memory operations

llvm::Value* VIRCodegen::codegen(const VIRAlloca* alloca) {
    // TODO: Implement in Phase 2
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRLoad* load) {
    auto* ptr = codegen(load->getPointer());
    auto* type = lowerType(load->getType());
    return builder_->CreateLoad(type, ptr);
}

llvm::Value* VIRCodegen::codegen(const VIRStore* store) {
    auto* ptr = codegen(store->getPointer());
    auto* val = codegen(store->getValue());
    return builder_->CreateStore(val, ptr);
}

// Control flow

llvm::Value* VIRCodegen::codegen(const VIRIfExpr* ifExpr) {
    // TODO
    return nullptr;
}

// Struct operations

llvm::Value* VIRCodegen::codegen(const VIRStructNew* structNew) {
    // TODO: Implement in Phase 8
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRStructGet* structGet) {
    // TODO: Implement in Phase 8
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRStructSet* structSet) {
    // TODO: Implement in Phase 8
    return nullptr;
}

// Array operations

llvm::Value* VIRCodegen::codegen(const VIRArrayNew* arrayNew) {
    // TODO: Implement in Phase 6
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRArrayGet* arrayGet) {
    // TODO: Implement in Phase 6
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRArraySet* arraySet) {
    // TODO: Implement in Phase 6
    return nullptr;
}

// Safety operations

llvm::Value* VIRCodegen::codegen(const VIRBoundsCheck* boundsCheck) {
    // TODO: Implement in Phase 6
    return nullptr;
}

// ============================================================================
// Fixed Array Operations (Phase 7)
// ============================================================================

llvm::Value* VIRCodegen::codegen(const VIRFixedArrayNew* arrayNew) {
    // TODO: Implement in Phase 7 Week 7 Days 4-5
    // 1. Get LLVM array type: llvm::ArrayType::get(elemType, size)
    // 2. Check isStackAllocated flag
    // 3. If stack:
    //    - CreateAlloca(arrayType)
    //    - Initialize elements with CreateGEP + CreateStore
    // 4. If heap:
    //    - Call volta_gc_alloc with array size
    //    - CreateBitCast to array pointer type
    //    - Initialize elements with CreateGEP + CreateStore
    // 5. Return pointer to array
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRFixedArrayGet* arrayGet) {
    // TODO: Implement in Phase 7 Week 7 Days 4-5
    // 1. Codegen array pointer
    // 2. Codegen index (already bounds-checked)
    // 3. CreateGEP to compute element address:
    //    CreateGEP(arrayType, arrayPtr, {0, index})
    // 4. CreateLoad to get element value
    // 5. Return loaded value
    return nullptr;
}

llvm::Value* VIRCodegen::codegen(const VIRFixedArraySet* arraySet) {
    // TODO: Implement in Phase 7 Week 7 Days 4-5
    // 1. Codegen array pointer
    // 2. Codegen index (already bounds-checked)
    // 3. Codegen value to store
    // 4. CreateGEP to compute element address:
    //    CreateGEP(arrayType, arrayPtr, {0, index})
    // 5. CreateStore to write value
    // 6. Return nullptr (unit type)
    return nullptr;
}

// ========================================================================
// Scope Managment
// ========================================================================

void VIRCodegen::pushScope() {
    scopeStack_.push_back({});
}

void VIRCodegen::popScope() {
    // the global scope is always present!
    if (scopeStack_.size() > 1) {
        scopeStack_.pop_back();
    }
}

void VIRCodegen::declareVariable(std::string name, llvm::AllocaInst* ptr) {
    scopeStack_.back()[name] = ptr;
}


llvm::AllocaInst* VIRCodegen::lookupVariable(std::string name) {
    for (auto rit = scopeStack_.rbegin(); rit != scopeStack_.rend(); ++rit) {
        auto it = rit->find(name);
        if (it != rit->end()) {
            return it->second;
        }
    }

    return nullptr; 
}


// ============================================================================
// Type Lowering
// ============================================================================

llvm::Type* VIRCodegen::lowerType(std::shared_ptr<volta::semantic::Type> type) {
    // by convention nullptr is a void type.
    if (!type || type->kind() == semantic::Type::Kind::Void) {
        return llvm::Type::getVoidTy(*context_);
    }

    switch (type->kind()) {
        case semantic::Type::Kind::I8:
        case semantic::Type::Kind::U8:
            return llvm::Type::getInt8Ty(*context_);

        case semantic::Type::Kind::I16:
        case semantic::Type::Kind::U16:
            return llvm::Type::getInt16Ty(*context_);

        case semantic::Type::Kind::I32:
        case semantic::Type::Kind::U32:
            return llvm::Type::getInt32Ty(*context_);

        case semantic::Type::Kind::I64:
        case semantic::Type::Kind::U64:
            return llvm::Type::getInt64Ty(*context_);

        case semantic::Type::Kind::Bool:
            return llvm::Type::getInt1Ty(*context_);

        // TODO: Allow support for half type.
        case semantic::Type::Kind::F8:
        case semantic::Type::Kind::F16:
        case semantic::Type::Kind::F32:
            return llvm::Type::getFloatTy(*context_);
        case semantic::Type::Kind::F64:
            return llvm::Type::getDoubleTy(*context_);

        // TODO: Handle strings, arrays, structs and enums.
        default:
            return nullptr;  // Unknown type
    }
}

llvm::FunctionType* VIRCodegen::lowerFunctionType(const VIRFunction* func) {
    llvm::Type* retType = lowerType(func->getReturnType());
    std::vector<llvm::Type*> paramTypes;

    for (auto& par : func->getParams()) {
        paramTypes.push_back(lowerType(par.type));
    }

    // For now no varargs.
    return llvm::FunctionType::get(retType, paramTypes, false);
}

// ============================================================================
// Helper Functions
// ============================================================================

llvm::AllocaInst* VIRCodegen::createEntryBlockAlloca(llvm::Function* fn,
                                                      const std::string& varName,
                                                      llvm::Type* type) {
    // TODO: Implement in Phase 1
    return nullptr;
}

llvm::Value* VIRCodegen::createBox(llvm::Value* value, llvm::Type* valueType) {
    // TODO: Implement in Phase 6
    return nullptr;
}

llvm::Value* VIRCodegen::createUnbox(llvm::Value* boxed, llvm::Type* targetType) {
    // TODO: Implement in Phase 6
    return nullptr;
}

llvm::StructType* VIRCodegen::getStructType(const std::string& name) {
    // TODO: Implement in Phase 8
    return nullptr;
}

void VIRCodegen::reportError(const std::string& message) {
    hasErrors_ = true;
    errors_.push_back(message);
}

} // namespace volta::vir
