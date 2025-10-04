#include <gtest/gtest.h>
#include "IR/Module.hpp"
#include "IR/Arena.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

using namespace volta::ir;

// ============================================================================
// Arena Tests
// ============================================================================

TEST(ArenaTest, BasicAllocation) {
    Module module("test");
    Arena arena(1024);

    int* i = arena.allocate<int>(42);
    ASSERT_NE(i, nullptr);
    EXPECT_EQ(*i, 42);

    double* d = arena.allocate<double>(3.14);
    ASSERT_NE(d, nullptr);
    EXPECT_DOUBLE_EQ(*d, 3.14);
}

TEST(ArenaTest, MultipleAllocations) {
    Module module("test");
    Arena arena(1024);

    std::vector<int*> pointers;
    for (int i = 0; i < 100; ++i) {
        int* ptr = arena.allocate<int>(i);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(*ptr, i);
        pointers.push_back(ptr);
    }

    // Verify all values are still correct
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(*pointers[i], i);
    }
}

TEST(ArenaTest, Alignment) {
    Module module("test");
    Arena arena(1024);

    // Allocate types with different alignment requirements
    char* c = arena.allocate<char>('x');
    int* i = arena.allocate<int>(42);
    double* d = arena.allocate<double>(3.14);

    // Check alignment
    EXPECT_EQ((uintptr_t)i % alignof(int), 0);
    EXPECT_EQ((uintptr_t)d % alignof(double), 0);

    // Values should be correct
    EXPECT_EQ(*c, 'x');
    EXPECT_EQ(*i, 42);
    EXPECT_DOUBLE_EQ(*d, 3.14);
}

TEST(ArenaTest, LargeAllocation) {
    Module module("test");
    Arena arena(512);  // Small chunks

    // Allocate something larger than a chunk
    char* large = (char*)arena.allocateRaw(1024, 1);
    ASSERT_NE(large, nullptr);

    // Fill and verify
    for (int i = 0; i < 1024; ++i) {
        large[i] = (char)i;
    }
    for (int i = 0; i < 1024; ++i) {
        EXPECT_EQ(large[i], (char)i);
    }
}

TEST(ArenaTest, Reset) {
    Module module("test");
    Arena arena(1024);

    int* i1 = arena.allocate<int>(42);
    EXPECT_NE(i1, nullptr);

    size_t used1 = arena.getBytesAllocated();
    EXPECT_GT(used1, 0);

    arena.reset();

    EXPECT_EQ(arena.getBytesAllocated(), 0);

    int* i2 = arena.allocate<int>(100);
    EXPECT_NE(i2, nullptr);
    EXPECT_EQ(*i2, 100);
}

TEST(ArenaTest, Statistics) {
    Module module("test");
    Arena arena(1024);

    EXPECT_EQ(arena.getNumChunks(), 1);  // Initial chunk

    for (int i = 0; i < 10; ++i) {
        arena.allocate<int>(i);
    }

    EXPECT_GT(arena.getBytesUsed(), 0);
    EXPECT_GE(arena.getBytesAllocated(), arena.getBytesUsed());
}

TEST(ArenaTest, ZeroSizeAllocation) {
    Module module("test");
    Arena arena(1024);

    // Allocating zero bytes should still return valid pointer
    void* ptr = arena.allocateRaw(0, 1);
    EXPECT_NE(ptr, nullptr);
}

TEST(ArenaTest, HighAlignmentRequirement) {
    Module module("test");
    Arena arena(1024);

    // Allocate with 64-byte alignment
    void* ptr = arena.allocateRaw(32, 64);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ((uintptr_t)ptr % 64, 0);
}

TEST(ArenaTest, MixedSizeAllocations) {
    Module module("test");
    Arena arena(512);

    // Mix small and medium allocations
    char* small1 = arena.allocate<char>('a');
    int* medium = arena.allocate<int>(42);
    char* small2 = arena.allocate<char>('b');
    double* large = arena.allocate<double>(3.14);

    EXPECT_EQ(*small1, 'a');
    EXPECT_EQ(*medium, 42);
    EXPECT_EQ(*small2, 'b');
    EXPECT_DOUBLE_EQ(*large, 3.14);
}

TEST(ArenaTest, ResetMultipleTimes) {
    Module module("test");
    Arena arena(1024);

    for (int round = 0; round < 3; ++round) {
        int* ptr = arena.allocate<int>(round * 10);
        EXPECT_NE(ptr, nullptr);
        EXPECT_EQ(*ptr, round * 10);

        arena.reset();
        EXPECT_EQ(arena.getBytesAllocated(), 0);
        EXPECT_EQ(arena.getBytesUsed(), 0);
    }
}

