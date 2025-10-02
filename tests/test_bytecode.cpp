#include <gtest/gtest.h>
#include "bytecode/Bytecode.hpp"
#include "bytecode/BytecodeCompiler.hpp"
#include "bytecode/Disassembler.hpp"
#include "bytecode/BytecodeSerializer.hpp"
#include "IR/IRModule.hpp"
#include "IR/IRBuilder.hpp"
#include "semantic/Type.hpp"
#include <sstream>
#include <fstream>

using namespace volta::bytecode;
using namespace volta::ir;
using namespace volta::semantic;

// ============================================================================
// Bytecode Chunk Tests
// ============================================================================

TEST(BytecodeTest, Chunk_EmitOpcode) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);

    EXPECT_EQ(chunk.code().size(), 1);
    EXPECT_EQ(static_cast<Opcode>(chunk.code()[0]), Opcode::ConstInt);
}

TEST(BytecodeTest, Chunk_EmitInt32) {
    Chunk chunk;
    chunk.emitInt32(0x12345678);

    EXPECT_EQ(chunk.code().size(), 4);
    // Little-endian: 78 56 34 12
    EXPECT_EQ(chunk.code()[0], 0x78);
    EXPECT_EQ(chunk.code()[1], 0x56);
    EXPECT_EQ(chunk.code()[2], 0x34);
    EXPECT_EQ(chunk.code()[3], 0x12);
}

TEST(BytecodeTest, Chunk_EmitInt64) {
    Chunk chunk;
    chunk.emitInt64(42);

    EXPECT_EQ(chunk.code().size(), 8);
    // First byte should be 42, rest zeros
    EXPECT_EQ(chunk.code()[0], 42);
}

TEST(BytecodeTest, Chunk_EmitFloat64) {
    Chunk chunk;
    chunk.emitFloat64(3.14);

    EXPECT_EQ(chunk.code().size(), 8);
}

TEST(BytecodeTest, Chunk_EmitBool) {
    Chunk chunk;
    chunk.emitBool(true);
    chunk.emitBool(false);

    EXPECT_EQ(chunk.code().size(), 2);
    EXPECT_EQ(chunk.code()[0], 1);
    EXPECT_EQ(chunk.code()[1], 0);
}

TEST(BytecodeTest, Chunk_CurrentOffset) {
    Chunk chunk;
    EXPECT_EQ(chunk.currentOffset(), 0);

    chunk.emitOpcode(Opcode::ConstInt);
    EXPECT_EQ(chunk.currentOffset(), 1);

    chunk.emitInt64(42);
    EXPECT_EQ(chunk.currentOffset(), 9);
}

TEST(BytecodeTest, Chunk_PatchInt32) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::Jump);
    size_t patchOffset = chunk.currentOffset();
    chunk.emitInt32(0);  // Placeholder

    chunk.patchInt32(patchOffset, 100);

    // Verify the patch
    EXPECT_EQ(chunk.code()[patchOffset], 100);
}

TEST(BytecodeTest, Chunk_AddLineNumber) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);
    chunk.addLineNumber(0, 42);

    EXPECT_EQ(chunk.getLineNumber(0), 42);
}

TEST(BytecodeTest, Chunk_GetLineNumber_NotFound) {
    Chunk chunk;

    // Line number for offset without debug info should return 0
    EXPECT_EQ(chunk.getLineNumber(999), 0);
}

TEST(BytecodeTest, GetOpcodeName) {
    EXPECT_EQ(getOpcodeName(Opcode::AddInt), "AddInt");
    EXPECT_EQ(getOpcodeName(Opcode::ConstFloat), "ConstFloat");
    EXPECT_EQ(getOpcodeName(Opcode::Jump), "Jump");
    EXPECT_EQ(getOpcodeName(Opcode::Return), "Return");
}

