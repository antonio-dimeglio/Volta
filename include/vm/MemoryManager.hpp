#pragma once

#include "GC.hpp"
#include "Allocator.hpp"
#include "VM.hpp"
#include <memory>
#include <cstddef>

namespace volta::vm {

// Forward declarations
class GarbageCollector;

// ============================================================================
// Memory Manager
// ============================================================================

/**
 * Memory manager - coordinates allocation and garbage collection
 *
 * Architecture:
 * - Young generation: uses Arena (bump-pointer allocation)
 * - Old generation: uses FreeListAllocator (size-segregated)
 * - Large objects (>4KB): uses PageAllocator
 * - Small fixed-size objects: optionally use SlabAllocator
 *
 * Integration with GC:
 * - Automatically triggers GC when thresholds are exceeded
 * - Provides allocation statistics for GC decisions
 * - Manages object metadata (headers, GC bits)
 */
class MemoryManager {
public:
    /**
     * Create a memory manager for a VM instance
     */
    explicit MemoryManager(VM* vm, const GCConfig& config = GCConfig{});

    ~MemoryManager();

    // Disable copying
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // ========== Allocation Interface ==========

    /**
     * Allocate a struct object
     * Uses young generation arena (fast path)
     * May trigger GC if heap is full
     */
    StructObject* allocateStruct(uint32_t typeId, uint32_t fieldCount);

    /**
     * Allocate an array object
     * Small arrays: young generation
     * Large arrays (>4KB): page allocator
     * May trigger GC if heap is full
     */
    ArrayObject* allocateArray(uint32_t length);

    /**
     * Generic allocation (for custom object types)
     * Chooses allocator based on size:
     * - Small (<4KB): arena or free list
     * - Large (>=4KB): page allocator
     */
    void* allocate(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId = 0);

    // ========== Garbage Collection Control ==========

    /**
     * Get the garbage collector instance
     */
    GarbageCollector* gc() { return gc_.get(); }
    const GarbageCollector* gc() const { return gc_.get(); }

    /**
     * Enable/disable automatic GC
     * When disabled, GC must be triggered manually
     */
    void setAutoGC(bool enabled) { autoGC_ = enabled; }

    /**
     * Check if auto-GC is enabled
     */
    bool isAutoGCEnabled() const { return autoGC_; }

    /**
     * Manually trigger a GC collection
     * Returns number of bytes freed
     */
    size_t collect();

    /**
     * Manually trigger a minor collection
     */
    size_t collectMinor();

    /**
     * Manually trigger a major collection
     */
    size_t collectMajor();

    // ========== Memory Statistics ==========

    /**
     * Get total allocated memory (young + old + large)
     */
    size_t totalAllocated() const;

    /**
     * Get young generation size
     */
    size_t youngGenSize() const;

    /**
     * Get old generation size
     */
    size_t oldGenSize() const;

    /**
     * Get large object size
     */
    size_t largeObjectSize() const;

    /**
     * Get memory usage percentage (0.0 to 1.0)
     */
    float memoryUsagePercent() const;

    /**
     * Print memory statistics
     */
    void printStats(std::ostream& out) const;

    // ========== Configuration ==========

    /**
     * Get current GC configuration
     */
    const GCConfig& gcConfig() const;

    /**
     * Update GC configuration
     */
    void setGCConfig(const GCConfig& config);

    /**
     * Enable/disable memory logging
     */
    void setLogging(bool enabled) { logging_ = enabled; }

    // ========== Write Barrier ==========

    /**
     * Write barrier for object field updates
     * Must be called when storing a young-gen object into an old-gen object
     *
     * Example:
     *   oldStruct->fields[0] = youngValue;
     *   memoryManager.writeBarrier(oldStruct, youngValue.asObject);
     */
    void writeBarrier(void* oldGenObject, void* youngGenObject);

    // ========== Debugging & Profiling ==========

    /**
     * Verify heap integrity (debug only)
     * Checks for corruption, dangling pointers, etc.
     * Returns true if heap is valid
     */
    bool verifyHeap() const;

    /**
     * Dump memory layout (debug only)
     * Prints all allocated objects and their addresses
     */
    void dumpHeap(std::ostream& out) const;

    /**
     * Get allocation count (total objects allocated since start)
     */
    size_t allocationCount() const { return allocationCount_; }

    /**
     * Reset allocation counters
     */
    void resetCounters();

private:
    // ========== Internal Allocation Helpers ==========

    /**
     * Try to allocate from young generation (arena)
     * Returns nullptr if arena is full
     */
    void* allocateYoung(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId);

    /**
     * Allocate from old generation (free list)
     * Used for promoted objects or when young gen is full
     */
    void* allocateOld(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId);

    /**
     * Allocate a large object (page allocator)
     * Used for objects >= 4KB
     */
    void* allocateLarge(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId);

    /**
     * Initialize object header with GC metadata
     */
    void initializeObjectHeader(void* obj, size_t size, ObjectHeader::ObjectKind kind,
                                uint32_t typeId, bool isYoung);

    /**
     * Check if allocation should trigger GC
     * Called before each allocation
     */
    void checkGCTrigger(size_t requestedSize);

    /**
     * Slow path: allocation failed, try to GC and retry
     * Returns nullptr if still can't allocate after GC
     */
    void* allocateWithGC(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId);

    // ========== State ==========

    /// Reference to the VM
    VM* vm_;

    /// Garbage collector
    std::unique_ptr<GarbageCollector> gc_;

    /// Allocators for different strategies
    std::unique_ptr<Arena> youngArena_;              ///< Young generation (bump-pointer)
    std::unique_ptr<FreeListAllocator> oldAllocator_;  ///< Old generation (free list)
    std::unique_ptr<PageAllocator> largeAllocator_;    ///< Large objects (pages)

    /// Optional slab allocators for common sizes
    /// TODO: Initialize these for common struct/array sizes
    std::vector<std::unique_ptr<SlabAllocator>> slabAllocators_;

    /// Configuration
    bool autoGC_ = true;            ///< Automatically trigger GC
    bool logging_ = false;          ///< Log allocation/GC activity

    /// Statistics
    size_t allocationCount_ = 0;    ///< Total allocations
    size_t youngAllocBytes_ = 0;    ///< Bytes allocated in young gen
    size_t oldAllocBytes_ = 0;      ///< Bytes allocated in old gen
    size_t largeAllocBytes_ = 0;    ///< Bytes allocated as large objects

    /// Large object threshold (allocate via pages instead of heap)
    static constexpr size_t LARGE_OBJECT_THRESHOLD = 4096;
};

// ============================================================================
// Memory Utilities
// ============================================================================

/**
 * Get the size of an object from its header
 */
size_t getObjectSize(void* obj);

/**
 * Get the kind of an object from its header
 */
ObjectHeader::ObjectKind getObjectKind(void* obj);

/**
 * Check if an object is marked (for GC)
 */
bool isMarked(void* obj);

/**
 * Set the mark bit on an object
 */
void setMarked(void* obj, bool marked);

/**
 * Get the generation of an object (0 = young, 1 = old)
 */
uint8_t getGeneration(void* obj);

/**
 * Set the generation of an object
 */
void setGeneration(void* obj, uint8_t generation);

/**
 * Get the age of an object (GC cycles survived)
 */
uint8_t getAge(void* obj);

/**
 * Increment the age of an object
 */
void incrementAge(void* obj);

/**
 * Calculate the size needed for a struct object
 */
size_t calculateStructSize(uint32_t fieldCount);

/**
 * Calculate the size needed for an array object
 */
size_t calculateArraySize(uint32_t elementCount);

} // namespace volta::vm
