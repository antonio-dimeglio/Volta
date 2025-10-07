#include <gtest/gtest.h>
#include <memory>
#include "IR/IRBuilder.hpp"
#include "IR/Module.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

using namespace volta::ir;

// ============================================================================
// Test Fixture
// ============================================================================

class IRBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("test_module");
        builder = std::make_unique<IRBuilder>(*module);
    }

    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder> builder;
};

// ============================================================================
// Insertion Point Tests
// ============================================================================

TEST_F(IRBuilderTest, InsertionPoint_InitialState) {
    EXPECT_FALSE(builder->hasInsertionPoint());
    EXPECT_EQ(builder->getInsertionBlock(), nullptr);
    EXPECT_EQ(builder->getInsertionPoint(), nullptr);
}

TEST_F(IRBuilderTest, InsertionPoint_SetToBlock) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {}, false);
    auto* block = module->createBasicBlock("entry", func);

    builder->setInsertionPoint(block);

    EXPECT_TRUE(builder->hasInsertionPoint());
    EXPECT_EQ(builder->getInsertionBlock(), block);
    EXPECT_EQ(builder->getInsertionPoint(), nullptr);  // Insert at end
}

TEST_F(IRBuilderTest, InsertionPoint_Clear) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {}, false);
    auto* block = module->createBasicBlock("entry", func);
    builder->setInsertionPoint(block);

    builder->clearInsertionPoint();

    EXPECT_FALSE(builder->hasInsertionPoint());
    EXPECT_EQ(builder->getInsertionBlock(), nullptr);
}

TEST_F(IRBuilderTest, InsertionPoint_Before) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* intVal = builder->getInt(42);
    auto* inst1 = module->createBinaryOp(Instruction::Opcode::Add, intVal, intVal);
    builder->getInsertionBlock()->addInstruction(inst1);

    builder->setInsertionPointBefore(inst1);

    EXPECT_TRUE(builder->hasInsertionPoint());
    EXPECT_EQ(builder->getInsertionPoint(), inst1);
}

TEST_F(IRBuilderTest, InsertionPoint_After) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* intVal = builder->getInt(42);
    auto* inst1 = module->createBinaryOp(Instruction::Opcode::Add, intVal, intVal);
    builder->getInsertionBlock()->addInstruction(inst1);

    builder->setInsertionPointAfter(inst1);

    EXPECT_TRUE(builder->hasInsertionPoint());
    // After inst1 means either at the next instruction or at the end
}

// ============================================================================
// Type Helper Tests
// ============================================================================

TEST_F(IRBuilderTest, TypeHelpers_PrimitiveTypes) {
    auto intType = builder->getIntType();
    auto floatType = builder->getFloatType();
    auto boolType = builder->getBoolType();
    auto stringType = builder->getStringType();
    auto voidType = builder->getVoidType();

    ASSERT_NE(intType, nullptr);
    ASSERT_NE(floatType, nullptr);
    ASSERT_NE(boolType, nullptr);
    ASSERT_NE(stringType, nullptr);
    ASSERT_NE(voidType, nullptr);

    EXPECT_EQ(intType->kind(), IRType::Kind::I64);
    EXPECT_EQ(floatType->kind(), IRType::Kind::F64);
    EXPECT_EQ(boolType->kind(), IRType::Kind::I1);
    EXPECT_EQ(voidType->kind(), IRType::Kind::Void);
}

TEST_F(IRBuilderTest, TypeHelpers_PointerType) {
    auto intType = builder->getIntType();
    auto ptrType = builder->getPointerType(intType);

    ASSERT_NE(ptrType, nullptr);
    EXPECT_EQ(ptrType->kind(), IRType::Kind::Pointer);
}

TEST_F(IRBuilderTest, TypeHelpers_ArrayType) {
    auto intType = builder->getIntType();
    auto arrType = builder->getArrayType(intType, 10);

    ASSERT_NE(arrType, nullptr);
    EXPECT_EQ(arrType->kind(), IRType::Kind::Array);
}

TEST_F(IRBuilderTest, TypeHelpers_OptionType) {
    auto intType = builder->getIntType();
    auto optType = builder->getOptionType(intType);

    ASSERT_NE(optType, nullptr);
    EXPECT_EQ(optType->kind(), IRType::Kind::Option);
}

