#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

namespace volta::vm {

// ============================================================================
// Memory Arena
// ============================================================================

/**
 * Memory arena for fast bump-pointer allocation
 * Used by the GC for young generation allocation
 *
 * Benefits:
 * - Very fast allocation (just increment a pointer)
 * - Good cache locality (sequential allocation)
 * - Easy to reset entire arena after collection
 *
 * Limitations:
 * - No individual deallocation (only bulk reset)
 * - Fixed maximum size
 */
class Arena {
public:
    /**
     * Create an arena with the given size
     */
    explicit Arena(size_t size);

    ~Arena();

    // Disable copying
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    /**
     * Allocate memory from the arena
     * Returns nullptr if not enough space
     */
    void* allocate(size_t size, size_t alignment = 8);

    /**
     * Reset the arena (makes all memory available again)
     * Called after GC collection
     */
    void reset();

    /**
     * Get the current allocation position
     */
    size_t used() const { return current_ - base_; }

    /**
     * Get the total arena size
     */
    size_t capacity() const { return size_; }

    /**
     * Get the remaining free space
     */
    size_t available() const { return capacity() - used(); }

    /**
     * Check if an address is within this arena
     */
    bool contains(void* ptr) const;

private:
    uint8_t* base_;       ///< Start of arena memory
    uint8_t* current_;    ///< Current allocation pointer
    size_t size_;         ///< Total arena size
};

// ============================================================================
// Free List Allocator
// ============================================================================

/**
 * Free list entry (header for each free block)
 */
struct FreeBlock {
    size_t size;           ///< Size of this free block (including header)
    FreeBlock* next;       ///< Next free block in the list
};

/**
 * Free list allocator for old generation
 * Maintains lists of free blocks by size class
 *
 * Size classes (powers of 2):
 * - 16, 32, 64, 128, 256, 512, 1024, 2048, 4096+ bytes
 *
 * Benefits:
 * - Allows individual deallocation
 * - Reduces fragmentation with size classes
 * - Good for long-lived objects
 *
 * Limitations:
 * - Slower than arena allocation
 * - Can suffer from fragmentation
 */
class FreeListAllocator {
public:
    FreeListAllocator() = default;
    ~FreeListAllocator();

    // Disable copying
    FreeListAllocator(const FreeListAllocator&) = delete;
    FreeListAllocator& operator=(const FreeListAllocator&) = delete;

    /**
     * Allocate a block of the given size
     * Returns nullptr if no suitable block available
     */
    void* allocate(size_t size);

    /**
     * Free a block (return it to the free list)
     */
    void free(void* ptr, size_t size);

    /**
     * Allocate a new memory region from the OS
     * Used when free lists are empty
     */
    void* allocateFromOS(size_t size);

    /**
     * Get total allocated memory (not including free blocks)
     */
    size_t totalAllocated() const { return totalAllocated_; }

    /**
     * Get total free memory (in free lists)
     */
    size_t totalFree() const { return totalFree_; }

    /**
     * Compact free lists (merge adjacent blocks)
     * Called periodically to reduce fragmentation
     */
    void compact();

    /**
     * Print free list statistics (for debugging)
     */
    void printStats(std::ostream& out) const;

private:
    /**
     * Get size class index for a given size
     * Returns index into freeLists_ array
     */
    size_t getSizeClass(size_t size) const;

    /**
     * Get size class size (rounded up to power of 2)
     */
    size_t getSizeClassSize(size_t sizeClass) const;

    /**
     * Find a free block in the appropriate size class
     * Returns nullptr if not found
     */
    FreeBlock* findFreeBlock(size_t size);

    /**
     * Split a free block if it's much larger than needed
     */
    void splitBlock(FreeBlock* block, size_t size);

    /**
     * Merge adjacent free blocks (coalesce)
     */
    void mergeBlocks();

    // ========== State ==========

    /// Free lists for each size class
    static constexpr size_t NUM_SIZE_CLASSES = 9;  // 16B to 4KB+
    FreeBlock* freeLists_[NUM_SIZE_CLASSES] = {nullptr};

