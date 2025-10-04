#include <gtest/gtest.h>
#include <memory>
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

// ============================================================================
// Basic Block Creation Tests
// ============================================================================

TEST(BasicBlockTest, CreateBasicBlock) {
    auto* bb = BasicBlock::create("entry");

    ASSERT_NE(bb, nullptr);
    EXPECT_EQ(bb->getName(), "entry");
    EXPECT_TRUE(bb->empty());
    EXPECT_EQ(bb->getNumInstructions(), 0);
    EXPECT_FALSE(bb->hasTerminator());

    delete bb;
}

TEST(BasicBlockTest, CreateUnnamedBlock) {
    auto* bb = BasicBlock::create();

    ASSERT_NE(bb, nullptr);
    EXPECT_TRUE(bb->getName().empty() || bb->getName().find("bb") != std::string::npos);
    EXPECT_TRUE(bb->empty());

    delete bb;
}

TEST(BasicBlockTest, CreateWithParent) {
    // Note: We'll need Function class for this, so parent might be nullptr for now
    auto* bb = BasicBlock::create("test", nullptr);

    ASSERT_NE(bb, nullptr);
    EXPECT_EQ(bb->getParent(), nullptr);

    delete bb;
}

// ============================================================================
// Instruction Management Tests
// ============================================================================

TEST(BasicBlockTest, AddSingleInstruction) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();
    auto lhs = ConstantInt::get(10, intType);
    auto rhs = ConstantInt::get(20, intType);
    auto add = BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs);

    bb->addInstruction(add);

    EXPECT_FALSE(bb->empty());
    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getFirstInstruction(), add);
    EXPECT_EQ(add->getParent(), bb);

    delete bb;
}

TEST(BasicBlockTest, AddMultipleInstructions) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();
    auto v1 = ConstantInt::get(10, intType);
    auto v2 = ConstantInt::get(20, intType);

    auto add = BinaryOperator::create(Instruction::Opcode::Add, v1, v2);
    auto sub = BinaryOperator::create(Instruction::Opcode::Sub, v1, v2);
    auto mul = BinaryOperator::create(Instruction::Opcode::Mul, v1, v2);

    bb->addInstruction(add);
    bb->addInstruction(sub);
    bb->addInstruction(mul);

    EXPECT_EQ(bb->getNumInstructions(), 3);
    EXPECT_EQ(bb->getFirstInstruction(), add);

    const auto& insts = bb->getInstructions();
    EXPECT_EQ(insts[0], add);
    EXPECT_EQ(insts[1], sub);
    EXPECT_EQ(insts[2], mul);

    delete bb;
}

TEST(BasicBlockTest, AddTerminator) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();
    auto retVal = ConstantInt::get(42, intType);
    auto ret = ReturnInst::create(retVal);

    bb->addInstruction(ret);

    EXPECT_TRUE(bb->hasTerminator());
    EXPECT_EQ(bb->getTerminator(), ret);
    EXPECT_EQ(bb->getNumInstructions(), 1);

    delete bb;
}

TEST(BasicBlockTest, CannotAddInstructionAfterTerminator) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();

    auto ret = ReturnInst::create(nullptr);
    bb->addInstruction(ret);

    auto add = BinaryOperator::create(Instruction::Opcode::Add,
                                     ConstantInt::get(1, intType),
                                     ConstantInt::get(2, intType));

    // This should fail or throw
    // HINT: Your implementation should detect that a terminator exists
    // For this test, we expect the block to still have only 1 instruction
    bb->addInstruction(add);

    // After attempting to add, block should still have only the terminator
    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getTerminator(), ret);

    delete bb;
    delete add; // Clean up since it wasn't added
}

