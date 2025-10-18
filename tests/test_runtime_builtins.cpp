/**
 * Comprehensive test suite for volta_builtins.h
 * Tests all builtin functions without implementing functionality
 */

#include <gtest/gtest.h>
#include "runtime/volta_builtins.h"
#include "runtime/volta_string.h"
#include "runtime/volta_array.h"
#include <cstring>
#include <sstream>

// Capture stdout for testing print functions
class CaptureStdout {
private:
    std::stringstream buffer;
    std::streambuf* old;
public:
    CaptureStdout() : old(std::cout.rdbuf(buffer.rdbuf())) {}
    ~CaptureStdout() { std::cout.rdbuf(old); }
    std::string str() const { return buffer.str(); }
};

class VoltaBuiltinsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

// ============================================================================
// Print Function Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, PrintlnDoesNotCrash) {
    VoltaString* str = volta_string_from_cstr("Hello, World!");
    ASSERT_NE(str, nullptr);

    volta_println(str);
    // Should not crash

    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, PrintlnEmptyString) {
    VoltaString* str = volta_string_from_cstr("");
    ASSERT_NE(str, nullptr);

    volta_println(str);
    // Should not crash

    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, PrintlnNullPointer) {
    // Should handle gracefully without crashing
    volta_println(nullptr);
}

TEST_F(VoltaBuiltinsTest, PrintlnMultipleStrings) {
    for (int i = 0; i < 10; i++) {
        VoltaString* str = volta_string_from_cstr("Test line");
        ASSERT_NE(str, nullptr);
        volta_println(str);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, PrintlnVariousStrings) {
    const char* strings[] = {
        "Simple",
        "With spaces",
        "With\ttabs",
        "Special chars: !@#$%^&*()",
        "Numbers: 1234567890",
        "UTF-8: café ñ 日本語",
    };

    for (const char* s : strings) {
        VoltaString* str = volta_string_from_cstr(s);
        ASSERT_NE(str, nullptr);
        volta_println(str);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, PrintDoesNotCrash) {
    VoltaString* str = volta_string_from_cstr("No newline");
    ASSERT_NE(str, nullptr);

    volta_print(str);
    // Should not crash

    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, PrintEmptyString) {
    VoltaString* str = volta_string_from_cstr("");
    ASSERT_NE(str, nullptr);

    volta_print(str);
    // Should not crash

    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, PrintNullPointer) {
    volta_print(nullptr);
    // Should handle gracefully
}

TEST_F(VoltaBuiltinsTest, PrintMultipleTimes) {
    for (int i = 0; i < 10; i++) {
        VoltaString* str = volta_string_from_cstr(".");
        ASSERT_NE(str, nullptr);
        volta_print(str);
        volta_string_free(str);
    }
}

// ============================================================================
// Panic Function Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, PanicTerminates) {
    // We can't actually test panic since it aborts
    // But we can test that it's declared correctly
    // This test is just for documentation
    SUCCEED() << "Panic function exists but cannot be tested (it aborts)";
}

// ============================================================================
// Assert Function Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, AssertTrueCondition) {
    VoltaString* msg = volta_string_from_cstr("This should not panic");
    ASSERT_NE(msg, nullptr);

    volta_assert(true, msg);
    // Should not crash or abort

    volta_string_free(msg);
}

TEST_F(VoltaBuiltinsTest, AssertTrueMultipleTimes) {
    for (int i = 0; i < 100; i++) {
        VoltaString* msg = volta_string_from_cstr("Test");
        ASSERT_NE(msg, nullptr);
        volta_assert(true, msg);
        volta_string_free(msg);
    }
}

TEST_F(VoltaBuiltinsTest, AssertFalseCondition) {
    // Cannot test this as it will abort
    // Just documenting the behavior
    SUCCEED() << "Assert with false condition would abort, cannot test";
}

// ============================================================================
// Array Length Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, ArrayLenValidArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int32_t len = volta_array_len(arr);
    EXPECT_GE(len, 0);

    volta_array_free(arr);
}

TEST_F(VoltaBuiltinsTest, ArrayLenNullPointer) {
    int32_t len = volta_array_len(nullptr);
    // Should handle gracefully, likely return 0 or -1
    EXPECT_LE(len, 0);
}

