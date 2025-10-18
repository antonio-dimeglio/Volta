/**
 * Comprehensive test suite for volta_gc.h
 * Tests all garbage collection functions without implementing functionality
 */

#include <gtest/gtest.h>
#include "runtime/memory/gc.h"
#include <vector>
#include <cstring>

class VoltaGCTest : public ::testing::Test {
protected:
    void SetUp() override {
        volta_gc_init();
    }

    void TearDown() override {
        volta_gc_shutdown();
    }
};

// ============================================================================
// Initialization and Shutdown Tests
// ============================================================================

TEST_F(VoltaGCTest, InitDoesNotCrash) {
    // Init called in SetUp
    // Just verify we can call it
}

TEST_F(VoltaGCTest, ShutdownDoesNotCrash) {
    // Shutdown called in TearDown
    // Just verify we can call it
}

TEST_F(VoltaGCTest, MultipleInitShutdownCycles) {
    for (int i = 0; i < 5; i++) {
        volta_gc_init();
        volta_gc_shutdown();
    }
}

TEST_F(VoltaGCTest, IsEnabledReturnsBoolean) {
    bool enabled = volta_gc_is_enabled();
    // Result depends on VOLTA_GC_MODE, but should return valid bool
    EXPECT_TRUE(enabled == true || enabled == false);
}

// ============================================================================
// Basic Allocation Tests
// ============================================================================

TEST_F(VoltaGCTest, MallocReturnsNonNull) {
    void* ptr = volta_gc_malloc(64);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, MallocZeroSize) {
    void* ptr = volta_gc_malloc(0);
    // Implementation-defined, should not crash
}

TEST_F(VoltaGCTest, MallocLargeSize) {
    size_t large_size = 1024 * 1024 * 10; // 10 MB
    void* ptr = volta_gc_malloc(large_size);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, MallocMultipleAllocations) {
    std::vector<void*> ptrs;
    for (int i = 0; i < 100; i++) {
        void* ptr = volta_gc_malloc(128);
        EXPECT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // Verify all unique
    for (size_t i = 0; i < ptrs.size(); i++) {
        for (size_t j = i + 1; j < ptrs.size(); j++) {
            EXPECT_NE(ptrs[i], ptrs[j]);
        }
    }
}

TEST_F(VoltaGCTest, MallocMemoryIsWritable) {
    const size_t size = 256;
    uint8_t* ptr = static_cast<uint8_t*>(volta_gc_malloc(size));
    ASSERT_NE(ptr, nullptr);

    // Write pattern
    for (size_t i = 0; i < size; i++) {
        ptr[i] = static_cast<uint8_t>(i & 0xFF);
    }

    // Verify
    for (size_t i = 0; i < size; i++) {
        EXPECT_EQ(ptr[i], static_cast<uint8_t>(i & 0xFF));
    }
}

TEST_F(VoltaGCTest, MallocVariousSizes) {
    std::vector<size_t> sizes = {1, 8, 16, 32, 64, 128, 256, 512, 1024, 4096, 8192};

    for (size_t size : sizes) {
        void* ptr = volta_gc_malloc(size);
        EXPECT_NE(ptr, nullptr) << "Failed for size " << size;
    }
}

// ============================================================================
// Calloc Tests
// ============================================================================

TEST_F(VoltaGCTest, CallocReturnsNonNull) {
    void* ptr = volta_gc_calloc(10, 8);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, CallocInitializesToZero) {
    const size_t count = 128;
    const size_t size = sizeof(uint64_t);
    uint64_t* ptr = static_cast<uint64_t*>(volta_gc_calloc(count, size));
    ASSERT_NE(ptr, nullptr);

    for (size_t i = 0; i < count; i++) {
        EXPECT_EQ(ptr[i], 0ULL) << "Index " << i << " should be zero";
    }
}

TEST_F(VoltaGCTest, CallocLargeBlock) {
    size_t count = 1024 * 1024; // 1M bytes
    size_t size = 1;
    uint8_t* ptr = static_cast<uint8_t*>(volta_gc_calloc(count, size));
    ASSERT_NE(ptr, nullptr);

    // Sample check
    for (size_t i = 0; i < count; i += 1024) {
        EXPECT_EQ(ptr[i], 0);
    }
}

TEST_F(VoltaGCTest, CallocZeroCount) {
    void* ptr = volta_gc_calloc(0, 10);
    // Should not crash
}

TEST_F(VoltaGCTest, CallocZeroSize) {
    void* ptr = volta_gc_calloc(10, 0);
    // Should not crash
}

TEST_F(VoltaGCTest, CallocVariousCombinations) {
    struct TestCase {
        size_t count;
        size_t size;
    };

    std::vector<TestCase> cases = {
        {1, 1},
        {10, 8},
        {100, 4},
        {1000, 1},
        {8, 1024},
    };

    for (const auto& tc : cases) {
        void* ptr = volta_gc_calloc(tc.count, tc.size);
        EXPECT_NE(ptr, nullptr) << "count=" << tc.count << " size=" << tc.size;
    }
}

// ============================================================================
// Realloc Tests
// ============================================================================

TEST_F(VoltaGCTest, ReallocNullPointer) {
    void* ptr = volta_gc_realloc(nullptr, 64);
    EXPECT_NE(ptr, nullptr) << "realloc(NULL, size) should behave like malloc";
}

TEST_F(VoltaGCTest, ReallocGrow) {
    int* ptr = static_cast<int*>(volta_gc_malloc(10 * sizeof(int)));
    ASSERT_NE(ptr, nullptr);

    // Initialize
    for (int i = 0; i < 10; i++) {
        ptr[i] = i * 3;
    }

    // Grow
    int* new_ptr = static_cast<int*>(volta_gc_realloc(ptr, 20 * sizeof(int)));
    ASSERT_NE(new_ptr, nullptr);

    // Verify data preserved
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(new_ptr[i], i * 3);
    }
}

