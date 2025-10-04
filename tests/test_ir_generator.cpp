#include <gtest/gtest.h>
#include <memory>
#include "IR/IRGenerator.hpp"
#include "IR/Module.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "error/ErrorTypes.hpp"

using namespace volta::ir;
using namespace volta::ast;
using namespace volta::errors;

// Helper to create source location
SourceLocation loc() {
    return SourceLocation{"test.volta", 1, 1};
}

// ============================================================================
// Test Fixture
// ============================================================================

class IRGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("test_module");
        generator = std::make_unique<IRGenerator>(*module);
    }

    std::unique_ptr<Module> module;
    std::unique_ptr<IRGenerator> generator;
};

// ============================================================================
// Literal Expression Tests
// ============================================================================

TEST_F(IRGeneratorTest, IntegerLiteral) {
    IntegerLiteral lit(42, loc());

    auto* result = generator->generateIntLiteral(&lit);

    ASSERT_NE(result, nullptr);
    auto* constant = dyn_cast<ConstantInt>(result);
    ASSERT_NE(constant, nullptr);
    EXPECT_EQ(constant->getValue(), 42);
}

TEST_F(IRGeneratorTest, FloatLiteral) {
    FloatLiteral lit(3.14, loc());

    auto* result = generator->generateFloatLiteral(&lit);

    ASSERT_NE(result, nullptr);
    auto* constant = dyn_cast<ConstantFloat>(result);
    ASSERT_NE(constant, nullptr);
    EXPECT_DOUBLE_EQ(constant->getValue(), 3.14);
}

TEST_F(IRGeneratorTest, BooleanLiteral_True) {
    BooleanLiteral lit(true, loc());

    auto* result = generator->generateBoolLiteral(&lit);

    ASSERT_NE(result, nullptr);
    auto* constant = dyn_cast<ConstantBool>(result);
    ASSERT_NE(constant, nullptr);
    EXPECT_TRUE(constant->getValue());
}

TEST_F(IRGeneratorTest, BooleanLiteral_False) {
    BooleanLiteral lit(false, loc());

    auto* result = generator->generateBoolLiteral(&lit);

    ASSERT_NE(result, nullptr);
    auto* constant = dyn_cast<ConstantBool>(result);
    ASSERT_NE(constant, nullptr);
    EXPECT_FALSE(constant->getValue());
}

TEST_F(IRGeneratorTest, StringLiteral) {
    StringLiteral lit("hello world", loc());

    auto* result = generator->generateStringLiteral(&lit);

    ASSERT_NE(result, nullptr);
    // String could be ConstantString or GlobalVariable
}

TEST_F(IRGeneratorTest, NoneLiteral) {
    NoneLiteral lit(loc());

    auto* result = generator->generateNoneLiteral(&lit);

    ASSERT_NE(result, nullptr);
    auto* constant = dyn_cast<ConstantNone>(result);
    ASSERT_NE(constant, nullptr);
}

// ============================================================================
// Binary Expression Tests
// ============================================================================

TEST_F(IRGeneratorTest, BinaryOp_Mapping_Add) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::Add);
    EXPECT_EQ(op, Instruction::Opcode::Add);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_Subtract) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::Subtract);
    EXPECT_EQ(op, Instruction::Opcode::Sub);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_Multiply) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::Multiply);
    EXPECT_EQ(op, Instruction::Opcode::Mul);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_Divide) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::Divide);
    EXPECT_EQ(op, Instruction::Opcode::Div);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_Equal) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::Equal);
    EXPECT_EQ(op, Instruction::Opcode::Eq);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_NotEqual) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::NotEqual);
    EXPECT_EQ(op, Instruction::Opcode::Ne);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_Less) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::Less);
    EXPECT_EQ(op, Instruction::Opcode::Lt);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_LogicalAnd) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::LogicalAnd);
    EXPECT_EQ(op, Instruction::Opcode::And);
}

TEST_F(IRGeneratorTest, BinaryOp_Mapping_LogicalOr) {
    auto op = generator->mapBinaryOp(BinaryExpression::Operator::LogicalOr);
    EXPECT_EQ(op, Instruction::Opcode::Or);
}

