#include <gtest/gtest.h>
#include <memory>
#include "IR/Module.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

using namespace volta::ir;

// ============================================================================
// Basic Block Creation Tests
// ============================================================================

TEST(BasicBlockTest, CreateBasicBlock) {
    Module module("test");
    auto* bb = module.createBasicBlock("entry");

    ASSERT_NE(bb, nullptr);
    EXPECT_EQ(bb->getName(), "entry");
    EXPECT_TRUE(bb->empty());
    EXPECT_EQ(bb->getNumInstructions(), 0);
    EXPECT_FALSE(bb->hasTerminator());
}

TEST(BasicBlockTest, CreateUnnamedBlock) {
    Module module("test");
    auto* bb = module.createBasicBlock();

    ASSERT_NE(bb, nullptr);
    EXPECT_TRUE(bb->getName().empty() || bb->getName().find("bb") != std::string::npos);
    EXPECT_TRUE(bb->empty());

}

TEST(BasicBlockTest, CreateWithParent) {
    Module module("test");
    // Note: We'll need Function class for this, so parent might be nullptr for now
    auto* bb = module.createBasicBlock("test", nullptr);

    ASSERT_NE(bb, nullptr);
    EXPECT_EQ(bb->getParent(), nullptr);

}

// ============================================================================
// Instruction Management Tests
// ============================================================================

TEST(BasicBlockTest, AddSingleInstruction) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();
    auto lhs = module.getConstantInt(10, intType);
    auto rhs = module.getConstantInt(20, intType);
    auto add = module.createBinaryOp(Instruction::Opcode::Add, lhs, rhs);

    bb->addInstruction(add);

    EXPECT_FALSE(bb->empty());
    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getFirstInstruction(), add);
    EXPECT_EQ(add->getParent(), bb);

}

TEST(BasicBlockTest, AddMultipleInstructions) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();
    auto v1 = module.getConstantInt(10, intType);
    auto v2 = module.getConstantInt(20, intType);

    auto add = module.createBinaryOp(Instruction::Opcode::Add, v1, v2);
    auto sub = module.createBinaryOp(Instruction::Opcode::Sub, v1, v2);
    auto mul = module.createBinaryOp(Instruction::Opcode::Mul, v1, v2);

    bb->addInstruction(add);
    bb->addInstruction(sub);
    bb->addInstruction(mul);

    EXPECT_EQ(bb->getNumInstructions(), 3);
    EXPECT_EQ(bb->getFirstInstruction(), add);

    const auto& insts = bb->getInstructions();
    EXPECT_EQ(insts[0], add);
    EXPECT_EQ(insts[1], sub);
    EXPECT_EQ(insts[2], mul);

}

TEST(BasicBlockTest, AddTerminator) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();
    auto retVal = module.getConstantInt(42, intType);
    auto ret = module.createReturn(retVal);

    bb->addInstruction(ret);

    EXPECT_TRUE(bb->hasTerminator());
    EXPECT_EQ(bb->getTerminator(), ret);
    EXPECT_EQ(bb->getNumInstructions(), 1);

}

TEST(BasicBlockTest, CannotAddInstructionAfterTerminator) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();

    auto ret = module.createReturn(nullptr);
    bb->addInstruction(ret);

    auto add = module.createBinaryOp(Instruction::Opcode::Add,
                                     module.getConstantInt(1, intType),
                                     module.getConstantInt(2, intType));

    // This should fail or throw
    // HINT: Your implementation should detect that a terminator exists
    // For this test, we expect the block to still have only 1 instruction
    bb->addInstruction(add);

    // After attempting to add, block should still have only the terminator
    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getTerminator(), ret);

}

