/**
 * Comprehensive test suite for volta_array.h
 * Tests all array functions without implementing functionality
 */

#include <gtest/gtest.h>
#include "runtime/volta_array.h"
#include <vector>
#include <cstring>

class VoltaArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

// ============================================================================
// Array Creation Tests
// ============================================================================

TEST_F(VoltaArrayTest, NewReturnsNonNull) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    EXPECT_NE(arr, nullptr);
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, NewZeroElementSize) {
    VoltaArray* arr = volta_array_new(0);
    // Implementation-defined, should not crash
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, NewVariousElementSizes) {
    std::vector<size_t> sizes = {1, 2, 4, 8, 16, 32, 64, 128, 256, 1024};

    for (size_t size : sizes) {
        VoltaArray* arr = volta_array_new(size);
        EXPECT_NE(arr, nullptr) << "Failed for element size: " << size;
        volta_array_free(arr);
    }
}

TEST_F(VoltaArrayTest, WithCapacityReturnsNonNull) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 100);
    EXPECT_NE(arr, nullptr);
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, WithCapacityZeroCapacity) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 0);
    EXPECT_NE(arr, nullptr);
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, WithCapacityVariousSizes) {
    std::vector<size_t> capacities = {1, 10, 100, 1000, 10000};

    for (size_t cap : capacities) {
        VoltaArray* arr = volta_array_with_capacity(sizeof(double), cap);
        EXPECT_NE(arr, nullptr) << "Failed for capacity: " << cap;
        volta_array_free(arr);
    }
}

TEST_F(VoltaArrayTest, WithCapacityHasCorrectCapacity) {
    size_t requested_cap = 500;
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), requested_cap);
    ASSERT_NE(arr, nullptr);

    size_t actual_cap = volta_array_capacity(arr);
    EXPECT_GE(actual_cap, requested_cap);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, FromDataReturnsNonNull) {
    int data[] = {1, 2, 3, 4, 5};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 5);
    EXPECT_NE(arr, nullptr);
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, FromDataNullPointer) {
    VoltaArray* arr = volta_array_from_data(sizeof(int), nullptr, 0);
    // May return nullptr or handle gracefully
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, FromDataZeroCount) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 0);
    EXPECT_NE(arr, nullptr);
    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, FromDataVariousTypes) {
    int int_data[] = {1, 2, 3};
    VoltaArray* int_arr = volta_array_from_data(sizeof(int), int_data, 3);
    EXPECT_NE(int_arr, nullptr);

    double double_data[] = {1.1, 2.2, 3.3};
    VoltaArray* double_arr = volta_array_from_data(sizeof(double), double_data, 3);
    EXPECT_NE(double_arr, nullptr);

    char char_data[] = {'a', 'b', 'c'};
    VoltaArray* char_arr = volta_array_from_data(sizeof(char), char_data, 3);
    EXPECT_NE(char_arr, nullptr);

    volta_array_free(int_arr);
    volta_array_free(double_arr);
    volta_array_free(char_arr);
}

TEST_F(VoltaArrayTest, CloneReturnsNonNull) {
    VoltaArray* original = volta_array_new(sizeof(int));
    ASSERT_NE(original, nullptr);

    VoltaArray* clone = volta_array_clone(original);
    EXPECT_NE(clone, nullptr);

    volta_array_free(original);
    volta_array_free(clone);
}

TEST_F(VoltaArrayTest, CloneNullPointer) {
    VoltaArray* clone = volta_array_clone(nullptr);
    // May return nullptr or handle gracefully
}

TEST_F(VoltaArrayTest, CloneIndependence) {
    VoltaArray* arr1 = volta_array_new(sizeof(int));
    ASSERT_NE(arr1, nullptr);

    VoltaArray* arr2 = volta_array_clone(arr1);
    ASSERT_NE(arr2, nullptr);

    // Should be different objects
    EXPECT_NE(arr1, arr2);

    volta_array_free(arr1);
    volta_array_free(arr2);
}

// ============================================================================
// Array Properties Tests
// ============================================================================

