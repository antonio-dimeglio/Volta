/**
 * Comprehensive test suite for volta_string.h
 * Tests all string functions without implementing functionality
 */

#include <gtest/gtest.h>
#include "runtime/volta_string.h"
#include <cstring>
#include <string>
#include <vector>

class VoltaStringTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

// ============================================================================
// String Creation Tests
// ============================================================================

TEST_F(VoltaStringTest, FromLiteralReturnsNonNull) {
    const char* str = "Hello, World!";
    VoltaString* vs = volta_string_from_literal(str, strlen(str));
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, FromLiteralEmptyString) {
    VoltaString* vs = volta_string_from_literal("", 0);
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, FromLiteralNullPointer) {
    VoltaString* vs = volta_string_from_literal(nullptr, 0);
    // May return nullptr or handle gracefully
}

TEST_F(VoltaStringTest, FromLiteralZeroLength) {
    VoltaString* vs = volta_string_from_literal("test", 0);
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, FromLiteralVariousLengths) {
    std::vector<const char*> strings = {
        "a",
        "ab",
        "abc",
        "Hello",
        "This is a longer test string",
        "UTF-8: café ñ 日本語",
    };

    for (const char* str : strings) {
        VoltaString* vs = volta_string_from_literal(str, strlen(str));
        EXPECT_NE(vs, nullptr) << "Failed for: " << str;
        volta_string_free(vs);
    }
}

TEST_F(VoltaStringTest, FromCstrReturnsNonNull) {
    VoltaString* vs = volta_string_from_cstr("Test string");
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, FromCstrEmptyString) {
    VoltaString* vs = volta_string_from_cstr("");
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, FromCstrNullPointer) {
    VoltaString* vs = volta_string_from_cstr(nullptr);
    // May return nullptr or handle gracefully
}

TEST_F(VoltaStringTest, FromCstrVariousStrings) {
    std::vector<const char*> strings = {
        "a",
        "Hello, World!",
        "1234567890",
        "Special chars: !@#$%^&*()",
        "Tabs\tand\nnewlines",
    };

    for (const char* str : strings) {
        VoltaString* vs = volta_string_from_cstr(str);
        EXPECT_NE(vs, nullptr);
        volta_string_free(vs);
    }
}

TEST_F(VoltaStringTest, WithCapacityReturnsNonNull) {
    VoltaString* vs = volta_string_with_capacity(100);
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, WithCapacityZero) {
    VoltaString* vs = volta_string_with_capacity(0);
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, WithCapacityVariousSizes) {
    std::vector<size_t> capacities = {1, 10, 100, 1000, 10000};

    for (size_t cap : capacities) {
        VoltaString* vs = volta_string_with_capacity(cap);
        EXPECT_NE(vs, nullptr) << "Failed for capacity: " << cap;
        volta_string_free(vs);
    }
}

TEST_F(VoltaStringTest, CloneReturnsNonNull) {
    VoltaString* original = volta_string_from_cstr("Original");
    ASSERT_NE(original, nullptr);

    VoltaString* clone = volta_string_clone(original);
    EXPECT_NE(clone, nullptr);

    volta_string_free(original);
    volta_string_free(clone);
}

TEST_F(VoltaStringTest, CloneNullPointer) {
    VoltaString* clone = volta_string_clone(nullptr);
    // May return nullptr or handle gracefully
}

TEST_F(VoltaStringTest, CloneIndependence) {
    VoltaString* str1 = volta_string_from_cstr("Test");
    ASSERT_NE(str1, nullptr);

    VoltaString* str2 = volta_string_clone(str1);
    ASSERT_NE(str2, nullptr);

    // They should be different objects
    EXPECT_NE(str1, str2);

    volta_string_free(str1);
    volta_string_free(str2);
}

// ============================================================================
// String Properties Tests
// ============================================================================

