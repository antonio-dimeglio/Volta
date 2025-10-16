#ifndef VOLTA_ARRAY_H
#define VOLTA_ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "volta_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VoltaArray VoltaArray;

/**
 * Create a new empty array with default capacity
 * @param element_size Size of each element in bytes
 * @return New VoltaArray or NULL on allocation failure
 */
VoltaArray* volta_array_new(size_t element_size);

/**
 * Create a new array with a specific initial capacity
 * @param element_size Size of each element in bytes
 * @param capacity Initial capacity (number of elements)
 * @return New VoltaArray or NULL on allocation failure
 */
VoltaArray* volta_array_with_capacity(size_t element_size, size_t capacity);

/**
 * Create an array from existing data (copies the data)
 * @param element_size Size of each element in bytes
 * @param data Pointer to source data
 * @param count Number of elements to copy
 * @return New VoltaArray or NULL on allocation failure
 */
VoltaArray* volta_array_from_data(size_t element_size, const void* data, size_t count);

/**
 * Clone an array (deep copy)
 * @param arr Array to clone
 * @return New VoltaArray or NULL on allocation failure
 */
VoltaArray* volta_array_clone(const VoltaArray* arr);

/**
 * Free an array and release its memory
 * NOTE: If GC is enabled, this may be a no-op (GC handles deallocation)
 * If manual allocation is used, this calls volta_free() to release memory
 * @param arr Array to free (can be NULL - no-op)
 */
void volta_array_free(VoltaArray* arr);

/**
 * Get the length of an array (number of elements)
 * @param arr Array to query
 * @return Number of elements in the array
 */
int32_t volta_array_length(const VoltaArray* arr);

/**
 * Get the capacity of an array (allocated space)
 * @param arr Array to query
 * @return Capacity in number of elements
 */
size_t volta_array_capacity(const VoltaArray* arr);

/**
 * Check if an array is empty
 * @param arr Array to check
 * @return true if empty, false otherwise
 */
bool volta_array_is_empty(const VoltaArray* arr);

/**
 * Get the element size of an array
 * @param arr Array to query
 * @return Size of each element in bytes
 */
size_t volta_array_element_size(const VoltaArray* arr);

/**
 * Get a pointer to an element at a specific index (bounds checked)
 * @param arr Array to access
 * @param index Index of element (0-based)
 * @return Pointer to element, or NULL if out of bounds
 */
void* volta_array_get(VoltaArray* arr, int32_t index);

/**
 * Get a pointer to an element at a specific index (unchecked, faster)
 * WARNING: Undefined behavior if index is out of bounds
 * @param arr Array to access
 * @param index Index of element (0-based)
 * @return Pointer to element
 */
void* volta_array_get_unchecked(VoltaArray* arr, int32_t index);

/**
 * Set an element at a specific index (bounds checked, copies data)
 * @param arr Array to modify
 * @param index Index of element (0-based)
 * @param element Pointer to element data to copy
 * @return true on success, false if out of bounds
 */
bool volta_array_set(VoltaArray* arr, int32_t index, const void* element);

/**
 * Get raw pointer to the underlying data buffer
 * WARNING: Pointer may be invalidated if array is modified
 * @param arr Array to query
 * @return Pointer to internal data buffer
 */
void* volta_array_data(VoltaArray* arr);

/**
 * Push an element to the end of the array (grows if needed)
 * @param arr Array to modify
 * @param element Pointer to element data to copy
 * @return true on success, false on allocation failure
 */
bool volta_array_push(VoltaArray* arr, const void* element);

/**
 * Pop an element from the end of the array and copy it to destination
 * @param arr Array to modify
 * @param dest Destination to copy popped element (can be NULL to discard)
 * @return true on success, false if array is empty
 */
bool volta_array_pop(VoltaArray* arr, void* dest);

/**
 * Insert an element at a specific index (shifts elements right)
 * @param arr Array to modify
 * @param index Index to insert at (0-based, can be == length to append)
 * @param element Pointer to element data to copy
 * @return true on success, false on allocation failure or out of bounds
 */
bool volta_array_insert(VoltaArray* arr, int32_t index, const void* element);

/**
 * Remove an element at a specific index (shifts elements left)
 * @param arr Array to modify
 * @param index Index to remove (0-based)
 * @param dest Destination to copy removed element (can be NULL to discard)
 * @return true on success, false if out of bounds
 */
bool volta_array_remove(VoltaArray* arr, int32_t index, void* dest);

/**
 * Clear all elements from the array (length = 0, keeps capacity)
 * @param arr Array to clear
 */
void volta_array_clear(VoltaArray* arr);

/**
 * Reserve capacity for at least n elements (avoids reallocation)
 * @param arr Array to modify
 * @param capacity Minimum capacity to reserve
 * @return true on success, false on allocation failure
 */
bool volta_array_reserve(VoltaArray* arr, size_t capacity);

/**
 * Shrink capacity to match length (free unused space)
 * @param arr Array to modify
 * @return true on success, false on allocation failure
 */
bool volta_array_shrink_to_fit(VoltaArray* arr);

/**
 * Resize an array to a new length
 * If new length > current length, new elements are zero-initialized
 * @param arr Array to resize
 * @param new_length New length
 * @return true on success, false on allocation failure
 */
bool volta_array_resize(VoltaArray* arr, size_t new_length);

/**
 * Fill an array with a specific value (copies element to all positions)
 * @param arr Array to fill
 * @param element Pointer to element data to copy
 * @param count Number of times to fill (resizes array if needed)
 * @return true on success, false on allocation failure
 */
bool volta_array_fill(VoltaArray* arr, const void* element, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_ARRAY_H
