#include <gtest/gtest.h>
#include "../../include/MIR/MIR.hpp"
#include "../../include/MIR/MIRBuilder.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include <memory>

using namespace MIR;
using namespace Type;

// ============================================================================
// MIR Value Tests
// ============================================================================

class MIRValueTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRValueTest, LocalValue) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    Value local = Value::makeLocal("result", i32Type);

    EXPECT_EQ(local.kind, ValueKind::Local);
    EXPECT_EQ(local.name, "result");
    EXPECT_EQ(local.type, i32Type);
    EXPECT_FALSE(local.isConstant());
    EXPECT_TRUE(local.isLocal());
}

TEST_F(MIRValueTest, ParamValue) {
    auto* i64Type = types.getPrimitive(PrimitiveKind::I64);
    Value param = Value::makeParam("arg0", i64Type);

    EXPECT_EQ(param.kind, ValueKind::Param);
    EXPECT_EQ(param.name, "arg0");
    EXPECT_EQ(param.type, i64Type);
}

TEST_F(MIRValueTest, GlobalValue) {
    auto* voidType = types.getPrimitive(PrimitiveKind::Void);
    Value global = Value::makeGlobal("main", voidType);

    EXPECT_EQ(global.kind, ValueKind::Global);
    EXPECT_EQ(global.name, "main");
}

TEST_F(MIRValueTest, ConstantInt) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    Value constant = Value::makeConstantInt(42, i32Type);

    EXPECT_EQ(constant.kind, ValueKind::Constant);
    EXPECT_TRUE(constant.isConstant());
    ASSERT_TRUE(constant.constantInt.has_value());
    EXPECT_EQ(constant.constantInt.value(), 42);
}

TEST_F(MIRValueTest, ConstantIntNegative) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    Value constant = Value::makeConstantInt(-100, i32Type);

    ASSERT_TRUE(constant.constantInt.has_value());
    EXPECT_EQ(constant.constantInt.value(), -100);
}

TEST_F(MIRValueTest, ConstantBool) {
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);
    Value constantTrue = Value::makeConstantBool(true, boolType);
    Value constantFalse = Value::makeConstantBool(false, boolType);

    ASSERT_TRUE(constantTrue.constantBool.has_value());
    EXPECT_TRUE(constantTrue.constantBool.value());

    ASSERT_TRUE(constantFalse.constantBool.has_value());
    EXPECT_FALSE(constantFalse.constantBool.value());
}

TEST_F(MIRValueTest, ConstantFloat) {
    auto* f64Type = types.getPrimitive(PrimitiveKind::F64);
    Value constant = Value::makeConstantFloat(3.14159, f64Type);

    ASSERT_TRUE(constant.constantFloat.has_value());
    EXPECT_DOUBLE_EQ(constant.constantFloat.value(), 3.14159);
}

TEST_F(MIRValueTest, ConstantString) {
    auto* i8Type = types.getPrimitive(PrimitiveKind::I8);
    auto* strPtrType = types.getPointer(i8Type);
    Value constant = Value::makeConstantString("hello", strPtrType);

    ASSERT_TRUE(constant.constantString.has_value());
    EXPECT_EQ(constant.constantString.value(), "hello");
}

TEST_F(MIRValueTest, ValueToString) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Value local = Value::makeLocal("temp1", i32Type);
    EXPECT_EQ(local.toString(), "%temp1");

    Value param = Value::makeParam("arg0", i32Type);
    EXPECT_EQ(param.toString(), "%arg0");

    Value global = Value::makeGlobal("main", i32Type);
    EXPECT_EQ(global.toString(), "@main");

    Value constantInt = Value::makeConstantInt(42, i32Type);
    EXPECT_EQ(constantInt.toString(), "42");
}

// ============================================================================
// MIR Instruction Tests
// ============================================================================

class MIRInstructionTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRInstructionTest, IntegerAdd) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Value result = Value::makeLocal("0", i32Type);
    Value a = Value::makeLocal("a", i32Type);
    Value b = Value::makeLocal("b", i32Type);

    Instruction inst(Opcode::IAdd, result, {a, b});

    EXPECT_EQ(inst.opcode, Opcode::IAdd);
    EXPECT_EQ(inst.result.name, "0");
    EXPECT_EQ(inst.operands.size(), 2);
    EXPECT_EQ(inst.operands[0].name, "a");
    EXPECT_EQ(inst.operands[1].name, "b");
}