TEST_F(VoltaBuiltinsTest, ArrayLenNonEmptyArray) {
    int data[] = {1, 2, 3, 4, 5};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 5);
    ASSERT_NE(arr, nullptr);

    int32_t len = volta_array_len(arr);
    EXPECT_EQ(len, 5);

    volta_array_free(arr);
}

TEST_F(VoltaBuiltinsTest, ArrayLenAfterModification) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 42;
    for (int i = 0; i < 10; i++) {
        volta_array_push(arr, &value);
    }

    int32_t len = volta_array_len(arr);
    EXPECT_EQ(len, 10);

    volta_array_free(arr);
}

// ============================================================================
// Integer to String Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, I32ToStringZero) {
    VoltaString* str = volta_i32_to_string(0);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, I32ToStringPositive) {
    VoltaString* str = volta_i32_to_string(12345);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, I32ToStringNegative) {
    VoltaString* str = volta_i32_to_string(-6789);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, I32ToStringMinMax) {
    VoltaString* str_min = volta_i32_to_string(INT32_MIN);
    VoltaString* str_max = volta_i32_to_string(INT32_MAX);

    EXPECT_NE(str_min, nullptr);
    EXPECT_NE(str_max, nullptr);

    volta_string_free(str_min);
    volta_string_free(str_max);
}

TEST_F(VoltaBuiltinsTest, I32ToStringVariousValues) {
    std::vector<int32_t> values = {0, 1, -1, 42, -42, 1000, -1000, 999999, -999999};

    for (int32_t val : values) {
        VoltaString* str = volta_i32_to_string(val);
        EXPECT_NE(str, nullptr) << "Failed for value: " << val;
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, I64ToStringZero) {
    VoltaString* str = volta_i64_to_string(0);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, I64ToStringPositive) {
    VoltaString* str = volta_i64_to_string(1234567890123LL);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, I64ToStringNegative) {
    VoltaString* str = volta_i64_to_string(-9876543210LL);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, I64ToStringMinMax) {
    VoltaString* str_min = volta_i64_to_string(INT64_MIN);
    VoltaString* str_max = volta_i64_to_string(INT64_MAX);

    EXPECT_NE(str_min, nullptr);
    EXPECT_NE(str_max, nullptr);

    volta_string_free(str_min);
    volta_string_free(str_max);
}

TEST_F(VoltaBuiltinsTest, I64ToStringVariousValues) {
    std::vector<int64_t> values = {
        0LL, 1LL, -1LL,
        1000000000LL, -1000000000LL,
        9223372036854775807LL,  // INT64_MAX
    };

    for (int64_t val : values) {
        VoltaString* str = volta_i64_to_string(val);
        EXPECT_NE(str, nullptr);
        volta_string_free(str);
    }
}

// ============================================================================
// Float to String Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, F32ToStringZero) {
    VoltaString* str = volta_f32_to_string(0.0f);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F32ToStringPositive) {
    VoltaString* str = volta_f32_to_string(123.456f);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F32ToStringNegative) {
    VoltaString* str = volta_f32_to_string(-789.012f);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F32ToStringSpecialValues) {
    std::vector<float> values = {
        0.0f, -0.0f,
        1.0f, -1.0f,
        3.14159f,
        1e10f, 1e-10f,
        // Note: NaN and Infinity might crash if not handled
    };

    for (float val : values) {
        VoltaString* str = volta_f32_to_string(val);
        EXPECT_NE(str, nullptr);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, F64ToStringZero) {
    VoltaString* str = volta_f64_to_string(0.0);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F64ToStringPositive) {
    VoltaString* str = volta_f64_to_string(123456.789012);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F64ToStringNegative) {
    VoltaString* str = volta_f64_to_string(-987654.321098);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F64ToStringSpecialValues) {
    std::vector<double> values = {
        0.0, -0.0,
        1.0, -1.0,
        3.141592653589793,
        1e100, 1e-100,
        2.718281828459045,
    };

    for (double val : values) {
        VoltaString* str = volta_f64_to_string(val);
        EXPECT_NE(str, nullptr);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, F64ToStringVerySmall) {
    VoltaString* str = volta_f64_to_string(1e-300);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, F64ToStringVeryLarge) {
    VoltaString* str = volta_f64_to_string(1e300);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

// ============================================================================
// Bool to String Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, BoolToStringTrue) {
    VoltaString* str = volta_bool_to_string(true);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, BoolToStringFalse) {
    VoltaString* str = volta_bool_to_string(false);
    EXPECT_NE(str, nullptr);
    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, BoolToStringMultipleCalls) {
    for (int i = 0; i < 100; i++) {
        bool value = (i % 2 == 0);
        VoltaString* str = volta_bool_to_string(value);
        EXPECT_NE(str, nullptr);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, BoolToStringDifferentValues) {
    VoltaString* str_true = volta_bool_to_string(true);
    VoltaString* str_false = volta_bool_to_string(false);

    EXPECT_NE(str_true, nullptr);
    EXPECT_NE(str_false, nullptr);

    // They should not be equal
    bool equal = volta_string_equals(str_true, str_false);
    EXPECT_FALSE(equal);

    volta_string_free(str_true);
    volta_string_free(str_false);
}

// ============================================================================
// Sizeof Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, SizeofReturnsInput) {
    size_t result = volta_sizeof(sizeof(int));
    EXPECT_EQ(result, sizeof(int));
}

TEST_F(VoltaBuiltinsTest, SizeofZero) {
    size_t result = volta_sizeof(0);
    EXPECT_EQ(result, 0);
}

TEST_F(VoltaBuiltinsTest, SizeofVariousSizes) {
    std::vector<size_t> sizes = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};

    for (size_t size : sizes) {
        size_t result = volta_sizeof(size);
        EXPECT_EQ(result, size);
    }
}

TEST_F(VoltaBuiltinsTest, SizeofCommonTypes) {
    EXPECT_EQ(volta_sizeof(sizeof(char)), sizeof(char));
    EXPECT_EQ(volta_sizeof(sizeof(short)), sizeof(short));
    EXPECT_EQ(volta_sizeof(sizeof(int)), sizeof(int));
    EXPECT_EQ(volta_sizeof(sizeof(long)), sizeof(long));
    EXPECT_EQ(volta_sizeof(sizeof(float)), sizeof(float));
    EXPECT_EQ(volta_sizeof(sizeof(double)), sizeof(double));
    EXPECT_EQ(volta_sizeof(sizeof(void*)), sizeof(void*));
}

// ============================================================================
// Debug Print Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, DbgDoesNotCrash) {
    int value = 42;
    volta_dbg(&value, "int");
    // Should not crash
}