// ============================================================================
// Function and Block Creation Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateFunction_NoParams) {
    auto* func = builder->createFunction("test", builder->getIntType(), {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "test");
    EXPECT_EQ(func->getReturnType(), builder->getIntType());
    EXPECT_EQ(func->getNumParams(), 0);
    EXPECT_TRUE(func->hasBlocks());  // Should create entry block
    EXPECT_NE(func->getEntryBlock(), nullptr);
}

TEST_F(IRBuilderTest, CreateFunction_WithParams) {
    std::vector<std::shared_ptr<IRType>> paramTypes = {
        builder->getIntType(),
        builder->getFloatType()
    };

    auto* func = builder->createFunction("add", builder->getFloatType(), paramTypes);

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getNumParams(), 2);
    EXPECT_EQ(func->getParam(0)->getType(), builder->getIntType());
    EXPECT_EQ(func->getParam(1)->getType(), builder->getFloatType());
}

TEST_F(IRBuilderTest, CreateFunction_SetsInsertionPoint) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {}, true);

    EXPECT_TRUE(builder->hasInsertionPoint());
    EXPECT_EQ(builder->getInsertionBlock(), func->getEntryBlock());
}

TEST_F(IRBuilderTest, CreateFunction_NoInsertionPoint) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {}, false);

    EXPECT_FALSE(builder->hasInsertionPoint());
}

TEST_F(IRBuilderTest, CreateBasicBlock_Standalone) {
    auto* block = builder->createBasicBlock("test_block", false);

    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getName(), "test_block");
    EXPECT_EQ(block->getParent(), nullptr);  // Not inserted into function
}

TEST_F(IRBuilderTest, CreateBasicBlock_InCurrentFunction) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* block = builder->createBasicBlock("new_block", true);

    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getParent(), func);
    EXPECT_GT(func->getNumBlocks(), 1);  // Entry + new_block
}

// ============================================================================
// Constant Creation Tests
// ============================================================================

TEST_F(IRBuilderTest, ConstantInt_Creation) {
    auto* c1 = builder->getInt(42);
    auto* c2 = builder->getInt(-100);

    ASSERT_NE(c1, nullptr);
    ASSERT_NE(c2, nullptr);
    EXPECT_EQ(c1->getValue(), 42);
    EXPECT_EQ(c2->getValue(), -100);
    EXPECT_EQ(c1->getType(), builder->getIntType());
}

TEST_F(IRBuilderTest, ConstantFloat_Creation) {
    auto* c1 = builder->getFloat(3.14);
    auto* c2 = builder->getFloat(-2.5);

    ASSERT_NE(c1, nullptr);
    ASSERT_NE(c2, nullptr);
    EXPECT_DOUBLE_EQ(c1->getValue(), 3.14);
    EXPECT_DOUBLE_EQ(c2->getValue(), -2.5);
}

TEST_F(IRBuilderTest, ConstantBool_Creation) {
    auto* t = builder->getTrue();
    auto* f = builder->getFalse();
    auto* b1 = builder->getBool(true);
    auto* b2 = builder->getBool(false);

    ASSERT_NE(t, nullptr);
    ASSERT_NE(f, nullptr);
    EXPECT_TRUE(t->getValue());
    EXPECT_FALSE(f->getValue());
    EXPECT_TRUE(b1->getValue());
    EXPECT_FALSE(b2->getValue());
}

TEST_F(IRBuilderTest, ConstantString_Creation) {
    auto* s = builder->getString("hello world");

    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->getValue(), "hello world");
}

TEST_F(IRBuilderTest, ConstantNone_Creation) {
    auto optType = builder->getOptionType(builder->getIntType());
    auto* none = builder->getNone(optType);

    ASSERT_NE(none, nullptr);
    EXPECT_EQ(none->getType(), optType);
}

TEST_F(IRBuilderTest, UndefValue_Creation) {
    auto* undef = builder->getUndef(builder->getIntType());

    ASSERT_NE(undef, nullptr);
    EXPECT_EQ(undef->getType(), builder->getIntType());
}

// ============================================================================
// Arithmetic Instruction Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateAdd) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(10);
    auto* rhs = builder->getInt(20);

    auto* result = builder->createAdd(lhs, rhs, "sum");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getName(), "sum");
    EXPECT_TRUE(isa<Instruction>(result));
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Add);
    EXPECT_EQ(binOp->getLHS(), lhs);
    EXPECT_EQ(binOp->getRHS(), rhs);
}