TEST_F(VoltaStringTest, LengthOfEmptyString) {
    VoltaString* vs = volta_string_from_cstr("");
    ASSERT_NE(vs, nullptr);

    int32_t len = volta_string_length(vs);
    EXPECT_EQ(len, 0);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, LengthOfASCIIString) {
    VoltaString* vs = volta_string_from_cstr("Hello");
    ASSERT_NE(vs, nullptr);

    int32_t len = volta_string_length(vs);
    EXPECT_EQ(len, 5);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, LengthVsByteLengthASCII) {
    const char* str = "ASCII";
    VoltaString* vs = volta_string_from_cstr(str);
    ASSERT_NE(vs, nullptr);

    int32_t char_len = volta_string_length(vs);
    size_t byte_len = volta_string_byte_length(vs);

    // For ASCII, they should be equal
    EXPECT_EQ(char_len, byte_len);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, ByteLengthOfEmptyString) {
    VoltaString* vs = volta_string_from_cstr("");
    ASSERT_NE(vs, nullptr);

    size_t byte_len = volta_string_byte_length(vs);
    EXPECT_EQ(byte_len, 0);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, ByteLengthMatchesInput) {
    const char* str = "Test string";
    VoltaString* vs = volta_string_from_cstr(str);
    ASSERT_NE(vs, nullptr);

    size_t byte_len = volta_string_byte_length(vs);
    EXPECT_EQ(byte_len, strlen(str));

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, CapacityIsReasonable) {
    VoltaString* vs = volta_string_from_cstr("Test");
    ASSERT_NE(vs, nullptr);

    size_t capacity = volta_string_capacity(vs);
    size_t byte_len = volta_string_byte_length(vs);

    // Capacity should be >= byte length
    EXPECT_GE(capacity, byte_len);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, CapacityWithPreallocated) {
    size_t requested_cap = 1000;
    VoltaString* vs = volta_string_with_capacity(requested_cap);
    ASSERT_NE(vs, nullptr);

    size_t actual_cap = volta_string_capacity(vs);
    // Should be at least what we requested
    EXPECT_GE(actual_cap, requested_cap);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, IsEmptyOnEmptyString) {
    VoltaString* vs = volta_string_from_cstr("");
    ASSERT_NE(vs, nullptr);

    bool empty = volta_string_is_empty(vs);
    EXPECT_TRUE(empty);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, IsEmptyOnNonEmptyString) {
    VoltaString* vs = volta_string_from_cstr("Not empty");
    ASSERT_NE(vs, nullptr);

    bool empty = volta_string_is_empty(vs);
    EXPECT_FALSE(empty);

    volta_string_free(vs);
}

// ============================================================================
// String Concatenation Tests
// ============================================================================

TEST_F(VoltaStringTest, ConcatTwoStrings) {
    VoltaString* a = volta_string_from_cstr("Hello");
    VoltaString* b = volta_string_from_cstr(" World");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    VoltaString* result = volta_string_concat(a, b);
    EXPECT_NE(result, nullptr);

    volta_string_free(a);
    volta_string_free(b);
    volta_string_free(result);
}

TEST_F(VoltaStringTest, ConcatWithEmptyString) {
    VoltaString* a = volta_string_from_cstr("Test");
    VoltaString* b = volta_string_from_cstr("");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    VoltaString* result = volta_string_concat(a, b);
    EXPECT_NE(result, nullptr);

    volta_string_free(a);
    volta_string_free(b);
    volta_string_free(result);
}

TEST_F(VoltaStringTest, ConcatEmptyStrings) {
    VoltaString* a = volta_string_from_cstr("");
    VoltaString* b = volta_string_from_cstr("");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    VoltaString* result = volta_string_concat(a, b);
    EXPECT_NE(result, nullptr);

    volta_string_free(a);
    volta_string_free(b);
    volta_string_free(result);
}

TEST_F(VoltaStringTest, ConcatMultipleOperations) {
    VoltaString* s1 = volta_string_from_cstr("One");
    VoltaString* s2 = volta_string_from_cstr("Two");
    VoltaString* s3 = volta_string_from_cstr("Three");
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    ASSERT_NE(s3, nullptr);

    VoltaString* temp = volta_string_concat(s1, s2);
    ASSERT_NE(temp, nullptr);

    VoltaString* result = volta_string_concat(temp, s3);
    EXPECT_NE(result, nullptr);

    volta_string_free(s1);
    volta_string_free(s2);
    volta_string_free(s3);
    volta_string_free(temp);
    volta_string_free(result);
}

