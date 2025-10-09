#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "llvm/RuntimeFunctions.hpp"


namespace volta::llvm_codegen {

/**
 * LLVM backend code generator for Volta
 *
 * Transforms a semantically-analyzed Volta AST into optimized LLVM IR,
 * which can then be compiled to native machine code. This class bridges
 * the gap between Volta's high-level language constructs and LLVM's
 * low-level intermediate representation.
 *
 * The codegen process:
 * 1. Creates an LLVM module and IR builder
 * 2. Lowers Volta semantic types to LLVM types
 * 3. Generates LLVM IR for functions, expressions, and statements
 * 4. Emits IR to file or compiles to native object code
 *
 * Example usage:
 *   LLVMCodegen codegen("my_module");
 *   codegen.generate(program, analyzer);
 *   codegen.emitLLVMIR("output.ll");
 *   codegen.compileToObject("output.o");
 */
class LLVMCodegen {
public:
    /**
     * Constructs a new LLVM code generator
     *
     * @param moduleName The name for the LLVM module (typically the source filename)
     */
    explicit LLVMCodegen(const std::string& moduleName);
    ~LLVMCodegen();

    /**
     * Generate LLVM IR from a Volta program AST
     *
     * Walks the entire program AST and generates corresponding LLVM IR instructions.
     * Uses the semantic analyzer to resolve types and verify correctness.
     *
     * @param program The root AST node representing the entire Volta program
     * @param analyzer Semantic analyzer containing type information and symbol tables
     * @return true if generation succeeded, false if errors occurred
     */
    bool generate(const volta::ast::Program* program,
                  const volta::semantic::SemanticAnalyzer* analyzer);

    /**
     * Write the generated LLVM IR to a text file
     *
     * Outputs human-readable LLVM IR in textual format (.ll file).
     * Useful for debugging and understanding the generated code.
     *
     * @param filename Output path for the .ll file
     * @return true if write succeeded, false on I/O error
     */
    bool emitLLVMIR(const std::string& filename);

    /**
     * Compile the LLVM IR to a native object file
     *
     * Uses LLVM's backend to generate optimized machine code for the target
     * architecture. The resulting object file can be linked with other objects
     * or passed to a system linker to create an executable.
     *
     * @param filename Output path for the .o file
     * @return true if compilation succeeded, false on error
     */
    bool compileToObject(const std::string& filename);

    /**
     * Get the underlying LLVM module
     *
     * Provides direct access to the LLVM module for inspection, optimization passes,
     * or advanced manipulation. The module remains owned by this LLVMCodegen instance.
     *
     * @return Pointer to the LLVM module (non-owning)
     */
    llvm::Module* getModule() { return module_.get(); }

private:
    // ========================================================================
    // LLVM Core Infrastructure
    // ========================================================================
    std::unique_ptr<llvm::LLVMContext> context_;  ///< LLVM context for types and constants
    std::unique_ptr<llvm::Module> module_;         ///< LLVM module containing all generated IR
    std::unique_ptr<llvm::IRBuilder<>> builder_;   ///< IR builder for emitting instructions
    std::unique_ptr<llvm::IRBuilder<>> allocaBuilder_;  ///< IR builder for entry block allocas

    // ========================================================================
    // Runtime Functions (Interface to Volta runtime library)
    // ========================================================================
    std::unique_ptr<RuntimeFunctions> runtime_;    ///< All runtime function declarations

    // ========================================================================
    // Semantic Analysis Context (Read-only, provided by frontend)
    // ========================================================================
    const volta::semantic::SemanticAnalyzer* analyzer_;  ///< Type information provider

    // ========================================================================
    // Codegen State (Mutable, updated during code generation)
    // ========================================================================
    llvm::Function* currentFunction_;  ///< Function currently being generated

    /// Maps variable names to their LLVM values in the current function scope.
    /// For parameters: stores the Value* directly (no alloca needed)
    /// For locals: stores the alloca pointer (use CreateLoad/CreateStore)
    std::unordered_map<std::string, llvm::Value*> namedValues_;

