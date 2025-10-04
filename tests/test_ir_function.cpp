#include <gtest/gtest.h>
#include <memory>
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

using namespace volta::ir;

// Test helpers
static std::shared_ptr<IRType> makeIntType() {
    return std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
}

static std::shared_ptr<IRType> makeBoolType() {
    return std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
}

static std::shared_ptr<IRType> makeVoidType() {
    return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
}

// ============================================================================
// Function Creation Tests
// ============================================================================

TEST(FunctionTest, CreateEmptyFunction) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "test");
    EXPECT_EQ(func->getReturnType(), intType);
    EXPECT_EQ(func->getNumParams(), 0);
    EXPECT_TRUE(func->isDeclaration());
    EXPECT_FALSE(func->isDefinition());

    delete func;
}

TEST(FunctionTest, CreateFunctionWithParameters) {
    auto intType = makeIntType();
    auto* func = Function::create("add", intType, {intType, intType});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getNumParams(), 2);
    EXPECT_TRUE(func->hasParams());

    auto* arg0 = func->getParam(0);
    auto* arg1 = func->getParam(1);

    ASSERT_NE(arg0, nullptr);
    ASSERT_NE(arg1, nullptr);

    EXPECT_EQ(arg0->getArgNo(), 0);
    EXPECT_EQ(arg1->getArgNo(), 1);
    EXPECT_EQ(arg0->getParent(), func);
    EXPECT_EQ(arg1->getParent(), func);

    delete func;
}

TEST(FunctionTest, CreateVoidFunction) {
    auto voidType = makeVoidType();
    auto* func = Function::create("doSomething", voidType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getReturnType(), voidType);
    EXPECT_FALSE(func->hasReturnValue());

    delete func;
}

TEST(FunctionTest, CreateMainFunction) {
    auto intType = makeIntType();
    auto* func = Function::create("main", intType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "main");
    EXPECT_TRUE(func->hasReturnValue());

    delete func;
}

// ============================================================================
// Parameter Tests
// ============================================================================

TEST(FunctionTest, GetParameterByIndex) {
    auto intType = makeIntType();
    auto boolType = makeBoolType();
    auto* func = Function::create("test", intType, {intType, boolType, intType});

    EXPECT_EQ(func->getNumParams(), 3);

    auto* param0 = func->getParam(0);
    auto* param1 = func->getParam(1);
    auto* param2 = func->getParam(2);

    EXPECT_NE(param0, nullptr);
    EXPECT_NE(param1, nullptr);
    EXPECT_NE(param2, nullptr);

    EXPECT_EQ(param0->getType(), intType);
    EXPECT_EQ(param1->getType(), boolType);
    EXPECT_EQ(param2->getType(), intType);

    delete func;
}

TEST(FunctionTest, GetParameterOutOfBounds) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {intType});

    auto* param = func->getParam(5);
    EXPECT_EQ(param, nullptr);

    delete func;
}

TEST(FunctionTest, GetAllArguments) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {intType, intType, intType});

    const auto& args = func->getArguments();
    EXPECT_EQ(args.size(), 3);

    for (size_t i = 0; i < args.size(); ++i) {
        EXPECT_EQ(args[i]->getArgNo(), i);
        EXPECT_EQ(args[i]->getParent(), func);
    }

    delete func;
}

// ============================================================================
// Basic Block Management Tests
// ============================================================================

TEST(FunctionTest, AddFirstBasicBlock) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb = BasicBlock::create("entry");
    func->addBasicBlock(bb);

    EXPECT_FALSE(func->isDeclaration());
    EXPECT_TRUE(func->isDefinition());
    EXPECT_EQ(func->getNumBlocks(), 1);
    EXPECT_TRUE(func->hasBlocks());
    EXPECT_EQ(func->getEntryBlock(), bb);
    EXPECT_EQ(bb->getParent(), func);

    delete func;
}

TEST(FunctionTest, AddMultipleBasicBlocks) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb1 = BasicBlock::create("entry");
    auto* bb2 = BasicBlock::create("loop");
    auto* bb3 = BasicBlock::create("exit");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);
    func->addBasicBlock(bb3);

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(func->getEntryBlock(), bb1); // First block is entry

    const auto& blocks = func->getBlocks();
    EXPECT_EQ(blocks[0], bb1);
    EXPECT_EQ(blocks[1], bb2);
    EXPECT_EQ(blocks[2], bb3);

    delete func;
}

TEST(FunctionTest, RemoveBasicBlock) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    EXPECT_EQ(func->getNumBlocks(), 2);

    func->removeBasicBlock(bb2);

    EXPECT_EQ(func->getNumBlocks(), 1);
    EXPECT_EQ(bb2->getParent(), nullptr);

    delete func;
    delete bb2; // We own it after removal
}