TEST_F(MIRInstructionTest, IntegerSubtraction) {
    auto* i64Type = types.getPrimitive(PrimitiveKind::I64);

    Value result = Value::makeLocal("1", i64Type);
    Value x = Value::makeLocal("x", i64Type);
    Value y = Value::makeLocal("y", i64Type);

    Instruction inst(Opcode::ISub, result, {x, y});

    EXPECT_EQ(inst.opcode, Opcode::ISub);
}

TEST_F(MIRInstructionTest, IntegerMultiplication) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Value result = Value::makeLocal("2", i32Type);
    Value a = Value::makeConstantInt(5, i32Type);
    Value b = Value::makeConstantInt(10, i32Type);

    Instruction inst(Opcode::IMul, result, {a, b});

    EXPECT_EQ(inst.opcode, Opcode::IMul);
    EXPECT_TRUE(inst.operands[0].isConstant());
    EXPECT_TRUE(inst.operands[1].isConstant());
}

TEST_F(MIRInstructionTest, FloatArithmetic) {
    auto* f32Type = types.getPrimitive(PrimitiveKind::F32);

    Value result = Value::makeLocal("f", f32Type);
    Value a = Value::makeLocal("a", f32Type);
    Value b = Value::makeLocal("b", f32Type);

    Instruction fadd(Opcode::FAdd, result, {a, b});
    Instruction fsub(Opcode::FSub, result, {a, b});
    Instruction fmul(Opcode::FMul, result, {a, b});
    Instruction fdiv(Opcode::FDiv, result, {a, b});

    EXPECT_EQ(fadd.opcode, Opcode::FAdd);
    EXPECT_EQ(fsub.opcode, Opcode::FSub);
    EXPECT_EQ(fmul.opcode, Opcode::FMul);
    EXPECT_EQ(fdiv.opcode, Opcode::FDiv);
}

TEST_F(MIRInstructionTest, IntegerComparison) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);

    Value result = Value::makeLocal("cmp", boolType);
    Value a = Value::makeLocal("a", i32Type);
    Value b = Value::makeLocal("b", i32Type);

    Instruction eq(Opcode::ICmpEQ, result, {a, b});
    Instruction ne(Opcode::ICmpNE, result, {a, b});
    Instruction lt(Opcode::ICmpLT, result, {a, b});
    Instruction le(Opcode::ICmpLE, result, {a, b});
    Instruction gt(Opcode::ICmpGT, result, {a, b});
    Instruction ge(Opcode::ICmpGE, result, {a, b});

    EXPECT_EQ(eq.opcode, Opcode::ICmpEQ);
    EXPECT_EQ(ne.opcode, Opcode::ICmpNE);
    EXPECT_EQ(lt.opcode, Opcode::ICmpLT);
    EXPECT_EQ(le.opcode, Opcode::ICmpLE);
    EXPECT_EQ(gt.opcode, Opcode::ICmpGT);
    EXPECT_EQ(ge.opcode, Opcode::ICmpGE);
}

TEST_F(MIRInstructionTest, UnsignedComparison) {
    auto* u32Type = types.getPrimitive(PrimitiveKind::U32);
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);

    Value result = Value::makeLocal("cmp", boolType);
    Value a = Value::makeLocal("a", u32Type);
    Value b = Value::makeLocal("b", u32Type);

    Instruction ult(Opcode::ICmpULT, result, {a, b});
    Instruction ule(Opcode::ICmpULE, result, {a, b});
    Instruction ugt(Opcode::ICmpUGT, result, {a, b});
    Instruction uge(Opcode::ICmpUGE, result, {a, b});

    EXPECT_EQ(ult.opcode, Opcode::ICmpULT);
    EXPECT_EQ(ule.opcode, Opcode::ICmpULE);
    EXPECT_EQ(ugt.opcode, Opcode::ICmpUGT);
    EXPECT_EQ(uge.opcode, Opcode::ICmpUGE);
}

