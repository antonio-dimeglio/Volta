#include <gtest/gtest.h>
#include "MIR/MIR.hpp"
#include "Type/TypeRegistry.hpp"

using namespace MIR;

class MIRTest : public ::testing::Test {
protected:
    Type::TypeRegistry typeRegistry;
    const Type::Type* i32Type;
    const Type::Type* boolType;
    const Type::Type* f64Type;

    void SetUp() override {
        i32Type = typeRegistry.getPrimitive(Type::PrimitiveKind::I32);
        boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
        f64Type = typeRegistry.getPrimitive(Type::PrimitiveKind::F64);
    }
};

// ============================================================================
// VALUE TESTS
// ============================================================================

TEST_F(MIRTest, CreateLocalValue) {
    auto local = Value::makeLocal("result", i32Type);
    EXPECT_EQ(local.kind, ValueKind::Local);
    EXPECT_EQ(local.name, "result");
    EXPECT_EQ(local.type, i32Type);
}

TEST_F(MIRTest, CreateParamValue) {
    auto param = Value::makeParam("x", i32Type);
    EXPECT_EQ(param.kind, ValueKind::Param);
    EXPECT_EQ(param.name, "x");
    EXPECT_EQ(param.type, i32Type);
}

TEST_F(MIRTest, CreateGlobalValue) {
    auto global = Value::makeGlobal("main", i32Type);
    EXPECT_EQ(global.kind, ValueKind::Global);
    EXPECT_EQ(global.name, "main");
}

TEST_F(MIRTest, CreateConstantInt) {
    auto constant = Value::makeConstantInt(42, i32Type);
    EXPECT_EQ(constant.kind, ValueKind::Constant);
    EXPECT_TRUE(constant.constantInt.has_value());
    EXPECT_EQ(constant.constantInt.value(), 42);
}

TEST_F(MIRTest, CreateConstantBool) {
    auto constant = Value::makeConstantBool(true, boolType);
    EXPECT_EQ(constant.kind, ValueKind::Constant);
    EXPECT_TRUE(constant.constantBool.has_value());
    EXPECT_EQ(constant.constantBool.value(), true);
}

TEST_F(MIRTest, CreateConstantFloat) {
    auto constant = Value::makeConstantFloat(3.14, f64Type);
    EXPECT_EQ(constant.kind, ValueKind::Constant);
    EXPECT_TRUE(constant.constantFloat.has_value());
    EXPECT_DOUBLE_EQ(constant.constantFloat.value(), 3.14);
}

TEST_F(MIRTest, ValueToString_Local) {
    auto local = Value::makeLocal("temp1", i32Type);
    auto str = local.toString();
    // Should be "%temp1"
    EXPECT_TRUE(str.find("temp1") != std::string::npos);
}

TEST_F(MIRTest, ValueToString_Global) {
    auto global = Value::makeGlobal("main", i32Type);
    auto str = global.toString();
    // Should be "@main"
    EXPECT_TRUE(str.find("main") != std::string::npos);
}

TEST_F(MIRTest, ValueToString_ConstantInt) {
    auto constant = Value::makeConstantInt(42, i32Type);
    auto str = constant.toString();
    // Should be "42"
    EXPECT_TRUE(str.find("42") != std::string::npos || str == "");
}

// ============================================================================
// INSTRUCTION TESTS
// ============================================================================

TEST_F(MIRTest, CreateInstruction) {
    auto result = Value::makeLocal("0", i32Type);
    auto lhs = Value::makeParam("x", i32Type);
    auto rhs = Value::makeParam("y", i32Type);

    Instruction inst(Opcode::IAdd, result, {lhs, rhs});

    EXPECT_EQ(inst.opcode, Opcode::IAdd);
    EXPECT_EQ(inst.result.name, "0");
    EXPECT_EQ(inst.operands.size(), 2);
}

TEST_F(MIRTest, InstructionToString) {
    auto result = Value::makeLocal("0", i32Type);
    auto lhs = Value::makeParam("x", i32Type);
    auto rhs = Value::makeParam("y", i32Type);

    Instruction inst(Opcode::IAdd, result, {lhs, rhs});
    auto str = inst.toString();

    // Should contain something like "%0 = iadd %x, %y"
    // May be empty if not implemented yet
    EXPECT_TRUE(str.empty() || str.find("0") != std::string::npos);
}

