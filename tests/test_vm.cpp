#include <gtest/gtest.h>
#include "vm/VM.hpp"
#include "vm/Value.hpp"
#include "vm/MemoryManager.hpp"
#include "vm/GC.hpp"
#include "bytecode/BytecodeCompiler.hpp"
#include "IR/IRModule.hpp"
#include "IR/IRBuilder.hpp"
#include "semantic/Type.hpp"
#include <sstream>

using namespace volta::vm;
using namespace volta::bytecode;
using namespace volta::semantic;

// ============================================================================
// Value Tests
// ============================================================================

TEST(ValueTest, MakeNull) {
    Value v = Value::makeNull();
    EXPECT_EQ(v.type, ValueType::Null);
}

TEST(ValueTest, MakeInt) {
    Value v = Value::makeInt(42);
    EXPECT_EQ(v.type, ValueType::Int);
    EXPECT_EQ(v.asInt, 42);
}

TEST(ValueTest, MakeFloat) {
    Value v = Value::makeFloat(3.14);
    EXPECT_EQ(v.type, ValueType::Float);
    EXPECT_DOUBLE_EQ(v.asFloat, 3.14);
}

TEST(ValueTest, MakeBool) {
    Value v = Value::makeBool(true);
    EXPECT_EQ(v.type, ValueType::Bool);
    EXPECT_TRUE(v.asBool);
}

TEST(ValueTest, MakeString) {
    Value v = Value::makeString(0);
    EXPECT_EQ(v.type, ValueType::String);
    EXPECT_EQ(v.asStringIndex, 0);
}

TEST(ValueTest, MakeObject) {
    void* ptr = reinterpret_cast<void*>(0x1234);
    Value v = Value::makeObject(ptr);
    EXPECT_EQ(v.type, ValueType::Object);
    EXPECT_EQ(v.asObject, ptr);
}

TEST(ValueTest, IsTruthy_Null) {
    Value v = Value::makeNull();
    EXPECT_FALSE(v.isTruthy());
}

TEST(ValueTest, IsTruthy_Bool) {
    EXPECT_TRUE(Value::makeBool(true).isTruthy());
    EXPECT_FALSE(Value::makeBool(false).isTruthy());
}

TEST(ValueTest, IsTruthy_Int) {
    EXPECT_TRUE(Value::makeInt(1).isTruthy());
    EXPECT_TRUE(Value::makeInt(-1).isTruthy());
    EXPECT_FALSE(Value::makeInt(0).isTruthy());
}

TEST(ValueTest, IsTruthy_Float) {
    EXPECT_TRUE(Value::makeFloat(1.0).isTruthy());
    EXPECT_TRUE(Value::makeFloat(-1.0).isTruthy());
    EXPECT_FALSE(Value::makeFloat(0.0).isTruthy());
}

TEST(ValueTest, IsTruthy_String) {
    Value v = Value::makeString(0);
    EXPECT_TRUE(v.isTruthy());
}

TEST(ValueTest, IsTruthy_Object) {
    Value vNull = Value::makeObject(nullptr);
    EXPECT_FALSE(vNull.isTruthy());

    void* ptr = reinterpret_cast<void*>(0x1234);
    Value vObj = Value::makeObject(ptr);
    EXPECT_TRUE(vObj.isTruthy());
}

// ============================================================================
// Value Utility Tests
// ============================================================================

TEST(ValueUtilityTest, ValueTypeToString) {
    EXPECT_EQ(valueTypeToString(ValueType::Null), "Null");
    EXPECT_EQ(valueTypeToString(ValueType::Int), "Int");
    EXPECT_EQ(valueTypeToString(ValueType::Float), "Float");
    EXPECT_EQ(valueTypeToString(ValueType::Bool), "Bool");
    EXPECT_EQ(valueTypeToString(ValueType::String), "String");
    EXPECT_EQ(valueTypeToString(ValueType::Object), "Object");
}

TEST(ValueUtilityTest, ValueToString_Int) {
    Value v = Value::makeInt(42);
    std::string str = valueToString(v);
    EXPECT_EQ(str, "42");
}

TEST(ValueUtilityTest, ValueToString_Float) {
    Value v = Value::makeFloat(3.14);
    std::string str = valueToString(v);
    EXPECT_NE(str.find("3.14"), std::string::npos);
}

TEST(ValueUtilityTest, ValueToString_Bool) {
    EXPECT_EQ(valueToString(Value::makeBool(true)), "true");
    EXPECT_EQ(valueToString(Value::makeBool(false)), "false");
}

TEST(ValueUtilityTest, ValueToString_Null) {
    Value v = Value::makeNull();
    std::string str = valueToString(v);
    EXPECT_EQ(str, "null");
}