TEST(BasicBlockTest, GetFirstInstruction) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();

    // Empty block
    EXPECT_EQ(bb->getFirstInstruction(), nullptr);

    auto inst1 = module.createBinaryOp(Instruction::Opcode::Add,
                                       module.getConstantInt(1, intType),
                                       module.getConstantInt(2, intType));
    auto inst2 = module.createBinaryOp(Instruction::Opcode::Sub,
                                       module.getConstantInt(3, intType),
                                       module.getConstantInt(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    EXPECT_EQ(bb->getFirstInstruction(), inst1);

}

TEST(BasicBlockTest, RemoveInstruction) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();

    auto inst1 = module.createBinaryOp(Instruction::Opcode::Add,
                                       module.getConstantInt(1, intType),
                                       module.getConstantInt(2, intType));
    auto inst2 = module.createBinaryOp(Instruction::Opcode::Sub,
                                       module.getConstantInt(3, intType),
                                       module.getConstantInt(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    EXPECT_EQ(bb->getNumInstructions(), 2);

    bb->removeInstruction(inst1);

    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getFirstInstruction(), inst2);
    EXPECT_EQ(inst1->getParent(), nullptr);

}

TEST(BasicBlockTest, EraseInstruction) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();

    auto inst1 = module.createBinaryOp(Instruction::Opcode::Add,
                                       module.getConstantInt(1, intType),
                                       module.getConstantInt(2, intType));
    auto inst2 = module.createBinaryOp(Instruction::Opcode::Sub,
                                       module.getConstantInt(3, intType),
                                       module.getConstantInt(4, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    bb->eraseInstruction(inst1);

    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_EQ(bb->getFirstInstruction(), inst2);
    // inst1 is deleted, don't access it!

}

TEST(BasicBlockTest, InsertBefore) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();

    auto inst1 = module.createBinaryOp(Instruction::Opcode::Add,
                                       module.getConstantInt(1, intType),
                                       module.getConstantInt(2, intType));
    auto inst2 = module.createBinaryOp(Instruction::Opcode::Sub,
                                       module.getConstantInt(3, intType),
                                       module.getConstantInt(4, intType));
    auto instMiddle = module.createBinaryOp(Instruction::Opcode::Mul,
                                            module.getConstantInt(5, intType),
                                            module.getConstantInt(6, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);

    bb->insertBefore(instMiddle, inst2);

    EXPECT_EQ(bb->getNumInstructions(), 3);

    const auto& insts = bb->getInstructions();
    EXPECT_EQ(insts[0], inst1);
    EXPECT_EQ(insts[1], instMiddle);
    EXPECT_EQ(insts[2], inst2);

}

// ============================================================================
// CFG Tests (Predecessors/Successors)
// ============================================================================

TEST(BasicBlockTest, InitiallyNoPredecessors) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");

    EXPECT_EQ(bb->getNumPredecessors(), 0);
    EXPECT_TRUE(bb->getPredecessors().empty());

}

TEST(BasicBlockTest, AddPredecessor) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    bb2->addPredecessor(bb1);

    EXPECT_EQ(bb2->getNumPredecessors(), 1);
    EXPECT_TRUE(bb2->isPredecessor(bb1));
    EXPECT_FALSE(bb1->isPredecessor(bb2));

}

TEST(BasicBlockTest, AddMultiplePredecessors) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");
    auto* bb3 = module.createBasicBlock("bb3");

    bb3->addPredecessor(bb1);
    bb3->addPredecessor(bb2);

    EXPECT_EQ(bb3->getNumPredecessors(), 2);
    EXPECT_TRUE(bb3->hasMultiplePredecessors());

}

TEST(BasicBlockTest, RemovePredecessor) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    bb2->addPredecessor(bb1);
    EXPECT_EQ(bb2->getNumPredecessors(), 1);

    bb2->removePredecessor(bb1);
    EXPECT_EQ(bb2->getNumPredecessors(), 0);

}

TEST(BasicBlockTest, NoDuplicatePredecessors) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    bb2->addPredecessor(bb1);
    bb2->addPredecessor(bb1); // Try to add again

    // Should still have only 1 predecessor
    EXPECT_EQ(bb2->getNumPredecessors(), 1);

}