TEST(BytecodeTest, GetOpcodeOperandSize) {
    EXPECT_EQ(getOpcodeOperandSize(Opcode::Pop), 0);        // No operands
    EXPECT_EQ(getOpcodeOperandSize(Opcode::ConstInt), 8);   // 8-byte int
    EXPECT_EQ(getOpcodeOperandSize(Opcode::ConstFloat), 8); // 8-byte float
    EXPECT_EQ(getOpcodeOperandSize(Opcode::ConstBool), 1);  // 1-byte bool
    EXPECT_EQ(getOpcodeOperandSize(Opcode::LoadLocal), 4);  // 4-byte index
    EXPECT_EQ(getOpcodeOperandSize(Opcode::Jump), 4);       // 4-byte offset
}

// ============================================================================
// Bytecode Compiler Tests
// ============================================================================

TEST(BytecodeCompilerTest, CompileEmptyModule) {
    IRModule module("empty");

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->name(), "empty");
    EXPECT_EQ(compiled->functions().size(), 0);
}

TEST(BytecodeCompilerTest, CompileSimpleConstant) {
    // fn main() -> int { return 42; }
    IRModule module("test");
    IRBuilder builder;

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    auto func = builder.createFunction("main", funcType);
    auto entry = builder.createBasicBlock("entry", func.get());
    builder.setInsertPoint(entry.get());

    auto constant = builder.getInt(42);
    builder.createRet(constant);

    module.addFunction(std::move(func));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->functions().size(), 1);

    auto& mainFunc = compiled->functions()[0];
    EXPECT_EQ(mainFunc.name, "main");
    EXPECT_GT(mainFunc.chunk.code().size(), 0);
}

TEST(BytecodeCompilerTest, CompileArithmetic) {
    // fn add(a: int, b: int) -> int { return a + b; }
    IRModule module("test");
    IRBuilder builder;

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType, intType},
        intType
    );

    auto func = builder.createFunction("add", funcType);
    auto paramA = builder.createParameter(intType, "a", 0);
    auto paramB = builder.createParameter(intType, "b", 1);

    auto entry = builder.createBasicBlock("entry", func.get());
    builder.setInsertPoint(entry.get());

    auto sum = builder.createAdd(paramA.get(), paramB.get());
    builder.createRet(sum);

    module.addFunction(std::move(func));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->functions().size(), 1);

    auto& addFunc = compiled->functions()[0];
    EXPECT_EQ(addFunc.name, "add");
    EXPECT_EQ(addFunc.parameterCount, 2);
}

TEST(BytecodeCompilerTest, CompileConditionalBranch) {
    // fn abs(x: int) -> int {
    //     if x < 0 { return -x; } else { return x; }
    // }
    IRModule module("test");
    IRBuilder builder;

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto boolType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType},
        intType
    );

    auto func = builder.createFunction("abs", funcType);
    auto paramX = builder.createParameter(intType, "x", 0);

    auto entry = builder.createBasicBlock("entry", func.get());
    auto thenBlock = builder.createBasicBlock("then", func.get());
    auto elseBlock = builder.createBasicBlock("else", func.get());

    // Entry: if x < 0
    builder.setInsertPoint(entry.get());
    auto zero = builder.getInt(0);
    auto cond = builder.createLt(paramX.get(), zero);
    builder.createBrIf(cond, thenBlock.get(), elseBlock.get());

    // Then: return -x
    builder.setInsertPoint(thenBlock.get());
    auto negX = builder.createNeg(paramX.get());
    builder.createRet(negX);

    // Else: return x
    builder.setInsertPoint(elseBlock.get());
    builder.createRet(paramX.get());

    module.addFunction(std::move(func));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->functions().size(), 1);
}