TEST_F(IRBuilderTest, CreateSub) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(30);
    auto* rhs = builder->getInt(10);

    auto* result = builder->createSub(lhs, rhs, "diff");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Sub);
}

TEST_F(IRBuilderTest, CreateMul) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(5);
    auto* rhs = builder->getInt(6);

    auto* result = builder->createMul(lhs, rhs, "product");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Mul);
}

TEST_F(IRBuilderTest, CreateDiv) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(20);
    auto* rhs = builder->getInt(4);

    auto* result = builder->createDiv(lhs, rhs, "quotient");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Div);
}

TEST_F(IRBuilderTest, CreateRem) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(10);
    auto* rhs = builder->getInt(3);

    auto* result = builder->createRem(lhs, rhs, "remainder");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Rem);
}

TEST_F(IRBuilderTest, CreatePow) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* base = builder->getInt(2);
    auto* exp = builder->getInt(8);

    auto* result = builder->createPow(base, exp, "power");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Pow);
}

TEST_F(IRBuilderTest, CreateNeg) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* operand = builder->getInt(42);

    auto* result = builder->createNeg(operand, "negated");

    ASSERT_NE(result, nullptr);
    auto* unOp = dyn_cast<UnaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(unOp, nullptr);
    EXPECT_EQ(unOp->getOpcode(), Instruction::Opcode::Neg);
}

// ============================================================================
// Comparison Instruction Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateEq) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(10);
    auto* rhs = builder->getInt(10);

    auto* result = builder->createEq(lhs, rhs, "is_equal");

    ASSERT_NE(result, nullptr);
    auto* cmp = dyn_cast<CmpInst>(static_cast<Instruction*>(result));
    ASSERT_NE(cmp, nullptr);
    EXPECT_EQ(cmp->getOpcode(), Instruction::Opcode::Eq);
}

TEST_F(IRBuilderTest, CreateNe) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(10);
    auto* rhs = builder->getInt(20);

    auto* result = builder->createNe(lhs, rhs);

    ASSERT_NE(result, nullptr);
    auto* cmp = dyn_cast<CmpInst>(static_cast<Instruction*>(result));
    ASSERT_NE(cmp, nullptr);
    EXPECT_EQ(cmp->getOpcode(), Instruction::Opcode::Ne);
}

TEST_F(IRBuilderTest, CreateLt) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(10);
    auto* rhs = builder->getInt(20);

    auto* result = builder->createLt(lhs, rhs);

    ASSERT_NE(result, nullptr);
    auto* cmp = dyn_cast<CmpInst>(static_cast<Instruction*>(result));
    ASSERT_NE(cmp, nullptr);
    EXPECT_EQ(cmp->getOpcode(), Instruction::Opcode::Lt);
}

TEST_F(IRBuilderTest, AllComparisonOps) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getInt(10);
    auto* rhs = builder->getInt(20);

    auto* le = builder->createLe(lhs, rhs);
    auto* gt = builder->createGt(lhs, rhs);
    auto* ge = builder->createGe(lhs, rhs);

    ASSERT_NE(le, nullptr);
    ASSERT_NE(gt, nullptr);
    ASSERT_NE(ge, nullptr);

    EXPECT_EQ(dyn_cast<CmpInst>(static_cast<Instruction*>(le))->getOpcode(), Instruction::Opcode::Le);
    EXPECT_EQ(dyn_cast<CmpInst>(static_cast<Instruction*>(gt))->getOpcode(), Instruction::Opcode::Gt);
    EXPECT_EQ(dyn_cast<CmpInst>(static_cast<Instruction*>(ge))->getOpcode(), Instruction::Opcode::Ge);
}

// ============================================================================
// Logical Instruction Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateAnd) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getTrue();
    auto* rhs = builder->getFalse();

    auto* result = builder->createAnd(lhs, rhs, "and_result");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::And);
}

TEST_F(IRBuilderTest, CreateOr) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* lhs = builder->getTrue();
    auto* rhs = builder->getFalse();

    auto* result = builder->createOr(lhs, rhs, "or_result");

    ASSERT_NE(result, nullptr);
    auto* binOp = dyn_cast<BinaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->getOpcode(), Instruction::Opcode::Or);
}