TEST_F(VoltaBuiltinsTest, DbgNullPointer) {
    volta_dbg(nullptr, "null");
    // Should handle gracefully
}

TEST_F(VoltaBuiltinsTest, DbgNullTypeName) {
    int value = 10;
    volta_dbg(&value, nullptr);
    // Should handle gracefully
}

TEST_F(VoltaBuiltinsTest, DbgVariousTypes) {
    int i = 42;
    double d = 3.14;
    char c = 'x';
    bool b = true;

    volta_dbg(&i, "int");
    volta_dbg(&d, "double");
    volta_dbg(&c, "char");
    volta_dbg(&b, "bool");
}

TEST_F(VoltaBuiltinsTest, DbgMultipleCalls) {
    for (int i = 0; i < 10; i++) {
        volta_dbg(&i, "iteration");
    }
}

TEST_F(VoltaBuiltinsTest, DbgVariousTypeNames) {
    int value = 123;
    const char* type_names[] = {
        "int",
        "i32",
        "Integer",
        "custom_type",
        "MyStruct",
        "vec<int>",
    };

    for (const char* name : type_names) {
        volta_dbg(&value, name);
    }
}

// ============================================================================
// Integration Tests - Combining Multiple Functions
// ============================================================================

TEST_F(VoltaBuiltinsTest, PrintAndConvert) {
    int32_t value = 42;
    VoltaString* str = volta_i32_to_string(value);
    ASSERT_NE(str, nullptr);

    volta_println(str);

    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, ConvertAllTypesToString) {
    // Int conversions
    VoltaString* i32_str = volta_i32_to_string(42);
    VoltaString* i64_str = volta_i64_to_string(1234567890123LL);

    // Float conversions
    VoltaString* f32_str = volta_f32_to_string(3.14f);
    VoltaString* f64_str = volta_f64_to_string(2.718281828);

    // Bool conversion
    VoltaString* bool_str = volta_bool_to_string(true);

    // All should succeed
    EXPECT_NE(i32_str, nullptr);
    EXPECT_NE(i64_str, nullptr);
    EXPECT_NE(f32_str, nullptr);
    EXPECT_NE(f64_str, nullptr);
    EXPECT_NE(bool_str, nullptr);

    // Cleanup
    volta_string_free(i32_str);
    volta_string_free(i64_str);
    volta_string_free(f32_str);
    volta_string_free(f64_str);
    volta_string_free(bool_str);
}

