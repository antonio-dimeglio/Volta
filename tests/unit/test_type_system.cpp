#include <gtest/gtest.h>
#include "../helpers/test_utils.hpp"
#include "../../include/Type/Type.hpp"
#include "../../include/Type/TypeRegistry.hpp"

// Note: Do NOT use "using namespace Type;" here as it creates ambiguity
// between the namespace Type and the struct Type::Type

// ============================================================================
// Primitive Type Tests
// ============================================================================

TEST(TypeSystemTest, PrimitiveI32) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::I32);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "i32");
}

TEST(TypeSystemTest, PrimitiveI64) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::I64);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "i64");
}

TEST(TypeSystemTest, PrimitiveF32) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::F32);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "f32");
}

TEST(TypeSystemTest, PrimitiveF64) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::F64);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "f64");
}

TEST(TypeSystemTest, PrimitiveBool) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::Bool);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "bool");
}

TEST(TypeSystemTest, PrimitiveString) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::String);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "str");
}

TEST(TypeSystemTest, PrimitiveVoid) {
    Type::TypeRegistry registry;
    auto* type = registry.getPrimitive(Type::PrimitiveKind::Void);
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->toString(), "void");
}

// ============================================================================
// Type Equality Tests
// ============================================================================

TEST(TypeSystemTest, PrimitiveEquality) {
    Type::TypeRegistry registry;
    auto* i32_1 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* i32_2 = registry.getPrimitive(Type::PrimitiveKind::I32);

    EXPECT_TRUE(i32_1->equals(i32_2));
    EXPECT_TRUE(i32_2->equals(i32_1));
}

TEST(TypeSystemTest, PrimitiveInequality) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* i64 = registry.getPrimitive(Type::PrimitiveKind::I64);

    EXPECT_FALSE(i32->equals(i64));
    EXPECT_FALSE(i64->equals(i32));
}

TEST(TypeSystemTest, IntegerVsFloat) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    EXPECT_FALSE(i32->equals(f32));
}

// ============================================================================
// Integer Type Classification Tests
// ============================================================================

TEST(TypeSystemTest, SignedIntegerTypes) {
    Type::TypeRegistry registry;

    auto* i8 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::I8));
    auto* i16 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::I16));
    auto* i32 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::I32));
    auto* i64 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::I64));

    ASSERT_NE(i8, nullptr);
    ASSERT_NE(i16, nullptr);
    ASSERT_NE(i32, nullptr);
    ASSERT_NE(i64, nullptr);

    EXPECT_TRUE(i8->isSigned());
    EXPECT_TRUE(i16->isSigned());
    EXPECT_TRUE(i32->isSigned());
    EXPECT_TRUE(i64->isSigned());

    EXPECT_TRUE(i8->isInteger());
    EXPECT_TRUE(i16->isInteger());
    EXPECT_TRUE(i32->isInteger());
    EXPECT_TRUE(i64->isInteger());
}

TEST(TypeSystemTest, UnsignedIntegerTypes) {
    Type::TypeRegistry registry;

    auto* u8 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::U8));
    auto* u16 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::U16));
    auto* u32 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::U32));
    auto* u64 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::U64));

    ASSERT_NE(u8, nullptr);
    ASSERT_NE(u16, nullptr);
    ASSERT_NE(u32, nullptr);
    ASSERT_NE(u64, nullptr);

    EXPECT_TRUE(u8->isUnsigned());
    EXPECT_TRUE(u16->isUnsigned());
    EXPECT_TRUE(u32->isUnsigned());
    EXPECT_TRUE(u64->isUnsigned());

    EXPECT_TRUE(u8->isInteger());
    EXPECT_TRUE(u16->isInteger());
    EXPECT_TRUE(u32->isInteger());
    EXPECT_TRUE(u64->isInteger());
}

TEST(TypeSystemTest, FloatTypesNotInteger) {
    Type::TypeRegistry registry;

    auto* f32 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::F32));
    auto* f64 = dynamic_cast<const Type::PrimitiveType*>(registry.getPrimitive(Type::PrimitiveKind::F64));

    ASSERT_NE(f32, nullptr);
    ASSERT_NE(f64, nullptr);

    EXPECT_FALSE(f32->isInteger());
    EXPECT_FALSE(f64->isInteger());
    EXPECT_FALSE(f32->isSigned());
    EXPECT_FALSE(f64->isSigned());
    EXPECT_FALSE(f32->isUnsigned());
    EXPECT_FALSE(f64->isUnsigned());
}

// ============================================================================
// Array Type Tests
// ============================================================================

