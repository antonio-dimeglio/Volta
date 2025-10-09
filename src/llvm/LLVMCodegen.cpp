#include "llvm/LLVMCodegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

using namespace llvm;

namespace volta::llvm_codegen {

// ============================================================================
// Construction & Destruction
// ============================================================================

LLVMCodegen::LLVMCodegen(const std::string& moduleName)
    : context_(std::make_unique<LLVMContext>()),
      module_(std::make_unique<Module>(moduleName, *context_)),
      builder_(std::make_unique<IRBuilder<>>(*context_)),
      allocaBuilder_(std::make_unique<IRBuilder<>>(*context_)),
      runtime_(nullptr),  // Initialized in generate()
      analyzer_(nullptr),
      currentFunction_(nullptr),
      hasErrors_(false) {
    module_->setTargetTriple(sys::getDefaultTargetTriple());
}

LLVMCodegen::~LLVMCodegen() = default;

// ============================================================================
// Public API
// ============================================================================

bool LLVMCodegen::generate(
    const volta::ast::Program* program,
    const volta::semantic::SemanticAnalyzer* analyzer) {

    if (!program || !analyzer) {
        reportError("Null program or analyzer");
        return false;
    }

    analyzer_ = analyzer;

    // Initialize runtime functions
    runtime_ = std::make_unique<RuntimeFunctions>(module_.get(), context_.get());

    // Generate all top-level declarations
    for (const auto& stmt : program->statements) {
        if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt.get())) {
            generateFunction(fnDecl);
        }
        // TODO: Add struct declarations, enum declarations, global variables
    }

    // Verify the module
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    if (verifyModule(*module_, &errorStream)) {
        reportError("Module verification failed: " + errorMsg);
        return false;
    }

    return !hasErrors_;
}

bool LLVMCodegen::emitLLVMIR(const std::string& filename) {
    std::error_code ec;
    raw_fd_ostream file(filename, ec, sys::fs::OF_None);

    if (ec) {
        reportError("Failed to open file: " + filename);
        return false;
    }

    module_->print(file, nullptr);
    return true;
}

bool LLVMCodegen::compileToObject(const std::string& filename) {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    std::string error;
    auto targetTriple = module_->getTargetTriple();
    auto target = TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        reportError("Failed to lookup target: " + error);
        return false;
    }

    TargetOptions opt;
    auto rm = std::optional<Reloc::Model>();
    auto targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, rm);

    module_->setDataLayout(targetMachine->createDataLayout());

    std::error_code ec;
    raw_fd_ostream dest(filename, ec, sys::fs::OF_None);

    if (ec) {
        reportError("Could not open file: " + ec.message());
        return false;
    }

    legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr,
                                          CodeGenFileType::ObjectFile)) {
        reportError("TargetMachine can't emit a file of this type");
        return false;
    }

    pass.run(*module_);
    dest.flush();

    return true;
}

// ============================================================================
// Top-Level Code Generation
// ============================================================================

void LLVMCodegen::generateFunction(const volta::ast::FnDeclaration* fn) {
    namedValues_.clear();
    loopStack_.clear();

    // Build parameter types
    std::vector<Type*> paramTypes;
    for (const auto& param : fn->parameters) {
        auto paramType = analyzer_->resolveTypeAnnotation(param->type.get());
        paramTypes.push_back(lowerType(paramType));
    }

    // Build return type
    auto returnType = lowerType(analyzer_->resolveTypeAnnotation(fn->returnType.get()));

    // Create function signature
    FunctionType* funcType = FunctionType::get(returnType, paramTypes, false);
    Function* llvmFn = Function::Create(
        funcType,
        Function::ExternalLinkage,
        fn->identifier,
        module_.get()
    );

    // Create entry basic block
    BasicBlock* entry = BasicBlock::Create(*context_, "entry", llvmFn);
    builder_->SetInsertPoint(entry);
    // Set alloca builder to always insert at the start of entry block
    // This ensures all allocas are placed before any other instructions
    allocaBuilder_->SetInsertPoint(entry, entry->begin());

    // Set parameter names and store them in namedValues_
    auto paramIt = fn->parameters.begin();
    for (auto& arg : llvmFn->args()) {
        auto name = (*paramIt)->identifier;
        arg.setName(name);

        auto argPtr = createEntryBlockAlloca(llvmFn, name, arg.getType());
        builder_->CreateStore(&arg, argPtr);
        namedValues_[name] = argPtr; 
        ++paramIt;
    }


    currentFunction_ = llvmFn;

    // Generate function body
    if (fn->body) {
        generateBlock(fn->body.get());
    } else if (fn->expressionBody) {
        // Single-expression function body: fn foo() = expr
        Value* result = generateExpression(fn->expressionBody.get());
        if (result) {
            builder_->CreateRet(result);
        }
    }

    // Add implicit return if missing
    if (!builder_->GetInsertBlock()->getTerminator()) {
        if (returnType->isVoidTy()) {
            builder_->CreateRetVoid();
        } else {
            reportError("Function '" + fn->identifier + "' missing return statement");
        }
    }

    // Verify the function
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    if (verifyFunction(*llvmFn, &errorStream)) {
        reportError("Function verification failed: " + errorMsg);
        llvmFn->eraseFromParent();
    }

    currentFunction_ = nullptr;
}