TEST_F(IRBuilderTest, CreateNot) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* operand = builder->getTrue();

    auto* result = builder->createNot(operand, "not_result");

    ASSERT_NE(result, nullptr);
    auto* unOp = dyn_cast<UnaryOperator>(static_cast<Instruction*>(result));
    ASSERT_NE(unOp, nullptr);
    EXPECT_EQ(unOp->getOpcode(), Instruction::Opcode::Not);
}

// ============================================================================
// Memory Instruction Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateAlloca) {
    builder->createFunction("test", builder->getVoidType(), {});

    auto* alloca = builder->createAlloca(builder->getIntType(), "var");

    ASSERT_NE(alloca, nullptr);
    auto* allocaInst = dyn_cast<AllocaInst>(static_cast<Instruction*>(alloca));
    ASSERT_NE(allocaInst, nullptr);
    EXPECT_EQ(allocaInst->getAllocatedType(), builder->getIntType());
    EXPECT_EQ(allocaInst->getName(), "var");
}

TEST_F(IRBuilderTest, CreateLoad) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* ptr = builder->createAlloca(builder->getIntType());

    auto* load = builder->createLoad(ptr, "loaded_val");

    ASSERT_NE(load, nullptr);
    auto* loadInst = dyn_cast<LoadInst>(static_cast<Instruction*>(load));
    ASSERT_NE(loadInst, nullptr);
    EXPECT_EQ(loadInst->getPointer(), ptr);
}

TEST_F(IRBuilderTest, CreateStore) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* ptr = builder->createAlloca(builder->getIntType());
    auto* value = builder->getInt(42);

    builder->createStore(value, ptr);

    // Store doesn't return a value, but should add instruction to block
    auto* block = builder->getInsertionBlock();
    EXPECT_GT(block->getNumInstructions(), 1);  // alloca + store
}

TEST_F(IRBuilderTest, CreateGCAlloc) {
    builder->createFunction("test", builder->getVoidType(), {});

    auto* gcAlloc = builder->createGCAlloc(builder->getIntType(), "heap_var");

    ASSERT_NE(gcAlloc, nullptr);
    auto* gcAllocInst = dyn_cast<GCAllocInst>(static_cast<Instruction*>(gcAlloc));
    ASSERT_NE(gcAllocInst, nullptr);
}

// ============================================================================
// Array Instruction Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateArrayNew) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* size = builder->getInt(10);

    auto* arr = builder->createArrayNew(builder->getIntType(), size, "arr");

    ASSERT_NE(arr, nullptr);
    auto* arrInst = dyn_cast<ArrayNewInst>(static_cast<Instruction*>(arr));
    ASSERT_NE(arrInst, nullptr);
}

TEST_F(IRBuilderTest, CreateArrayGet) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* size = builder->getInt(10);
    auto* arr = builder->createArrayNew(builder->getIntType(), size);
    auto* index = builder->getInt(5);

    auto* elem = builder->createArrayGet(arr, index, "elem");

    ASSERT_NE(elem, nullptr);
    auto* getInst = dyn_cast<ArrayGetInst>(static_cast<Instruction*>(elem));
    ASSERT_NE(getInst, nullptr);
}

TEST_F(IRBuilderTest, CreateArraySet) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* size = builder->getInt(10);
    auto* arr = builder->createArrayNew(builder->getIntType(), size);
    auto* index = builder->getInt(5);
    auto* value = builder->getInt(42);

    builder->createArraySet(arr, index, value);

    // Should add instruction to block (ArrayNew + ArraySet = 2)
    auto* block = builder->getInsertionBlock();
    EXPECT_EQ(block->getNumInstructions(), 2);
}

TEST_F(IRBuilderTest, CreateArrayLen) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* size = builder->getInt(10);
    auto* arr = builder->createArrayNew(builder->getIntType(), size);

    auto* len = builder->createArrayLen(arr, "len");

    ASSERT_NE(len, nullptr);
    auto* lenInst = dyn_cast<ArrayLenInst>(static_cast<Instruction*>(len));
    ASSERT_NE(lenInst, nullptr);
}

TEST_F(IRBuilderTest, CreateArraySlice) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* size = builder->getInt(10);
    auto* arr = builder->createArrayNew(builder->getIntType(), size);
    auto* start = builder->getInt(2);
    auto* end = builder->getInt(8);

    auto* slice = builder->createArraySlice(arr, start, end, "slice");

    ASSERT_NE(slice, nullptr);
    auto* sliceInst = dyn_cast<ArraySliceInst>(static_cast<Instruction*>(slice));
    ASSERT_NE(sliceInst, nullptr);
}