TEST(BasicBlockTest, GetFirstInstruction) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();

    // Empty block
    EXPECT_EQ(bb->getFirstInstruction(), nullptr);

    auto inst1 = BinaryOperator::create(Instruction::Opcode::Add,
                                       ConstantInt::get(1, intType),
                                       ConstantInt::get(2, intType));
    auto inst2 = BinaryOperator::create(Instruction::Opcode::Sub,
                                       ConstantInt::get(3, intType),
                                       ConstantInt::get(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    EXPECT_EQ(bb->getFirstInstruction(), inst1);

    delete bb;
}

TEST(BasicBlockTest, RemoveInstruction) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();

    auto inst1 = BinaryOperator::create(Instruction::Opcode::Add,
                                       ConstantInt::get(1, intType),
                                       ConstantInt::get(2, intType));
    auto inst2 = BinaryOperator::create(Instruction::Opcode::Sub,
                                       ConstantInt::get(3, intType),
                                       ConstantInt::get(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    EXPECT_EQ(bb->getNumInstructions(), 2);

    bb->removeInstruction(inst1);

    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getFirstInstruction(), inst2);
    EXPECT_EQ(inst1->getParent(), nullptr);

    delete bb;
    delete inst1; // We removed it, so we own it
}

TEST(BasicBlockTest, EraseInstruction) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();

    auto inst1 = BinaryOperator::create(Instruction::Opcode::Add,
                                       ConstantInt::get(1, intType),
                                       ConstantInt::get(2, intType));
    auto inst2 = BinaryOperator::create(Instruction::Opcode::Sub,
                                       ConstantInt::get(3, intType),
                                       ConstantInt::get(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    bb->eraseInstruction(inst1);

    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getFirstInstruction(), inst2);
    // inst1 is deleted, don't access it!

    delete bb;
}

TEST(BasicBlockTest, InsertBefore) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();

    auto inst1 = BinaryOperator::create(Instruction::Opcode::Add,
                                       ConstantInt::get(1, intType),
                                       ConstantInt::get(2, intType));
    auto inst2 = BinaryOperator::create(Instruction::Opcode::Sub,
                                       ConstantInt::get(3, intType),
                                       ConstantInt::get(4, intType));
    auto instMiddle = BinaryOperator::create(Instruction::Opcode::Mul,
                                            ConstantInt::get(5, intType),
                                            ConstantInt::get(6, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    bb->insertBefore(instMiddle, inst2);

    EXPECT_EQ(bb->getNumInstructions(), 3);

    const auto& insts = bb->getInstructions();
    EXPECT_EQ(insts[0], inst1);
    EXPECT_EQ(insts[1], instMiddle);
    EXPECT_EQ(insts[2], inst2);

    delete bb;
}

// ============================================================================
// CFG Tests (Predecessors/Successors)
// ============================================================================

TEST(BasicBlockTest, InitiallyNoPredecessors) {
    auto* bb = BasicBlock::create("test");

    EXPECT_EQ(bb->getNumPredecessors(), 0);
    EXPECT_TRUE(bb->getPredecessors().empty());

    delete bb;
}

TEST(BasicBlockTest, AddPredecessor) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    bb2->addPredecessor(bb1);

    EXPECT_EQ(bb2->getNumPredecessors(), 1);
    EXPECT_TRUE(bb2->isPredecessor(bb1));
    EXPECT_FALSE(bb1->isPredecessor(bb2));

    delete bb1;
    delete bb2;
}

TEST(BasicBlockTest, AddMultiplePredecessors) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");
    auto* bb3 = BasicBlock::create("bb3");

    bb3->addPredecessor(bb1);
    bb3->addPredecessor(bb2);

    EXPECT_EQ(bb3->getNumPredecessors(), 2);
    EXPECT_TRUE(bb3->hasMultiplePredecessors());

    delete bb1;
    delete bb2;
    delete bb3;
}

TEST(BasicBlockTest, RemovePredecessor) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    bb2->addPredecessor(bb1);
    EXPECT_EQ(bb2->getNumPredecessors(), 1);

    bb2->removePredecessor(bb1);
    EXPECT_EQ(bb2->getNumPredecessors(), 0);

    delete bb1;
    delete bb2;
}

TEST(BasicBlockTest, NoDuplicatePredecessors) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    bb2->addPredecessor(bb1);
    bb2->addPredecessor(bb1); // Try to add again

    // Should still have only 1 predecessor
    EXPECT_EQ(bb2->getNumPredecessors(), 1);

    delete bb1;
    delete bb2;
}