// ============================================================================
// Statement Generation
// ============================================================================

void LLVMCodegen::generateStatement(const volta::ast::Statement* stmt) {
    if (auto* returnStmt = dynamic_cast<const volta::ast::ReturnStatement*>(stmt)) {
        generateReturnStatement(returnStmt);
    }
    else if (auto* ifStmt = dynamic_cast<const volta::ast::IfStatement*>(stmt)) {
        generateIfStatement(ifStmt);
    }
    else if (auto* whileStmt = dynamic_cast<const volta::ast::WhileStatement*>(stmt)) {
        generateWhileStatement(whileStmt);
    }
    else if (auto* forStmt = dynamic_cast<const volta::ast::ForStatement*>(stmt)) {
        generateForStatement(forStmt);
    }
    else if (auto* varDecl = dynamic_cast<const volta::ast::VarDeclaration*>(stmt)) {
        generateVarDeclaration(varDecl);
    }
    else if (auto* exprStmt = dynamic_cast<const volta::ast::ExpressionStatement*>(stmt)) {
        generateExpressionStatement(exprStmt);
    }
    else if (auto* breakStmt = dynamic_cast<const volta::ast::BreakStatement*>(stmt)) {
        generateBreakStatement(breakStmt);
    }
    else if (auto* continueStmt = dynamic_cast<const volta::ast::ContinueStatement*>(stmt)) {
        generateContinueStatement(continueStmt);
    }
    else if (auto* block = dynamic_cast<const volta::ast::Block*>(stmt)) {
        generateBlock(block);
    }
    else {
        reportError("Unsupported statement type at " + stmt->location.toString());
    }
}

void LLVMCodegen::generateBlock(const volta::ast::Block* block) {
    for (const auto& stmt : block->statements) {
        // Stop generating if we've already hit a terminator (return/break/continue)
        if (builder_->GetInsertBlock()->getTerminator()) {
            break;
        }
        generateStatement(stmt.get());
    }
}

void LLVMCodegen::generateExpressionStatement(const volta::ast::ExpressionStatement* stmt) {
    // Generate expression for side effects, discard result
    generateExpression(stmt->expr.get());
}

void LLVMCodegen::generateVarDeclaration(const volta::ast::VarDeclaration* stmt) {
    std::string varName = stmt->identifier;

    auto voltaType = analyzer_->getExpressionType(stmt->initializer.get());
    llvm::Type* llvmType = lowerType(voltaType);
    

    llvm::AllocaInst* varPtr = createEntryBlockAlloca(currentFunction_, varName, llvmType);
    

    llvm::Value* varValue = generateExpression(stmt->initializer.get());
    if (!varValue) {
        reportError("Failed to generate initializer for variable " + varName);
        return;
    }
    
    // Store and track
    builder_->CreateStore(varValue, varPtr);
    namedValues_[varName] = varPtr;
}

void LLVMCodegen::generateIfStatement(const volta::ast::IfStatement* stmt) {
    Value* cond = generateExpression(stmt->condition.get());
    if (!cond) return;

    auto* fn = currentFunction_;


    auto* thenBB = BasicBlock::Create(*context_, "then", fn);
    auto* elseBB = BasicBlock::Create(*context_, "else");
    auto* mergeBB = BasicBlock::Create(*context_, "merge");


    builder_->CreateCondBr(cond, thenBB, elseBB);

    builder_->SetInsertPoint(thenBB);
    generateBlock(stmt->thenBlock.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(mergeBB);
    }

    fn->insert(fn->end(), elseBB);
    builder_->SetInsertPoint(elseBB);
    if (stmt->elseBlock) {
        generateBlock(stmt->elseBlock.get());
    }
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(mergeBB);
    }

    fn->insert(fn->end(), mergeBB);
    builder_->SetInsertPoint(mergeBB);
}

void LLVMCodegen::generateWhileStatement(const volta::ast::WhileStatement* stmt) {
    auto* fn = currentFunction_;

    auto* condBB = BasicBlock::Create(*context_, "cond", fn);
    auto* bodyBB = BasicBlock::Create(*context_, "body", fn);
    auto* exitBB = BasicBlock::Create(*context_, "exit", fn);
    loopStack_.push_back({condBB, exitBB});

    builder_->CreateBr(condBB);
    builder_->SetInsertPoint(condBB);
    Value* cond = generateExpression(stmt->condition.get());
    if (!cond) return;

    builder_->CreateCondBr(cond, bodyBB, exitBB);
    builder_->SetInsertPoint(bodyBB);
    generateBlock(stmt->thenBlock.get());
    
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(condBB);
    }

    loopStack_.pop_back();

    builder_->SetInsertPoint(exitBB);
}

