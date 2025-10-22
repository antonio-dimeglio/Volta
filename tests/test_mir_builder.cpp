#include <gtest/gtest.h>
#include "MIR/MIRBuilder.hpp"
#include "Type/TypeRegistry.hpp"

using namespace MIR;

class MIRBuilderTest : public ::testing::Test {
protected:
    Type::TypeRegistry typeRegistry;
    const Type::Type* i32Type;
    const Type::Type* boolType;
    const Type::Type* voidType;
    const Type::Type* f64Type;

    void SetUp() override {
        i32Type = typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
        boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
        voidType = typeRegistry.getPrimitive(Type::PrimitiveKind::Void);
        f64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::F64);
    }
};

// ============================================================================
// FUNCTION MANAGEMENT
// ============================================================================

TEST_F(MIRBuilderTest, CreateFunctionWithNoParams) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, voidType);
    builder.createBlock("entry");
    builder.createReturnVoid();
    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions.size(), 1);
    EXPECT_EQ(program.functions[0].name, "test");
    EXPECT_EQ(program.functions[0].params.size(), 0);
    EXPECT_EQ(program.functions[0].returnType, voidType);
}

TEST_F(MIRBuilderTest, CreateFunctionWithParams) {
    MIRBuilder builder(typeRegistry);

    auto param1 = Value::makeParam("x", i32Type);
    auto param2 = Value::makeParam("y", i32Type);

    builder.createFunction("add", {param1, param2}, i32Type);
    builder.createBlock("entry");
    builder.createReturnVoid();  // Placeholder
    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions[0].params.size(), 2);
    EXPECT_EQ(program.functions[0].params[0].name, "x");
    EXPECT_EQ(program.functions[0].params[1].name, "y");
}

// ============================================================================
// BASIC BLOCK MANAGEMENT
// ============================================================================

TEST_F(MIRBuilderTest, CreateMultipleBlocks) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, voidType);
    builder.createBlock("entry");
    builder.createBlock("loop_body");
    builder.createBlock("exit");
    builder.createReturnVoid();
    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions[0].blocks.size(), 3);
    EXPECT_EQ(program.functions[0].blocks[0].label, "entry");
    EXPECT_EQ(program.functions[0].blocks[1].label, "loop_body");
    EXPECT_EQ(program.functions[0].blocks[2].label, "exit");
}

// ============================================================================
// VALUE CREATION
// ============================================================================

TEST_F(MIRBuilderTest, CreateTemporary) {
    MIRBuilder builder(typeRegistry);

    auto temp1 = builder.createTemporary(i32Type);
    auto temp2 = builder.createTemporary(i32Type);

    // Temporaries should have unique names (e.g., "0", "1", "2"...)
    EXPECT_NE(temp1.name, temp2.name);
    EXPECT_EQ(temp1.type, i32Type);
    EXPECT_EQ(temp2.type, i32Type);
}

TEST_F(MIRBuilderTest, CreateConstants) {
    MIRBuilder builder(typeRegistry);

    auto intConst = builder.createConstantInt(42, i32Type);
    auto boolConst = builder.createConstantBool(true, boolType);
    auto floatConst = builder.createConstantFloat(3.14, f64Type);

    EXPECT_EQ(intConst.kind, ValueKind::Constant);
    EXPECT_TRUE(intConst.constantInt.has_value());
    EXPECT_EQ(intConst.constantInt.value(), 42);

    EXPECT_TRUE(boolConst.constantBool.has_value());
    EXPECT_EQ(boolConst.constantBool.value(), true);

    EXPECT_TRUE(floatConst.constantFloat.has_value());
    EXPECT_DOUBLE_EQ(floatConst.constantFloat.value(), 3.14);
}

// ============================================================================
// ARITHMETIC INSTRUCTIONS
// ============================================================================