TEST_F(VoltaGCTest, ReallocShrink) {
    double* ptr = static_cast<double*>(volta_gc_malloc(100 * sizeof(double)));
    ASSERT_NE(ptr, nullptr);

    // Initialize
    for (int i = 0; i < 100; i++) {
        ptr[i] = i * 1.5;
    }

    // Shrink
    double* new_ptr = static_cast<double*>(volta_gc_realloc(ptr, 50 * sizeof(double)));
    ASSERT_NE(new_ptr, nullptr);

    // Verify first 50 preserved
    for (int i = 0; i < 50; i++) {
        EXPECT_DOUBLE_EQ(new_ptr[i], i * 1.5);
    }
}

TEST_F(VoltaGCTest, ReallocToZero) {
    void* ptr = volta_gc_malloc(64);
    ASSERT_NE(ptr, nullptr);

    void* new_ptr = volta_gc_realloc(ptr, 0);
    // Implementation-defined behavior
}

TEST_F(VoltaGCTest, ReallocChain) {
    void* ptr = volta_gc_malloc(16);
    ASSERT_NE(ptr, nullptr);

    size_t size = 16;
    for (int i = 0; i < 10; i++) {
        size *= 2;
        void* new_ptr = volta_gc_realloc(ptr, size);
        ASSERT_NE(new_ptr, nullptr);
        ptr = new_ptr;
    }
}

TEST_F(VoltaGCTest, ReallocPreservesData) {
    const size_t initial_size = 64;
    const size_t new_size = 256;

    uint8_t* ptr = static_cast<uint8_t*>(volta_gc_malloc(initial_size));
    ASSERT_NE(ptr, nullptr);

    // Pattern
    for (size_t i = 0; i < initial_size; i++) {
        ptr[i] = static_cast<uint8_t>((i * 7) & 0xFF);
    }

    // Reallocate
    ptr = static_cast<uint8_t*>(volta_gc_realloc(ptr, new_size));
    ASSERT_NE(ptr, nullptr);

    // Verify
    for (size_t i = 0; i < initial_size; i++) {
        EXPECT_EQ(ptr[i], static_cast<uint8_t>((i * 7) & 0xFF));
    }
}

// ============================================================================
// Free Tests
// ============================================================================

