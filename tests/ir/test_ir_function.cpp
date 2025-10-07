#include <gtest/gtest.h>
#include <memory>
#include "IR/Module.hpp"
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
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "test");
    EXPECT_EQ(func->getReturnType(), intType);
    EXPECT_EQ(func->getNumParams(), 0);
    EXPECT_TRUE(func->isDeclaration());
    EXPECT_FALSE(func->isDefinition());

}

TEST(FunctionTest, CreateFunctionWithParameters) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("add", intType, {intType, intType});

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

}

TEST(FunctionTest, CreateVoidFunction) {
    Module module("test");
    auto voidType = makeVoidType();
    auto* func = module.createFunction("doSomething", voidType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getReturnType(), voidType);
    EXPECT_FALSE(func->hasReturnValue());

}

TEST(FunctionTest, CreateMainFunction) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("main", intType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "main");
    EXPECT_TRUE(func->hasReturnValue());

}

// ============================================================================
// Parameter Tests
// ============================================================================

TEST(FunctionTest, GetParameterByIndex) {
    Module module("test");
    auto intType = makeIntType();
    auto boolType = makeBoolType();
    auto* func = module.createFunction("test", intType, {intType, boolType, intType});

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

}

TEST(FunctionTest, GetParameterOutOfBounds) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {intType});

    auto* param = func->getParam(5);
    EXPECT_EQ(param, nullptr);

}

TEST(FunctionTest, GetAllArguments) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {intType, intType, intType});

    const auto& args = func->getArguments();
    EXPECT_EQ(args.size(), 3);

    for (size_t i = 0; i < args.size(); ++i) {
        EXPECT_EQ(args[i]->getArgNo(), i);
        EXPECT_EQ(args[i]->getParent(), func);
    }

}

// ============================================================================
// Basic Block Management Tests
// ============================================================================

TEST(FunctionTest, AddFirstBasicBlock) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb = module.createBasicBlock("entry");
    func->addBasicBlock(bb);

    EXPECT_FALSE(func->isDeclaration());
    EXPECT_TRUE(func->isDefinition());
    EXPECT_EQ(func->getNumBlocks(), 1);
    EXPECT_TRUE(func->hasBlocks());
    EXPECT_EQ(func->getEntryBlock(), bb);
    EXPECT_EQ(bb->getParent(), func);

}

TEST(FunctionTest, AddMultipleBasicBlocks) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb1 = module.createBasicBlock("entry");
    auto* bb2 = module.createBasicBlock("loop");
    auto* bb3 = module.createBasicBlock("exit");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);
    func->addBasicBlock(bb3);

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(func->getEntryBlock(), bb1); // First block is entry

    const auto& blocks = func->getBlocks();
    EXPECT_EQ(blocks[0], bb1);
    EXPECT_EQ(blocks[1], bb2);
    EXPECT_EQ(blocks[2], bb3);

}

TEST(FunctionTest, RemoveBasicBlock) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    EXPECT_EQ(func->getNumBlocks(), 2);

    func->removeBasicBlock(bb2);

    EXPECT_EQ(func->getNumBlocks(), 1);
    EXPECT_EQ(bb2->getParent(), nullptr);

}

TEST(FunctionTest, EraseBasicBlock) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    func->eraseBasicBlock(bb2);

    EXPECT_EQ(func->getNumBlocks(), 1);
    // bb2 is deleted, don't access it!

}

TEST(FunctionTest, FindBlockByName) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry");
    auto* loop = module.createBasicBlock("loop");
    auto* exit = module.createBasicBlock("exit");

    func->addBasicBlock(entry);
    func->addBasicBlock(loop);
    func->addBasicBlock(exit);

    EXPECT_EQ(func->findBlock("entry"), entry);
    EXPECT_EQ(func->findBlock("loop"), loop);
    EXPECT_EQ(func->findBlock("exit"), exit);
    EXPECT_EQ(func->findBlock("nonexistent"), nullptr);

}

TEST(FunctionTest, SetEntryBlock) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    EXPECT_EQ(func->getEntryBlock(), bb1); // Initially first block

    func->setEntryBlock(bb2);
    EXPECT_EQ(func->getEntryBlock(), bb2);

}