void LLVMCodegen::generateForStatement(const volta::ast::ForStatement* stmt) {
    if (auto* binExpr = dynamic_cast<const volta::ast::BinaryExpression*>(stmt->expression.get())) {
        if (binExpr->op == volta::ast::BinaryExpression::Operator::Range ||
            binExpr->op == volta::ast::BinaryExpression::Operator::RangeInclusive) {
            
            generateForRangeLoop(stmt, binExpr);
            return;
        }
    }

    generateForArrayLoop(stmt);
}


void LLVMCodegen::generateForRangeLoop(const volta::ast::ForStatement* stmt, const volta::ast::BinaryExpression* binExpr) {
    auto iterVarName = stmt->identifier;
    auto iterVarType = analyzer_->getExpressionType(binExpr->left.get());
    llvm::Type* llvmIterType = lowerType(iterVarType);

    // Create variable for iteration
    auto* iterAlloca = createEntryBlockAlloca(currentFunction_, iterVarName, llvmIterType);
    namedValues_[iterVarName] = iterAlloca;
    
    // Generate start and end values
    Value* startValue = generateExpression(binExpr->left.get());
    Value* endValue = generateExpression(binExpr->right.get());
    
    // Store start value in iterator
    builder_->CreateStore(startValue, iterAlloca);
    
    // Determine if loop is reversed at runtime
    Value* isReversed = builder_->CreateICmpSGT(startValue, endValue, "isReversed");
    
    auto* fn = currentFunction_;
    
    // Create all basic blocks
    auto* forwardCondBB = BasicBlock::Create(*context_, "forward.cond", fn);
    auto* forwardBodyBB = BasicBlock::Create(*context_, "forward.body", fn);
    auto* reverseCondBB = BasicBlock::Create(*context_, "reverse.cond", fn);
    auto* reverseBodyBB = BasicBlock::Create(*context_, "reverse.body", fn);
    auto* exitBB = BasicBlock::Create(*context_, "for.exit");
    
    // Branch to appropriate loop
    builder_->CreateCondBr(isReversed, reverseCondBB, forwardCondBB);
    
    // Forward loop (start < end, increment)

    builder_->SetInsertPoint(forwardCondBB);
    Value* forwardIterValue = builder_->CreateLoad(llvmIterType, iterAlloca, iterVarName);
    
    // Comparison: i < end or i <= end (for inclusive)
    Value* forwardCond;
    if (binExpr->op == volta::ast::BinaryExpression::Operator::Range) {
        forwardCond = builder_->CreateICmpSLT(forwardIterValue, endValue, "forwardCmp");
    } else {
        forwardCond = builder_->CreateICmpSLE(forwardIterValue, endValue, "forwardCmp");
    }
    builder_->CreateCondBr(forwardCond, forwardBodyBB, exitBB);
    
    // Forward body
    builder_->SetInsertPoint(forwardBodyBB);
    loopStack_.push_back({forwardCondBB, exitBB});  // For break/continue
    generateBlock(stmt->thenBlock.get());
    loopStack_.pop_back();
    
    // Increment: i = i + 1
    if (!builder_->GetInsertBlock()->getTerminator()) {
        Value* forwardCurrentIter = builder_->CreateLoad(llvmIterType, iterAlloca, "loadIter");
        Value* one = ConstantInt::get(llvmIterType, 1);
        Value* forwardNextIter = builder_->CreateAdd(forwardCurrentIter, one, "nextIter");
        builder_->CreateStore(forwardNextIter, iterAlloca);
        builder_->CreateBr(forwardCondBB);
    }
    
    // ========================================================================
    // REVERSE LOOP (start > end, decrement)
    // ========================================================================
    
    fn->insert(fn->end(), reverseCondBB);
    builder_->SetInsertPoint(reverseCondBB);
    Value* reverseIterValue = builder_->CreateLoad(llvmIterType, iterAlloca, iterVarName);
    
    Value* reverseCond;
    if (binExpr->op == volta::ast::BinaryExpression::Operator::Range) {
        reverseCond = builder_->CreateICmpSGT(reverseIterValue, endValue, "reverseCmp");
    } else {
        reverseCond = builder_->CreateICmpSGE(reverseIterValue, endValue, "reverseCmp");
    }
    builder_->CreateCondBr(reverseCond, reverseBodyBB, exitBB);
    
    fn->insert(fn->end(), reverseBodyBB);
    builder_->SetInsertPoint(reverseBodyBB);
    loopStack_.push_back({reverseCondBB, exitBB});  // For break/continue
    generateBlock(stmt->thenBlock.get());
    loopStack_.pop_back();
    
    if (!builder_->GetInsertBlock()->getTerminator()) {
        Value* reverseCurrentIter = builder_->CreateLoad(llvmIterType, iterAlloca, "loadIter");
        Value* minusOne = ConstantInt::get(llvmIterType, -1);
        Value* reverseNextIter = builder_->CreateAdd(reverseCurrentIter, minusOne, "nextIter");
        builder_->CreateStore(reverseNextIter, iterAlloca);
        builder_->CreateBr(reverseCondBB);
    }
    
    // ========================================================================
    // EXIT
    // ========================================================================
    
    fn->insert(fn->end(), exitBB);
    builder_->SetInsertPoint(exitBB);
}

