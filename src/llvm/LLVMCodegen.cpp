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
#include "LLVMCodegen.hpp"

using namespace llvm;

namespace volta::llvm_codegen {

LLVMCodegen::LLVMCodegen(const std::string& moduleName)
    : context_(std::make_unique<LLVMContext>()),
      module_(std::make_unique<Module>(moduleName, *context_)),
      builder_(std::make_unique<IRBuilder<>>(*context_)),
      currentFunction_(nullptr),
      analyzer_(nullptr),
      hasErrors_(false) {
    // Set the target triple to match the host system
    module_->setTargetTriple(sys::getDefaultTargetTriple());
}

LLVMCodegen::~LLVMCodegen() = default;

bool LLVMCodegen::generate(
    const volta::ast::Program* program,
    const volta::semantic::SemanticAnalyzer* analyzer) {
    if (!program || !analyzer) {
        reportError("Null program or analyzer");
        return false;
    }

    analyzer_ = analyzer;

    for (const auto& stmt : program->statements) {
        // TODO: Dispatch other top-level statement
        if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt.get())) {
            generateFunction(fnDecl);
        }
    }

    std::string errorMsg; 
    raw_string_ostream errorStream(errorMsg);

    if (verifyModule(*module_, &errorStream))  {
        reportError("Module verificaiton failed: " + errorMsg);
        return false;
    }

    return !hasErrors_;
}

bool LLVMCodegen::emitLLVMIR(const std::string& filename) {
    std::error_code ec;
    raw_fd_ostream dest(filename, ec, sys::fs::OF_None);

    if (ec) {
        reportError("Could not open file: " + ec.message());
        return false;
    }

    module_->print(dest, nullptr);
    return true;
}

bool LLVMCodegen::compileToObject(const std::string &filename) {
    return false;
}

void LLVMCodegen::generateFunction(const volta::ast::FnDeclaration *fn) {
    std::string fnName = fn->identifier;

    auto semReturnType = analyzer_->resolveTypeAnnotation(fn->returnType.get());
    Type* returnType = lowerType(semReturnType);

    std::vector<Type*> paramTypes;
    for (const auto& param : fn->parameters) {
        auto semType = analyzer_->resolveTypeAnnotation(param->type.get());
        paramTypes.push_back(lowerType(semType));
    }

    // TODO: Add support for variadic arguments
    FunctionType* fnType = FunctionType::get(returnType, paramTypes, false);

    Function* llvmFn = Function::Create(
        fnType, 
        Function::ExternalLinkage,
        fnName,
        module_.get()
    );

    functions_[fnName] = llvmFn;

    BasicBlock* entry = BasicBlock::Create(*context_, "entry_"+fnName, llvmFn);
    builder_->SetInsertPoint(entry);

    currentFunction_ = llvmFn;
    namedValues_.clear();

    for (auto& arg : llvmFn->args()) {
        namedValues_[std::string(arg.getName())] = &arg;
    }

    if (fn->body) {
        for (const auto& stmt : fn->body->statements) {
            generateStatement(stmt.get());
        }
    }

    // If no return was generated and function returns void, add implicit return
    if (!builder_->GetInsertBlock()->getTerminator()) {
        if (returnType->isVoidTy()) {
            builder_->CreateRetVoid();
        } else {
            // TODO: This should be handled by the semantic analysis!!
            reportError("Function '" + fnName + "' missing return statement");
        }
    }

    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    if (verifyFunction(*llvmFn, &errorStream)) {
        reportError("Function verification failed: " + errorMsg);
        llvmFn->eraseFromParent();  
    }

    currentFunction_ = nullptr;
}

