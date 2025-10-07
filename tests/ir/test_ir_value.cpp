#include "IR/Value.hpp"
#include "IR/IRType.hpp"
#include "IR/Module.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace volta::ir;

// Test fixture for Value tests
class ValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create IR types (not semantic types)
        intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
        floatType = std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
        boolType = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
        stringType = std::make_shared<IRPrimitiveType>(IRType::Kind::String);
        voidType = std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
    }



    std::shared_ptr<IRType> intType;
    std::shared_ptr<IRType> floatType;
    std::shared_ptr<IRType> boolType;
    std::shared_ptr<IRType> stringType;
    std::shared_ptr<IRType> voidType;
    Module module{"test"};
};

// ============================================================================
// ConstantInt Tests
// ============================================================================

TEST_F(ValueTest, ConstantIntCreation) {
    auto* ci = module.getConstantInt(42, intType);

    ASSERT_NE(ci, nullptr);
    EXPECT_EQ(ci->getValue(), 42);
    EXPECT_EQ(ci->getKind(), Value::ValueKind::ConstantInt);
    EXPECT_EQ(ci->getType(), intType);
}

TEST_F(ValueTest, ConstantIntNegativeValue) {
    auto* ci = module.getConstantInt(-100, intType);

    ASSERT_NE(ci, nullptr);
    EXPECT_EQ(ci->getValue(), -100);
}

TEST_F(ValueTest, ConstantIntZero) {
    auto* ci = module.getConstantInt(0, intType);

    ASSERT_NE(ci, nullptr);
    EXPECT_EQ(ci->getValue(), 0);
}

TEST_F(ValueTest, ConstantIntToString) {
    auto* ci = module.getConstantInt(123, intType);

    std::string str = ci->toString();
    EXPECT_FALSE(str.empty());
    // The exact format is up to your implementation
}

TEST_F(ValueTest, ConstantIntClassof) {
    auto* ci = module.getConstantInt(42, intType);
    Value* v = ci;

    EXPECT_TRUE(ConstantInt::classof(v));
    EXPECT_TRUE(Constant::classof(v));
}

// ============================================================================
// ConstantFloat Tests
// ============================================================================

TEST_F(ValueTest, ConstantFloatCreation) {
    auto* cf = module.getConstantFloat(3.14, floatType);

    ASSERT_NE(cf, nullptr);
    EXPECT_DOUBLE_EQ(cf->getValue(), 3.14);
    EXPECT_EQ(cf->getKind(), Value::ValueKind::ConstantFloat);
    EXPECT_EQ(cf->getType(), floatType);
}

TEST_F(ValueTest, ConstantFloatNegative) {
    auto* cf = module.getConstantFloat(-2.5, floatType);

    ASSERT_NE(cf, nullptr);
    EXPECT_DOUBLE_EQ(cf->getValue(), -2.5);
}

TEST_F(ValueTest, ConstantFloatZero) {
    auto* cf = module.getConstantFloat(0.0, floatType);

    ASSERT_NE(cf, nullptr);
    EXPECT_DOUBLE_EQ(cf->getValue(), 0.0);
}

TEST_F(ValueTest, ConstantFloatToString) {
    auto* cf = module.getConstantFloat(1.5, floatType);

    std::string str = cf->toString();
    EXPECT_FALSE(str.empty());
}

TEST_F(ValueTest, ConstantFloatClassof) {
    auto* cf = module.getConstantFloat(3.14, floatType);
    Value* v = cf;

    EXPECT_TRUE(ConstantFloat::classof(v));
    EXPECT_TRUE(Constant::classof(v));
    EXPECT_FALSE(ConstantInt::classof(v));
}

// ============================================================================
// ConstantBool Tests
// ============================================================================

TEST_F(ValueTest, ConstantBoolTrue) {
    auto* cb = module.getConstantBool(true, boolType);

    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->getValue());
    EXPECT_EQ(cb->getKind(), Value::ValueKind::ConstantBool);
    EXPECT_EQ(cb->getType(), boolType);
}

TEST_F(ValueTest, ConstantBoolFalse) {
    auto* cb = module.getConstantBool(false, boolType);

    ASSERT_NE(cb, nullptr);
    EXPECT_FALSE(cb->getValue());
}

TEST_F(ValueTest, ConstantBoolGetTrue) {
    auto* cb = module.getConstantBool(true, boolType);

    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->getValue());
}

TEST_F(ValueTest, ConstantBoolGetFalse) {
    auto* cb = module.getConstantBool(false, boolType);

    ASSERT_NE(cb, nullptr);
    EXPECT_FALSE(cb->getValue());
}

