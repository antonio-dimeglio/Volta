#include <gtest/gtest.h>
#include "vm/VM.hpp"
#include "vm/BytecodeModule.hpp"
#include "vm/OPCode.hpp"
#include "vm/Value.hpp"
#include "gc/Object.hpp"

using namespace volta::vm;

/**
 * Direct VM Array Tests
 *
 * These tests directly create bytecode to test array operations
 * before parser/IR support is implemented.
 */

class VMArrayTest : public ::testing::Test {
protected:
    VM vm;
    std::unique_ptr<BytecodeModule> module;

    void SetUp() override {
        module = std::make_unique<BytecodeModule>();
    }

    /**
     * Helper to emit a bytecode instruction
     */
    void emit(Opcode op) {
        module->emitByte(static_cast<uint8_t>(op));
    }

    void emitByte(uint8_t byte) {
        module->emitByte(byte);
    }

    void emitShort(uint16_t val) {
        module->emitU16(val);
    }
};

// Test 1: Create an empty array
TEST_F(VMArrayTest, CreateEmptyArray) {
    // Function: fn main() -> int {
    //   arr := array_new(0)
    //   return 0
    // }

    // Load size 0
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);  // dest = r0
    emitShort(module->addIntConstant(0));  // size = 0

    // Create array
    emit(Opcode::ARRAY_NEW);
    emitByte(1);  // dest = r1
    emitByte(0);  // size_reg = r0

    // Return 0
    emit(Opcode::LOAD_CONST_INT);
    emitByte(2);  // dest = r2
    emitShort(module->addIntConstant(0));

    emit(Opcode::RET);
    emitByte(2);  // return r2

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 3);

    // Execute
    Value result = vm.execute(*module, "main");
    EXPECT_EQ(result.type, ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 0);
}

// Test 2: Create array and get length
TEST_F(VMArrayTest, ArrayLength) {
    // Function: fn main() -> int {
    //   arr := array_new(5)
    //   len := array_len(arr)
    //   return len  // should be 5
    // }

    // Load size 5
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);  // dest = r0
    emitShort(module->addIntConstant(5));

    // Create array
    emit(Opcode::ARRAY_NEW);
    emitByte(1);  // dest = r1 (array)
    emitByte(0);  // size_reg = r0

    // Get length
    emit(Opcode::ARRAY_LEN);
    emitByte(2);  // dest = r2 (length)
    emitByte(1);  // array_reg = r1

    // Return length
    emit(Opcode::RET);
    emitByte(2);  // return r2

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 3);

    // Execute
    Value result = vm.execute(*module, "main");
    EXPECT_EQ(result.type, ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 5);
}

// Test 3: Set and get array element
TEST_F(VMArrayTest, SetAndGetElement) {
    // Function: fn main() -> int {
    //   arr := array_new(3)
    //   arr[1] = 42
    //   return arr[1]  // should be 42
    // }

    // Load size 3
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);  // dest = r0
    emitShort(module->addIntConstant(3));

    // Create array
    emit(Opcode::ARRAY_NEW);
    emitByte(1);  // dest = r1 (array)
    emitByte(0);  // size_reg = r0

    // Load index 1
    emit(Opcode::LOAD_CONST_INT);
    emitByte(2);  // dest = r2
    emitShort(module->addIntConstant(1));

    // Load value 42
    emit(Opcode::LOAD_CONST_INT);
    emitByte(3);  // dest = r3
    emitShort(module->addIntConstant(42));

    // arr[1] = 42
    emit(Opcode::ARRAY_SET);
    emitByte(1);  // array_reg = r1
    emitByte(2);  // index_reg = r2
    emitByte(3);  // value_reg = r3

    // val = arr[1]
    emit(Opcode::ARRAY_GET);
    emitByte(4);  // dest = r4
    emitByte(1);  // array_reg = r1
    emitByte(2);  // index_reg = r2

    // Return val
    emit(Opcode::RET);
    emitByte(4);  // return r4

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 5);

    // Execute
    Value result = vm.execute(*module, "main");
    EXPECT_EQ(result.type, ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

// Test 4: Multiple array operations
TEST_F(VMArrayTest, MultipleOperations) {
    // Function: fn main() -> int {
    //   arr := array_new(5)
    //   arr[0] = 10
    //   arr[1] = 20
    //   arr[2] = 30
    //   return arr[0] + arr[1] + arr[2]  // should be 60
    // }

    // Load size 5
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);  // r0 = 5
    emitShort(module->addIntConstant(5));

    // Create array
    emit(Opcode::ARRAY_NEW);
    emitByte(1);  // r1 = array
    emitByte(0);

    // Load index 0
    emit(Opcode::LOAD_CONST_INT);
    emitByte(2);  // r2 = 0
    emitShort(module->addIntConstant(0));

    // Load value 10
    emit(Opcode::LOAD_CONST_INT);
    emitByte(3);  // r3 = 10
    emitShort(module->addIntConstant(10));

    // arr[0] = 10
    emit(Opcode::ARRAY_SET);
    emitByte(1);  // array = r1
    emitByte(2);  // index = r2
    emitByte(3);  // value = r3

    // Load index 1
    emit(Opcode::LOAD_CONST_INT);
    emitByte(4);  // r4 = 1
    emitShort(module->addIntConstant(1));

    // Load value 20
    emit(Opcode::LOAD_CONST_INT);
    emitByte(5);  // r5 = 20
    emitShort(module->addIntConstant(20));

    // arr[1] = 20
    emit(Opcode::ARRAY_SET);
    emitByte(1);  // array = r1
    emitByte(4);  // index = r4
    emitByte(5);  // value = r5

    // Load index 2
    emit(Opcode::LOAD_CONST_INT);
    emitByte(6);  // r6 = 2
    emitShort(module->addIntConstant(2));

    // Load value 30
    emit(Opcode::LOAD_CONST_INT);
    emitByte(7);  // r7 = 30
    emitShort(module->addIntConstant(30));

    // arr[2] = 30
    emit(Opcode::ARRAY_SET);
    emitByte(1);  // array = r1
    emitByte(6);  // index = r6
    emitByte(7);  // value = r7

    // Get arr[0]
    emit(Opcode::ARRAY_GET);
    emitByte(8);  // r8 = arr[0]
    emitByte(1);  // array = r1
    emitByte(2);  // index = r2

    // Get arr[1]
    emit(Opcode::ARRAY_GET);
    emitByte(9);  // r9 = arr[1]
    emitByte(1);  // array = r1
    emitByte(4);  // index = r4

    // Get arr[2]
    emit(Opcode::ARRAY_GET);
    emitByte(10);  // r10 = arr[2]
    emitByte(1);   // array = r1
    emitByte(6);   // index = r6

    // sum1 = arr[0] + arr[1]
    emit(Opcode::ADD);
    emitByte(11);  // r11 = sum1
    emitByte(8);   // r8
    emitByte(9);   // r9

    // sum2 = sum1 + arr[2]
    emit(Opcode::ADD);
    emitByte(12);  // r12 = sum2
    emitByte(11);  // r11
    emitByte(10);  // r10

    // Return sum2
    emit(Opcode::RET);
    emitByte(12);

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 13);

    // Execute
    Value result = vm.execute(*module, "main");
    EXPECT_EQ(result.type, ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 60);
}