TEST_F(VoltaStringTest, AppendToString) {
    VoltaString* dest = volta_string_from_cstr("Hello");
    VoltaString* src = volta_string_from_cstr(" World");
    ASSERT_NE(dest, nullptr);
    ASSERT_NE(src, nullptr);

    bool success = volta_string_append(dest, src);
    EXPECT_TRUE(success);

    volta_string_free(dest);
    volta_string_free(src);
}

TEST_F(VoltaStringTest, AppendEmptyString) {
    VoltaString* dest = volta_string_from_cstr("Test");
    VoltaString* src = volta_string_from_cstr("");
    ASSERT_NE(dest, nullptr);
    ASSERT_NE(src, nullptr);

    bool success = volta_string_append(dest, src);
    EXPECT_TRUE(success);

    volta_string_free(dest);
    volta_string_free(src);
}

TEST_F(VoltaStringTest, AppendToEmptyString) {
    VoltaString* dest = volta_string_from_cstr("");
    VoltaString* src = volta_string_from_cstr("Content");
    ASSERT_NE(dest, nullptr);
    ASSERT_NE(src, nullptr);

    bool success = volta_string_append(dest, src);
    EXPECT_TRUE(success);

    volta_string_free(dest);
    volta_string_free(src);
}

TEST_F(VoltaStringTest, AppendMultipleTimes) {
    VoltaString* dest = volta_string_from_cstr("Start");
    ASSERT_NE(dest, nullptr);

    for (int i = 0; i < 10; i++) {
        VoltaString* part = volta_string_from_cstr(".");
        ASSERT_NE(part, nullptr);

        bool success = volta_string_append(dest, part);
        EXPECT_TRUE(success);

        volta_string_free(part);
    }

    volta_string_free(dest);
}

// ============================================================================
// String Comparison Tests
// ============================================================================

TEST_F(VoltaStringTest, EqualsIdenticalStrings) {
    VoltaString* a = volta_string_from_cstr("Test");
    VoltaString* b = volta_string_from_cstr("Test");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    bool equal = volta_string_equals(a, b);
    EXPECT_TRUE(equal);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, EqualsDifferentStrings) {
    VoltaString* a = volta_string_from_cstr("Hello");
    VoltaString* b = volta_string_from_cstr("World");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    bool equal = volta_string_equals(a, b);
    EXPECT_FALSE(equal);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, EqualsEmptyStrings) {
    VoltaString* a = volta_string_from_cstr("");
    VoltaString* b = volta_string_from_cstr("");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    bool equal = volta_string_equals(a, b);
    EXPECT_TRUE(equal);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, EqualsCaseSensitive) {
    VoltaString* a = volta_string_from_cstr("Test");
    VoltaString* b = volta_string_from_cstr("test");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    bool equal = volta_string_equals(a, b);
    EXPECT_FALSE(equal) << "Should be case-sensitive";

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, CompareSameStrings) {
    VoltaString* a = volta_string_from_cstr("Equal");
    VoltaString* b = volta_string_from_cstr("Equal");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    int result = volta_string_compare(a, b);
    EXPECT_EQ(result, 0);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, CompareFirstLess) {
    VoltaString* a = volta_string_from_cstr("Apple");
    VoltaString* b = volta_string_from_cstr("Banana");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    int result = volta_string_compare(a, b);
    EXPECT_LT(result, 0);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, CompareFirstGreater) {
    VoltaString* a = volta_string_from_cstr("Zebra");
    VoltaString* b = volta_string_from_cstr("Apple");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    int result = volta_string_compare(a, b);
    EXPECT_GT(result, 0);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, CompareEmptyStrings) {
    VoltaString* a = volta_string_from_cstr("");
    VoltaString* b = volta_string_from_cstr("");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    int result = volta_string_compare(a, b);
    EXPECT_EQ(result, 0);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, CompareEmptyWithNonEmpty) {
    VoltaString* a = volta_string_from_cstr("");
    VoltaString* b = volta_string_from_cstr("Something");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    int result = volta_string_compare(a, b);
    EXPECT_LT(result, 0);

    volta_string_free(a);
    volta_string_free(b);
}