// ============================================================================
// Unary Expression Tests
// ============================================================================

TEST_F(IRGeneratorTest, UnaryOp_Mapping_Negate) {
    auto op = generator->mapUnaryOp(UnaryExpression::Operator::Negate);
    EXPECT_EQ(op, Instruction::Opcode::Neg);
}

TEST_F(IRGeneratorTest, UnaryOp_Mapping_Not) {
    auto op = generator->mapUnaryOp(UnaryExpression::Operator::Not);
    EXPECT_EQ(op, Instruction::Opcode::Not);
}

// ============================================================================
// Symbol Table Tests
// ============================================================================

TEST_F(IRGeneratorTest, SymbolTable_Lookup_NotFound) {
    auto* result = generator->lookupVariable("nonexistent");
    EXPECT_EQ(result, nullptr);
}

TEST_F(IRGeneratorTest, SymbolTable_DeclareAndLookup) {
    generator->enterScope();

    // Create a dummy value (in real code, this would be an alloca)
    auto* dummyValue = ConstantInt::get(42, std::make_shared<IRPrimitiveType>(IRType::Kind::I64));

    generator->declareVariable("x", dummyValue);
    auto* result = generator->lookupVariable("x");

    EXPECT_EQ(result, dummyValue);

    generator->exitScope();
}

TEST_F(IRGeneratorTest, SymbolTable_NestedScopes) {
    generator->enterScope();
    auto* outer = ConstantInt::get(1, std::make_shared<IRPrimitiveType>(IRType::Kind::I64));
    generator->declareVariable("x", outer);

    generator->enterScope();
    auto* inner = ConstantInt::get(2, std::make_shared<IRPrimitiveType>(IRType::Kind::I64));
    generator->declareVariable("x", inner);

    // Inner scope should shadow outer
    EXPECT_EQ(generator->lookupVariable("x"), inner);

    generator->exitScope();

    // Back to outer scope
    EXPECT_EQ(generator->lookupVariable("x"), outer);

    generator->exitScope();
}

TEST_F(IRGeneratorTest, SymbolTable_LookupOuter) {
    generator->enterScope();
    auto* outer = ConstantInt::get(1, std::make_shared<IRPrimitiveType>(IRType::Kind::I64));
    generator->declareVariable("x", outer);

    generator->enterScope();
    // Don't declare x in inner scope

    // Should find x from outer scope
    EXPECT_EQ(generator->lookupVariable("x"), outer);

    generator->exitScope();
    generator->exitScope();
}

// ============================================================================
// Error Tracking Tests
// ============================================================================

TEST_F(IRGeneratorTest, ErrorTracking_InitiallyNoErrors) {
    EXPECT_FALSE(generator->hasErrors());
    EXPECT_EQ(generator->getErrors().size(), 0);
}

TEST_F(IRGeneratorTest, ErrorTracking_ReportError) {
    generator->reportError("Test error");

    EXPECT_TRUE(generator->hasErrors());
    EXPECT_EQ(generator->getErrors().size(), 1);
    EXPECT_EQ(generator->getErrors()[0], "Test error");
}

TEST_F(IRGeneratorTest, ErrorTracking_MultipleErrors) {
    generator->reportError("Error 1");
    generator->reportError("Error 2");
    generator->reportError("Error 3");

    EXPECT_TRUE(generator->hasErrors());
    EXPECT_EQ(generator->getErrors().size(), 3);
}

// ============================================================================
// Loop Target Tests
// ============================================================================

TEST_F(IRGeneratorTest, LoopTargets_InitiallyNull) {
    EXPECT_EQ(generator->getBreakTarget(), nullptr);
    EXPECT_EQ(generator->getContinueTarget(), nullptr);
}

TEST_F(IRGeneratorTest, LoopTargets_SetAndGet) {
    auto* breakBlock = module->createBasicBlock("break");
    auto* continueBlock = module->createBasicBlock("continue");

    generator->setLoopTargets(breakBlock, continueBlock);

    EXPECT_EQ(generator->getBreakTarget(), breakBlock);
    EXPECT_EQ(generator->getContinueTarget(), continueBlock);
}

