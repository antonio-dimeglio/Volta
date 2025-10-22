#include "runtime/volta_builtins.h"
#include "runtime/volta_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

void volta_println(VoltaString* str) {
    if (!str) {
        printf("\n");
        return;
    }
    printf("%.*s\n", (int)volta_string_length(str), volta_string_data(str));
}

void volta_print(VoltaString* str) {
    if (!str) return;
    printf("%.*s", (int)volta_string_length(str), volta_string_data(str));
}

// Print helpers for different types
void volta_print_i32(int32_t val) {
    printf("%d", val);
}

void volta_print_i64(int64_t val) {
    printf("%lld", (long long)val);
}

void volta_print_u32(uint32_t val) {
    printf("%u", val);
}

void volta_print_u64(uint64_t val) {
    printf("%llu", (unsigned long long)val);
}

void volta_print_f32(float val) {
    printf("%g", val);
}

void volta_print_f64(double val) {
    printf("%g", val);
}

void volta_print_bool(bool val) {
    printf("%s", val ? "true" : "false");
}

void volta_print_char(char c) {
    printf("%c", c);
}

void volta_print_ptr(void* ptr) {
    printf("%p", ptr);
}

void volta_print_newline(void) {
    printf("\n");
}

void volta_panic(VoltaString* msg) {
    if (!msg) {
        fprintf(stderr, "PANIC\n");
    } else {
        fprintf(stderr, "PANIC: %.*s\n", (int)volta_string_length(msg), volta_string_data(msg));
    }
    abort();
}

void volta_assert(bool cond, VoltaString* msg) {
    if (!cond) {
        if (!msg) {
            fprintf(stderr, "Assertion failed\n");
        } else {
            fprintf(stderr, "Assertion failed: %.*s\n", (int)volta_string_length(msg), volta_string_data(msg));
        }
        abort();
    }
}

int32_t volta_array_len(void* arr_ptr) {
    return volta_array_length((VoltaArray*)arr_ptr);
}

VoltaString* volta_i32_to_string(int32_t val) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d", val);
    if (len < 0) return NULL;
    return volta_string_from_literal(buffer, len);
}

VoltaString* volta_i64_to_string(int64_t val) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%lld", (long long)val);
    if (len < 0) return NULL;
    return volta_string_from_literal(buffer, len);
}

VoltaString* volta_f32_to_string(float val) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "%g", val);
    if (len < 0) return NULL;
    return volta_string_from_literal(buffer, len);
}

VoltaString* volta_f64_to_string(double val) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "%g", val);
    if (len < 0) return NULL;
    return volta_string_from_literal(buffer, len);
}

VoltaString* volta_bool_to_string(bool val) {
    if (val) {
        return volta_string_from_literal("true", 4);
    } else {
        return volta_string_from_literal("false", 5);
    }
}

size_t volta_sizeof(size_t type_size) {
    return type_size;
}

void volta_dbg(void* ptr, const char* type_name) {
    if (!type_name) {
        printf("[Debug] %p (unknown type)\n", ptr);
        return;
    }
    printf("[Debug] %s @ %p\n", type_name, ptr);
}
