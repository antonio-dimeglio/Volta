#include "runtime/volta_string.h"
#include "runtime/volta_gc.h"
#include <string.h>
#include <stdlib.h>

size_t utf8_length(const char* s, size_t byte_length) {
    size_t count = 0;
    size_t i = 0;
    while (i < byte_length) {
        unsigned char c = (unsigned char)s[i];
        size_t step;

        if ((c & 0x80) == 0x00) step = 1;          
        else if ((c & 0xE0) == 0xC0) step = 2;     
        else if ((c & 0xF0) == 0xE0) step = 3;     
        else if ((c & 0xF8) == 0xF0) step = 4;     
        else return count;                         

        if (i + step > byte_length) break;         
        i += step;
        count++;
    }
    return count;
}


typedef struct VoltaString {
    char* data;
    size_t size;
    size_t length;
} VoltaString;

VoltaString* volta_string_from_literal(const char* utf8_bytes, size_t byte_length) {
    if (!utf8_bytes) return NULL;

    VoltaString* str = (VoltaString*)volta_alloc(sizeof(VoltaString));
    if (!str) return NULL;

    str->data = (char*)volta_alloc(byte_length);
    if (!str->data) {
        volta_free(str);
        return NULL;
    }

    memcpy(str->data, utf8_bytes, byte_length);
    str->size = byte_length;
    str->length = utf8_length(utf8_bytes, byte_length);
    return str;
}


VoltaString* volta_string_from_cstr(const char* c_str) {
    if (!c_str) return NULL;
    size_t byte_length = strlen(c_str);
    return volta_string_from_literal(c_str, byte_length);
}

VoltaString* volta_string_with_capacity(size_t capacity) {
    VoltaString* str = (VoltaString*)volta_alloc(sizeof(VoltaString));
    if (!str) return NULL;

    str->data = (char*)volta_gc_calloc(capacity, sizeof(char));
    if (!str->data) {
        volta_free(str);
        return NULL;
    }
    str->size = capacity;
    str->length = 0;
    return str;
}

VoltaString* volta_string_clone(const VoltaString* str) {
    if (!str) return NULL;
    return volta_string_from_literal(str->data, str->size);
}

void volta_string_free(VoltaString* str) {
    if (str) {
        if (str->data) volta_free(str->data);
        volta_free(str);
    }
}

int32_t volta_string_length(const VoltaString* str) {
    return str->length;
}

size_t volta_string_byte_length(const VoltaString* str) {
    return str->size;
}

size_t volta_string_capacity(const VoltaString* str) {
    return str->size;
}

bool volta_string_is_empty(const VoltaString* str) {
    return str->length == 0;
}

VoltaString* volta_string_concat(const VoltaString* a, const VoltaString* b) {
    VoltaString* newString = volta_string_with_capacity(a->size + b->size);
    if (newString) {
        memcpy(newString->data, a->data, a->size);
        memcpy(newString->data+a->size, b->data, b->size);
        newString->length = utf8_length(newString->data, newString->size);
    }
    return newString;
}

bool volta_string_append(VoltaString* dest, const VoltaString* src) {
    if (!dest || !src) return false;
    if (src->size == 0) return true; // Nothing to append

    size_t new_size = dest->size + src->size;
    char* new_data = (char*)volta_realloc(dest->data, new_size);
    if (!new_data) return false;

    memcpy(new_data + dest->size, src->data, src->size);
    dest->data = new_data;
    dest->size = new_size;
    dest->length = utf8_length(dest->data, dest->size);

    return true;
}

bool volta_string_equals(const VoltaString* a, const VoltaString* b) {
    return volta_string_compare(a, b) == 0;
}

int volta_string_compare(const VoltaString* a, const VoltaString* b) {
    if (a->size != b->size) return (a->size > b->size) ? 1 : -1;
    return memcmp(a->data, b->data, a->size);
}

const char* volta_string_to_cstr(const VoltaString* str) {
    char* buf = malloc(str->size+ 1);
    if (!buf) return NULL;
    memcpy(buf, str->data, str->size);
    buf[str->size] = '\0';
    return buf;
}

const char* volta_string_data(const VoltaString* str) {
    return str->data;
}