TEST_F(ValueTest, ConstantBoolToString) {
    auto* cbTrue = module.getConstantBool(true, boolType);
    auto* cbFalse = module.getConstantBool(false, boolType);

    std::string strTrue = cbTrue->toString();
    std::string strFalse = cbFalse->toString();

    EXPECT_FALSE(strTrue.empty());
    EXPECT_FALSE(strFalse.empty());
    EXPECT_NE(strTrue, strFalse);
}

TEST_F(ValueTest, ConstantBoolClassof) {
    auto* cb = module.getConstantBool(true, boolType);
    Value* v = cb;

    EXPECT_TRUE(ConstantBool::classof(v));
    EXPECT_TRUE(Constant::classof(v));
    EXPECT_FALSE(ConstantInt::classof(v));
}

// ============================================================================
// ConstantString Tests
// ============================================================================


// ============================================================================
// UndefValue Tests
// ============================================================================

TEST_F(ValueTest, UndefValueCreation) {
    auto* uv = module.getArena().allocate<UndefValue>(intType);

    ASSERT_NE(uv, nullptr);
    EXPECT_EQ(uv->getKind(), Value::ValueKind::UndefValue);
    EXPECT_EQ(uv->getType(), intType);
}

TEST_F(ValueTest, UndefValueToString) {
    auto* uv = module.getArena().allocate<UndefValue>(intType);

    std::string str = uv->toString();
    EXPECT_FALSE(str.empty());
}

TEST_F(ValueTest, UndefValueClassof) {
    auto* uv = module.getArena().allocate<UndefValue>(intType);
    Value* v = uv;

    EXPECT_TRUE(UndefValue::classof(v));
    EXPECT_TRUE(Constant::classof(v));
    EXPECT_FALSE(ConstantInt::classof(v));
}

// ============================================================================
// Argument Tests
// ============================================================================

TEST_F(ValueTest, ArgumentCreation) {
    Argument arg(intType, 0, "x");

    EXPECT_EQ(arg.getKind(), Value::ValueKind::Argument);
    EXPECT_EQ(arg.getType(), intType);
    EXPECT_EQ(arg.getArgNo(), 0);
    EXPECT_EQ(arg.getName(), "x");
}

TEST_F(ValueTest, ArgumentWithoutName) {
    Argument arg(floatType, 2);

    EXPECT_EQ(arg.getArgNo(), 2);
    EXPECT_TRUE(arg.getName().empty());
}

TEST_F(ValueTest, ArgumentToString) {
    Argument arg(intType, 0, "param");

    std::string str = arg.toString();
    EXPECT_FALSE(str.empty());
}

TEST_F(ValueTest, ArgumentClassof) {
    Argument arg(intType, 0);
    Value* v = &arg;

    EXPECT_TRUE(Argument::classof(v));
    EXPECT_FALSE(Constant::classof(v));
}

// ============================================================================
// GlobalVariable Tests
// ============================================================================

TEST_F(ValueTest, GlobalVariableCreation) {
    GlobalVariable gv(intType, "counter", nullptr, false);

    EXPECT_EQ(gv.getKind(), Value::ValueKind::GlobalVariable);
    EXPECT_EQ(gv.getType(), intType);
    EXPECT_EQ(gv.getName(), "counter");
    EXPECT_FALSE(gv.isConstant());
    EXPECT_EQ(gv.getInitializer(), nullptr);
}

TEST_F(ValueTest, GlobalVariableWithInitializer) {
    auto* init = module.getConstantInt(100, intType);
    GlobalVariable gv(intType, "value", init, false);

    EXPECT_EQ(gv.getInitializer(), init);
}

TEST_F(ValueTest, GlobalVariableConstant) {
    auto* init = module.getConstantInt(42, intType);
    GlobalVariable gv(intType, "PI", init, true);

    EXPECT_TRUE(gv.isConstant());
}

TEST_F(ValueTest, GlobalVariableToString) {
    GlobalVariable gv(intType, "global", nullptr, false);

    std::string str = gv.toString();
    EXPECT_FALSE(str.empty());
}

TEST_F(ValueTest, GlobalVariableClassof) {
    GlobalVariable gv(intType, "global", nullptr, false);
    Value* v = &gv;

    EXPECT_TRUE(GlobalVariable::classof(v));
    EXPECT_FALSE(Constant::classof(v));
}

// ============================================================================
// Value Base Class Tests
// ============================================================================

