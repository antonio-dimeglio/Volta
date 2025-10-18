#include "runtime/volta_array.h"
#include "runtime/volta_gc.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 8
#define GROWTH_RATE 2

typedef struct VoltaArray {
    void* data;
    size_t element_size;
    size_t length;
    size_t capacity;
} VoltaArray;

VoltaArray* volta_array_new(size_t element_size) {
    VoltaArray* arr = volta_alloc(sizeof(VoltaArray));
    if (!arr) return NULL;

    arr->data = volta_alloc(element_size * DEFAULT_CAPACITY);

    if (!arr->data) {
        volta_free(arr);
        return NULL;
    }

    arr->element_size = element_size;
    arr->length = 0;
    arr->capacity = DEFAULT_CAPACITY;
    return arr;
}

VoltaArray* volta_array_with_capacity(size_t element_size, size_t capacity) {
    VoltaArray* arr = volta_alloc(sizeof(VoltaArray));
    if (!arr) return NULL;

    arr->data = volta_alloc(element_size * capacity);

    if (!arr->data) {
        volta_free(arr);
        return NULL;
    }

    arr->element_size = element_size;
    arr->length = 0;
    arr->capacity = capacity;
    return arr;
}

VoltaArray* volta_array_from_data(size_t element_size, const void* data, size_t count) {
    if (!data) return NULL;

    VoltaArray* arr = volta_alloc(sizeof(VoltaArray));
    if (!arr) return NULL;

    arr->data = volta_alloc(element_size * count);

    if (!arr->data) {
        volta_free(arr);
        return NULL;
    }
    memcpy(arr->data, data, element_size * count);
    arr->element_size = element_size;
    arr->length = count;
    arr->capacity = count;
    return arr;
}

VoltaArray* volta_array_clone(const VoltaArray* arr) {
    if (arr == NULL) return NULL;
    return volta_array_from_data(arr->element_size, arr->data, arr->length);
}

void volta_array_free(VoltaArray* arr) {
    if (arr) {            
        volta_free(arr->data);
        volta_free(arr);
    }
}

int32_t volta_array_length(const VoltaArray* arr) {
    return arr ? arr->length : 0;
}

size_t volta_array_capacity(const VoltaArray* arr) {
    return arr ? arr->capacity : 0;
}

bool volta_array_is_empty(const VoltaArray* arr) {
    return arr ? arr->length == 0 : false;
}

size_t volta_array_element_size(const VoltaArray* arr) {
    return arr ? arr->element_size : 0;
}

void* volta_array_get(VoltaArray* arr, int32_t index) {
    if (!arr) return NULL;
    if (index < 0 || index >= arr->length) return NULL;
    void* elem = (char*)arr->data + index * arr->element_size;
    return elem;
}

void* volta_array_get_unchecked(VoltaArray* arr, int32_t index) {
    if (!arr) return NULL;
    return (char*)arr->data + index * arr->element_size;
}

bool volta_array_set(VoltaArray* arr, int32_t index, const void* element) {
    if (!arr || !element) return false;
    if (index < 0 || index >= arr->length) return false;
    memcpy((char*)arr->data + index * arr->element_size, element, arr->element_size);
    return true;
}

void* volta_array_data(VoltaArray* arr) {
    return arr ? arr->data : NULL;
}

bool volta_array_push(VoltaArray* arr, const void* element) {
    if (!arr) return false;
    if (arr->capacity == arr->length) {
        size_t new_capacity = arr->capacity * GROWTH_RATE;
        arr->data = volta_gc_realloc(arr->data, arr->element_size * new_capacity);
        if (!arr->data) {
            volta_gc_free(arr);
            return false;
        }
        arr->capacity = new_capacity;
    }

    memcpy((char*)arr->data + arr->length * arr->element_size, element, arr->element_size);
    arr->length++;
    return true;
}

bool volta_array_pop(VoltaArray* arr, void* dest) {
    if (!arr || arr->length == 0) return false;

    arr->length--;
    void* src = (char*)arr->data + arr->length * arr->element_size;

    if (dest) {
        memcpy(dest, src, arr->element_size);
    }

    memset(src, 0, arr->element_size);

    return true;
}

bool volta_array_insert(VoltaArray* arr, int32_t index, const void* element) {
    if (!arr || !element) return false;
    if (index < 0 || index > arr->length) return false;
    
    if (arr->length >= arr->capacity) {
        size_t new_capacity = arr->capacity * GROWTH_RATE;
        void* new_data = volta_realloc(arr->data, arr->element_size * new_capacity);
        if (!new_data) return false;
        arr->data = new_data;
        arr->capacity = new_capacity;
    }

    if (index < arr->length) {
        void* src = (char*)arr->data + index * arr->element_size;
        void* dst = (char*)arr->data + (index + 1) * arr->element_size;
        memmove(dst, src, (arr->length - index) * arr->element_size);
    }
    
    memcpy((char*)arr->data + index * arr->element_size, element, arr->element_size);
    arr->length++;
    return true;
}

bool volta_array_remove(VoltaArray* arr, int32_t index, void* dest) {
    if (!arr) return false;
    if (index < 0 || index >= arr->length) return false;

    if (dest) {
        memcpy(dest, (char*)arr->data + index * arr->element_size, arr->element_size);
    }
    
    if (index < arr->length - 1) {
        void* dst = (char*)arr->data + index * arr->element_size;
        void* src = (char*)arr->data + (index + 1) * arr->element_size;
        memmove(dst, src, (arr->length - index - 1) * arr->element_size);
    }

    arr->length--;
    return true;
}

void volta_array_clear(VoltaArray* arr) {
    if (!arr) return;
    arr->length = 0;
}

bool volta_array_reserve(VoltaArray* arr, size_t capacity) {
    if (!arr) return false;

    void* new_data = volta_realloc(arr->data, arr->element_size * capacity);
    if (!new_data) return false;

    arr->data = new_data;
    arr->capacity = capacity;
    return true;
}

bool volta_array_shrink_to_fit(VoltaArray* arr) {
    if (!arr) return false;
    if (arr->length == arr->capacity) return true; 
    if (arr->length == 0) return true; 

    void* new_data = volta_realloc(arr->data, arr->element_size * arr->length);
    if (!new_data) return false;

    arr->data = new_data;
    arr->capacity = arr->length;
    return true;
}

bool volta_array_resize(VoltaArray* arr, size_t new_length) {
    if (!arr) return false;
    
    if (new_length > arr->capacity) {
        if (!volta_array_reserve(arr, new_length)) {
            return false;
        }
    }

    if (new_length > arr->length) {
        void* start = (char*)arr->data + arr->length * arr->element_size;
        size_t bytes_to_zero = (new_length - arr->length) * arr->element_size;
        memset(start, 0, bytes_to_zero);
    }

    arr->length = new_length;
    return true;
}

bool volta_array_fill(VoltaArray* arr, const void* element, size_t count) {
    if (!arr || !element) return false;

    if (!volta_array_resize(arr, count)) {
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        memcpy((char*)arr->data + i * arr->element_size, element, arr->element_size);
    }

    return true;
}