TEST_F(MIRInstructionTest, FloatComparison) {
    auto* f64Type = types.getPrimitive(PrimitiveKind::F64);
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);

    Value result = Value::makeLocal("fcmp", boolType);
    Value a = Value::makeLocal("a", f64Type);
    Value b = Value::makeLocal("b", f64Type);

    Instruction feq(Opcode::FCmpEQ, result, {a, b});
    Instruction fne(Opcode::FCmpNE, result, {a, b});
    Instruction flt(Opcode::FCmpLT, result, {a, b});
    Instruction fle(Opcode::FCmpLE, result, {a, b});
    Instruction fgt(Opcode::FCmpGT, result, {a, b});
    Instruction fge(Opcode::FCmpGE, result, {a, b});

    EXPECT_EQ(feq.opcode, Opcode::FCmpEQ);
    EXPECT_EQ(fne.opcode, Opcode::FCmpNE);
    EXPECT_EQ(flt.opcode, Opcode::FCmpLT);
    EXPECT_EQ(fle.opcode, Opcode::FCmpLE);
    EXPECT_EQ(fgt.opcode, Opcode::FCmpGT);
    EXPECT_EQ(fge.opcode, Opcode::FCmpGE);
}

TEST_F(MIRInstructionTest, LogicalOperations) {
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);

    Value result = Value::makeLocal("r", boolType);
    Value a = Value::makeLocal("a", boolType);
    Value b = Value::makeLocal("b", boolType);

    Instruction andInst(Opcode::And, result, {a, b});
    Instruction orInst(Opcode::Or, result, {a, b});
    Instruction notInst(Opcode::Not, result, {a});

    EXPECT_EQ(andInst.opcode, Opcode::And);
    EXPECT_EQ(andInst.operands.size(), 2);

    EXPECT_EQ(orInst.opcode, Opcode::Or);
    EXPECT_EQ(orInst.operands.size(), 2);

    EXPECT_EQ(notInst.opcode, Opcode::Not);
    EXPECT_EQ(notInst.operands.size(), 1);
}

TEST_F(MIRInstructionTest, MemoryAlloca) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* ptrType = types.getPointer(i32Type);

    Value result = Value::makeLocal("ptr", ptrType);

    Instruction allocaInst(Opcode::Alloca, result, {});

    EXPECT_EQ(allocaInst.opcode, Opcode::Alloca);
    EXPECT_EQ(allocaInst.operands.size(), 0);
}

TEST_F(MIRInstructionTest, MemoryLoad) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* ptrType = types.getPointer(i32Type);

    Value result = Value::makeLocal("value", i32Type);
    Value ptr = Value::makeLocal("ptr", ptrType);

    Instruction load(Opcode::Load, result, {ptr});

    EXPECT_EQ(load.opcode, Opcode::Load);
    EXPECT_EQ(load.operands.size(), 1);
}

TEST_F(MIRInstructionTest, MemoryStore) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* ptrType = types.getPointer(i32Type);

    Value value = Value::makeConstantInt(42, i32Type);
    Value ptr = Value::makeLocal("ptr", ptrType);

    // Store doesn't have a result value (it's void)
    Instruction store(Opcode::Store, Value(), {value, ptr});

    EXPECT_EQ(store.opcode, Opcode::Store);
    EXPECT_EQ(store.operands.size(), 2);
}

TEST_F(MIRInstructionTest, TypeConversionSExt) {
    auto* i8Type = types.getPrimitive(PrimitiveKind::I8);
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Value result = Value::makeLocal("extended", i32Type);
    Value source = Value::makeLocal("byte", i8Type);

    Instruction sext(Opcode::SExt, result, {source});

    EXPECT_EQ(sext.opcode, Opcode::SExt);
}

TEST_F(MIRInstructionTest, TypeConversionZExt) {
    auto* u8Type = types.getPrimitive(PrimitiveKind::U8);
    auto* u32Type = types.getPrimitive(PrimitiveKind::U32);

    Value result = Value::makeLocal("extended", u32Type);
    Value source = Value::makeLocal("byte", u8Type);

    Instruction zext(Opcode::ZExt, result, {source});

    EXPECT_EQ(zext.opcode, Opcode::ZExt);
}

