/**
 * @file volta_runtime.h
 * @brief Volta language runtime library
 *
 * This header defines the C runtime library that Volta programs link against.
 * It provides essential services including:
 *   - Garbage collection and memory management
 *   - Standard output functions for printing values
 *   - Built-in data structure implementations (strings, arrays)
 *
 * The runtime is designed to be minimal and efficient, providing only the
 * core functionality needed by compiled Volta programs. All functions use
 * C linkage to ensure compatibility with LLVM-generated code.
 *
 * Linking: Volta programs are compiled to object files that must be linked
 * with the runtime library (libvolta_runtime.a or volta_runtime.o).
 */
#ifndef VOLTA_RUNTIME_H
#define VOLTA_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Memory Management
// ============================================================================

/**
 * Allocate memory from the garbage-collected heap
 *
 * Allocates a block of memory that will be automatically freed by the GC
 * when no longer reachable. The allocated memory is zeroed.
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on allocation failure
 */
void* volta_gc_alloc(size_t size);

/**
 * Trigger a garbage collection cycle
 *
 * Forces an immediate GC run to free unreachable objects. Normally called
 * automatically when memory pressure is high, but can be manually invoked
 * for testing or performance tuning.
 */
void volta_gc_collect(void);

void* volta_gc_alloc_atomic(size_t size);

void* volta_gc_realloc(void* ptr, size_t new_size);

void volta_gc_register_finalizer(void* obj, void (*finalizer)(void*, void*));

size_t volta_gc_get_heap_size(void);
size_t volta_gc_get_bytes_since_gc(void);

// ============================================================================
// Print Functions
// ============================================================================

/**
 * Print a signed integer to stdout
 *
 * Handles all signed integer types (i8, i16, i32, i64) by widening to i64.
 * Output is formatted in decimal with no trailing newline.
 *
 * @param value The integer value to print
 */
void volta_print_int(int64_t value);

/**
 * Print an unsigned integer to stdout
 *
 * Handles all unsigned integer types (u8, u16, u32, u64) by widening to u64.
 * Output is formatted in decimal with no trailing newline.
 *
 * @param value The unsigned integer value to print
 */
void volta_print_uint(uint64_t value);

/**
 * Print a floating-point number to stdout
 *
 * Handles both f32 and f64 types (f32 is widened to double).
 * Output is formatted with appropriate precision, no trailing newline.
 *
 * @param value The floating-point value to print
 */
void volta_print_float(double value);

/**
 * Print a boolean value to stdout
 *
 * Outputs "true" or "false" with no trailing newline.
 *
 * @param value The boolean value (0 = false, non-zero = true)
 */
void volta_print_bool(int8_t value);

/**
 * Print a null-terminated string to stdout
 *
 * Outputs the string as-is with no trailing newline.
 *
 * @param str Pointer to null-terminated C string
 */
void volta_print_string(const char* str);

// ============================================================================
// String Type
// ============================================================================

/**
 * Volta string object
 *
 * Represents an immutable, garbage-collected string. Unlike C strings,
 * Volta strings store their length explicitly and may contain null bytes.
 */
typedef struct {
    size_t length;  ///< Number of bytes in the string (excluding null terminator)
    char* data;     ///< Pointer to string data (null-terminated for C interop)
} VoltaString;

/**
 * Create a new Volta string
 *
 * Allocates a GC-managed string and copies the provided data into it.
 * The resulting string is null-terminated for C interoperability.
 *
 * @param data Pointer to string data to copy
 * @param length Number of bytes to copy (excluding null terminator)
 * @return Pointer to new VoltaString, or NULL on allocation failure
 */
VoltaString* volta_string_new(const char* data, size_t length);

/**
 * Get the length of a Volta string
 *
 * Returns the number of bytes in the string, not including the null terminator.
 * Corresponds to the Volta built-in function `len(string)`.
 *
 * @param str Pointer to VoltaString
 * @return Length in bytes, or 0 if str is NULL
 */
int64_t volta_string_length(VoltaString* str);

/**
 * Compare two Volta strings for equality
 *
 * Checks if two strings have the same length and content.
 *
 * @param a First string to compare
 * @param b Second string to compare
 * @return 1 if equal, 0 if not equal
 */
int8_t volta_string_eq(VoltaString* a, VoltaString* b);