TEST(BasicBlockTest, GetSuccessors_ReturnInst) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();
    auto ret = ReturnInst::create(ConstantInt::get(42, intType));

    bb->addInstruction(ret);

    auto successors = bb->getSuccessors();
    EXPECT_EQ(successors.size(), 0); // Return has no successors

    delete bb;
}

TEST(BasicBlockTest, GetSuccessors_BranchInst) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    auto br = BranchInst::create(bb2);
    bb1->addInstruction(br);

    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 1);
    EXPECT_EQ(successors[0], bb2);

    EXPECT_TRUE(bb1->isSuccessor(bb2));

    delete bb1;
    delete bb2;
}

TEST(BasicBlockTest, GetSuccessors_CondBranchInst) {
    auto* bb1 = BasicBlock::create("entry");
    auto* bb2 = BasicBlock::create("then");
    auto* bb3 = BasicBlock::create("else");

    auto boolType = makeBoolType();
    auto cond = ConstantBool::get(true, boolType);
    auto condBr = CondBranchInst::create(cond, bb2, bb3);

    bb1->addInstruction(condBr);

    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 2);

    // Check that both successors are present (order doesn't matter for this test)
    EXPECT_TRUE(bb1->isSuccessor(bb2));
    EXPECT_TRUE(bb1->isSuccessor(bb3));

    delete bb1;
    delete bb2;
    delete bb3;
}

TEST(BasicBlockTest, GetNumSuccessors) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");
    auto* bb3 = BasicBlock::create("bb3");

    // No terminator = 0 successors
    EXPECT_EQ(bb1->getNumSuccessors(), 0);

    // Unconditional branch = 1 successor
    bb1->addInstruction(BranchInst::create(bb2));
    EXPECT_EQ(bb1->getNumSuccessors(), 1);

    delete bb1;
    delete bb2;
    delete bb3;
}

// ============================================================================
// CFG Builder Utility Tests
// ============================================================================

TEST(CFGBuilderTest, ConnectBlocks) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    cfg::connectBlocks(bb1, bb2);

    // bb1 should have a branch to bb2
    EXPECT_TRUE(bb1->hasTerminator());
    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 1);
    EXPECT_EQ(successors[0], bb2);

    // bb2 should have bb1 as predecessor
    EXPECT_TRUE(bb2->isPredecessor(bb1));

    delete bb1;
    delete bb2;
}

TEST(CFGBuilderTest, ConnectBlocksConditional) {
    auto* bb1 = BasicBlock::create("entry");
    auto* bb2 = BasicBlock::create("then");
    auto* bb3 = BasicBlock::create("else");

    auto boolType = makeBoolType();
    auto cond = ConstantBool::get(true, boolType);

    cfg::connectBlocksConditional(bb1, cond, bb2, bb3);

    // bb1 should have conditional branch
    EXPECT_TRUE(bb1->hasTerminator());
    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 2);

    // Both bb2 and bb3 should have bb1 as predecessor
    EXPECT_TRUE(bb2->isPredecessor(bb1));
    EXPECT_TRUE(bb3->isPredecessor(bb1));

    delete bb1;
    delete bb2;
    delete bb3;
}

// ============================================================================
// Phi Node Tests
// ============================================================================

TEST(BasicBlockTest, InitiallyNoPhiNodes) {
    auto* bb = BasicBlock::create("test");

    EXPECT_FALSE(bb->hasPhiNodes());
    EXPECT_TRUE(bb->getPhiNodes().empty());

    delete bb;
}

TEST(BasicBlockTest, AddPhiNode) {
    auto* bb = BasicBlock::create("merge");
    auto intType = makeIntType();

    auto phi = PhiNode::create(intType, {}, "merged_val");
    bb->addPhiNode(phi);

    EXPECT_TRUE(bb->hasPhiNodes());
    auto phis = bb->getPhiNodes();
    EXPECT_EQ(phis.size(), 1);
    EXPECT_EQ(phis[0], phi);

    delete bb;
}

