#ifndef VOLTA_STRING_H
#define VOLTA_STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "volta_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VoltaString VoltaString;

/**
 * Create a string from a UTF-8 encoded C string literal
 * @param utf8_bytes Pointer to UTF-8 encoded bytes (NOT null-terminated required)
 * @param byte_length Number of bytes (not characters!) in the string
 * @return New VoltaString or NULL on allocation failure
 */
VoltaString* volta_string_from_literal(const char* utf8_bytes, size_t byte_length);

/**
 * Create a string from a null-terminated C string
 * @param c_str Null-terminated C string (UTF-8 encoded)
 * @return New VoltaString or NULL on allocation failure
 */
VoltaString* volta_string_from_cstr(const char* c_str);

/**
 * Create an empty string with a given capacity
 * @param capacity Initial capacity in bytes
 * @return New VoltaString or NULL on allocation failure
 */
VoltaString* volta_string_with_capacity(size_t capacity);

/**
 * Duplicate a string (deep copy)
 * @param str String to clone
 * @return New VoltaString or NULL on allocation failure
 */
VoltaString* volta_string_clone(const VoltaString* str);

/**
 * Free a string and release its memory
 * NOTE: If GC is enabled, this may be a no-op (GC handles deallocation)
 * If manual allocation is used, this calls volta_free() to release memory
 * @param str String to free (can be NULL - no-op)
 */
void volta_string_free(VoltaString* str);

/**
 * Get the length of a string in Unicode code points (not bytes!)
 * @param str String to measure
 * @return Number of Unicode code points
 */
int32_t volta_string_length(const VoltaString* str);

/**
 * Get the byte length of a string (UTF-8 encoding size)
 * @param str String to measure
 * @return Number of bytes
 */
size_t volta_string_byte_length(const VoltaString* str);

/**
 * Get the current capacity of a string in bytes
 * @param str String to query
 * @return Capacity in bytes
 */
size_t volta_string_capacity(const VoltaString* str);

/**
 * Check if a string is empty
 * @param str String to check
 * @return true if empty, false otherwise
 */
bool volta_string_is_empty(const VoltaString* str);

/**
 * Concatenate two strings (creates a new string)
 * @param a First string
 * @param b Second string
 * @return New string containing a + b, or NULL on allocation failure
 */
VoltaString* volta_string_concat(const VoltaString* a, const VoltaString* b);

/**
 * Append a string to another (mutates the first string)
 * @param dest Destination string (modified in-place)
 * @param src Source string to append
 * @return true on success, false on allocation failure
 */
bool volta_string_append(VoltaString* dest, const VoltaString* src);

/**
 * Compare two strings for equality
 * @param a First string
 * @param b Second string
 * @return true if equal, false otherwise
 */
bool volta_string_equals(const VoltaString* a, const VoltaString* b);

/**
 * Compare two strings lexicographically
 * @param a First string
 * @param b Second string
 * @return <0 if a < b, 0 if a == b, >0 if a > b
 */
int volta_string_compare(const VoltaString* a, const VoltaString* b);

/**
 * Get a null-terminated C string from a VoltaString
 * WARNING: The returned pointer is valid only while the VoltaString exists
 * and is not modified. Do NOT free() the returned pointer.
 *
 * @param str String to convert
 * @return Null-terminated C string (internal buffer, do not free!)
 */
const char* volta_string_to_cstr(const VoltaString* str);

/**
 * Get raw UTF-8 bytes from a VoltaString (not necessarily null-terminated)
 * @param str String to query
 * @return Pointer to internal UTF-8 bytes (do not free!)
 */
const char* volta_string_data(const VoltaString* str);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_STRING_H