TEST(FunctionTest, EraseBasicBlock) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    func->eraseBasicBlock(bb2);

    EXPECT_EQ(func->getNumBlocks(), 1);
    // bb2 is deleted, don't access it!

    delete func;
}

TEST(FunctionTest, FindBlockByName) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* entry = BasicBlock::create("entry");
    auto* loop = BasicBlock::create("loop");
    auto* exit = BasicBlock::create("exit");

    func->addBasicBlock(entry);
    func->addBasicBlock(loop);
    func->addBasicBlock(exit);

    EXPECT_EQ(func->findBlock("entry"), entry);
    EXPECT_EQ(func->findBlock("loop"), loop);
    EXPECT_EQ(func->findBlock("exit"), exit);
    EXPECT_EQ(func->findBlock("nonexistent"), nullptr);

    delete func;
}

TEST(FunctionTest, SetEntryBlock) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    EXPECT_EQ(func->getEntryBlock(), bb1); // Initially first block

    func->setEntryBlock(bb2);
    EXPECT_EQ(func->getEntryBlock(), bb2);

    delete func;
}

TEST(FunctionTest, HasSingleBlock) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    EXPECT_FALSE(func->hasSingleBlock());

    auto* bb = BasicBlock::create("entry");
    func->addBasicBlock(bb);

    EXPECT_TRUE(func->hasSingleBlock());

    auto* bb2 = BasicBlock::create("bb2");
    func->addBasicBlock(bb2);

    EXPECT_FALSE(func->hasSingleBlock());

    delete func;
}

// ============================================================================
// Instruction Access Tests
// ============================================================================

TEST(FunctionTest, GetAllInstructions) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {intType, intType});

    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    auto* arg0 = func->getParam(0);
    auto* arg1 = func->getParam(1);

    auto* add1 = BinaryOperator::create(Instruction::Opcode::Add, arg0, arg1);
    auto* add2 = BinaryOperator::create(Instruction::Opcode::Add, arg0, arg1);
    auto* ret = ReturnInst::create(add1);

    bb1->addInstruction(add1);
    bb2->addInstruction(add2);
    bb2->addInstruction(ret);

    auto instructions = func->getAllInstructions();

    EXPECT_EQ(instructions.size(), 3);
    EXPECT_EQ(func->getNumInstructions(), 3);

    delete func;
}

TEST(FunctionTest, GetNumInstructions) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    EXPECT_EQ(func->getNumInstructions(), 0);

    auto* bb = BasicBlock::create("entry");
    func->addBasicBlock(bb);

    auto* inst1 = BinaryOperator::create(Instruction::Opcode::Add,
                                        ConstantInt::get(1, intType),
                                        ConstantInt::get(2, intType));
    auto* inst2 = BinaryOperator::create(Instruction::Opcode::Mul,
                                        ConstantInt::get(3, intType),
                                        ConstantInt::get(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    EXPECT_EQ(func->getNumInstructions(), 2);

    delete func;
}

// ============================================================================
// CFG Analysis Tests
// ============================================================================

TEST(FunctionTest, GetExitBlocks) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");
    auto* bb3 = BasicBlock::create("bb3");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);
    func->addBasicBlock(bb3);

    // Add returns to bb2 and bb3
    bb2->addInstruction(ReturnInst::create(ConstantInt::get(1, intType)));
    bb3->addInstruction(ReturnInst::create(ConstantInt::get(2, intType)));

    auto exitBlocks = func->getExitBlocks();

    EXPECT_EQ(exitBlocks.size(), 2);
    EXPECT_NE(std::find(exitBlocks.begin(), exitBlocks.end(), bb2), exitBlocks.end());
    EXPECT_NE(std::find(exitBlocks.begin(), exitBlocks.end(), bb3), exitBlocks.end());

    delete func;
}

TEST(FunctionTest, GetUnreachableBlocks) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* entry = BasicBlock::create("entry");
    auto* reachable = BasicBlock::create("reachable");
    auto* unreachable1 = BasicBlock::create("unreachable1");
    auto* unreachable2 = BasicBlock::create("unreachable2");

    func->addBasicBlock(entry);
    func->addBasicBlock(reachable);
    func->addBasicBlock(unreachable1);
    func->addBasicBlock(unreachable2);

    // Connect entry -> reachable
    cfg::connectBlocks(entry, reachable);

    // unreachable1 and unreachable2 have no incoming edges
    unreachable1->addInstruction(ReturnInst::create(nullptr));
    unreachable2->addInstruction(ReturnInst::create(nullptr));

    auto unreachableBlocks = func->getUnreachableBlocks();

    EXPECT_EQ(unreachableBlocks.size(), 2);
    EXPECT_NE(std::find(unreachableBlocks.begin(), unreachableBlocks.end(), unreachable1),
              unreachableBlocks.end());
    EXPECT_NE(std::find(unreachableBlocks.begin(), unreachableBlocks.end(), unreachable2),
              unreachableBlocks.end());

    delete func;
}

