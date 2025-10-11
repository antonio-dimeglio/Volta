#pragma once

#include "vir/VIRModule.hpp"
#include "vir/RuntimeFunctionRegistry.hpp"
#include "ast/Statement.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include <memory>
#include <unordered_map>

namespace volta::vir {

/**
 * VIR Lowering Pass: AST → VIR
 *
 * Transforms semantically-analyzed AST into VIR.
 * Responsibilities:
 * - Monomorphize generics
 * - Desugar method calls to function calls
 * - Make boxing/unboxing explicit
 * - Insert implicit casts
 * - Lower for loops to while loops
 * - Create wrapper functions for higher-order functions
 */
class VIRLowering {
public:
    VIRLowering(const volta::ast::Program* program,
                const volta::semantic::SemanticAnalyzer* analyzer);

    /**
     * Lower the entire program to VIR
     */
    std::unique_ptr<VIRModule> lower();

private:
    const volta::ast::Program* program_;
    const volta::semantic::SemanticAnalyzer* analyzer_;
    std::unique_ptr<VIRModule> module_;

    // Monomorphization tracking
    std::unordered_map<std::string, std::vector<std::string>> genericInstantiations_;

    // String literal interning
    std::unordered_map<std::string, std::string> internedStrings_;
    size_t stringCounter_ = 0;

    // Temporary variable generation
    size_t tempCounter_ = 0;

    // Runtime function registry
    RuntimeFunctionRegistry runtimeRegistry_;

    // ========================================================================
    // Top-Level Lowering
    // ========================================================================

    /**
     * Phase 1: Collect and monomorphize all type declarations
     */
    void monomorphizeGenerics();

    /**
     * Mangle name for monomorphic instantiation
     * Example: Box[i32] -> Box_i32
     */
    std::string mangleName(const std::string& baseName,
                           const std::vector<std::shared_ptr<volta::semantic::Type>>& typeArgs);

    // ========================================================================
    // Declaration Lowering
    // ========================================================================

    std::unique_ptr<VIRFunction> lowerFunctionDeclaration(const volta::ast::FnDeclaration* fn);
    std::unique_ptr<VIRStructDecl> lowerStructDeclaration(const volta::ast::StructDeclaration* structDecl);
    std::unique_ptr<VIREnumDecl> lowerEnumDeclaration(const volta::ast::EnumDeclaration* enumDecl);

    // ========================================================================
    // Statement Lowering
    // ========================================================================

    std::unique_ptr<VIRStmt> lowerStatement(const volta::ast::Statement* stmt);
    std::unique_ptr<VIRBlock> lowerBlock(const volta::ast::Block* block);
    std::unique_ptr<VIRStmt> lowerVarDeclaration(const volta::ast::VarDeclaration* varDecl);
    std::unique_ptr<VIRStmt> lowerReturnStatement(const volta::ast::ReturnStatement* ret);
    std::unique_ptr<VIRStmt> lowerIfStatement(const volta::ast::IfStatement* ifStmt);
    std::unique_ptr<VIRStmt> lowerWhileStatement(const volta::ast::WhileStatement* whileStmt);
    std::unique_ptr<VIRStmt> lowerForStatement(const volta::ast::ForStatement* forStmt);
    std::unique_ptr<VIRStmt> lowerBreakStatement(const volta::ast::BreakStatement* breakStmt);
    std::unique_ptr<VIRStmt> lowerContinueStatement(const volta::ast::ContinueStatement* continueStmt);

    /**
     * Desugar for loop over range to while loop
     * Example: for i in 0..10 { body }
     * Becomes: let mut i = 0; while i < 10 { body; i = i + 1 }
     */
    std::unique_ptr<VIRStmt> lowerForRange(const volta::ast::ForStatement* forStmt);

    /**
     * Desugar for loop over array to while loop with index
     */
    std::unique_ptr<VIRBlock> lowerForArray(const volta::ast::ForStatement* forStmt);

    // ========================================================================
    // Expression Lowering
    // ========================================================================

    std::unique_ptr<VIRExpr> lowerExpression(const volta::ast::Expression* expr);

