#include <gtest/gtest.h>
#include "MIR/EscapeAnalysis.hpp"
#include "MIR/MIR.hpp"
#include "Type/Type.hpp"

using namespace MIR;

// ============================================================================
// Helper Functions for Creating Test MIR
// ============================================================================

// Create a simple i32 type
const Type::Type* makeI32Type() {
    static Type::PrimitiveType i32Type(Type::PrimitiveKind::I32);
    return &i32Type;
}

// Create a simple f64 type
const Type::Type* makeF64Type() {
    static Type::PrimitiveType f64Type(Type::PrimitiveKind::F64);
    return &f64Type;
}

// Create a pointer type
const Type::Type* makePtrType() {
    static Type::PointerType ptrType(makeI32Type());
    return &ptrType;
}

// Create a small struct type (Point: {x: f64, y: f64}) = 16 bytes
const Type::Type* makeSmallStructType() {
    static std::vector<Type::FieldInfo> fields = {
        {"x", makeF64Type(), true},
        {"y", makeF64Type(), true}
    };
    static Type::StructType pointType("Point", fields);
    return &pointType;
}

// Create a large array type ([i32; 100]) = 400 bytes
const Type::Type* makeLargeArrayType() {
    static Type::ArrayType arrType(makeI32Type(), {100});
    return &arrType;
}

// ============================================================================
// EscapeInfo Tests
// ============================================================================

TEST(EscapeInfoTest, Constructor) {
    EscapeInfo info(64);
    EXPECT_EQ(info.getSizeThreshold(), 64);
}

TEST(EscapeInfoTest, AddAllocation) {
    EscapeInfo info(64);
    info.addAllocation("%ptr", 16, makeSmallStructType());

    const AllocationInfo* allocInfo = info.getAllocationInfo("%ptr");
    ASSERT_NE(allocInfo, nullptr);
    EXPECT_EQ(allocInfo->allocaName, "%ptr");
    EXPECT_EQ(allocInfo->size, 16);
    EXPECT_EQ(allocInfo->status, EscapeStatus::Unknown);
}

TEST(EscapeInfoTest, MarkEscape) {
    EscapeInfo info(64);
    info.addAllocation("%ptr", 16, makeSmallStructType());
    info.markEscape("%ptr", EscapeReason::ReturnedFromFunction);

    EXPECT_TRUE(info.escapes("%ptr"));
    EXPECT_EQ(info.getStatus("%ptr"), EscapeStatus::Escapes);

    const AllocationInfo* allocInfo = info.getAllocationInfo("%ptr");
    EXPECT_EQ(allocInfo->reason, EscapeReason::ReturnedFromFunction);
}

TEST(EscapeInfoTest, MarkDoesNotEscape) {
    EscapeInfo info(64);
    info.addAllocation("%ptr", 16, makeSmallStructType());
    info.markDoesNotEscape("%ptr");

    EXPECT_FALSE(info.escapes("%ptr"));
    EXPECT_EQ(info.getStatus("%ptr"), EscapeStatus::DoesNotEscape);
}

TEST(EscapeInfoTest, NonExistentValue) {
    EscapeInfo info(64);

    EXPECT_FALSE(info.escapes("%nonexistent"));
    EXPECT_EQ(info.getStatus("%nonexistent"), EscapeStatus::Unknown);
    EXPECT_EQ(info.getAllocationInfo("%nonexistent"), nullptr);
}

TEST(EscapeInfoTest, Clear) {
    EscapeInfo info(64);
    info.addAllocation("%ptr1", 16, makeSmallStructType());
    info.addAllocation("%ptr2", 8, makeI32Type());

    info.clear();

    EXPECT_EQ(info.getAllocationInfo("%ptr1"), nullptr);
    EXPECT_EQ(info.getAllocationInfo("%ptr2"), nullptr);
}

// ============================================================================
// EscapeAnalyzer Tests - Size Checking
// ============================================================================

TEST(EscapeAnalyzerTest, SmallAllocationDoesNotEscapeBySize) {
    // Small allocation (16 bytes) should not escape due to size
    EscapeInfo info(64);  // Threshold: 64 bytes
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // Create pointer to small struct type
    static Type::PointerType ptrToPoint(makeSmallStructType());

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", &ptrToPoint),
                      {});
    entry.addInstruction(allocaInst);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    analyzer.analyze(func);

    // Should be registered but not escape due to size
    const AllocationInfo* allocInfo = info.getAllocationInfo("%ptr");
    ASSERT_NE(allocInfo, nullptr);
    EXPECT_EQ(allocInfo->size, 16);
    // Note: Will escape if returned, but not due to size alone
}