// Test 5: Out of bounds access (should throw)
TEST_F(VMArrayTest, OutOfBoundsAccess) {
    // Function: fn main() -> int {
    //   arr := array_new(3)
    //   return arr[5]  // should throw
    // }

    // Load size 3
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);
    emitShort(module->addIntConstant(3));

    // Create array
    emit(Opcode::ARRAY_NEW);
    emitByte(1);
    emitByte(0);

    // Load index 5 (out of bounds)
    emit(Opcode::LOAD_CONST_INT);
    emitByte(2);
    emitShort(module->addIntConstant(5));

    // Try to access arr[5]
    emit(Opcode::ARRAY_GET);
    emitByte(3);
    emitByte(1);
    emitByte(2);

    // Return
    emit(Opcode::RET);
    emitByte(3);

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 4);

    // Execute - should throw
    EXPECT_THROW(vm.execute(*module, "main"), std::runtime_error);
}

// Test 6: Negative index (should throw)
TEST_F(VMArrayTest, NegativeIndex) {
    // Function: fn main() -> int {
    //   arr := array_new(3)
    //   return arr[-1]  // should throw
    // }

    // Load size 3
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);
    emitShort(module->addIntConstant(3));

    // Create array
    emit(Opcode::ARRAY_NEW);
    emitByte(1);
    emitByte(0);

    // Load index -1
    emit(Opcode::LOAD_CONST_INT);
    emitByte(2);
    emitShort(module->addIntConstant(-1));

    // Try to access arr[-1]
    emit(Opcode::ARRAY_GET);
    emitByte(3);
    emitByte(1);
    emitByte(2);

    // Return
    emit(Opcode::RET);
    emitByte(3);

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 4);

    // Execute - should throw
    EXPECT_THROW(vm.execute(*module, "main"), std::runtime_error);
}

// Test 7: Array with GC stress test
TEST_F(VMArrayTest, MultipleArraysGCStress) {
    // Function: fn main() -> int {
    //   arr1 := array_new(100)
    //   arr2 := array_new(100)
    //   arr3 := array_new(100)
    //   arr1[50] = 123
    //   return arr1[50]
    // }

    // Create arr1 (size 100)
    emit(Opcode::LOAD_CONST_INT);
    emitByte(0);
    emitShort(module->addIntConstant(100));

    emit(Opcode::ARRAY_NEW);
    emitByte(1);  // r1 = arr1
    emitByte(0);

    // Create arr2 (size 100)
    emit(Opcode::LOAD_CONST_INT);
    emitByte(2);
    emitShort(module->addIntConstant(100));

    emit(Opcode::ARRAY_NEW);
    emitByte(3);  // r3 = arr2
    emitByte(2);

    // Create arr3 (size 100)
    emit(Opcode::LOAD_CONST_INT);
    emitByte(4);
    emitShort(module->addIntConstant(100));

    emit(Opcode::ARRAY_NEW);
    emitByte(5);  // r5 = arr3
    emitByte(4);

    // Load index 50
    emit(Opcode::LOAD_CONST_INT);
    emitByte(6);
    emitShort(module->addIntConstant(50));

    // Load value 123
    emit(Opcode::LOAD_CONST_INT);
    emitByte(7);
    emitShort(module->addIntConstant(123));

    // arr1[50] = 123
    emit(Opcode::ARRAY_SET);
    emitByte(1);  // array = r1
    emitByte(6);  // index = r6
    emitByte(7);  // value = r7

    // Get arr1[50]
    emit(Opcode::ARRAY_GET);
    emitByte(8);  // r8 = arr1[50]
    emitByte(1);  // array = r1
    emitByte(6);  // index = r6

    // Return arr1[50]
    emit(Opcode::RET);
    emitByte(8);

    // Register function
    uint32_t codeLength = module->getCurrentOffset();
    module->addFunction("main", 0, codeLength, 0, 9);

    // Execute
    Value result = vm.execute(*module, "main");
    EXPECT_EQ(result.type, ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 123);
}