// ============================================================================
// Type Conversion Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateCast) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* intVal = builder->getInt(42);

    auto* floatVal = builder->createCast(intVal, builder->getFloatType(), "as_float");

    ASSERT_NE(floatVal, nullptr);
    auto* cast = dyn_cast<CastInst>(static_cast<Instruction*>(floatVal));
    ASSERT_NE(cast, nullptr);
    EXPECT_EQ(cast->getSrcType(), builder->getIntType());
    EXPECT_EQ(cast->getDestType(), builder->getFloatType());
}

// ============================================================================
// Control Flow Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateRet_WithValue) {
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* retVal = builder->getInt(42);

    auto* block = builder->getInsertionBlock();  // Save block before createRet clears it
    builder->createRet(retVal);
    ASSERT_TRUE(block->hasTerminator());
    auto* term = block->getTerminator();
    auto* ret = dyn_cast<ReturnInst>(term);
    ASSERT_NE(ret, nullptr);
    EXPECT_EQ(ret->getReturnValue(), retVal);
}

TEST_F(IRBuilderTest, CreateRet_Void) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});

    auto* block = builder->getInsertionBlock();
    builder->createRet();

    ASSERT_TRUE(block->hasTerminator());
    auto* ret = dyn_cast<ReturnInst>(block->getTerminator());
    ASSERT_NE(ret, nullptr);
    EXPECT_FALSE(ret->hasReturnValue());
}

TEST_F(IRBuilderTest, CreateBr) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* targetBlock = builder->createBasicBlock("target");
    auto* block = builder->getInsertionBlock();
    builder->createBr(targetBlock);
    ASSERT_TRUE(block->hasTerminator());
    auto* br = dyn_cast<BranchInst>(block->getTerminator());
    ASSERT_NE(br, nullptr);
    EXPECT_EQ(br->getDestination(), targetBlock);
}

TEST_F(IRBuilderTest, CreateCondBr) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* cond = builder->getTrue();
    auto* thenBlock = builder->createBasicBlock("then");
    auto* elseBlock = builder->createBasicBlock("else");
    auto* block = builder->getInsertionBlock();
    builder->createCondBr(cond, thenBlock, elseBlock);
    ASSERT_TRUE(block->hasTerminator());
    auto* condBr = dyn_cast<CondBranchInst>(block->getTerminator());

    ASSERT_NE(condBr, nullptr);
    EXPECT_EQ(condBr->getCondition(), cond);
    EXPECT_EQ(condBr->getTrueDest(), thenBlock);
    EXPECT_EQ(condBr->getFalseDest(), elseBlock);
}

// ============================================================================
// Function Call Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateCall_NoArgs) {
    auto* callee = builder->createFunction("callee", builder->getIntType(), {}, false);
    auto* caller = builder->createFunction("caller", builder->getVoidType(), {});

    auto* result = builder->createCall(callee, {}, "result");

    ASSERT_NE(result, nullptr);
    auto* call = dyn_cast<CallInst>(static_cast<Instruction*>(result));
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->getCallee(), callee);
    EXPECT_EQ(call->getNumArguments(), 0);
}

TEST_F(IRBuilderTest, CreateCall_WithArgs) {
    std::vector<std::shared_ptr<IRType>> paramTypes = {
        builder->getIntType(),
        builder->getIntType()
    };
    auto* callee = builder->createFunction("add", builder->getIntType(), paramTypes, false);
    auto* caller = builder->createFunction("caller", builder->getVoidType(), {});

    auto* arg1 = builder->getInt(10);
    auto* arg2 = builder->getInt(20);
    auto* result = builder->createCall(callee, {arg1, arg2}, "sum");

    ASSERT_NE(result, nullptr);
    auto* call = dyn_cast<CallInst>(static_cast<Instruction*>(result));
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->getNumArguments(), 2);
    EXPECT_EQ(call->getArgument(0), arg1);
    EXPECT_EQ(call->getArgument(1), arg2);
}

// ============================================================================
// Phi Node Tests
// ============================================================================

