#pragma once

#include "Object.hpp"
#include "GCRootProvider.hpp"
#include "TypeRegistry.hpp"
#include <cstddef>
#include <unordered_set>

namespace Volta {
namespace GC {

/**
 * Heap space for a single generation
 *
 * Uses semi-space copying (from-space and to-space).
 * Allocation is done via bump pointer in from-space.
 * During GC, live objects are copied to to-space, then spaces are swapped.
 */
struct HeapSpace {
    void*  fromSpace;     // Active space (allocate here)
    void*  toSpace;       // Reserve space (copy here during GC)
    size_t size;          // Size of each space (total = 2 * size)
    void*  bump;          // Bump pointer (next free location)
    void*  limit;         // End of from-space

    HeapSpace()
        : fromSpace(nullptr)
        , toSpace(nullptr)
        , size(0)
        , bump(nullptr)
        , limit(nullptr)
    {}
};

/**
 * Collection type (for algorithms)
 */
enum CollectionType {
    MINOR_COLLECTION,  // Collect nursery only
    MAJOR_COLLECTION,  // Collect entire heap
};

/**
 * Generational Garbage Collector
 *
 * Two-generation copying collector:
 * - Nursery (young generation): 2 MB, frequent collections
 * - Old generation: 16 MB initial, rare collections, growable
 *
 * Features:
 * - Bump pointer allocation (fast path)
 * - Minor GC (nursery only) and Major GC (full heap)
 * - Automatic promotion after 2 survived collections
 * - Write barriers with remembered set
 * - Automatic compaction (no fragmentation)
 *
 * Usage:
 *   GarbageCollector gc;
 *   gc.setRootProvider(&myVM);
 *
 *   StringObject* str = gc.allocateString(5);
 *   ArrayObject* arr = gc.allocateArray(10, TYPE_INT64, 8);
 *
 *   // When writing object pointers, use write barrier:
 *   arr->elements[0] = str;
 *   gc.writeBarrier((Object*)arr, (Object*)str);
 */
class GarbageCollector {
public:
    /**
     * Constructor
     *
     * @param nurserySize Size of nursery (per space, total = 2x). Default: 1 MB
     * @param oldGenSize Size of old gen (per space, total = 2x). Default: 8 MB
     */
    explicit GarbageCollector(size_t nurserySize = 1024 * 1024,
                              size_t oldGenSize = 8 * 1024 * 1024);

    /**
     * Destructor - frees all heap memory
     */
    ~GarbageCollector();

    // Prevent copying
    GarbageCollector(const GarbageCollector&) = delete;
    GarbageCollector& operator=(const GarbageCollector&) = delete;

    /**
     * Set the root provider (must be called before allocation)
     *
     * @param provider Pointer to root provider (VM, runtime, etc.)
     */
    void setRootProvider(GCRootProvider* provider);

    /**
     * Set the type registry (must be called before allocation)
     *
     * @param registry Pointer to type registry (for tracing type info)
     */
    void setTypeRegistry(TypeRegistry* registry);

    // ========== ALLOCATION ==========

    /**
     * Allocate a string object
     *
     * @param length Number of characters (excluding null terminator)
     * @return Pointer to allocated string object
     * @throws std::runtime_error if out of memory
     */
    StringObject* allocateString(size_t length);

    /**
     * Allocate an array object
     *
     * @param elementCount Number of elements
     * @param elementTypeId Type ID of elements (for tracing)
     * @param elementSize Size of each element in bytes
     * @return Pointer to allocated array object
     * @throws std::runtime_error if out of memory
     */
    ArrayObject* allocateArray(size_t elementCount, uint32_t elementTypeId, size_t elementSize);

    /**
     * Allocate a struct object
     *
     * @param structTypeId Type ID of struct (for field layout lookup)
     * @param fieldDataSize Total size of field data in bytes
     * @return Pointer to allocated struct object
     * @throws std::runtime_error if out of memory
     */
    StructObject* allocateStruct(uint32_t structTypeId, size_t fieldDataSize);

    // ========== WRITE BARRIER ==========

    /**
     * Write barrier - MUST be called when writing object pointers
     *
     * Tracks old→young pointers for generational GC correctness.
     *
     * Call this whenever you write an object pointer to a field:
     *   obj->field = value;
     *   gc.writeBarrier(obj, value);
     *
     * @param obj Object being modified
     * @param value Object being written to obj's field
     */
    void writeBarrier(Object* obj, Object* value);

    // ========== GARBAGE COLLECTION ==========

    /**
     * Perform a minor GC (collect nursery only)
     *
     * Traces from roots + remembered set, copies live objects to to-space.
     * Promotes objects that have survived 2+ collections to old gen.
     */
    void minorGC();