TEST_F(IRGeneratorTest, LoopTargets_Clear) {
    auto* breakBlock = module->createBasicBlock("break");
    auto* continueBlock = module->createBasicBlock("continue");

    generator->setLoopTargets(breakBlock, continueBlock);
    generator->clearLoopTargets();

    EXPECT_EQ(generator->getBreakTarget(), nullptr);
    EXPECT_EQ(generator->getContinueTarget(), nullptr);
}

// ============================================================================
// Integration Tests - Simple Programs
// ============================================================================

TEST_F(IRGeneratorTest, Integration_EmptyProgram) {
    // Create empty program
    std::vector<std::unique_ptr<Statement>> statements;
    Program program(std::move(statements), loc());

    bool success = generator->generate(&program);

    // Should succeed even with empty program
    EXPECT_FALSE(generator->hasErrors());
}

TEST_F(IRGeneratorTest, Integration_SimpleFunction) {
    // Create: fn main() -> void { }

    // Note: This test will need actual semantic types
    // For now, just test the structure is set up

    auto* func = generator->getCurrentFunction();
    EXPECT_EQ(func, nullptr);  // No function being generated yet
}

// ============================================================================
// Helper Tests
// ============================================================================

TEST_F(IRGeneratorTest, GetModule) {
    auto& mod = generator->getModule();
    EXPECT_EQ(&mod, module.get());
}

TEST_F(IRGeneratorTest, GetModule_Const) {
    const auto& constGen = *generator;
    const auto& mod = constGen.getModule();
    EXPECT_EQ(&mod, module.get());
}

TEST_F(IRGeneratorTest, IsBlockTerminated_NoInsertionPoint) {
    // No insertion point means we're not in a block
    bool terminated = generator->isBlockTerminated();
    // Implementation-dependent, but should handle gracefully
}

// ============================================================================
// Type Lowering Tests
// ============================================================================

TEST_F(IRGeneratorTest, TypeLowering_IntType) {
    auto semType = std::make_shared<volta::semantic::PrimitiveType>(
        volta::semantic::PrimitiveType::PrimitiveKind::Int);
    auto irType = generator->lowerType(semType);

    ASSERT_NE(irType, nullptr);
    EXPECT_EQ(irType->kind(), IRType::Kind::I64);
}

TEST_F(IRGeneratorTest, TypeLowering_FloatType) {
    auto semType = std::make_shared<volta::semantic::PrimitiveType>(
        volta::semantic::PrimitiveType::PrimitiveKind::Float);
    auto irType = generator->lowerType(semType);

    ASSERT_NE(irType, nullptr);
    EXPECT_EQ(irType->kind(), IRType::Kind::F64);
}

TEST_F(IRGeneratorTest, TypeLowering_BoolType) {
    auto semType = std::make_shared<volta::semantic::PrimitiveType>(
        volta::semantic::PrimitiveType::PrimitiveKind::Bool);
    auto irType = generator->lowerType(semType);

    ASSERT_NE(irType, nullptr);
    EXPECT_EQ(irType->kind(), IRType::Kind::I1);
}

TEST_F(IRGeneratorTest, TypeLowering_StringType) {
    auto semType = std::make_shared<volta::semantic::PrimitiveType>(
        volta::semantic::PrimitiveType::PrimitiveKind::String);
    auto irType = generator->lowerType(semType);

    ASSERT_NE(irType, nullptr);
    EXPECT_EQ(irType->kind(), IRType::Kind::String);
}

TEST_F(IRGeneratorTest, TypeLowering_VoidType) {
    auto semType = std::make_shared<volta::semantic::PrimitiveType>(
        volta::semantic::PrimitiveType::PrimitiveKind::Void);
    auto irType = generator->lowerType(semType);

    ASSERT_NE(irType, nullptr);
    EXPECT_EQ(irType->kind(), IRType::Kind::Void);
}

// ============================================================================
// Array Literal Tests
// ============================================================================

TEST_F(IRGeneratorTest, ArrayLiteral_Empty) {
    std::vector<std::unique_ptr<Expression>> elements;
    ArrayLiteral lit(std::move(elements), loc());

    auto* result = generator->generateArrayLiteral(&lit);

    // Should create empty array
    // Exact behavior depends on implementation
}

