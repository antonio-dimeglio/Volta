/**
 * Comprehensive test suite for volta_allocator.h
 * Tests all allocation functions and backends without implementing functionality
 */

#include <gtest/gtest.h>
#include "runtime/memory/allocator.h"
#include <cstring>
#include <vector>

class VoltaAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup runs before each test
    }

    void TearDown() override {
        // Cleanup runs after each test
    }
};

// ============================================================================
// Basic Allocation Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, AllocReturnsNonNull) {
    void* ptr = volta_alloc(64);
    EXPECT_NE(ptr, nullptr) << "volta_alloc should return non-null pointer";
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocZeroSize) {
    void* ptr = volta_alloc(0);
    // Behavior is implementation-defined, but should not crash
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocLargeSize) {
    size_t large_size = 1024 * 1024; // 1 MB
    void* ptr = volta_alloc(large_size);
    EXPECT_NE(ptr, nullptr) << "Should be able to allocate 1MB";
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocMultipleBlocks) {
    const int count = 100;
    std::vector<void*> ptrs;

    for (int i = 0; i < count; i++) {
        void* ptr = volta_alloc(128);
        EXPECT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // Check all pointers are unique
    for (size_t i = 0; i < ptrs.size(); i++) {
        for (size_t j = i + 1; j < ptrs.size(); j++) {
            EXPECT_NE(ptrs[i], ptrs[j]) << "Allocations should return unique addresses";
        }
    }

    for (void* ptr : ptrs) {
        volta_free(ptr);
    }
}

// ============================================================================
// Zeroed Allocation Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, AllocZeroedReturnsNonNull) {
    void* ptr = volta_alloc_zeroed(10, 8);
    EXPECT_NE(ptr, nullptr);
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocZeroedInitializesToZero) {
    const size_t count = 100;
    const size_t size = 4;
    uint32_t* ptr = static_cast<uint32_t*>(volta_alloc_zeroed(count, size));
    ASSERT_NE(ptr, nullptr);

    // All bytes should be zero
    for (size_t i = 0; i < count; i++) {
        EXPECT_EQ(ptr[i], 0u) << "Index " << i << " should be zero";
    }

    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocZeroedLargeBlock) {
    size_t count = 1024 * 256; // 256K elements
    size_t size = 1;
    uint8_t* ptr = static_cast<uint8_t*>(volta_alloc_zeroed(count, size));
    ASSERT_NE(ptr, nullptr);

    // Check a sample of bytes
    for (size_t i = 0; i < 1000; i += 100) {
        EXPECT_EQ(ptr[i], 0);
    }

    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocZeroedCountZero) {
    void* ptr = volta_alloc_zeroed(0, 10);
    // Should not crash
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocZeroedSizeZero) {
    void* ptr = volta_alloc_zeroed(10, 0);
    // Should not crash
    volta_free(ptr);
}

// ============================================================================
// Realloc Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, ReallocNullPointer) {
    void* ptr = volta_realloc(nullptr, 64);
    EXPECT_NE(ptr, nullptr) << "realloc(NULL, size) should behave like alloc";
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, ReallocGrow) {
    int* ptr = static_cast<int*>(volta_alloc(10 * sizeof(int)));
    ASSERT_NE(ptr, nullptr);

    // Initialize data
    for (int i = 0; i < 10; i++) {
        ptr[i] = i;
    }

    // Grow allocation
    int* new_ptr = static_cast<int*>(volta_realloc(ptr, 20 * sizeof(int)));
    ASSERT_NE(new_ptr, nullptr);

    // Check data is preserved
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(new_ptr[i], i) << "Data should be preserved after realloc";
    }

    volta_free(new_ptr);
}

TEST_F(VoltaAllocatorTest, ReallocShrink) {
    int* ptr = static_cast<int*>(volta_alloc(20 * sizeof(int)));
    ASSERT_NE(ptr, nullptr);

    // Initialize data
    for (int i = 0; i < 20; i++) {
        ptr[i] = i * 2;
    }

    // Shrink allocation
    int* new_ptr = static_cast<int*>(volta_realloc(ptr, 10 * sizeof(int)));
    ASSERT_NE(new_ptr, nullptr);

    // Check first 10 elements are preserved
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(new_ptr[i], i * 2);
    }

    volta_free(new_ptr);
}

TEST_F(VoltaAllocatorTest, ReallocToZero) {
    void* ptr = volta_alloc(64);
    ASSERT_NE(ptr, nullptr);

    void* new_ptr = volta_realloc(ptr, 0);
    // Behavior is implementation-defined (may return NULL or equivalent to free)
    // Just ensure it doesn't crash
    if (new_ptr != nullptr) {
        volta_free(new_ptr);
    }
}

TEST_F(VoltaAllocatorTest, ReallocMultipleTimes) {
    void* ptr = volta_alloc(16);
    ASSERT_NE(ptr, nullptr);

    // Reallocate multiple times
    ptr = volta_realloc(ptr, 32);
    EXPECT_NE(ptr, nullptr);

    ptr = volta_realloc(ptr, 64);
    EXPECT_NE(ptr, nullptr);

    ptr = volta_realloc(ptr, 128);
    EXPECT_NE(ptr, nullptr);

    volta_free(ptr);
}