TEST(ValueUtilityTest, ValueToString_String) {
    std::vector<std::string> pool = {"hello", "world"};
    Value v = Value::makeString(0);
    std::string str = valueToString(v, &pool);
    EXPECT_EQ(str, "\"hello\"");
}

TEST(ValueUtilityTest, ValuesEqual_SameType) {
    Value a = Value::makeInt(42);
    Value b = Value::makeInt(42);
    Value c = Value::makeInt(99);

    EXPECT_TRUE(valuesEqual(a, b));
    EXPECT_FALSE(valuesEqual(a, c));
}

TEST(ValueUtilityTest, ValuesEqual_DifferentTypes) {
    Value intVal = Value::makeInt(42);
    Value floatVal = Value::makeFloat(42.0);

    EXPECT_FALSE(valuesEqual(intVal, floatVal));
}

TEST(ValueUtilityTest, SameType) {
    Value a = Value::makeInt(1);
    Value b = Value::makeInt(2);
    Value c = Value::makeFloat(1.0);

    EXPECT_TRUE(sameType(a, b));
    EXPECT_FALSE(sameType(a, c));
}

TEST(ValueUtilityTest, PrintValue) {
    std::ostringstream out;
    Value v = Value::makeInt(42);
    printValue(out, v);

    EXPECT_EQ(out.str(), "42");
}

// ============================================================================
// VM Stack Tests
// ============================================================================

TEST(VMTest, CreateVM) {
    auto module = std::make_shared<CompiledModule>("test");

    EXPECT_NO_THROW({
        VM vm(module);
    });
}

TEST(VMTest, PushPop) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    vm.push(Value::makeInt(42));
    EXPECT_EQ(vm.stackSize(), 1);

    Value v = vm.pop();
    EXPECT_EQ(v.type, ValueType::Int);
    EXPECT_EQ(v.asInt, 42);
    EXPECT_EQ(vm.stackSize(), 0);
}

TEST(VMTest, Peek) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    vm.push(Value::makeInt(10));
    vm.push(Value::makeInt(20));

    EXPECT_EQ(vm.peek(0).asInt, 20);
    EXPECT_EQ(vm.peek(1).asInt, 10);
    EXPECT_EQ(vm.stackSize(), 2);
}

TEST(VMTest, MultipleValues) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    for (int i = 0; i < 100; i++) {
        vm.push(Value::makeInt(i));
    }

    EXPECT_EQ(vm.stackSize(), 100);

    for (int i = 99; i >= 0; i--) {
        Value v = vm.pop();
        EXPECT_EQ(v.asInt, i);
    }

    EXPECT_EQ(vm.stackSize(), 0);
}

// ============================================================================
// VM Execution Tests (Simple Bytecode)
// ============================================================================

TEST(VMExecutionTest, ExecuteConstInt) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.index = 0;
    func.parameterCount = 0;
    func.localCount = 0;
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(42);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.type, ValueType::Int);
    EXPECT_EQ(result.asInt, 42);
}

TEST(VMExecutionTest, ExecuteAddInt) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.index = 0;
    func.parameterCount = 0;
    func.localCount = 0;

    // Push 10, Push 20, Add, Return
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(10);
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(20);
    func.chunk.emitOpcode(Opcode::AddInt);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.type, ValueType::Int);
    EXPECT_EQ(result.asInt, 30);
}

TEST(VMExecutionTest, ExecuteSubInt) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.index = 0;
    func.parameterCount = 0;
    func.localCount = 0;

    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(50);
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(30);
    func.chunk.emitOpcode(Opcode::SubInt);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.asInt, 20);
}

TEST(VMExecutionTest, ExecuteMulInt) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(6);
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(7);
    func.chunk.emitOpcode(Opcode::MulInt);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.asInt, 42);
}

TEST(VMExecutionTest, ExecuteFloatArithmetic) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ConstFloat);
    func.chunk.emitFloat64(3.0);
    func.chunk.emitOpcode(Opcode::ConstFloat);
    func.chunk.emitFloat64(2.0);
    func.chunk.emitOpcode(Opcode::AddFloat);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.type, ValueType::Float);
    EXPECT_DOUBLE_EQ(result.asFloat, 5.0);
}

TEST(VMExecutionTest, ExecuteComparison) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(10);
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(20);
    func.chunk.emitOpcode(Opcode::LtInt);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.type, ValueType::Bool);
    EXPECT_TRUE(result.asBool);
}

TEST(VMExecutionTest, ExecuteLogicalAnd) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ConstBool);
    func.chunk.emitBool(true);
    func.chunk.emitOpcode(Opcode::ConstBool);
    func.chunk.emitBool(false);
    func.chunk.emitOpcode(Opcode::And);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.type, ValueType::Bool);
    EXPECT_FALSE(result.asBool);
}

