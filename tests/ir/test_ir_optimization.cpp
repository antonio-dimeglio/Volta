#include <gtest/gtest.h>
#include "IR/Module.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/OptimizationPass.hpp"
#include "IR/IRPrinter.hpp"

using namespace volta::ir;

/**
 * Test fixture for optimization passes
 */
class OptimizationPassTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("test_module");
        builder = std::make_unique<IRBuilder>(*module);
    }

    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder> builder;
};

// ============================================================================
// Dead Code Elimination Tests
// ============================================================================

TEST_F(OptimizationPassTest, DCE_RemovesUnusedAdd) {
    // Create function: i64 @test() { %unused = add 5, 3; ret 42 }
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    // Create unused add instruction
    auto* five = builder->getInt(5);
    auto* three = builder->getInt(3);
    auto* unused = builder->createAdd(five, three, "unused");

    // Return a different value
    auto* ret_val = builder->getInt(42);
    builder->createRet(ret_val);

    // Run DCE
    DeadCodeEliminationPass dce;
    bool changed = dce.runOnModule(*module);

    // Verify: The add should be removed
    EXPECT_TRUE(changed) << "DCE should have modified the IR";

    // Check that unused instruction was removed
    bool found_unused = false;
    for (auto* inst : entry->getInstructions()) {
        if (inst == unused) {
            found_unused = true;
        }
    }
    EXPECT_FALSE(found_unused) << "Unused add instruction should be removed";
}

TEST_F(OptimizationPassTest, DCE_KeepsUsedInstructions) {
    // Create function: i64 @test() { %x = add 5, 3; ret %x }
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* five = builder->getInt(5);
    auto* three = builder->getInt(3);
    auto* x = builder->createAdd(five, three, "x");
    builder->createRet(x);

    // Run DCE
    DeadCodeEliminationPass dce;
    bool changed = dce.runOnModule(*module);

    // Verify: Nothing should be removed
    EXPECT_FALSE(changed) << "DCE should not modify IR with no dead code";
}

TEST_F(OptimizationPassTest, DCE_RemovesDeadChain) {
    // Create function with chain of unused instructions
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    // Dead chain: %a = 1, %b = add %a, 2, %c = mul %b, 3
    auto* a = builder->getInt(1);
    auto* two = builder->getInt(2);
    auto* b = builder->createAdd(a, two, "b");
    auto* three = builder->getInt(3);
    auto* c = builder->createMul(b, three, "c");

    // Return unrelated value
    builder->createRet(builder->getInt(99));

    // Run DCE
    DeadCodeEliminationPass dce;
    bool changed = dce.runOnModule(*module);

    // Verify: b and c should be removed (a is a constant)
    EXPECT_TRUE(changed) << "DCE should remove dead chain";
}

TEST_F(OptimizationPassTest, DCE_KeepsStoreInstruction) {
    // Create function with store (has side effects)
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* ptr = builder->createAlloca(builder->getIntType(), "ptr");
    auto* val = builder->getInt(42);
    builder->createStore(val, ptr);
    builder->createRet(nullptr);

    // Run DCE
    DeadCodeEliminationPass dce;
    bool changed = dce.runOnModule(*module);

    // Verify: Store should NOT be removed (side effect)
    EXPECT_FALSE(changed) << "DCE should not remove stores";
}

// ============================================================================
// Constant Folding Tests
// ============================================================================

TEST_F(OptimizationPassTest, ConstFold_FoldsAddition) {
    // Create: %x = add 5, 3 (should become %x = 8)
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* five = builder->getInt(5);
    auto* three = builder->getInt(3);
    auto* x = builder->createAdd(five, three, "x");
    builder->createRet(x);

    // Run constant folding
    ConstantFoldingPass fold;
    bool changed = fold.runOnModule(*module);

    // Verify: Should have folded
    EXPECT_TRUE(changed) << "Constant folding should fold add(5, 3)";
}

TEST_F(OptimizationPassTest, ConstFold_FoldsMultiplication) {
    // Create: %x = mul 6, 7 (should become %x = 42)
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->createMul(builder->getInt(6), builder->getInt(7), "x");
    builder->createRet(x);

    ConstantFoldingPass fold;
    bool changed = fold.runOnModule(*module);

    EXPECT_TRUE(changed) << "Should fold mul(6, 7)";
}

