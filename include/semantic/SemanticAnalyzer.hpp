#pragma once

#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <set>
#include "ast/Expression.hpp"
#include "ast/Statement.hpp"
#include "error/ErrorReporter.hpp"
#include "semantic/Type.hpp"
#include "semantic/SymbolTable.hpp"

namespace volta::semantic {

// Forward declaration for recursive structure
struct PatternCoverage;

/**
 * Tracks coverage of nested patterns within match expressions
 *
 * Used to determine exhaustiveness of match patterns with nested structures.
 * For example, matching on Option[Result[T, E]] requires tracking which
 * combinations of Some/None and Ok/Err have been covered.
 */
struct NestedCoverage {
    bool hasWildcard;                                                        ///< True if a wildcard (_) pattern was seen
    std::set<std::string> coveredVariants;                                   ///< Set of variant names covered at this level
    std::map<std::string, std::vector<NestedCoverage>> nestedByVariant;     ///< Nested coverage for each variant's fields

    NestedCoverage() : hasWildcard(false) {}
};

/**
 * Tracks which patterns are covered in a match expression
 *
 * Used for exhaustiveness checking to ensure all possible values
 * of the matched type are handled by at least one pattern.
 */
struct PatternCoverage {
    bool coversEverything;                                                   ///< True if a catch-all pattern exists
    std::set<std::string> coveredVariants;                                   ///< Enum variants explicitly matched
    std::map<std::string, std::vector<NestedCoverage>> nestedCoverageByVariant;  ///< Nested pattern coverage per variant

    PatternCoverage() : coversEverything(false) {}
};


/**
 * Semantic analyzer for Volta programs
 *
 * Performs multi-pass static analysis on the AST to verify program correctness:
 *
 * Pass 1: Declaration Collection
 *   - Discovers all top-level types, functions, structs, and enums
 *   - Builds initial symbol table
 *   - Enables forward references
 *
 * Pass 2: Type Resolution
 *   - Resolves all type references to concrete types
 *   - Instantiates generic types
 *   - Builds complete type information
 *
 * Pass 3: Type Checking and Validation
 *   - Checks type compatibility in expressions
 *   - Validates control flow (return, break, continue)
 *   - Enforces mutability constraints
 *   - Verifies match expression exhaustiveness
 *   - Infers types where not explicitly annotated
 *
 * After successful analysis, the analyzer provides:
 *   - Type information for all expressions
 *   - Symbol table with all declarations
 *   - Error diagnostics if validation failed
 *
 * Example usage:
 *   SemanticAnalyzer analyzer(errorReporter);
 *   analyzer.registerBuiltins();
 *   if (analyzer.analyze(program)) {
 *       auto exprType = analyzer.getExpressionType(someExpr);
 *       // Proceed to code generation
 *   }
 */
class SemanticAnalyzer {
public:
    /**
     * Constructs a semantic analyzer
     *
     * @param reporter Error reporter for diagnostics
     */
    explicit SemanticAnalyzer(volta::errors::ErrorReporter& reporter);

    /**
     * Analyze an entire Volta program
     *
     * Performs multi-pass analysis to check types, resolve names,
     * and validate control flow. Reports errors via the error reporter.
     *
     * @param program The AST root node
     * @return true if analysis succeeded with no errors, false otherwise
     */
    bool analyze(const volta::ast::Program& program);

    /**
     * Get the type of an expression after analysis
     *
     * Retrieves the inferred or checked type for any expression node.
     * Should only be called after a successful analyze() pass.
     *
     * @param expr Expression AST node
     * @return The semantic type of the expression, or nullptr if not analyzed
     */
    std::shared_ptr<Type> getExpressionType(const volta::ast::Expression* expr) const;

    /**
     * Resolve a type annotation to a semantic type
     *
     * Converts AST type syntax (e.g., "Array[i32]") into a semantic type object.
     * Handles primitive types, generics, functions, structs, and enums.
     *
     * @param typeAnnotation AST type node
     * @return Resolved semantic type, or unknown type if resolution fails
     */
    std::shared_ptr<Type> resolveTypeAnnotation(const volta::ast::Type* typeAnnotation) const;

    /**
     * Register built-in functions and types
     *
     * Populates the symbol table with Volta's standard library functions
     * like print(), println(), len(), etc. Should be called before analyze().
     */
    void registerBuiltins();

private:
    // === Pass 1: Collect top-level declarations ===
    void collectDeclarations(const volta::ast::Program& program);
    void collectEnum(const volta::ast::EnumDeclaration& enumDecl);
    void collectFunction(const volta::ast::FnDeclaration& fn);
    void collectStruct(const volta::ast::StructDeclaration& structDecl);

    // === Pass 2: Resolve types ===
    void resolveTypes(const volta::ast::Program& program);
    

    // === Pass 3: Type check and validate ===
    void analyzeProgram(const volta::ast::Program& program);
    void analyzeStatement(const volta::ast::Statement* stmt);
    void analyzeVarDeclaration(const volta::ast::VarDeclaration* varDecl);
    void analyzeFnDeclaration(const volta::ast::FnDeclaration* fnDecl);
    void analyzeIfStatement(const volta::ast::IfStatement* ifStmt);
    void analyzeWhileStatement(const volta::ast::WhileStatement* whileStmt);
    void analyzeForStatement(const volta::ast::ForStatement* forStmt);
    void analyzeReturnStatement(const volta::ast::ReturnStatement* returnStmt);
    std::shared_ptr<Type> analyzeMatchExpression(const volta::ast::MatchExpression* matchExpr);