    /// Loop state for break/continue (stack for nested loops)
    struct LoopState {
        llvm::BasicBlock* conditionBlock;  ///< Where to jump for continue
        llvm::BasicBlock* exitBlock;       ///< Where to jump for break
    };
    std::vector<LoopState> loopStack_;

    // Error tracking
    bool hasErrors_;                      ///< Whether any errors occurred during codegen
    std::vector<std::string> errors_;     ///< Accumulated error messages

    // ========================================================================
    // Top-Level Code Generation
    // ========================================================================

    /**
     * Generate LLVM IR for a function declaration
     *
     * Entry point for function codegen. Creates LLVM function, sets up parameters,
     * generates the body, and verifies the result.
     *
     * @param fn Volta function AST node
     */
    void generateFunction(const volta::ast::FnDeclaration* fn);

    // ========================================================================
    // Statement Generation
    // ========================================================================

    /**
     * Generate LLVM IR for a statement (dispatcher)
     *
     * Routes to the appropriate handler based on statement type.
     *
     * @param stmt Volta statement AST node
     */
    void generateStatement(const volta::ast::Statement* stmt);

    /**
     * Generate IR for a block of statements
     *
     * Simply iterates and generates each statement in sequence.
     *
     * @param block Block AST node containing multiple statements
     */
    void generateBlock(const volta::ast::Block* block);

    /**
     * Generate IR for an expression statement
     *
     * Evaluates expression for side effects, discards result.
     *
     * @param stmt Expression statement AST node
     */
    void generateExpressionStatement(const volta::ast::ExpressionStatement* stmt);

    /**
     * Generate IR for a variable declaration (let/mut)
     *
     * Creates an alloca, generates the initializer, stores the value.
     *
     * @param stmt Variable declaration statement
     */
    void generateVarDeclaration(const volta::ast::VarDeclaration* stmt);

    /**
     * Generate IR for an if/else statement
     *
     * Creates basic blocks for then/else/merge, generates condition and branches.
     *
     * @param stmt If statement AST node
     */
    void generateIfStatement(const volta::ast::IfStatement* stmt);

    /**
     * Generate IR for a while loop
     *
     * Creates basic blocks for condition/body/exit, handles break/continue.
     *
     * @param stmt While statement AST node
     */
    void generateWhileStatement(const volta::ast::WhileStatement* stmt);

    /**
     * Generate IR for a for loop
     *
     * Desugars into while loop over iterator/range.
     *
     * @param stmt For statement AST node
     */
    void generateForStatement(const volta::ast::ForStatement* stmt);

    void generateForRangeLoop(const volta::ast::ForStatement* stmt, const volta::ast::BinaryExpression* binExpr);

    void generateForArrayLoop(const volta::ast::ForStatement* stmt);

    /**
     * Generate IR for a return statement
     *
     * Evaluates the return expression and emits a ret instruction.
     *
     * @param stmt Return statement AST node
     */
    void generateReturnStatement(const volta::ast::ReturnStatement* stmt);

    /**
     * Generate IR for a break statement
     *
     * Branches to loop exit block.
     *
     * @param stmt Break statement AST node
     */
    void generateBreakStatement(const volta::ast::BreakStatement* stmt);

    /**
     * Generate IR for a continue statement
     *
     * Branches to loop condition block.
     *
     * @param stmt Continue statement AST node
     */
    void generateContinueStatement(const volta::ast::ContinueStatement* stmt);

    // ========================================================================
    // Expression Generation
    // ========================================================================

    /**
     * Generate LLVM IR for an expression (dispatcher)
     *
     * Recursively evaluates expressions and returns the resulting LLVM value.
     * Routes to specific handlers based on expression type.
     *
     * @param expr Volta expression AST node
     * @return LLVM value representing the computed result (nullptr on error)
     */
    llvm::Value* generateExpression(const volta::ast::Expression* expr);

    // ------------------------------------------------------------------------
    // Literal Expressions
    // ------------------------------------------------------------------------

    /**
     * Generate IR for an integer literal
     *
     * Uses semantic analyzer to determine exact type (i8/i16/i32/i64/u8/etc.).
     *
     * @param lit Integer literal AST node
     * @return LLVM constant integer
     */
    llvm::Value* generateIntegerLiteral(const volta::ast::IntegerLiteral* lit);

