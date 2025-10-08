#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "IR/Module.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/Value.hpp"
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "semantic/Type.hpp"

namespace volta::semantic {
    class SemanticAnalyzer;
}

namespace volta::ir {

/**
 * IRGenerator - Converts AST to IR
 *
 * Design Philosophy:
 * - Visitor Pattern: Traverses AST and generates IR
 * - Uses IRBuilder: High-level API for IR construction
 * - Symbol Table: Tracks variables and their IR values
 * - Type Lowering: Converts semantic types to IR types
 * - SSA Form: Generates SSA using alloca/load/store (mem2reg comes later)
 *
 * Key Concepts:
 * - AST → IR translation
 * - Expression evaluation produces Values
 * - Statements produce side effects (control flow, variables)
 * - Symbol table maps names to IR values (alloca pointers)
 * - Break/continue handled with basic blocks
 *
 * Usage:
 *   Module module("my_program");
 *   IRGenerator generator(module);
 *   generator.generate(astProgram);
 *   std::cout << module.toString();
 *
 * Learning Goals:
 * - Understand compiler frontend → backend bridge
 * - Master visitor pattern for AST traversal
 * - Learn SSA construction techniques
 * - Practice symbol table management
 * - See how high-level constructs lower to IR
 */
class IRGenerator {
public:
    /**
     * Create IR generator for a module
     * @param module The module to generate IR into
     */
    explicit IRGenerator(Module& module);

    /**
     * Create IR generator with semantic analyzer (for type information)
     * @param module The module to generate IR into
     * @param analyzer Semantic analyzer with type information
     */
    IRGenerator(Module& module, const volta::semantic::SemanticAnalyzer* analyzer);

    // ========================================================================
    // Top-Level Generation
    // ========================================================================

    /**
     * Generate IR for entire program
     * @param program The AST program node
     * @return true on success, false on error
     *
     * LEARNING TIP: This is the entry point. It generates IR for all
     * top-level declarations (functions, globals, etc.)
     */
    bool generate(const ast::Program* program);

    /**
     * Get the generated module
     */
    Module& getModule() { return module_; }
    const Module& getModule() const { return module_; }

    /**
     * Check if errors occurred during generation
     */
    bool hasErrors() const { return hasErrors_; }

    /**
     * Get error messages
     */
    const std::vector<std::string>& getErrors() const { return errors_; }

    // ========================================================================
    // Statement Generation
    // ========================================================================

    /**
     * Generate IR for a statement
     * @param stmt The statement to generate
     *
     * LEARNING TIP: Dispatch to specific statement handlers
     */
    void generateStatement(const ast::Statement* stmt);

    /**
     * Generate IR for function declaration
     * Creates IR function, generates parameters, generates body
     */
    void generateFunctionDecl(const ast::FnDeclaration* decl);

    /**
     * Generate IR for variable declaration
     * Creates alloca, evaluates initializer, stores value
     */
    void generateVarDecl(const ast::VarDeclaration* decl);

    /**
     * Generate IR for if statement
     * Creates conditional branches, merge blocks, and phi nodes if needed
     */
    void generateIfStmt(const ast::IfStatement* stmt);
    
    /**
     * Generate IR for while loop
     * Creates header, body, and exit blocks with appropriate branches
     */
    void generateWhileStmt(const ast::WhileStatement* stmt);

    /**
     * Generate IR for for loop
     * Desugars into while loop with iterator variable
     */
    void generateForStmt(const ast::ForStatement* stmt);

    /**
     * Generate IR for return statement
     * Creates return instruction, validates type
     */
    void generateReturnStmt(const ast::ReturnStatement* stmt);

    /**
     * Generate IR for break statement
     * Branches to loop exit block
     */
    void generateBreakStmt(const ast::BreakStatement* stmt);

    /**
     * Generate IR for continue statement
     * Branches to loop header block
     */
    void generateContinueStmt(const ast::ContinueStatement* stmt);

    /**
     * Generate IR for expression statement
     * Evaluates expression, discards result
     */
    void generateExprStmt(const ast::ExpressionStatement* stmt);