TEST_F(IRGeneratorTest, ArrayLiteral_WithElements) {
    std::vector<std::unique_ptr<Expression>> elements;
    elements.push_back(std::make_unique<IntegerLiteral>(1, loc()));
    elements.push_back(std::make_unique<IntegerLiteral>(2, loc()));
    elements.push_back(std::make_unique<IntegerLiteral>(3, loc()));

    ArrayLiteral lit(std::move(elements), loc());

    auto* result = generator->generateArrayLiteral(&lit);

    // Should create array and initialize elements
}

// ============================================================================
// Complex Integration Tests
// ============================================================================

TEST_F(IRGeneratorTest, Integration_ArithmeticExpression) {
    // Test: 2 + 3
    auto left = std::make_unique<IntegerLiteral>(2, loc());
    auto right = std::make_unique<IntegerLiteral>(3, loc());

    BinaryExpression expr(
        std::move(left),
        BinaryExpression::Operator::Add,
        std::move(right),
        loc()
    );

    // This would need IRBuilder set up properly
    // For now, just test structure
}

TEST_F(IRGeneratorTest, Integration_NestedArithmetic) {
    // Test: (2 + 3) * 4
    auto two = std::make_unique<IntegerLiteral>(2, loc());
    auto three = std::make_unique<IntegerLiteral>(3, loc());
    auto four = std::make_unique<IntegerLiteral>(4, loc());

    auto add = std::make_unique<BinaryExpression>(
        std::move(two),
        BinaryExpression::Operator::Add,
        std::move(three),
        loc()
    );

    BinaryExpression mul(
        std::move(add),
        BinaryExpression::Operator::Multiply,
        std::move(four),
        loc()
    );

    // Test structure
}

TEST_F(IRGeneratorTest, Integration_UnaryNegation) {
    // Test: -42
    auto operand = std::make_unique<IntegerLiteral>(42, loc());

    UnaryExpression expr(
        UnaryExpression::Operator::Negate,
        std::move(operand),
        loc()
    );

    // Test structure
}

TEST_F(IRGeneratorTest, Integration_LogicalNot) {
    // Test: not true
    auto operand = std::make_unique<BooleanLiteral>(true, loc());

    UnaryExpression expr(
        UnaryExpression::Operator::Not,
        std::move(operand),
        loc()
    );

    // Test structure
}

TEST_F(IRGeneratorTest, Integration_Comparison) {
    // Test: 5 < 10
    auto left = std::make_unique<IntegerLiteral>(5, loc());
    auto right = std::make_unique<IntegerLiteral>(10, loc());

    BinaryExpression expr(
        std::move(left),
        BinaryExpression::Operator::Less,
        std::move(right),
        loc()
    );

    // Test structure
}

// ============================================================================
// Statement Generation Tests (Structure Only)
// ============================================================================

TEST_F(IRGeneratorTest, ReturnStatement_WithValue) {
    auto value = std::make_unique<IntegerLiteral>(42, loc());
    ReturnStatement stmt(std::move(value), loc());

    // Test that it doesn't crash
    // Actual functionality depends on having a function context
}

TEST_F(IRGeneratorTest, ReturnStatement_Void) {
    ReturnStatement stmt(nullptr, loc());

    // Test that it doesn't crash
}

TEST_F(IRGeneratorTest, ExpressionStatement) {
    auto expr = std::make_unique<IntegerLiteral>(42, loc());
    ExpressionStatement stmt(std::move(expr), loc());

    // Test structure
}

TEST_F(IRGeneratorTest, Block_Empty) {
    std::vector<std::unique_ptr<Statement>> statements;
    Block block(std::move(statements), loc());

    // Test structure
}

TEST_F(IRGeneratorTest, Block_WithStatements) {
    std::vector<std::unique_ptr<Statement>> statements;

    auto expr1 = std::make_unique<IntegerLiteral>(1, loc());
    statements.push_back(std::make_unique<ExpressionStatement>(std::move(expr1), loc()));

    auto expr2 = std::make_unique<IntegerLiteral>(2, loc());
    statements.push_back(std::make_unique<ExpressionStatement>(std::move(expr2), loc()));

    Block block(std::move(statements), loc());

    // Test structure
}