TEST_F(MIRInstructionTest, TypeConversionTrunc) {
    auto* i64Type = types.getPrimitive(PrimitiveKind::I64);
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Value result = Value::makeLocal("truncated", i32Type);
    Value source = Value::makeLocal("wide", i64Type);

    Instruction trunc(Opcode::Trunc, result, {source});

    EXPECT_EQ(trunc.opcode, Opcode::Trunc);
}

TEST_F(MIRInstructionTest, FloatExtendTruncate) {
    auto* f32Type = types.getPrimitive(PrimitiveKind::F32);
    auto* f64Type = types.getPrimitive(PrimitiveKind::F64);

    Value f32Val = Value::makeLocal("f32", f32Type);
    Value f64Val = Value::makeLocal("f64", f64Type);

    Instruction fpext(Opcode::FPExt, f64Val, {f32Val});
    Instruction fptrunc(Opcode::FPTrunc, f32Val, {f64Val});

    EXPECT_EQ(fpext.opcode, Opcode::FPExt);
    EXPECT_EQ(fptrunc.opcode, Opcode::FPTrunc);
}

TEST_F(MIRInstructionTest, IntFloatConversion) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* u32Type = types.getPrimitive(PrimitiveKind::U32);
    auto* f32Type = types.getPrimitive(PrimitiveKind::F32);

    Value intVal = Value::makeLocal("i", i32Type);
    Value uintVal = Value::makeLocal("u", u32Type);
    Value floatVal = Value::makeLocal("f", f32Type);

    Instruction sitofp(Opcode::SIToFP, floatVal, {intVal});
    Instruction uitofp(Opcode::UIToFP, floatVal, {uintVal});
    Instruction fptosi(Opcode::FPToSI, intVal, {floatVal});
    Instruction fptoui(Opcode::FPToUI, uintVal, {floatVal});

    EXPECT_EQ(sitofp.opcode, Opcode::SIToFP);
    EXPECT_EQ(uitofp.opcode, Opcode::UIToFP);
    EXPECT_EQ(fptosi.opcode, Opcode::FPToSI);
    EXPECT_EQ(fptoui.opcode, Opcode::FPToUI);
}

TEST_F(MIRInstructionTest, GetElementPtr) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* arrayType = types.getArray(i32Type, {10});
    auto* ptrType = types.getPointer(arrayType);
    auto* elemPtrType = types.getPointer(i32Type);

    Value result = Value::makeLocal("elemPtr", elemPtrType);
    Value arrayPtr = Value::makeLocal("arr", ptrType);
    Value index = Value::makeConstantInt(5, i32Type);

    Instruction gep(Opcode::GetElementPtr, result, {arrayPtr, index});

    EXPECT_EQ(gep.opcode, Opcode::GetElementPtr);
    EXPECT_EQ(gep.operands.size(), 2);
}

TEST_F(MIRInstructionTest, Call) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Value result = Value::makeLocal("ret", i32Type);
    Value arg1 = Value::makeLocal("a", i32Type);
    Value arg2 = Value::makeLocal("b", i32Type);

    Instruction call(Opcode::Call, result, {arg1, arg2});
    call.callTarget = "add";

    EXPECT_EQ(call.opcode, Opcode::Call);
    EXPECT_TRUE(call.callTarget.has_value());
    EXPECT_EQ(call.callTarget.value(), "add");
    EXPECT_EQ(call.operands.size(), 2);
}

// ============================================================================
// MIR Terminator Tests
// ============================================================================

class MIRTerminatorTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRTerminatorTest, ReturnWithValue) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    Value retVal = Value::makeConstantInt(42, i32Type);

    Terminator ret = Terminator::makeReturn(retVal);

    EXPECT_EQ(ret.kind, TerminatorKind::Return);
    EXPECT_EQ(ret.operands.size(), 1);
    EXPECT_EQ(ret.operands[0].constantInt.value(), 42);
}

