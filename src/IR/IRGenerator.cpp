#include "IR/IRGenerator.hpp"

namespace volta::ir {

IRGenerator::IRGenerator(volta::errors::ErrorReporter& reporter)
    : reporter_(reporter),
      analyzer_(nullptr),
      module_(nullptr),
      currentFunction_(nullptr),
      currentBlock_(nullptr),
      hasErrors_(false) {
}

// ============================================================================
// Main Entry Point
// ============================================================================

std::unique_ptr<IRModule> IRGenerator::generate(
    const volta::ast::Program& program,
    const volta::semantic::SemanticAnalyzer& analyzer
) {
    // TODO: This is the main entry point for Phase 4.
    // You'll implement this after completing Phases 1-3.
    //
    // Steps:
    // 1. Store analyzer reference: analyzer_ = &analyzer;
    // 2. Create IRModule: module_ = std::make_unique<IRModule>("program");
    // 3. Generate all global variables
    // 4. Generate all function declarations
    // 5. Generate all function bodies
    // 6. Return module_ (or nullptr if hasErrors_)

    return nullptr;
}

// ============================================================================
// Top-Level Generation
// ============================================================================

void IRGenerator::generateProgram(const volta::ast::Program& program) {
    // TODO: Walk through program.statements and generate IR for each.
    // Separate into:
    // - Global variables (VarDeclaration at top level)
    // - Functions (FnDeclaration)
    // - Structs (StructDeclaration)
}

void IRGenerator::generateGlobalVariable(const volta::ast::VarDeclaration& varDecl) {
    // TODO: Create a GlobalVariable in the module.
    // Example:
    //   Volta: counter: mut int = 0
    //   IR:    @counter = global i64 0
}

void IRGenerator::generateFunction(const volta::ast::FnDeclaration& fnDecl) {
    // TODO: Create a Function in the module.
    // Steps:
    // 1. Create parameters from fnDecl.parameters
    // 2. Create Function with name, params, return type
    // 3. Generate function body (generateBlock)
    // 4. Use SSABuilder for proper SSA form
}

void IRGenerator::generateStruct(const volta::ast::StructDeclaration& structDecl) {
    // TODO: Handle struct declarations.
    // For Phase 4, you might want to defer this and focus on functions first.
}

// ============================================================================
// Statement Generation
// ============================================================================

void IRGenerator::generateStatement(const volta::ast::Statement* stmt) {
    // TODO: Dispatch to appropriate generator based on statement type.
    // Hint: Use dynamic_cast or visitor pattern
}

void IRGenerator::generateBlock(const volta::ast::Block* block) {
    // TODO: Generate IR for each statement in the block.
}

void IRGenerator::generateVarDeclaration(const volta::ast::VarDeclaration* varDecl) {
    // TODO: Generate local variable declaration.
    // In LLVM style:
    // 1. Create alloca for the variable
    // 2. Generate initializer expression
    // 3. Store initializer into alloca
    // 4. Track variable name -> alloca pointer
}

void IRGenerator::generateIfStatement(const volta::ast::IfStatement* ifStmt) {
    // TODO: Generate if-statement control flow.
    // Create basic blocks:
    //   - then block
    //   - else block (if present)
    //   - merge block (continuation)
    // Generate condition, conditional branch, then/else bodies
}

void IRGenerator::generateWhileStatement(const volta::ast::WhileStatement* whileStmt) {
    // TODO: Generate while-loop control flow.
    // Create basic blocks:
    //   - header (evaluate condition)
    //   - body (loop body)
    //   - exit (after loop)
}

void IRGenerator::generateForStatement(const volta::ast::ForStatement* forStmt) {
    // TODO: Generate for-loop control flow.
    // Similar to while loop, but with iterator setup.
}

void IRGenerator::generateReturnStatement(const volta::ast::ReturnStatement* returnStmt) {
    // TODO: Generate return instruction.
    // If returnStmt->expression is null, use createRetVoid()
    // Otherwise, generate expression and createRet(value)
}

void IRGenerator::generateExpressionStatement(const volta::ast::ExpressionStatement* exprStmt) {
    // TODO: Generate expression and discard result (if any).
}

// ============================================================================
// Expression Generation
// ============================================================================

Value* IRGenerator::generateExpression(const volta::ast::Expression* expr) {
    // TODO: Dispatch to appropriate generator based on expression type.
    // Return the SSA value representing the expression result.
    return nullptr;
}

Value* IRGenerator::generateBinaryExpression(const volta::ast::BinaryExpression* binExpr) {
    // TODO: Generate binary operation.
    // Steps:
    // 1. Generate left operand
    // 2. Generate right operand
    // 3. Create appropriate instruction (add, mul, icmp, etc.)
    return nullptr;
}

Value* IRGenerator::generateUnaryExpression(const volta::ast::UnaryExpression* unaryExpr) {
    // TODO: Generate unary operation (not, negate).
    return nullptr;
}

Value* IRGenerator::generateCallExpression(const volta::ast::CallExpression* callExpr) {
    // TODO: Generate function call.
    // Steps:
    // 1. Generate callee expression (should be a function)
    // 2. Generate all argument expressions
    // 3. Create call instruction
    return nullptr;
}

Value* IRGenerator::generateIdentifier(const volta::ast::IdentifierExpression* identifier) {
    // TODO: Look up identifier and return its value.
    // Check:
    // 1. Is it a local variable? Load from alloca
    // 2. Is it a global variable? Load from global
    // 3. Is it a function parameter? Use parameter value directly
    return nullptr;
}

Value* IRGenerator::generateLiteral(const volta::ast::Expression* literal) {
    // TODO: Generate constant literal.
    // Examples:
    //   IntegerLiteral -> Constant::getInt(value)
    //   FloatLiteral -> Constant::getFloat(value)
    //   BooleanLiteral -> Constant::getBool(value)
    //   StringLiteral -> Constant::getString(value)
    return nullptr;
}

Value* IRGenerator::generateIfExpression(const volta::ast::IfExpression* ifExpr) {
    // TODO: Generate if-expression (similar to if-statement, but returns a value).
    // Must create phi node in merge block to combine then/else results.
    return nullptr;
}

Value* IRGenerator::generateMatchExpression(const volta::ast::MatchExpression* matchExpr) {
    // TODO: Generate match expression (pattern matching).
    // This is advanced - implement after basic features work.
    return nullptr;
}

Value* IRGenerator::generateLambdaExpression(const volta::ast::LambdaExpression* lambdaExpr) {
    // TODO: Generate lambda/closure.
    // This requires closure conversion - implement after basic features.
    return nullptr;
}

Value* IRGenerator::generateArrayLiteral(const volta::ast::ArrayLiteral* arrayLit) {
    // TODO: Generate array literal.
    // Allocate array, store elements.
    return nullptr;
}

Value* IRGenerator::generateStructLiteral(const volta::ast::StructLiteral* structLit) {
    // TODO: Generate struct literal.
    // Allocate struct, initialize fields.
    return nullptr;
}

Value* IRGenerator::generateMemberExpression(const volta::ast::MemberExpression* memberExpr) {
    // TODO: Generate member access (struct.field).
    // Use GEP (GetElementPtr) instruction.
    return nullptr;
}

Value* IRGenerator::generateIndexExpression(const volta::ast::IndexExpression* indexExpr) {
    // TODO: Generate array indexing (array[index]).
    // Use GEP instruction.
    return nullptr;
}

Value* IRGenerator::generateMethodCall(const volta::ast::MethodCallExpression* methodCall) {
    // TODO: Generate method call (object.method(args)).
    // Desugar to regular function call with object as first argument.
    return nullptr;
}

// ============================================================================
// Helpers
// ============================================================================

std::shared_ptr<semantic::Type> IRGenerator::getType(const volta::ast::Expression* expr) {
    // TODO: Get the type of an expression from semantic analysis.
    // Use analyzer_->getExpressionType(expr)
    return nullptr;
}

void IRGenerator::error(const std::string& message, volta::errors::SourceLocation loc) {
    // TODO: Report an error.
    hasErrors_ = true;
    reporter_.error(message, loc);
}

} // namespace volta::ir