TEST(BasicBlockTest, PhiNodesBeforeRegularInstructions) {
    auto* bb = BasicBlock::create("merge");
    auto intType = makeIntType();

    auto phi = PhiNode::create(intType, {}, "phi1");
    auto add = BinaryOperator::create(Instruction::Opcode::Add,
                                     ConstantInt::get(1, intType),
                                     ConstantInt::get(2, intType));

    bb->addInstruction(add);
    bb->addPhiNode(phi);

    // Phi should come before add
    const auto& insts = bb->getInstructions();
    EXPECT_EQ(insts[0], phi);
    EXPECT_EQ(insts[1], add);

    delete bb;
}

TEST(BasicBlockTest, MultiplePhiNodes) {
    auto* bb = BasicBlock::create("merge");
    auto intType = makeIntType();

    auto phi1 = PhiNode::create(intType, {}, "phi1");
    auto phi2 = PhiNode::create(intType, {}, "phi2");

    bb->addPhiNode(phi1);
    bb->addPhiNode(phi2);

    auto phis = bb->getPhiNodes();
    EXPECT_EQ(phis.size(), 2);

    delete bb;
}

// ============================================================================
// toString Tests
// ============================================================================

TEST(BasicBlockTest, ToStringEmpty) {
    auto* bb = BasicBlock::create("entry");

    std::string str = bb->toString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("entry"), std::string::npos);

    delete bb;
}

TEST(BasicBlockTest, ToStringWithInstructions) {
    auto* bb = BasicBlock::create("test");
    auto intType = makeIntType();

    auto add = BinaryOperator::create(Instruction::Opcode::Add,
                                     ConstantInt::get(1, intType),
                                     ConstantInt::get(2, intType));
    bb->addInstruction(add);

    std::string str = bb->toString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("test"), std::string::npos);

    delete bb;
}

TEST(BasicBlockTest, ToStringWithPredecessors) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    bb2->addPredecessor(bb1);

    std::string str = bb2->toString();
    EXPECT_NE(str.find("preds"), std::string::npos);

    delete bb1;
    delete bb2;
}

// ============================================================================
// Complex CFG Tests
// ============================================================================

TEST(CFGTest, SimpleDiamondCFG) {
    // entry
    //   |
    //   v
    //  cond
    //  /  \
    // then else
    //  \  /
    //   merge

    auto* entry = BasicBlock::create("entry");
    auto* cond = BasicBlock::create("cond");
    auto* thenBlock = BasicBlock::create("then");
    auto* elseBlock = BasicBlock::create("else");
    auto* merge = BasicBlock::create("merge");

    auto boolType = makeBoolType();
    auto condVal = ConstantBool::get(true, boolType);

    // Connect blocks
    cfg::connectBlocks(entry, cond);
    cfg::connectBlocksConditional(cond, condVal, thenBlock, elseBlock);
    cfg::connectBlocks(thenBlock, merge);
    cfg::connectBlocks(elseBlock, merge);

    // Verify structure
    EXPECT_EQ(cond->getNumPredecessors(), 1);
    EXPECT_EQ(cond->getNumSuccessors(), 2);

    EXPECT_EQ(merge->getNumPredecessors(), 2);
    EXPECT_TRUE(merge->hasMultiplePredecessors());

    EXPECT_TRUE(merge->isPredecessor(thenBlock));
    EXPECT_TRUE(merge->isPredecessor(elseBlock));

    delete entry;
    delete cond;
    delete thenBlock;
    delete elseBlock;
    delete merge;
}

// ============================================================================
// Advanced CFG Utility Tests
// ============================================================================

TEST(CFGUtilityTest, HasPath_DirectConnection) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    cfg::connectBlocks(bb1, bb2);

    EXPECT_TRUE(cfg::hasPath(bb1, bb2));
    EXPECT_FALSE(cfg::hasPath(bb2, bb1)); // No path backwards

    delete bb1;
    delete bb2;
}