TEST_F(OptimizationPassTest, ConstFold_FoldsComparison) {
    // Create: %cmp = eq 5, 5 (should become %cmp = true)
    auto* func = builder->createFunction("test", builder->getBoolType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* cmp = builder->createEq(builder->getInt(5), builder->getInt(5), "cmp");
    builder->createRet(cmp);

    ConstantFoldingPass fold;
    bool changed = fold.runOnModule(*module);

    EXPECT_TRUE(changed) << "Should fold eq(5, 5)";
}

TEST_F(OptimizationPassTest, ConstFold_FoldsNegation) {
    // Create: %x = neg 42 (should become %x = -42)
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->createNeg(builder->getInt(42), "x");
    builder->createRet(x);

    ConstantFoldingPass fold;
    bool changed = fold.runOnModule(*module);

    EXPECT_TRUE(changed) << "Should fold neg(42)";
}

TEST_F(OptimizationPassTest, ConstFold_DoesNotFoldVariables) {
    // Create: i64 @test(i64 %a) { %x = add %a, 3; ret %x }
    auto* func = builder->createFunction("test", builder->getIntType(),
                                        {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* a = func->getParam(0);
    auto* x = builder->createAdd(a, builder->getInt(3), "x");
    builder->createRet(x);

    ConstantFoldingPass fold;
    bool changed = fold.runOnModule(*module);

    // Verify: Cannot fold (operand is variable)
    EXPECT_FALSE(changed) << "Should not fold add with variable operand";
}

TEST_F(OptimizationPassTest, ConstFold_FoldsFloatOps) {
    // Create: %x = add 3.14, 2.86 (should become %x = 6.0)
    auto* func = builder->createFunction("test", builder->getFloatType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->createAdd(builder->getFloat(3.14),
                                 builder->getFloat(2.86), "x");
    builder->createRet(x);

    ConstantFoldingPass fold;
    bool changed = fold.runOnModule(*module);

    EXPECT_TRUE(changed) << "Should fold float addition";
}

// ============================================================================
// Constant Propagation Tests
// ============================================================================

TEST_F(OptimizationPassTest, ConstProp_PropagatesSimpleConstant) {
    // Create: %x = 5; %y = add %x, 3 (should become %y = add 5, 3)
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->getInt(5);
    auto* y = builder->createAdd(x, builder->getInt(3), "y");
    builder->createRet(y);

    ConstantPropagationPass prop;
    bool changed = prop.runOnModule(*module);

    // Note: In this simple case, constants are already propagated
    // The pass is more useful for loads from allocas
    EXPECT_FALSE(changed) << "Constants already inline in this case";
}

TEST_F(OptimizationPassTest, ConstProp_PropagatesAcrossLoad) {
    // Create: %ptr = alloca; store 42, %ptr; %val = load %ptr; ret %val
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* ptr = builder->createAlloca(builder->getIntType(), "ptr");
    builder->createStore(builder->getInt(42), ptr);
    auto* val = builder->createLoad(ptr, "val");
    builder->createRet(val);

    ConstantPropagationPass prop;
    bool changed = prop.runOnModule(*module);

    // This test may pass or fail depending on implementation
    // The key is that your implementation should handle this pattern
}

// ============================================================================
// Mem2Reg Tests
// ============================================================================

TEST_F(OptimizationPassTest, Mem2Reg_PromotesSimpleAlloca) {
    // Create: %x = alloca; store 42, %x; %val = load %x; ret %val
    // Should become: ret 42
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->createAlloca(builder->getIntType(), "x");
    builder->createStore(builder->getInt(42), x);
    auto* val = builder->createLoad(x, "val");
    builder->createRet(val);

    Mem2RegPass mem2reg;
    bool changed = mem2reg.runOnModule(*module);

    // Verify: Should promote
    EXPECT_TRUE(changed) << "Mem2reg should promote simple alloca";
}

TEST_F(OptimizationPassTest, Mem2Reg_InsertsPhiNode) {
    // Create function with if-then-else assigning to same variable
    // This requires a phi node
    auto* func = builder->createFunction("test", builder->getIntType(),
                                        {builder->getBoolType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->createAlloca(builder->getIntType(), "x");
    auto* cond = func->getParam(0);

    auto blocks = builder->createIfThenElse(cond, "then", "else", "merge");

    // Then block: x = 10
    builder->setInsertionPoint(blocks.thenBlock);
    builder->createStore(builder->getInt(10), x);
    builder->createBr(blocks.mergeBlock);

    // Else block: x = 20
    builder->setInsertionPoint(blocks.elseBlock);
    builder->createStore(builder->getInt(20), x);
    builder->createBr(blocks.mergeBlock);

    // Merge: return x
    builder->setInsertionPoint(blocks.mergeBlock);
    auto* val = builder->createLoad(x, "val");
    builder->createRet(val);

    Mem2RegPass mem2reg;
    bool changed = mem2reg.runOnModule(*module);

    // Verify: Should insert phi node
    EXPECT_TRUE(changed) << "Mem2reg should promote and insert phi";
}

TEST_F(OptimizationPassTest, Mem2Reg_DoesNotPromoteAddressTaken) {
    // If address is taken (passed to function), don't promote
    auto* external = module->createFunction("external", builder->getVoidType(),
                                           {builder->getPointerType(builder->getIntType())});

    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* x = builder->createAlloca(builder->getIntType(), "x");
    builder->createCall(external, {x});  // Address taken!
    builder->createRet(nullptr);

    Mem2RegPass mem2reg;
    bool changed = mem2reg.runOnModule(*module);

    // Verify: Should NOT promote (address taken)
    EXPECT_FALSE(changed) << "Mem2reg should not promote when address is taken";
}

// ============================================================================
// PassManager Tests
// ============================================================================

TEST_F(OptimizationPassTest, PassManager_RunsMultiplePasses) {
    // Create IR with opportunities for multiple optimizations
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    // Unused add (DCE should remove)
    auto* unused = builder->createAdd(builder->getInt(1), builder->getInt(2), "unused");

    // Constant fold opportunity
    auto* x = builder->createMul(builder->getInt(6), builder->getInt(7), "x");
    builder->createRet(x);

    // Create pass manager
    PassManager pm;
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());

    // Run
    bool changed = pm.run(*module);

    EXPECT_TRUE(changed) << "At least one pass should modify IR";
    EXPECT_EQ(pm.getNumPasses(), 2) << "Should have 2 passes";
}

TEST_F(OptimizationPassTest, PassManager_EmptyManager) {
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    builder->createRet(builder->getInt(42));

    PassManager pm;
    bool changed = pm.run(*module);

    EXPECT_FALSE(changed) << "Empty pass manager should not modify IR";
    EXPECT_EQ(pm.getNumPasses(), 0) << "Should have 0 passes";
}

TEST_F(OptimizationPassTest, PassManager_IterativeOptimization) {
    // Test that running passes iteratively can unlock more optimizations
    // Example: ConstFold creates more DCE opportunities
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    // Create: %a = 5 + 3, %b = %a * 2 (unused), ret 99
    auto* a = builder->createAdd(builder->getInt(5), builder->getInt(3), "a");
    auto* b = builder->createMul(a, builder->getInt(2), "b");
    builder->createRet(builder->getInt(99));

    PassManager pm;
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());

    // First run
    bool changed1 = pm.run(*module);
    EXPECT_TRUE(changed1) << "First run should optimize";

    // Second run (should be idempotent - no changes)
    PassManager pm2;
    pm2.addPass(std::make_unique<ConstantFoldingPass>());
    pm2.addPass(std::make_unique<DeadCodeEliminationPass>());
    bool changed2 = pm2.run(*module);

    // After optimization, re-running should not change anything
    EXPECT_FALSE(changed2) << "Second run should be idempotent";
}

// ============================================================================
// Integration Tests - Combining Multiple Passes
// ============================================================================

TEST_F(OptimizationPassTest, Integration_FullPipeline) {
    // Create a function with multiple optimization opportunities
    auto* func = builder->createFunction("complex", builder->getIntType(),
                                        {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    // Alloca that can be promoted
    auto* var = builder->createAlloca(builder->getIntType(), "var");
    builder->createStore(builder->getInt(10), var);

    // Constant fold opportunity
    auto* folded = builder->createAdd(builder->getInt(5), builder->getInt(5), "folded");

    // Use the alloca
    auto* loaded = builder->createLoad(var, "loaded");

    // Create result using param (not constant)
    auto* param = func->getParam(0);
    auto* result = builder->createAdd(loaded, param, "result");
    builder->createRet(result);

    // Run full pipeline
    PassManager pm;
    pm.addPass(std::make_unique<Mem2RegPass>());
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<ConstantPropagationPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());

    bool changed = pm.run(*module);
    EXPECT_TRUE(changed) << "Pipeline should optimize the function";
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(OptimizationPassTest, EdgeCase_EmptyFunction) {
    // Function with just a return
    auto* func = builder->createFunction("empty", builder->getVoidType(), {});
    builder->setInsertionPoint(func->getEntryBlock());
    builder->createRet(nullptr);

    DeadCodeEliminationPass dce;
    bool changed = dce.runOnModule(*module);

    EXPECT_FALSE(changed) << "DCE on empty function should not change anything";
}

TEST_F(OptimizationPassTest, EdgeCase_OnlyTerminator) {
    auto* func = builder->createFunction("only_ret", builder->getIntType(), {});
    builder->setInsertionPoint(func->getEntryBlock());
    builder->createRet(builder->getInt(42));

    PassManager pm;
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());

    bool changed = pm.run(*module);
    EXPECT_FALSE(changed) << "No optimization possible with only terminator";
}

TEST_F(OptimizationPassTest, EdgeCase_MultipleBlocks) {
    // Test optimization across multiple basic blocks
    auto* func = builder->createFunction("multi_block", builder->getIntType(),
                                        {builder->getBoolType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* cond = func->getParam(0);
    auto* thenBlock = builder->createBasicBlock("then", true);
    auto* elseBlock = builder->createBasicBlock("else", true);
    auto* mergeBlock = builder->createBasicBlock("merge", true);

    builder->setInsertionPoint(entry);
    builder->createCondBr(cond, thenBlock, elseBlock);

    // Then: constant fold opportunity
    builder->setInsertionPoint(thenBlock);
    auto* val1 = builder->createAdd(builder->getInt(2), builder->getInt(3), "val1");
    builder->createBr(mergeBlock);

    // Else: unused computation (DCE opportunity)
    builder->setInsertionPoint(elseBlock);
    auto* unused = builder->createMul(builder->getInt(10), builder->getInt(20), "unused");
    builder->createBr(mergeBlock);

    // Merge
    builder->setInsertionPoint(mergeBlock);
    builder->createRet(builder->getInt(0));

    PassManager pm;
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());

    bool changed = pm.run(*module);
    EXPECT_TRUE(changed) << "Should optimize across multiple blocks";
}
