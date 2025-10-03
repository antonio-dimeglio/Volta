#pragma once

#include "IRModule.hpp"
#include "IRBuilder.hpp"
#include "SSABuilder.hpp"
#include "../ast/Statement.hpp"
#include "../ast/Expression.hpp"
#include "../semantic/SemanticAnalyzer.hpp"
#include "../error/ErrorReporter.hpp"
#include <memory>
#include <unordered_map>

namespace volta::ir {

// ============================================================================
// IRGenerator - Converts AST to IR
// ============================================================================

/**
 * IRGenerator walks the semantically-analyzed AST and generates IR.
 *
 * This is Phase 4 - you'll implement this AFTER completing:
 * - Phase 1: Value, Instruction, BasicBlock
 * - Phase 2: IRBuilder
 * - Phase 3: SSABuilder
 *
 * Process:
 * 1. Create IRModule
 * 2. Generate global variables
 * 3. Generate function declarations
 * 4. Generate function bodies (expressions + statements)
 * 5. Use SSABuilder for proper SSA form
 *
 * Example usage:
 *   IRGenerator generator(errorReporter);
 *   std::unique_ptr<IRModule> module = generator.generate(ast, analyzer);
 *   if (module) {
 *       // IR generated successfully
 *       std::cout << module->toString() << std::endl;
 *   }
 */
class IRGenerator {
public:
    explicit IRGenerator(volta::errors::ErrorReporter& reporter);

    /**
     * Generate IR from AST.
     * Returns nullptr on error.
     */
    std::unique_ptr<IRModule> generate(
        const volta::ast::Program& program,
        const volta::semantic::SemanticAnalyzer& analyzer
    );

private:
    // ========== Top-Level Generation ==========

    void generateProgram(const volta::ast::Program& program);
    void generateGlobalVariable(const volta::ast::VarDeclaration& varDecl);
    void generateFunction(const volta::ast::FnDeclaration& fnDecl);
    void generateStruct(const volta::ast::StructDeclaration& structDecl);

    // ========== Statement Generation ==========

    void generateStatement(const volta::ast::Statement* stmt);
    void generateBlock(const volta::ast::Block* block);
    void generateVarDeclaration(const volta::ast::VarDeclaration* varDecl);
    void generateIfStatement(const volta::ast::IfStatement* ifStmt);
    void generateWhileStatement(const volta::ast::WhileStatement* whileStmt);
    void generateForStatement(const volta::ast::ForStatement* forStmt);
    void generateReturnStatement(const volta::ast::ReturnStatement* returnStmt);
    void generateExpressionStatement(const volta::ast::ExpressionStatement* exprStmt);

    // ========== Expression Generation ==========

    /**
     * Generate IR for an expression.
     * Returns the SSA value representing the expression result.
     */
    Value* generateExpression(const volta::ast::Expression* expr);

    Value* generateBinaryExpression(const volta::ast::BinaryExpression* binExpr);
    Value* generateUnaryExpression(const volta::ast::UnaryExpression* unaryExpr);
    Value* generateCallExpression(const volta::ast::CallExpression* callExpr);
    Value* generateIdentifier(const volta::ast::IdentifierExpression* identifier);
    Value* generateLiteral(const volta::ast::Expression* literal);
    Value* generateIfExpression(const volta::ast::IfExpression* ifExpr);
    Value* generateMatchExpression(const volta::ast::MatchExpression* matchExpr);
    Value* generateLambdaExpression(const volta::ast::LambdaExpression* lambdaExpr);
    Value* generateArrayLiteral(const volta::ast::ArrayLiteral* arrayLit);
    Value* generateStructLiteral(const volta::ast::StructLiteral* structLit);
    Value* generateMemberExpression(const volta::ast::MemberExpression* memberExpr);
    Value* generateIndexExpression(const volta::ast::IndexExpression* indexExpr);
    Value* generateMethodCall(const volta::ast::MethodCallExpression* methodCall);

    // ========== Helpers ==========

    /**
     * Get the IR type for an AST expression (from semantic analysis).
     */
    std::shared_ptr<semantic::Type> getType(const volta::ast::Expression* expr);

    /**
     * Error reporting.
     */
    void error(const std::string& message, volta::errors::SourceLocation loc);

private:
    volta::errors::ErrorReporter& reporter_;
    const volta::semantic::SemanticAnalyzer* analyzer_;

    // IR being constructed
    std::unique_ptr<IRModule> module_;
    IRBuilder builder_;
    SSABuilder ssaBuilder_;

    // Current context
    Function* currentFunction_;
    BasicBlock* currentBlock_;

    // Symbol tables (map AST symbols to IR values)
    std::unordered_map<std::string, GlobalVariable*> globalVariables_;
    std::unordered_map<std::string, Function*> functions_;

    // Local variables (name -> alloca pointer)
    std::unordered_map<std::string, Value*> localVariables_;

    bool hasErrors_;
};

} // namespace volta::ir