    /**
     * Generate IR for a float literal
     *
     * Uses semantic analyzer to determine exact type (f32/f64).
     *
     * @param lit Float literal AST node
     * @return LLVM constant float
     */
    llvm::Value* generateFloatLiteral(const volta::ast::FloatLiteral* lit);

    /**
     * Generate IR for a string literal
     *
     * Creates a global string constant.
     *
     * @param lit String literal AST node
     * @return Pointer to string data (i8*)
     */
    llvm::Value* generateStringLiteral(const volta::ast::StringLiteral* lit);

    /**
     * Generate IR for a boolean literal
     *
     * @param lit Boolean literal AST node
     * @return LLVM i1 constant (0 or 1)
     */
    llvm::Value* generateBooleanLiteral(const volta::ast::BooleanLiteral* lit);

    /**
     * Generate IR for an array literal
     *
     * Allocates VoltaArray, initializes with elements.
     *
     * @param lit Array literal AST node
     * @return Pointer to VoltaArray*
     */
    llvm::Value* generateArrayLiteral(const volta::ast::ArrayLiteral* lit);

    /**
     * Generate IR for a tuple literal
     *
     * Creates a struct with the tuple elements.
     *
     * @param lit Tuple literal AST node
     * @return LLVM struct value
     */
    llvm::Value* generateTupleLiteral(const volta::ast::TupleLiteral* lit);

    /**
     * Generate IR for a struct literal
     *
     * Allocates struct, initializes fields.
     *
     * @param lit Struct literal AST node
     * @return Pointer to struct
     */
    llvm::Value* generateStructLiteral(const volta::ast::StructLiteral* lit);

    // ------------------------------------------------------------------------
    // Variable & Identifier Expressions
    // ------------------------------------------------------------------------

    /**
     * Generate IR for a variable/identifier reference
     *
     * Looks up the variable in namedValues_.
     * For parameters: returns value directly.
     * For locals: loads from alloca.
     *
     * @param expr Identifier expression AST node
     * @return LLVM value
     */
    llvm::Value* generateIdentifierExpression(const volta::ast::IdentifierExpression* expr);

    // ------------------------------------------------------------------------
    // Operator Expressions
    // ------------------------------------------------------------------------

    /**
     * Generate IR for a binary operation
     *
     * Handles arithmetic (+, -, *, /, %), comparisons (==, <, >, etc.),
     * logical operators (&&, ||), and assignments (=, +=, etc.).
     *
     * For numeric types: uses LLVM arithmetic/comparison instructions.
     * For strings: calls runtime functions.
     *
     * @param expr Binary expression AST node
     * @return LLVM value representing the result
     */
    llvm::Value* generateBinaryExpression(const volta::ast::BinaryExpression* expr);

    /**
     * Generate IR for a unary operation
     *
     * Handles negation (-) and logical not (!).
     *
     * @param expr Unary expression AST node
     * @return LLVM value representing the result
     */
    llvm::Value* generateUnaryExpression(const volta::ast::UnaryExpression* expr);

    // ------------------------------------------------------------------------
    // Function & Method Calls
    // ------------------------------------------------------------------------

    /**
     * Generate IR for a function call
     *
     * Looks up the function in the module, generates arguments,
     * creates call instruction.
     *
     * @param expr Call expression AST node
     * @return LLVM value (function return value)
     */
    llvm::Value* generateCallExpression(const volta::ast::CallExpression* expr);

    /**
     * Generate IR for a method call
     *
     * Similar to function call but passes object as first argument.
     *
     * @param expr Method call expression AST node
     * @return LLVM value (method return value)
     */
    llvm::Value* generateMethodCallExpression(const volta::ast::MethodCallExpression* expr);

    // ------------------------------------------------------------------------
    // Access Expressions
    // ------------------------------------------------------------------------

    /**
     * Generate IR for array/tuple indexing
     *
     * Generates bounds check, computes element pointer, loads value.
     *
     * @param expr Index expression AST node
     * @return LLVM value (element value)
     */
    llvm::Value* generateIndexExpression(const volta::ast::IndexExpression* expr);