llvm::Value *LLVMCodegen::generateExpression(const volta::ast::Expression *expr) {
    if (auto* call = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        return generateCallExpression(call);
    }
    if (auto* strLit = dynamic_cast<const volta::ast::StringLiteral*>(expr)) {
        return generateStringLiteral(strLit);
    }

    // Primitive types
    if (auto* intLit = dynamic_cast<const volta::ast::IntegerLiteral*>(expr)) {
        auto voltaType = analyzer_->getExpressionType(expr);
        auto* llvmType = lowerType(voltaType);

        return ConstantInt::get(llvmType, intLit->value);
    }
    if (auto* floatLit = dynamic_cast<const volta::ast::FloatLiteral*>(expr)) {
        auto voltaType = analyzer_->getExpressionType(expr);
        auto* llvmType = lowerType(voltaType);

        return ConstantFP::get(llvmType, floatLit->value);
    }

    reportError("Unsupported expression type");
    return nullptr;
}

llvm::Value *LLVMCodegen::generateBinaryExpression(const volta::ast::BinaryExpression *expr) {
    Value* lhs = generateExpression(expr->left.get());
    Value* rhs = generateExpression(expr->right.get());

    if (!lhs || !rhs) return nullptr;

    // Get types from semantic analyzer
    auto lhsType = analyzer_->getExpressionType(expr->left.get());
    auto rhsType = analyzer_->getExpressionType(expr->right.get());
    bool isSigned = lhsType->isSignedInteger();

    // Promote to common type
    Type* targetType = getPromotedType(lhs->getType(), rhs->getType());
    lhs = convertToType(lhs, targetType, lhsType->isSignedInteger());
    rhs = convertToType(rhs, targetType, rhsType->isSignedInteger());

    bool isFloat = targetType->isFloatingPointTy();

    switch(expr->op) {
        case ast::BinaryExpression::Operator::Add:
            return isFloat ? builder_->CreateFAdd(lhs, rhs, "faddTmp") 
                        : builder_->CreateAdd(lhs, rhs, "addTmp");
        
        case ast::BinaryExpression::Operator::Subtract:
            return isFloat ? builder_->CreateFSub(lhs, rhs, "fsubTmp") 
                        : builder_->CreateSub(lhs, rhs, "subTmp");
        
        case ast::BinaryExpression::Operator::Multiply:
            return isFloat ? builder_->CreateFMul(lhs, rhs, "fmulTmp") 
                        : builder_->CreateMul(lhs, rhs, "mulTmp");
        
        case ast::BinaryExpression::Operator::Divide:
            if (isFloat) return builder_->CreateFDiv(lhs, rhs, "fdivTmp");
            return isSigned ? builder_->CreateSDiv(lhs, rhs, "sdivTmp")
                            : builder_->CreateUDiv(lhs, rhs, "udivTmp");
        
        case ast::BinaryExpression::Operator::Modulo:
            if (isFloat) return builder_->CreateFRem(lhs, rhs, "fremTmp");
            return isSigned ? builder_->CreateSRem(lhs, rhs, "sremTmp")
                            : builder_->CreateURem(lhs, rhs, "uremTmp");
        
        default:
            return nullptr;
    }
}
llvm::Value *LLVMCodegen::generateUnaryExpression(const volta::ast::UnaryExpression *expr) {
    return nullptr;
}

Value *LLVMCodegen::generateCallExpression(const volta::ast::CallExpression *call)
{
    auto* callee = dynamic_cast<const volta::ast::IdentifierExpression*>(call->callee.get());
    if (!callee) {
        reportError("Complex callee not supported yet");
        return nullptr;
    }

    std::string fnName = callee->name;

    // TODO: Add proper support for builtins.
    if (fnName == "print") {
        return generatePrintCall(call);
    }

    Function* fn = module_->getFunction(fnName);
    if (!fn) {
        reportError("Unknown function: " + fnName);
        return nullptr;
    }

    // Args generation
    std::vector<Value*> args;
    for (const auto& arg : call->arguments) {
        Value* argValue = generateExpression(arg.get());
        if (!argValue) return nullptr;
        args.push_back(argValue);
    }

    return builder_->CreateCall(fn, args);
}

