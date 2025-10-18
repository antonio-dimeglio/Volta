/**
 * Comprehensive test suite for volta_runtime.h
 * Tests runtime initialization and shutdown functions
 */

#include <gtest/gtest.h>
#include "runtime/volta_runtime.h"
#include "runtime/memory/gc.h"
#include "runtime/memory/allocator.h"
#include "runtime/volta_string.h"
#include "runtime/volta_array.h"
#include <vector>

class VoltaRuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Don't initialize in SetUp - let each test control it
    }

    void TearDown() override {
        // Don't shutdown in TearDown - let each test control it
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, InitDoesNotCrash) {
    volta_runtime_init();
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, InitMultipleTimes) {
    volta_runtime_init();
    volta_runtime_init(); // Should handle gracefully
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, InitThenShutdown) {
    for (int i = 0; i < 10; i++) {
        volta_runtime_init();
        volta_runtime_shutdown();
    }
}

// ============================================================================
// Shutdown Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, ShutdownDoesNotCrash) {
    volta_runtime_init();
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ShutdownWithoutInit) {
    // Should handle gracefully
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, MultipleShutdowns) {
    volta_runtime_init();
    volta_runtime_shutdown();
    volta_runtime_shutdown(); // Should handle gracefully
}

// ============================================================================
// Runtime Lifecycle Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, InitShutdownCycles) {
    for (int i = 0; i < 100; i++) {
        volta_runtime_init();
        volta_runtime_shutdown();
    }
}

TEST_F(VoltaRuntimeTest, InitShutdownWithDelay) {
    for (int i = 0; i < 10; i++) {
        volta_runtime_init();
        // Simulate some work
        for (volatile int j = 0; j < 1000; j++);
        volta_runtime_shutdown();
    }
}

// ============================================================================
// Integration with GC Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, InitEnablesGC) {
    volta_runtime_init();

    // GC should be usable after runtime init
    void* ptr = volta_gc_malloc(64);
    EXPECT_NE(ptr, nullptr);

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, GCAllocationsAfterInit) {
    volta_runtime_init();

    std::vector<void*> ptrs;
    for (int i = 0; i < 100; i++) {
        void* ptr = volta_gc_malloc(128);
        EXPECT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, GCCollectionAfterInit) {
    volta_runtime_init();

    for (int i = 0; i < 10; i++) {
        volta_gc_malloc(1024);
    }

    volta_gc_collect();

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, GCStatsAfterInit) {
    volta_runtime_init();

    VoltaGCStats stats;
    volta_gc_get_stats(&stats);

    // Should not crash
    EXPECT_GE(stats.total_allocations, 0);

    volta_runtime_shutdown();
}

// ============================================================================
// Integration with Allocator Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, AllocatorAfterInit) {
    volta_runtime_init();

    void* ptr = volta_alloc(256);
    EXPECT_NE(ptr, nullptr);

    volta_free(ptr);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, AllocatorZeroedAfterInit) {
    volta_runtime_init();

    uint32_t* ptr = static_cast<uint32_t*>(volta_alloc_zeroed(10, sizeof(uint32_t)));
    ASSERT_NE(ptr, nullptr);

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(ptr[i], 0u);
    }

    volta_free(ptr);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, AllocatorReallocAfterInit) {
    volta_runtime_init();

    void* ptr = volta_alloc(64);
    ASSERT_NE(ptr, nullptr);

    ptr = volta_realloc(ptr, 256);
    EXPECT_NE(ptr, nullptr);

    volta_free(ptr);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, AllocatorArrayAfterInit) {
    volta_runtime_init();

    void* ptr = volta_alloc_array(100, sizeof(int));
    EXPECT_NE(ptr, nullptr);

    volta_free(ptr);
    volta_runtime_shutdown();
}

// ============================================================================
// Integration with String Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, StringCreationAfterInit) {
    volta_runtime_init();

    VoltaString* str = volta_string_from_cstr("Hello, Runtime!");
    EXPECT_NE(str, nullptr);

    volta_string_free(str);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, StringOperationsAfterInit) {
    volta_runtime_init();

    VoltaString* s1 = volta_string_from_cstr("Hello");
    VoltaString* s2 = volta_string_from_cstr(" World");
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);

    VoltaString* result = volta_string_concat(s1, s2);
    EXPECT_NE(result, nullptr);

    volta_string_free(s1);
    volta_string_free(s2);
    volta_string_free(result);

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, StringCloneAfterInit) {
    volta_runtime_init();

    VoltaString* original = volta_string_from_cstr("Original");
    ASSERT_NE(original, nullptr);

    VoltaString* clone = volta_string_clone(original);
    EXPECT_NE(clone, nullptr);

    volta_string_free(original);
    volta_string_free(clone);

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ManyStringOperations) {
    volta_runtime_init();

    for (int i = 0; i < 100; i++) {
        VoltaString* str = volta_string_from_cstr("Test string");
        ASSERT_NE(str, nullptr);
        volta_string_free(str);
    }

    volta_runtime_shutdown();
}