void LLVMCodegen::generateForArrayLoop(const volta::ast::ForStatement* stmt) {
    // 1. Create counter alloca (index)
    // 2. Store 0 in counter
    // 3. Get array length (runtime call)
    // 4. Create blocks: cond, body, exit
    // 5. In cond: load counter, compare to length, branch
    // 6. In body:
    //    a. Index into array with counter
    //    b. Create alloca for 'identifier' (elem)
    //    c. Store array element in elem alloca
    //    d. Execute thenBlock
    //    e. Increment counter
    // 7. Set insert point to exit
    reportError("For loops for array not yet implemented.");
    return;
}

void LLVMCodegen::generateReturnStatement(const volta::ast::ReturnStatement* stmt) {
    if (stmt->expression) {
        Value* returnValue = generateExpression(stmt->expression.get());
        if (returnValue) {
            builder_->CreateRet(returnValue);
        }
    } else {
        builder_->CreateRetVoid();
    }
}

void LLVMCodegen::generateBreakStatement(const volta::ast::BreakStatement* stmt) {
    if (loopStack_.empty()) {
        reportError("Break statement outside loop at " + stmt->location.toString());
        return;
    }
    builder_->CreateBr(loopStack_.back().exitBlock);
}

void LLVMCodegen::generateContinueStatement(const volta::ast::ContinueStatement* stmt) {
    if (loopStack_.empty()) {
        reportError("Continue statement outside loop at " + stmt->location.toString());
        return;
    }
    builder_->CreateBr(loopStack_.back().conditionBlock);
}

// ============================================================================
// Expression Generation - Main Dispatcher
// ============================================================================

Value* LLVMCodegen::generateExpression(const volta::ast::Expression* expr) {
    // Literals
    if (auto* intLit = dynamic_cast<const volta::ast::IntegerLiteral*>(expr)) {
        return generateIntegerLiteral(intLit);
    }
    if (auto* floatLit = dynamic_cast<const volta::ast::FloatLiteral*>(expr)) {
        return generateFloatLiteral(floatLit);
    }
    if (auto* strLit = dynamic_cast<const volta::ast::StringLiteral*>(expr)) {
        return generateStringLiteral(strLit);
    }
    if (auto* boolLit = dynamic_cast<const volta::ast::BooleanLiteral*>(expr)) {
        return generateBooleanLiteral(boolLit);
    }
    if (auto* arrayLit = dynamic_cast<const volta::ast::ArrayLiteral*>(expr)) {
        return generateArrayLiteral(arrayLit);
    }
    if (auto* tupleLit = dynamic_cast<const volta::ast::TupleLiteral*>(expr)) {
        return generateTupleLiteral(tupleLit);
    }
    if (auto* structLit = dynamic_cast<const volta::ast::StructLiteral*>(expr)) {
        return generateStructLiteral(structLit);
    }

    // Variables
    if (auto* identifier = dynamic_cast<const volta::ast::IdentifierExpression*>(expr)) {
        return generateIdentifierExpression(identifier);
    }

    // Operators
    if (auto* binExpr = dynamic_cast<const volta::ast::BinaryExpression*>(expr)) {
        return generateBinaryExpression(binExpr);
    }
    if (auto* unaryExpr = dynamic_cast<const volta::ast::UnaryExpression*>(expr)) {
        return generateUnaryExpression(unaryExpr);
    }

    // Calls
    if (auto* callExpr = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        return generateCallExpression(callExpr);
    }
    if (auto* methodCall = dynamic_cast<const volta::ast::MethodCallExpression*>(expr)) {
        return generateMethodCallExpression(methodCall);
    }

    // Access
    if (auto* indexExpr = dynamic_cast<const volta::ast::IndexExpression*>(expr)) {
        return generateIndexExpression(indexExpr);
    }
    if (auto* sliceExpr = dynamic_cast<const volta::ast::SliceExpression*>(expr)) {
        return generateSliceExpression(sliceExpr);
    }
    if (auto* memberExpr = dynamic_cast<const volta::ast::MemberExpression*>(expr)) {
        return generateMemberExpression(memberExpr);
    }

    // Control flow
    if (auto* ifExpr = dynamic_cast<const volta::ast::IfExpression*>(expr)) {
        return generateIfExpression(ifExpr);
    }
    if (auto* matchExpr = dynamic_cast<const volta::ast::MatchExpression*>(expr)) {
        return generateMatchExpression(matchExpr);
    }

    // Advanced
    if (auto* lambdaExpr = dynamic_cast<const volta::ast::LambdaExpression*>(expr)) {
        return generateLambdaExpression(lambdaExpr);
    }
    if (auto* castExpr = dynamic_cast<const volta::ast::CastExpression*>(expr)) {
        return generateCastExpression(castExpr);
    }

    reportError("Unsupported expression type at " + expr->location.toString());
    return nullptr;
}

