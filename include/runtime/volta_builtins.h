#ifndef VOLTA_BUILTINS_H
#define VOLTA_BUILTINS_H

#include "volta_string.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Print a string to stdout with a newline
 * Volta signature: fn println(s: string) -> void
 * @param str String to print
 */
void volta_println(VoltaString* str);

/**
 * Print a string to stdout without a newline
 * Volta signature: fn print(s: string) -> void
 * @param str String to print
 */
void volta_print(VoltaString* str);

/**
 * Print helpers for primitive types
 */
void volta_print_i32(int32_t val);
void volta_print_i64(int64_t val);
void volta_print_u32(uint32_t val);
void volta_print_u64(uint64_t val);
void volta_print_f32(float val);
void volta_print_f64(double val);
void volta_print_bool(bool val);
void volta_print_char(char c);
void volta_print_ptr(void* ptr);
void volta_print_newline(void);

/**
 * Panic with a message and abort execution
 * Volta signature: fn panic(msg: string) -> !
 * This function never returns (noreturn)
 * @param msg Error message to print before aborting
 */
void volta_panic(VoltaString* msg) __attribute__((noreturn));

/**
 * Assert a condition, panic if false
 * Volta signature: fn assert(cond: bool, msg: string) -> void
 * @param cond Condition to check
 * @param msg Message to print if assertion fails
 */
void volta_assert(bool cond, VoltaString* msg);

/**
 * Get the length of a dynamically-sized array
 * Volta signature: fn len<T>(arr: Array<T>) -> i32
 * Note: For static arrays [T; N], length is known at compile time
 * @param arr_ptr Pointer to array structure
 * @return Length of the array
 */
int32_t volta_array_len(void* arr_ptr);

/**
 * Convert an integer to a string
 * Volta signature: fn to_string(val: i32) -> string
 * @param val Integer value
 * @return String representation
 */
VoltaString* volta_i32_to_string(int32_t val);

/**
 * Convert an integer to a string
 * Volta signature: fn to_string(val: i64) -> string
 * @param val Integer value
 * @return String representation
 */
VoltaString* volta_i64_to_string(int64_t val);

/**
 * Convert a float to a string
 * Volta signature: fn to_string(val: f32) -> string
 * @param val Float value
 * @return String representation
 */
VoltaString* volta_f32_to_string(float val);

/**
 * Convert a float to a string
 * Volta signature: fn to_string(val: f64) -> string
 * @param val Double value
 * @return String representation
 */
VoltaString* volta_f64_to_string(double val);

/**
 * Convert a boolean to a string ("true" or "false")
 * Volta signature: fn to_string(val: bool) -> string
 * @param val Boolean value
 * @return String "true" or "false"
 */
VoltaString* volta_bool_to_string(bool val);

/**
 * Get the size of a type in bytes (like C sizeof)
 * Volta signature: fn sizeof<T>() -> usize
 * Note: This is typically resolved at compile time, but runtime version exists
 * @param type_size Size of type (passed by compiler)
 * @return Size in bytes
 */
size_t volta_sizeof(size_t type_size);

/**
 * Print debug representation of a value (implementation-defined format)
 * Volta signature: fn dbg<T>(val: T) -> void
 * Note: This is a generic function, actual implementation may vary by type
 * @param ptr Pointer to value
 * @param type_name Name of the type (for debugging)
 */
void volta_dbg(void* ptr, const char* type_name);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_BUILTINS_H