TEST(TypeSystemTest, SimpleArrayType) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* arrayType = registry.getArray(i32, {10});

    ASSERT_NE(arrayType, nullptr);
    EXPECT_EQ(arrayType->toString(), "[i32; 10]");
}

TEST(TypeSystemTest, ArrayTypeEquality) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* arr1 = registry.getArray(i32, {10});
    auto* arr2 = registry.getArray(i32, {10});

    EXPECT_TRUE(arr1->equals(arr2));
}

TEST(TypeSystemTest, ArrayTypeDifferentSizes) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* arr10 = registry.getArray(i32, {10});
    auto* arr20 = registry.getArray(i32, {20});

    EXPECT_FALSE(arr10->equals(arr20));
}

TEST(TypeSystemTest, ArrayTypeDifferentElements) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);
    auto* arr_i32 = registry.getArray(i32, {10});
    auto* arr_f32 = registry.getArray(f32, {10});

    EXPECT_FALSE(arr_i32->equals(arr_f32));
}

TEST(TypeSystemTest, NestedArrayType) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* inner = registry.getArray(i32, {5});
    auto* outer = registry.getArray(inner, {10});

    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->toString(), "[[i32; 5]; 10]");
}

// ============================================================================
// Pointer Type Tests
// ============================================================================

TEST(TypeSystemTest, SimplePointerType) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* ptrType = registry.getPointer(i32);

    ASSERT_NE(ptrType, nullptr);
    EXPECT_EQ(ptrType->toString(), "ptr i32");
}

TEST(TypeSystemTest, PointerTypeEquality) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* ptr1 = registry.getPointer(i32);
    auto* ptr2 = registry.getPointer(i32);

    EXPECT_TRUE(ptr1->equals(ptr2));
}

TEST(TypeSystemTest, PointerTypeDifferentPointee) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);
    auto* ptr_i32 = registry.getPointer(i32);
    auto* ptr_f32 = registry.getPointer(f32);

    EXPECT_FALSE(ptr_i32->equals(ptr_f32));
}

TEST(TypeSystemTest, PointerToPointer) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* ptr = registry.getPointer(i32);
    auto* ptrptr = registry.getPointer(ptr);

    ASSERT_NE(ptrptr, nullptr);
    EXPECT_EQ(ptrptr->toString(), "ptr ptr i32");
}

TEST(TypeSystemTest, OpaquePointerType) {
    Type::TypeRegistry registry;
    auto* opaque = registry.getOpaque();
    auto* ptr = registry.getPointer(opaque);

    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->toString(), "ptr opaque");
}

// ============================================================================
// Struct Type Tests
// ============================================================================

TEST(TypeSystemTest, SimpleStructType) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    std::vector<::Type::FieldInfo> fields = {
        ::Type::FieldInfo("x", f32, true),
        ::Type::FieldInfo("y", f32, true)
    };

    auto* structType = registry.registerStruct("Point", fields);
    ASSERT_NE(structType, nullptr);
    EXPECT_EQ(structType->toString(), "Point");
}

TEST(TypeSystemTest, StructFieldAccess) {
    Type::TypeRegistry registry;
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    std::vector<::Type::FieldInfo> fields = {
        ::Type::FieldInfo("x", f32, true),
        ::Type::FieldInfo("y", f32, true)
    };

    auto* structType = registry.registerStruct("Point", fields);
    ASSERT_NE(structType, nullptr);

    auto* xType = structType->getFieldType("x");
    auto* yType = structType->getFieldType("y");
    auto* zType = structType->getFieldType("z");

    ASSERT_NE(xType, nullptr);
    ASSERT_NE(yType, nullptr);
    EXPECT_EQ(zType, nullptr); // Non-existent field

    EXPECT_TRUE(xType->equals(f32));
    EXPECT_TRUE(yType->equals(f32));
}

TEST(TypeSystemTest, StructTypeEquality) {
    Type::TypeRegistry registry;
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    std::vector<::Type::FieldInfo> fields = {
        ::Type::FieldInfo("x", f32, true),
        ::Type::FieldInfo("y", f32, true)
    };

    auto* struct1 = registry.registerStruct("Point2D", fields);
    auto* struct2 = registry.getStruct("Point2D");

    ASSERT_NE(struct1, nullptr);
    ASSERT_NE(struct2, nullptr);
    EXPECT_TRUE(struct1->equals(struct2));
}

TEST(TypeSystemTest, StructTypeDifferentNames) {
    Type::TypeRegistry registry;
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    std::vector<::Type::FieldInfo> fields = {
        ::Type::FieldInfo("x", f32, true),
        ::Type::FieldInfo("y", f32, true)
    };

    auto* point = registry.registerStruct("Point", fields);
    auto* vec = registry.registerStruct("Vec2", fields);

    EXPECT_FALSE(point->equals(vec));
}

