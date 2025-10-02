#pragma once

#include "IR.hpp"
#include "IRModule.hpp"
#include "IRBuilder.hpp"
#include "ast/Statement.hpp"
#include "../semantic/SemanticAnalyzer.hpp"
#include "../error/ErrorReporter.hpp"
#include <memory>
#include <unordered_map>

namespace volta::ir {

/**
 * IRGenerator - Converts AST to IR
 *
 * This class walks the semantically-analyzed AST and generates IR code.
 * It uses IRBuilder to construct IR instructions.
 *
 * Design:
 * - Multi-pass generation (can be single-pass for now)
 * - Uses symbol table from semantic analysis for type info
 * - Generates SSA-like code (fresh variables for each value)
 *
 * Example usage:
 *   IRGenerator generator(errorReporter);
 *   std::unique_ptr<IRModule> module = generator.generate(ast, semanticAnalyzer);
 *   if (module) {
 *       // Success - IR generated
 *   }
 */
class IRGenerator {
public:
    explicit IRGenerator(volta::errors::ErrorReporter& reporter)
        : reporter_(reporter), currentFunction_(nullptr) {}

    /**
     * Generate IR from AST
     * Returns nullptr on error
     */
    std::unique_ptr<IRModule> generate(const volta::ast::Program& program,
                                       const volta::semantic::SemanticAnalyzer& analyzer);

private:
    // ========== Top-Level Generation ==========

    /**
     * Generate IR for entire program
     */
    void generateProgram(const volta::ast::Program& program);

    /**
     * Generate IR for a function declaration
     */
    void generateFunction(const volta::ast::FnDeclaration& funcDecl);

    /**
     * Generate IR for a struct declaration
     * (Registers type info, no code generation needed)
     */
    void generateStruct(const volta::ast::StructDeclaration& structDecl);

    /**
     * Generate IR for global variable
     */
    void generateGlobal(const volta::ast::VarDeclaration& varDecl);

    /**
     * Generate IR for top-level initialization function
     * Wraps top-level statements in a special __init__ function
     */
    void generateInitFunction(const std::vector<const volta::ast::Statement*>& stmts);

    // ========== Statement Generation ==========

    /**
     * Generate IR for a statement
     */
    void generateStatement(const volta::ast::Statement* stmt);

    void generateVarDeclaration(const volta::ast::VarDeclaration& varDecl);
    void generateExpressionStatement(const volta::ast::ExpressionStatement& stmt);
    void generateIfStatement(const volta::ast::IfStatement& stmt);
    void generateWhileStatement(const volta::ast::WhileStatement& stmt);
    void generateForStatement(const volta::ast::ForStatement& stmt);
    void generateReturnStatement(const volta::ast::ReturnStatement& stmt);
    void generateBlock(const volta::ast::Block& block);

    // ========== Expression Generation ==========

    /**
     * Generate IR for an expression
     * Returns the Value* representing the result
     */
    Value* generateExpression(const volta::ast::Expression* expr);

    Value* generateBinaryExpression(const volta::ast::BinaryExpression& expr);
    Value* generateUnaryExpression(const volta::ast::UnaryExpression& expr);
    Value* generateCallExpression(const volta::ast::CallExpression& expr);
    Value* generateIndexExpression(const volta::ast::IndexExpression& expr);
    Value* generateSliceExpression(const volta::ast::SliceExpression& expr);
    Value* generateMemberExpression(const volta::ast::MemberExpression& expr);
    Value* generateMethodCallExpression(const volta::ast::MethodCallExpression& expr);
    Value* generateIfExpression(const volta::ast::IfExpression& expr);
    Value* generateMatchExpression(const volta::ast::MatchExpression& expr);
    Value* generateLambdaExpression(const volta::ast::LambdaExpression& expr);
    Value* generateIdentifierExpression(const volta::ast::IdentifierExpression& expr);
    Value* generateIntegerLiteral(const volta::ast::IntegerLiteral& lit);
    Value* generateFloatLiteral(const volta::ast::FloatLiteral& lit);
    Value* generateStringLiteral(const volta::ast::StringLiteral& lit);
    Value* generateBooleanLiteral(const volta::ast::BooleanLiteral& lit);
    Value* generateNoneLiteral(const volta::ast::NoneLiteral& lit);
    Value* generateSomeLiteral(const volta::ast::SomeLiteral& lit);
    Value* generateArrayLiteral(const volta::ast::ArrayLiteral& lit);
    Value* generateTupleLiteral(const volta::ast::TupleLiteral& lit);
    Value* generateStructLiteral(const volta::ast::StructLiteral& lit);

    // ========== Type Conversion ==========

    /**
     * Convert AST type to semantic type
     * Uses semantic analyzer's type resolution
     */
    std::shared_ptr<semantic::Type> resolveType(const volta::ast::Type* astType);

    // ========== Symbol Management ==========

    /**
     * Register a variable in current scope
     * Maps variable name to its IR value (alloca result)
     */
    void declareVariable(const std::string& name, Value* value);

    /**
     * Look up a variable in current scope
     * Returns nullptr if not found
     */
    Value* lookupVariable(const std::string& name);

    // ========== Error Reporting ==========

    void error(const std::string& message, const volta::errors::SourceLocation& loc);

    // ========== Built-in Functions ==========

    /**
     * Register built-in/foreign functions like print
     */
    void registerBuiltinFunctions();

    // ========== State ==========

    volta::errors::ErrorReporter& reporter_;
    IRBuilder builder_;
    std::unique_ptr<IRModule> module_;

    // Current context
    Function* currentFunction_;

    // Variable mapping (name -> IR value)
    // In SSA-like form, each variable declaration creates a fresh alloca
    std::unordered_map<std::string, Value*> variableMap_;

    // Function mapping (name -> IR function)
    std::unordered_map<std::string, Function*> functionMap_;

    // Reference to semantic analyzer for type info
    const volta::semantic::SemanticAnalyzer* analyzer_;
};

} // namespace volta::ir