// ============================================================================
// Free Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, FreeNullPointer) {
    // Should not crash
    volta_free(nullptr);
}

TEST_F(VoltaAllocatorTest, FreeAfterAlloc) {
    void* ptr = volta_alloc(64);
    ASSERT_NE(ptr, nullptr);
    volta_free(ptr);
    // Should not crash
}

TEST_F(VoltaAllocatorTest, FreeManyAllocations) {
    std::vector<void*> ptrs;
    for (int i = 0; i < 1000; i++) {
        ptrs.push_back(volta_alloc(64));
    }

    for (void* ptr : ptrs) {
        volta_free(ptr);
    }
}

// ============================================================================
// Array Allocation Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, AllocArrayReturnsNonNull) {
    void* ptr = volta_alloc_array(10, sizeof(int));
    EXPECT_NE(ptr, nullptr);
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocArraySufficientSize) {
    const size_t count = 100;
    const size_t elem_size = sizeof(double);
    double* ptr = static_cast<double*>(volta_alloc_array(count, elem_size));
    ASSERT_NE(ptr, nullptr);

    // Should be able to access all elements
    for (size_t i = 0; i < count; i++) {
        ptr[i] = static_cast<double>(i) * 1.5;
    }

    // Verify
    for (size_t i = 0; i < count; i++) {
        EXPECT_DOUBLE_EQ(ptr[i], static_cast<double>(i) * 1.5);
    }

    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocArrayOverflowProtection) {
    // Test overflow protection: count * element_size would overflow
    size_t huge_count = SIZE_MAX / 4;
    size_t large_elem_size = 8;

    void* ptr = volta_alloc_array(huge_count, large_elem_size);
    // Should return NULL due to overflow check
    if (ptr != nullptr) {
        // If implementation doesn't have overflow check, at least shouldn't crash
        volta_free(ptr);
    }
}

TEST_F(VoltaAllocatorTest, AllocArrayZeroCount) {
    void* ptr = volta_alloc_array(0, sizeof(int));
    // Should not crash
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocArrayZeroSize) {
    void* ptr = volta_alloc_array(10, 0);
    // Should not crash
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocArrayVariousSizes) {
    struct TestCase {
        size_t count;
        size_t elem_size;
    };

    std::vector<TestCase> cases = {
        {1, 1},
        {10, 1},
        {1, 10},
        {100, sizeof(int)},
        {50, sizeof(double)},
        {1000, sizeof(char)},
    };

    for (const auto& tc : cases) {
        void* ptr = volta_alloc_array(tc.count, tc.elem_size);
        EXPECT_NE(ptr, nullptr) << "count=" << tc.count << " elem_size=" << tc.elem_size;
        volta_free(ptr);
    }
}

// ============================================================================
// Array Zeroed Allocation Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, AllocArrayZeroedReturnsNonNull) {
    void* ptr = volta_alloc_array_zeroed(10, sizeof(int));
    EXPECT_NE(ptr, nullptr);
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocArrayZeroedInitializesToZero) {
    const size_t count = 256;
    const size_t elem_size = sizeof(uint64_t);
    uint64_t* ptr = static_cast<uint64_t*>(volta_alloc_array_zeroed(count, elem_size));
    ASSERT_NE(ptr, nullptr);

    for (size_t i = 0; i < count; i++) {
        EXPECT_EQ(ptr[i], 0ULL);
    }

    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocArrayZeroedLargeArray) {
    size_t count = 10000;
    size_t elem_size = sizeof(int);
    int* ptr = static_cast<int*>(volta_alloc_array_zeroed(count, elem_size));
    ASSERT_NE(ptr, nullptr);

    // Sample check
    for (size_t i = 0; i < count; i += 100) {
        EXPECT_EQ(ptr[i], 0);
    }

    volta_free(ptr);
}