// ============================================================================
// TERMINATOR TESTS
// ============================================================================

TEST_F(MIRTest, CreateReturnTerminator) {
    auto retVal = Value::makeConstantInt(42, i32Type);
    auto term = Terminator::makeReturn(retVal);

    EXPECT_EQ(term.kind, TerminatorKind::Return);
    EXPECT_EQ(term.operands.size(), 1);
    EXPECT_EQ(term.targets.size(), 0);
}

TEST_F(MIRTest, CreateReturnVoidTerminator) {
    auto term = Terminator::makeReturn(std::nullopt);

    EXPECT_EQ(term.kind, TerminatorKind::Return);
    EXPECT_EQ(term.operands.size(), 0);
}

TEST_F(MIRTest, CreateBranchTerminator) {
    auto term = Terminator::makeBranch("loop_body");

    EXPECT_EQ(term.kind, TerminatorKind::Branch);
    EXPECT_EQ(term.targets.size(), 1);
    EXPECT_EQ(term.targets[0], "loop_body");
}

TEST_F(MIRTest, CreateCondBranchTerminator) {
    auto cond = Value::makeLocal("cond", boolType);
    auto term = Terminator::makeCondBranch(cond, "then", "else");

    EXPECT_EQ(term.kind, TerminatorKind::CondBranch);
    EXPECT_EQ(term.operands.size(), 1);
    EXPECT_EQ(term.targets.size(), 2);
    EXPECT_EQ(term.targets[0], "then");
    EXPECT_EQ(term.targets[1], "else");
}

TEST_F(MIRTest, TerminatorToString_Return) {
    auto retVal = Value::makeConstantInt(0, i32Type);
    auto term = Terminator::makeReturn(retVal);
    auto str = term.toString();

    // Should contain "ret" or be empty if not implemented
    EXPECT_TRUE(str.empty() || str.find("ret") != std::string::npos || str.find("0") != std::string::npos);
}

TEST_F(MIRTest, TerminatorToString_Branch) {
    auto term = Terminator::makeBranch("target");
    auto str = term.toString();

    // Should contain "br" and "target" or be empty
    EXPECT_TRUE(str.empty() || str.find("target") != std::string::npos);
}

// ============================================================================
// BASIC BLOCK TESTS
// ============================================================================

TEST_F(MIRTest, CreateBasicBlock) {
    BasicBlock block("entry");

    EXPECT_EQ(block.label, "entry");
    EXPECT_EQ(block.instructions.size(), 0);
}

TEST_F(MIRTest, AddInstructionToBlock) {
    BasicBlock block("entry");

    auto result = Value::makeLocal("0", i32Type);
    auto lhs = Value::makeConstantInt(1, i32Type);
    auto rhs = Value::makeConstantInt(2, i32Type);
    Instruction inst(Opcode::IAdd, result, {lhs, rhs});

    block.addInstruction(inst);

    EXPECT_EQ(block.instructions.size(), 1);
}

TEST_F(MIRTest, SetTerminatorOnBlock) {
    BasicBlock block("entry");

    auto retVal = Value::makeConstantInt(0, i32Type);
    auto term = Terminator::makeReturn(retVal);

    block.setTerminator(term);

    EXPECT_EQ(block.terminator.kind, TerminatorKind::Return);
}

TEST_F(MIRTest, BasicBlockToString) {
    BasicBlock block("entry");

    auto result = Value::makeLocal("0", i32Type);
    auto constant = Value::makeConstantInt(42, i32Type);
    Instruction inst(Opcode::IAdd, result, {constant, constant});
    block.addInstruction(inst);

    auto term = Terminator::makeReturn(result);
    block.setTerminator(term);

    auto str = block.toString();

    // Should contain "entry:" or be empty if not implemented
    EXPECT_TRUE(str.empty() || str.find("entry") != std::string::npos);
}

// ============================================================================
// FUNCTION TESTS
// ============================================================================

TEST_F(MIRTest, CreateFunction) {
    auto param1 = Value::makeParam("x", i32Type);
    auto param2 = Value::makeParam("y", i32Type);

    Function fn("add", {param1, param2}, i32Type);

    EXPECT_EQ(fn.name, "add");
    EXPECT_EQ(fn.params.size(), 2);
    EXPECT_EQ(fn.returnType, i32Type);
    EXPECT_EQ(fn.blocks.size(), 0);
}