TEST(FunctionTest, RemoveUnreachableBlocks) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* entry = BasicBlock::create("entry");
    auto* reachable = BasicBlock::create("reachable");
    auto* unreachable = BasicBlock::create("unreachable");

    func->addBasicBlock(entry);
    func->addBasicBlock(reachable);
    func->addBasicBlock(unreachable);

    cfg::connectBlocks(entry, reachable);
    unreachable->addInstruction(ReturnInst::create(nullptr));

    EXPECT_EQ(func->getNumBlocks(), 3);

    size_t removed = func->removeUnreachableBlocks();

    EXPECT_EQ(removed, 1);
    EXPECT_EQ(func->getNumBlocks(), 2);

    delete func;
}

TEST(FunctionTest, CanReturn) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* bb = BasicBlock::create("entry");
    func->addBasicBlock(bb);

    // No return yet
    EXPECT_FALSE(func->canReturn());

    bb->addInstruction(ReturnInst::create(ConstantInt::get(42, intType)));

    EXPECT_TRUE(func->canReturn());

    delete func;
}

TEST(FunctionTest, DoesNotReturn) {
    auto voidType = makeVoidType();
    auto* func = Function::create("abort", voidType, {});

    auto* bb = BasicBlock::create("entry");
    func->addBasicBlock(bb);

    // Add some instruction but no return (simulating noreturn)
    // For this test, we just don't add a return
    auto intType = makeIntType();
    bb->addInstruction(BinaryOperator::create(Instruction::Opcode::Add,
                                             ConstantInt::get(1, intType),
                                             ConstantInt::get(2, intType)));

    // Note: This would be invalid IR (no terminator), but for testing doesNotReturn logic
    // In reality, you'd use an unreachable instruction

    delete func;
}

TEST(FunctionTest, HasReturnValue) {
    auto intType = makeIntType();
    auto voidType = makeVoidType();

    auto* func1 = Function::create("returnsInt", intType, {});
    auto* func2 = Function::create("returnsVoid", voidType, {});

    EXPECT_TRUE(func1->hasReturnValue());
    EXPECT_FALSE(func2->hasReturnValue());

    delete func1;
    delete func2;
}

// ============================================================================
// Verification Tests
// ============================================================================

TEST(FunctionTest, VerifyValidFunction) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {intType});

    auto* entry = BasicBlock::create("entry");
    func->addBasicBlock(entry);

    auto* arg = func->getParam(0);
    entry->addInstruction(ReturnInst::create(arg));

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

    delete func;
}

TEST(FunctionTest, VerifyDeclarationIsValid) {
    auto intType = makeIntType();
    auto* func = Function::create("external", intType, {intType, intType});

    // Declaration with no body
    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

    delete func;
}

TEST(FunctionTest, VerifyFailsWithoutTerminator) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* entry = BasicBlock::create("entry");
    func->addBasicBlock(entry);

    // Add instruction but no terminator
    entry->addInstruction(BinaryOperator::create(Instruction::Opcode::Add,
                                                 ConstantInt::get(1, intType),
                                                 ConstantInt::get(2, intType)));

    std::string error;
    EXPECT_FALSE(func->verify(&error));
    EXPECT_FALSE(error.empty());

    delete func;
}

TEST(FunctionTest, VerifyComplexCFG) {
    auto intType = makeIntType();
    auto boolType = makeBoolType();
    auto* func = Function::create("test", intType, {intType});

    auto* entry = BasicBlock::create("entry");
    auto* thenBlock = BasicBlock::create("then");
    auto* elseBlock = BasicBlock::create("else");
    auto* merge = BasicBlock::create("merge");

    func->addBasicBlock(entry);
    func->addBasicBlock(thenBlock);
    func->addBasicBlock(elseBlock);
    func->addBasicBlock(merge);

    auto* arg = func->getParam(0);
    auto* cond = CmpInst::create(Instruction::Opcode::Gt, arg, ConstantInt::get(0, intType));

    entry->addInstruction(cond);
    cfg::connectBlocksConditional(entry, cond, thenBlock, elseBlock);

    thenBlock->addInstruction(BinaryOperator::create(Instruction::Opcode::Add, arg, ConstantInt::get(1, intType)));
    cfg::connectBlocks(thenBlock, merge);

    elseBlock->addInstruction(BinaryOperator::create(Instruction::Opcode::Sub, arg, ConstantInt::get(1, intType)));
    cfg::connectBlocks(elseBlock, merge);

    merge->addInstruction(ReturnInst::create(arg));

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

    delete func;
}

// ============================================================================
// toString Tests
// ============================================================================