TEST_F(IRBuilderTest, CreatePhi_Empty) {
    builder->createFunction("test", builder->getVoidType(), {});

    auto* phi = builder->createPhi(builder->getIntType(), {}, "merged");

    ASSERT_NE(phi, nullptr);
    auto* phiNode = dyn_cast<PhiNode>(static_cast<Instruction*>(phi));
    ASSERT_NE(phiNode, nullptr);
    EXPECT_EQ(phiNode->getNumIncomingValues(), 0);
}

// ============================================================================
// High-Level Control Flow Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateIfThenElse) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* cond = builder->getTrue();

    auto blocks = builder->createIfThenElse(cond, "if.then", "if.else", "if.merge");

    ASSERT_NE(blocks.thenBlock, nullptr);
    ASSERT_NE(blocks.elseBlock, nullptr);
    ASSERT_NE(blocks.mergeBlock, nullptr);

    EXPECT_EQ(blocks.thenBlock->getName(), "if.then");
    EXPECT_EQ(blocks.elseBlock->getName(), "if.else");
    EXPECT_EQ(blocks.mergeBlock->getName(), "if.merge");
}

TEST_F(IRBuilderTest, CreateLoop) {
    builder->createFunction("test", builder->getVoidType(), {});

    auto blocks = builder->createLoop("loop.header", "loop.body", "loop.exit");

    ASSERT_NE(blocks.headerBlock, nullptr);
    ASSERT_NE(blocks.bodyBlock, nullptr);
    ASSERT_NE(blocks.exitBlock, nullptr);

    EXPECT_EQ(blocks.headerBlock->getName(), "loop.header");
    EXPECT_EQ(blocks.bodyBlock->getName(), "loop.body");
    EXPECT_EQ(blocks.exitBlock->getName(), "loop.exit");
}

// ============================================================================
// Variable Management Tests
// ============================================================================

TEST_F(IRBuilderTest, CreateVariable_NoInitializer) {
    builder->createFunction("test", builder->getVoidType(), {});

    auto* var = builder->createVariable("x", builder->getIntType());

    ASSERT_NE(var, nullptr);
    auto* allocaInst = dyn_cast<AllocaInst>(static_cast<Instruction*>(var));
    ASSERT_NE(allocaInst, nullptr);
}

TEST_F(IRBuilderTest, CreateVariable_WithInitializer) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* initVal = builder->getInt(42);

    auto* var = builder->createVariable("x", builder->getIntType(), initVal);

    ASSERT_NE(var, nullptr);
    // Should have both alloca and store instructions
    auto* block = builder->getInsertionBlock();
    EXPECT_GE(block->getNumInstructions(), 2);
}

TEST_F(IRBuilderTest, ReadVariable) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* var = builder->createVariable("x", builder->getIntType(), builder->getInt(42));

    auto* value = builder->readVariable(var, "x_val");

    ASSERT_NE(value, nullptr);
    auto* loadInst = dyn_cast<LoadInst>(static_cast<Instruction*>(value));
    ASSERT_NE(loadInst, nullptr);
}

TEST_F(IRBuilderTest, WriteVariable) {
    builder->createFunction("test", builder->getVoidType(), {});
    auto* var = builder->createVariable("x", builder->getIntType());
    auto* newVal = builder->getInt(100);

    builder->writeVariable(var, newVal);

    // Should add a store instruction
    auto* block = builder->getInsertionBlock();
    EXPECT_GE(block->getNumInstructions(), 2);
}

// ============================================================================
// Named Value Tests
// ============================================================================

TEST_F(IRBuilderTest, NamedValues_SetAndGet) {
    auto* val = builder->getInt(42);

    builder->setNamedValue("x", val);
    auto* retrieved = builder->getNamedValue("x");

    EXPECT_EQ(retrieved, val);
}

TEST_F(IRBuilderTest, NamedValues_NotFound) {
    auto* val = builder->getNamedValue("nonexistent");

    EXPECT_EQ(val, nullptr);
}

TEST_F(IRBuilderTest, NamedValues_Remove) {
    auto* val = builder->getInt(42);
    builder->setNamedValue("x", val);

    builder->removeNamedValue("x");
    auto* retrieved = builder->getNamedValue("x");

    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(IRBuilderTest, NamedValues_Clear) {
    builder->setNamedValue("x", builder->getInt(1));
    builder->setNamedValue("y", builder->getInt(2));
    builder->setNamedValue("z", builder->getInt(3));

    builder->clearNamedValues();

    EXPECT_EQ(builder->getNamedValue("x"), nullptr);
    EXPECT_EQ(builder->getNamedValue("y"), nullptr);
    EXPECT_EQ(builder->getNamedValue("z"), nullptr);
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_F(IRBuilderTest, GetCurrentFunction) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});

    auto* currentFunc = builder->getCurrentFunction();

    EXPECT_EQ(currentFunc, func);
}

