#pragma once

#include "VM.hpp"
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <memory>

namespace volta::vm {

// ============================================================================
// GC Object Header Extensions
// ============================================================================

/**
 * GC metadata stored in object header
 * This extends the ObjectHeader with GC-specific information
 */
struct GCHeader {
    /// Mark bit for mark-and-sweep (0 = white, 1 = black/marked)
    uint8_t marked : 1;

    /// Generation this object belongs to (0 = young, 1 = old)
    uint8_t generation : 1;

    /// Age (number of GC cycles survived in current generation)
    uint8_t age : 6;

    /// Pointer to next object in free list or allocation list
    void* next;
};

// ============================================================================
// GC Statistics
// ============================================================================

/**
 * Statistics about GC activity (for profiling and debugging)
 */
struct GCStats {
    // Collection counts
    uint64_t minorCollections = 0;      ///< Number of young generation collections
    uint64_t majorCollections = 0;      ///< Number of full heap collections

    // Memory stats
    size_t youngGenSize = 0;            ///< Current size of young generation (bytes)
    size_t oldGenSize = 0;              ///< Current size of old generation (bytes)
    size_t totalAllocated = 0;          ///< Total bytes allocated since start
    size_t totalFreed = 0;              ///< Total bytes freed by GC

    // Object counts
    size_t youngGenObjects = 0;         ///< Number of objects in young generation
    size_t oldGenObjects = 0;           ///< Number of objects in old generation

    // Performance metrics
    uint64_t totalGCTimeMs = 0;         ///< Total time spent in GC (milliseconds)
    uint64_t lastGCTimeMs = 0;          ///< Time of last GC cycle (milliseconds)

    // Promotion tracking
    uint64_t objectsPromoted = 0;       ///< Total objects promoted from young to old gen
};

// ============================================================================
// Garbage Collector Configuration
// ============================================================================

/**
 * Configuration parameters for the generational GC
 */
struct GCConfig {
    // Heap size limits
    size_t youngGenMaxSize = 2 * 1024 * 1024;    ///< Max young gen size (2MB default)
    size_t oldGenMaxSize = 16 * 1024 * 1024;     ///< Max old gen size (16MB default)
    size_t totalMaxSize = 64 * 1024 * 1024;      ///< Max total heap size (64MB default)

    // GC trigger thresholds
    float youngGenThreshold = 0.75f;    ///< Trigger minor GC when 75% full
    float oldGenThreshold = 0.80f;      ///< Trigger major GC when 80% full

    // Promotion parameters
    uint8_t promotionAge = 3;           ///< Promote objects after surviving 3 minor GCs
    float promotionRatio = 0.5f;        ///< Promote if >50% of young gen survives

    // Performance tuning
    bool enableIncrementalCollection = false;  ///< Enable incremental collection (future)
    bool enableConcurrentMarking = false;      ///< Enable concurrent marking (future)
    size_t allocationBumpSize = 64;            ///< Bump pointer allocation size (bytes)
};

// ============================================================================
// Generational Garbage Collector
// ============================================================================

/**
 * Generational garbage collector for Volta VM
 *
 * Architecture:
 * - Two generations: young (nursery) and old (tenured)
 * - Young generation: newly allocated objects, collected frequently
 * - Old generation: long-lived objects, collected infrequently
 * - Write barrier: tracks old-to-young references (remembered set)
 *
 * Collection strategies:
 * - Minor GC: collect young generation only (fast, frequent)
 * - Major GC: collect entire heap (slow, infrequent)
 *
 * Algorithm:
 * - Mark and sweep for both generations
 * - Promotion: objects surviving multiple minor GCs move to old gen
 * - Write barrier: maintain remembered set for old->young pointers
 *
 * Future optimizations:
 * - Copying collector for young generation (semi-space)
 * - Incremental marking for major GC
 * - Concurrent marking and sweeping
 */
class GarbageCollector {
public:
    /**
     * Create a garbage collector for a VM instance
     */
    explicit GarbageCollector(VM* vm, const GCConfig& config = GCConfig{});

    ~GarbageCollector();

    // ========== Allocation ==========

    /**
     * Allocate a struct object in the young generation
     * May trigger GC if heap is full
     * Returns pointer to allocated StructObject
     */
    StructObject* allocateStruct(uint32_t typeId, uint32_t fieldCount);

    /**
     * Allocate an array object in the young generation
     * May trigger GC if heap is full
     * Returns pointer to allocated ArrayObject
     */
    ArrayObject* allocateArray(uint32_t length);

    /**
     * Allocate a raw object in the young generation
     * Generic allocation for custom object types
     */
    void* allocate(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId = 0);

    // ========== Collection ==========

    /**
     * Perform a minor collection (young generation only)
     * Fast, collects only recently allocated objects
     * Returns number of bytes freed
     */
    size_t collectMinor();

    /**
     * Perform a major collection (full heap)
     * Slow, collects both young and old generations
     * Returns number of bytes freed
     */
    size_t collectMajor();

