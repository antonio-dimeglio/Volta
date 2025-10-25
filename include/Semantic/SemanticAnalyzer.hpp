#pragma once

#include "../Parser/ASTVisitor.hpp"
#include "../Parser/AST.hpp"
#include "../HIR/HIR.hpp"
#include "../Type/TypeRegistry.hpp"
#include "SymbolTable.hpp"
#include "FunctionRegistry.hpp"
#include "GenericRegistry.hpp"
#include "../Error/Error.hpp"
#include <unordered_map>
#include <set>

namespace Semantic {

class SemanticAnalyzer : public HIR::HIRStmtVisitor, public ExprVisitor<const Type::Type*> {
private:
    Type::TypeRegistry& typeRegistry;
    SymbolTable symbolTable;
    DiagnosticManager& diag;
    FunctionRegistry* functionRegistry = nullptr;  // Optional: for cross-module function resolution

    const Type::Type* currentReturnType = nullptr;
    bool inLoop = false;
    std::string currentFunctionName;

    // Type map: stores computed type for each expression
    std::unordered_map<const Expr*, const Type::Type*> exprTypes;

    // Map from sizeof() FnCall nodes to their computed size values (for MIR lowering)
    std::unordered_map<const Expr*, size_t> sizeofValues;

    // Expected type context for top-down type inference (e.g., for array fill initialization)
    const Type::Type* expectedType = nullptr;

    // NEW: Generic tracking and monomorphization
    Volta::GenericRegistry genericRegistry;  // Local registry (used if external not provided)
    Volta::GenericRegistry* externalGenericRegistry = nullptr;  // Shared registry across modules

    // NEW: Store monomorphized AST nodes (we own these)
    std::vector<std::unique_ptr<FnDecl>> monomorphizedFunctions;
    std::vector<std::unique_ptr<StructDecl>> monomorphizedStructs;

    // NEW: Prevent infinite recursion during monomorphization
    std::set<std::string> currentlyAnalyzing;

    // Helper to get the active generic registry (external if set, otherwise local)
    Volta::GenericRegistry& getGenericRegistry() {
        return externalGenericRegistry ? *externalGenericRegistry : genericRegistry;
    }

public:
    SemanticAnalyzer(Type::TypeRegistry& types, DiagnosticManager& diag, Volta::GenericRegistry* genRegistry = nullptr);

    // Set the function registry for cross-module function resolution
    void setFunctionRegistry(FunctionRegistry* registry) { functionRegistry = registry; }

    // Set the generic registry for cross-module generic resolution
    void setGenericRegistry(Volta::GenericRegistry* registry) { externalGenericRegistry = registry; }

    void analyzeProgram(const HIR::HIRProgram& program);
    SymbolTable& getSymbolTable() { return symbolTable; }
    [[nodiscard]] const SymbolTable& getSymbolTable() const { return symbolTable; }

    // Get the type map (for HIR to MIR lowering)
    [[nodiscard]] const std::unordered_map<const Expr*, const Type::Type*>& getExprTypes() const { return exprTypes; }

    // Get the monomorphized functions (for HIR to MIR lowering)
    [[nodiscard]] const std::vector<std::unique_ptr<FnDecl>>& getMonomorphizedFunctions() const { return monomorphizedFunctions; }

    // Public methods for cross-module type management
    void registerStructTypes(const HIR::HIRProgram& program);
    void resolveUnresolvedTypes(HIR::HIRProgram& program);

    // NEW: Register generic templates from AST (must be called before HIR lowering or semantic analysis)
    void registerGenericTemplates(const Program& ast);

    // HIR Statement visitors
    void visitVarDecl(HIR::HIRVarDecl& node) override;
    void visitFnDecl(HIR::HIRFnDecl& node) override;
    void visitReturnStmt(HIR::HIRReturnStmt& node) override;
    void visitIfStmt(HIR::HIRIfStmt& node) override;
    void visitWhileStmt(HIR::HIRWhileStmt& node) override;
    void visitExprStmt(HIR::HIRExprStmt& node) override;
    void visitBlockStmt(HIR::HIRBlockStmt& node) override;
    void visitBreakStmt(HIR::HIRBreakStmt& node) override;
    void visitContinueStmt(HIR::HIRContinueStmt& node) override;
    void visitExternBlock(HIR::HIRExternBlock& node) override;
    void visitImportStmt(HIR::HIRImportStmt& node) override;
    void visitStructDecl(HIR::HIRStructDecl& node) override;