TEST_F(MIRBuilderTest, CreateArithmeticInstructions) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    auto val1 = builder.createConstantInt(10, i32Type);
    auto val2 = builder.createConstantInt(20, i32Type);

    auto add = builder.createIAdd(val1, val2);
    auto sub = builder.createISub(val1, val2);
    auto mul = builder.createIMul(val1, val2);
    auto div = builder.createIDiv(val1, val2);

    // Should create instructions with unique result values
    EXPECT_NE(add.name, "");
    EXPECT_NE(sub.name, "");
    EXPECT_NE(mul.name, "");
    EXPECT_NE(div.name, "");

    builder.createReturn(add);
    builder.finishFunction();

    auto program = builder.getProgram();
    EXPECT_EQ(program.functions[0].blocks[0].instructions.size(), 4);
}

// ============================================================================
// COMPARISON INSTRUCTIONS
// ============================================================================

TEST_F(MIRBuilderTest, CreateComparisonInstructions) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, boolType);
    builder.createBlock("entry");

    auto val1 = builder.createConstantInt(10, i32Type);
    auto val2 = builder.createConstantInt(20, i32Type);

    auto eq = builder.createICmpEQ(val1, val2);
    auto ne = builder.createICmpNE(val1, val2);
    auto lt = builder.createICmpLT(val1, val2);
    auto le = builder.createICmpLE(val1, val2);

    EXPECT_EQ(eq.type, boolType);
    EXPECT_EQ(ne.type, boolType);
    EXPECT_EQ(lt.type, boolType);
    EXPECT_EQ(le.type, boolType);

    builder.createReturn(eq);
    builder.finishFunction();

    auto program = builder.getProgram();
    EXPECT_EQ(program.functions[0].blocks[0].instructions.size(), 4);
}

// ============================================================================
// MEMORY INSTRUCTIONS
// ============================================================================

TEST_F(MIRBuilderTest, CreateAllocaLoadStore) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    // Allocate stack space for i32
    auto ptr = builder.createAlloca(i32Type);

    // Store a value
    auto val = builder.createConstantInt(42, i32Type);
    builder.createStore(val, ptr);

    // Load the value back
    auto loaded = builder.createLoad(ptr);

    builder.createReturn(loaded);
    builder.finishFunction();

    auto program = builder.getProgram();

    // Should have: alloca, store, load
    EXPECT_GE(program.functions[0].blocks[0].instructions.size(), 3);
}

// ============================================================================
// TERMINATORS
// ============================================================================

TEST_F(MIRBuilderTest, CreateReturnVoid) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, voidType);
    builder.createBlock("entry");
    builder.createReturnVoid();
    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions[0].blocks[0].terminator.kind, TerminatorKind::Return);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.operands.size(), 0);
}

TEST_F(MIRBuilderTest, CreateReturnValue) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    auto val = builder.createConstantInt(42, i32Type);
    builder.createReturn(val);
    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions[0].blocks[0].terminator.kind, TerminatorKind::Return);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.operands.size(), 1);
}

TEST_F(MIRBuilderTest, CreateBranch) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, voidType);
    builder.createBlock("entry");
    builder.createBlock("target");

    builder.setInsertionPoint("entry");
    builder.createBranch("target");

    builder.setInsertionPoint("target");
    builder.createReturnVoid();

    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions[0].blocks[0].terminator.kind, TerminatorKind::Branch);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.targets.size(), 1);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.targets[0], "target");
}

TEST_F(MIRBuilderTest, CreateCondBranch) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, voidType);
    builder.createBlock("entry");
    builder.createBlock("then");
    builder.createBlock("else");

    builder.setInsertionPoint("entry");
    auto cond = builder.createConstantBool(true, boolType);
    builder.createCondBranch(cond, "then", "else");

    builder.setInsertionPoint("then");
    builder.createReturnVoid();

    builder.setInsertionPoint("else");
    builder.createReturnVoid();

    builder.finishFunction();

    auto program = builder.getProgram();

    EXPECT_EQ(program.functions[0].blocks[0].terminator.kind, TerminatorKind::CondBranch);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.operands.size(), 1);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.targets.size(), 2);
}

// ============================================================================
// SYMBOL TABLE
// ============================================================================