TEST(FunctionTest, HasSingleBlock) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    EXPECT_FALSE(func->hasSingleBlock());

    auto* bb = module.createBasicBlock("entry");
    func->addBasicBlock(bb);

    EXPECT_TRUE(func->hasSingleBlock());

    auto* bb2 = module.createBasicBlock("bb2");
    func->addBasicBlock(bb2);

    EXPECT_FALSE(func->hasSingleBlock());

}

// ============================================================================
// Instruction Access Tests
// ============================================================================

TEST(FunctionTest, GetAllInstructions) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {intType, intType});

    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);

    auto* arg0 = func->getParam(0);
    auto* arg1 = func->getParam(1);

    auto* add1 = module.createBinaryOp(Instruction::Opcode::Add, arg0, arg1);
    auto* add2 = module.createBinaryOp(Instruction::Opcode::Add, arg0, arg1);
    auto* ret = module.createReturn(add1);

    bb1->addInstruction(add1);
    bb2->addInstruction(add2);
    bb2->addInstruction(ret);

    auto instructions = func->getAllInstructions();

    EXPECT_EQ(instructions.size(), 3);
    EXPECT_EQ(func->getNumInstructions(), 3);

}

TEST(FunctionTest, GetNumInstructions) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    EXPECT_EQ(func->getNumInstructions(), 0);

    auto* bb = module.createBasicBlock("entry");
    func->addBasicBlock(bb);

    auto* inst1 = module.createBinaryOp(Instruction::Opcode::Add,
                                        module.getConstantInt(1, intType),
                                        module.getConstantInt(2, intType));
    auto* inst2 = module.createBinaryOp(Instruction::Opcode::Mul,
                                        module.getConstantInt(3, intType),
                                        module.getConstantInt(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    EXPECT_EQ(func->getNumInstructions(), 2);

}

// ============================================================================
// CFG Analysis Tests
// ============================================================================

TEST(FunctionTest, GetExitBlocks) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");
    auto* bb3 = module.createBasicBlock("bb3");

    func->addBasicBlock(bb1);
    func->addBasicBlock(bb2);
    func->addBasicBlock(bb3);

    // Add returns to bb2 and bb3
    bb2->addInstruction(module.createReturn(module.getConstantInt(1, intType)));
    bb3->addInstruction(module.createReturn(module.getConstantInt(2, intType)));

    auto exitBlocks = func->getExitBlocks();

    EXPECT_EQ(exitBlocks.size(), 2);
    EXPECT_NE(std::find(exitBlocks.begin(), exitBlocks.end(), bb2), exitBlocks.end());
    EXPECT_NE(std::find(exitBlocks.begin(), exitBlocks.end(), bb3), exitBlocks.end());

}

TEST(FunctionTest, GetUnreachableBlocks) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry");
    auto* reachable = module.createBasicBlock("reachable");
    auto* unreachable1 = module.createBasicBlock("unreachable1");
    auto* unreachable2 = module.createBasicBlock("unreachable2");

    func->addBasicBlock(entry);
    func->addBasicBlock(reachable);
    func->addBasicBlock(unreachable1);
    func->addBasicBlock(unreachable2);

    // Connect entry -> reachable
    cfg::connectBlocks(module, entry, reachable);

    // unreachable1 and unreachable2 have no incoming edges
    unreachable1->addInstruction(module.createReturn(nullptr));
    unreachable2->addInstruction(module.createReturn(nullptr));

    auto unreachableBlocks = func->getUnreachableBlocks();

    EXPECT_EQ(unreachableBlocks.size(), 2);
    EXPECT_NE(std::find(unreachableBlocks.begin(), unreachableBlocks.end(), unreachable1),
              unreachableBlocks.end());
    EXPECT_NE(std::find(unreachableBlocks.begin(), unreachableBlocks.end(), unreachable2),
              unreachableBlocks.end());

}