    // Expression type checking (returns inferred type)
    // Expression analysis with optional expected type for contextual inference
    std::shared_ptr<Type> analyzeExpression(const volta::ast::Expression* expr,
                                           std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeBinaryExpression(const volta::ast::BinaryExpression* binExpr,
                                                  std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeUnaryExpression(const volta::ast::UnaryExpression* unaryExpr,
                                                 std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeCallExpression(const volta::ast::CallExpression* callExpr,
                                               std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeIdentifier(const volta::ast::IdentifierExpression* identifier);
    std::shared_ptr<Type> analyzeLiteral(const volta::ast::Expression* literal);
    std::shared_ptr<Type> analyzeMemberExpression(const volta::ast::MemberExpression* memberExpr,
                                                  std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeMethodCallExpression(const volta::ast::MethodCallExpression* methodCall,
                                                      std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeArrayLiteral(const volta::ast::ArrayLiteral* arrayLit,
                                             std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeIndexExpression(const volta::ast::IndexExpression* indexExpr);
    std::shared_ptr<Type> analyzeStructLiteral(const volta::ast::StructLiteral* structLit,
                                              std::shared_ptr<Type> expectedType = nullptr);
    std::shared_ptr<Type> analyzeCastExpression(const volta::ast::CastExpression* castExpr);
    void bindPatternVariables(const volta::ast::Pattern* pattern,
                             std::shared_ptr<Type> patternType);
    PatternCoverage analyzePattern(const volta::ast::Pattern* pattern,
                                 std::shared_ptr<Type> matchedType);
    void mergeNestedCoverage(std::vector<NestedCoverage>& target,
                            const std::vector<NestedCoverage>& source);
    bool isNestedCoverageExhaustive(const std::vector<NestedCoverage>& coverage,
                                   const std::vector<std::shared_ptr<Type>>& types);
    void checkExhaustiveness(std::shared_ptr<Type> matchedType,
                            const std::set<std::string>& coveredVariants,
                            bool hasWildcard,
                            const std::map<std::string, std::vector<NestedCoverage>>& nestedCoverageByVariant,
                            const volta::errors::SourceLocation& loc);

    // Type operations
    bool areTypesCompatible(const Type* expected, const Type* actual);
    std::shared_ptr<Type> inferBinaryOpType(volta::ast::BinaryExpression::Operator op, const Type* left, const Type* right);
    std::shared_ptr<Type> inferUnaryOpType(volta::ast::UnaryExpression::Operator op, const Type* operand);

    // Symbol table operations
    void enterScope();
    void exitScope();
    void declareVariable(const std::string& name, std::shared_ptr<Type> type, bool isMutable, volta::errors::SourceLocation loc);
    void declareFunction(const std::string& name, std::shared_ptr<Type> functionType, volta::errors::SourceLocation loc);
    void declareType(const std::string& name, std::shared_ptr<Type> type, volta::errors::SourceLocation loc);

    // Lookup operations
    std::shared_ptr<Type> lookupVariable(const std::string& name, volta::errors::SourceLocation loc);
    std::shared_ptr<Type> lookupFunction(const std::string& name, volta::errors::SourceLocation loc);
    bool isVariableMutable(const std::string& name);
    bool isExpressionMutable(const volta::ast::Expression* expr);

    // Control flow state
    void enterLoop();
    void exitLoop();
    void enterFunction(std::shared_ptr<Type> returnType);
    void exitFunction();
    bool inLoop() const { return loopDepth_ > 0; }
    std::shared_ptr<Type> currentFunctionReturnType() const { return currentReturnType_; }

public:
    /**
     * Get the symbol table for code generation
     *
     * Provides access to the complete symbol table containing all
     * variable, function, type, and struct declarations discovered
     * during analysis. Used by code generators to look up symbols.
     *
     * @return Pointer to the symbol table (non-owning)
     */
    SymbolTable* getSymbolTable() { return symbolTable_.get(); }

    /**
     * Get the symbol table (const version)
     *
     * @return Const pointer to the symbol table (non-owning)
     */
    const SymbolTable* getSymbolTable() const { return symbolTable_.get(); }

private:

    // Error reporting helpers
    void error(const std::string& message, volta::errors::SourceLocation loc);
    void typeError(const std::string& message, const Type* expected, const Type* actual, volta::errors::SourceLocation loc);

    // Type substitution helper for generics
    std::shared_ptr<Type> substituteTypeVariables(
        const std::shared_ptr<Type>& type,
        const std::vector<std::string>& typeParams,
        const std::vector<std::shared_ptr<Type>>& typeArgs
    );

private:
    volta::errors::ErrorReporter& errorReporter_;
    std::unique_ptr<SymbolTable> symbolTable_;
    TypeCache typeCache_;

    // Store inferred types for expressions
    std::unordered_map<const volta::ast::Expression*, std::shared_ptr<Type>> expressionTypes_;

    // Control flow state
    int loopDepth_ = 0;
    std::shared_ptr<Type> currentReturnType_ = nullptr;

    // Analysis state
    bool hasErrors_ = false;
};

} // namespace volta::semantic