TEST(EscapeAnalyzerTest, LargeAllocationEscapes) {
    // Large allocation (400 bytes) should escape due to size
    EscapeInfo info(64);  // Threshold: 64 bytes
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // Create pointer to large array type
    static Type::PointerType ptrToLargeArray(makeLargeArrayType());

    // %arr = alloca [i32; 100], 400
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%arr", &ptrToLargeArray),
                      {});
    entry.addInstruction(allocaInst);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    analyzer.analyze(func);

    const AllocationInfo* allocInfo = info.getAllocationInfo("%arr");
    ASSERT_NE(allocInfo, nullptr);
    EXPECT_TRUE(info.escapes("%arr"));
    EXPECT_EQ(allocInfo->reason, EscapeReason::TooLarge);
}

// ============================================================================
// EscapeAnalyzer Tests - Return Escapes
// ============================================================================

TEST(EscapeAnalyzerTest, ReturnedPointerEscapes) {
    // Pointer returned from function should escape
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makePtrType());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // ret %ptr
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%ptr", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_TRUE(info.escapes("%ptr"));

    const AllocationInfo* allocInfo = info.getAllocationInfo("%ptr");
    EXPECT_EQ(allocInfo->reason, EscapeReason::ReturnedFromFunction);
}

TEST(EscapeAnalyzerTest, NotReturnedDoesNotEscape) {
    // Pointer not returned should not escape (if no other reasons)
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // ret 0 (not returning %ptr)
    entry.setTerminator(Terminator::makeReturn(Value::makeConstantInt(0, makeI32Type())));
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_FALSE(info.escapes("%ptr"));
}

// ============================================================================
// EscapeAnalyzer Tests - Function Call Escapes
// ============================================================================

TEST(EscapeAnalyzerTest, PassedToFunctionEscapes) {
    // Pointer passed to function should escape (conservative)
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // call @external(%ptr)
    Instruction call(Opcode::Call,
                    Value::makeLocal("%result", makeI32Type()),
                    {Value::makeLocal("%ptr", makePtrType())});
    call.callTarget = "external";
    entry.addInstruction(call);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_TRUE(info.escapes("%ptr"));

    const AllocationInfo* allocInfo = info.getAllocationInfo("%ptr");
    EXPECT_EQ(allocInfo->reason, EscapeReason::PassedToFunction);
}

// ============================================================================
// EscapeAnalyzer Tests - Store Escapes
// ============================================================================

TEST(EscapeAnalyzerTest, StoredToHeapEscapes) {
    // Pointer stored to heap location should escape
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %local = alloca Point, 16
    Instruction localAlloca(Opcode::Alloca,
                           Value::makeLocal("%local", makePtrType()),
                           {});
    entry.addInstruction(localAlloca);

    // %heap = halloca Object, 100 (assume this is marked as heap)
    Instruction heapAlloca(Opcode::HAlloca,
                          Value::makeLocal("%heap", makePtrType()),
                          {});
    entry.addInstruction(heapAlloca);

    // %field = getfieldptr %heap, 0
    Instruction gfp(Opcode::GetFieldPtr,
                   Value::makeLocal("%field", makePtrType()),
                   {Value::makeLocal("%heap", makePtrType()),
                    Value::makeConstantInt(0, makeI32Type())});
    entry.addInstruction(gfp);

    // store %local, %field
    Instruction store(Opcode::Store,
                     Value(),
                     {Value::makeLocal("%local", makePtrType()),
                      Value::makeLocal("%field", makePtrType())});
    entry.addInstruction(store);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_TRUE(info.escapes("%local"));

    const AllocationInfo* allocInfo = info.getAllocationInfo("%local");
    EXPECT_EQ(allocInfo->reason, EscapeReason::StoredToHeap);
}

// ============================================================================
// EscapeAnalyzer Tests - Derived Pointers
// ============================================================================

