#pragma once

#include "Value.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace volta::vm {

// Forward declaration
class VM;

/**
 * @brief Foreign Function Interface Manager
 *
 * Enables calling C functions from Volta code.
 * Handles:
 * - Dynamic library loading (dlopen/LoadLibrary)
 * - Function registration with type signatures
 * - Argument marshaling (Volta → C)
 * - Return value marshaling (C → Volta)
 * - GC coordination (pause during C calls)
 */
class FFIManager {
public:
    /**
     * @brief FFI type descriptors
     *
     * Maps Volta types to C types for marshaling
     */
    enum class FFIType {
        VOID,           ///< void (for return type only)
        BOOL,           ///< C bool
        INT64,          ///< int64_t
        DOUBLE,         ///< double (64-bit float)
        STRING,         ///< const char* (null-terminated)
        ARRAY_INT64,    ///< int64_t* (pointer to array elements)
        ARRAY_DOUBLE,   ///< double* (pointer to array elements)
        POINTER         ///< void* (opaque pointer)
    };

    /**
     * @brief FFI function signature
     *
     * Describes parameter types and return type for a C function
     */
    struct FFISignature {
        std::vector<FFIType> param_types;   ///< Parameter types in order
        FFIType return_type;                ///< Return type
        std::string symbol_name;            ///< Symbol name in shared library
    };

    /**
     * @brief Registered FFI function
     */
    struct FFIFunction {
        void* function_ptr;         ///< Function pointer from dlsym
        FFISignature signature;     ///< Type signature
        std::string name;           ///< Volta-side name
    };

    /**
     * @brief Construct FFI manager
     * @param vm VM instance
     */
    explicit FFIManager(VM* vm);

    /**
     * @brief Destructor (closes all libraries)
     */
    ~FFIManager();

    // ========================================================================
    // Library Loading
    // ========================================================================

    /**
     * @brief Load a shared library
     * @param path Path to .so (Linux), .dylib (Mac), or .dll (Windows)
     * @return True if loaded successfully
     *
     * Implementation:
     * 1. Call dlopen(path, RTLD_LAZY) on POSIX
     *    or LoadLibrary(path) on Windows
     * 2. If failed, log error and return false
     * 3. Store handle in library_handles_
     * 4. Return true
     *
     * Example: load_library("libopenblas.so")
     */
    bool load_library(const std::string& path);

    /**
     * @brief Load standard math library
     *
     * Convenience function to load libm (sin, cos, sqrt, etc.)
     * Implementation: Call load_library("libm.so" / "libm.dylib")
     */
    void load_math_library();

    /**
     * @brief Load BLAS library
     *
     * Tries common BLAS library names:
     * - libopenblas.so
     * - libblas.so
     * - Accelerate.framework (macOS)
     */
    void load_blas_library();

    // ========================================================================
    // Function Registration
    // ========================================================================

    /**
     * @brief Register a C function
     * @param name Volta-side name
     * @param symbol_name C symbol name (often different, e.g., "dgemm_")
     * @param signature Function signature
     * @return True if registered successfully
     *
     * Implementation:
     * 1. Search all loaded libraries for symbol_name using dlsym
     * 2. If not found, log error and return false
     * 3. Create FFIFunction with pointer and signature
     * 4. Store in functions_[name]
     * 5. Return true
     *
     * Example:
     * register_function("dgemm", "dgemm_",
     *   FFISignature{
     *     {STRING, STRING, INT64, INT64, INT64, DOUBLE,
     *      ARRAY_DOUBLE, INT64, ARRAY_DOUBLE, INT64,
     *      DOUBLE, ARRAY_DOUBLE, INT64},
     *     VOID
     *   })
     */
    bool register_function(
        const std::string& name,
        const std::string& symbol_name,
        const FFISignature& signature
    );

    /**
     * @brief Register a simple function with automatic signature
     * @param name Function name (used for both Volta and C)
     * @param param_types Parameter types
     * @param return_type Return type
     */
    bool register_simple_function(
        const std::string& name,
        const std::vector<FFIType>& param_types,
        FFIType return_type
    );

    /**
     * @brief Check if function is registered
     * @param name Function name
     */
    bool has_function(const std::string& name) const;

    // ========================================================================
    // Function Calling
    // ========================================================================

    /**
     * @brief Call a registered FFI function
     * @param name Function name
     * @param args Volta arguments
     * @return Volta return value
     *
     * Implementation:
     * 1. Look up function in functions_ map
     * 2. Validate argument count matches signature
     * 3. Marshal each argument to C type
     * 4. Pause GC (vm->gc.pause())
     * 5. Call C function using libffi or manual calling convention
     * 6. Resume GC (vm->gc.resume())
     * 7. Marshal return value to Volta value
     * 8. Free any temporary marshaling buffers
     * 9. Return result
     *
     * Example:
     * // Volta code: sin(3.14159)
     * // C code: double sin(double x)
     * Value result = call("sin", {Value::float_value(3.14159)});
     */
    Value call(const std::string& name, const std::vector<Value>& args);