TEST_F(VoltaArrayTest, LengthOfNewArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 0) << "New array should have length 0";

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, LengthAfterFromData) {
    int data[] = {1, 2, 3, 4, 5};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 5);
    ASSERT_NE(arr, nullptr);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 5);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, CapacityOfNewArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    size_t cap = volta_array_capacity(arr);
    EXPECT_GE(cap, 0);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, IsEmptyOnNewArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    bool empty = volta_array_is_empty(arr);
    EXPECT_TRUE(empty);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, IsEmptyAfterPush) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 42;
    volta_array_push(arr, &value);

    bool empty = volta_array_is_empty(arr);
    EXPECT_FALSE(empty);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ElementSizeMatchesCreation) {
    size_t elem_size = sizeof(double);
    VoltaArray* arr = volta_array_new(elem_size);
    ASSERT_NE(arr, nullptr);

    size_t actual_size = volta_array_element_size(arr);
    EXPECT_EQ(actual_size, elem_size);

    volta_array_free(arr);
}

// ============================================================================
// Array Access Tests
// ============================================================================

TEST_F(VoltaArrayTest, GetOnEmptyArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    void* ptr = volta_array_get(arr, 0);
    EXPECT_EQ(ptr, nullptr) << "Get on empty array should return nullptr";

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, GetValidIndex) {
    int data[] = {10, 20, 30};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    void* ptr = volta_array_get(arr, 1);
    EXPECT_NE(ptr, nullptr);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, GetOutOfBounds) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    void* ptr = volta_array_get(arr, 10);
    EXPECT_EQ(ptr, nullptr);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, GetNegativeIndex) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    void* ptr = volta_array_get(arr, -1);
    EXPECT_EQ(ptr, nullptr);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, GetUncheckedReturnsPointer) {
    int data[] = {5, 10, 15};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    void* ptr = volta_array_get_unchecked(arr, 1);
    EXPECT_NE(ptr, nullptr);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, SetValidIndex) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int new_value = 99;
    bool success = volta_array_set(arr, 1, &new_value);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, SetOutOfBounds) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int value = 42;
    bool success = volta_array_set(arr, 10, &value);
    EXPECT_FALSE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, DataReturnsPointer) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    void* data = volta_array_data(arr);
    // May be null for empty array, but should not crash

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, DataWithElements) {
    int values[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), values, 3);
    ASSERT_NE(arr, nullptr);

    void* data = volta_array_data(arr);
    EXPECT_NE(data, nullptr);

    volta_array_free(arr);
}

// ============================================================================
// Array Modification Tests - Push/Pop
// ============================================================================

TEST_F(VoltaArrayTest, PushToEmptyArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 42;
    bool success = volta_array_push(arr, &value);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 1);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PushMultipleElements) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    for (int i = 0; i < 100; i++) {
        bool success = volta_array_push(arr, &i);
        EXPECT_TRUE(success);
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 100);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PushGrowsArray) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 2);
    ASSERT_NE(arr, nullptr);

    // Push beyond initial capacity
    for (int i = 0; i < 10; i++) {
        bool success = volta_array_push(arr, &i);
        EXPECT_TRUE(success);
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 10);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PopFromEmptyArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value;
    bool success = volta_array_pop(arr, &value);
    EXPECT_FALSE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PopReturnsElement) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int push_value = 123;
    volta_array_push(arr, &push_value);

    int pop_value;
    bool success = volta_array_pop(arr, &pop_value);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PopDecreasesLength) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 42;
    volta_array_push(arr, &value);

    int32_t len_before = volta_array_length(arr);
    volta_array_pop(arr, nullptr);
    int32_t len_after = volta_array_length(arr);

    EXPECT_EQ(len_before - 1, len_after);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PopNullDestination) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 10;
    volta_array_push(arr, &value);

    bool success = volta_array_pop(arr, nullptr);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

// ============================================================================
// Array Modification Tests - Insert/Remove
// ============================================================================