// ============================================================================
// Integration with Array Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, ArrayCreationAfterInit) {
    volta_runtime_init();

    VoltaArray* arr = volta_array_new(sizeof(int));
    EXPECT_NE(arr, nullptr);

    volta_array_free(arr);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ArrayOperationsAfterInit) {
    volta_runtime_init();

    VoltaArray* arr = volta_array_new(sizeof(int));
    ASSERT_NE(arr, nullptr);

    for (int i = 0; i < 100; i++) {
        bool success = volta_array_push(arr, &i);
        EXPECT_TRUE(success);
    }

    int32_t len = volta_array_length(arr);
    EXPECT_EQ(len, 100);

    volta_array_free(arr);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ArrayFromDataAfterInit) {
    volta_runtime_init();

    int data[] = {1, 2, 3, 4, 5};
    VoltaArray* arr = volta_array_from_data(sizeof(int), data, 5);
    EXPECT_NE(arr, nullptr);

    volta_array_free(arr);
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ArrayCloneAfterInit) {
    volta_runtime_init();

    VoltaArray* original = volta_array_new(sizeof(double));
    ASSERT_NE(original, nullptr);

    VoltaArray* clone = volta_array_clone(original);
    EXPECT_NE(clone, nullptr);

    volta_array_free(original);
    volta_array_free(clone);

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ManyArrayOperations) {
    volta_runtime_init();

    for (int i = 0; i < 100; i++) {
        VoltaArray* arr = volta_array_new(sizeof(int));
        ASSERT_NE(arr, nullptr);
        volta_array_free(arr);
    }

    volta_runtime_shutdown();
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, StressAllComponents) {
    volta_runtime_init();

    // Allocations
    std::vector<void*> allocs;
    for (int i = 0; i < 50; i++) {
        allocs.push_back(volta_alloc(128));
    }

    // GC allocations
    for (int i = 0; i < 50; i++) {
        volta_gc_malloc(256);
    }

    // Strings
    std::vector<VoltaString*> strings;
    for (int i = 0; i < 50; i++) {
        strings.push_back(volta_string_from_cstr("Test"));
    }

    // Arrays
    std::vector<VoltaArray*> arrays;
    for (int i = 0; i < 50; i++) {
        arrays.push_back(volta_array_new(sizeof(int)));
    }

    // Cleanup
    for (void* ptr : allocs) {
        volta_free(ptr);
    }
    for (VoltaString* str : strings) {
        volta_string_free(str);
    }
    for (VoltaArray* arr : arrays) {
        volta_array_free(arr);
    }

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, StressMixedOperations) {
    volta_runtime_init();

    for (int round = 0; round < 10; round++) {
        // Allocate
        void* ptr = volta_alloc(64);

        // Create string
        VoltaString* str = volta_string_from_cstr("Round");

        // Create array
        VoltaArray* arr = volta_array_new(sizeof(int));

        // GC allocation
        volta_gc_malloc(128);

        // Cleanup
        volta_free(ptr);
        volta_string_free(str);
        volta_array_free(arr);
    }

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, StressRapidInitShutdown) {
    for (int i = 0; i < 1000; i++) {
        volta_runtime_init();

        // Quick allocation
        void* ptr = volta_alloc(32);
        volta_free(ptr);

        volta_runtime_shutdown();
    }
}