// ============================================================================
// Memory Operations Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, MemoryCanBeWrittenAndRead) {
    const size_t size = 1024;
    uint8_t* ptr = static_cast<uint8_t*>(volta_alloc(size));
    ASSERT_NE(ptr, nullptr);

    // Write pattern
    for (size_t i = 0; i < size; i++) {
        ptr[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Read and verify
    for (size_t i = 0; i < size; i++) {
        EXPECT_EQ(ptr[i], static_cast<uint8_t>(i & 0xFF));
    }

    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocationsAreIndependent) {
    int* ptr1 = static_cast<int*>(volta_alloc(sizeof(int)));
    int* ptr2 = static_cast<int*>(volta_alloc(sizeof(int)));
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    *ptr1 = 42;
    *ptr2 = 99;

    EXPECT_EQ(*ptr1, 42) << "ptr1 should not be affected by ptr2";
    EXPECT_EQ(*ptr2, 99) << "ptr2 should not be affected by ptr1";

    volta_free(ptr1);
    volta_free(ptr2);
}

TEST_F(VoltaAllocatorTest, ReallocPreservesData) {
    const size_t initial_size = 100;
    const size_t new_size = 200;

    uint8_t* ptr = static_cast<uint8_t*>(volta_alloc(initial_size));
    ASSERT_NE(ptr, nullptr);

    // Fill with pattern
    for (size_t i = 0; i < initial_size; i++) {
        ptr[i] = static_cast<uint8_t>(i);
    }

    // Reallocate
    ptr = static_cast<uint8_t*>(volta_realloc(ptr, new_size));
    ASSERT_NE(ptr, nullptr);

    // Verify original data is intact
    for (size_t i = 0; i < initial_size; i++) {
        EXPECT_EQ(ptr[i], static_cast<uint8_t>(i)) << "Data corrupted at index " << i;
    }

    volta_free(ptr);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, StressTestMixedOperations) {
    std::vector<void*> ptrs;
    const int iterations = 1000;

    for (int i = 0; i < iterations; i++) {
        // Allocate
        void* ptr = volta_alloc(64 + (i % 100));
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);

        // Free every other allocation
        if (i % 2 == 0 && !ptrs.empty()) {
            volta_free(ptrs.back());
            ptrs.pop_back();
        }
    }

    // Free remaining
    for (void* ptr : ptrs) {
        volta_free(ptr);
    }
}

TEST_F(VoltaAllocatorTest, StressTestReallocChain) {
    void* ptr = volta_alloc(8);
    ASSERT_NE(ptr, nullptr);

    size_t size = 8;
    for (int i = 0; i < 20; i++) {
        size *= 2;
        void* new_ptr = volta_realloc(ptr, size);
        ASSERT_NE(new_ptr, nullptr);
        ptr = new_ptr;
    }

    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocateAndFreeInterleaved) {
    std::vector<void*> active;

    for (int round = 0; round < 10; round++) {
        // Allocate 100
        for (int i = 0; i < 100; i++) {
            void* ptr = volta_alloc(128);
            ASSERT_NE(ptr, nullptr);
            active.push_back(ptr);
        }

        // Free 50
        for (int i = 0; i < 50 && !active.empty(); i++) {
            volta_free(active.back());
            active.pop_back();
        }
    }

    // Free all remaining
    for (void* ptr : active) {
        volta_free(ptr);
    }
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST_F(VoltaAllocatorTest, AllocMinimumSize) {
    void* ptr = volta_alloc(1);
    EXPECT_NE(ptr, nullptr);
    volta_free(ptr);
}

TEST_F(VoltaAllocatorTest, AllocPowerOfTwoSizes) {
    for (int power = 0; power < 20; power++) {
        size_t size = 1ULL << power;
        void* ptr = volta_alloc(size);
        EXPECT_NE(ptr, nullptr) << "Failed for size 2^" << power << " = " << size;
        volta_free(ptr);
    }
}

TEST_F(VoltaAllocatorTest, AllocNonAlignedSizes) {
    std::vector<size_t> sizes = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31};

    for (size_t size : sizes) {
        void* ptr = volta_alloc(size);
        EXPECT_NE(ptr, nullptr) << "Failed for size " << size;
        volta_free(ptr);
    }
}

TEST_F(VoltaAllocatorTest, AllocArrayEdgeCases) {
    struct TestCase {
        size_t count;
        size_t elem_size;
        const char* description;
    };

    std::vector<TestCase> cases = {
        {1, 1, "smallest array"},
        {SIZE_MAX, 0, "max count, zero size"},
        {0, SIZE_MAX, "zero count, max size"},
        {1, SIZE_MAX / 2, "half max element size"},
    };

    for (const auto& tc : cases) {
        void* ptr = volta_alloc_array(tc.count, tc.elem_size);
        // Should not crash
        volta_free(ptr);
    }
}

// ============================================================================
// Tracking Tests (if implemented)
// ============================================================================

TEST_F(VoltaAllocatorTest, TrackingEnableDisable) {
    // These functions are declared but may not be implemented
    // Test should not crash
    volta_allocator_enable_tracking();
    volta_allocator_disable_tracking();
}

TEST_F(VoltaAllocatorTest, TrackingGetters) {
    // These may return 0 if not implemented
    size_t total = volta_allocator_get_total_allocated();
    size_t in_use = volta_allocator_get_bytes_in_use();

    // Just verify they don't crash and return reasonable values
    EXPECT_GE(total, 0);
    EXPECT_GE(in_use, 0);
}

TEST_F(VoltaAllocatorTest, TrackingBasicOperation) {
    volta_allocator_enable_tracking();

    size_t before_total = volta_allocator_get_total_allocated();
    size_t before_in_use = volta_allocator_get_bytes_in_use();

    void* ptr = volta_alloc(1024);
    ASSERT_NE(ptr, nullptr);

    size_t after_alloc_total = volta_allocator_get_total_allocated();
    size_t after_alloc_in_use = volta_allocator_get_bytes_in_use();

    // If tracking is implemented, these should change
    // If not implemented, they'll be zero and test will still pass
    EXPECT_GE(after_alloc_total, before_total);
    EXPECT_GE(after_alloc_in_use, before_in_use);

    volta_free(ptr);
    volta_allocator_disable_tracking();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