// ============================================================================
// Expression Generation - Literals
// ============================================================================

Value* LLVMCodegen::generateIntegerLiteral(const volta::ast::IntegerLiteral* lit) {
    auto voltaType = analyzer_->getExpressionType(lit);
    auto* llvmType = lowerType(voltaType);
    return ConstantInt::get(llvmType, lit->value);
}

Value* LLVMCodegen::generateFloatLiteral(const volta::ast::FloatLiteral* lit) {
    auto voltaType = analyzer_->getExpressionType(lit);
    auto* llvmType = lowerType(voltaType);
    return ConstantFP::get(llvmType, lit->value);
}

Value* LLVMCodegen::generateStringLiteral(const volta::ast::StringLiteral* lit) {
    // Create global string constant
    Constant* strConstant = ConstantDataArray::getString(*context_, lit->value, true);

    auto* globalStr = new GlobalVariable(
        *module_,
        strConstant->getType(),
        true,  // isConstant
        GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );

    // Return pointer to string data (i8*)
    return builder_->CreatePointerCast(
        globalStr,
        PointerType::get(*context_, 0)
    );
}

Value* LLVMCodegen::generateBooleanLiteral(const volta::ast::BooleanLiteral* lit) {
    return ConstantInt::get(Type::getInt1Ty(*context_), lit->value ? 1 : 0);
}

Value* LLVMCodegen::generateArrayLiteral(const volta::ast::ArrayLiteral* lit) {
    // TODO: Implement array literals
    // STEPS:
    // 1. Call volta_array_new(capacity) to allocate array
    // 2. For each element: generate expression, call volta_array_push(arr, elem)
    // 3. Return array pointer

    reportError("Array literals not yet implemented at " + lit->location.toString());
    return nullptr;
}

Value* LLVMCodegen::generateTupleLiteral(const volta::ast::TupleLiteral* lit) {
    // TODO: Implement tuple literals
    // Tuples are represented as LLVM structs
    // STEPS:
    // 1. Get tuple type from semantic analyzer
    // 2. Create alloca for tuple struct
    // 3. Generate each element expression
    // 4. Use GEP + store to initialize each field
    // 5. Load and return the struct value

    reportError("Tuple literals not yet implemented at " + lit->location.toString());
    return nullptr;
}

Value* LLVMCodegen::generateStructLiteral(const volta::ast::StructLiteral* lit) {
    // TODO: Implement struct literals
    // Similar to tuples but with named fields
    // STEPS:
    // 1. Allocate struct (GC or stack)
    // 2. For each field initialization: generate expression, store to field
    // 3. Return struct pointer

    reportError("Struct literals not yet implemented at " + lit->location.toString());
    return nullptr;
}

// ============================================================================
// Expression Generation - Variables
// ============================================================================

Value* LLVMCodegen::generateIdentifierExpression(const volta::ast::IdentifierExpression* expr) {
    auto it = namedValues_.find(expr->name);
    if (it == namedValues_.end()) {
        reportError("Unknown variable: " + expr->name + " at " + expr->location.toString());
        return nullptr;
    }

    Value* val = it->second;

    auto* alloca = llvm::cast<AllocaInst>(it->second);
    return builder_->CreateLoad(alloca->getAllocatedType(), alloca, expr->name);
}

// ============================================================================
// Expression Generation - Operators
// ============================================================================