TEST_F(VoltaStringTest, ComparePrefixes) {
    VoltaString* a = volta_string_from_cstr("Test");
    VoltaString* b = volta_string_from_cstr("Testing");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    int result = volta_string_compare(a, b);
    EXPECT_LT(result, 0);

    volta_string_free(a);
    volta_string_free(b);
}

// ============================================================================
// String Conversion Tests
// ============================================================================

TEST_F(VoltaStringTest, ToCstrReturnsNonNull) {
    VoltaString* vs = volta_string_from_cstr("Test");
    ASSERT_NE(vs, nullptr);

    const char* cstr = volta_string_to_cstr(vs);
    EXPECT_NE(cstr, nullptr);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, ToCstrNullTerminated) {
    const char* original = "Hello";
    VoltaString* vs = volta_string_from_cstr(original);
    ASSERT_NE(vs, nullptr);

    const char* cstr = volta_string_to_cstr(vs);
    ASSERT_NE(cstr, nullptr);

    // Should be null-terminated
    EXPECT_EQ(cstr[strlen(original)], '\0');

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, ToCstrEmptyString) {
    VoltaString* vs = volta_string_from_cstr("");
    ASSERT_NE(vs, nullptr);

    const char* cstr = volta_string_to_cstr(vs);
    EXPECT_NE(cstr, nullptr);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, DataReturnsNonNull) {
    VoltaString* vs = volta_string_from_cstr("Data test");
    ASSERT_NE(vs, nullptr);

    const char* data = volta_string_data(vs);
    EXPECT_NE(data, nullptr);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, DataPointsToValidMemory) {
    const char* original = "Valid data";
    VoltaString* vs = volta_string_from_cstr(original);
    ASSERT_NE(vs, nullptr);

    const char* data = volta_string_data(vs);
    ASSERT_NE(data, nullptr);

    // Should be able to read the data
    size_t len = volta_string_byte_length(vs);
    EXPECT_GT(len, 0);

    volta_string_free(vs);
}

// ============================================================================
// Memory Management Tests
// ============================================================================

TEST_F(VoltaStringTest, FreeNullPointer) {
    volta_string_free(nullptr);
    // Should not crash
}

TEST_F(VoltaStringTest, FreeAfterCreation) {
    VoltaString* vs = volta_string_from_cstr("Test");
    ASSERT_NE(vs, nullptr);
    volta_string_free(vs);
    // Should not crash
}

TEST_F(VoltaStringTest, FreeManyStrings) {
    for (int i = 0; i < 1000; i++) {
        VoltaString* vs = volta_string_from_cstr("Test string");
        ASSERT_NE(vs, nullptr);
        volta_string_free(vs);
    }
}

// ============================================================================
// UTF-8 Handling Tests
// ============================================================================

TEST_F(VoltaStringTest, UTF8SimpleCharacters) {
    const char* utf8_str = "café";
    VoltaString* vs = volta_string_from_cstr(utf8_str);
    EXPECT_NE(vs, nullptr);
    volta_string_free(vs);
}

TEST_F(VoltaStringTest, UTF8VariousLanguages) {
    std::vector<const char*> utf8_strings = {
        "Hello",           // ASCII
        "Héllo",          // Latin-1 supplement
        "Привет",         // Cyrillic
        "你好",            // Chinese
        "こんにちは",        // Japanese
        "مرحبا",          // Arabic
        "🎉🎊🎈",         // Emojis
    };

    for (const char* str : utf8_strings) {
        VoltaString* vs = volta_string_from_cstr(str);
        EXPECT_NE(vs, nullptr) << "Failed for: " << str;
        volta_string_free(vs);
    }
}