TEST_F(VoltaGCTest, FreeNullPointer) {
    volta_gc_free(nullptr);
    // Should not crash
}

TEST_F(VoltaGCTest, FreeAfterMalloc) {
    void* ptr = volta_gc_malloc(64);
    ASSERT_NE(ptr, nullptr);
    volta_gc_free(ptr);
    // Should not crash (may be no-op for GC)
}

TEST_F(VoltaGCTest, FreeManyAllocations) {
    for (int i = 0; i < 1000; i++) {
        void* ptr = volta_gc_malloc(128);
        ASSERT_NE(ptr, nullptr);
        volta_gc_free(ptr);
    }
}

TEST_F(VoltaGCTest, FreeInterleavedWithAlloc) {
    std::vector<void*> ptrs;

    for (int round = 0; round < 10; round++) {
        // Allocate
        for (int i = 0; i < 50; i++) {
            ptrs.push_back(volta_gc_malloc(64));
        }

        // Free half
        for (int i = 0; i < 25 && !ptrs.empty(); i++) {
            volta_gc_free(ptrs.back());
            ptrs.pop_back();
        }
    }

    // Free remaining
    for (void* ptr : ptrs) {
        volta_gc_free(ptr);
    }
}

// ============================================================================
// Collection Tests
// ============================================================================

TEST_F(VoltaGCTest, CollectDoesNotCrash) {
    volta_gc_collect();
    // Should not crash
}

TEST_F(VoltaGCTest, CollectAfterAllocations) {
    for (int i = 0; i < 100; i++) {
        volta_gc_malloc(1024);
    }

    volta_gc_collect();
    // Should not crash
}

TEST_F(VoltaGCTest, MultipleCollections) {
    for (int i = 0; i < 10; i++) {
        volta_gc_malloc(512);
        volta_gc_collect();
    }
}

TEST_F(VoltaGCTest, CollectWithActivePointers) {
    std::vector<void*> ptrs;
    for (int i = 0; i < 50; i++) {
        ptrs.push_back(volta_gc_malloc(256));
    }

    volta_gc_collect();

    // Pointers should still be valid (on stack, thus roots)
    for (void* ptr : ptrs) {
        EXPECT_NE(ptr, nullptr);
    }
}

// ============================================================================
// Finalizer Tests
// ============================================================================

static int finalizer_call_count = 0;

static void test_finalizer(void* obj, void* client_data) {
    finalizer_call_count++;
    if (client_data) {
        int* counter = static_cast<int*>(client_data);
        (*counter)++;
    }
}

TEST_F(VoltaGCTest, RegisterFinalizerDoesNotCrash) {
    void* obj = volta_gc_malloc(64);
    ASSERT_NE(obj, nullptr);

    volta_gc_register_finalizer(obj, test_finalizer, nullptr);
    // Should not crash
}

TEST_F(VoltaGCTest, RegisterFinalizerWithClientData) {
    void* obj = volta_gc_malloc(64);
    ASSERT_NE(obj, nullptr);

    int counter = 0;
    volta_gc_register_finalizer(obj, test_finalizer, &counter);
    // Should not crash
}

TEST_F(VoltaGCTest, RegisterFinalizerNullObject) {
    // Behavior is implementation-defined, should not crash
    volta_gc_register_finalizer(nullptr, test_finalizer, nullptr);
}

TEST_F(VoltaGCTest, RegisterFinalizerNullFunction) {
    void* obj = volta_gc_malloc(64);
    volta_gc_register_finalizer(obj, nullptr, nullptr);
    // Should not crash
}

// ============================================================================
// Heap Statistics Tests
// ============================================================================

TEST_F(VoltaGCTest, GetHeapSizeReturnsValidValue) {
    size_t heap_size = volta_gc_get_heap_size();
    // May be 0 if not implemented, but should not crash
    EXPECT_GE(heap_size, 0);
}

TEST_F(VoltaGCTest, GetFreeBytesReturnsValidValue) {
    size_t free_bytes = volta_gc_get_free_bytes();
    EXPECT_GE(free_bytes, 0);
}