// ============================================================================
// Generic Type Tests
// ============================================================================

TEST(TypeSystemTest, SimpleGenericType) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);

    std::vector<const ::Type::Type*> params = {i32};
    auto* generic = registry.getGeneric("Array", params);

    ASSERT_NE(generic, nullptr);
    EXPECT_EQ(generic->toString(), "Array<i32>");
}

TEST(TypeSystemTest, GenericTypeEquality) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);

    std::vector<const ::Type::Type*> params = {i32};
    auto* gen1 = registry.getGeneric("Array", params);
    auto* gen2 = registry.getGeneric("Array", params);

    EXPECT_TRUE(gen1->equals(gen2));
}

TEST(TypeSystemTest, GenericTypeDifferentParams) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    std::vector<const ::Type::Type*> params1 = {i32};
    std::vector<const ::Type::Type*> params2 = {f32};

    auto* gen1 = registry.getGeneric("Array", params1);
    auto* gen2 = registry.getGeneric("Array", params2);

    EXPECT_FALSE(gen1->equals(gen2));
}

TEST(TypeSystemTest, GenericWithMultipleParams) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* str = registry.getPrimitive(Type::PrimitiveKind::String);

    std::vector<const ::Type::Type*> params = {i32, str};
    auto* generic = registry.getGeneric("HashMap", params);

    ASSERT_NE(generic, nullptr);
    EXPECT_EQ(generic->toString(), "HashMap<i32, str>");
}

// ============================================================================
// Complex Type Combinations
// ============================================================================

TEST(TypeSystemTest, ArrayOfPointers) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* ptr = registry.getPointer(i32);
    auto* arr = registry.getArray(ptr, {10});

    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(arr->toString(), "[ptr i32; 10]");
}

TEST(TypeSystemTest, PointerToArray) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* arr = registry.getArray(i32, {10});
    auto* ptr = registry.getPointer(arr);

    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->toString(), "ptr [i32; 10]");
}

TEST(TypeSystemTest, StructWithArrayFields) {
    Type::TypeRegistry registry;
    auto* i32 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* arr = registry.getArray(i32, {10});

    std::vector<Type::FieldInfo> fields = {
        Type::FieldInfo("data", arr, true),
        Type::FieldInfo("size", i32, true)
    };

    auto* structType = registry.registerStruct("Buffer", fields);
    ASSERT_NE(structType, nullptr);

    auto* dataType = structType->getFieldType("data");
    ASSERT_NE(dataType, nullptr);
    EXPECT_TRUE(dataType->equals(arr));
}

TEST(TypeSystemTest, GenericOfStruct) {
    Type::TypeRegistry registry;
    auto* f32 = registry.getPrimitive(Type::PrimitiveKind::F32);

    std::vector<::Type::FieldInfo> fields = {
        ::Type::FieldInfo("x", f32, true),
        ::Type::FieldInfo("y", f32, true)
    };

    auto* point = registry.registerStruct("PointGen", fields);
    std::vector<const ::Type::Type*> params = {point};
    auto* generic = registry.getGeneric("Array", params);

    ASSERT_NE(generic, nullptr);
    EXPECT_EQ(generic->toString(), "Array<PointGen>");
}

// ============================================================================
// Type Registry Caching Tests
// ============================================================================

TEST(TypeSystemTest, TypeRegistryCaching) {
    Type::TypeRegistry registry;

    // Same primitive type should return same pointer
    auto* i32_1 = registry.getPrimitive(Type::PrimitiveKind::I32);
    auto* i32_2 = registry.getPrimitive(Type::PrimitiveKind::I32);
    EXPECT_EQ(i32_1, i32_2);

    // Same array type should return same pointer
    auto* arr1 = registry.getArray(i32_1, {10});
    auto* arr2 = registry.getArray(i32_1, {10});
    EXPECT_EQ(arr1, arr2);
}

// ============================================================================
// Future Features (Currently Skipped)
// ============================================================================

TEST(TypeSystemTest, DISABLED_EnumType) {
    SKIP_UNIMPLEMENTED_FEATURE("enum types");
    // Will test enum type representation when implemented
}

TEST(TypeSystemTest, DISABLED_VariantType) {
    SKIP_UNIMPLEMENTED_FEATURE("variant types");
    // Will test variant type representation when implemented
}

TEST(TypeSystemTest, DISABLED_UnionType) {
    SKIP_UNIMPLEMENTED_FEATURE("union types");
    // Will test union type representation when implemented
}