TEST(FunctionTest, ToStringDeclaration) {
    auto intType = makeIntType();
    auto* func = Function::create("add", intType, {intType, intType});

    std::string str = func->toString();

    EXPECT_NE(str.find("@add"), std::string::npos);
    EXPECT_NE(str.find("i64"), std::string::npos);

    delete func;
}

TEST(FunctionTest, ToStringDefinition) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* entry = BasicBlock::create("entry");
    func->addBasicBlock(entry);
    entry->addInstruction(ReturnInst::create(ConstantInt::get(42, intType)));

    std::string str = func->toString();

    EXPECT_NE(str.find("@test"), std::string::npos);
    EXPECT_NE(str.find("entry"), std::string::npos);
    EXPECT_NE(str.find("ret"), std::string::npos);

    delete func;
}

TEST(FunctionTest, ToStringWithParameters) {
    auto intType = makeIntType();
    auto* func = Function::create("add", intType, {intType, intType});

    auto* entry = BasicBlock::create("entry");
    func->addBasicBlock(entry);

    auto* arg0 = func->getParam(0);
    auto* arg1 = func->getParam(1);
    auto* add = BinaryOperator::create(Instruction::Opcode::Add, arg0, arg1);

    entry->addInstruction(add);
    entry->addInstruction(ReturnInst::create(add));

    std::string str = func->toString();

    EXPECT_NE(str.find("@add"), std::string::npos);
    EXPECT_NE(str.find("arg0"), std::string::npos);
    EXPECT_NE(str.find("arg1"), std::string::npos);

    delete func;
}

// ============================================================================
// CFG Printing Tests
// ============================================================================

TEST(FunctionTest, PrintCFG) {
    auto intType = makeIntType();
    auto* func = Function::create("test", intType, {});

    auto* entry = BasicBlock::create("entry");
    auto* bb1 = BasicBlock::create("bb1");

    func->addBasicBlock(entry);
    func->addBasicBlock(bb1);

    cfg::connectBlocks(entry, bb1);
    bb1->addInstruction(ReturnInst::create(nullptr));

    std::string dot = func->printCFG();

    EXPECT_NE(dot.find("digraph"), std::string::npos);
    EXPECT_NE(dot.find("entry"), std::string::npos);
    EXPECT_NE(dot.find("bb1"), std::string::npos);
    EXPECT_NE(dot.find("->"), std::string::npos); // Edge

    delete func;
}

// ============================================================================
// Complex Integration Tests
// ============================================================================

TEST(FunctionTest, FactorialFunction) {
    // function @factorial(n: i64) -> i64 {
    // entry:
    //   %cond = le i64 %n, i64 1
    //   condbr %cond, base_case, recursive_case
    // base_case:
    //   ret i64 1
    // recursive_case:
    //   %n_minus_1 = sub i64 %n, i64 1
    //   ret i64 0  // Simplified (would normally have recursive call)
    // }

    auto intType = makeIntType();
    auto* func = Function::create("factorial", intType, {intType});

    auto* entry = BasicBlock::create("entry");
    auto* baseCase = BasicBlock::create("base_case");
    auto* recursiveCase = BasicBlock::create("recursive_case");

    func->addBasicBlock(entry);
    func->addBasicBlock(baseCase);
    func->addBasicBlock(recursiveCase);

    auto* n = func->getParam(0);
    auto* cond = CmpInst::create(Instruction::Opcode::Le, n, ConstantInt::get(1, intType));

    entry->addInstruction(cond);
    cfg::connectBlocksConditional(entry, cond, baseCase, recursiveCase);

    baseCase->addInstruction(ReturnInst::create(ConstantInt::get(1, intType)));

    auto* nMinus1 = BinaryOperator::create(Instruction::Opcode::Sub, n, ConstantInt::get(1, intType));
    recursiveCase->addInstruction(nMinus1);
    recursiveCase->addInstruction(ReturnInst::create(ConstantInt::get(0, intType)));

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(func->getExitBlocks().size(), 2);

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

    delete func;
}

TEST(FunctionTest, LoopFunction) {
    // function @loop(n: i64) -> i64 {
    // entry:
    //   br loop_header
    // loop_header:
    //   br exit
    // exit:
    //   ret i64 0
    // }

    auto intType = makeIntType();
    auto* func = Function::create("loop", intType, {intType});

    auto* entry = BasicBlock::create("entry");
    auto* loopHeader = BasicBlock::create("loop_header");
    auto* exit = BasicBlock::create("exit");

    func->addBasicBlock(entry);
    func->addBasicBlock(loopHeader);
    func->addBasicBlock(exit);

    cfg::connectBlocks(entry, loopHeader);
    cfg::connectBlocks(loopHeader, exit);

    exit->addInstruction(ReturnInst::create(ConstantInt::get(0, intType)));

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(loopHeader->getNumPredecessors(), 1);

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

    delete func;
}