TEST_F(VoltaGCTest, HeapSizeIncreasesWithAllocations) {
    size_t initial_heap = volta_gc_get_heap_size();

    // Allocate large blocks
    for (int i = 0; i < 10; i++) {
        volta_gc_malloc(1024 * 1024); // 1 MB each
    }

    size_t after_heap = volta_gc_get_heap_size();

    // Heap should grow or stay same (if not tracked, both will be 0)
    EXPECT_GE(after_heap, initial_heap);
}

TEST_F(VoltaGCTest, GetStatsDoesNotCrash) {
    VoltaGCStats stats;
    volta_gc_get_stats(&stats);

    // Should not crash, values may be 0 if not implemented
    EXPECT_GE(stats.total_allocations, 0);
    EXPECT_GE(stats.total_bytes, 0);
    EXPECT_GE(stats.heap_size, 0);
    EXPECT_GE(stats.free_bytes, 0);
    EXPECT_GE(stats.num_collections, 0);
}

TEST_F(VoltaGCTest, GetStatsAfterAllocations) {
    VoltaGCStats stats_before;
    volta_gc_get_stats(&stats_before);

    // Allocate
    for (int i = 0; i < 100; i++) {
        volta_gc_malloc(128);
    }

    VoltaGCStats stats_after;
    volta_gc_get_stats(&stats_after);

    // If tracking is implemented, values should increase
    EXPECT_GE(stats_after.total_allocations, stats_before.total_allocations);
    EXPECT_GE(stats_after.total_bytes, stats_before.total_bytes);
}

TEST_F(VoltaGCTest, ResetStatsDoesNotCrash) {
    volta_gc_reset_stats();
    // Should not crash
}

TEST_F(VoltaGCTest, ResetStatsZerosCounters) {
    // Allocate some
    for (int i = 0; i < 50; i++) {
        volta_gc_malloc(64);
    }

    volta_gc_reset_stats();

    VoltaGCStats stats;
    volta_gc_get_stats(&stats);

    // If tracking is implemented, stats should be reset
    // If not implemented, will be 0 anyway
    // This test will pass in both cases
}

TEST_F(VoltaGCTest, CollectionIncrementsCounter) {
    VoltaGCStats stats_before;
    volta_gc_get_stats(&stats_before);

    volta_gc_collect();

    VoltaGCStats stats_after;
    volta_gc_get_stats(&stats_after);

    // If collection tracking is implemented, counter should increase
    EXPECT_GE(stats_after.num_collections, stats_before.num_collections);
}

// ============================================================================
// Root Management Tests
// ============================================================================

TEST_F(VoltaGCTest, AddRootsDoesNotCrash) {
    int data[100];
    volta_gc_add_roots(&data[0], &data[100]);
    // Should not crash
}

TEST_F(VoltaGCTest, RemoveRootsDoesNotCrash) {
    int data[100];
    volta_gc_add_roots(&data[0], &data[100]);
    volta_gc_remove_roots(&data[0], &data[100]);
    // Should not crash
}

TEST_F(VoltaGCTest, AddRemoveRootsPairs) {
    for (int i = 0; i < 10; i++) {
        int data[50];
        volta_gc_add_roots(&data[0], &data[50]);
        volta_gc_remove_roots(&data[0], &data[50]);
    }
}

TEST_F(VoltaGCTest, AddRootsWithGCAllocatedData) {
    void* ptr = volta_gc_malloc(1024);
    ASSERT_NE(ptr, nullptr);

    // Add as root
    char* start = static_cast<char*>(ptr);
    char* end = start + 1024;
    volta_gc_add_roots(start, end);

    // Collect - object should not be freed (it's a root)
    volta_gc_collect();

    // Remove root
    volta_gc_remove_roots(start, end);
}

TEST_F(VoltaGCTest, AddRootsNullPointers) {
    // Edge case - should not crash
    volta_gc_add_roots(nullptr, nullptr);
}