TEST(ArenaTest, MultipleVeryLargeAllocations) {
    Module module("test");
    Arena arena(256);  // Small chunk size

    // Allocate multiple items larger than chunk size
    char* large1 = (char*)arena.allocateRaw(512, 1);
    char* large2 = (char*)arena.allocateRaw(1024, 1);
    char* large3 = (char*)arena.allocateRaw(768, 1);

    ASSERT_NE(large1, nullptr);
    ASSERT_NE(large2, nullptr);
    ASSERT_NE(large3, nullptr);

    // Fill and verify each allocation
    for (int i = 0; i < 512; ++i) large1[i] = 'A';
    for (int i = 0; i < 1024; ++i) large2[i] = 'B';
    for (int i = 0; i < 768; ++i) large3[i] = 'C';

    EXPECT_EQ(large1[0], 'A');
    EXPECT_EQ(large2[0], 'B');
    EXPECT_EQ(large3[0], 'C');
}

TEST(ArenaTest, MoveConstructor) {
    Module module("test");
    Arena arena1(1024);
    int* ptr1 = arena1.allocate<int>(42);
    EXPECT_EQ(*ptr1, 42);

    size_t allocated = arena1.getBytesAllocated();

    // Move construct
    Arena arena2(std::move(arena1));

    // arena2 should have the data
    EXPECT_EQ(arena2.getBytesAllocated(), allocated);
    EXPECT_EQ(*ptr1, 42);  // Pointer still valid

    // arena1 should be empty
    EXPECT_EQ(arena1.getBytesAllocated(), 0);
}

TEST(ArenaTest, MoveAssignment) {
    Module module("test");
    Arena arena1(1024);
    int* ptr1 = arena1.allocate<int>(99);
    EXPECT_EQ(*ptr1, 99);

    Arena arena2(512);
    arena2.allocate<int>(1);

    // Move assign
    arena2 = std::move(arena1);

    // arena2 should have arena1's data
    EXPECT_EQ(*ptr1, 99);

    // arena1 should be empty
    EXPECT_EQ(arena1.getBytesAllocated(), 0);
}

TEST(ArenaTest, StructAllocation) {
    Module module("test");
    struct TestStruct {
        int a;
        double b;
        char c;
        TestStruct(int x, double y, char z) : a(x), b(y), c(z) {}
    };

    Arena arena(1024);

    auto* s = arena.allocate<TestStruct>(42, 3.14, 'X');
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->a, 42);
    EXPECT_DOUBLE_EQ(s->b, 3.14);
    EXPECT_EQ(s->c, 'X');
}

// ============================================================================
// Module Creation Tests
// ============================================================================

TEST(ModuleTest, CreateModule) {
    Module module("test");

    EXPECT_EQ(module.getName(), "test");
    EXPECT_EQ(module.getNumFunctions(), 0);
    EXPECT_EQ(module.getNumGlobals(), 0);
}

TEST(ModuleTest, ModuleName) {
    Module module("my_program");

    EXPECT_EQ(module.getName(), "my_program");

    module.setName("renamed");
    EXPECT_EQ(module.getName(), "renamed");
}

// ============================================================================
// Function Management Tests
// ============================================================================

TEST(ModuleTest, CreateFunction) {
    Module module("test");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* func = module.createFunction("main", intType, {});

    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "main");
    EXPECT_EQ(func->getReturnType(), intType);
    EXPECT_EQ(module.getNumFunctions(), 1);
}

TEST(ModuleTest, GetFunction) {
    Module module("test");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* func = module.createFunction("fibonacci", intType, {intType});

    auto* found = module.getFunction("fibonacci");
    EXPECT_EQ(found, func);

    auto* notFound = module.getFunction("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST(ModuleTest, MultipleFunctions) {
    Module module("test");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto voidType = std::make_shared<IRPrimitiveType>(IRType::Kind::Void);

    auto* func1 = module.createFunction("main", intType, {});
    auto* func2 = module.createFunction("helper", voidType, {});
    auto* func3 = module.createFunction("fibonacci", intType, {intType});

    EXPECT_EQ(module.getNumFunctions(), 3);

    const auto& funcs = module.getFunctions();
    EXPECT_EQ(funcs.size(), 3);
    EXPECT_EQ(funcs[0], func1);
    EXPECT_EQ(funcs[1], func2);
    EXPECT_EQ(funcs[2], func3);
}

TEST(ModuleTest, GetOrInsertFunction) {
    Module module("test");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);

    // First call creates
    auto* func1 = module.getOrInsertFunction("test", intType, {});
    EXPECT_NE(func1, nullptr);
    EXPECT_EQ(module.getNumFunctions(), 1);

    // Second call returns existing
    auto* func2 = module.getOrInsertFunction("test", intType, {});
    EXPECT_EQ(func1, func2);
    EXPECT_EQ(module.getNumFunctions(), 1);
}