TEST(BasicBlockTest, GetSuccessors_ReturnInst) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();
    auto ret = module.createReturn(module.getConstantInt(42, intType));

    bb->addInstruction(ret);

    auto successors = bb->getSuccessors();
    EXPECT_EQ(successors.size(), 0); // Return has no successors

}

TEST(BasicBlockTest, GetSuccessors_BranchInst) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    auto br = module.createBranch(bb2);
    bb1->addInstruction(br);

    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 1);
    EXPECT_EQ(successors[0], bb2);

    EXPECT_TRUE(bb1->isSuccessor(bb2));

}

TEST(BasicBlockTest, GetSuccessors_CondBranchInst) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("entry");
    auto* bb2 = module.createBasicBlock("then");
    auto* bb3 = module.createBasicBlock("else");

    auto boolType = module.getBoolType();
    auto cond = module.getConstantBool(true, boolType);
    auto condBr = module.createCondBranch(cond, bb2, bb3);

    bb1->addInstruction(condBr);

    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 2);

    // Check that both successors are present (order doesn't matter for this test)
    EXPECT_TRUE(bb1->isSuccessor(bb2));
    EXPECT_TRUE(bb1->isSuccessor(bb3));

}

TEST(BasicBlockTest, GetNumSuccessors) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");
    auto* bb3 = module.createBasicBlock("bb3");

    // No terminator = 0 successors
    EXPECT_EQ(bb1->getNumSuccessors(), 0);

    // Unconditional branch = 1 successor
    bb1->addInstruction(module.createBranch(bb2));
    EXPECT_EQ(bb1->getNumSuccessors(), 1);

}

// ============================================================================
// CFG Builder Utility Tests
// ============================================================================

TEST(CFGBuilderTest, ConnectBlocks) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    cfg::connectBlocks(module, bb1, bb2);

    // bb1 should have a branch to bb2
    EXPECT_TRUE(bb1->hasTerminator());
    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 1);
    EXPECT_EQ(successors[0], bb2);

    // bb2 should have bb1 as predecessor
    EXPECT_TRUE(bb2->isPredecessor(bb1));

}

TEST(CFGBuilderTest, ConnectBlocksConditional) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("entry");
    auto* bb2 = module.createBasicBlock("then");
    auto* bb3 = module.createBasicBlock("else");

    auto boolType = module.getBoolType();
    auto cond = module.getConstantBool(true, boolType);

    cfg::connectBlocksConditional(module, bb1, cond, bb2, bb3);

    // bb1 should have conditional branch
    EXPECT_TRUE(bb1->hasTerminator());
    auto successors = bb1->getSuccessors();
    EXPECT_EQ(successors.size(), 2);

    // Both bb2 and bb3 should have bb1 as predecessor
    EXPECT_TRUE(bb2->isPredecessor(bb1));
    EXPECT_TRUE(bb3->isPredecessor(bb1));

}

// ============================================================================
// Phi Node Tests
// ============================================================================

TEST(BasicBlockTest, InitiallyNoPhiNodes) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");

    EXPECT_FALSE(bb->hasPhiNodes());
    EXPECT_TRUE(bb->getPhiNodes().empty());

}

TEST(BasicBlockTest, AddPhiNode) {
    Module module("test");
    auto* bb = module.createBasicBlock("merge");
    auto intType = module.getIntType();

    auto phi = module.createPhi(intType, {}, "merged_val");
    bb->addPhiNode(phi);

    EXPECT_TRUE(bb->hasPhiNodes());
    auto phis = bb->getPhiNodes();
    EXPECT_EQ(phis.size(), 1);
    EXPECT_EQ(phis[0], phi);

}

