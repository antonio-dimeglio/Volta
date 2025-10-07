#include <gtest/gtest.h>
#include "vm/BytecodeModule.hpp"
#include "vm/Value.hpp"

using namespace volta::vm;

// ============================================================================
// Constant Pool Tests
// ============================================================================

TEST(BytecodeModuleTest, AddIntConstant) {
    BytecodeModule module;

    uint16_t idx1 = module.addIntConstant(42);
    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(module.getIntConstant(idx1), 42);

    uint16_t idx2 = module.addIntConstant(100);
    EXPECT_EQ(idx2, 1);
    EXPECT_EQ(module.getIntConstant(idx2), 100);
}

TEST(BytecodeModuleTest, IntConstantInterning) {
    BytecodeModule module;

    // Add same constant twice
    uint16_t idx1 = module.addIntConstant(42);
    uint16_t idx2 = module.addIntConstant(42);

    // Should return same index (interning)
    EXPECT_EQ(idx1, idx2);
    EXPECT_EQ(module.getIntConstantCount(), 1);
}

TEST(BytecodeModuleTest, AddFloatConstant) {
    BytecodeModule module;

    uint16_t idx1 = module.addFloatConstant(3.14);
    EXPECT_EQ(idx1, 0);
    EXPECT_DOUBLE_EQ(module.getFloatConstant(idx1), 3.14);

    uint16_t idx2 = module.addFloatConstant(2.71);
    EXPECT_EQ(idx2, 1);
    EXPECT_DOUBLE_EQ(module.getFloatConstant(idx2), 2.71);
}

TEST(BytecodeModuleTest, FloatConstantInterning) {
    BytecodeModule module;

    uint16_t idx1 = module.addFloatConstant(3.14);
    uint16_t idx2 = module.addFloatConstant(3.14);

    EXPECT_EQ(idx1, idx2);
    EXPECT_EQ(module.getFloatConstantCount(), 1);
}

TEST(BytecodeModuleTest, AddStringConstant) {
    BytecodeModule module;

    uint16_t idx1 = module.addStringConstant("hello");
    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(module.getStringConstant(idx1), "hello");

    uint16_t idx2 = module.addStringConstant("world");
    EXPECT_EQ(idx2, 1);
    EXPECT_EQ(module.getStringConstant(idx2), "world");
}

TEST(BytecodeModuleTest, StringConstantInterning) {
    BytecodeModule module;

    uint16_t idx1 = module.addStringConstant("hello");
    uint16_t idx2 = module.addStringConstant("hello");

    EXPECT_EQ(idx1, idx2);
    EXPECT_EQ(module.getStringConstantCount(), 1);
}

TEST(BytecodeModuleTest, MixedConstants) {
    BytecodeModule module;

    uint16_t int_idx = module.addIntConstant(42);
    uint16_t float_idx = module.addFloatConstant(3.14);
    uint16_t str_idx = module.addStringConstant("test");

    EXPECT_EQ(module.getIntConstantCount(), 1);
    EXPECT_EQ(module.getFloatConstantCount(), 1);
    EXPECT_EQ(module.getStringConstantCount(), 1);

    EXPECT_EQ(module.getIntConstant(int_idx), 42);
    EXPECT_DOUBLE_EQ(module.getFloatConstant(float_idx), 3.14);
    EXPECT_EQ(module.getStringConstant(str_idx), "test");
}

// ============================================================================
// Function Table Tests
// ============================================================================

TEST(BytecodeModuleTest, AddBytecodeFunction) {
    BytecodeModule module;

    uint16_t idx = module.addFunction("test_func", 0, 10, 2, 5);

    EXPECT_EQ(idx, 0);
    EXPECT_EQ(module.getFunctionCount(), 1);

    const FunctionInfo& func = module.getFunction(idx);
    EXPECT_EQ(func.getName(), "test_func");
    EXPECT_EQ(func.getCodeOffset(), 0);
    EXPECT_EQ(func.getCodeLength(), 10);
    EXPECT_EQ(func.getParamCount(), 2);
    EXPECT_EQ(func.getRegisterCount(), 5);
    EXPECT_FALSE(func.isNative());
}

// Dummy runtime function for testing
Value dummy_runtime_func(VM* vm, Value* args, int arg_count) {
    return Value{};
}

TEST(BytecodeModuleTest, AddNativeFunction) {
    BytecodeModule module;

    uint16_t idx = module.addNativeFunction("__print", dummy_runtime_func, 1);

    EXPECT_EQ(idx, 0);
    EXPECT_EQ(module.getFunctionCount(), 1);

    const FunctionInfo& func = module.getFunction(idx);
    EXPECT_EQ(func.getName(), "__print");
    EXPECT_EQ(func.getParamCount(), 1);
    EXPECT_TRUE(func.isNative());
    EXPECT_EQ(func.getNativePtr(), dummy_runtime_func);
}

