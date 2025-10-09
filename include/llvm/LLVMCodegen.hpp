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
    // LLVM core infrastructure
    std::unique_ptr<llvm::LLVMContext> context_;  ///< LLVM context for types and constants
    std::unique_ptr<llvm::Module> module_;         ///< LLVM module containing all generated IR
    std::unique_ptr<llvm::IRBuilder<>> builder_;   ///< IR builder for emitting instructions

    /**
     * Generate LLVM IR for a function declaration
     *
     * Creates LLVM function signature and basic blocks, then generates IR for the body.
     * Handles parameter setup, local variables, and control flow.
     *
     * @param fn Volta function AST node
     */
    void generateFunction(const volta::ast::FnDeclaration* fn);

    /**
     * Generate LLVM IR for an expression
     *
     * Recursively evaluates expressions and returns the resulting LLVM value.
     * Handles literals, variables, binary ops, calls, and complex expressions.
     *
     * @param expr Volta expression AST node
     * @return LLVM value representing the computed result
     */

    llvm::Value* generateExpression(const volta::ast::Expression* expr);

    llvm::Value* generateBinaryExpression(const volta::ast::BinaryExpression* expr);

    llvm::Value* generateUnaryExpression(const volta::ast::UnaryExpression* expr);
    
    llvm::Value* generateCallExpression(const volta::ast::CallExpression* call);
    

    // Type promotion utils
    llvm::Type* getPromotedType(llvm::Type* t1, llvm::Type* t2);
    llvm::Value* convertToType(llvm::Value* val, llvm::Type* targetType, bool isSigned);



    llvm::Value* generatePrintCall(const volta::ast::CallExpression* call);

    llvm::Value* generateStringLiteral(const volta::ast::StringLiteral* lit);


    llvm::Function* getPrintIntFunction();
    llvm::Function* getPrintDoubleFunction();
    llvm::Function* getPrintStringFunction();

    /**
     * Generate LLVM IR for a statement
     *
     * Emits IR for control flow, variable declarations, assignments, and other statements.
     * May create new basic blocks for branching and loops.
     *
     * @param stmt Volta statement AST node
     */
    void generateStatement(const volta::ast::Statement* stmt);

    void generateReturn(const volta::ast::ReturnStatement* stmt);

    /**
     * Convert a Volta semantic type to an LLVM type
     *
     * Maps Volta's type system to LLVM's type system:
     * - i8/i16/i32/i64 → LLVM integer types
     * - f32/f64 → LLVM float/double
     * - bool → i1
     * - structs → LLVM struct types
     * - arrays → LLVM pointers
     *
     * @param voltaType Volta semantic type
     * @return Corresponding LLVM type
     */
    llvm::Type* lowerType(std::shared_ptr<volta::semantic::Type> voltaType);

    // Symbol tables for name resolution during codegen
    std::unordered_map<std::string, llvm::Value*> namedValues_;    ///< Local variables in current scope
    std::unordered_map<std::string, llvm::Function*> functions_;   ///< All declared functions

    llvm::Function* currentFunction_;  ///< Function currently being generated

    const volta::semantic::SemanticAnalyzer* analyzer_;  ///< Type information provider

    // Error tracking
    bool hasErrors_;                      ///< Whether any errors occurred during codegen
    std::vector<std::string> errors_;     ///< Accumulated error messages

    /**
     * Record a codegen error
     *
     * @param message Error description
     */
    void reportError(const std::string& message);
};

}