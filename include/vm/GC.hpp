#pragma once

#include "Object.hpp"
#include "Value.hpp"
#include <vector>

namespace volta::vm {

// Forward declaration
class VM;

/**
 * @brief Mark-and-Sweep Garbage Collector
 *
 * Automatic memory management for heap-allocated objects.
 *
 * Algorithm:
 * 1. Mark Phase: Trace from roots, mark all reachable objects
 * 2. Sweep Phase: Free all unmarked objects
 *
 * GC Roots (starting points for tracing):
 * - VM registers (current frame)
 * - Call stack (all frames)
 * - Global variables
 * - Constant pool
 */
class GarbageCollector {
public:
    /**
     * @brief Construct GC for a VM
     * @param vm VM instance to manage
     */
    explicit GarbageCollector(VM* vm);

    // ========================================================================
    // Main GC Interface
    // ========================================================================

    /**
     * @brief Run garbage collection cycle
     *
     * Implementation:
     * 1. Call mark_roots() to mark all reachable objects
     * 2. Call sweep() to free unmarked objects
     * 3. Update next_gc_threshold based on bytes_allocated
     * 4. Print GC stats if debug mode enabled
     *
     * Trigger conditions:
     * - bytes_allocated >= next_gc_threshold
     * - Explicit gc.collect() call
     * - Out of memory
     */
    void collect();

    /**
     * @brief Pause GC (for FFI calls)
     *
     * During C function calls, GC must not run because:
     * - C code may hold pointers to Volta objects
     * - Moving objects would invalidate those pointers
     *
     * Implementation:
     * 1. Increment pause_count_
     * 2. While paused, allocations can still occur but collect() is no-op
     */
    void pause();

    /**
     * @brief Resume GC after pause
     *
     * Implementation:
     * 1. Decrement pause_count_
     * 2. If pause_count_ == 0 and should_collect_, run collection
     */
    void resume();

    /**
     * @brief Check if GC is currently paused
     */
    bool is_paused() const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get number of bytes currently allocated
     */
    size_t bytes_allocated() const;

    /**
     * @brief Get threshold for next GC
     */
    size_t next_gc_threshold() const;

    /**
     * @brief Get total number of objects allocated
     */
    size_t object_count() const;

    /**
     * @brief Get number of GC cycles run
     */
    size_t collection_count() const;

    // Tuning parameters
    static constexpr double GC_HEAP_GROW_FACTOR = 2.0;  ///< Next GC at 2x current size
    static constexpr size_t INITIAL_GC_THRESHOLD = 1024 * 1024;  ///< 1 MB
private:
    // ========================================================================
    // Mark Phase
    // ========================================================================

    /**
     * @brief Mark all reachable objects starting from roots
     *
     * Implementation:
     * 1. Mark current frame registers
     * 2. Mark all call stack frames
     * 3. Mark global variables
     * 4. Mark constant pool objects
     */
    void mark_roots();

    /**
     * @brief Mark a single value
     * @param value Value to mark
     *
     * Implementation:
     * 1. If value is not OBJECT type, return
     * 2. Call mark_object(value->as.obj)
     */
    void mark_value(const Value& value);

    /**
     * @brief Mark an object and its references
     * @param obj Object to mark
     *
     * Implementation:
     * 1. If obj == nullptr, return
     * 2. If obj->marked == true, return (already visited)
     * 3. Set obj->marked = true
     * 4. Add obj to gray_stack_ for processing
     * 5. Process gray stack: for each object, mark its references
     *
     * Reference tracing by type:
     * - STRING: No references
     * - ARRAY: Mark each element
     * - STRUCT: Mark each field
     * - CLOSURE: Mark function and upvalues
     * - NATIVE_FUNCTION: No references
     */
    void mark_object(Object* obj);

    /**
     * @brief Process gray stack (mark object's references)
     *
     * Gray stack contains objects that are marked but whose
     * children haven't been processed yet (tri-color marking)
     *
     * Implementation:
     * While gray_stack_ not empty:
     * 1. Pop object from stack
     * 2. Mark all objects it references
     */
    void process_gray_stack();

    // ========================================================================
    // Sweep Phase
    // ========================================================================

    /**
     * @brief Free all unmarked objects
     *
     * Implementation:
     * 1. Walk VM's object linked list
     * 2. For each object:
     *    - If marked: unmark it (for next GC cycle), keep it
     *    - If not marked: free it, remove from list
     * 3. Update bytes_allocated
     */
    void sweep();

    /**
     * @brief Free a single object
     * @param obj Object to free
     *
     * Implementation:
     * 1. Calculate object size using object_size()
     * 2. Update bytes_allocated -= size
     * 3. Delete object based on type (calls destructor)
     * 4. Update object_count_
     */
    void free_object(Object* obj);

    // ========================================================================
    // State
    // ========================================================================

    VM* vm_;                            ///< VM being managed
    std::vector<Object*> gray_stack_;   ///< Objects to process in mark phase

    // Pause state
    int pause_count_;                   ///< Nesting count for pause/resume
    bool should_collect_;               ///< Collection requested while paused

    // Statistics
    size_t collection_count_;           ///< Number of GC cycles run
};

} // namespace volta::vm
