#ifndef VOLTA_GC_H
#define VOLTA_GC_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VOLTA_GC_MODE
#define VOLTA_GC_MODE VOLTA_GC_BOEHM  // Default to Boehm GC
#endif

#define VOLTA_GC_BOEHM  1
#define VOLTA_GC_MANUAL 2
#define VOLTA_GC_CUSTOM 3

/**
 * Initialize the garbage collector
 * Must be called before any allocations.
 * For Boehm GC, this calls GC_INIT().
 */
void volta_gc_init(void);

/**
 * Shutdown the garbage collector
 * Optional for GC (automatic cleanup), required for manual mode.
 * For Boehm GC, this is a no-op.
 */
void volta_gc_shutdown(void);

/**
 * Check if GC is enabled
 * @return true if using GC (Boehm or custom), false if manual
 */
bool volta_gc_is_enabled(void);

/**
 * Allocate memory from the GC heap
 * For Boehm GC: Uses GC_malloc() - memory is automatically reclaimed
 * For manual: Uses malloc() - must call volta_gc_free()
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* volta_gc_malloc(size_t size);

/**
 * Allocate zeroed memory from the GC heap
 * Equivalent to calloc() but GC-aware.
 *
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
void* volta_gc_calloc(size_t count, size_t size);

/**
 * Reallocate memory (resize existing allocation)
 * For Boehm GC: Uses GC_realloc()
 * For manual: Uses realloc()
 *
 * @param ptr Existing allocation (can be NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* volta_gc_realloc(void* ptr, size_t new_size);

/**
 * Free memory explicitly (only for manual mode)
 * For Boehm GC: This is a no-op (memory is reclaimed automatically)
 * For manual: Calls free()
 *
 * NOTE: In GC mode, you DON'T need to call this. The GC handles it.
 * This exists for compatibility with manual memory management.
 *
 * @param ptr Memory to free (can be NULL)
 */
void volta_gc_free(void* ptr);

/**
 * Trigger a garbage collection cycle
 * For Boehm GC: Calls GC_gcollect()
 * For manual: No-op
 *
 * This is typically not needed - the GC runs automatically.
 */
void volta_gc_collect(void);

/**
 * Register a finalizer for an object (called before object is freed)
 * For Boehm GC: Uses GC_register_finalizer()
 * For manual: Not supported (no-op)
 *
 * @param obj Pointer to object
 * @param finalizer Function to call before freeing (void (*)(void*, void*))
 * @param client_data Data to pass to finalizer
 */
void volta_gc_register_finalizer(void* obj, void (*finalizer)(void*, void*), void* client_data);

/**
 * Get total heap size in bytes
 * For Boehm GC: Returns GC_get_heap_size()
 * For manual: Returns 0 (not tracked)
 *
 * @return Heap size in bytes
 */
size_t volta_gc_get_heap_size(void);

/**
 * Get amount of free space in heap
 * For Boehm GC: Returns GC_get_free_bytes()
 * For manual: Returns 0 (not tracked)
 *
 * @return Free bytes in heap
 */
size_t volta_gc_get_free_bytes(void);

/**
 * GC statistics structure
 */
typedef struct VoltaGCStats {
    size_t total_allocations;   // Total number of allocations
    size_t total_bytes;          // Total bytes allocated
    size_t heap_size;            // Current heap size
    size_t free_bytes;           // Current free bytes
    size_t num_collections;      // Number of GC cycles run
} VoltaGCStats;

/**
 * Get GC statistics
 * @param stats Pointer to stats structure to fill
 */
void volta_gc_get_stats(VoltaGCStats* stats);

/**
 * Reset GC statistics counters
 */
void volta_gc_reset_stats(void);

/**
 * Register a root pointer with the GC (prevents collection)
 * For Boehm GC: Uses GC_add_roots()
 * For manual: No-op
 *
 * This is needed if you have global pointers to GC-allocated objects.
 *
 * @param start Start of root region
 * @param end End of root region
 */
void volta_gc_add_roots(void* start, void* end);

/**
 * Remove a root pointer region
 * For Boehm GC: Uses GC_remove_roots()
 * For manual: No-op
 *
 * @param start Start of root region
 * @param end End of root region
 */
void volta_gc_remove_roots(void* start, void* end);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_GC_H