    /**
     * Trigger collection if necessary based on heap thresholds
     * Returns true if collection was performed
     */
    bool collectIfNeeded();

    /**
     * Force an immediate collection (for testing/debugging)
     */
    void forceCollect();

    // ========== Write Barrier ==========

    /**
     * Write barrier: call when an old-gen object references a young-gen object
     * Updates the remembered set to track old->young pointers
     *
     * Example usage:
     *   oldObject->field = youngObject;
     *   gc.writeBarrier(oldObject, youngObject);
     */
    void writeBarrier(void* oldGenObject, void* youngGenObject);

    // ========== GC Roots ==========

    /**
     * Register a GC root (global variable, stack value, etc.)
     * GC roots are starting points for mark phase
     * The VM automatically registers stack/global roots before each collection
     */
    void addRoot(Value* root);

    /**
     * Remove a GC root (when it goes out of scope)
     */
    void removeRoot(Value* root);

    /**
     * Clear all GC roots (called internally before each collection)
     */
    void clearRoots();

    // ========== Configuration ==========

    /**
     * Get the current GC configuration
     */
    const GCConfig& config() const { return config_; }

    /**
     * Update GC configuration (applies immediately)
     */
    void setConfig(const GCConfig& config);

    // ========== Statistics & Debugging ==========

    /**
     * Get GC statistics
     */
    const GCStats& stats() const { return stats_; }

    /**
     * Reset GC statistics
     */
    void resetStats();

    /**
     * Enable/disable GC logging (prints collection activity)
     */
    void setLogging(bool enabled) { logging_ = enabled; }

    /**
     * Check if an object is in the young generation
     */
    bool isYoung(void* obj) const;

    /**
     * Check if an object is in the old generation
     */
    bool isOld(void* obj) const;

    /**
     * Get total heap size (young + old)
     */
    size_t totalHeapSize() const { return stats_.youngGenSize + stats_.oldGenSize; }

    /**
     * Print heap statistics to output stream
     */
    void printStats(std::ostream& out) const;

private:
    // ========== Mark Phase ==========

    /**
     * Mark phase: traverse from roots and mark all reachable objects
     * For minor GC: marks young gen objects + remembered set
     * For major GC: marks all objects
     */
    void mark(bool fullHeap);

    /**
     * Mark a single value (if it's an object reference)
     */
    void markValue(const Value& value);

    /**
     * Mark an object and recursively mark its references
     */
    void markObject(void* obj);

    /**
     * Mark all roots (stack, globals, call frames)
     */
    void markRoots();

    /**
     * Mark objects in the remembered set (old->young references)
     */
    void markRememberedSet();

    // ========== Sweep Phase ==========

    /**
     * Sweep phase: free unmarked objects and reset mark bits
     * For minor GC: sweeps young generation only
     * For major GC: sweeps both generations
     * Returns number of bytes freed
     */
    size_t sweep(bool fullHeap);

    /**
     * Sweep a single generation
     * Returns number of bytes freed
     */
    size_t sweepGeneration(std::vector<void*>& generation, bool isYoung);

    /**
     * Free a single object (return to free list or OS)
     */
    void freeObject(void* obj);

    // ========== Promotion ==========

    /**
     * Promote objects from young to old generation
     * Called during minor GC for objects that have survived enough cycles
     */
    void promoteObjects();

    /**
     * Promote a single object to old generation
     */
    void promoteObject(void* obj);

    // ========== Helpers ==========

    /**
     * Get GC header for an object
     */
    GCHeader* getGCHeader(void* obj) const;

    /**
     * Check if collection should be triggered
     */
    bool shouldCollect() const;

    /**
     * Check if minor collection should be triggered
     */
    bool shouldCollectMinor() const;

    /**
     * Check if major collection should be triggered
     */
    bool shouldCollectMajor() const;

    /**
     * Update statistics after collection
     */
    void updateStats(bool wasMinor, size_t bytesFreed, uint64_t durationMs);

    // ========== State ==========

    /// Reference to the VM (for accessing stack, globals, etc.)
    VM* vm_;

    /// GC configuration
    GCConfig config_;

    /// GC statistics
    GCStats stats_;

    /// Object lists by generation
    std::vector<void*> youngGenObjects_;
    std::vector<void*> oldGenObjects_;

    /// Remembered set: old generation objects that reference young generation
    /// Only these old-gen objects need to be scanned during minor GC
    std::unordered_set<void*> rememberedSet_;

    /// GC roots (global variables, stack references)
    std::vector<Value*> roots_;

    /// Free lists for allocation (size-segregated for efficiency)
    /// TODO: implement free list allocation for better performance
    std::unordered_map<size_t, std::vector<void*>> freeLists_;

    /// Logging flag
    bool logging_ = false;

    /// Collection in progress flag (prevent nested collections)
    bool collecting_ = false;
};

} // namespace volta::vm
