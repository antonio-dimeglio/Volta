#pragma once

#include "TypeRegistry.hpp"
#include "SymbolTable.hpp"
#include "../HIR/HIR.hpp"
#include "../Error/Error.hpp"
#include <string>
#include <memory>

namespace Semantic {

/**
 * SemanticAnalyzer: Performs type checking and semantic validation on HIR
 *
 * Two-Pass Analysis:
 * 1. First Pass: Collect all function declarations into symbol table
 *    - Validates function signatures
 *    - Builds function registry for lookup during second pass
 *
 * 2. Second Pass: Analyze function bodies and statements
 *    - Type checking for expressions
 *    - Variable declaration and usage validation
 *    - Control flow validation (break/continue in loops)
 *    - Return type checking
 */
class SemanticAnalyzer {
private:
    TypeRegistry& type_registry;
    SymbolTable symbol_table;
    DiagnosticManager& diag;

    std::string current_function; 
    const Type* current_return_type;
    bool in_loop;

public:
    SemanticAnalyzer(TypeRegistry& type_registry, DiagnosticManager& diag);

    /**
     * Analyze entire program (two-pass)
     */
    void analyzeProgram(const HIR::Program& program);

    /**
     * Get the symbol table (for use by later compilation phases)
     */
    SymbolTable& getSymbolTable() { return symbol_table; }

private:

    /**
     * Collect function declaration and add to symbol table
     */
    void collectFnDecl(const FnDecl* fn);

    /**
     * Analyze a statement
     */
    void analyzeStmt(const Stmt* stmt);

    /**
     * Analyze function declaration body
     */
    void analyzeFnDecl(const FnDecl* fn);

    /**
     * Analyze variable declaration
     */
    void analyzeVarDecl(const VarDecl* varDecl);

    /**
     * Analyze return statement
     */
    void analyzeReturn(const ReturnStmt* returnStmt);

    /**
     * Analyze if statement
     */
    void analyzeIf(const IfStmt* ifStmt);


    /**
     * Analyze HIR return statement
     */
    void analyzeHIRReturn(const HIR::HIRReturnStmt* returnStmt);

    /**
     * Analyze HIR if statement
     */
    void analyzeHIRIf(const HIR::HIRIfStmt* ifStmt);

    /**
     * Analyze HIR while statement (including desugared for loops)
     */
    void analyzeHIRWhile(const HIR::HIRWhileStmt* whileStmt);

    /**
     * Analyze HIR block statement
     */
    void analyzeHIRBlock(const HIR::HIRBlockStmt* blockStmt);

    /**
     * Analyze HIR break statement
     */
    void analyzeHIRBreak(const HIR::HIRBreakStmt* breakStmt);

    /**
     * Analyze HIR continue statement
     */
    void analyzeHIRContinue(const HIR::HIRContinueStmt* continueStmt);


    /**
     * Analyze while statement
     */
    void analyzeWhile(const WhileStmt* whileStmt);

    /*
     * Analyze for statement
    */
    void analyzeFor(const ForStmt* forStmt);

    /**
     * Analyze break statement
     */
    void analyzeBreak(const BreakStmt* breakStmt);

    /**
     * Analyze continue statement
     */
    void analyzeContinue(const ContinueStmt* continueStmt);

    /**
     * Analyze block statement
     */
    void analyzeBlock(const BlockStmt* blockStmt);


    /**
     * Analyze expression (recursively validates sub-expressions)
     * Returns the type of the expression
     */
    const Type* analyzeExpr(const Expr* expr);

    /**
     * Infer the type of an expression
     * Returns interned type pointer
     */
    const Type* inferExprType(const Expr* expr);


    /**
     * Validate binary expression operands
     */
    void validateBinaryExpr(const BinaryExpr* expr);

    /**
     * Validate unary expression operand
     */
    void validateUnaryExpr(const UnaryExpr* expr);

    /**
     * Validate function call arguments and reference modes
     */
    void validateFnCall(const FnCall* expr);

    /**
     * Validate assignment (mutability and type compatibility)
     */
    void validateAssignment(const Assignment* expr);

    /**
     * Validate array indexing
     */
    void validateIndexExpr(const IndexExpr* expr);

    /**
     * Validate array literal type consistency
     */
    void validateArrayLiteral(const ArrayLiteral* expr);

    /**
     * Validate increment expression (mutability check)
     */
    void validateIncrement(const Increment* expr);

    /**
     * Validate decrement expression (mutability check)
     */
    void validateDecrement(const Decrement* expr);

    /**
     * Validate compound assignment (mutability and type compatibility)
     */
    void validateCompoundAssign(const CompoundAssign* expr);


    /**
     * Infer type of array literal
     * Validates all elements have same type
     */
    const Type* inferArrayLiteralType(const ArrayLiteral* arr);

    /**
     * Check that actual type is compatible with expected type
     * Reports error with context message if incompatible
     */
    void validateTypeCompatibility(const Type* expected_type,
                                   const Type* actual_type,
                                   const std::string& context);
};

} // namespace Semantic