TEST(BasicBlockTest, PhiNodesBeforeRegularInstructions) {
    Module module("test");
    auto* bb = module.createBasicBlock("merge");
    auto intType = module.getIntType();

    auto phi = module.createPhi(intType, {}, "phi1");
    auto add = module.createBinaryOp(Instruction::Opcode::Add,
                                     module.getConstantInt(1, intType),
                                     module.getConstantInt(2, intType));

    bb->addInstruction(add);
    bb->addPhiNode(phi);

    // Phi should come before add
    const auto& insts = bb->getInstructions();
    EXPECT_EQ(insts[0], phi);
    EXPECT_EQ(insts[1], add);

}

TEST(BasicBlockTest, MultiplePhiNodes) {
    Module module("test");
    auto* bb = module.createBasicBlock("merge");
    auto intType = module.getIntType();

    auto phi1 = module.createPhi(intType, {}, "phi1");
    auto phi2 = module.createPhi(intType, {}, "phi2");

    bb->addPhiNode(phi1);
    bb->addPhiNode(phi2);

    auto phis = bb->getPhiNodes();
    EXPECT_EQ(phis.size(), 2);

}

// ============================================================================
// toString Tests
// ============================================================================

TEST(BasicBlockTest, ToStringEmpty) {
    Module module("test");
    auto* bb = module.createBasicBlock("entry");

    std::string str = bb->toString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("entry"), std::string::npos);

}

TEST(BasicBlockTest, ToStringWithInstructions) {
    Module module("test");
    auto* bb = module.createBasicBlock("test");
    auto intType = module.getIntType();

    auto add = module.createBinaryOp(Instruction::Opcode::Add,
                                     module.getConstantInt(1, intType),
                                     module.getConstantInt(2, intType));
    bb->addInstruction(add);

    std::string str = bb->toString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("test"), std::string::npos);

}

TEST(BasicBlockTest, ToStringWithPredecessors) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    bb2->addPredecessor(bb1);

    std::string str = bb2->toString();
    EXPECT_NE(str.find("preds"), std::string::npos);

}

// ============================================================================
// Complex CFG Tests
// ============================================================================

TEST(CFGTest, SimpleDiamondCFG) {
    Module module("test");
    // entry
    //   |
    //   v
    //  cond
    //  /  \
    // then else
    //  \  /
    //   merge

    auto* entry = module.createBasicBlock("entry");
    auto* cond = module.createBasicBlock("cond");
    auto* thenBlock = module.createBasicBlock("then");
    auto* elseBlock = module.createBasicBlock("else");
    auto* merge = module.createBasicBlock("merge");

    auto boolType = module.getBoolType();
    auto condVal = module.getConstantBool(true, boolType);

    // Connect blocks
    cfg::connectBlocks(module, entry, cond);
    cfg::connectBlocksConditional(module, cond, condVal, thenBlock, elseBlock);
    cfg::connectBlocks(module, thenBlock, merge);
    cfg::connectBlocks(module, elseBlock, merge);

    // Verify structure
    EXPECT_EQ(cond->getNumPredecessors(), 1);
    EXPECT_EQ(cond->getNumSuccessors(), 2);

    EXPECT_EQ(merge->getNumPredecessors(), 2);
    EXPECT_TRUE(merge->hasMultiplePredecessors());

    EXPECT_TRUE(merge->isPredecessor(thenBlock));
    EXPECT_TRUE(merge->isPredecessor(elseBlock));

}

// ============================================================================
// Advanced CFG Utility Tests
// ============================================================================

TEST(CFGUtilityTest, HasPath_DirectConnection) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    cfg::connectBlocks(module, bb1, bb2);

    EXPECT_TRUE(cfg::hasPath(bb1, bb2));
    EXPECT_FALSE(cfg::hasPath(bb2, bb1)); // No path backwards

}