TEST(BytecodeCompilerTest, CompileFunctionCall) {
    // fn helper() -> int { return 1; }
    // fn main() -> int { return helper(); }
    IRModule module("test");
    IRBuilder builder;

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    // Helper function
    auto helper = builder.createFunction("helper", funcType);
    auto helperEntry = builder.createBasicBlock("entry", helper.get());
    builder.setInsertPoint(helperEntry.get());
    auto one = builder.getInt(1);
    builder.createRet(one);

    Function* helperPtr = helper.get();
    module.addFunction(std::move(helper));

    // Main function
    auto main = builder.createFunction("main", funcType);
    auto mainEntry = builder.createBasicBlock("entry", main.get());
    builder.setInsertPoint(mainEntry.get());
    auto result = builder.createCall(helperPtr, {});
    builder.createRet(result);

    module.addFunction(std::move(main));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->functions().size(), 2);
}

TEST(BytecodeCompilerTest, CompileStringLiterals) {
    // fn main() -> str { return "hello"; }
    IRModule module("test");
    IRBuilder builder;

    auto strType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        strType
    );

    auto func = builder.createFunction("main", funcType);
    auto entry = builder.createBasicBlock("entry", func.get());
    builder.setInsertPoint(entry.get());

    auto str = builder.getString("hello");
    builder.createRet(str);

    module.addFunction(std::move(func));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->stringPool().size(), 1);
    EXPECT_EQ(compiled->stringPool()[0], "hello");
}

TEST(BytecodeCompilerTest, CompileGlobalVariable) {
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto global = std::make_unique<GlobalVariable>(intType, "x");
    module.addGlobal(std::move(global));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_GE(compiled->globalCount(), 1);
}

TEST(BytecodeCompilerTest, CompileForeignFunction) {
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType},
        intType
    );

    module.declareForeignFunction("print", funcType);

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_GE(compiled->foreignFunctions().size(), 1);
}

// ============================================================================
// Disassembler Tests
// ============================================================================

TEST(DisassemblerTest, DisassembleEmptyChunk) {
    Chunk chunk;

    Disassembler disasm;
    std::string output = disasm.disassembleChunk(chunk);

    // Should not crash
    EXPECT_TRUE(output.empty() || !output.empty());
}

TEST(DisassemblerTest, DisassembleSimpleInstruction) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);
    chunk.emitInt64(42);

    Disassembler disasm;
    std::string output = disasm.disassembleChunk(chunk);

    // Should contain "ConstInt" and "42"
    EXPECT_NE(output.find("ConstInt"), std::string::npos);
    EXPECT_NE(output.find("42"), std::string::npos);
}

TEST(DisassemblerTest, DisassembleMultipleInstructions) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);
    chunk.emitInt64(10);
    chunk.emitOpcode(Opcode::ConstInt);
    chunk.emitInt64(20);
    chunk.emitOpcode(Opcode::AddInt);

    Disassembler disasm;
    std::string output = disasm.disassembleChunk(chunk);

    // Should contain all instructions
    EXPECT_NE(output.find("ConstInt"), std::string::npos);
    EXPECT_NE(output.find("AddInt"), std::string::npos);
}

TEST(DisassemblerTest, DisassembleWithOffsets) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::Pop);

    Disassembler disasm;
    disasm.setShowOffsets(true);
    std::string output = disasm.disassembleChunk(chunk);

    // Should show offset 0000
    EXPECT_NE(output.find("0000"), std::string::npos);
}

TEST(DisassemblerTest, DisassembleStringConstant) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstString);
    chunk.emitInt32(0);  // String pool index

    std::vector<std::string> stringPool = {"hello"};

    Disassembler disasm;
    disasm.setStringPool(&stringPool);
    std::string output = disasm.disassembleChunk(chunk);

    // Should contain "hello"
    EXPECT_NE(output.find("hello"), std::string::npos);
}

TEST(DisassemblerTest, DisassembleModule) {
    CompiledModule module("test");

    Disassembler disasm;
    std::string output = disasm.disassembleModule(module);

    // Should contain module name
    EXPECT_NE(output.find("test"), std::string::npos);
}