TEST_F(ValueTest, ValueNaming) {
    auto* ci = module.getConstantInt(42, intType);

    EXPECT_TRUE(ci->getName().empty());
    EXPECT_FALSE(ci->hasName());

    ci->setName("myconst");
    EXPECT_EQ(ci->getName(), "myconst");
    EXPECT_TRUE(ci->hasName());
}

TEST_F(ValueTest, ValueUniqueID) {
    auto* ci1 = module.getConstantInt(1, intType);
    auto* ci2 = module.getConstantInt(2, intType);

    EXPECT_NE(ci1->getID(), ci2->getID());
}

TEST_F(ValueTest, ValueTypeChecking) {
    auto* ci = module.getConstantInt(42, intType);

    EXPECT_TRUE(ci->isConstant());
    EXPECT_FALSE(ci->isInstruction());
    EXPECT_FALSE(ci->isArgument());
    EXPECT_FALSE(ci->isGlobalValue());
}

TEST_F(ValueTest, ArgumentTypeChecking) {
    Argument arg(intType, 0);

    EXPECT_FALSE(arg.isConstant());
    EXPECT_FALSE(arg.isInstruction());
    EXPECT_TRUE(arg.isArgument());
    EXPECT_FALSE(arg.isGlobalValue());
}

TEST_F(ValueTest, GlobalVariableTypeChecking) {
    GlobalVariable gv(intType, "g", nullptr, false);

    EXPECT_FALSE(gv.isConstant());
    EXPECT_FALSE(gv.isInstruction());
    EXPECT_FALSE(gv.isArgument());
    EXPECT_TRUE(gv.isGlobalValue());
}

// ============================================================================
// LLVM-style RTTI (dyn_cast, isa) Tests
// ============================================================================

TEST_F(ValueTest, DynCastConstantInt) {
    auto* ci = module.getConstantInt(42, intType);
    Value* v = ci;

    auto* casted = dyn_cast<ConstantInt>(v);
    ASSERT_NE(casted, nullptr);
    EXPECT_EQ(casted->getValue(), 42);
}

TEST_F(ValueTest, DynCastFailure) {
    auto* ci = module.getConstantInt(42, intType);
    Value* v = ci;

    auto* casted = dyn_cast<ConstantFloat>(v);
    EXPECT_EQ(casted, nullptr);
}

TEST_F(ValueTest, IsaConstant) {
    auto* ci = module.getConstantInt(42, intType);
    Value* v = ci;

    EXPECT_TRUE(isa<Constant>(v));
    EXPECT_TRUE(isa<ConstantInt>(v));
    EXPECT_FALSE(isa<ConstantFloat>(v));
}

TEST_F(ValueTest, IsaArgument) {
    Argument arg(intType, 0);
    Value* v = &arg;

    EXPECT_TRUE(isa<Argument>(v));
    EXPECT_FALSE(isa<Constant>(v));
}

// ============================================================================
// Use-Def Chain Tests (Basic - will need Instruction class later)
// ============================================================================

TEST_F(ValueTest, ValueUsesEmpty) {
    auto* ci = module.getConstantInt(42, intType);

    EXPECT_FALSE(ci->hasUses());
    EXPECT_EQ(ci->getNumUses(), 0);
    EXPECT_TRUE(ci->getUses().empty());
}

// Note: Full use-def chain tests will require Instruction class
// These are placeholder tests to ensure the API compiles

TEST_F(ValueTest, ValueUsesAPIExists) {
    auto* ci = module.getConstantInt(42, intType);

    // Just verify the methods exist and are callable
    const auto& uses = ci->getUses();
    EXPECT_EQ(uses.size(), 0);
}

// ============================================================================
// Multiple Constant Types Together
// ============================================================================

TEST_F(ValueTest, MultipleConstantTypes) {
    auto* ci = module.getConstantInt(42, intType);
    auto* cf = module.getConstantFloat(3.14, floatType);
    auto* cb = module.getConstantBool(true, boolType);

    EXPECT_NE(ci->getKind(), cf->getKind());
    EXPECT_NE(ci->getKind(), cb->getKind());

    EXPECT_TRUE(ci->isConstant());
    EXPECT_TRUE(cf->isConstant());
    EXPECT_TRUE(cb->isConstant());
}

TEST_F(ValueTest, ConstantPolymorphism) {
    auto* ci = module.getConstantInt(42, intType);
    auto* cf = module.getConstantFloat(3.14, floatType);

    Constant* c1 = ci;
    Constant* c2 = cf;
    Value* v1 = ci;
    Value* v2 = cf;

    EXPECT_TRUE(Constant::classof(v1));
    EXPECT_TRUE(Constant::classof(v2));

    EXPECT_NE(c1->toString(), c2->toString());
}