TEST(ModuleTest, RemoveFunction) {
    Module module("test");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* func = module.createFunction("test", intType, {});

    EXPECT_EQ(module.getNumFunctions(), 1);

    module.removeFunction(func);

    EXPECT_EQ(module.getNumFunctions(), 0);
    EXPECT_EQ(module.getFunction("test"), nullptr);
}

// ============================================================================
// Global Variable Tests
// ============================================================================

TEST(ModuleTest, CreateGlobal) {
    Module module("test");

    auto intType = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    auto* initializer = module.getConstantInt(42, intType);

    auto* global = module.createGlobalVariable("counter", intType, initializer, false);

    ASSERT_NE(global, nullptr);
    EXPECT_EQ(global->getName(), "counter");
    EXPECT_EQ(module.getNumGlobals(), 1);

}

TEST(ModuleTest, GetGlobal) {
    Module module("test");

    auto floatType = std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
    auto* pi = module.createGlobalVariable("PI", floatType, nullptr, true);

    auto* found = module.getGlobalVariable("PI");
    EXPECT_EQ(found, pi);

    auto* notFound = module.getGlobalVariable("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

// ============================================================================
// Type Caching Tests
// ============================================================================

TEST(ModuleTest, PrimitiveTypeCaching) {
    Module module("test");

    auto int1 = module.getIntType();
    auto int2 = module.getIntType();
    EXPECT_EQ(int1, int2);  // Same instance

    auto float1 = module.getFloatType();
    auto float2 = module.getFloatType();
    EXPECT_EQ(float1, float2);

    auto bool1 = module.getBoolType();
    auto bool2 = module.getBoolType();
    EXPECT_EQ(bool1, bool2);
}

TEST(ModuleTest, PointerTypeCaching) {
    Module module("test");

    auto intType = module.getIntType();
    auto ptr1 = module.getPointerType(intType);
    auto ptr2 = module.getPointerType(intType);

    EXPECT_EQ(ptr1, ptr2);  // Cached

    auto floatType = module.getFloatType();
    auto ptr3 = module.getPointerType(floatType);

    EXPECT_NE(ptr1, ptr3);  // Different pointee type
}

// ============================================================================
// IR Object Creation Tests
// ============================================================================

TEST(ModuleTest, CreateBasicBlock) {
    Module module("test");

    auto* bb = module.createBasicBlock("entry");
    ASSERT_NE(bb, nullptr);
    EXPECT_EQ(bb->getName(), "entry");
}

TEST(ModuleTest, CreateInstruction) {
    Module module("test");

    auto intType = module.getIntType();
    auto* lhs = module.getConstantInt(10, intType);
    auto* rhs = module.getConstantInt(20, intType);

    auto* add = module.createBinaryOp(
        Instruction::Opcode::Add, lhs, rhs, "sum"
    );

    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->getOpcode(), Instruction::Opcode::Add);

}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ModuleTest, BuildSimpleFunction) {
    Module module("test");

    // Create function: i64 @add(i64 %a, i64 %b)
    auto intType = module.getIntType();
    auto* func = module.createFunction("add", intType, {intType, intType});

    // Create entry block
    auto* entry = module.createBasicBlock("entry", func);
    // Note: createBasicBlock already adds block to function

    // Get parameters
    auto* a = func->getParam(0);
    auto* b = func->getParam(1);

    // Create add instruction
    auto* sum = module.createBinaryOp(
        Instruction::Opcode::Add, a, b, "sum"
    );
    entry->addInstruction(sum);

    // Create return
    auto* ret = module.createReturn(sum);
    entry->addInstruction(ret);

    // Verify
    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;
    EXPECT_TRUE(module.verify(&error)) << "Error: " << error;
}

