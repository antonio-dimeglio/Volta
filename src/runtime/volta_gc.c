#include "runtime/volta_gc.h"
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal statistics tracking
static size_t gc_total_allocations = 0;
static size_t gc_total_bytes = 0;
static size_t gc_num_collections = 0;

void volta_gc_init(void) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        GC_INIT();
    } else {
        printf("Unsupported GC mode.");
        exit(1);
    }
}

void volta_gc_shutdown(void) {
    if (VOLTA_GC_MODE != VOLTA_GC_BOEHM) {
        printf("Unsupported GC mode.");
        exit(1);
    } 
}

bool volta_gc_is_enabled(void) {
    return VOLTA_GC_MODE == VOLTA_GC_BOEHM ||
            VOLTA_GC_MODE == VOLTA_GC_CUSTOM;
}

void* volta_gc_malloc(size_t size) {
    void* ptr = NULL;
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        ptr = GC_malloc(size);
    } else if (VOLTA_GC_MODE == VOLTA_GC_MANUAL) {
        ptr = malloc(size);
    }

    if (ptr) {
        gc_total_allocations++;
        gc_total_bytes += size;
    }

    return ptr;
}

void* volta_gc_calloc(size_t count, size_t size) {
    void* ptr = NULL;
    size_t total_size = count * size;

    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        ptr = GC_malloc(total_size);
        if (ptr) memset(ptr, 0, total_size);
    } else if (VOLTA_GC_MODE == VOLTA_GC_MANUAL) {
        ptr = calloc(count, size);
    }

    if (ptr) {
        gc_total_allocations++;
        gc_total_bytes += total_size;
    }

    return ptr;
}

void* volta_gc_realloc(void* ptr, size_t new_size) {
    // realloc(NULL, size) should behave like malloc(size)
    if (!ptr) {
        return volta_gc_malloc(new_size);
    }

    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        return GC_realloc(ptr, new_size);
    }
    if (VOLTA_GC_MODE == VOLTA_GC_MANUAL) {
        return realloc(ptr, new_size);
    }
    return NULL;
}

void volta_gc_free(void* ptr) {
    if (!ptr) return;
    if (VOLTA_GC_MODE == VOLTA_GC_MANUAL) {
        free(ptr);
    }
}

void volta_gc_collect(void) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        GC_gcollect();
        gc_num_collections++;
    }
}


void volta_gc_register_finalizer(void* obj, void (*finalizer)(void*, void*), void* client_data) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        GC_register_finalizer(
            obj,
            (GC_finalization_proc)finalizer,
            client_data,
            NULL,
            NULL
        );
    }
}

size_t volta_gc_get_heap_size(void) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        return GC_get_heap_size();
    }

    return 0;
}

size_t volta_gc_get_free_bytes(void) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        return GC_get_free_bytes();
    }

    return 0;
}

void volta_gc_get_stats(VoltaGCStats* stats) {
    if (!stats) return;

    stats->total_allocations = gc_total_allocations;
    stats->total_bytes = gc_total_bytes;
    stats->heap_size = volta_gc_get_heap_size();
    stats->free_bytes = volta_gc_get_free_bytes();
    stats->num_collections = gc_num_collections;
}

void volta_gc_reset_stats(void) {
    gc_total_allocations = 0;
    gc_total_bytes = 0;
    gc_num_collections = 0;
}

void volta_gc_add_roots(void* start, void* end) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        GC_add_roots(start, end);
    }
}

void volta_gc_remove_roots(void* start, void* end) {
    if (VOLTA_GC_MODE == VOLTA_GC_BOEHM) {
        GC_remove_roots(start, end);
    }
}