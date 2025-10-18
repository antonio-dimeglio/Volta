#include "runtime/volta_allocator.h"
#include "runtime/volta_gc.h"
#include <stdbool.h>

// Allocation tracking state
static bool tracking_enabled = false;
static size_t total_allocated = 0;
static size_t total_freed = 0;

void volta_allocator_enable_tracking(void) {
    tracking_enabled = true;
}

void volta_allocator_disable_tracking(void) {
    tracking_enabled = false;
}

size_t volta_allocator_get_total_allocated(void) {
    return tracking_enabled ? total_allocated : 0;
}

size_t volta_allocator_get_bytes_in_use(void) {
    return tracking_enabled ? (total_allocated - total_freed) : 0;
}

