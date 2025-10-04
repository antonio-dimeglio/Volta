#include <gtest/gtest.h>
#include <memory>
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

using namespace volta::ir;

// Test helper to create a simple int type (IR type, not semantic)
std::shared_ptr<IRType> makeIntType() {
    return std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
}

// Test helper to create a simple float type
std::shared_ptr<IRType> makeFloatType() {
    return std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
}

// Test helper to create a bool type
std::shared_ptr<IRType> makeBoolType() {
    return std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
}

// Global cleanup tracker for standalone tests
static std::vector<Value*> g_allocatedValues;

// Helper to track allocated values for cleanup
template<typename T>
T* track(T* ptr) {
    g_allocatedValues.push_back(ptr);
    return ptr;
}

// Cleanup function to be called after each test
void cleanupAllocatedValues() {
    for (auto* val : g_allocatedValues) {
        delete val;
    }
    g_allocatedValues.clear();
}

// Test listener to cleanup after each test
class CleanupListener : public ::testing::EmptyTestEventListener {
    void OnTestEnd(const ::testing::TestInfo&) override {
        cleanupAllocatedValues();
    }
};

// Register the listener
struct ListenerRegistrar {
    ListenerRegistrar() {
        ::testing::TestEventListeners& listeners =
            ::testing::UnitTest::GetInstance()->listeners();
        listeners.Append(new CleanupListener);
    }
};
static ListenerRegistrar g_registrar;

// ============================================================================
// Opcode Name Tests
// ============================================================================

TEST(InstructionTest, OpcodeNames_Arithmetic) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Add), "add");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Sub), "sub");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Mul), "mul");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Div), "div");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Rem), "rem");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Pow), "pow");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Neg), "neg");
}

TEST(InstructionTest, OpcodeNames_Comparison) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Eq), "eq");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Ne), "ne");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Lt), "lt");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Le), "le");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Gt), "gt");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Ge), "ge");
}

TEST(InstructionTest, OpcodeNames_Logical) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::And), "and");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Or), "or");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Not), "not");
}

TEST(InstructionTest, OpcodeNames_Memory) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Alloca), "alloca");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Load), "load");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Store), "store");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::GCAlloc), "gcalloc");
}

TEST(InstructionTest, OpcodeNames_Array) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::ArrayNew), "array.new");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::ArrayGet), "array.get");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::ArraySet), "array.set");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::ArrayLen), "array.len");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::ArraySlice), "array.slice");
}

TEST(InstructionTest, OpcodeNames_ControlFlow) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Ret), "ret");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Br), "br");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::CondBr), "condbr");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Switch), "switch");
}

TEST(InstructionTest, OpcodeNames_Call) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Call), "call");
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::CallIndirect), "call.indirect");
}

TEST(InstructionTest, OpcodeNames_SSA) {
    EXPECT_STREQ(Instruction::getOpcodeName(Instruction::Opcode::Phi), "phi");
}

// ============================================================================
// Binary Operator Tests
// ============================================================================

TEST(BinaryOperatorTest, Creation_Add) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));

    auto add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs, "sum"));

    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->getOpcode(), Instruction::Opcode::Add);
    EXPECT_EQ(add->getLHS(), lhs);
    EXPECT_EQ(add->getRHS(), rhs);
    EXPECT_EQ(add->getName(), "sum");
    EXPECT_EQ(add->getNumOperands(), 2);
}

TEST(BinaryOperatorTest, TypeQueries) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));
    auto add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs));

    EXPECT_TRUE(add->isBinaryOp());
    EXPECT_TRUE(add->isArithmetic());
    EXPECT_FALSE(add->isTerminator());
    EXPECT_FALSE(add->isComparison());
}

TEST(BinaryOperatorTest, AllArithmeticOpcodes) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));

    auto sub = track(BinaryOperator::create(Instruction::Opcode::Sub, lhs, rhs, "diff"));
    EXPECT_EQ(sub->getOpcode(), Instruction::Opcode::Sub);

    auto mul = track(BinaryOperator::create(Instruction::Opcode::Mul, lhs, rhs));
    EXPECT_EQ(mul->getOpcode(), Instruction::Opcode::Mul);

    auto div = track(BinaryOperator::create(Instruction::Opcode::Div, lhs, rhs));
    EXPECT_EQ(div->getOpcode(), Instruction::Opcode::Div);
}

TEST(BinaryOperatorTest, OperandModification) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));
    auto add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs));

    auto newRhs = track(ConstantInt::get(30, intType));
    add->setRHS(newRhs);
    EXPECT_EQ(add->getRHS(), newRhs);
}

// ============================================================================
// Unary Operator Tests
// ============================================================================