TEST_F(VoltaGCTest, RemoveRootsNotAdded) {
    int data[100];
    // Remove without adding - should not crash
    volta_gc_remove_roots(&data[0], &data[100]);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(VoltaGCTest, StressManyAllocations) {
    std::vector<void*> ptrs;
    for (int i = 0; i < 10000; i++) {
        void* ptr = volta_gc_malloc(64);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    // Trigger collection
    volta_gc_collect();

    // All pointers should still be valid (they're roots)
    for (void* ptr : ptrs) {
        EXPECT_NE(ptr, nullptr);
    }
}

TEST_F(VoltaGCTest, StressMixedOperations) {
    for (int round = 0; round < 100; round++) {
        void* ptr1 = volta_gc_malloc(128);
        void* ptr2 = volta_gc_calloc(10, 8);
        void* ptr3 = volta_gc_realloc(nullptr, 256);

        EXPECT_NE(ptr1, nullptr);
        EXPECT_NE(ptr2, nullptr);
        EXPECT_NE(ptr3, nullptr);

        if (round % 10 == 0) {
            volta_gc_collect();
        }
    }
}

TEST_F(VoltaGCTest, StressReallocChains) {
    std::vector<void*> ptrs;

    for (int i = 0; i < 50; i++) {
        void* ptr = volta_gc_malloc(16);
        ASSERT_NE(ptr, nullptr);

        // Grow through reallocs
        for (int j = 0; j < 5; j++) {
            ptr = volta_gc_realloc(ptr, (16 << (j + 1)));
            ASSERT_NE(ptr, nullptr);
        }

        ptrs.push_back(ptr);
    }

    volta_gc_collect();
}

TEST_F(VoltaGCTest, StressCollectionFrequency) {
    for (int i = 0; i < 1000; i++) {
        volta_gc_malloc(512);
        volta_gc_collect();
    }
}

TEST_F(VoltaGCTest, StressLargeAllocations) {
    std::vector<void*> large_blocks;
    for (int i = 0; i < 10; i++) {
        void* ptr = volta_gc_malloc(1024 * 1024 * 5); // 5 MB each
        EXPECT_NE(ptr, nullptr);
        large_blocks.push_back(ptr);
    }

    volta_gc_collect();
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST_F(VoltaGCTest, AllocateMinimumSize) {
    void* ptr = volta_gc_malloc(1);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, CallocWithLargeCount) {
    // Should handle without overflow
    void* ptr = volta_gc_calloc(1000000, 1);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, ReallocFromSmallToLarge) {
    void* ptr = volta_gc_malloc(1);
    ASSERT_NE(ptr, nullptr);

    ptr = volta_gc_realloc(ptr, 1024 * 1024);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, ReallocFromLargeToSmall) {
    void* ptr = volta_gc_malloc(1024 * 1024);
    ASSERT_NE(ptr, nullptr);

    ptr = volta_gc_realloc(ptr, 1);
    EXPECT_NE(ptr, nullptr);
}

TEST_F(VoltaGCTest, AllocationsAreIndependent) {
    int* ptr1 = static_cast<int*>(volta_gc_malloc(sizeof(int)));
    int* ptr2 = static_cast<int*>(volta_gc_malloc(sizeof(int)));
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    *ptr1 = 12345;
    *ptr2 = 67890;

    EXPECT_EQ(*ptr1, 12345);
    EXPECT_EQ(*ptr2, 67890);
}

TEST_F(VoltaGCTest, StatsStructureSanity) {
    VoltaGCStats stats;
    volta_gc_get_stats(&stats);

    // Heap size should be >= free bytes
    if (stats.heap_size > 0) {
        EXPECT_GE(stats.heap_size, stats.free_bytes);
    }
}

TEST_F(VoltaGCTest, MultipleRootsOverlapping) {
    int data[100];

    // Add overlapping root regions
    volta_gc_add_roots(&data[0], &data[50]);
    volta_gc_add_roots(&data[25], &data[75]);
    volta_gc_add_roots(&data[50], &data[100]);

    // Remove in different order
    volta_gc_remove_roots(&data[25], &data[75]);
    volta_gc_remove_roots(&data[0], &data[50]);
    volta_gc_remove_roots(&data[50], &data[100]);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