TEST(EscapeAnalyzerTest, DerivedPointerEscapeCausesRootToEscape) {
    // If derived pointer escapes, root allocation should escape
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makePtrType());
    BasicBlock entry("entry");

    // %array = alloca [i32; 10], 40
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%array", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // %elem = getelementptr %array, 5
    Instruction gep(Opcode::GetElementPtr,
                   Value::makeLocal("%elem", makePtrType()),
                   {Value::makeLocal("%array", makePtrType()),
                    Value::makeConstantInt(5, makeI32Type())});
    entry.addInstruction(gep);

    // ret %elem (derived pointer escapes)
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%elem", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    // Both %elem and %array should escape
    EXPECT_TRUE(info.escapes("%elem"));
    EXPECT_TRUE(info.escapes("%array"));

    const AllocationInfo* allocInfo = info.getAllocationInfo("%array");
    EXPECT_EQ(allocInfo->reason, EscapeReason::DerivedPointerEscapes);
}

// ============================================================================
// AllocationTransformer Tests
// ============================================================================

TEST(AllocationTransformerTest, TransformStackAllocation) {
    // Non-escaping allocation should become SAlloca
    EscapeInfo info(64);
    info.addAllocation("%ptr", 16, makeSmallStructType());
    info.markDoesNotEscape("%ptr");

    AllocationTransformer transformer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    transformer.transform(func);

    // Check that Alloca was transformed to SAlloca
    EXPECT_EQ(func.blocks[0].instructions[0].opcode, Opcode::SAlloca);
}

TEST(AllocationTransformerTest, TransformHeapAllocation) {
    // Escaping allocation should become HAlloca
    EscapeInfo info(64);
    info.addAllocation("%ptr", 16, makeSmallStructType());
    info.markEscape("%ptr", EscapeReason::ReturnedFromFunction);

    AllocationTransformer transformer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    transformer.transform(func);

    // Check that Alloca was transformed to HAlloca
    EXPECT_EQ(func.blocks[0].instructions[0].opcode, Opcode::HAlloca);
}

// ============================================================================
// Integration Tests - Full Analysis Pipeline
// ============================================================================

TEST(IntegrationTest, SimpleLocalVariable) {
    // Local variable used only locally → Stack
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // %x_ptr = getfieldptr %ptr, 0
    Instruction gfp(Opcode::GetFieldPtr,
                   Value::makeLocal("%x_ptr", makePtrType()),
                   {Value::makeLocal("%ptr", makePtrType()),
                    Value::makeConstantInt(0, makeI32Type())});
    entry.addInstruction(gfp);

    // store 42.0, %x_ptr
    Instruction store(Opcode::Store,
                     Value(),
                     {Value::makeConstantFloat(42.0, makeF64Type()),
                      Value::makeLocal("%x_ptr", makePtrType())});
    entry.addInstruction(store);

    // ret 0
    entry.setTerminator(Terminator::makeReturn(Value::makeConstantInt(0, makeI32Type())));
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_FALSE(info.escapes("%ptr"));

    AllocationTransformer transformer(info);
    transformer.transform(func);

    EXPECT_EQ(func.blocks[0].instructions[0].opcode, Opcode::SAlloca);
}

TEST(IntegrationTest, ReturnedStructMustBeHeap) {
    // Struct returned from function → Heap
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("createPoint", {}, makePtrType());
    BasicBlock entry("entry");

    // %ptr = alloca Point, 16
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%ptr", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // ... initialize %ptr ...

    // ret %ptr
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%ptr", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_TRUE(info.escapes("%ptr"));

    AllocationTransformer transformer(info);
    transformer.transform(func);

    EXPECT_EQ(func.blocks[0].instructions[0].opcode, Opcode::HAlloca);
}

TEST(IntegrationTest, LargeArrayMustBeHeap) {
    // Large array (> 64 bytes) → Heap
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // Create pointer to large array type
    static Type::PointerType ptrToLargeArray(makeLargeArrayType());

    // %arr = alloca [i32; 100], 400
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%arr", &ptrToLargeArray),
                      {});
    entry.addInstruction(allocaInst);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    analyzer.analyze(func);

    EXPECT_TRUE(info.escapes("%arr"));

    AllocationTransformer transformer(info);
    transformer.transform(func);

    EXPECT_EQ(func.blocks[0].instructions[0].opcode, Opcode::HAlloca);
}

// ============================================================================
// Advanced Propagation Tests
// ============================================================================