    /// Blocks allocated from OS (for cleanup)
    std::vector<void*> osBlocks_;

    /// Statistics
    size_t totalAllocated_ = 0;
    size_t totalFree_ = 0;
};

// ============================================================================
// Slab Allocator
// ============================================================================

/**
 * Slab allocator for fixed-size objects
 * Extremely fast allocation/deallocation for common object sizes
 *
 * Each slab contains many objects of the same size, pre-allocated
 * Allocation is just popping from a free list
 *
 * Use cases:
 * - Common struct sizes
 * - Small arrays
 * - Internal GC structures
 */
class SlabAllocator {
public:
    /**
     * Create a slab allocator for objects of the given size
     * @param objectSize Size of each object (must be >= sizeof(void*))
     * @param objectsPerSlab Number of objects per slab (default 64)
     */
    SlabAllocator(size_t objectSize, size_t objectsPerSlab = 64);

    ~SlabAllocator();

    // Disable copying
    SlabAllocator(const SlabAllocator&) = delete;
    SlabAllocator& operator=(const SlabAllocator&) = delete;

    /**
     * Allocate an object from the slab
     * Very fast: O(1) pointer pop
     */
    void* allocate();

    /**
     * Free an object back to the slab
     * Very fast: O(1) pointer push
     */
    void free(void* ptr);

    /**
     * Get the object size this slab manages
     */
    size_t objectSize() const { return objectSize_; }

    /**
     * Get total number of allocated objects
     */
    size_t allocatedCount() const { return allocatedCount_; }

    /**
     * Get total number of free objects
     */
    size_t freeCount() const { return freeCount_; }

    /**
     * Release empty slabs back to OS
     * Called periodically to reduce memory usage
     */
    void releaseEmptySlabs();

private:
    /**
     * Slab structure (contains multiple objects)
     */
    struct Slab {
        uint8_t* memory;           ///< Allocated memory block
        void* freeList;            ///< Head of free list within this slab
        size_t freeCount;          ///< Number of free objects in this slab
        Slab* next;                ///< Next slab in chain
    };

    /**
     * Allocate a new slab from the OS
     */
    Slab* allocateSlab();

    /**
     * Free a slab back to the OS
     */
    void freeSlab(Slab* slab);

    /**
     * Initialize free list within a slab
     */
    void initializeSlabFreeList(Slab* slab);

    // ========== State ==========

    size_t objectSize_;           ///< Size of each object
    size_t objectsPerSlab_;       ///< Number of objects per slab
    size_t slabSize_;             ///< Total size of each slab (bytes)

    Slab* currentSlab_;           ///< Current slab for allocation
    Slab* slabList_;              ///< List of all slabs

    void* freeList_;              ///< Global free list head

    size_t allocatedCount_;       ///< Total allocated objects
    size_t freeCount_;            ///< Total free objects
};

// ============================================================================
// Page Allocator
// ============================================================================

/**
 * Page allocator for large objects (>4KB)
 * Allocates directly from OS using mmap/VirtualAlloc
 *
 * Benefits:
 * - No fragmentation (each object gets its own pages)
 * - Can return pages to OS immediately when freed
 * - Memory protection (can mark pages read-only, etc.)
 */
class PageAllocator {
public:
    PageAllocator() = default;
    ~PageAllocator();

    // Disable copying
    PageAllocator(const PageAllocator&) = delete;
    PageAllocator& operator=(const PageAllocator&) = delete;

    /**
     * Allocate pages for a large object
     * Size is rounded up to page boundary
     */
    void* allocate(size_t size);

    /**
     * Free pages (returns to OS immediately)
     */
    void free(void* ptr, size_t size);

    /**
     * Get the system page size
     */
    static size_t pageSize();

    /**
     * Round size up to page boundary
     */
    static size_t roundToPage(size_t size);

    /**
     * Get total allocated memory
     */
    size_t totalAllocated() const { return totalAllocated_; }

private:
    /**
     * Track allocated pages for cleanup
     */
    struct PageInfo {
        void* address;
        size_t size;
    };

    std::vector<PageInfo> pages_;
    size_t totalAllocated_ = 0;
};

} // namespace volta::vm