Value* LLVMCodegen::generatePrintCall(const volta::ast::CallExpression* call) {
    // TODO: Semantic analysis should prevent this from happening, maybe?
    // or we just dont print anything.
    if (call->arguments.size() != 1) {
        reportError("print() expects exactly 1 argument");
        return nullptr;
    }

    Value* arg = generateExpression(call->arguments[0].get());
    if (!arg) return nullptr; // We dont report an error as it might have already been done before.

    Type* argType = arg->getType();
    Function* printFunc = nullptr;

    if (argType->isIntegerTy()) {
        auto voltaType = analyzer_->getExpressionType(call->arguments[0].get());
        if (voltaType->isSignedInteger()) {
            arg = builder_->CreateSExt(arg, Type::getInt64Ty(*context_));
        }
        else if (voltaType->isUnsignedInteger()) {
            if (!arg->getType()->isIntegerTy(64)) {
                arg = builder_->CreateZExt(arg, Type::getInt64Ty(*context_));
            }
        }
        printFunc = getPrintIntFunction();
    } else if (argType->isFloatTy()) {
        arg = builder_->CreateFPExt(arg, Type::getDoubleTy(*context_));
    } else if (argType->isDoubleTy()) {
        printFunc = getPrintDoubleFunction();
    }else if (argType->isPointerTy()) {
        // Assume string for now
        printFunc = getPrintStringFunction();
    } else {
        reportError("Unsupported type for print()");
        return nullptr;
    }

    return builder_->CreateCall(printFunc, {arg});

}

Function* LLVMCodegen::getPrintIntFunction() {
    Function* fn = module_->getFunction("volta_print_int");

    if(!fn) {
        FunctionType* ft = FunctionType::get(
            Type::getVoidTy(*context_),
            {Type::getInt64Ty(*context_)},
            false
        );
        fn = Function::Create(ft, Function::ExternalLinkage, "volta_print_int", module_.get());
    }

    return fn;
}

Function* LLVMCodegen::getPrintDoubleFunction() {
    Function* fn = module_->getFunction("volta_print_double");
    
    if(!fn) {
        FunctionType* ft = FunctionType::get(
            Type::getVoidTy(*context_),
            {Type::getDoubleTy(*context_)},
            false
        );
        fn = Function::Create(ft, Function::ExternalLinkage, "volta_print_double", module_.get());
    }

    return fn;
}

Function* LLVMCodegen::getPrintStringFunction() {
    Function* f = module_->getFunction("volta_print_string");
    if (!f) {
        FunctionType* ft = FunctionType::get(
            Type::getVoidTy(*context_),
            {PointerType::get(*context_, 0)},
            false
        );
        f = Function::Create(ft, Function::ExternalLinkage, "volta_print_string", module_.get());
    }
    return f;
}

Value* LLVMCodegen::generateStringLiteral(const volta::ast::StringLiteral* lit) {
    Constant* strConstant = ConstantDataArray::getString(*context_, lit->value, true);

    GlobalVariable* globalStr = new GlobalVariable(
        *module_,
        strConstant->getType(),
        true,  // isConstant
        GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );
    
    // Get pointer to first element (char*)
    Value* zero = ConstantInt::get(Type::getInt32Ty(*context_), 0);
    return builder_->CreateInBoundsGEP(
        strConstant->getType(),
        globalStr,
        {zero, zero}
    );
}

void LLVMCodegen::generateStatement(const volta::ast::Statement *stmt) {
    if (auto* retStmt = dynamic_cast<const volta::ast::ReturnStatement*>(stmt)) {
        generateReturn(retStmt);
    }
    if (auto* exprStmt = dynamic_cast<const volta::ast::ExpressionStatement*>(stmt)) {
        generateExpression(exprStmt->expr.get());
    }
    
}



void LLVMCodegen::generateReturn(const volta::ast::ReturnStatement* stmt) {
    if (stmt->expression) {
        auto exprValue = generateExpression(stmt->expression.get());
        builder_->CreateRet(exprValue);
    } else {
        builder_->CreateRetVoid();
    }
}