    /**
     * Generate IR for block (sequence of statements)
     * Creates new scope, generates statements, restores scope
     */
    void generateBlock(const ast::Block* block);

    // ========================================================================
    // Expression Generation
    // ========================================================================

    /**
     * Generate IR for an expression
     * @param expr The expression to generate
     * @return The IR value representing the expression result
     *
     * LEARNING TIP: Every expression produces a value!
     * This is the core of the visitor pattern for expressions.
     */
    Value* generateExpression(const ast::Expression* expr);

    /**
     * Generate IR for integer literal
     * @return ConstantInt value
     */
    Value* generateIntLiteral(const ast::IntegerLiteral* lit);

    /**
     * Generate IR for float literal
     * @return ConstantFloat value
     */
    Value* generateFloatLiteral(const ast::FloatLiteral* lit);

    /**
     * Generate IR for boolean literal
     * @return ConstantBool value
     */
    Value* generateBoolLiteral(const ast::BooleanLiteral* lit);

    /**
     * Generate IR for string literal
     * @return ConstantString value (or global variable)
     */
    Value* generateStringLiteral(const ast::StringLiteral* lit);

    // Note: Some and None are now enum variants, not special literals

    /**
     * Generate IR for array literal [1, 2, 3]
     * Allocates array, stores elements
     * @return Pointer to array
     */
    Value* generateArrayLiteral(const ast::ArrayLiteral* lit);
    Value* generateStructLiteral(const ast::StructLiteral* lit);

    /**
     * Generate IR for identifier (variable reference)
     * Looks up in symbol table, loads value
     * @return Loaded value
     *
     * LEARNING TIP: Variables are stored as allocas, so we need to load!
     */
    Value* generateIdentifier(const ast::IdentifierExpression* expr);

    /**
     * Generate IR for binary expression (a + b, a == b, etc.)
     * Generates left and right operands, creates appropriate instruction
     * @return Result value
     *
     * LEARNING TIP: Map AST operators to IR opcodes
     */
    Value* generateBinaryExpr(const ast::BinaryExpression* expr);

    /**
     * Generate IR for unary expression (-a, not a)
     * Generates operand, creates appropriate instruction
     * @return Result value
     */
    Value* generateUnaryExpr(const ast::UnaryExpression* expr);

    /**
     * Generate IR for function call
     * Generates arguments, creates call instruction
     * @return Call result (or nullptr for void functions)
     */
    Value* generateCallExpr(const ast::CallExpression* expr);

    /**
     * Generate IR for array index (arr[i])
     * Generates array and index, creates ArrayGet instruction
     * @return Element value
     */
    Value* generateIndexExpr(const ast::IndexExpression* expr);

    /**
     * Generate IR for member access (obj.member)
     * For structs, generates GEP or ExtractValue
     * @return Member value
     */
    Value* generateMemberExpr(const ast::MemberExpression* expr);
    Value* generateMethodCallExpr(const ast::MethodCallExpression* expr);
    Value* generateEnumConstruction(const ast::MethodCallExpression* expr);

    Value* generateCastExpr(const ast::CastExpression* expr);

    /**
     * Generate IR for assignment (x = 5, arr[i] = val, etc.)
     * Generates lvalue and rvalue, creates store
     * @return Assigned value
     *
     * LEARNING TIP: Assignment is tricky! Need to handle:
     * - Simple variable assignment (x = 5)
     * - Array element assignment (arr[i] = 5)
     * - Field assignment (obj.field = 5)
     */
    Value* generateAssignment(const ast::BinaryExpression* expr);

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * Look up variable in symbol table
     * @param name Variable name
     * @return IR value (alloca pointer), or nullptr if not found
     *
     * LEARNING TIP: The symbol table stores allocas, not values!
     * You need to load from the alloca to get the actual value.
     */
    Value* lookupVariable(const std::string& name);

    /**
     * Declare variable in symbol table
     * @param name Variable name
     * @param value IR value (typically an alloca)
     *
     * LEARNING TIP: This associates a name with an alloca.
     * Later, we'll use mem2reg to convert allocas to SSA values.
     */
    void declareVariable(const std::string& name, Value* value);

