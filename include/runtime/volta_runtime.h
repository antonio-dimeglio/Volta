#ifndef VOLTA_RUNTIME_H
#define VOLTA_RUNTIME_H

#include "volta_gc.h"
#include "volta_allocator.h"
#include "volta_string.h"
#include "volta_array.h"
#include "volta_builtins.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the Volta runtime
 * This should be called before any other runtime functions.
 * Initializes the garbage collector (if enabled) and runtime subsystems.
 */
void volta_runtime_init(void);

/**
 * Shutdown the Volta runtime
 * This should be called before program exit to clean up resources.
 * Shuts down the garbage collector (if enabled) and runtime subsystems.
 */
void volta_runtime_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_RUNTIME_H