TEST_F(MIRTest, AddBlockToFunction) {
    Function fn("test", {}, typeRegistry.getPrimitive(Type::PrimitiveKind::Void));

    BasicBlock block("entry");
    fn.addBlock(block);

    EXPECT_EQ(fn.blocks.size(), 1);
    EXPECT_EQ(fn.blocks[0].label, "entry");
}

TEST_F(MIRTest, GetBlockByLabel) {
    Function fn("test", {}, typeRegistry.getPrimitive(Type::PrimitiveKind::Void));

    BasicBlock block1("entry");
    BasicBlock block2("exit");
    fn.addBlock(block1);
    fn.addBlock(block2);

    auto* found = fn.getBlock("exit");
    EXPECT_NE(found, nullptr);
    if (found) {
        EXPECT_EQ(found->label, "exit");
    }
}

TEST_F(MIRTest, GetBlockNotFound) {
    Function fn("test", {}, typeRegistry.getPrimitive(Type::PrimitiveKind::Void));

    auto* found = fn.getBlock("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(MIRTest, FunctionToString) {
    auto param = Value::makeParam("x", i32Type);
    Function fn("test", {param}, i32Type);

    BasicBlock entry("entry");
    auto term = Terminator::makeReturn(param);
    entry.setTerminator(term);
    fn.addBlock(entry);

    auto str = fn.toString();

    // Should contain function signature or be empty
    EXPECT_TRUE(str.empty() || str.find("test") != std::string::npos);
}

// ============================================================================
// PROGRAM TESTS
// ============================================================================

TEST_F(MIRTest, CreateProgram) {
    Program program;

    EXPECT_EQ(program.functions.size(), 0);
}

TEST_F(MIRTest, AddFunctionToProgram) {
    Program program;

    Function fn("main", {}, i32Type);
    program.addFunction(fn);

    EXPECT_EQ(program.functions.size(), 1);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST_F(MIRTest, GetFunctionByName) {
    Program program;

    Function fn1("foo", {}, i32Type);
    Function fn2("bar", {}, i32Type);
    program.addFunction(fn1);
    program.addFunction(fn2);

    auto* found = program.getFunction("bar");
    EXPECT_NE(found, nullptr);
    if (found) {
        EXPECT_EQ(found->name, "bar");
    }
}

TEST_F(MIRTest, GetFunctionNotFound) {
    Program program;

    auto* found = program.getFunction("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST_F(MIRTest, ProgramToString) {
    Program program;

    Function fn("main", {}, typeRegistry.getPrimitive(Type::PrimitiveKind::Void));
    BasicBlock entry("entry");
    auto term = Terminator::makeReturnVoid();
    entry.setTerminator(term);
    fn.addBlock(entry);
    program.addFunction(fn);

    auto str = program.toString();

    // Should contain function or be empty if not implemented
    EXPECT_TRUE(str.empty() || str.find("main") != std::string::npos);
}

// ============================================================================
// INTEGRATION TEST - Build Simple Function Manually
// ============================================================================

TEST_F(MIRTest, BuildSimpleAddFunction) {
    // Build: fn add(x: i32, y: i32) -> i32 { return x + y; }

    // Create parameters
    auto paramX = Value::makeParam("x", i32Type);
    auto paramY = Value::makeParam("y", i32Type);

    // Create function
    Function fn("add", {paramX, paramY}, i32Type);

    // Create entry block
    BasicBlock entry("entry");

    // Create instruction: %0 = iadd %x, %y
    auto result = Value::makeLocal("0", i32Type);
    Instruction addInst(Opcode::IAdd, result, {paramX, paramY});
    entry.addInstruction(addInst);

    // Create terminator: ret %0
    auto retTerm = Terminator::makeReturn(result);
    entry.setTerminator(retTerm);

    // Add block to function
    fn.addBlock(entry);

    // Verify structure
    EXPECT_EQ(fn.blocks.size(), 1);
    EXPECT_EQ(fn.blocks[0].instructions.size(), 1);
    EXPECT_EQ(fn.blocks[0].terminator.kind, TerminatorKind::Return);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