llvm::Type *LLVMCodegen::lowerType(
    std::shared_ptr<volta::semantic::Type> voltaType) {
    
    if (!voltaType) return Type::getVoidTy(*context_);
    
    switch (voltaType->kind()) {
        case volta::semantic::Type::Kind::I8:
        case volta::semantic::Type::Kind::U8: 
            return Type::getInt8Ty(*context_);
        case volta::semantic::Type::Kind::I16:
        case volta::semantic::Type::Kind::U16: 
            return Type::getInt16Ty(*context_);
        case volta::semantic::Type::Kind::I32:
        case volta::semantic::Type::Kind::U32: 
            return Type::getInt32Ty(*context_);
        case volta::semantic::Type::Kind::I64:
        case volta::semantic::Type::Kind::U64: 
            return Type::getInt64Ty(*context_);
        // LLVM does not support F8 types, so we go for 2 bytes
        case volta::semantic::Type::Kind::F8: 
        case volta::semantic::Type::Kind::F16:
            return Type::getHalfTy(*context_);
        case volta::semantic::Type::Kind::F32:
            return Type::getFloatTy(*context_);
        case volta::semantic::Type::Kind::F64:
            return Type::getDoubleTy(*context_);
        case volta::semantic::Type::Kind::Bool:
            return Type::getInt1Ty(*context_);
        case volta::semantic::Type::Kind::String:
            return PointerType::get(*context_, 0);
        case volta::semantic::Type::Kind::Void:
            return Type::getVoidTy(*context_);
        case volta::semantic::Type::Kind::Array: 
            return PointerType::get(*context_, 0);
        case volta::semantic::Type::Kind::Struct:
            return PointerType::get(*context_, 0);
        case volta::semantic::Type::Kind::Enum:
            return PointerType::get(*context_, 0);
        default:
            reportError("Unsupported type in type lowering");
            return Type::getVoidTy(*context_);
    }
    
    return nullptr;
}


void LLVMCodegen::reportError(const std::string &message) {
    std::cerr << "Codegen error: " << message << "\n";
    hasErrors_ = true;
}


llvm::Type* LLVMCodegen::getPromotedType(llvm::Type* t1, llvm::Type* t2) {
    // Float hierarchy: double > float > integers
    if (t1->isDoubleTy() || t2->isDoubleTy()) return Type::getDoubleTy(*context_);
    if (t1->isFloatTy() || t2->isFloatTy()) return Type::getFloatTy(*context_);
    
    // Integer hierarchy: larger bit width wins
    if (t1->isIntegerTy() && t2->isIntegerTy()) {
        unsigned w1 = t1->getIntegerBitWidth();
        unsigned w2 = t2->getIntegerBitWidth();
        return w1 >= w2 ? t1 : t2;
    }
    
    return t1; // fallback
}


llvm::Value* LLVMCodegen::convertToType(llvm::Value* val, llvm::Type* targetType, bool isSigned) {
    Type* srcType = val->getType();
    if (srcType == targetType) return val;
    
    // Int -> Float/Double
    if (srcType->isIntegerTy() && targetType->isFloatingPointTy()) {
        return isSigned ? 
            builder_->CreateSIToFP(val, targetType) :
            builder_->CreateUIToFP(val, targetType);
    }
    
    // Float -> Double or Double -> Float
    if (srcType->isFloatingPointTy() && targetType->isFloatingPointTy()) {
        unsigned srcBits = srcType->getPrimitiveSizeInBits();
        unsigned dstBits = targetType->getPrimitiveSizeInBits();
        return srcBits < dstBits ?
            builder_->CreateFPExt(val, targetType) :
            builder_->CreateFPTrunc(val, targetType);
    }
    
    // Int -> Int (different widths)
    if (srcType->isIntegerTy() && targetType->isIntegerTy()) {
        unsigned srcBits = srcType->getIntegerBitWidth();
        unsigned dstBits = targetType->getIntegerBitWidth();
        if (srcBits < dstBits) {
            return isSigned ? 
                builder_->CreateSExt(val, targetType) :
                builder_->CreateZExt(val, targetType);
        } else {
            return builder_->CreateTrunc(val, targetType);
        }
    }
    
    return val;

}

}