TEST(ModuleTest, BuildFibonacci) {
    Module module("fibonacci");

    auto intType = module.getIntType();
    auto boolType = module.getBoolType();

    // Create function
    auto* func = module.createFunction("fibonacci", intType, {intType});

    // Create blocks
    auto* entry = module.createBasicBlock("entry", func);
    auto* baseCase = module.createBasicBlock("base_case", func);
    auto* recursiveCase = module.createBasicBlock("recursive_case", func);
    // Note: createBasicBlock already adds blocks to function

    // Entry block
    auto* n = func->getParam(0);
    auto* one = module.getConstantInt(1, intType);
    auto* cond = module.createCmp(
        Instruction::Opcode::Le, n, one, "cond"
    );
    entry->addInstruction(cond);

    auto* condBr = module.createCondBranch(cond, baseCase, recursiveCase);
    entry->addInstruction(condBr);

    // Base case
    auto* retOne = module.createReturn(one);
    baseCase->addInstruction(retOne);

    // Recursive case (simplified - just return 0 for now)
    auto* zero = module.getConstantInt(0, intType);
    auto* retZero = module.createReturn(zero);
    recursiveCase->addInstruction(retZero);

    // CFG edges are automatically added when conditional branch is added to entry block

    // Verify
    std::string error;
    EXPECT_TRUE(func->verify(&error)) << "Error: " << error;
    EXPECT_TRUE(module.verify(&error)) << "Error: " << error;

    EXPECT_EQ(func->getNumBlocks(), 3);
    EXPECT_EQ(func->getExitBlocks().size(), 2);

    // Cleanup standalone constants
}

// ============================================================================
// Verification Tests
// ============================================================================

TEST(ModuleTest, VerifyValid) {
    Module module("test");

    auto intType = module.getIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* entry = module.createBasicBlock("entry", func);
    // Note: createBasicBlock already adds block to function

    auto* constant = module.getConstantInt(42, intType);
    auto* ret = module.createReturn(constant);
    entry->addInstruction(ret);

    std::string error;
    EXPECT_TRUE(module.verify(&error)) << "Error: " << error;

}

TEST(ModuleTest, VerifyInvalidFunction) {
    Module module("test");

    auto intType = module.getIntType();
    auto* func = module.createFunction("bad", intType, {});

    auto* entry = module.createBasicBlock("entry", func);
    // Note: createBasicBlock already adds block to function

    // Missing terminator!

    std::string error;
    EXPECT_FALSE(module.verify(&error));
    EXPECT_FALSE(error.empty());
}

// ============================================================================
// Printing Tests
// ============================================================================

TEST(ModuleTest, ToString) {
    Module module("example");

    auto intType = module.getIntType();
    auto* func = module.createFunction("main", intType, {});

    std::string str = module.toString();

    EXPECT_NE(str.find("Module: example"), std::string::npos);
    EXPECT_NE(str.find("@main"), std::string::npos);
}

TEST(ModuleTest, ToStringDetailed) {
    Module module("example");

    auto intType = module.getIntType();
    module.createFunction("func1", intType, {});
    module.createFunction("func2", intType, {});

    std::string str = module.toStringDetailed();

    EXPECT_NE(str.find("Functions: 2"), std::string::npos);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST(ModuleTest, Statistics) {
    Module module("test");

    auto intType = module.getIntType();
    auto* func = module.createFunction("test", intType, {});

    auto* bb1 = module.createBasicBlock("bb1", func);
    auto* bb2 = module.createBasicBlock("bb2", func);
    // Note: createBasicBlock already adds blocks to function

    auto* c1 = module.getConstantInt(1, intType);
    auto* c2 = module.getConstantInt(2, intType);
    auto* inst1 = module.createBinaryOp(
        Instruction::Opcode::Add, c1, c2
    );
    bb1->addInstruction(inst1);

    EXPECT_EQ(module.getTotalBasicBlocks(), 2);
    EXPECT_EQ(module.getTotalInstructions(), 1);
    EXPECT_GT(module.getArenaUsage(), 0);

}

// ============================================================================
// Memory Safety Tests (Arena Allocation Benefit)
// ============================================================================

TEST(ModuleTest, NoCrashOnDestruction) {
    Module module("test");
    // This test demonstrates arena allocation benefits
    // Even with complex cross-references, no use-after-free!

    {
        Module module("test");

        auto intType = module.getIntType();
        auto* func = module.createFunction("test", intType, {intType});

        auto* bb1 = module.createBasicBlock("bb1", func);
        auto* bb2 = module.createBasicBlock("bb2", func);
        // Note: createBasicBlock already adds blocks to function

        auto* arg = func->getParam(0);

        // bb1 creates instruction
        auto* inst1 = module.createBinaryOp(
            Instruction::Opcode::Add, arg, arg
        );
        bb1->addInstruction(inst1);

        // bb2 uses instruction from bb1 (cross-block reference)
        auto* ret = module.createReturn(inst1);
        bb2->addInstruction(ret);

        // When module goes out of scope, EVERYTHING is freed safely
        // No use-after-free even though bb2's ret uses bb1's inst1!
    }

    // If we get here without crashing, arena allocation works! ✅
    SUCCEED();
}