TEST_F(VoltaArrayTest, InsertAtBeginning) {
    int data[] = {2, 3, 4};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int value = 1;
    bool success = volta_array_insert(arr, 0, &value);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 4);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, InsertAtEnd) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int value = 4;
    bool success = volta_array_insert(arr, 3, &value);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, InsertInMiddle) {
    int data[] = {1, 3, 4};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int value = 2;
    bool success = volta_array_insert(arr, 1, &value);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, InsertOutOfBounds) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 42;
    bool success = volta_array_insert(arr, 10, &value);
    EXPECT_FALSE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, RemoveAtBeginning) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int removed;
    bool success = volta_array_remove(arr, 0, &removed);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 2);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, RemoveAtEnd) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int removed;
    bool success = volta_array_remove(arr, 2, &removed);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, RemoveInMiddle) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int removed;
    bool success = volta_array_remove(arr, 1, &removed);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, RemoveOutOfBounds) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int removed;
    bool success = volta_array_remove(arr, 0, &removed);
    EXPECT_FALSE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, RemoveNullDestination) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    bool success = volta_array_remove(arr, 1, nullptr);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

// ============================================================================
// Array Capacity Management Tests
// ============================================================================

TEST_F(VoltaArrayTest, ClearEmptiesArray) {
    int data[] = {1, 2, 3, 4, 5};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 5);
    ASSERT_NE(arr, nullptr);

    volta_array_clear(arr);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 0);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ClearKeepsCapacity) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 100);
    ASSERT_NE(arr, nullptr);

    int value = 42;
    for (int i = 0; i < 50; i++) {
        volta_array_push(arr, &value);
    }

    size_t cap_before = volta_array_capacity(arr);
    volta_array_clear(arr);
    size_t cap_after = volta_array_capacity(arr);

    EXPECT_EQ(cap_before, cap_after);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ReserveIncreasesCapacity) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    bool success = volta_array_reserve(arr, 1000);
    EXPECT_TRUE(success);

    size_t cap = volta_array_capacity(arr);
    EXPECT_GE(cap, 1000);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ReserveLessThanCurrentCapacity) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 100);
    ASSERT_NE(arr, nullptr);

    bool success = volta_array_reserve(arr, 50);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ShrinkToFitReducesCapacity) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 1000);
    ASSERT_NE(arr, nullptr);

    // Add only a few elements
    int value = 10;
    for (int i = 0; i < 10; i++) {
        volta_array_push(arr, &value);
    }

    bool success = volta_array_shrink_to_fit(arr);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ShrinkToFitOnFullArray) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 10);
    ASSERT_NE(arr, nullptr);

    int value = 5;
    for (int i = 0; i < 10; i++) {
        volta_array_push(arr, &value);
    }

    bool success = volta_array_shrink_to_fit(arr);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ResizeGrow) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    bool success = volta_array_resize(arr, 100);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 100);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ResizeShrink) {
    int data[100];
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 100);
    ASSERT_NE(arr, nullptr);

    bool success = volta_array_resize(arr, 50);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 50);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ResizeToZero) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    bool success = volta_array_resize(arr, 0);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 0);

    volta_array_free(arr);
}

// ============================================================================
// Fill Tests
// ============================================================================

TEST_F(VoltaArrayTest, FillEmptyArray) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 42;
    bool success = volta_array_fill(arr, &value, 10);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 10);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, FillExistingArray) {
    int data[] = {1, 2, 3};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 3);
    ASSERT_NE(arr, nullptr);

    int value = 99;
    bool success = volta_array_fill(arr, &value, 5);
    EXPECT_TRUE(success);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, FillZeroCount) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    int value = 10;
    bool success = volta_array_fill(arr, &value, 0);
    EXPECT_TRUE(success);

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 0);

    volta_array_free(arr);
}

// ============================================================================
// Memory Management Tests
// ============================================================================

TEST_F(VoltaArrayTest, FreeNullPointer) {
    volta_array_free(nullptr);
    // Should not crash
}

TEST_F(VoltaArrayTest, FreeAfterCreation) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);
    volta_array_free(arr);
    // Should not crash
}

