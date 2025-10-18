#include "runtime/volta_runtime.h"

void volta_runtime_init(void) {
    volta_gc_init();
}

void volta_runtime_shutdown(void) {
    volta_gc_shutdown();
}