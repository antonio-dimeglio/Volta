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

struct NestedCoverage {
    bool hasWildcard;
    std::set<std::string> coveredVariants;
    // For each nested variant, track its own nested coverage
    // This allows arbitrary nesting depth
    std::map<std::string, std::vector<NestedCoverage>> nestedByVariant;

    NestedCoverage() : hasWildcard(false) {}
};

struct PatternCoverage {
    bool coversEverything;
    std::set<std::string> coveredVariants;
    // For each variant we cover, track the nested coverage
    // Key: variant name, Value: coverage for each nested argument position
    std::map<std::string, std::vector<NestedCoverage>> nestedCoverageByVariant;

    PatternCoverage() : coversEverything(false) {}
};


/**
 * SemanticAnalyzer performs static analysis on the AST:
 * - Type checking and inference
 * - Name resolution
 * - Mutability checking
 * - Control flow validation
 */
class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(volta::errors::ErrorReporter& reporter);

    // Main entry point: analyze entire program
    bool analyze(const volta::ast::Program& program);

    // Get the inferred/checked type of an expression (after analysis)
    std::shared_ptr<Type> getExpressionType(const volta::ast::Expression* expr) const;
    std::shared_ptr<Type> resolveTypeAnnotation(const volta::ast::Type* typeAnnotation) const;

    // Register builtin functions (should be called before analysis)
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
    // Symbol table access (for IR generation)
    SymbolTable* getSymbolTable() { return symbolTable_.get(); }
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