TEST(FunctionTest, RemoveUnreachableBlocks) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry");
    auto* reachable = module.createBasicBlock("reachable");
    auto* unreachable = module.createBasicBlock("unreachable");

    func->addBasicBlock(entry);
    func->addBasicBlock(reachable);
    func->addBasicBlock(unreachable);

    cfg::connectBlocks(module, entry, reachable);
    unreachable->addInstruction(module.createReturn(nullptr));

    EXPECT_EQ(func->getNumBlocks(), 3);

    size_t removed = func->removeUnreachableBlocks();

    EXPECT_EQ(removed, 1);
    EXPECT_EQ(func->getNumBlocks(), 2);

}

TEST(FunctionTest, CanReturn) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb = module.createBasicBlock("entry");
    func->addBasicBlock(bb);

    // No return yet
    EXPECT_FALSE(func->canReturn());

    bb->addInstruction(module.createReturn(module.getConstantInt(42, intType)));

    EXPECT_TRUE(func->canReturn());

}

TEST(FunctionTest, DoesNotReturn) {
    Module module("test");
    auto voidType = makeVoidType();
    auto* func = module.createFunction("abort", voidType, {});

    auto* bb = module.createBasicBlock("entry");
    func->addBasicBlock(bb);

    // Add some instruction but no return (simulating noreturn)
    // For this test, we just don't add a return
    auto intType = makeIntType();
    bb->addInstruction(module.createBinaryOp(Instruction::Opcode::Add,
                                             module.getConstantInt(1, intType),
                                             module.getConstantInt(2, intType)));

    // Note: This would be invalid IR (no terminator), but for testing doesNotReturn logic
    // In reality, you'd use an unreachable instruction

}

TEST(FunctionTest, HasReturnValue) {
    Module module("test");
    auto intType = makeIntType();
    auto voidType = makeVoidType();

    auto* func1 = module.createFunction("returnsInt", intType, {});
    auto* func2 = module.createFunction("returnsVoid", voidType, {});

    EXPECT_TRUE(func1->hasReturnValue());
    EXPECT_FALSE(func2->hasReturnValue());

}

// ============================================================================
// Verification Tests
// ============================================================================

TEST(FunctionTest, VerifyValidFunction) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {intType});

    auto* entry = module.createBasicBlock("entry");
    func->addBasicBlock(entry);

    auto* arg = func->getParam(0);
    entry->addInstruction(module.createReturn(arg));

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

}

TEST(FunctionTest, VerifyDeclarationIsValid) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("external", intType, {intType, intType});

    // Declaration with no body
    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

}

TEST(FunctionTest, VerifyFailsWithoutTerminator) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry");
    func->addBasicBlock(entry);

    // Add instruction but no terminator
    entry->addInstruction(module.createBinaryOp(Instruction::Opcode::Add,
                                                 module.getConstantInt(1, intType),
                                                 module.getConstantInt(2, intType)));

    std::string error;
    EXPECT_FALSE(func->verify(&error));
    EXPECT_FALSE(error.empty());

}

TEST(FunctionTest, VerifyComplexCFG) {
    Module module("test");
    auto intType = makeIntType();
    auto boolType = makeBoolType();
    auto* func = module.createFunction("test", intType, {intType});

    auto* entry = module.createBasicBlock("entry");
    auto* thenBlock = module.createBasicBlock("then");
    auto* elseBlock = module.createBasicBlock("else");
    auto* merge = module.createBasicBlock("merge");

    func->addBasicBlock(entry);
    func->addBasicBlock(thenBlock);
    func->addBasicBlock(elseBlock);
    func->addBasicBlock(merge);

    auto* arg = func->getParam(0);
    auto* cond = module.createCmp(Instruction::Opcode::Gt, arg, module.getConstantInt(0, intType));

    entry->addInstruction(cond);
    cfg::connectBlocksConditional(module, entry, cond, thenBlock, elseBlock);

    thenBlock->addInstruction(module.createBinaryOp(Instruction::Opcode::Add, arg, module.getConstantInt(1, intType)));
    cfg::connectBlocks(module, thenBlock, merge);

    elseBlock->addInstruction(module.createBinaryOp(Instruction::Opcode::Sub, arg, module.getConstantInt(1, intType)));
    cfg::connectBlocks(module, elseBlock, merge);

    merge->addInstruction(module.createReturn(arg));

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

}

// ============================================================================
// toString Tests
// ============================================================================