TEST(UnaryOperatorTest, Creation_Neg) {
    auto intType = makeIntType();
    auto operand = track(ConstantInt::get(42, intType));

    auto neg = UnaryOperator::create(Instruction::Opcode::Neg, operand, "negated");

    ASSERT_NE(neg, nullptr);
    EXPECT_EQ(neg->getOpcode(), Instruction::Opcode::Neg);
    EXPECT_EQ(neg->getOperand(), operand);
    EXPECT_EQ(neg->getName(), "negated");
    EXPECT_EQ(neg->getNumOperands(), 1);
}

TEST(UnaryOperatorTest, TypeQueries) {
    auto intType = makeIntType();
    auto operand = track(ConstantInt::get(42, intType));
    auto neg = UnaryOperator::create(Instruction::Opcode::Neg, operand);

    EXPECT_TRUE(neg->isUnaryOp());
    EXPECT_TRUE(neg->isArithmetic());
    EXPECT_FALSE(neg->isTerminator());
    EXPECT_FALSE(neg->isBinaryOp());
}

TEST(UnaryOperatorTest, Not) {
    auto boolType = makeBoolType();
    auto boolVal = ConstantBool::get(true, boolType);
    auto notOp = UnaryOperator::create(Instruction::Opcode::Not, boolVal);

    EXPECT_EQ(notOp->getOpcode(), Instruction::Opcode::Not);
    EXPECT_TRUE(notOp->isUnaryOp());
}

// ============================================================================
// Comparison Instruction Tests
// ============================================================================

TEST(ComparisonTest, Creation_Eq) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));

    auto eq = CmpInst::create(Instruction::Opcode::Eq, lhs, rhs, "is_equal");

    ASSERT_NE(eq, nullptr);
    EXPECT_EQ(eq->getOpcode(), Instruction::Opcode::Eq);
    EXPECT_EQ(eq->getLHS(), lhs);
    EXPECT_EQ(eq->getRHS(), rhs);
}

TEST(ComparisonTest, TypeQueries) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));
    auto eq = CmpInst::create(Instruction::Opcode::Eq, lhs, rhs);

    EXPECT_TRUE(eq->isComparison());
    EXPECT_FALSE(eq->isArithmetic());
    EXPECT_FALSE(eq->isBinaryOp()); // Comparisons are not classified as binary ops
}

TEST(ComparisonTest, AllComparisonOpcodes) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));

    auto ne = CmpInst::create(Instruction::Opcode::Ne, lhs, rhs);
    EXPECT_EQ(ne->getOpcode(), Instruction::Opcode::Ne);

    auto lt = CmpInst::create(Instruction::Opcode::Lt, lhs, rhs);
    EXPECT_EQ(lt->getOpcode(), Instruction::Opcode::Lt);

    auto le = CmpInst::create(Instruction::Opcode::Le, lhs, rhs);
    EXPECT_EQ(le->getOpcode(), Instruction::Opcode::Le);

    auto gt = CmpInst::create(Instruction::Opcode::Gt, lhs, rhs);
    EXPECT_EQ(gt->getOpcode(), Instruction::Opcode::Gt);

    auto ge = CmpInst::create(Instruction::Opcode::Ge, lhs, rhs);
    EXPECT_EQ(ge->getOpcode(), Instruction::Opcode::Ge);
}

// ============================================================================
// Memory Instruction Tests
// ============================================================================

TEST(MemoryTest, AllocaInst) {
    auto intType = makeIntType();
    auto alloca = AllocaInst::create(intType, "var");

    ASSERT_NE(alloca, nullptr);
    EXPECT_EQ(alloca->getOpcode(), Instruction::Opcode::Alloca);
    EXPECT_EQ(alloca->getAllocatedType(), intType);
    EXPECT_EQ(alloca->getName(), "var");
    EXPECT_TRUE(alloca->isMemoryOp());
    EXPECT_FALSE(alloca->isTerminator());
}

TEST(MemoryTest, LoadInst) {
    auto intType = makeIntType();
    auto alloca = AllocaInst::create(intType);
    auto load = LoadInst::create(alloca, "loaded_val");

    ASSERT_NE(load, nullptr);
    EXPECT_EQ(load->getOpcode(), Instruction::Opcode::Load);
    EXPECT_EQ(load->getPointer(), alloca);
    EXPECT_TRUE(load->isMemoryOp());
}

TEST(MemoryTest, StoreInst) {
    auto intType = makeIntType();
    auto alloca = AllocaInst::create(intType);
    auto value = track(ConstantInt::get(42, intType));
    auto store = StoreInst::create(value, alloca);

    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store->getOpcode(), Instruction::Opcode::Store);
    EXPECT_EQ(store->getValue(), value);
    EXPECT_EQ(store->getPointer(), alloca);
    EXPECT_TRUE(store->isMemoryOp());
}