    // Expression visitors
    const Type::Type* visitLiteral(Literal& node) override;
    const Type::Type* visitVariable(Variable& node) override;
    const Type::Type* visitBinaryExpr(BinaryExpr& node) override;
    const Type::Type* visitUnaryExpr(UnaryExpr& node) override;
    const Type::Type* visitFnCall(FnCall& node) override;
    const Type::Type* visitAssignment(Assignment& node) override;
    const Type::Type* visitGroupingExpr(GroupingExpr& node) override;
    const Type::Type* visitArrayLiteral(ArrayLiteral& node) override;
    const Type::Type* visitIndexExpr(IndexExpr& node) override;
    const Type::Type* visitAddrOf(AddrOf& node) override;
    const Type::Type* visitSizeOf(SizeOf& node) override;
    const Type::Type* visitCompoundAssign(CompoundAssign& node) override;
    const Type::Type* visitIncrement(Increment& node) override;
    const Type::Type* visitDecrement(Decrement& node) override;
    const Type::Type* visitRange(Range& node) override;
    const Type::Type* visitStructLiteral(StructLiteral& node) override;
    const Type::Type* visitFieldAccess(FieldAccess& node) override;
    const Type::Type* visitStaticMethodCall(StaticMethodCall& node) override;
    const Type::Type* visitInstanceMethodCall(InstanceMethodCall& node) override;

    // NEW: Generic support
    void validateTypeParameters(const std::vector<Token>& typeParams, int line, int column);
    void registerGenericFunction(FnDecl* fn);
    void registerGenericStruct(StructDecl* structDecl);

    FnDecl* monomorphizeFunction(
        const std::string& genericName,
        const std::vector<const Type::Type*>& typeArgs,
        int callSiteLine,
        int callSiteColumn
    );

    const Type::StructType* monomorphizeStruct(
        const std::string& genericName,
        const std::vector<const Type::Type*>& typeArgs,
        int useSiteLine,
        int useSiteColumn
    );

private:
    void collectFunctionSignatures(const HIR::HIRProgram& program);
    void registerFunction(HIR::HIRFnDecl& node);
    void resolveTypesInStmts(std::vector<std::unique_ptr<HIR::HIRStmt>>& stmts);
    void resolveTypesInStmt(HIR::HIRStmt* stmt);
    const Type::Type* resolveType(const Type::Type* type);
    const Type::Type* errorType();

    // ========================================================================
    // Type Classification Utilities
    // ========================================================================

    /// Check if type is any numeric type (integer or float)
    static bool isNumericType(const Type::Type* type);

    /// Check if type is any integer type (signed or unsigned)
    static bool isIntegerType(const Type::Type* type);

    /// Check if type is a signed integer type
    static bool isSignedIntegerType(const Type::Type* type);

    /// Check if type is an unsigned integer type
    static bool isUnsignedIntegerType(const Type::Type* type);

    /// Check if type is a floating-point type
    static bool isFloatType(const Type::Type* type);

    /// Check if a type contains a generic type parameter (recursively checks GenericType)
    static bool containsGenericType(const Type::Type* type);

    /// Get bit width of a primitive type (returns 0 for non-primitives)
    static int getTypeBitWidth(const Type::Type* type);

    /// Get size of a type in bytes (for sizeof implementation)
    static size_t getTypeSize(const Type::Type* type);

    // ========================================================================
    // Type Compatibility Checking Utilities
    // ========================================================================

    /// Check if two types are exactly the same (no conversion)
    static bool areTypesEqual(const Type::Type* a, const Type::Type* b);

    /// Check if 'from' can be IMPLICITLY converted to 'to'
    /// This enforces strict rules:
    /// - No implicit conversions between signed and unsigned
    /// - No implicit narrowing conversions
    /// - No implicit conversions between int and float
    /// - Allows widening within same signedness (i8→i32, u8→u32, f32→f64)
    /// - Special cases: str→ptr<i8>, struct↔ptr<struct>
    bool isImplicitlyConvertible(const Type::Type* from, const Type::Type* to);

    /// Check if 'from' type has the same signedness as 'to' type
    /// Returns true if both are signed, both are unsigned, or either is non-integer
    bool haveSameSignedness(const Type::Type* from, const Type::Type* to);

    /// Check if conversion from 'from' to 'to' is a widening conversion
    /// (going from smaller to larger bit width)
    bool isWideningConversion(const Type::Type* from, const Type::Type* to);

    /// Check if conversion from 'from' to 'to' is a narrowing conversion
    /// (going from larger to smaller bit width)
    bool isNarrowingConversion(const Type::Type* from, const Type::Type* to);

    /// Check if a literal value fits within the range of a target type
    /// Used for validating integer literal assignments
    static bool doesLiteralFitInType(int64_t value, const Type::Type* targetType);

    /// Check if conversion from 'from' to 'to' would be valid with explicit cast
    /// More permissive than implicit conversion - allows:
    /// - All numeric to numeric conversions (for future 'as' operator)
    /// - Sign changes, narrowing, int↔float, etc.
    bool isExplicitlyConvertible(const Type::Type* from, const Type::Type* to);

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /// Try to coerce an integer literal (including negative literals) to a target type
    /// Returns true if successful, false if the literal doesn't fit or expr is not a literal
    bool tryCoerceIntegerLiteral(Expr* expr, const Type::Type* targetType);

    /// Check if a statement list contains a return statement (recursively)
    bool containsReturn(const std::vector<std::unique_ptr<HIR::HIRStmt>>& stmts);
};

} // namespace Semantic