TEST(PropagationTest, MultiLevelDerivedPointers) {
    // Test propagation through multiple levels of GEP
    // %base -> %derived1 -> %derived2 (escapes) => all must escape
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makePtrType());
    BasicBlock entry("entry");

    // %base = alloca [i32; 10]
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%base", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // %derived1 = getelementptr %base, 2
    Instruction gep1(Opcode::GetElementPtr,
                    Value::makeLocal("%derived1", makePtrType()),
                    {Value::makeLocal("%base", makePtrType()),
                     Value::makeConstantInt(2, makeI32Type())});
    entry.addInstruction(gep1);

    // %derived2 = getelementptr %derived1, 3
    Instruction gep2(Opcode::GetElementPtr,
                    Value::makeLocal("%derived2", makePtrType()),
                    {Value::makeLocal("%derived1", makePtrType()),
                     Value::makeConstantInt(3, makeI32Type())});
    entry.addInstruction(gep2);

    // ret %derived2 (deepest derived pointer escapes)
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%derived2", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    // All three should escape due to propagation
    EXPECT_TRUE(info.escapes("%derived2"));
    EXPECT_TRUE(info.escapes("%derived1"));
    EXPECT_TRUE(info.escapes("%base"));
}

TEST(PropagationTest, StoreLoadChain) {
    // Test propagation through store/load chain
    // %alloc1 escapes, stored to %alloc2, loaded to %val => %val should escape
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makePtrType());
    BasicBlock entry("entry");

    // %alloc1 = alloca i32
    Instruction alloc1(Opcode::Alloca,
                      Value::makeLocal("%alloc1", makePtrType()),
                      {});
    entry.addInstruction(alloc1);

    // %alloc2 = alloca ptr<i32>
    Instruction alloc2(Opcode::Alloca,
                      Value::makeLocal("%alloc2", makePtrType()),
                      {});
    entry.addInstruction(alloc2);

    // store %alloc1, %alloc2  (store escaping value to %alloc2)
    Instruction store(Opcode::Store,
                     Value(),
                     {Value::makeLocal("%alloc1", makePtrType()),
                      Value::makeLocal("%alloc2", makePtrType())});
    entry.addInstruction(store);

    // %val = load %alloc2
    Instruction load(Opcode::Load,
                    Value::makeLocal("%val", makePtrType()),
                    {Value::makeLocal("%alloc2", makePtrType())});
    entry.addInstruction(load);

    // ret %alloc1 (causes %alloc1 to escape)
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%alloc1", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    // %alloc1 escapes via return
    EXPECT_TRUE(info.escapes("%alloc1"));

    // %alloc2 should escape because it stores an escaping value
    EXPECT_TRUE(info.escapes("%alloc2"));

    // %val loads from escaping location, so it should escape too
    EXPECT_TRUE(info.escapes("%val"));
}

TEST(PropagationTest, MultipleAllocationsIndependent) {
    // Multiple independent allocations - only one escapes
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makePtrType());
    BasicBlock entry("entry");

    // %alloc1 = alloca i32
    Instruction alloc1(Opcode::Alloca,
                      Value::makeLocal("%alloc1", makePtrType()),
                      {});
    entry.addInstruction(alloc1);

    // %alloc2 = alloca i32
    Instruction alloc2(Opcode::Alloca,
                      Value::makeLocal("%alloc2", makePtrType()),
                      {});
    entry.addInstruction(alloc2);

    // %alloc3 = alloca i32
    Instruction alloc3(Opcode::Alloca,
                      Value::makeLocal("%alloc3", makePtrType()),
                      {});
    entry.addInstruction(alloc3);

    // store 42, %alloc1 (local use only)
    Instruction store1(Opcode::Store,
                      Value(),
                      {Value::makeConstantInt(42, makeI32Type()),
                       Value::makeLocal("%alloc1", makePtrType())});
    entry.addInstruction(store1);

    // store 100, %alloc3 (local use only)
    Instruction store2(Opcode::Store,
                      Value(),
                      {Value::makeConstantInt(100, makeI32Type()),
                       Value::makeLocal("%alloc3", makePtrType())});
    entry.addInstruction(store2);

    // ret %alloc2 (only alloc2 escapes)
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%alloc2", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    // Only %alloc2 should escape
    EXPECT_FALSE(info.escapes("%alloc1"));
    EXPECT_TRUE(info.escapes("%alloc2"));
    EXPECT_FALSE(info.escapes("%alloc3"));
}