TEST_F(MIRTerminatorTest, ReturnVoid) {
    Terminator ret = Terminator::makeReturnVoid();

    EXPECT_EQ(ret.kind, TerminatorKind::Return);
    EXPECT_EQ(ret.operands.size(), 0);
}

TEST_F(MIRTerminatorTest, UnconditionalBranch) {
    Terminator br = Terminator::makeBranch("loop_body");

    EXPECT_EQ(br.kind, TerminatorKind::Branch);
    EXPECT_EQ(br.targets.size(), 1);
    EXPECT_EQ(br.targets[0], "loop_body");
}

TEST_F(MIRTerminatorTest, ConditionalBranch) {
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);
    Value condition = Value::makeLocal("cond", boolType);

    Terminator condbr = Terminator::makeCondBranch(condition, "then_block", "else_block");

    EXPECT_EQ(condbr.kind, TerminatorKind::CondBranch);
    EXPECT_EQ(condbr.operands.size(), 1);
    EXPECT_EQ(condbr.targets.size(), 2);
    EXPECT_EQ(condbr.targets[0], "then_block");
    EXPECT_EQ(condbr.targets[1], "else_block");
}

TEST_F(MIRTerminatorTest, Unreachable) {
    Terminator unreachable(TerminatorKind::Unreachable);

    EXPECT_EQ(unreachable.kind, TerminatorKind::Unreachable);
}

// ============================================================================
// MIR Basic Block Tests
// ============================================================================

class MIRBasicBlockTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRBasicBlockTest, EmptyBlock) {
    BasicBlock block("entry");

    EXPECT_EQ(block.label, "entry");
    EXPECT_EQ(block.instructions.size(), 0);
    EXPECT_FALSE(block.hasTerminator());
}

TEST_F(MIRBasicBlockTest, AddInstructions) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    BasicBlock block("compute");

    Value result = Value::makeLocal("0", i32Type);
    Value a = Value::makeLocal("a", i32Type);
    Value b = Value::makeLocal("b", i32Type);

    block.addInstruction(Instruction(Opcode::IAdd, result, {a, b}));
    block.addInstruction(Instruction(Opcode::IMul, result, {result, a}));

    EXPECT_EQ(block.instructions.size(), 2);
    EXPECT_EQ(block.instructions[0].opcode, Opcode::IAdd);
    EXPECT_EQ(block.instructions[1].opcode, Opcode::IMul);
}

TEST_F(MIRBasicBlockTest, SetTerminator) {
    BasicBlock block("exit");

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    Value retVal = Value::makeConstantInt(0, i32Type);

    block.setTerminator(Terminator::makeReturn(retVal));

    EXPECT_TRUE(block.hasTerminator());
    EXPECT_EQ(block.terminator.kind, TerminatorKind::Return);
}

TEST_F(MIRBasicBlockTest, CompleteBlock) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    BasicBlock block("add_block");

    // Add instructions
    Value result = Value::makeLocal("sum", i32Type);
    Value a = Value::makeParam("a", i32Type);
    Value b = Value::makeParam("b", i32Type);

    block.addInstruction(Instruction(Opcode::IAdd, result, {a, b}));

    // Add terminator
    block.setTerminator(Terminator::makeReturn(result));

    EXPECT_EQ(block.instructions.size(), 1);
    EXPECT_TRUE(block.hasTerminator());
}

// ============================================================================
// MIR Function Tests
// ============================================================================

class MIRFunctionTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRFunctionTest, EmptyFunction) {
    auto* voidType = types.getPrimitive(PrimitiveKind::Void);

    Function func("empty", {}, voidType);

    EXPECT_EQ(func.name, "empty");
    EXPECT_EQ(func.params.size(), 0);
    EXPECT_EQ(func.returnType, voidType);
    EXPECT_EQ(func.blocks.size(), 0);
}

TEST_F(MIRFunctionTest, FunctionWithParams) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    std::vector<Value> params;
    params.push_back(Value::makeParam("a", i32Type));
    params.push_back(Value::makeParam("b", i32Type));

    Function func("add", params, i32Type);

    EXPECT_EQ(func.name, "add");
    EXPECT_EQ(func.params.size(), 2);
    EXPECT_EQ(func.params[0].name, "a");
    EXPECT_EQ(func.params[1].name, "b");
    EXPECT_EQ(func.returnType, i32Type);
}

