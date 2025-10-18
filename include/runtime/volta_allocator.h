#ifndef VOLTA_ALLOCATOR_H
#define VOLTA_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Allocate memory
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
static inline void* volta_alloc(size_t size);

/**
 * Allocate zeroed memory
 *
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
static inline void* volta_alloc_zeroed(size_t count, size_t size);

/**
 * Reallocate memory (resize allocation)
 *
 * @param ptr Existing allocation (can be NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
static inline void* volta_realloc(void* ptr, size_t new_size);

/**
 * Free memory explicitly (may be no-op for GC)
 * Implementation delegates to volta_gc_free() or other allocator.
 *
 * @param ptr Memory to free (can be NULL)
 */
static inline void volta_free(void* ptr);

/**
 * Allocate memory for an array of elements (type-safe)
 * Equivalent to: malloc(count * element_size)
 *
 * @param count Number of elements
 * @param element_size Size of each element
 * @return Pointer to allocated array, or NULL on failure
 */
static inline void* volta_alloc_array(size_t count, size_t element_size);

/**
 * Allocate and zero memory for an array (type-safe)
 * Equivalent to: calloc(count, element_size)
 *
 * @param count Number of elements
 * @param element_size Size of each element
 * @return Pointer to allocated array, or NULL on failure
 */
static inline void* volta_alloc_array_zeroed(size_t count, size_t element_size);

/**
 * VOLTA_ALLOCATOR_BACKEND defines which allocator to use:
 * - VOLTA_ALLOCATOR_GC: Use volta_gc.h (GC-managed)
 * - VOLTA_ALLOCATOR_MALLOC: Use stdlib malloc/free (manual)
 * - VOLTA_ALLOCATOR_CUSTOM: Use custom allocator (you provide impl)
 *
 * Set via compiler flag: -DVOLTA_ALLOCATOR_BACKEND=VOLTA_ALLOCATOR_GC
 */
#ifndef VOLTA_ALLOCATOR_BACKEND
#define VOLTA_ALLOCATOR_BACKEND VOLTA_ALLOCATOR_GC
#endif

#define VOLTA_ALLOCATOR_GC     1
#define VOLTA_ALLOCATOR_MALLOC 2
#define VOLTA_ALLOCATOR_CUSTOM 3

#if VOLTA_ALLOCATOR_BACKEND == VOLTA_ALLOCATOR_GC
    #include "volta_gc.h"

    static inline void* volta_alloc(size_t size) {
        return volta_gc_malloc(size);
    }

    static inline void* volta_alloc_zeroed(size_t count, size_t size) {
        return volta_gc_calloc(count, size);
    }

    static inline void* volta_realloc(void* ptr, size_t new_size) {
        return volta_gc_realloc(ptr, new_size);
    }

    static inline void volta_free(void* ptr) {
        volta_gc_free(ptr); 
    }

#elif VOLTA_ALLOCATOR_BACKEND == VOLTA_ALLOCATOR_MALLOC
    #include <stdlib.h>
    #include <string.h>

    static inline void* volta_alloc(size_t size) {
        return malloc(size);
    }

    static inline void* volta_alloc_zeroed(size_t count, size_t size) {
        return calloc(count, size);
    }

    static inline void* volta_realloc(void* ptr, size_t new_size) {
        return realloc(ptr, new_size);
    }

    static inline void volta_free(void* ptr) {
        free(ptr);
    }

#elif VOLTA_ALLOCATOR_BACKEND == VOLTA_ALLOCATOR_CUSTOM
    extern void* volta_alloc(size_t size);
    extern void* volta_alloc_zeroed(size_t count, size_t size);
    extern void* volta_realloc(void* ptr, size_t new_size);
    extern void volta_free(void* ptr);

#else
    #error "Invalid VOLTA_ALLOCATOR_BACKEND value"
#endif

static inline void* volta_alloc_array(size_t count, size_t element_size) {
    // Check for overflow: count * element_size
    if (count > 0 && element_size > SIZE_MAX / count) {
        return NULL;  // Would overflow
    }
    return volta_alloc(count * element_size);
}

static inline void* volta_alloc_array_zeroed(size_t count, size_t element_size) {
    return volta_alloc_zeroed(count, element_size);
}

/**
 * Enable allocation tracking (records allocations for debugging)
 * This is optional and may not be supported by all backends.
 */
void volta_allocator_enable_tracking(void);

/**
 * Disable allocation tracking
 */
void volta_allocator_disable_tracking(void);

/**
 * Get total number of bytes allocated (if tracking enabled)
 * @return Total bytes allocated, or 0 if tracking disabled
 */
size_t volta_allocator_get_total_allocated(void);

/**
 * Get current number of bytes in use (allocated - freed)
 * @return Bytes in use, or 0 if tracking disabled
 */
size_t volta_allocator_get_bytes_in_use(void);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_ALLOCATOR_H
