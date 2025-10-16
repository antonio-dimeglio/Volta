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
    
    // Map variable name to (pointer, element type)
    std::map<std::string, std::pair<llvm::Value*, llvm::Type*>> variables;
    std::map<std::string, const FnDecl*> functionDecls;
    const Type* currentFunctionReturnType;

    // Loop control: track current loop's continue and break targets
    llvm::BasicBlock* currentLoopContinue = nullptr;
    llvm::BasicBlock* currentLoopBreak = nullptr;

public:
    Codegen(const Program& hirProgram, const TypeRegistry& typeRegistry,
            DiagnosticManager& diag):
        hirProgram(hirProgram), typeRegistry(typeRegistry),
        diag(diag) {}
    
    llvm::Module*  generate();

private:
    // Statement generators
    void generateStmt(const Stmt* stmt);
    void generateFnDecl(const FnDecl* stmt);
    void generateReturn(const ReturnStmt* stmt);
    void generateIfStmt(const IfStmt* stmt);
    void generateWhileStmt(const WhileStmt* stmt);
    void generateBlockStmt(const BlockStmt* stmt);
    void generateBreak(const BreakStmt* stmt);
    void generateContinue(const ContinueStmt* stmt);
    void generateVarDecl(const VarDecl* stmt);
    void generateExprStmt(const ExprStmt* stmt);

    // Expression generators (return llvm::Value*)
    llvm::Value* generateExpr(const Expr* expr, const Type* expectedType = nullptr);
    llvm::Value* generateLiteral(const Literal* expr, const Type* expectedType = nullptr);
    llvm::Value* generateVariable(const Variable* expr);
    llvm::Value* generateFnCall(const FnCall* expr);
    llvm::Value* generateBinaryExpr(const BinaryExpr* expr, const Type* expectedType = nullptr);
    llvm::Value* generateUnaryExpr(const UnaryExpr* expr, const Type* expectedType = nullptr);
    llvm::Value* generateAssignment(const Assignment* expr);
    llvm::Value* generateArrayLiteral(const ArrayLiteral* expr);
    llvm::Value* generateIndexExpr(const IndexExpr* expr);

    // Helper function to fill an array with values from an array literal
    void fillArrayLiteral(llvm::Value* arrayPtr, llvm::Type* arrayType, const ArrayLiteral* expr);
};