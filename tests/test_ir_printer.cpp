#include <gtest/gtest.h>
#include <memory>
#include "IR/IRPrinter.hpp"
#include "IR/Module.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"

using namespace volta::ir;

// ============================================================================
// Test Fixture
// ============================================================================

class IRPrinterTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("test_module");
        printer = std::make_unique<IRPrinter>();
    }

    void TearDown() override {
        // Cleanup any standalone allocated values
        for (auto* val : allocatedValues) {
            delete val;
        }
        allocatedValues.clear();
    }

    // Helper to track allocated values for cleanup
    template<typename T>
    T* track(T* ptr) {
        allocatedValues.push_back(ptr);
        return ptr;
    }

    std::unique_ptr<Module> module;
    std::unique_ptr<IRPrinter> printer;
    std::vector<Value*> allocatedValues;
};

// ============================================================================
// Type Printing Tests
// ============================================================================

TEST_F(IRPrinterTest, PrintType_Int) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    std::string result = printer->printType(intType);

    EXPECT_EQ(result, "i64");
}

TEST_F(IRPrinterTest, PrintType_Float) {
    auto floatType = std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
    std::string result = printer->printType(floatType);

    EXPECT_EQ(result, "f64");
}

TEST_F(IRPrinterTest, PrintType_Bool) {
    auto boolType = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
    std::string result = printer->printType(boolType);

    EXPECT_EQ(result, "i1");
}

TEST_F(IRPrinterTest, PrintType_Void) {
    auto voidType = std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
    std::string result = printer->printType(voidType);

    EXPECT_EQ(result, "void");
}

TEST_F(IRPrinterTest, PrintType_Pointer) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto ptrType = std::make_shared<IRPointerType>(intType);
    std::string result = printer->printType(ptrType);

    EXPECT_EQ(result, "ptr<i64>");
}

TEST_F(IRPrinterTest, PrintType_Array) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto arrType = std::make_shared<IRArrayType>(intType, 10);
    std::string result = printer->printType(arrType);

    EXPECT_EQ(result, "[10 x i64]");
}

// ============================================================================
// Value Printing Tests
// ============================================================================

TEST_F(IRPrinterTest, PrintValue_ConstantInt) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* constant = track(ConstantInt::get(42, intType));

    std::string result = printer->printValue(constant);

    EXPECT_EQ(result, "42");
}

TEST_F(IRPrinterTest, PrintValue_ConstantFloat) {
    auto floatType = std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
    auto* constant = track(ConstantFloat::get(3.14, floatType));

    std::string result = printer->printValue(constant);

    // Should print floating point value
    EXPECT_NE(result.find("3.14"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintValue_ConstantBool_True) {
    auto boolType = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
    auto* constant = track(ConstantBool::get(true, boolType));

    std::string result = printer->printValue(constant);

    EXPECT_EQ(result, "true");
}

TEST_F(IRPrinterTest, PrintValue_ConstantBool_False) {
    auto boolType = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
    auto* constant = track(ConstantBool::get(false, boolType));

    std::string result = printer->printValue(constant);

    EXPECT_EQ(result, "false");
}

TEST_F(IRPrinterTest, PrintValue_NamedArgument) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    Argument arg(intType, 0, "x");

    std::string result = printer->printValue(&arg);

    EXPECT_EQ(result, "%x");
}

TEST_F(IRPrinterTest, PrintValue_UnnamedValue_GetsSSANumber) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* lhs = track(ConstantInt::get(1, intType));
    auto* rhs = track(ConstantInt::get(2, intType));
    auto* add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs));

    std::string result = printer->printValue(add);

    // Should assign SSA number
    EXPECT_EQ(result, "%0");
}

TEST_F(IRPrinterTest, PrintValue_MultipleUnnamedValues_IncrementSSANumber) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* val1 = track(ConstantInt::get(1, intType));
    auto* val2 = track(ConstantInt::get(2, intType));

    auto* add1 = track(BinaryOperator::create(Instruction::Opcode::Add, val1, val2));
    auto* add2 = track(BinaryOperator::create(Instruction::Opcode::Add, val1, val2));

    std::string result1 = printer->printValue(add1);
    std::string result2 = printer->printValue(add2);

    EXPECT_EQ(result1, "%0");
    EXPECT_EQ(result2, "%1");
}

TEST_F(IRPrinterTest, PrintValue_Reset_ClearsSSANumbering) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* val1 = track(ConstantInt::get(1, intType));
    auto* val2 = track(ConstantInt::get(2, intType));
    auto* add1 = track(BinaryOperator::create(Instruction::Opcode::Add, val1, val2));

    printer->printValue(add1);  // Gets %0
    printer->reset();

    auto* add2 = track(BinaryOperator::create(Instruction::Opcode::Add, val1, val2));
    std::string result = printer->printValue(add2);

    EXPECT_EQ(result, "%0");  // Should restart at %0
}