TEST(VMExecutionTest, ExecuteLocalVariables) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.localCount = 2;

    // x = 42; y = x + 10; return y;
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(42);
    func.chunk.emitOpcode(Opcode::StoreLocal);
    func.chunk.emitInt32(0);  // x

    func.chunk.emitOpcode(Opcode::LoadLocal);
    func.chunk.emitInt32(0);  // x
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(10);
    func.chunk.emitOpcode(Opcode::AddInt);
    func.chunk.emitOpcode(Opcode::StoreLocal);
    func.chunk.emitInt32(1);  // y

    func.chunk.emitOpcode(Opcode::LoadLocal);
    func.chunk.emitInt32(1);  // y
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.asInt, 52);
}

TEST(VMExecutionTest, ExecuteJump) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.index = 0;
    func.parameterCount = 0;
    func.localCount = 0;

    // Jump over the 99
    size_t jumpStart = func.chunk.currentOffset();
    func.chunk.emitOpcode(Opcode::Jump);
    size_t patchOffset = func.chunk.currentOffset();
    func.chunk.emitInt32(0);  // Placeholder

    // This should be skipped
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(99);

    // Jump target
    size_t targetOffset = func.chunk.currentOffset();
    func.chunk.patchInt32(patchOffset, targetOffset - jumpStart);

    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(42);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.asInt, 42);
}

TEST(VMExecutionTest, ExecuteConditionalJump) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.index = 0;
    func.parameterCount = 0;
    func.localCount = 0;

    // if (true) return 1; else return 2;
    func.chunk.emitOpcode(Opcode::ConstBool);
    func.chunk.emitBool(true);

    size_t jumpIfFalseStart = func.chunk.currentOffset();
    func.chunk.emitOpcode(Opcode::JumpIfFalse);
    size_t elsePatch = func.chunk.currentOffset();
    func.chunk.emitInt32(0);

    // Then branch
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(1);
    func.chunk.emitOpcode(Opcode::Return);

    // Else branch
    size_t elseOffset = func.chunk.currentOffset();
    func.chunk.patchInt32(elsePatch, elseOffset - jumpIfFalseStart);
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(2);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.asInt, 1);
}

TEST(VMExecutionTest, ExecuteStringConstant) {
    auto module = std::make_shared<CompiledModule>("test");
    module->stringPool().push_back("hello");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ConstString);
    func.chunk.emitInt32(0);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    Value result = vm.executeFunction("main");

    EXPECT_EQ(result.type, ValueType::String);
    EXPECT_EQ(result.asStringIndex, 0);
}

// ============================================================================
// VM Foreign Function Interface Tests
// ============================================================================

TEST(VMFFITest, RegisterNativeFunction) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    int callCount = 0;
    vm.registerNativeFunction("test_func", [&callCount](VM& vm) {
        callCount++;
        return 0;
    });

    EXPECT_EQ(callCount, 0);
}

TEST(VMFFITest, CallNativeFunction) {
    auto module = std::make_shared<CompiledModule>("test");
    module->foreignFunctions().push_back("add_one");

    VM vm(module);

    vm.registerNativeFunction("add_one", [](VM& vm) {
        Value arg = vm.pop();
        vm.push(Value::makeInt(arg.asInt + 1));
        return 1;  // One return value
    });

    vm.push(Value::makeInt(41));
    vm.callNativeFunction(0, 1);

    Value result = vm.pop();
    EXPECT_EQ(result.asInt, 42);
}

// ============================================================================
// VM Debug Tests
// ============================================================================

TEST(VMDebugTest, SetDebugTrace) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    EXPECT_NO_THROW(vm.setDebugTrace(true));
    EXPECT_NO_THROW(vm.setDebugTrace(false));
}

TEST(VMDebugTest, DumpStack) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    vm.push(Value::makeInt(1));
    vm.push(Value::makeInt(2));

    std::ostringstream out;
    EXPECT_NO_THROW(vm.dumpStack(out));
    EXPECT_GT(out.str().size(), 0);
}

TEST(VMDebugTest, DumpCallStack) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    std::ostringstream out;
    EXPECT_NO_THROW(vm.dumpCallStack(out));
}

// ============================================================================
// Object Tests
// ============================================================================

TEST(ObjectTest, GetStructSize) {
    size_t size = getStructSize(5);  // 5 fields
    EXPECT_GT(size, sizeof(ObjectHeader));
}

TEST(ObjectTest, GetArraySize) {
    size_t size = getArraySize(10);  // 10 elements
    EXPECT_GT(size, sizeof(ObjectHeader));
}

TEST(ObjectTest, CalculateStructSize) {
    size_t size = calculateStructSize(3);
    EXPECT_GT(size, 0);
}

TEST(ObjectTest, CalculateArraySize) {
    size_t size = calculateArraySize(100);
    EXPECT_GT(size, 0);
}