TEST_F(IRBuilderTest, GetCurrentFunction_NoInsertionPoint) {
    auto* currentFunc = builder->getCurrentFunction();

    EXPECT_EQ(currentFunc, nullptr);
}

TEST_F(IRBuilderTest, Validate_ValidState) {
    builder->createFunction("test", builder->getVoidType(), {});

    std::string error;
    bool valid = builder->validate(&error);

    EXPECT_TRUE(valid);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(IRBuilderTest, Integration_SimpleFunction) {
    // Build: function @add(a: i64, b: i64) -> i64 {
    //          entry:
    //            %sum = add i64 %a, %b
    //            ret i64 %sum
    //        }

    std::vector<std::shared_ptr<IRType>> params = {
        builder->getIntType(),
        builder->getIntType()
    };
    auto* func = builder->createFunction("add", builder->getIntType(), params);

    auto* a = func->getParam(0);
    auto* b = func->getParam(1);
    auto* sum = builder->createAdd(a, b, "sum");
    builder->createRet(sum);

    EXPECT_EQ(func->getNumBlocks(), 1);
    auto* entry = func->getEntryBlock();
    EXPECT_EQ(entry->getNumInstructions(), 2);  // add + ret
    EXPECT_TRUE(entry->hasTerminator());
}

TEST_F(IRBuilderTest, Integration_IfThenElse) {
    // Build: function @abs(x: i64) -> i64 {
    //          entry:
    //            %cond = lt i64 %x, 0
    //            br %cond, if.then, if.else
    //          if.then:
    //            %neg = neg i64 %x
    //            br if.merge
    //          if.else:
    //            br if.merge
    //          if.merge:
    //            %result = phi i64 [%neg, if.then], [%x, if.else]
    //            ret i64 %result
    //        }

    std::vector<std::shared_ptr<IRType>> params = {builder->getIntType()};
    auto* func = builder->createFunction("abs", builder->getIntType(), params);

    auto* x = func->getParam(0);
    auto* zero = builder->getInt(0);
    auto* cond = builder->createLt(x, zero, "cond");

    auto blocks = builder->createIfThenElse(cond);

    // Build then branch
    builder->setInsertionPoint(blocks.thenBlock);
    auto* neg = builder->createNeg(x, "neg");
    builder->createBr(blocks.mergeBlock);

    // Build else branch
    builder->setInsertionPoint(blocks.elseBlock);
    builder->createBr(blocks.mergeBlock);

    // Build merge
    builder->setInsertionPoint(blocks.mergeBlock);
    std::vector<PhiNode::IncomingValue> incoming = {
        {neg, blocks.thenBlock},
        {x, blocks.elseBlock}
    };
    auto* result = builder->createPhi(builder->getIntType(), incoming, "result");
    builder->createRet(result);

    EXPECT_EQ(func->getNumBlocks(), 4);  // entry + then + else + merge
}

TEST_F(IRBuilderTest, Integration_Loop) {
    // Build a simple counting loop
    auto* func = builder->createFunction("count", builder->getVoidType(), {});

    auto* counter = builder->createVariable("counter", builder->getIntType(), builder->getInt(0));
    auto* limit = builder->getInt(10);

    auto blocks = builder->createLoop();

    // Loop header: check condition
    builder->setInsertionPoint(blocks.headerBlock);
    auto* i = builder->readVariable(counter);
    auto* cond = builder->createLt(i, limit, "cond");
    builder->createCondBr(cond, blocks.bodyBlock, blocks.exitBlock);

    // Loop body: increment counter
    builder->setInsertionPoint(blocks.bodyBlock);
    auto* i2 = builder->readVariable(counter);
    auto* one = builder->getInt(1);
    auto* next = builder->createAdd(i2, one, "next");
    builder->writeVariable(counter, next);
    builder->createBr(blocks.headerBlock);

    // Exit
    builder->setInsertionPoint(blocks.exitBlock);
    builder->createRet();

    EXPECT_EQ(func->getNumBlocks(), 4);  // entry + header + body + exit
}