TEST_F(VoltaBuiltinsTest, PrintMultipleConverted) {
    for (int i = 0; i < 10; i++) {
        VoltaString* str = volta_i32_to_string(i);
        ASSERT_NE(str, nullptr);
        volta_println(str);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, AssertWithConvertedString) {
    VoltaString* msg = volta_i32_to_string(123);
    ASSERT_NE(msg, nullptr);

    volta_assert(true, msg);

    volta_string_free(msg);
}

TEST_F(VoltaBuiltinsTest, ArrayLenAndConvert) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 10;
    for (int i = 0; i < 5; i++) {
        volta_array_push(arr, &value);
    }

    int32_t len = volta_array_len(arr);
    VoltaString* len_str = volta_i32_to_string(len);
    ASSERT_NE(len_str, nullptr);

    volta_println(len_str);

    volta_string_free(len_str);
    volta_array_free(arr);
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_F(VoltaBuiltinsTest, ConvertManyIntegers) {
    for (int i = -1000; i < 1000; i++) {
        VoltaString* str = volta_i32_to_string(i);
        EXPECT_NE(str, nullptr);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, ConvertManyFloats) {
    for (int i = 0; i < 100; i++) {
        float value = static_cast<float>(i) * 0.1f;
        VoltaString* str = volta_f32_to_string(value);
        EXPECT_NE(str, nullptr);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, PrintManyStrings) {
    for (int i = 0; i < 1000; i++) {
        VoltaString* str = volta_string_from_cstr("Line");
        ASSERT_NE(str, nullptr);
        volta_println(str);
        volta_string_free(str);
    }
}

TEST_F(VoltaBuiltinsTest, MixedTypeConversions) {
    struct TestCase {
        const char* name;
        VoltaString* (*converter)();
    };

    for (int round = 0; round < 100; round++) {
        VoltaString* s1 = volta_i32_to_string(round);
        VoltaString* s2 = volta_f64_to_string(round * 1.5);
        VoltaString* s3 = volta_bool_to_string(round % 2 == 0);

        EXPECT_NE(s1, nullptr);
        EXPECT_NE(s2, nullptr);
        EXPECT_NE(s3, nullptr);

        volta_string_free(s1);
        volta_string_free(s2);
        volta_string_free(s3);
    }
}

TEST_F(VoltaBuiltinsTest, LongStringPrinting) {
    std::string long_content(10000, 'x');
    VoltaString* str = volta_string_from_cstr(long_content.c_str());
    ASSERT_NE(str, nullptr);

    volta_println(str);

    volta_string_free(str);
}

TEST_F(VoltaBuiltinsTest, RapidAssertions) {
    VoltaString* msg = volta_string_from_cstr("Fast assertion");
    ASSERT_NE(msg, nullptr);

    for (int i = 0; i < 10000; i++) {
        volta_assert(true, msg);
    }

    volta_string_free(msg);
}

TEST_F(VoltaBuiltinsTest, DbgManyValues) {
    for (int i = 0; i < 100; i++) {
        volta_dbg(&i, "counter");
    }
}

TEST_F(VoltaBuiltinsTest, SizeofStressTest) {
    for (size_t i = 1; i < 10000; i++) {
        size_t result = volta_sizeof(i);
        EXPECT_EQ(result, i);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