    /**
     * Enter new scope (for blocks, functions)
     * Creates new symbol table frame
     */
    void enterScope();

    /**
     * Exit current scope
     * Restores previous symbol table frame
     */
    void exitScope();

    /**
     * Convert semantic type to IR type
     * @param semType Semantic type from type checker
     * @return IR type
     *
     * LEARNING TIP: Delegates to TypeLowering::lower()
     */
    std::shared_ptr<IRType> lowerType(std::shared_ptr<semantic::Type> semType);

    /**
     * Get current function being generated
     * @return Current function, or nullptr if at global scope
     */
    Function* getCurrentFunction() const;

    /**
     * Map AST binary operator to IR opcode
     * @param op AST operator
     * @return IR opcode
     *
     * LEARNING TIP: This is where you translate Add → Opcode::Add, etc.
     */
    Instruction::Opcode mapBinaryOp(ast::BinaryExpression::Operator op);

    /**
     * Map AST unary operator to IR opcode
     * @param op AST operator
     * @return IR opcode
     */
    Instruction::Opcode mapUnaryOp(ast::UnaryExpression::Operator op);

    /**
     * Report error during generation
     * @param message Error message
     */
    void reportError(const std::string& message);

    /**
     * Check if current block is terminated
     * @return true if block has terminator (can't insert more instructions)
     *
     * LEARNING TIP: After return/break/continue, the block is terminated.
     * Don't try to insert more instructions!
     */
    bool isBlockTerminated() const;

    // ========================================================================
    // Control Flow Helpers
    // ========================================================================

    /**
     * Get current break target (for break statement in loops)
     * @return Basic block to jump to on break
     */
    BasicBlock* getBreakTarget() const;

    /**
     * Get current continue target (for continue statement in loops)
     * @return Basic block to jump to on continue
     */
    BasicBlock* getContinueTarget() const;

    /**
     * Set break/continue targets (when entering loop)
     */
    void setLoopTargets(BasicBlock* breakTarget, BasicBlock* continueTarget);

    /**
     * Clear break/continue targets (when exiting loop)
     */
    void clearLoopTargets();

private:
    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Mangle function name for overload resolution
     * Example: print(int) -> print_i64, print(str) -> print_str
     */
    std::string mangleFunctionName(const std::string& name,
                                   const std::vector<std::shared_ptr<IRType>>& paramTypes);

    // ========================================================================
    // State
    // ========================================================================

    Module& module_;                              // The module we're generating into
    IRBuilder builder_;                           // IR builder for construction

    // Symbol table (stack of scopes)
    // Each scope maps variable names to IR values (allocas)
    std::vector<std::unordered_map<std::string, Value*>> symbolTable_;

    // Loop control flow
    BasicBlock* breakTarget_;                     // Where to jump on break
    BasicBlock* continueTarget_;                  // Where to jump on continue

    // Error tracking
    bool hasErrors_;
    std::vector<std::string> errors_;

    // Current function being generated
    Function* currentFunction_;

    // Optional semantic analyzer (for type information)
    const volta::semantic::SemanticAnalyzer* analyzer_;

    // Cache of generic method AST nodes for monomorphization
    // Key: base method name (e.g., "Box.new"), Value: AST node
    std::unordered_map<std::string, const ast::FnDeclaration*> genericMethodCache_;

    // Helper to generate a monomorphized version of a generic method
    Function* generateMonomorphizedMethod(
        const std::string& baseName,
        const std::vector<std::shared_ptr<volta::semantic::Type>>& typeArgs);
};

// ========================================================================
// Standalone API - Generate IR from AST + Semantic Analysis
// ========================================================================

/**
 * Generate IR module from type-checked AST
 * @param program The AST program
 * @param analyzer Semantic analyzer with type information
 * @param moduleName Name for the IR module
 * @return IR Module (nullptr on error)
 */
std::unique_ptr<Module> generateIR(
    const volta::ast::Program& program,
    const volta::semantic::SemanticAnalyzer& analyzer,
    const std::string& moduleName = "program"
);

} // namespace volta::ir