TEST_F(MIRFunctionTest, AddBlock) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Function func("test", {}, i32Type);

    BasicBlock entry("entry");
    func.addBlock(std::move(entry));

    EXPECT_EQ(func.blocks.size(), 1);
    EXPECT_EQ(func.blocks[0].label, "entry");
}

TEST_F(MIRFunctionTest, GetBlockByLabel) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Function func("test", {}, i32Type);

    BasicBlock entry("entry");
    BasicBlock body("body");
    BasicBlock exit("exit");

    func.addBlock(std::move(entry));
    func.addBlock(std::move(body));
    func.addBlock(std::move(exit));

    BasicBlock* foundEntry = func.getBlock("entry");
    BasicBlock* foundBody = func.getBlock("body");
    BasicBlock* foundExit = func.getBlock("exit");
    BasicBlock* notFound = func.getBlock("nonexistent");

    ASSERT_NE(foundEntry, nullptr);
    EXPECT_EQ(foundEntry->label, "entry");

    ASSERT_NE(foundBody, nullptr);
    EXPECT_EQ(foundBody->label, "body");

    ASSERT_NE(foundExit, nullptr);
    EXPECT_EQ(foundExit->label, "exit");

    EXPECT_EQ(notFound, nullptr);
}

TEST_F(MIRFunctionTest, CompleteAddFunction) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    // fn add(a: i32, b: i32) -> i32
    std::vector<Value> params;
    params.push_back(Value::makeParam("a", i32Type));
    params.push_back(Value::makeParam("b", i32Type));

    Function func("add", params, i32Type);

    // entry:
    //   %0 = iadd %a, %b
    //   ret %0
    BasicBlock entry("entry");

    Value result = Value::makeLocal("0", i32Type);
    entry.addInstruction(Instruction(Opcode::IAdd, result, {params[0], params[1]}));
    entry.setTerminator(Terminator::makeReturn(result));

    func.addBlock(std::move(entry));

    EXPECT_EQ(func.name, "add");
    EXPECT_EQ(func.params.size(), 2);
    EXPECT_EQ(func.blocks.size(), 1);
    EXPECT_EQ(func.blocks[0].instructions.size(), 1);
    EXPECT_TRUE(func.blocks[0].hasTerminator());
}

// ============================================================================
// MIR Program Tests
// ============================================================================

class MIRProgramTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRProgramTest, EmptyProgram) {
    Program program;

    EXPECT_EQ(program.functions.size(), 0);
}

TEST_F(MIRProgramTest, AddFunction) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Program program;

    Function func("main", {}, i32Type);
    program.addFunction(std::move(func));

    EXPECT_EQ(program.functions.size(), 1);
    EXPECT_EQ(program.functions[0].name, "main");
}

TEST_F(MIRProgramTest, GetFunctionByName) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    Program program;

    Function main("main", {}, i32Type);
    Function add("add", {}, i32Type);

    program.addFunction(std::move(main));
    program.addFunction(std::move(add));

    Function* foundMain = program.getFunction("main");
    Function* foundAdd = program.getFunction("add");
    Function* notFound = program.getFunction("nonexistent");

    ASSERT_NE(foundMain, nullptr);
    EXPECT_EQ(foundMain->name, "main");

    ASSERT_NE(foundAdd, nullptr);
    EXPECT_EQ(foundAdd->name, "add");

    EXPECT_EQ(notFound, nullptr);
}

// ============================================================================
// MIR Builder Tests (Construction Interface)
// ============================================================================

class MIRBuilderTest : public ::testing::Test {
protected:
    TypeRegistry types;
};

TEST_F(MIRBuilderTest, CreateFunction) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    std::vector<Value> params;
    params.push_back(Value::makeParam("x", i32Type));

    builder.createFunction("test", params, i32Type);

    EXPECT_NE(builder.getCurrentFunction(), nullptr);
    EXPECT_EQ(builder.getCurrentFunction()->name, "test");
}

TEST_F(MIRBuilderTest, CreateBlock) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    EXPECT_EQ(builder.getCurrentBlockLabel(), "entry");
}