// ============================================================================
// Memory Manager Tests
// ============================================================================

TEST(MemoryManagerTest, Create) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    EXPECT_NO_THROW({
        MemoryManager mm(&vm);
    });
}

TEST(MemoryManagerTest, AllocateStruct) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    MemoryManager mm(&vm);

    StructObject* obj = mm.allocateStruct(1, 3);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->header.kind, ObjectHeader::ObjectKind::Struct);
}

TEST(MemoryManagerTest, AllocateArray) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    MemoryManager mm(&vm);

    ArrayObject* arr = mm.allocateArray(10);
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(arr->header.kind, ObjectHeader::ObjectKind::Array);
    EXPECT_EQ(arr->length, 10);
}

TEST(MemoryManagerTest, AutoGC) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    MemoryManager mm(&vm);

    EXPECT_TRUE(mm.isAutoGCEnabled());

    mm.setAutoGC(false);
    EXPECT_FALSE(mm.isAutoGCEnabled());
}

TEST(MemoryManagerTest, ManualCollect) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    MemoryManager mm(&vm);

    mm.setAutoGC(false);

    // Allocate some objects
    for (int i = 0; i < 10; i++) {
        mm.allocateStruct(1, 2);
    }

    // Manual collection should not crash
    EXPECT_NO_THROW(mm.collect());
}

TEST(MemoryManagerTest, MemoryStats) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    MemoryManager mm(&vm);

    size_t before = mm.totalAllocated();

    mm.allocateStruct(1, 5);

    size_t after = mm.totalAllocated();
    EXPECT_GT(after, before);
}

TEST(MemoryManagerTest, PrintStats) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    MemoryManager mm(&vm);

    std::ostringstream out;
    EXPECT_NO_THROW(mm.printStats(out));
}

// ============================================================================
// GC Tests
// ============================================================================

TEST(GCTest, Create) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    EXPECT_NO_THROW({
        GarbageCollector gc(&vm);
    });
}

TEST(GCTest, AllocateInYoungGen) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    GarbageCollector gc(&vm);

    StructObject* obj = gc.allocateStruct(1, 2);
    ASSERT_NE(obj, nullptr);
}

TEST(GCTest, MinorCollection) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    GarbageCollector gc(&vm);

    // Allocate some young objects
    for (int i = 0; i < 10; i++) {
        gc.allocateStruct(1, 2);
    }

    EXPECT_NO_THROW(gc.collectMinor());
}

TEST(GCTest, MajorCollection) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    GarbageCollector gc(&vm);

    EXPECT_NO_THROW(gc.collectMajor());
}

TEST(GCTest, GCStats) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    GarbageCollector gc(&vm);

    const GCStats& stats = gc.stats();
    EXPECT_EQ(stats.minorCollections, 0);
    EXPECT_EQ(stats.majorCollections, 0);
}

TEST(GCTest, SetLogging) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);
    GarbageCollector gc(&vm);

    EXPECT_NO_THROW(gc.setLogging(true));
    EXPECT_NO_THROW(gc.setLogging(false));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(VMEdgeCaseTest, EmptyProgram) {
    auto module = std::make_shared<CompiledModule>("empty");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ReturnVoid);
    module->functions().push_back(std::move(func));

    VM vm(module);
    EXPECT_NO_THROW(vm.execute());
}

TEST(VMEdgeCaseTest, StackOverflow) {
    auto module = std::make_shared<CompiledModule>("test");
    VM vm(module);

    // Try to overflow stack
    EXPECT_THROW({
        for (int i = 0; i < 1000000; i++) {
            vm.push(Value::makeInt(i));
        }
    }, VM::RuntimeError);
}

TEST(VMEdgeCaseTest, DivisionByZero) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(10);
    func.chunk.emitOpcode(Opcode::ConstInt);
    func.chunk.emitInt64(0);
    func.chunk.emitOpcode(Opcode::DivInt);
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    EXPECT_THROW(vm.executeFunction("main"), VM::RuntimeError);
}

TEST(VMEdgeCaseTest, InvalidOpcode) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.chunk.code_.push_back(255);  // Invalid opcode

    module->functions().push_back(std::move(func));

    VM vm(module);
    EXPECT_THROW(vm.executeFunction("main"), VM::RuntimeError);
}

TEST(VMEdgeCaseTest, OutOfBoundsLocalAccess) {
    auto module = std::make_shared<CompiledModule>("test");

    CompiledFunction func;
    func.name = "main";
    func.localCount = 1;
    func.chunk.emitOpcode(Opcode::LoadLocal);
    func.chunk.emitInt32(999);  // Out of bounds
    func.chunk.emitOpcode(Opcode::Return);

    module->functions().push_back(std::move(func));

    VM vm(module);
    EXPECT_THROW(vm.executeFunction("main"), VM::RuntimeError);
}