TEST(BytecodeModuleTest, GetFunctionByName) {
    BytecodeModule module;

    module.addFunction("main", 0, 10, 0, 3);
    module.addFunction("helper", 10, 5, 1, 2);

    EXPECT_EQ(module.getFunctionIndex("main"), 0);
    EXPECT_EQ(module.getFunctionIndex("helper"), 1);
    EXPECT_TRUE(module.hasFunction("main"));
    EXPECT_TRUE(module.hasFunction("helper"));
    EXPECT_FALSE(module.hasFunction("nonexistent"));
}

TEST(BytecodeModuleTest, MultipleFunctions) {
    BytecodeModule module;

    module.addNativeFunction("__alloc_array", dummy_runtime_func, 2);
    module.addNativeFunction("__print", dummy_runtime_func, 1);
    module.addFunction("main", 0, 20, 0, 5);
    module.addFunction("fibonacci", 20, 30, 1, 10);

    EXPECT_EQ(module.getFunctionCount(), 4);

    // Runtime functions first
    EXPECT_EQ(module.getFunction(0).getName(), "__alloc_array");
    EXPECT_EQ(module.getFunction(1).getName(), "__print");

    // User functions after
    EXPECT_EQ(module.getFunction(2).getName(), "main");
    EXPECT_EQ(module.getFunction(3).getName(), "fibonacci");
}

// ============================================================================
// Code Emission Tests
// ============================================================================

TEST(BytecodeModuleTest, EmitByte) {
    BytecodeModule module;

    EXPECT_EQ(module.getCurrentOffset(), 0);

    module.emitByte(0x42);
    EXPECT_EQ(module.getCurrentOffset(), 1);

    module.emitByte(0xFF);
    EXPECT_EQ(module.getCurrentOffset(), 2);

    EXPECT_EQ(module.readByte(0), 0x42);
    EXPECT_EQ(module.readByte(1), 0xFF);
}

TEST(BytecodeModuleTest, EmitU16LittleEndian) {
    BytecodeModule module;

    module.emitU16(0x1234);

    // Little-endian: low byte first, then high byte
    EXPECT_EQ(module.readByte(0), 0x34);  // Low byte
    EXPECT_EQ(module.readByte(1), 0x12);  // High byte
}

TEST(BytecodeModuleTest, EmitI16) {
    BytecodeModule module;

    module.emitI16(-1);

    // -1 in 16-bit two's complement = 0xFFFF
    EXPECT_EQ(module.readByte(0), 0xFF);
    EXPECT_EQ(module.readByte(1), 0xFF);
}

TEST(BytecodeModuleTest, EmitU32LittleEndian) {
    BytecodeModule module;

    module.emitU32(0x12345678);

    // Little-endian: lowest byte first
    EXPECT_EQ(module.readByte(0), 0x78);
    EXPECT_EQ(module.readByte(1), 0x56);
    EXPECT_EQ(module.readByte(2), 0x34);
    EXPECT_EQ(module.readByte(3), 0x12);
}

TEST(BytecodeModuleTest, EmitMultipleInstructions) {
    BytecodeModule module;

    // Emit: ADD r1, r2, r3 (opcode + 3 registers)
    module.emitByte(0x01);  // ADD opcode
    module.emitByte(1);     // r1
    module.emitByte(2);     // r2
    module.emitByte(3);     // r3

    EXPECT_EQ(module.getCurrentOffset(), 4);
}

// ============================================================================
// Code Patching Tests
// ============================================================================

TEST(BytecodeModuleTest, PatchU16) {
    BytecodeModule module;

    // Emit placeholder
    module.emitU16(0x0000);

    // Patch with actual value
    module.patchU16(0, 0xABCD);

    EXPECT_EQ(module.readByte(0), 0xCD);  // Low byte
    EXPECT_EQ(module.readByte(1), 0xAB);  // High byte
}

TEST(BytecodeModuleTest, PatchI16) {
    BytecodeModule module;

    module.emitI16(0);
    module.patchI16(0, -100);

    // Read back and verify
    uint32_t ip = 0;
    int16_t value = module.fetchI16(ip);
    EXPECT_EQ(value, -100);
}