TEST(DisassemblerTest, DisassembleFunction) {
    CompiledFunction func;
    func.name = "test_func";
    func.parameterCount = 2;
    func.localCount = 5;
    func.chunk.emitOpcode(Opcode::ReturnVoid);

    Disassembler disasm;
    std::string output = disasm.disassembleFunction(func);

    // Should contain function metadata
    EXPECT_NE(output.find("test_func"), std::string::npos);
    EXPECT_NE(output.find("ReturnVoid"), std::string::npos);
}

TEST(DisassemblerTest, DumpUtilities) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::Halt);

    // Should not crash
    EXPECT_NO_THROW(dumpChunk(chunk));
}

// ============================================================================
// Serializer Tests
// ============================================================================

TEST(BytecodeSerializerTest, SerializeEmptyModule) {
    CompiledModule module("empty");

    std::ostringstream out;
    BytecodeSerializer serializer;
    bool success = serializer.serialize(module, out);

    EXPECT_TRUE(success);
    EXPECT_GT(out.str().size(), 0);
}

TEST(BytecodeSerializerTest, SerializeDeserializeRoundTrip) {
    CompiledModule original("test");
    original.stringPool().push_back("hello");
    original.stringPool().push_back("world");
    original.foreignFunctions().push_back("print");
    original.setGlobalCount(2);

    // Add a simple function
    CompiledFunction func;
    func.name = "main";
    func.index = 0;
    func.parameterCount = 0;
    func.localCount = 1;
    func.isForeign = false;
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(42);
    func.chunk.emitOpcode(Opcode::Return);
    original.functions().push_back(std::move(func));

    // Serialize
    std::ostringstream out;
    BytecodeSerializer serializer;
    ASSERT_TRUE(serializer.serialize(original, out));

    // Deserialize
    std::istringstream in(out.str());
    auto deserialized = serializer.deserialize(in);

    ASSERT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->name(), "test");
    EXPECT_EQ(deserialized->stringPool().size(), 2);
    EXPECT_EQ(deserialized->stringPool()[0], "hello");
    EXPECT_EQ(deserialized->stringPool()[1], "world");
    EXPECT_EQ(deserialized->foreignFunctions().size(), 1);
    EXPECT_EQ(deserialized->globalCount(), 2);
    EXPECT_EQ(deserialized->functions().size(), 1);
    EXPECT_EQ(deserialized->functions()[0].name, "main");
}

TEST(BytecodeSerializerTest, SerializeToFile) {
    CompiledModule module("file_test");

    BytecodeSerializer serializer;
    std::string filepath = "/tmp/test_module.vltb";

    bool success = serializer.serializeToFile(module, filepath);
    EXPECT_TRUE(success);

    // Verify file exists
    std::ifstream file(filepath);
    EXPECT_TRUE(file.good());
}

TEST(BytecodeSerializerTest, DeserializeFromFile) {
    CompiledModule original("file_test");
    original.stringPool().push_back("test_string");

    std::string filepath = "/tmp/test_deserialize.vltb";

    BytecodeSerializer serializer;
    ASSERT_TRUE(serializer.serializeToFile(original, filepath));

    auto deserialized = serializer.deserializeFromFile(filepath);
    ASSERT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->name(), "file_test");
    EXPECT_EQ(deserialized->stringPool().size(), 1);
}

TEST(BytecodeSerializerTest, DeserializeInvalidFile) {
    BytecodeSerializer serializer;
    auto result = serializer.deserializeFromFile("/nonexistent/file.vltb");

    EXPECT_EQ(result, nullptr);
    EXPECT_TRUE(serializer.hasError());
}

TEST(BytecodeSerializerTest, DeserializeCorruptedData) {
    std::string corrupted = "This is not valid bytecode!";
    std::istringstream in(corrupted);

    BytecodeSerializer serializer;
    auto result = serializer.deserialize(in);

    EXPECT_EQ(result, nullptr);
    EXPECT_TRUE(serializer.hasError());
}