Value* LLVMCodegen::generateBinaryExpression(const volta::ast::BinaryExpression* expr) {
    Value* lhs = generateExpression(expr->left.get());
    Value* rhs = generateExpression(expr->right.get());

    if (!lhs || !rhs) return nullptr;

    auto lhsType = analyzer_->getExpressionType(expr->left.get());
    auto rhsType = analyzer_->getExpressionType(expr->right.get());
    bool isSigned = lhsType->isSignedInteger();

    // Promote to common type
    Type* targetType = getPromotedType(lhs->getType(), rhs->getType());
    lhs = convertToType(lhs, targetType, lhsType->isSignedInteger());
    rhs = convertToType(rhs, targetType, rhsType->isSignedInteger());

    bool isFloat = targetType->isFloatingPointTy();

    using Op = ast::BinaryExpression::Operator;
    switch(expr->op) {
        // Arithmetic
        case Op::Add:
            return isFloat ? builder_->CreateFAdd(lhs, rhs, "faddTmp")
                          : builder_->CreateAdd(lhs, rhs, "addTmp");
        case Op::Subtract:
            return isFloat ? builder_->CreateFSub(lhs, rhs, "fsubTmp")
                          : builder_->CreateSub(lhs, rhs, "subTmp");
        case Op::Multiply:
            return isFloat ? builder_->CreateFMul(lhs, rhs, "fmulTmp")
                          : builder_->CreateMul(lhs, rhs, "mulTmp");
        case Op::Divide:
            if (isFloat) return builder_->CreateFDiv(lhs, rhs, "fdivTmp");
            return isSigned ? builder_->CreateSDiv(lhs, rhs, "sdivTmp")
                            : builder_->CreateUDiv(lhs, rhs, "udivTmp");
        case Op::Modulo:
            if (isFloat) return builder_->CreateFRem(lhs, rhs, "fremTmp");
            return isSigned ? builder_->CreateSRem(lhs, rhs, "sremTmp")
                            : builder_->CreateURem(lhs, rhs, "uremTmp");

        // Comparisons
        case Op::Equal:
            if (isFloat) return builder_->CreateFCmpOEQ(lhs, rhs, "fcmpEqTmp");
            if (lhsType->kind() == semantic::Type::Kind::String) {
                Function* strEqFn = runtime_->getStringEq();
                return builder_->CreateCall(strEqFn, {lhs, rhs}, "strEqTmp");
            }
            return builder_->CreateICmpEQ(lhs, rhs, "icmpEqTmp");

        case Op::NotEqual:
            if (isFloat) return builder_->CreateFCmpONE(lhs, rhs, "fcmpNeTmp");
            if (lhsType->kind() == semantic::Type::Kind::String) {
                Function* strEqFn = runtime_->getStringEq();
                Value* eq = builder_->CreateCall(strEqFn, {lhs, rhs}, "strEqTmp");
                return builder_->CreateNot(eq, "strNeTmp");
            }
            return builder_->CreateICmpNE(lhs, rhs, "icmpNeTmp");

        case Op::Less:
            if (isFloat) return builder_->CreateFCmpOLT(lhs, rhs, "fcmpLtTmp");
            if (lhsType->kind() == semantic::Type::Kind::String) {
                Function* strCmpFn = runtime_->getStringCmp();
                Value* cmpResult = builder_->CreateCall(strCmpFn, {lhs, rhs}, "strCmpTmp");
                return builder_->CreateICmpSLT(cmpResult,
                                               ConstantInt::get(Type::getInt32Ty(*context_), 0),
                                               "strLtTmp");
            }
            return isSigned ? builder_->CreateICmpSLT(lhs, rhs, "icmpSltTmp")
                            : builder_->CreateICmpULT(lhs, rhs, "icmpUltTmp");

        case Op::LessEqual:
            if (isFloat) return builder_->CreateFCmpOLE(lhs, rhs, "fcmpLeTmp");
            if (lhsType->kind() == semantic::Type::Kind::String) {
                Function* strCmpFn = runtime_->getStringCmp();
                Value* cmpResult = builder_->CreateCall(strCmpFn, {lhs, rhs}, "strCmpTmp");
                return builder_->CreateICmpSLE(cmpResult,
                                               ConstantInt::get(Type::getInt32Ty(*context_), 0),
                                               "strLeTmp");
            }
            return isSigned ? builder_->CreateICmpSLE(lhs, rhs, "icmpSleTmp")
                            : builder_->CreateICmpULE(lhs, rhs, "icmpUleTmp");

        case Op::Greater:
            if (isFloat) return builder_->CreateFCmpOGT(lhs, rhs, "fcmpGtTmp");
            if (lhsType->kind() == semantic::Type::Kind::String) {
                Function* strCmpFn = runtime_->getStringCmp();
                Value* cmpResult = builder_->CreateCall(strCmpFn, {lhs, rhs}, "strCmpTmp");
                return builder_->CreateICmpSGT(cmpResult,
                                               ConstantInt::get(Type::getInt32Ty(*context_), 0),
                                               "strGtTmp");
            }
            return isSigned ? builder_->CreateICmpSGT(lhs, rhs, "icmpSgtTmp")
                            : builder_->CreateICmpUGT(lhs, rhs, "icmpUgtTmp");

        case Op::GreaterEqual:
            if (isFloat) return builder_->CreateFCmpOGE(lhs, rhs, "fcmpGeTmp");
            if (lhsType->kind() == semantic::Type::Kind::String) {
                Function* strCmpFn = runtime_->getStringCmp();
                Value* cmpResult = builder_->CreateCall(strCmpFn, {lhs, rhs}, "strCmpTmp");
                return builder_->CreateICmpSGE(cmpResult,
                                               ConstantInt::get(Type::getInt32Ty(*context_), 0),
                                               "strGeTmp");
            }
            return isSigned ? builder_->CreateICmpSGE(lhs, rhs, "icmpSgeTmp")
                            : builder_->CreateICmpUGE(lhs, rhs, "icmpUgeTmp");

        // TODO: Implement logical operators (&&, ||) - they need short-circuit evaluation!
        // TODO: Implement assignment operators (=, +=, -=, etc.)
        case Op::Assign: {
                // TODO: left must be an identifier for now, but this needs to be extended!!
                auto* identExpr = dynamic_cast<const volta::ast::IdentifierExpression*>(expr->left.get());
                if (!identExpr) {
                    reportError("Left side of assignment must be a variable");
                    return nullptr;
                }
                
                // Look up the variable's alloca
                auto it = namedValues_.find(identExpr->name);
                if (it == namedValues_.end()) {
                    reportError("Unknown variable: " + identExpr->name);
                    return nullptr;
                }
                
                auto* varPtr = it->second;
                Value* rhs = generateExpression(expr->right.get());
                builder_->CreateStore(rhs, varPtr);
                
                // Assignment expressions return the assigned value
                return rhs;
            }
        default:
            reportError("Unsupported binary operator at " + expr->location.toString());
            return nullptr;
    }
}