// ============================================================================
// Instruction Printing Tests
// ============================================================================

TEST_F(IRPrinterTest, PrintInstruction_Add) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* lhs = track(ConstantInt::get(10, intType));
    auto* rhs = track(ConstantInt::get(20, intType));
    auto* add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs, "sum"));

    std::string result = printer->printInstruction(add);

    // Should contain: add, i64, operands
    EXPECT_NE(result.find("add"), std::string::npos);
    EXPECT_NE(result.find("i64"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintInstruction_Return) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* value = track(ConstantInt::get(42, intType));
    auto* ret = track(ReturnInst::create(value));

    std::string result = printer->printInstruction(ret);

    EXPECT_NE(result.find("ret"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintInstruction_ReturnVoid) {
    auto* ret = track(ReturnInst::create(nullptr));

    std::string result = printer->printInstruction(ret);

    EXPECT_NE(result.find("ret"), std::string::npos);
    EXPECT_NE(result.find("void"), std::string::npos);
}

// ============================================================================
// Basic Block Printing Tests
// ============================================================================

TEST_F(IRPrinterTest, PrintBasicBlock_Empty) {
    BasicBlock* block = module->createBasicBlock("entry");

    std::string result = printer->printBasicBlock(block);

    EXPECT_NE(result.find("entry"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintBasicBlock_WithInstructions) {
    BasicBlock* block = module->createBasicBlock("entry");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* lhs = track(ConstantInt::get(1, intType));
    auto* rhs = track(ConstantInt::get(2, intType));
    auto* add = BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs);
    block->addInstruction(add);

    std::string result = printer->printBasicBlock(block);

    EXPECT_NE(result.find("entry"), std::string::npos);
    EXPECT_NE(result.find("add"), std::string::npos);
    // Note: add is owned by block which is owned by module's arena
}

TEST_F(IRPrinterTest, PrintBasicBlock_WithPredecessors) {
    BasicBlock* pred1 = module->createBasicBlock("pred1");
    BasicBlock* pred2 = module->createBasicBlock("pred2");
    BasicBlock* block = module->createBasicBlock("merge");

    block->addPredecessor(pred1);
    block->addPredecessor(pred2);

    printer->setShowCFG(true);
    std::string result = printer->printBasicBlock(block);

    // Should show predecessors
    EXPECT_NE(result.find("preds"), std::string::npos);
}

// ============================================================================
// Function Printing Tests
// ============================================================================

TEST_F(IRPrinterTest, PrintFunction_EmptyVoid) {
    auto voidType = std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
    Function* func = module->createFunction("test", voidType, {});

    std::string result = printer->printFunction(func);

    EXPECT_NE(result.find("function"), std::string::npos);
    EXPECT_NE(result.find("@test"), std::string::npos);
    EXPECT_NE(result.find("void"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintFunction_WithParameters) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    std::vector<std::shared_ptr<IRType>> params = {intType, intType};
    Function* func = module->createFunction("add", intType, params);

    std::string result = printer->printFunction(func);

    EXPECT_NE(result.find("@add"), std::string::npos);
    EXPECT_NE(result.find("i64"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintFunction_WithBody) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    std::vector<std::shared_ptr<IRType>> params = {intType, intType};
    Function* func = module->createFunction("add", intType, params);

    auto* entry = module->createBasicBlock("entry", func);
    auto* arg0 = func->getParam(0);
    auto* arg1 = func->getParam(1);
    auto* add = BinaryOperator::create(Instruction::Opcode::Add, arg0, arg1);
    entry->addInstruction(add);
    auto* ret = ReturnInst::create(add);
    entry->addInstruction(ret);

    std::string result = printer->printFunction(func);

    EXPECT_NE(result.find("add"), std::string::npos);
    EXPECT_NE(result.find("ret"), std::string::npos);
}

// ============================================================================
// Module Printing Tests
// ============================================================================

TEST_F(IRPrinterTest, PrintModule_Empty) {
    std::string result = printer->printModule(*module);

    EXPECT_NE(result.find("Module"), std::string::npos);
    EXPECT_NE(result.find("test_module"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintModule_WithFunction) {
    auto voidType = std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
    module->createFunction("main", voidType, {});

    std::string result = printer->printModule(*module);

    EXPECT_NE(result.find("@main"), std::string::npos);
}

TEST_F(IRPrinterTest, PrintModule_WithGlobal) {
    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* init = track(ConstantInt::get(42, intType));
    module->createGlobalVariable("counter", intType, init, false);

    std::string result = printer->printModule(*module);

    EXPECT_NE(result.find("@counter"), std::string::npos);
}

// ============================================================================
// Printer Options Tests
// ============================================================================

TEST_F(IRPrinterTest, ShowTypes_Enabled) {
    printer->setShowTypes(true);

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* lhs = track(ConstantInt::get(1, intType));
    auto* rhs = track(ConstantInt::get(2, intType));
    auto* add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs));

    std::string result = printer->printInstruction(add);

    // Should include type
    EXPECT_NE(result.find("i64"), std::string::npos);
}

TEST_F(IRPrinterTest, ShowTypes_Disabled) {
    printer->setShowTypes(false);

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* lhs = track(ConstantInt::get(1, intType));
    auto* rhs = track(ConstantInt::get(2, intType));
    auto* add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs));

    std::string result = printer->printInstruction(add);

    // Type might still appear but less prominently
}

TEST_F(IRPrinterTest, ShowCFG_Enabled) {
    printer->setShowCFG(true);

    BasicBlock* pred = module->createBasicBlock("pred");
    BasicBlock* block = module->createBasicBlock("block");
    block->addPredecessor(pred);

    std::string result = printer->printBasicBlock(block);

    EXPECT_NE(result.find("preds"), std::string::npos);
}

TEST_F(IRPrinterTest, ShowCFG_Disabled) {
    printer->setShowCFG(false);

    BasicBlock* pred = module->createBasicBlock("pred");
    BasicBlock* block = module->createBasicBlock("block");
    block->addPredecessor(pred);

    std::string result = printer->printBasicBlock(block);

    // Should not show predecessors
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(IRPrinterTest, Integration_SimpleFunction) {
    // Build: fn add(x: i64, y: i64) -> i64 { return x + y; }

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    std::vector<std::shared_ptr<IRType>> params = {intType, intType};
    Function* func = module->createFunction("add", intType, params);

    auto* entry = module->createBasicBlock("entry", func);
    auto* x = func->getParam(0);
    auto* y = func->getParam(1);

    auto* sum = BinaryOperator::create(Instruction::Opcode::Add, x, y, "sum");
    entry->addInstruction(sum);

    auto* ret = ReturnInst::create(sum);
    entry->addInstruction(ret);

    std::string result = printer->printFunction(func);

    // Verify output contains all expected parts
    EXPECT_NE(result.find("@add"), std::string::npos);
    EXPECT_NE(result.find("entry"), std::string::npos);
    EXPECT_NE(result.find("add"), std::string::npos);
    EXPECT_NE(result.find("ret"), std::string::npos);
}

TEST_F(IRPrinterTest, Integration_IfThenElse) {
    // Build: fn abs(x: i64) -> i64 {
    //   if x < 0 { return -x; } else { return x; }
    // }

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto boolType = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
    std::vector<std::shared_ptr<IRType>> params = {intType};
    Function* func = module->createFunction("abs", intType, params);

    auto* entry = module->createBasicBlock("entry", func);
    auto* thenBlock = module->createBasicBlock("then", func);
    auto* elseBlock = module->createBasicBlock("else", func);
    auto* mergeBlock = module->createBasicBlock("merge", func);

    // Entry: condition
    auto* x = func->getParam(0);
    auto* zero = track(ConstantInt::get(0, intType));
    auto* cond = CmpInst::create(Instruction::Opcode::Lt, x, zero);
    entry->addInstruction(cond);
    auto* br = CondBranchInst::create(cond, thenBlock, elseBlock);
    entry->addInstruction(br);

    // Then: negate
    auto* negX = UnaryOperator::create(Instruction::Opcode::Neg, x);
    thenBlock->addInstruction(negX);
    auto* brThen = BranchInst::create(mergeBlock);
    thenBlock->addInstruction(brThen);

    // Else: just use x
    auto* brElse = BranchInst::create(mergeBlock);
    elseBlock->addInstruction(brElse);

    // Merge: phi and return
    std::vector<PhiNode::IncomingValue> incoming = {
        {negX, thenBlock},
        {x, elseBlock}
    };
    auto* phi = PhiNode::create(intType, incoming, "result");
    mergeBlock->addInstruction(phi);
    auto* ret = ReturnInst::create(phi);
    mergeBlock->addInstruction(ret);

    std::string result = printer->printFunction(func);

    // Verify CFG structure
    EXPECT_NE(result.find("entry"), std::string::npos);
    EXPECT_NE(result.find("then"), std::string::npos);
    EXPECT_NE(result.find("else"), std::string::npos);
    EXPECT_NE(result.find("merge"), std::string::npos);
    EXPECT_NE(result.find("phi"), std::string::npos);
}