    // ========================================================================
    // Type Marshaling
    // ========================================================================

    /**
     * @brief Marshal Volta value to C argument
     * @param value Volta value
     * @param type Expected C type
     * @return Pointer to C data (may allocate temporary buffer)
     *
     * Implementation by type:
     * - BOOL: return new bool(value.as_bool())
     * - INT64: return new int64_t(value.as_int())
     * - DOUBLE: return new double(value.to_number())
     * - STRING: return value.as_object()->as<StringObject>()->data
     * - ARRAY_INT64:
     *   1. Get array object
     *   2. Allocate int64_t buffer
     *   3. Copy elements, converting to int64
     *   4. Return buffer pointer
     * - ARRAY_DOUBLE: Similar to ARRAY_INT64
     * - POINTER: return value.as_object() (raw pointer)
     *
     * Note: Caller must free returned pointer (except STRING/POINTER)
     */
    void* marshal_to_c(const Value& value, FFIType type);

    /**
     * @brief Marshal C return value to Volta value
     * @param c_value Pointer to C return value
     * @param type C type
     * @return Volta value
     *
     * Implementation by type:
     * - VOID: return Value::none()
     * - BOOL: return Value::bool_value(*(bool*)c_value)
     * - INT64: return Value::int_value(*(int64_t*)c_value)
     * - DOUBLE: return Value::float_value(*(double*)c_value)
     * - STRING: return create string object from (char*)c_value
     * - POINTER: return Value::object_value((Object*)c_value)
     * - ARRAY types: Not supported for return (would need size info)
     */
    Value marshal_from_c(void* c_value, FFIType type);

    /**
     * @brief Free marshaled C arguments
     * @param c_args Vector of marshaled arguments
     * @param signature Function signature
     *
     * Implementation:
     * For each argument:
     * - If type is ARRAY_*, delete[] the buffer
     * - If type is BOOL/INT64/DOUBLE, delete the single value
     * - If type is STRING/POINTER, don't free (points to Volta object)
     */
    void free_marshaled_args(
        const std::vector<void*>& c_args,
        const FFISignature& signature
    );

    // ========================================================================
    // Built-in FFI Functions
    // ========================================================================

    /**
     * @brief Register standard math functions
     *
     * Registers: sin, cos, tan, asin, acos, atan, atan2,
     *           sinh, cosh, tanh, exp, log, log10, log2,
     *           sqrt, cbrt, pow, abs, fabs, floor, ceil,
     *           round, fmod, fmax, fmin
     *
     * Implementation:
     * 1. Call load_math_library()
     * 2. For each function, call register_simple_function()
     */
    void register_math_functions();

    /**
     * @brief Register BLAS functions
     *
     * Registers level 1, 2, 3 BLAS operations:
     * - Level 1: daxpy, ddot, dnrm2, dscal, etc.
     * - Level 2: dgemv, dger, etc.
     * - Level 3: dgemm, dsyrk, etc.
     *
     * Implementation:
     * 1. Call load_blas_library()
     * 2. Register each function with proper signature
     *    (note: Fortran adds '_' suffix to names)
     */
    void register_blas_functions();

    /**
     * @brief Register LAPACK functions
     *
     * Registers common LAPACK operations:
     * - Linear solvers: dgesv, dposv, etc.
     * - Eigenvalue: dsyev, dgeev, etc.
     * - SVD: dgesvd, etc.
     */
    void register_lapack_functions();

private:
    // ========================================================================
    // libffi Integration
    // ========================================================================

    /**
     * @brief Call C function using libffi
     * @param func_ptr C function pointer
     * @param signature Function signature
     * @param c_args Marshaled C arguments
     * @return Pointer to return value (caller must cast)
     *
     * Implementation:
     * 1. Create ffi_cif (call interface)
     * 2. Convert FFIType to ffi_type for each parameter
     * 3. Call ffi_prep_cif()
     * 4. Allocate return value buffer
     * 5. Call ffi_call(cif, func_ptr, return_buffer, c_args)
     * 6. Return return_buffer
     *
     * Note: This is the magic that handles calling conventions
     *       across different architectures (x86, x64, ARM, etc.)
     */
    void* call_with_libffi(
        void* func_ptr,
        const FFISignature& signature,
        const std::vector<void*>& c_args
    );

    /**
     * @brief Convert FFIType to ffi_type (libffi's type descriptor)
     */
    void* ffi_type_from_ffi_type(FFIType type);

    // ========================================================================
    // State
    // ========================================================================

    VM* vm_;                                            ///< VM instance
    std::vector<void*> library_handles_;                ///< dlopen handles
    std::unordered_map<std::string, FFIFunction> functions_;  ///< Registered functions
};

} // namespace volta::vm