TEST(CFGUtilityTest, HasPath_IndirectConnection) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");
    auto* bb3 = module.createBasicBlock("bb3");

    cfg::connectBlocks(module, bb1, bb2);
    cfg::connectBlocks(module, bb2, bb3);

    EXPECT_TRUE(cfg::hasPath(bb1, bb3));
    EXPECT_FALSE(cfg::hasPath(bb3, bb1));

}

TEST(CFGUtilityTest, GetReachableBlocks) {
    Module module("test");
    auto* entry = module.createBasicBlock("entry");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    cfg::connectBlocks(module, entry, bb1);
    cfg::connectBlocks(module, bb1, bb2);

    auto reachable = cfg::getReachableBlocks(entry);

    EXPECT_EQ(reachable.size(), 3); // entry, bb1, bb2
    EXPECT_NE(std::find(reachable.begin(), reachable.end(), entry), reachable.end());
    EXPECT_NE(std::find(reachable.begin(), reachable.end(), bb1), reachable.end());
    EXPECT_NE(std::find(reachable.begin(), reachable.end(), bb2), reachable.end());

}

TEST(CFGUtilityTest, ReversePostorder) {
    Module module("test");
    auto* entry = module.createBasicBlock("entry");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    cfg::connectBlocks(module, entry, bb1);
    cfg::connectBlocks(module, bb1, bb2);

    auto rpo = cfg::reversePostorder(entry);

    EXPECT_EQ(rpo.size(), 3);
    // In reverse postorder, entry should come first
    EXPECT_EQ(rpo[0], entry);

}

// ============================================================================
// Block Split Tests
// ============================================================================

TEST(BasicBlockTest, SplitAtInstruction) {
    Module module("test");
    auto* bb = module.createBasicBlock("original");
    auto intType = module.getIntType();

    auto inst1 = module.createBinaryOp(Instruction::Opcode::Add,
                                       module.getConstantInt(1, intType),
                                       module.getConstantInt(2, intType));
    auto inst2 = module.createBinaryOp(Instruction::Opcode::Sub,
                                       module.getConstantInt(3, intType),
                                       module.getConstantInt(4, intType));
    auto inst3 = module.createBinaryOp(Instruction::Opcode::Mul,
                                       module.getConstantInt(5, intType),
                                       module.getConstantInt(6, intType));

    bb->addInstruction(inst1);
    bb->addInstruction(inst2);
    bb->addInstruction(inst3);

    // Split at inst2
    auto* newBlock = bb->splitAt(module, inst2, "split");

    ASSERT_NE(newBlock, nullptr);

    // Original block should have inst1 + branch
    EXPECT_EQ(bb->getNumInstructions(), 2); // inst1 + branch
    EXPECT_TRUE(bb->hasTerminator());

    // New block should have inst2 and inst3
    EXPECT_EQ(newBlock->getNumInstructions(), 2);

}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(BasicBlockTest, EmptyBlockHasNoTerminator) {
    Module module("test");
    auto* bb = module.createBasicBlock("empty");

    EXPECT_FALSE(bb->hasTerminator());
    EXPECT_EQ(bb->getTerminator(), nullptr);

}

TEST(BasicBlockTest, BlockWithOnlyTerminator) {
    Module module("test");
    auto* bb = module.createBasicBlock("ret_only");
    auto ret = module.createReturn(nullptr);

    bb->addInstruction(ret);

    EXPECT_EQ(bb->getNumInstructions(), 1);
    EXPECT_TRUE(bb->hasTerminator());
    EXPECT_EQ(bb->getFirstInstruction(), ret);
    EXPECT_EQ(bb->getTerminator(), ret);

}

TEST(BasicBlockTest, IsReachable) {
    Module module("test");
    auto* bb1 = module.createBasicBlock("bb1");
    auto* bb2 = module.createBasicBlock("bb2");

    // Initially unreachable (no predecessors)
    EXPECT_FALSE(bb2->isReachable());

    // Add predecessor
    bb2->addPredecessor(bb1);

    // Now reachable
    EXPECT_TRUE(bb2->isReachable());

}