TEST(MemoryTest, GCAllocInst) {
    auto intType = makeIntType();
    auto gcAlloc = GCAllocInst::create(intType, "heap_var");

    ASSERT_NE(gcAlloc, nullptr);
    EXPECT_EQ(gcAlloc->getOpcode(), Instruction::Opcode::GCAlloc);
    EXPECT_EQ(gcAlloc->getAllocatedType(), intType);
    EXPECT_TRUE(gcAlloc->isMemoryOp());
}

// ============================================================================
// Array Instruction Tests
// ============================================================================

TEST(ArrayTest, ArrayNewInst) {
    auto intType = makeIntType();
    auto size = track(ConstantInt::get(10, intType));
    auto arrayNew = ArrayNewInst::create(intType, size, "arr");

    ASSERT_NE(arrayNew, nullptr);
    EXPECT_EQ(arrayNew->getOpcode(), Instruction::Opcode::ArrayNew);
    EXPECT_EQ(arrayNew->getElementType(), intType);
    EXPECT_EQ(arrayNew->getSize(), size);
    EXPECT_TRUE(arrayNew->isArrayOp());
}

TEST(ArrayTest, ArrayGetInst) {
    auto intType = makeIntType();
    auto size = track(ConstantInt::get(10, intType));
    auto arrayNew = ArrayNewInst::create(intType, size);
    auto index = track(ConstantInt::get(5, intType));
    auto arrayGet = ArrayGetInst::create(arrayNew, index, "elem");

    ASSERT_NE(arrayGet, nullptr);
    EXPECT_EQ(arrayGet->getOpcode(), Instruction::Opcode::ArrayGet);
    EXPECT_EQ(arrayGet->getArray(), arrayNew);
    EXPECT_EQ(arrayGet->getIndex(), index);
    EXPECT_TRUE(arrayGet->isArrayOp());
}

TEST(ArrayTest, ArraySetInst) {
    auto intType = makeIntType();
    auto size = track(ConstantInt::get(10, intType));
    auto arrayNew = ArrayNewInst::create(intType, size);
    auto index = track(ConstantInt::get(5, intType));
    auto value = track(ConstantInt::get(42, intType));
    auto arraySet = ArraySetInst::create(arrayNew, index, value);

    ASSERT_NE(arraySet, nullptr);
    EXPECT_EQ(arraySet->getOpcode(), Instruction::Opcode::ArraySet);
    EXPECT_EQ(arraySet->getArray(), arrayNew);
    EXPECT_EQ(arraySet->getIndex(), index);
    EXPECT_EQ(arraySet->getValue(), value);
}

TEST(ArrayTest, ArrayLenInst) {
    auto intType = makeIntType();
    auto size = track(ConstantInt::get(10, intType));
    auto arrayNew = ArrayNewInst::create(intType, size);
    auto arrayLen = ArrayLenInst::create(arrayNew, "len");

    ASSERT_NE(arrayLen, nullptr);
    EXPECT_EQ(arrayLen->getOpcode(), Instruction::Opcode::ArrayLen);
    EXPECT_EQ(arrayLen->getArray(), arrayNew);
}

TEST(ArrayTest, ArraySliceInst) {
    auto intType = makeIntType();
    auto size = track(ConstantInt::get(10, intType));
    auto arrayNew = ArrayNewInst::create(intType, size);
    auto start = track(ConstantInt::get(2, intType));
    auto end = track(ConstantInt::get(8, intType));
    auto arraySlice = ArraySliceInst::create(arrayNew, start, end, "slice");

    ASSERT_NE(arraySlice, nullptr);
    EXPECT_EQ(arraySlice->getOpcode(), Instruction::Opcode::ArraySlice);
    EXPECT_EQ(arraySlice->getArray(), arrayNew);
    EXPECT_EQ(arraySlice->getStart(), start);
    EXPECT_EQ(arraySlice->getEnd(), end);
}

// ============================================================================
// Type Instruction Tests
// ============================================================================

TEST(TypeTest, CastInst) {
    auto intType = makeIntType();
    auto floatType = makeFloatType();
    auto intVal = ConstantInt::get(42, intType);
    auto cast = CastInst::create(intVal, floatType, "as_float");

    ASSERT_NE(cast, nullptr);
    EXPECT_EQ(cast->getOpcode(), Instruction::Opcode::Cast);
    EXPECT_EQ(cast->getValue(), intVal);
    EXPECT_EQ(cast->getSrcType(), intType);
    EXPECT_EQ(cast->getDestType(), floatType);
}