TEST_F(VoltaRuntimeTest, StressLargeAllocations) {
    volta_runtime_init();

    for (int i = 0; i < 10; i++) {
        void* ptr = volta_alloc(1024 * 1024); // 1 MB
        EXPECT_NE(ptr, nullptr);
        volta_free(ptr);
    }

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, StressManySmallAllocations) {
    volta_runtime_init();

    std::vector<void*> ptrs;
    for (int i = 0; i < 10000; i++) {
        void* ptr = volta_alloc(8);
        EXPECT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    for (void* ptr : ptrs) {
        volta_free(ptr);
    }

    volta_runtime_shutdown();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(VoltaRuntimeTest, AllocAfterShutdown) {
    volta_runtime_init();
    volta_runtime_shutdown();

    // Allocation after shutdown - behavior is implementation-defined
    // Should not crash catastrophically
    void* ptr = volta_alloc(64);
    volta_free(ptr);
}

TEST_F(VoltaRuntimeTest, StringAfterShutdown) {
    volta_runtime_init();
    volta_runtime_shutdown();

    // String operations after shutdown
    VoltaString* str = volta_string_from_cstr("Test");
    volta_string_free(str);
}

TEST_F(VoltaRuntimeTest, ArrayAfterShutdown) {
    volta_runtime_init();
    volta_runtime_shutdown();

    // Array operations after shutdown
    VoltaArray* arr = volta_array_new(sizeof(int));
    volta_array_free(arr);
}

TEST_F(VoltaRuntimeTest, GCAfterShutdown) {
    volta_runtime_init();
    volta_runtime_shutdown();

    // GC operations after shutdown
    void* ptr = volta_gc_malloc(64);
    // Should not crash
}

// ============================================================================
// Lifecycle Edge Cases
// ============================================================================

TEST_F(VoltaRuntimeTest, InitWithoutShutdown) {
    // Initialize but don't explicitly shutdown
    // Should be handled by test cleanup or program exit
    volta_runtime_init();
    // Intentionally no shutdown
}

TEST_F(VoltaRuntimeTest, NestedInitShutdown) {
    volta_runtime_init();
    volta_runtime_init(); // Nested init

    volta_runtime_shutdown();
    volta_runtime_shutdown(); // Nested shutdown
}

TEST_F(VoltaRuntimeTest, AllocBeforeInit) {
    // Allocate before runtime init
    // Should work if allocator is independent of runtime
    void* ptr = volta_alloc(64);
    volta_free(ptr);

    volta_runtime_init();
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, ComplexLifecycle) {
    // Complex initialization pattern
    volta_runtime_init();
    void* ptr1 = volta_alloc(64);

    volta_runtime_shutdown();
    volta_runtime_init();

    void* ptr2 = volta_alloc(64);

    volta_free(ptr1);
    volta_free(ptr2);

    volta_runtime_shutdown();
}

// ============================================================================
// Integration Tests - Full Workflow
// ============================================================================

TEST_F(VoltaRuntimeTest, FullWorkflow) {
    // Initialize runtime
    volta_runtime_init();

    // Create some data structures
    VoltaString* message = volta_string_from_cstr("Runtime test");
    VoltaArray* numbers = volta_array_new(sizeof(int));

    // Populate array
    for (int i = 0; i < 10; i++) {
        volta_array_push(numbers, &i);
    }

    // Verify
    EXPECT_NE(message, nullptr);
    EXPECT_NE(numbers, nullptr);
    EXPECT_EQ(volta_array_length(numbers), 10);

    // Cleanup
    volta_string_free(message);
    volta_array_free(numbers);

    // Shutdown runtime
    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, MultipleComponentsWorkflow) {
    volta_runtime_init();

    // Strings
    VoltaString* s1 = volta_string_from_cstr("Hello");
    VoltaString* s2 = volta_string_from_cstr("World");
    VoltaString* concat = volta_string_concat(s1, s2);

    // Arrays
    VoltaArray* arr1 = volta_array_new(sizeof(double));
    VoltaArray* arr2 = volta_array_clone(arr1);

    // Allocations
    void* mem1 = volta_alloc(1024);
    void* mem2 = volta_gc_malloc(2048);

    // Verify all succeeded
    EXPECT_NE(concat, nullptr);
    EXPECT_NE(arr2, nullptr);
    EXPECT_NE(mem1, nullptr);
    EXPECT_NE(mem2, nullptr);

    // Cleanup
    volta_string_free(s1);
    volta_string_free(s2);
    volta_string_free(concat);
    volta_array_free(arr1);
    volta_array_free(arr2);
    volta_free(mem1);

    volta_runtime_shutdown();
}

TEST_F(VoltaRuntimeTest, LongRunningSession) {
    volta_runtime_init();

    // Simulate long-running program
    for (int round = 0; round < 100; round++) {
        // Allocate
        VoltaString* str = volta_string_from_cstr("Iteration");
        VoltaArray* arr = volta_array_new(sizeof(int));

        // Use
        int value = round;
        volta_array_push(arr, &value);

        // Cleanup
        volta_string_free(str);
        volta_array_free(arr);

        // Periodic GC
        if (round % 10 == 0) {
            volta_gc_collect();
        }
    }

    volta_runtime_shutdown();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