TEST(PropagationTest, GetFieldPtrPropagation) {
    // Test GetFieldPtr propagation (similar to GEP but for struct fields)
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makePtrType());
    BasicBlock entry("entry");

    // %struct = alloca Point
    Instruction allocaInst(Opcode::Alloca,
                      Value::makeLocal("%struct", makePtrType()),
                      {});
    entry.addInstruction(allocaInst);

    // %x_ptr = getfieldptr %struct, 0
    Instruction gfp1(Opcode::GetFieldPtr,
                    Value::makeLocal("%x_ptr", makePtrType()),
                    {Value::makeLocal("%struct", makePtrType()),
                     Value::makeConstantInt(0, makeI32Type())});
    entry.addInstruction(gfp1);

    // %y_ptr = getfieldptr %struct, 1
    Instruction gfp2(Opcode::GetFieldPtr,
                    Value::makeLocal("%y_ptr", makePtrType()),
                    {Value::makeLocal("%struct", makePtrType()),
                     Value::makeConstantInt(1, makeI32Type())});
    entry.addInstruction(gfp2);

    // ret %x_ptr (one field pointer escapes)
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%x_ptr", makePtrType())));
    func.addBlock(entry);

    analyzer.analyze(func);

    // %x_ptr escapes directly
    EXPECT_TRUE(info.escapes("%x_ptr"));

    // %struct escapes because derived pointer escaped
    EXPECT_TRUE(info.escapes("%struct"));

    // %y_ptr escapes because base struct escaped (forward propagation)
    EXPECT_TRUE(info.escapes("%y_ptr"));
}

TEST(PropagationTest, ComplexForwardAndBackwardPropagation) {
    // Test both forward and backward propagation in same function
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %base = alloca [i32; 10]
    Instruction alloc(Opcode::Alloca,
                     Value::makeLocal("%base", makePtrType()),
                     {});
    entry.addInstruction(alloc);

    // %ptr1 = getelementptr %base, 0
    Instruction gep1(Opcode::GetElementPtr,
                    Value::makeLocal("%ptr1", makePtrType()),
                    {Value::makeLocal("%base", makePtrType()),
                     Value::makeConstantInt(0, makeI32Type())});
    entry.addInstruction(gep1);

    // %ptr2 = getelementptr %base, 1
    Instruction gep2(Opcode::GetElementPtr,
                    Value::makeLocal("%ptr2", makePtrType()),
                    {Value::makeLocal("%base", makePtrType()),
                     Value::makeConstantInt(1, makeI32Type())});
    entry.addInstruction(gep2);

    // call someFunc(%ptr1) - causes %ptr1 to escape
    Instruction call(Opcode::Call,
                    Value(),
                    {Value::makeLocal("%ptr1", makePtrType())});
    entry.addInstruction(call);

    entry.setTerminator(Terminator::makeReturnVoid());
    func.addBlock(entry);

    analyzer.analyze(func);

    // %ptr1 escapes via call
    EXPECT_TRUE(info.escapes("%ptr1"));

    // %base escapes via backward propagation from %ptr1
    EXPECT_TRUE(info.escapes("%base"));

    // %ptr2 escapes via forward propagation from %base
    EXPECT_TRUE(info.escapes("%ptr2"));
}

TEST(PropagationTest, NoEscapeThroughLocalStoreLoad) {
    // Values stored and loaded locally should not escape if container doesn't escape
    EscapeInfo info(64);
    EscapeAnalyzer analyzer(info);

    Function func("test", {}, makeI32Type());
    BasicBlock entry("entry");

    // %container = alloca i32
    Instruction alloc(Opcode::Alloca,
                     Value::makeLocal("%container", makePtrType()),
                     {});
    entry.addInstruction(alloc);

    // store 42, %container
    Instruction store(Opcode::Store,
                     Value(),
                     {Value::makeConstantInt(42, makeI32Type()),
                      Value::makeLocal("%container", makePtrType())});
    entry.addInstruction(store);

    // %val = load %container
    Instruction load(Opcode::Load,
                    Value::makeLocal("%val", makeI32Type()),
                    {Value::makeLocal("%container", makePtrType())});
    entry.addInstruction(load);

    // ret %val (returning loaded value, not pointer)
    entry.setTerminator(Terminator::makeReturn(Value::makeLocal("%val", makeI32Type())));
    func.addBlock(entry);

    analyzer.analyze(func);

    // %container should NOT escape - we're returning the value, not the pointer
    // Note: This depends on %val being a value type (i32), not a pointer
    EXPECT_FALSE(info.escapes("%container"));
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