TEST(FunctionTest, ToStringDeclaration) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("add", intType, {intType, intType});

    std::string str = func->toString();

    EXPECT_NE(str.find("@add"), std::string::npos);
    EXPECT_NE(str.find("i64"), std::string::npos);

}

TEST(FunctionTest, ToStringDefinition) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry");
    func->addBasicBlock(entry);
    entry->addInstruction(module.createReturn(module.getConstantInt(42, intType)));

    std::string str = func->toString();

    EXPECT_NE(str.find("@test"), std::string::npos);
    EXPECT_NE(str.find("entry"), std::string::npos);
    EXPECT_NE(str.find("ret"), std::string::npos);

}

TEST(FunctionTest, ToStringWithParameters) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("add", intType, {intType, intType});

    auto* entry = module.createBasicBlock("entry");
    func->addBasicBlock(entry);

    auto* arg0 = func->getParam(0);
    auto* arg1 = func->getParam(1);
    auto* add = module.createBinaryOp(Instruction::Opcode::Add, arg0, arg1);

    entry->addInstruction(add);
    entry->addInstruction(module.createReturn(add));

    std::string str = func->toString();

    EXPECT_NE(str.find("@add"), std::string::npos);
    EXPECT_NE(str.find("arg0"), std::string::npos);
    EXPECT_NE(str.find("arg1"), std::string::npos);

}

// ============================================================================
// CFG Printing Tests
// ============================================================================

TEST(FunctionTest, PrintCFG) {
    Module module("test");
    auto intType = makeIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry");
    auto* bb1 = module.createBasicBlock("bb1");

    func->addBasicBlock(entry);
    func->addBasicBlock(bb1);

    cfg::connectBlocks(module, entry, bb1);
    bb1->addInstruction(module.createReturn(nullptr));

    std::string dot = func->printCFG();

    EXPECT_NE(dot.find("digraph"), std::string::npos);
    EXPECT_NE(dot.find("entry"), std::string::npos);
    EXPECT_NE(dot.find("bb1"), std::string::npos);
    EXPECT_NE(dot.find("->"), std::string::npos); // Edge

}

// ============================================================================
// Complex Integration Tests
// ============================================================================

TEST(FunctionTest, FactorialFunction) {
    Module module("test");
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
    auto* func = module.createFunction("factorial", intType, {intType});

    auto* entry = module.createBasicBlock("entry");
    auto* baseCase = module.createBasicBlock("base_case");
    auto* recursiveCase = module.createBasicBlock("recursive_case");

    func->addBasicBlock(entry);
    func->addBasicBlock(baseCase);
    func->addBasicBlock(recursiveCase);

    auto* n = func->getParam(0);
    auto* cond = module.createCmp(Instruction::Opcode::Le, n, module.getConstantInt(1, intType));

    entry->addInstruction(cond);
    cfg::connectBlocksConditional(module, entry, cond, baseCase, recursiveCase);

    baseCase->addInstruction(module.createReturn(module.getConstantInt(1, intType)));

    auto* nMinus1 = module.createBinaryOp(Instruction::Opcode::Sub, n, module.getConstantInt(1, intType));
    recursiveCase->addInstruction(nMinus1);
    recursiveCase->addInstruction(module.createReturn(module.getConstantInt(0, intType)));

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(func->getExitBlocks().size(), 2);

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

}

TEST(FunctionTest, LoopFunction) {
    Module module("test");
    // function @loop(n: i64) -> i64 {
    // entry:
    //   br loop_header
    // loop_header:
    //   br exit
    // exit:
    //   ret i64 0
    // }

    auto intType = makeIntType();
    auto* func = module.createFunction("loop", intType, {intType});

    auto* entry = module.createBasicBlock("entry");
    auto* loopHeader = module.createBasicBlock("loop_header");
    auto* exit = module.createBasicBlock("exit");

    func->addBasicBlock(entry);
    func->addBasicBlock(loopHeader);
    func->addBasicBlock(exit);

    cfg::connectBlocks(module, entry, loopHeader);
    cfg::connectBlocks(module, loopHeader, exit);

    exit->addInstruction(module.createReturn(module.getConstantInt(0, intType)));

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(loopHeader->getNumPredecessors(), 1);

    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;

}