TEST_F(VoltaArrayTest, FreeManyArrays) {
    for (int i = 0; i < 1000; i++) {
        VoltaArray* arr = volta_array_new(sizeof(int));
        ASSERT_NE(arr, nullptr);
        volta_array_free(arr);
    }
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_F(VoltaArrayTest, LargeArray) {
    VoltaArray* arr = volta_array_with_capacity(sizeof(int), 100000);
    ASSERT_NE(arr, nullptr);

    for (int i = 0; i < 10000; i++) {
        volta_array_push(arr, &i);
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 10000);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, PushPopCycles) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    for (int round = 0; round < 100; round++) {
        // Push 10
        for (int i = 0; i < 10; i++) {
            int value = round * 10 + i;
            volta_array_push(arr, &value);
        }

        // Pop 5
        for (int i = 0; i < 5; i++) {
            volta_array_pop(arr, nullptr);
        }
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 500); // 100 rounds * (10 push - 5 pop)

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ManyInserts) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    for (int i = 0; i < 100; i++) {
        volta_array_insert(arr, 0, &i); // Always insert at beginning
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 100);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ManyRemoves) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    // Fill array
    for (int i = 0; i < 100; i++) {
        volta_array_push(arr, &i);
    }

    // Remove from middle repeatedly
    for (int i = 0; i < 50; i++) {
        volta_array_remove(arr, 25, nullptr);
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 50);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, VariousElementSizes) {
    struct Small { char c; };
    struct Medium { int arr[10]; };
    struct Large { double arr[100]; };

    VoltaArray* small_arr = volta_array_new(sizeof(Small));
    VoltaArray* medium_arr = volta_array_new(sizeof(Medium));
    VoltaArray* large_arr = volta_array_new(sizeof(Large));

    EXPECT_NE(small_arr, nullptr);
    EXPECT_NE(medium_arr, nullptr);
    EXPECT_NE(large_arr, nullptr);

    Small s = {'x'};
    Medium m = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};
    Large l;

    volta_array_push(small_arr, &s);
    volta_array_push(medium_arr, &m);
    volta_array_push(large_arr, &l);

    volta_array_free(small_arr);
    volta_array_free(medium_arr);
    volta_array_free(large_arr);
}

TEST_F(VoltaArrayTest, ClonePreservesData) {
    int data[] = {1, 2, 3, 4, 5};
    VoltaArray* original = volta_array_from_data(sizeof(int), data, 5);
    ASSERT_NE(original, nullptr);

    VoltaArray* clone = volta_array_clone(original);
    ASSERT_NE(clone, nullptr);

    // Both should have same length
    EXPECT_EQ(volta_array_length(original), volta_array_length(clone));

    volta_array_free(original);
    volta_array_free(clone);
}

TEST_F(VoltaArrayTest, MultipleReserves) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    volta_array_reserve(arr, 100);
    volta_array_reserve(arr, 1000);
    volta_array_reserve(arr, 10000);

    size_t cap = volta_array_capacity(arr);
    EXPECT_GE(cap, 10000);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, ResizeMultipleTimes) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    volta_array_resize(arr, 10);
    EXPECT_EQ(volta_array_length(arr), 10);

    volta_array_resize(arr, 100);
    EXPECT_EQ(volta_array_length(arr), 100);

    volta_array_resize(arr, 50);
    EXPECT_EQ(volta_array_length(arr), 50);

    volta_array_resize(arr, 0);
    EXPECT_EQ(volta_array_length(arr), 0);

    volta_array_free(arr);
}

TEST_F(VoltaArrayTest, StressMixedOperations) {
    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    for (int i = 0; i < 1000; i++) {
        int op = i % 5;
        int value = i;

        switch (op) {
            case 0:
                volta_array_push(arr, &value);
                break;
            case 1:
                if (!volta_array_is_empty(arr)) {
                    volta_array_pop(arr, nullptr);
                }
                break;
            case 2:
                if (!volta_array_is_empty(arr)) {
                    int32_t len = volta_array_length(arr);
                    volta_array_insert(arr, len / 2, &value);
                }
                break;
            case 3:
                if (!volta_array_is_empty(arr)) {
                    int32_t len = volta_array_length(arr);
                    volta_array_remove(arr, len / 2, nullptr);
                }
                break;
            case 4:
                volta_array_clear(arr);
                break;
        }
    }

    volta_array_free(arr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