TEST(BytecodeSerializerTest, UtilityFunctions) {
    CompiledModule module("util_test");
    std::string filepath = "/tmp/test_utils.vltb";

    // Test utility save
    bool saved = saveCompiledModule(module, filepath);
    EXPECT_TRUE(saved);

    // Test utility load
    auto loaded = loadCompiledModule(filepath);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->name(), "util_test");
}

// ============================================================================
// Compiled Module Tests
// ============================================================================

TEST(CompiledModuleTest, GetFunctionByIndex) {
    CompiledModule module("test");

    CompiledFunction func;
    func.name = "test";
    func.index = 0;
    module.functions().push_back(std::move(func));

    auto* retrieved = module.getFunction(0);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "test");
}

TEST(CompiledModuleTest, GetFunctionByName) {
    CompiledModule module("test");

    CompiledFunction func;
    func.name = "my_func";
    func.index = 0;
    module.functions().push_back(std::move(func));

    auto* retrieved = module.getFunction("my_func");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "my_func");
}

TEST(CompiledModuleTest, GetFunctionByName_NotFound) {
    CompiledModule module("test");

    auto* retrieved = module.getFunction("nonexistent");
    EXPECT_EQ(retrieved, nullptr);
}

TEST(CompiledModuleTest, GetEntryPoint) {
    CompiledModule module("test");

    CompiledFunction main;
    main.name = "main";
    main.index = 0;
    module.functions().push_back(std::move(main));

    auto* entry = module.getEntryPoint();
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->name, "main");
}

TEST(CompiledModuleTest, GetEntryPoint_NotFound) {
    CompiledModule module("test");

    auto* entry = module.getEntryPoint();
    EXPECT_EQ(entry, nullptr);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(BytecodeEdgeCaseTest, EmptyFunction) {
    IRModule module("test");
    IRBuilder builder;

    auto voidType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Void);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        voidType
    );

    auto func = builder.createFunction("empty", funcType);
    auto entry = builder.createBasicBlock("entry", func.get());
    builder.setInsertPoint(entry.get());
    builder.createRetVoid();

    module.addFunction(std::move(func));

    BytecodeCompiler compiler;
    auto compiled = compiler.compile(module);

    ASSERT_NE(compiled, nullptr);
    EXPECT_EQ(compiled->functions().size(), 1);
}

TEST(BytecodeEdgeCaseTest, VeryLargeConstant) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);
    chunk.emitInt64(INT64_MAX);

    EXPECT_GT(chunk.code().size(), 0);
}

TEST(BytecodeEdgeCaseTest, NegativeConstant) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);
    chunk.emitInt64(-42);

    EXPECT_GT(chunk.code().size(), 0);
}

TEST(BytecodeEdgeCaseTest, EmptyStringLiteral) {
    CompiledModule module("test");
    module.stringPool().push_back("");

    EXPECT_EQ(module.stringPool().size(), 1);
    EXPECT_EQ(module.stringPool()[0], "");
}

TEST(BytecodeEdgeCaseTest, ManyInstructions) {
    Chunk chunk;

    for (int i = 0; i < 1000; i++) {
        chunk.emitOpcode(Opcode::Pop);
    }

    EXPECT_EQ(chunk.code().size(), 1000);
}

TEST(BytecodeEdgeCaseTest, DeepJumpNesting) {
    Chunk chunk;

    for (int i = 0; i < 100; i++) {
        chunk.emitOpcode(Opcode::Jump);
        chunk.emitInt32(i * 10);
    }

    EXPECT_GT(chunk.code().size(), 0);
}

TEST(BytecodeEdgeCaseTest, LargeStringPool) {
    CompiledModule module("test");

    for (int i = 0; i < 1000; i++) {
        module.stringPool().push_back("string_" + std::to_string(i));
    }

    EXPECT_EQ(module.stringPool().size(), 1000);
}