TEST_F(MIRBuilderTest, VariableTracking) {
    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    auto val = builder.createConstantInt(42, i32Type);
    builder.setVariable("x", val);

    EXPECT_TRUE(builder.hasVariable("x"));
    EXPECT_FALSE(builder.hasVariable("y"));

    auto retrieved = builder.getVariable("x");
    EXPECT_EQ(retrieved.name, val.name);

    builder.createReturnVoid();
    builder.finishFunction();
}

// ============================================================================
// INTEGRATION TEST - Build Complete Function
// ============================================================================

TEST_F(MIRBuilderTest, BuildAddFunction) {
    // Build: fn add(x: i32, y: i32) -> i32 { return x + y; }

    MIRBuilder builder(typeRegistry);

    auto paramX = Value::makeParam("x", i32Type);
    auto paramY = Value::makeParam("y", i32Type);

    builder.createFunction("add", {paramX, paramY}, i32Type);
    builder.createBlock("entry");

    // Add x and y
    auto sum = builder.createIAdd(paramX, paramY);

    // Return the sum
    builder.createReturn(sum);

    builder.finishFunction();

    auto program = builder.getProgram();

    // Verify structure
    EXPECT_EQ(program.functions.size(), 1);
    EXPECT_EQ(program.functions[0].name, "add");
    EXPECT_EQ(program.functions[0].params.size(), 2);
    EXPECT_EQ(program.functions[0].blocks.size(), 1);
    EXPECT_EQ(program.functions[0].blocks[0].instructions.size(), 1);
    EXPECT_EQ(program.functions[0].blocks[0].instructions[0].opcode, Opcode::IAdd);
    EXPECT_EQ(program.functions[0].blocks[0].terminator.kind, TerminatorKind::Return);
}

TEST_F(MIRBuilderTest, BuildFunctionWithLocalVariables) {
    // Build: fn test() -> i32 {
    //   let x: i32 = alloca
    //   store 10, x
    //   let y: i32 = alloca
    //   store 20, y
    //   let sum = load x + load y
    //   return sum
    // }

    MIRBuilder builder(typeRegistry);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    // Allocate and initialize x = 10
    auto xPtr = builder.createAlloca(i32Type);
    auto const10 = builder.createConstantInt(10, i32Type);
    builder.createStore(const10, xPtr);

    // Allocate and initialize y = 20
    auto yPtr = builder.createAlloca(i32Type);
    auto const20 = builder.createConstantInt(20, i32Type);
    builder.createStore(const20, yPtr);

    // Load x and y
    auto xVal = builder.createLoad(xPtr);
    auto yVal = builder.createLoad(yPtr);

    // Add them
    auto sum = builder.createIAdd(xVal, yVal);

    // Return
    builder.createReturn(sum);

    builder.finishFunction();

    auto program = builder.getProgram();

    // Should have: 2 allocas, 2 stores, 2 loads, 1 add
    EXPECT_EQ(program.functions[0].blocks[0].instructions.size(), 7);
}

TEST_F(MIRBuilderTest, BuildFunctionWithControlFlow) {
    // Build: fn test(cond: bool) -> i32 {
    //   if (cond) {
    //     return 1;
    //   } else {
    //     return 0;
    //   }
    // }

    MIRBuilder builder(typeRegistry);

    auto condParam = Value::makeParam("cond", boolType);

    builder.createFunction("test", {condParam}, i32Type);

    builder.createBlock("entry");
    builder.createBlock("then");
    builder.createBlock("else");

    // Entry: branch based on condition
    builder.setInsertionPoint("entry");
    builder.createCondBranch(condParam, "then", "else");

    // Then block: return 1
    builder.setInsertionPoint("then");
    auto const1 = builder.createConstantInt(1, i32Type);
    builder.createReturn(const1);

    // Else block: return 0
    builder.setInsertionPoint("else");
    auto const0 = builder.createConstantInt(0, i32Type);
    builder.createReturn(const0);

    builder.finishFunction();

    auto program = builder.getProgram();

    // Should have 3 blocks
    EXPECT_EQ(program.functions[0].blocks.size(), 3);
    // All blocks should have terminators
    for (const auto& block : program.functions[0].blocks) {
        EXPECT_TRUE(block.terminator.kind != TerminatorKind::Unreachable || !block.terminator.targets.empty() || !block.terminator.operands.empty());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