TEST(BytecodeModuleTest, MultiplePatchesInTwoPassCompilation) {
    BytecodeModule module;

    // Simulate two-pass compilation with forward branches

    // Pass 1: Emit code with placeholders
    module.emitByte(0x40);      // BR opcode
    uint32_t patch_offset_1 = module.getCurrentOffset();
    module.emitI16(0);          // Placeholder offset

    module.emitByte(0x41);      // BR_IF_TRUE opcode
    module.emitByte(1);         // Condition register
    uint32_t patch_offset_2 = module.getCurrentOffset();
    module.emitI16(0);          // Placeholder offset

    // Pass 2: Patch offsets
    module.patchI16(patch_offset_1, 10);
    module.patchI16(patch_offset_2, -5);

    // Verify
    uint32_t ip = 1;
    EXPECT_EQ(module.fetchI16(ip), 10);

    ip = 5;
    EXPECT_EQ(module.fetchI16(ip), -5);
}

// ============================================================================
// Code Reading Tests
// ============================================================================

TEST(BytecodeModuleTest, FetchByte) {
    BytecodeModule module;

    module.emitByte(0x01);
    module.emitByte(0x02);
    module.emitByte(0x03);

    uint32_t ip = 0;
    EXPECT_EQ(module.fetchByte(ip), 0x01);
    EXPECT_EQ(ip, 1);

    EXPECT_EQ(module.fetchByte(ip), 0x02);
    EXPECT_EQ(ip, 2);

    EXPECT_EQ(module.fetchByte(ip), 0x03);
    EXPECT_EQ(ip, 3);
}

TEST(BytecodeModuleTest, FetchU16) {
    BytecodeModule module;

    module.emitU16(0x1234);
    module.emitU16(0xABCD);

    uint32_t ip = 0;
    EXPECT_EQ(module.fetchU16(ip), 0x1234);
    EXPECT_EQ(ip, 2);

    EXPECT_EQ(module.fetchU16(ip), 0xABCD);
    EXPECT_EQ(ip, 4);
}

TEST(BytecodeModuleTest, FetchI16) {
    BytecodeModule module;

    module.emitI16(100);
    module.emitI16(-50);

    uint32_t ip = 0;
    EXPECT_EQ(module.fetchI16(ip), 100);
    EXPECT_EQ(ip, 2);

    EXPECT_EQ(module.fetchI16(ip), -50);
    EXPECT_EQ(ip, 4);
}

