#pragma once
#include "HIR/HIR.hpp"
#include "Semantic/TypeRegistry.hpp"
#include "Error/Error.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#include <map>

using namespace Semantic;

class Codegen {
private:
private:
    const Program& hirProgram;
    const TypeRegistry& typeRegistry;
    DiagnosticManager& diag;
    
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    std::map<std::string, std::pair<llvm::Value*, llvm::Type*>> variables;
    std::map<std::string, const FnDecl*> functionDecls;
    const Type* currentFunctionReturnType;

    struct LoopContext {
        llvm::BasicBlock* continueTarget; 
        llvm::BasicBlock* breakTarget;    
    };
    std::vector<LoopContext> loopStack;

public:
    Codegen(const Program& hirProgram, const TypeRegistry& typeRegistry,
            DiagnosticManager& diag):
        hirProgram(hirProgram), typeRegistry(typeRegistry),
        diag(diag) {}
    
    llvm::Module*  generate();

private:
    void generateStmt(const Stmt* stmt);

    void generateFnDecl(const FnDecl* stmt);
    void generateVarDecl(const VarDecl* stmt);

    void generateHIRReturn(const HIR::HIRReturnStmt* stmt);
    void generateHIRIfStmt(const HIR::HIRIfStmt* stmt);
    void generateHIRWhileStmt(const HIR::HIRWhileStmt* stmt);
    void generateHIRBlockStmt(const HIR::HIRBlockStmt* stmt);
    void generateHIRBreak(const HIR::HIRBreakStmt* stmt);
    void generateHIRContinue(const HIR::HIRContinueStmt* stmt);
    void generateHIRExprStmt(const HIR::HIRExprStmt* stmt);


    llvm::Value* generateExpr(const Expr* expr, const Type* expectedType = nullptr);
    llvm::Value* generateLiteral(const Literal* expr, const Type* expectedType = nullptr);
    llvm::Value* generateVariable(const Variable* expr);
    llvm::Value* generateFnCall(const FnCall* expr);
    llvm::Value* generateBinaryExpr(const BinaryExpr* expr, const Type* expectedType = nullptr);
    llvm::Value* generateUnaryExpr(const UnaryExpr* expr, const Type* expectedType = nullptr);
    llvm::Value* generateAssignment(const Assignment* expr);
    llvm::Value* generateArrayLiteral(const ArrayLiteral* expr);
    llvm::Value* generateIndexExpr(const IndexExpr* expr);


    void fillArrayLiteral(llvm::Value* arrayPtr, llvm::Type* arrayType, const ArrayLiteral* expr);
};