// ============================================================================
// Control Flow Instruction Tests
// ============================================================================

TEST(ControlFlowTest, ReturnInst_WithValue) {
    auto intType = makeIntType();
    auto returnVal = ConstantInt::get(42, intType);
    auto ret = ReturnInst::create(returnVal);

    ASSERT_NE(ret, nullptr);
    EXPECT_EQ(ret->getOpcode(), Instruction::Opcode::Ret);
    EXPECT_TRUE(ret->hasReturnValue());
    EXPECT_EQ(ret->getReturnValue(), returnVal);
    EXPECT_TRUE(ret->isTerminator());
}

TEST(ControlFlowTest, ReturnInst_Void) {
    auto retVoid = ReturnInst::create(nullptr);

    ASSERT_NE(retVoid, nullptr);
    EXPECT_FALSE(retVoid->hasReturnValue());
    EXPECT_TRUE(retVoid->isTerminator());
}

// ============================================================================
// Phi Node Tests
// ============================================================================

TEST(PhiNodeTest, Creation) {
    auto intType = makeIntType();
    auto phi = PhiNode::create(intType, {}, "merged");

    ASSERT_NE(phi, nullptr);
    EXPECT_EQ(phi->getOpcode(), Instruction::Opcode::Phi);
    EXPECT_EQ(phi->getType(), intType);
    EXPECT_EQ(phi->getNumIncomingValues(), 0);
}

// ============================================================================
// Instruction Query Tests
// ============================================================================

TEST(InstructionQueryTest, BinaryOperator) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));
    auto add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs));

    EXPECT_TRUE(add->isBinaryOp());
    EXPECT_TRUE(add->isArithmetic());
    EXPECT_FALSE(add->isUnaryOp());
    EXPECT_FALSE(add->isComparison());
    EXPECT_FALSE(add->isMemoryOp());
    EXPECT_FALSE(add->isArrayOp());
    EXPECT_FALSE(add->isTerminator());
}

TEST(InstructionQueryTest, Comparison) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));
    auto cmp = CmpInst::create(Instruction::Opcode::Lt, lhs, rhs);

    EXPECT_TRUE(cmp->isComparison());
    EXPECT_FALSE(cmp->isArithmetic());
    EXPECT_FALSE(cmp->isBinaryOp());
}

TEST(InstructionQueryTest, MemoryOp) {
    auto intType = makeIntType();
    auto alloca = AllocaInst::create(intType);

    EXPECT_TRUE(alloca->isMemoryOp());
    EXPECT_FALSE(alloca->isArithmetic());
}

TEST(InstructionQueryTest, ArrayOp) {
    auto intType = makeIntType();
    auto size = track(ConstantInt::get(10, intType));
    auto arrayNew = ArrayNewInst::create(intType, size);

    EXPECT_TRUE(arrayNew->isArrayOp());
    EXPECT_FALSE(arrayNew->isMemoryOp());
}

TEST(InstructionQueryTest, Terminator) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto ret = ReturnInst::create(lhs);

    EXPECT_TRUE(ret->isTerminator());
    EXPECT_FALSE(ret->isArithmetic());
}

// ============================================================================
// Use-Def Chain Tests
// ============================================================================

TEST(UseDefChainTest, InitialUses) {
    auto intType = makeIntType();
    auto constant = ConstantInt::get(42, intType);

    EXPECT_EQ(constant->getNumUses(), 0);
    EXPECT_FALSE(constant->hasUses());
}

TEST(UseDefChainTest, SingleUse) {
    auto intType = makeIntType();
    auto constant = ConstantInt::get(42, intType);
    auto neg = UnaryOperator::create(Instruction::Opcode::Neg, constant);

    EXPECT_EQ(constant->getNumUses(), 1);
    EXPECT_TRUE(constant->hasUses());
}

TEST(UseDefChainTest, MultipleUses) {
    auto intType = makeIntType();
    auto constant = ConstantInt::get(42, intType);
    auto neg = UnaryOperator::create(Instruction::Opcode::Neg, constant);
    auto add = track(BinaryOperator::create(Instruction::Opcode::Add, constant, constant));

    // constant is used once in neg, twice in add = 3 total
    EXPECT_EQ(constant->getNumUses(), 3);
}

// ============================================================================
// ToString Tests
// ============================================================================

TEST(ToStringTest, BinaryOperator) {
    auto intType = makeIntType();
    auto lhs = track(ConstantInt::get(10, intType));
    auto rhs = track(ConstantInt::get(20, intType));
    auto add = track(BinaryOperator::create(Instruction::Opcode::Add, lhs, rhs, "sum"));

    std::string str = add->toString();

    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("add"), std::string::npos);
}