TEST(CFGUtilityTest, HasPath_IndirectConnection) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");
    auto* bb3 = BasicBlock::create("bb3");

    cfg::connectBlocks(bb1, bb2);
    cfg::connectBlocks(bb2, bb3);

    EXPECT_TRUE(cfg::hasPath(bb1, bb3));
    EXPECT_FALSE(cfg::hasPath(bb3, bb1));

    delete bb1;
    delete bb2;
    delete bb3;
}

TEST(CFGUtilityTest, GetReachableBlocks) {
    auto* entry = BasicBlock::create("entry");
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    cfg::connectBlocks(entry, bb1);
    cfg::connectBlocks(bb1, bb2);

    auto reachable = cfg::getReachableBlocks(entry);

    EXPECT_EQ(reachable.size(), 3); // entry, bb1, bb2
    EXPECT_NE(std::find(reachable.begin(), reachable.end(), entry), reachable.end());
    EXPECT_NE(std::find(reachable.begin(), reachable.end(), bb1), reachable.end());
    EXPECT_NE(std::find(reachable.begin(), reachable.end(), bb2), reachable.end());

    delete entry;
    delete bb1;
    delete bb2;
}

TEST(CFGUtilityTest, ReversePostorder) {
    auto* entry = BasicBlock::create("entry");
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    cfg::connectBlocks(entry, bb1);
    cfg::connectBlocks(bb1, bb2);

    auto rpo = cfg::reversePostorder(entry);

    EXPECT_EQ(rpo.size(), 3);
    // In reverse postorder, entry should come first
    EXPECT_EQ(rpo[0], entry);

    delete entry;
    delete bb1;
    delete bb2;
}

// ============================================================================
// Block Split Tests
// ============================================================================

TEST(BasicBlockTest, SplitAtInstruction) {
    auto* bb = BasicBlock::create("original");
    auto intType = makeIntType();

    auto inst1 = BinaryOperator::create(Instruction::Opcode::Add,
                                       ConstantInt::get(1, intType),
                                       ConstantInt::get(2, intType));
    auto inst2 = BinaryOperator::create(Instruction::Opcode::Sub,
                                       ConstantInt::get(3, intType),
                                       ConstantInt::get(4, intType));
    auto inst3 = BinaryOperator::create(Instruction::Opcode::Mul,
                                       ConstantInt::get(5, intType),
                                       ConstantInt::get(6, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);
    bb->addInstruction(inst3);

    // Split at inst2
    auto* newBlock = bb->splitAt(inst2, "split");

    ASSERT_NE(newBlock, nullptr);

    // Original block should have inst1 + branch
    EXPECT_EQ(bb->getNumInstructions(), 2); // inst1 + branch
    EXPECT_TRUE(bb->hasTerminator());

    // New block should have inst2 and inst3
    EXPECT_EQ(newBlock->getNumInstructions(), 2);

    delete bb;
    delete newBlock;
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(BasicBlockTest, EmptyBlockHasNoTerminator) {
    auto* bb = BasicBlock::create("empty");

    EXPECT_FALSE(bb->hasTerminator());
    EXPECT_EQ(bb->getTerminator(), nullptr);

    delete bb;
}

TEST(BasicBlockTest, BlockWithOnlyTerminator) {
    auto* bb = BasicBlock::create("ret_only");
    auto ret = ReturnInst::create(nullptr);

    bb->addInstruction(ret);

    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_TRUE(bb->hasTerminator());
    EXPECT_EQ(bb->getFirstInstruction(), ret);
    EXPECT_EQ(bb->getTerminator(), ret);

    delete bb;
}

TEST(BasicBlockTest, IsReachable) {
    auto* bb1 = BasicBlock::create("bb1");
    auto* bb2 = BasicBlock::create("bb2");

    // Initially unreachable (no predecessors)
    EXPECT_FALSE(bb2->isReachable());

    // Add predecessor
    bb2->addPredecessor(bb1);

    // Now reachable
    EXPECT_TRUE(bb2->isReachable());

    delete bb1;
    delete bb2;
}