TEST_F(MIRBuilderTest, CreateTemporary) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    builder.createFunction("test", {}, i32Type);

    Value temp1 = builder.createTemporary(i32Type);
    Value temp2 = builder.createTemporary(i32Type);
    Value temp3 = builder.createTemporary(i32Type);

    EXPECT_EQ(temp1.name, "0");
    EXPECT_EQ(temp2.name, "1");
    EXPECT_EQ(temp3.name, "2");
}

TEST_F(MIRBuilderTest, CreateIntegerArithmetic) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    Value a = builder.createConstantInt(10, i32Type);
    Value b = builder.createConstantInt(20, i32Type);

    Value sum = builder.createIAdd(a, b);
    Value diff = builder.createISub(a, b);
    Value prod = builder.createIMul(a, b);
    Value quot = builder.createIDiv(a, b);

    EXPECT_EQ(sum.type, i32Type);
    EXPECT_EQ(diff.type, i32Type);
    EXPECT_EQ(prod.type, i32Type);
    EXPECT_EQ(quot.type, i32Type);
}

TEST_F(MIRBuilderTest, CreateIntegerComparison) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);

    builder.createFunction("test", {}, boolType);
    builder.createBlock("entry");

    Value a = builder.createConstantInt(10, i32Type);
    Value b = builder.createConstantInt(20, i32Type);

    Value eq = builder.createICmpEQ(a, b);
    Value lt = builder.createICmpLT(a, b);

    EXPECT_EQ(eq.type, boolType);
    EXPECT_EQ(lt.type, boolType);
}

TEST_F(MIRBuilderTest, CreateAlloca) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* ptrType = types.getPointer(i32Type);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    Value ptr = builder.createAlloca(i32Type);

    EXPECT_EQ(ptr.type, ptrType);
}

TEST_F(MIRBuilderTest, CreateLoadStore) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    Value ptr = builder.createAlloca(i32Type);
    Value constant = builder.createConstantInt(42, i32Type);

    builder.createStore(constant, ptr);
    Value loaded = builder.createLoad(ptr);

    EXPECT_EQ(loaded.type, i32Type);
}

TEST_F(MIRBuilderTest, CreateBranches) {
    MIRBuilder builder(types);

    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    Value condition = builder.createConstantBool(true, boolType);
    builder.createCondBranch(condition, "then_block", "else_block");

    EXPECT_TRUE(builder.currentBlockTerminated());
}

TEST_F(MIRBuilderTest, CreateReturn) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    builder.createFunction("test", {}, i32Type);
    builder.createBlock("entry");

    Value retVal = builder.createConstantInt(0, i32Type);
    builder.createReturn(retVal);

    EXPECT_TRUE(builder.currentBlockTerminated());
}

TEST_F(MIRBuilderTest, BuildCompleteFunction) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    // fn add(a: i32, b: i32) -> i32
    std::vector<Value> params;
    params.push_back(Value::makeParam("a", i32Type));
    params.push_back(Value::makeParam("b", i32Type));

    builder.createFunction("add", params, i32Type);
    builder.createBlock("entry");

    Value sum = builder.createIAdd(params[0], params[1]);
    builder.createReturn(sum);

    builder.finishFunction();

    Program program = builder.getProgram();

    EXPECT_EQ(program.functions.size(), 1);
    EXPECT_EQ(program.functions[0].name, "add");
    EXPECT_EQ(program.functions[0].params.size(), 2);
    EXPECT_EQ(program.functions[0].blocks.size(), 1);
    EXPECT_TRUE(program.functions[0].blocks[0].hasTerminator());
}

TEST_F(MIRBuilderTest, SymbolTable) {
    MIRBuilder builder(types);

    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    builder.createFunction("test", {}, i32Type);

    Value var = builder.createNamedValue("x", i32Type);
    builder.setVariable("x", var);

    EXPECT_TRUE(builder.hasVariable("x"));

    Value retrieved = builder.getVariable("x");
    EXPECT_EQ(retrieved.name, var.name);

    EXPECT_FALSE(builder.hasVariable("nonexistent"));
}