Value* LLVMCodegen::generateUnaryExpression(const volta::ast::UnaryExpression* expr) {
    // TODO: Implement unary operators
    // OPERATORS:
    //   - Negate: -x → sub 0, x (for integers), fneg x (for floats)
    //   - Not: !x → xor x, 1 (for booleans)

    reportError("Unary expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

// ============================================================================
// Expression Generation - Calls
// ============================================================================

Value* LLVMCodegen::generateCallExpression(const volta::ast::CallExpression* expr) {
    auto* callee = dynamic_cast<const volta::ast::IdentifierExpression*>(expr->callee.get());
    if (!callee) {
        reportError("Complex callee not supported yet at " + expr->location.toString());
        return nullptr;
    }

    std::string fnName = callee->name;

    // Handle builtins
    if (fnName == "print") {
        return generatePrintCall(expr);
    }

    // Look up user-defined function
    Function* fn = module_->getFunction(fnName);
    if (!fn) {
        reportError("Unknown function: " + fnName + " at " + expr->location.toString());
        return nullptr;
    }

    // Generate arguments
    std::vector<Value*> args;
    for (const auto& arg : expr->arguments) {
        Value* argValue = generateExpression(arg.get());
        if (!argValue) return nullptr;
        args.push_back(argValue);
    }

    return builder_->CreateCall(fn, args);
}

Value* LLVMCodegen::generateMethodCallExpression(const volta::ast::MethodCallExpression* expr) {
    // TODO: Implement method calls
    // STEPS:
    // 1. Generate the object expression
    // 2. Look up the method (it's a function with receiver as first param)
    // 3. Generate arguments
    // 4. Call with object as first argument

    reportError("Method calls not yet implemented at " + expr->location.toString());
    return nullptr;
}

// ============================================================================
// Expression Generation - Access
// ============================================================================

Value* LLVMCodegen::generateIndexExpression(const volta::ast::IndexExpression* expr) {
    // TODO: Implement indexing (array[i], tuple.0, etc.)
    // For arrays:
    //   1. Generate array expression
    //   2. Generate index expression
    //   3. Bounds check (optional for now)
    //   4. GEP into array data: getelementptr %arr.data, %index
    //   5. Load element

    reportError("Index expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

Value* LLVMCodegen::generateSliceExpression(const volta::ast::SliceExpression* expr) {
    // TODO: Implement slicing (array[1:5], string[2:], etc.)
    // Call runtime function volta_array_slice(arr, start, end)

    reportError("Slice expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

Value* LLVMCodegen::generateMemberExpression(const volta::ast::MemberExpression* expr) {
    // TODO: Implement member access (struct.field)
    // STEPS:
    // 1. Generate object expression
    // 2. Look up field index from semantic analyzer
    // 3. GEP to field: getelementptr %struct, 0, %field_index
    // 4. Load field value

    reportError("Member expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

// ============================================================================
// Expression Generation - Control Flow
// ============================================================================

Value* LLVMCodegen::generateIfExpression(const volta::ast::IfExpression* expr) {
    // TODO: Implement if expressions
    // Similar to if statement but returns a value
    // Need to create phi node to merge results from branches

    reportError("If expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

Value* LLVMCodegen::generateMatchExpression(const volta::ast::MatchExpression* expr) {
    // TODO: Implement match expressions
    // Complex! Pattern matching with exhaustiveness checking
    // Can desugar to cascading if-else for now

    reportError("Match expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

// ============================================================================
// Expression Generation - Advanced
// ============================================================================

Value* LLVMCodegen::generateLambdaExpression(const volta::ast::LambdaExpression* expr) {
    // TODO: Implement lambdas
    // Very complex! Need closures if capturing variables

    reportError("Lambda expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

Value* LLVMCodegen::generateCastExpression(const volta::ast::CastExpression* expr) {
    // TODO: Implement casts
    // Use existing convertToType function

    reportError("Cast expressions not yet implemented at " + expr->location.toString());
    return nullptr;
}

// ============================================================================
// Type System
// ============================================================================

Type* LLVMCodegen::lowerType(std::shared_ptr<volta::semantic::Type> voltaType) {
    using Kind = volta::semantic::Type::Kind;

    switch (voltaType->kind()) {
        case Kind::I8:  return Type::getInt8Ty(*context_);
        case Kind::I16: return Type::getInt16Ty(*context_);
        case Kind::I32: return Type::getInt32Ty(*context_);
        case Kind::I64: return Type::getInt64Ty(*context_);
        case Kind::U8:  return Type::getInt8Ty(*context_);
        case Kind::U16: return Type::getInt16Ty(*context_);
        case Kind::U32: return Type::getInt32Ty(*context_);
        case Kind::U64: return Type::getInt64Ty(*context_);
        case Kind::F32: return Type::getFloatTy(*context_);
        case Kind::F64: return Type::getDoubleTy(*context_);
        case Kind::Bool: return Type::getInt1Ty(*context_);
        case Kind::String: return PointerType::get(*context_, 0);
        case Kind::Void: return Type::getVoidTy(*context_);
        case Kind::Array: return PointerType::get(*context_, 0);

        // TODO: Implement struct types, function types, option types
        default:
            reportError("Unsupported type: " + voltaType->toString());
            return Type::getVoidTy(*context_);
    }
}

Type* LLVMCodegen::getPromotedType(Type* t1, Type* t2) {
    if (t1 == t2) return t1;

    // Float promotion
    if (t1->isDoubleTy() || t2->isDoubleTy()) return Type::getDoubleTy(*context_);
    if (t1->isFloatTy() || t2->isFloatTy()) return Type::getFloatTy(*context_);

    // Integer promotion: use larger type
    if (t1->isIntegerTy() && t2->isIntegerTy()) {
        unsigned width1 = t1->getIntegerBitWidth();
        unsigned width2 = t2->getIntegerBitWidth();
        return width1 >= width2 ? t1 : t2;
    }

    return t1;
}

Value* LLVMCodegen::convertToType(Value* val, Type* targetType, bool isSigned) {
    Type* srcType = val->getType();
    if (srcType == targetType) return val;

    // Integer conversions
    if (srcType->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned srcWidth = srcType->getIntegerBitWidth();
        unsigned targetWidth = targetType->getIntegerBitWidth();

        if (srcWidth < targetWidth) {
            return isSigned ? builder_->CreateSExt(val, targetType)
                            : builder_->CreateZExt(val, targetType);
        } else if (srcWidth > targetWidth) {
            return builder_->CreateTrunc(val, targetType);
        }
    }

    // Int to float
    if (srcType->isIntegerTy() && targetType->isFloatingPointTy()) {
        return isSigned ? builder_->CreateSIToFP(val, targetType)
                        : builder_->CreateUIToFP(val, targetType);
    }

    // Float to float
    if (srcType->isFloatingPointTy() && targetType->isFloatingPointTy()) {
        return builder_->CreateFPCast(val, targetType);
    }

    return val;
}

llvm::AllocaInst* LLVMCodegen::createEntryBlockAlloca(llvm::Function* fn, std::string varName, llvm::Type* type) {
    return allocaBuilder_->CreateAlloca(type, nullptr, varName);
}

// ============================================================================
// Runtime Function Declarations
// ============================================================================
// NOTE: Runtime functions are now managed by the RuntimeFunctions class.
// They are accessed via runtime_->getPrintInt(), runtime_->getStringEq(), etc.

// ============================================================================
// Builtin Function Helpers
// ============================================================================

Value* LLVMCodegen::generatePrintCall(const volta::ast::CallExpression* call) {
    if (call->arguments.size() != 1) {
        reportError("print() expects exactly 1 argument");
        return nullptr;
    }

    Value* arg = generateExpression(call->arguments[0].get());
    if (!arg) return nullptr;

    Type* argType = arg->getType();
    Function* printFunc = nullptr;

    if (argType->isIntegerTy()) {
        auto voltaType = analyzer_->getExpressionType(call->arguments[0].get());
        if (voltaType->isSignedInteger()) {
            arg = builder_->CreateSExt(arg, Type::getInt64Ty(*context_));
        } else if (voltaType->isUnsignedInteger()) {
            if (!arg->getType()->isIntegerTy(64)) {
                arg = builder_->CreateZExt(arg, Type::getInt64Ty(*context_));
            }
        }
        printFunc = runtime_->getPrintInt();
    } else if (argType->isFloatTy()) {
        arg = builder_->CreateFPExt(arg, Type::getDoubleTy(*context_));
        printFunc = runtime_->getPrintFloat();
    } else if (argType->isDoubleTy()) {
        printFunc = runtime_->getPrintFloat();
    } else if (argType->isPointerTy()) {
        printFunc = runtime_->getPrintString();
    } else {
        reportError("Unsupported type for print()");
        return nullptr;
    }

    return builder_->CreateCall(printFunc, {arg});
}

// ============================================================================
// Error Reporting
// ============================================================================

void LLVMCodegen::reportError(const std::string& message) {
    errors_.push_back(message);
    hasErrors_ = true;
    llvm::errs() << "Codegen error: " << message << "\n";
}

} // namespace volta::llvm_codegen