    /**
     * Perform a major GC (collect entire heap)
     *
     * Traces from roots, copies all live objects, compacts both generations.
     * Grows old gen if needed (when >80% full after collection).
     */
    void majorGC();

    // ========== STATISTICS ==========

    /**
     * Get bytes currently used in nursery
     */
    size_t getNurseryUsed() const;

    /**
     * Get bytes currently used in old generation
     */
    size_t getOldGenUsed() const;

    /**
     * Get total nursery size (one space)
     */
    size_t getNurserySize() const { return nursery.size; }

    /**
     * Get total old gen size (one space)
     */
    size_t getOldGenSize() const { return oldGen.size; }

    /**
     * Get number of minor GCs performed
     */
    size_t getNumMinorGCs() const { return numMinorGCs; }

    /**
     * Get number of major GCs performed
     */
    size_t getNumMajorGCs() const { return numMajorGCs; }

    /**
     * Get total bytes allocated (lifetime)
     */
    size_t getTotalAllocated() const { return totalAllocated; }

private:
    // ========== HEAP SPACES ==========

    HeapSpace nursery;  // Young generation
    HeapSpace oldGen;   // Old generation

    // ========== GC STATE ==========

    GCRootProvider* rootProvider;                     // Provides roots for tracing
    TypeRegistry* typeRegistry;                       // Provides type info for tracing
    std::unordered_set<Object*> rememberedSet;        // Old->young pointers
    CollectionType currentCollection;                 // Current collection type

    // ========== STATISTICS ==========

    size_t numMinorGCs;      // Count of minor GCs
    size_t numMajorGCs;      // Count of major GCs
    size_t totalAllocated;   // Total bytes allocated (lifetime)

    // ========== INTERNAL HELPERS ==========

    /**
     * Initialize a heap space
     *
     * @param space Heap space to initialize
     * @param size Size of each semi-space (total = 2 * size)
     */
    void initHeapSpace(HeapSpace& space, size_t size);

    /**
     * Free a heap space
     */
    void freeHeapSpace(HeapSpace& space);

    /**
     * Try to allocate in nursery (fast path)
     *
     * @param size Size in bytes (must be 8-byte aligned)
     * @return Pointer to allocated memory, or nullptr if not enough space
     */
    void* tryAllocateNursery(size_t size);

    /**
     * Allocate in old generation
     *
     * @param size Size in bytes (must be 8-byte aligned)
     * @return Pointer to allocated memory
     * @throws std::runtime_error if not enough space
     */
    void* allocateOldGen(size_t size);

    /**
     * Generic allocation with GC fallback
     *
     * Tries nursery, triggers GCs if needed, handles OOM.
     *
     * @param size Size in bytes (must be 8-byte aligned)
     * @return Pointer to allocated memory
     * @throws std::runtime_error if out of memory
     */
    void* allocate(size_t size);

    /**
     * Copy an object during GC
     *
     * Handles forwarding pointers, recursion, and promotion logic.
     *
     * @param obj Object to copy
     * @return Pointer to new location (may be same object if already copied)
     */
    Object* copyObject(Object* obj);

    /**
     * Copy object to nursery to-space
     *
     * @param obj Object to copy
     * @return Pointer to copied object in nursery to-space
     */
    Object* copyToNursery(Object* obj);

    /**
     * Copy object to old gen to-space
     *
     * @param obj Object to copy
     * @return Pointer to copied object in old gen to-space
     */
    Object* copyToOldGen(Object* obj);

    /**
     * Trace object fields (recursively copy referenced objects)
     *
     * @param obj Object whose fields to trace
     */
    void traceObject(Object* obj);

    /**
     * Swap from-space and to-space
     *
     * @param space Heap space to swap
     */
    void swapSpaces(HeapSpace& space);

    /**
     * Grow old generation (double size)
     */
    void growOldGen();

    /**
     * Check if a pointer points into a heap space
     *
     * @param ptr Pointer to check
     * @param space Heap space to check against
     * @return True if ptr is in space's from-space
     */
    bool isInSpace(void* ptr, const HeapSpace& space) const;

    /**
     * Check if old gen should be grown
     *
     * @return True if old gen is >80% full after major GC
     */
    bool shouldGrowOldGen() const;
};

// ========== CONSTANTS ==========

/**
 * Promotion age threshold
 * Objects that survive this many collections are promoted to old gen
 */
constexpr uint8_t PROMOTION_AGE = 2;

/**
 * Large object threshold
 * Objects larger than this are allocated directly in old gen
 */
constexpr size_t LARGE_OBJECT_THRESHOLD = 256 * 1024; // 256 KB

/**
 * Old gen growth threshold
 * Grow old gen when this % full after major GC
 */
constexpr double OLD_GEN_GROWTH_THRESHOLD = 0.8; // 80%

} // namespace GC
} // namespace Volta
