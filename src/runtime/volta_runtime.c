#include "runtime/volta_runtime.h"

#include <gc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

static void init_gc(void) {
    static int initialized = 0;
    if (!initialized) {
        GC_INIT();
        initialized = 1;
    }
}

void* volta_gc_alloc(size_t size) {
    init_gc();
    void* ptr = GC_MALLOC(size);
    if (!ptr) {
        fprintf(stderr, "GC allocation failed.\n");
        exit(1);
    }

    return ptr;
}


void volta_gc_collect(void) {
    GC_gcollect();
}

void* volta_gc_alloc_atomic(size_t size) {
    return GC_MALLOC_ATOMIC(size);
}

void* volta_gc_realloc(void* ptr, size_t new_size) {
    return GC_REALLOC(ptr, new_size);
}

void volta_gc_register_finalizer(void* obj, void (*finalizer)(void*, void*)) {
    GC_register_finalizer(obj, finalizer, NULL, NULL, NULL);
}

size_t volta_gc_get_heap_size(void) {
    return GC_get_heap_size();
}

size_t volta_gc_get_bytes_since_gc(void) {
    return GC_get_bytes_since_gc();
}



void volta_print_int(int64_t value) {
    printf("%" PRId64, value);
    fflush(stdout);
}

void volta_print_uint(uint64_t value) {
    printf("%" PRIu64, value);
    fflush(stdout);
}

void volta_print_float(double value) {
    printf("%g", value);
    fflush(stdout);
}

void volta_print_bool(int8_t value) {
    printf("%s\n", value ? "true" : "false");
    fflush(stdout);
}

void volta_print_string(const char* str) {
    printf("%s\n", str);
    fflush(stdout);
}

VoltaString* volta_string_new(const char* data, size_t length) {
    VoltaString* str = volta_gc_alloc(sizeof(VoltaString));
    str->length = length;
    str->data = volta_gc_alloc(length + 1);
    memcpy(str->data, data, length);
    str->data[length] = '\0';
    return str;
}

int64_t volta_string_length(VoltaString* str) {
    return str->length;
}

int8_t volta_string_eq(VoltaString* a, VoltaString* b) {
    if (a == b) return 1;  // Same pointer
    if (!a || !b) return 0;  // One is NULL
    if (a->length != b->length) return 0;  // Different lengths
    return memcmp(a->data, b->data, a->length) == 0 ? 1 : 0;
}

int32_t volta_string_cmp(VoltaString* a, VoltaString* b) {
    if (a == b) return 0;  // Same pointer
    if (!a) return -1;  // NULL comes before everything
    if (!b) return 1;

    size_t minLen = a->length < b->length ? a->length : b->length;
    int cmp = memcmp(a->data, b->data, minLen);
    if (cmp != 0) return cmp;

    // Equal up to minLen, so shorter string is "less"
    if (a->length < b->length) return -1;
    if (a->length > b->length) return 1;
    return 0;
}

VoltaString* volta_string_concat(VoltaString* a, VoltaString* b) {
    if (!a || !b) return NULL;

    size_t newLength = a->length + b->length;
    VoltaString* result = volta_gc_alloc(sizeof(VoltaString));
    result->length = newLength;
    result->data = volta_gc_alloc(newLength + 1);

    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length);
    result->data[newLength] = '\0';

    return result;
}

VoltaArray* volta_array_new(size_t capacity) {
    VoltaArray* arr = volta_gc_alloc(sizeof(VoltaArray));
    arr->length = 0;
    arr->capacity = capacity;
    arr->data = volta_gc_alloc(capacity * sizeof(void*));
    return arr;
}

VoltaArray* volta_array_from_values(void** values, size_t count) {
    VoltaArray* arr = volta_array_new(count);
    arr->length = count;
    memcpy(arr->data, values, count * sizeof(void*));
    return arr;
}

int64_t volta_array_length(VoltaArray* arr) {
    return arr->length;
}

void* volta_array_get(VoltaArray* arr, int64_t index) {
    if (index < 0 || (size_t)index >= arr->length) {
        fprintf(stderr, "Array index out of bounds: %ld (length: %zu)\n",
                index, arr->length);
        exit(1);
    }
    return arr->data[index];
}

void volta_array_set(VoltaArray* arr, int64_t index, void* value) {
    if (index < 0 || (size_t)index >= arr->length) {
        fprintf(stderr, "Array index out of bounds: %ld (length: %zu)\n",
                index, arr->length);
        exit(1);
    }
    arr->data[index] = value;
}

void volta_array_push(VoltaArray* arr, void* elem) {
    if (arr->length >= arr->capacity) {
        size_t new_capacity = arr->capacity == 0 ? 8 : arr->capacity * 2;
        void** new_data = volta_gc_alloc(new_capacity * sizeof(void*));
        memcpy(new_data, arr->data, arr->length * sizeof(void*));
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    arr->data[arr->length++] = elem;
}

void* volta_array_pop(VoltaArray* arr) {
    if (arr->length == 0) {
        fprintf(stderr, "Array pop on empty array\n");
        exit(1);
    }
    return arr->data[--arr->length];
}

VoltaArray* volta_array_map(VoltaArray* arr, void* (*func)(void*)) {
    VoltaArray* result = volta_array_new(arr->length);
    result->length = arr->length;
    for (size_t i = 0; i < arr->length; i++) {
        result->data[i] = func(arr->data[i]);
    }
    return result;
}

VoltaArray* volta_array_filter(VoltaArray* arr, int (*predicate)(void*)) {
    VoltaArray* result = volta_array_new(arr->length);
    for (size_t i = 0; i < arr->length; i++) {
        if (predicate(arr->data[i])) {
            volta_array_push(result, arr->data[i]);
        }
    }
    return result;
}