    // Literals
    std::unique_ptr<VIRExpr> lowerIntegerLiteral(const volta::ast::IntegerLiteral* lit);
    std::unique_ptr<VIRExpr> lowerFloatLiteral(const volta::ast::FloatLiteral* lit);
    std::unique_ptr<VIRExpr> lowerBooleanLiteral(const volta::ast::BooleanLiteral* lit);
    std::unique_ptr<VIRExpr> lowerStringLiteral(const volta::ast::StringLiteral* lit);
    std::unique_ptr<VIRExpr> lowerArrayLiteral(const volta::ast::ArrayLiteral* lit);

    // Identifiers
    std::unique_ptr<VIRExpr> lowerIdentifierExpression(const volta::ast::IdentifierExpression* expr);

    // Operations
    std::unique_ptr<VIRExpr> lowerBinaryExpression(const volta::ast::BinaryExpression* expr);
    std::unique_ptr<VIRExpr> lowerUnaryExpression(const volta::ast::UnaryExpression* expr);
    std::unique_ptr<VIRExpr> lowerAssignment(const volta::ast::BinaryExpression* expr);
    
    // Calls
    std::unique_ptr<VIRExpr> lowerCallExpression(const volta::ast::CallExpression* call);
    std::unique_ptr<VIRExpr> lowerMethodCallExpression(const volta::ast::MethodCallExpression* call);

    /**
     * Desugar method call to function call
     * Example: obj.method(args) -> Type_method(obj, args)
     */
    std::unique_ptr<VIRCall> desugarMethodCall(const volta::ast::MethodCallExpression* call);

    // Control Flow
    std::unique_ptr<VIRExpr> lowerIfExpression(const volta::ast::IfExpression* expr);

    // Array operations
    std::unique_ptr<VIRExpr> lowerIndexExpression(const volta::ast::IndexExpression* expr);

    // ========================================================================
    // Fixed Array Operations (Phase 7)
    // ========================================================================

    /**
     * Lower fixed array literal to VIRFixedArrayNew
     * Example: [1, 2, 3] -> VIRFixedArrayNew(elements=[...], size=3)
     * Example: [0; 256] -> VIRFixedArrayNew(elements=[0], size=256, repeat=true)
     */
    std::unique_ptr<VIRExpr> lowerFixedArrayLiteral(const volta::ast::ArrayLiteral* lit);

    /**
     * Lower array index expression to VIRFixedArrayGet
     * Inserts bounds check: arr[i] -> VIRFixedArrayGet(arr, VIRBoundsCheck(arr, i))
     */
    std::unique_ptr<VIRExpr> lowerFixedArrayGet(const volta::ast::IndexExpression* expr);

    /**
     * Lower array index assignment to VIRFixedArraySet
     * Example: arr[i] = value -> VIRFixedArraySet(arr, VIRBoundsCheck(arr, i), value)
     */
    std::unique_ptr<VIRExpr> lowerFixedArraySet(const volta::ast::IndexExpression* lhs,
                                                  std::unique_ptr<VIRExpr> value);

    /**
     * Perform escape analysis on a variable to determine allocation strategy
     * Returns true if variable should be stack-allocated, false for heap
     *
     * Stack allocation criteria:
     * - Variable is local (not returned, not captured in closure)
     * - Array size is "reasonable" (< 1KB, configurable threshold)
     * - Not explicitly marked for heap allocation
     */
    bool shouldStackAllocate(const volta::ast::VarDeclaration* varDecl);

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Insert implicit cast if needed
     */
    std::unique_ptr<VIRExpr> insertImplicitCast(std::unique_ptr<VIRExpr> expr,
                                                  std::shared_ptr<volta::semantic::Type> targetType);

    /**
     * Intern a string literal (deduplicate identical strings)
     */
    std::string internString(const std::string& value);

    /**
     * Generate a unique temporary variable name
     */
    std::string generateTempName();

    /**
     * Get type from semantic analyzer for an AST node
     */
    std::shared_ptr<volta::semantic::Type> getType(const volta::ast::Expression* expr);

    /**
     * Convert AST binary operator to VIR binary operator kind
     */
    VIRBinaryOpKind convertBinaryOp(volta::ast::BinaryExpression::Operator op);

    /**
     * Convert AST unary operator to VIR unary operator kind
     */
    VIRUnaryOpKind convertUnaryOp(volta::ast::UnaryExpression::Operator op);
};

} // namespace volta::vir