TEST_F(VoltaStringTest, UTF8ByteLengthVsCharLength) {
    // "café" is 4 chars but 5 bytes (é is 2 bytes in UTF-8)
    const char* str = "café";
    VoltaString* vs = volta_string_from_cstr(str);
    ASSERT_NE(vs, nullptr);

    int32_t char_len = volta_string_length(vs);
    size_t byte_len = volta_string_byte_length(vs);

    // For this string, byte_len should be > char_len
    EXPECT_EQ(char_len, 4);
    EXPECT_EQ(byte_len, 5);

    volta_string_free(vs);
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_F(VoltaStringTest, VeryLongString) {
    std::string long_str(10000, 'x');
    VoltaString* vs = volta_string_from_cstr(long_str.c_str());
    ASSERT_NE(vs, nullptr);

    int32_t len = volta_string_length(vs);
    EXPECT_EQ(len, 10000);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, StringWithNullBytes) {
    const char data[] = {'H', 'e', 'l', '\0', 'l', 'o'};
    VoltaString* vs = volta_string_from_literal(data, 6);
    ASSERT_NE(vs, nullptr);

    size_t byte_len = volta_string_byte_length(vs);
    EXPECT_EQ(byte_len, 6);

    volta_string_free(vs);
}

TEST_F(VoltaStringTest, RepeatedConcatenations) {
    VoltaString* result = volta_string_from_cstr("");
    ASSERT_NE(result, nullptr);

    for (int i = 0; i < 100; i++) {
        VoltaString* part = volta_string_from_cstr("x");
        ASSERT_NE(part, nullptr);

        bool success = volta_string_append(result, part);
        EXPECT_TRUE(success);

        volta_string_free(part);
    }

    int32_t len = volta_string_length(result);
    EXPECT_EQ(len, 100);

    volta_string_free(result);
}

TEST_F(VoltaStringTest, CloneChain) {
    VoltaString* s1 = volta_string_from_cstr("Original");
    ASSERT_NE(s1, nullptr);

    VoltaString* s2 = volta_string_clone(s1);
    ASSERT_NE(s2, nullptr);

    VoltaString* s3 = volta_string_clone(s2);
    ASSERT_NE(s3, nullptr);

    bool all_equal = volta_string_equals(s1, s2) && volta_string_equals(s2, s3);
    EXPECT_TRUE(all_equal);

    volta_string_free(s1);
    volta_string_free(s2);
    volta_string_free(s3);
}

TEST_F(VoltaStringTest, CompareVariousStrings) {
    struct TestCase {
        const char* a;
        const char* b;
        bool should_be_equal;
    };

    std::vector<TestCase> cases = {
        {"", "", true},
        {"a", "a", true},
        {"test", "test", true},
        {"", "a", false},
        {"a", "b", false},
        {"abc", "abcd", false},
    };

    for (const auto& tc : cases) {
        VoltaString* a = volta_string_from_cstr(tc.a);
        VoltaString* b = volta_string_from_cstr(tc.b);
        ASSERT_NE(a, nullptr);
        ASSERT_NE(b, nullptr);

        bool equal = volta_string_equals(a, b);
        EXPECT_EQ(equal, tc.should_be_equal)
            << "Failed for a=\"" << tc.a << "\" b=\"" << tc.b << "\"";

        volta_string_free(a);
        volta_string_free(b);
    }
}

TEST_F(VoltaStringTest, SpecialCharacters) {
    std::vector<const char*> special_strings = {
        "\n",
        "\t",
        "\r\n",
        "\\",
        "\"",
        "\'",
        "\0",
    };

    for (const char* str : special_strings) {
        size_t len = strlen(str);
        VoltaString* vs = volta_string_from_literal(str, len);
        EXPECT_NE(vs, nullptr);
        volta_string_free(vs);
    }
}

TEST_F(VoltaStringTest, CreateManyStrings) {
    std::vector<VoltaString*> strings;

    for (int i = 0; i < 1000; i++) {
        VoltaString* vs = volta_string_from_cstr("Test");
        ASSERT_NE(vs, nullptr);
        strings.push_back(vs);
    }

    for (VoltaString* vs : strings) {
        volta_string_free(vs);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
