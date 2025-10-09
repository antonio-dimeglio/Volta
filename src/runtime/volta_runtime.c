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

void volta_print_int(int64_t value) {
    printf("%" PRId64, value);
}

void volta_print_uint(uint64_t value) {
    printf("%" PRIu64, value);
}

void volta_print_bool(int8_t value) {
    printf("%s\n", value ? "true" : "false");
}

void volta_print_string(const char* str) {
    printf("%s\n", str);
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

void volta_array_push(VoltaArray* arr, void* elem) {
    if (arr->length >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        void** new_data = volta_gc_alloc(new_capacity * sizeof(void*));
        memcpy(new_data, arr->data, arr->length * sizeof(void*));
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    arr->data[arr->length++] = elem;
}

int64_t volta_array_length(VoltaArray* arr) {
    return arr->length;
}