/**
 * Compare two Volta strings lexicographically
 *
 * Returns a value indicating the relative ordering of two strings.
 *
 * @param a First string to compare
 * @param b Second string to compare
 * @return <0 if a<b, 0 if a==b, >0 if a>b (like strcmp)
 */
int32_t volta_string_cmp(VoltaString* a, VoltaString* b);

/**
 * Concatenate two Volta strings
 *
 * Creates a new string by concatenating two strings.
 *
 * @param a First string
 * @param b Second string
 * @return New string containing a + b
 */
VoltaString* volta_string_concat(VoltaString* a, VoltaString* b);

// ============================================================================
// Array Type
// ============================================================================

/**
 * Volta dynamic array
 *
 * Represents a growable, garbage-collected array of pointers. Arrays
 * automatically expand when elements are pushed beyond capacity.
 */
typedef struct {
    size_t length;    ///< Number of elements currently in the array
    size_t capacity;  ///< Total allocated capacity (in elements)
    void** data;      ///< Pointer to array data
} VoltaArray;

/**
 * Create a new Volta array
 *
 * Allocates a GC-managed array with the specified initial capacity.
 * The array starts empty (length = 0) but has space pre-allocated.
 *
 * @param capacity Initial number of elements to allocate space for
 * @return Pointer to new VoltaArray, or NULL on allocation failure
 */
VoltaArray* volta_array_new(size_t capacity);

/**
 * Create a Volta array from existing values
 *
 * Creates a new array and initializes it with the provided values.
 * The array capacity and length are both set to count.
 *
 * @param values Pointer to array of void* values to copy
 * @param count Number of values in the array
 * @return Pointer to new VoltaArray containing the values
 */
VoltaArray* volta_array_from_values(void** values, size_t count);

/**
 * Get the length of a Volta array
 *
 * Returns the number of elements currently in the array.
 * Corresponds to the Volta built-in function `len(array)`.
 *
 * @param arr Pointer to VoltaArray
 * @return Number of elements, or 0 if arr is NULL
 */
int64_t volta_array_length(VoltaArray* arr);

/**
 * Get element at index in a Volta array
 *
 * Retrieves the element at the specified index with bounds checking.
 * Terminates the program if index is out of bounds.
 *
 * @param arr Pointer to VoltaArray
 * @param index Index of element to retrieve (0-based)
 * @return Pointer to element at index
 */
void* volta_array_get(VoltaArray* arr, int64_t index);

/**
 * Set element at index in a Volta array
 *
 * Stores a value at the specified index with bounds checking.
 * Terminates the program if index is out of bounds.
 *
 * @param arr Pointer to VoltaArray
 * @param index Index where to store the value (0-based)
 * @param value Value to store
 */
void volta_array_set(VoltaArray* arr, int64_t index, void* value);

/**
 * Append an element to a Volta array
 *
 * Adds an element to the end of the array, growing the capacity if needed.
 * The array will automatically reallocate with increased capacity when full.
 *
 * @param arr Pointer to VoltaArray
 * @param elem Element to append (any pointer value)
 */
void volta_array_push(VoltaArray* arr, void* elem);

/**
 * Remove and return the last element from a Volta array
 *
 * Removes the last element from the array and returns it.
 * Terminates the program if the array is empty.
 *
 * @param arr Pointer to VoltaArray
 * @return The removed element
 */
void* volta_array_pop(VoltaArray* arr);

/**
 * Apply a function to each element of an array
 *
 * Creates a new array by applying the given function to each element
 * of the input array. The original array is not modified.
 *
 * @param arr Pointer to VoltaArray to map over
 * @param func Function to apply to each element (takes void*, returns void*)
 * @return New VoltaArray containing the transformed elements
 */
VoltaArray* volta_array_map(VoltaArray* arr, void* (*func)(void*));

/**
 * Filter an array based on a predicate
 *
 * Creates a new array containing only the elements for which the predicate
 * returns true (non-zero). The original array is not modified.
 *
 * @param arr Pointer to VoltaArray to filter
 * @param predicate Function to test each element (takes void*, returns int)
 * @return New VoltaArray containing only elements that passed the predicate
 */
VoltaArray* volta_array_filter(VoltaArray* arr, int (*predicate)(void*));


#ifdef __cplusplus
}
#endif

#endif 