TEST(BytecodeModuleTest, PeekByte) {
    BytecodeModule module;

    module.emitByte(0x42);
    module.emitByte(0xFF);

    // Peek doesn't advance ip
    EXPECT_EQ(module.peekByte(0), 0x42);
    EXPECT_EQ(module.peekByte(0), 0x42);  // Still 0x42
    EXPECT_EQ(module.peekByte(1), 0xFF);
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST(BytecodeModuleTest, IsValidOffset) {
    BytecodeModule module;

    module.emitByte(0x01);
    module.emitByte(0x02);
    module.emitByte(0x03);

    EXPECT_TRUE(module.isValidOffset(0));
    EXPECT_TRUE(module.isValidOffset(1));
    EXPECT_TRUE(module.isValidOffset(2));
    EXPECT_FALSE(module.isValidOffset(3));
    EXPECT_FALSE(module.isValidOffset(100));
}

TEST(BytecodeModuleTest, IsValidFunctionIndex) {
    BytecodeModule module;

    module.addFunction("func1", 0, 10, 0, 5);
    module.addFunction("func2", 10, 5, 1, 3);

    EXPECT_TRUE(module.isValidFunctionIndex(0));
    EXPECT_TRUE(module.isValidFunctionIndex(1));
    EXPECT_FALSE(module.isValidFunctionIndex(2));
    EXPECT_FALSE(module.isValidFunctionIndex(100));
}

TEST(BytecodeModuleTest, VerifyValidModule) {
    BytecodeModule module;

    // Add valid bytecode
    module.emitByte(0x01);
    module.emitByte(0x02);
    module.emitByte(0x03);

    // Add valid function
    module.addFunction("test", 0, 3, 0, 2);

    std::string error;
    EXPECT_TRUE(module.verify(&error));
    EXPECT_TRUE(error.empty());
}

TEST(BytecodeModuleTest, VerifyInvalidCodeOffset) {
    BytecodeModule module;

    // Add code
    module.emitByte(0x01);

    // Add function with invalid offset (beyond code size)
    module.addFunction("bad_func", 100, 5, 0, 2);

    std::string error;
    EXPECT_FALSE(module.verify(&error));
    EXPECT_FALSE(error.empty());
}

TEST(BytecodeModuleTest, VerifyCodeExceedsBounds) {
    BytecodeModule module;

    // Add 5 bytes of code
    for (int i = 0; i < 5; i++) {
        module.emitByte(i);
    }

    // Add function that extends beyond code
    module.addFunction("bad_func", 0, 10, 0, 2);  // Length 10 > code size 5

    std::string error;
    EXPECT_FALSE(module.verify(&error));
    EXPECT_FALSE(error.empty());
}

TEST(BytecodeModuleTest, VerifySkipsNativeFunctions) {
    BytecodeModule module;

    // Native functions don't have code, so verification should skip them
    module.addNativeFunction("__print", dummy_runtime_func, 1);

    std::string error;
    EXPECT_TRUE(module.verify(&error));
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST(BytecodeModuleTest, ClearModule) {
    BytecodeModule module;

    // Populate module
    module.addIntConstant(42);
    module.addFloatConstant(3.14);
    module.addStringConstant("test");
    module.addFunction("main", 0, 10, 0, 5);
    module.emitByte(0x01);
    module.emitByte(0x02);

    EXPECT_GT(module.getIntConstantCount(), 0);
    EXPECT_GT(module.getFloatConstantCount(), 0);
    EXPECT_GT(module.getStringConstantCount(), 0);
    EXPECT_GT(module.getFunctionCount(), 0);
    EXPECT_GT(module.getCurrentOffset(), 0);

    // Clear
    module.clear();

    // Verify everything is empty
    EXPECT_EQ(module.getIntConstantCount(), 0);
    EXPECT_EQ(module.getFloatConstantCount(), 0);
    EXPECT_EQ(module.getStringConstantCount(), 0);
    EXPECT_EQ(module.getFunctionCount(), 0);
    EXPECT_EQ(module.getCurrentOffset(), 0);
}

TEST(BytecodeModuleTest, ReuseAfterClear) {
    BytecodeModule module;

    // First use
    module.addIntConstant(42);
    module.addFunction("func1", 0, 5, 0, 2);

    // Clear and reuse
    module.clear();

    uint16_t idx = module.addIntConstant(100);
    EXPECT_EQ(idx, 0);  // Should start from 0 again
    EXPECT_EQ(module.getIntConstant(idx), 100);

    uint16_t func_idx = module.addFunction("func2", 0, 10, 1, 3);
    EXPECT_EQ(func_idx, 0);  // Should start from 0 again
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(BytecodeModuleTest, SimulateSimpleFunctionCompilation) {
    BytecodeModule module;

    // Compile: fn test() -> int { return 5 + 3; }

    // Constants
    uint16_t const_5 = module.addIntConstant(5);
    uint16_t const_3 = module.addIntConstant(3);

    EXPECT_EQ(const_5, 0);
    EXPECT_EQ(const_3, 1);

    // Function
    uint32_t func_start = module.getCurrentOffset();

    // Bytecode:
    module.emitByte(0x30);              // LOAD_CONST_INT
    module.emitByte(1);                 // r1
    module.emitU16(const_5);            // pool[0]

    module.emitByte(0x30);              // LOAD_CONST_INT
    module.emitByte(2);                 // r2
    module.emitU16(const_3);            // pool[1]

    module.emitByte(0x01);              // ADD
    module.emitByte(3);                 // r3
    module.emitByte(1);                 // r1
    module.emitByte(2);                 // r2

    module.emitByte(0x44);              // RET
    module.emitByte(3);                 // r3

    uint32_t func_length = module.getCurrentOffset() - func_start;

    // Add function metadata
    module.addFunction("test", func_start, func_length, 0, 4);

    // Verify
    std::string error;
    EXPECT_TRUE(module.verify(&error)) << error;
    EXPECT_EQ(module.getFunctionCount(), 1);
    EXPECT_EQ(module.getIntConstantCount(), 2);
}

TEST(BytecodeModuleTest, GetStats) {
    BytecodeModule module;

    module.addIntConstant(1);
    module.addIntConstant(2);
    module.addFloatConstant(3.14);
    module.addStringConstant("hello");
    module.addFunction("main", 0, 10, 0, 5);

    for (int i = 0; i < 10; i++) {
        module.emitByte(i);
    }

    ModuleStats stats = module.getStats();

    EXPECT_EQ(stats.functionCount, 1);
    EXPECT_EQ(stats.intConstantCount, 2);
    EXPECT_EQ(stats.floatConstantCount, 1);
    EXPECT_EQ(stats.stringConstantCount, 1);
    EXPECT_EQ(stats.totalCodeSize, 10);
    EXPECT_GT(stats.totalMemoryUsage, 0);
}