    /**
     * Generate IR for array/string slicing
     *
     * Creates new array/string from slice range.
     *
     * @param expr Slice expression AST node
     * @return LLVM value (new array/string)
     */
    llvm::Value* generateSliceExpression(const volta::ast::SliceExpression* expr);

    /**
     * Generate IR for struct member access
     *
     * Computes field pointer using GEP, loads value.
     *
     * @param expr Member expression AST node
     * @return LLVM value (field value)
     */
    llvm::Value* generateMemberExpression(const volta::ast::MemberExpression* expr);

    // ------------------------------------------------------------------------
    // Control Flow Expressions
    // ------------------------------------------------------------------------

    /**
     * Generate IR for if expression
     *
     * Similar to if statement but returns a value.
     * Creates phi node to merge results from branches.
     *
     * @param expr If expression AST node
     * @return LLVM value (result of if expression)
     */
    llvm::Value* generateIfExpression(const volta::ast::IfExpression* expr);

    /**
     * Generate IR for match expression
     *
     * Pattern matching with exhaustiveness checking.
     * Generates cascading if-else or switch table.
     *
     * @param expr Match expression AST node
     * @return LLVM value (result of matched arm)
     */
    llvm::Value* generateMatchExpression(const volta::ast::MatchExpression* expr);

    // ------------------------------------------------------------------------
    // Advanced Expressions
    // ------------------------------------------------------------------------

    /**
     * Generate IR for lambda/anonymous function
     *
     * Creates a closure struct if captures exist, generates function.
     *
     * @param expr Lambda expression AST node
     * @return LLVM value (function pointer or closure struct)
     */
    llvm::Value* generateLambdaExpression(const volta::ast::LambdaExpression* expr);

    /**
     * Generate IR for type cast
     *
     * Numeric conversions, pointer casts, etc.
     *
     * @param expr Cast expression AST node
     * @return LLVM value (casted value)
     */
    llvm::Value* generateCastExpression(const volta::ast::CastExpression* expr);

    // ========================================================================
    // Type System
    // ========================================================================

    /**
     * Convert a Volta semantic type to an LLVM type
     *
     * This is the bridge between Volta's type system and LLVM's type system.
     * The semantic analyzer has already validated types - we just lower them.
     *
     * @param voltaType Volta semantic type (from analyzer)
     * @return Corresponding LLVM type
     */
    llvm::Type* lowerType(std::shared_ptr<volta::semantic::Type> voltaType);

    /**
     * Get the "larger" of two numeric types for implicit promotion
     *
     * Used in binary expressions to determine the common type.
     * Example: i32 + i64 → both promoted to i64
     *
     * @param t1 First LLVM type
     * @param t2 Second LLVM type
     * @return The promoted type
     */
    llvm::Type* getPromotedType(llvm::Type* t1, llvm::Type* t2);

    /**
     * Convert a value to a target type (int widening, sign extension, etc.)
     *
     * @param val Value to convert
     * @param targetType Target LLVM type
     * @param isSigned Whether to use sign extension (vs zero extension)
     * @return Converted value
     */
    llvm::Value* convertToType(llvm::Value* val, llvm::Type* targetType, bool isSigned);


    // Allocation utils 

    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn, std::string varName, llvm::Type* type);
    // ========================================================================
    // Runtime Function Access
    // ========================================================================
    // NOTE: Runtime functions are now managed by the RuntimeFunctions class.
    // Access them via: runtime_->getPrintInt(), runtime_->getStringEq(), etc.
    // No getter methods needed here anymore!

    // ========================================================================
    // Builtin Function Helpers
    // ========================================================================

    /**
     * Generate a call to the print() builtin
     *
     * Dispatches to the correct runtime function based on argument type.
     *
     * @param call Call expression for print(...)
     * @return Void (print returns nothing)
     */
    llvm::Value* generatePrintCall(const volta::ast::CallExpression* call);

    // ========================================================================
    // Error Reporting
    // ========================================================================

    /**
     * Record a codegen error
     *
     * @param message Error description
     */
    void reportError(const std::string& message);
};

}