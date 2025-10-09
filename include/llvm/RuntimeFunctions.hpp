#pragma once

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>

namespace volta::llvm_codegen {

/**
 * Manages all Volta runtime function declarations
 *
 * This class encapsulates the interface between the compiler and the
 * Volta runtime library. It declares all external C functions that
 * compiled Volta programs will call at runtime.
 *
 * All functions are declared once at initialization, then cached for
 * fast repeated access during code generation.
 *
 * Runtime functions are implemented in src/runtime/volta_runtime.c
 */
class RuntimeFunctions {
public:
    /**
     * Initialize and declare all runtime functions
     *
     * @param module The LLVM module to declare functions in
     * @param context The LLVM context for creating types
     */
    RuntimeFunctions(llvm::Module* module, llvm::LLVMContext* context);

    // ========================================================================
    // Memory Management
    // ========================================================================

    /**
     * Get volta_gc_alloc function
     *
     * Signature: void* volta_gc_alloc(size_t size)
     * Allocates garbage-collected memory.
     */
    llvm::Function* getGCAlloc() const { return gcAlloc_; }

    /**
     * Get volta_gc_collect function
     *
     * Signature: void volta_gc_collect()
     * Triggers garbage collection cycle.
     */
    llvm::Function* getGCCollect() const { return gcCollect_; }

    // ========================================================================
    // Print Functions (for built-in print())
    // ========================================================================

    /**
     * Get volta_print_int function
     *
     * Signature: void volta_print_int(i64 value)
     * Prints a signed integer to stdout.
     */
    llvm::Function* getPrintInt() const { return printInt_; }

    /**
     * Get volta_print_uint function
     *
     * Signature: void volta_print_uint(u64 value)
     * Prints an unsigned integer to stdout.
     */
    llvm::Function* getPrintUInt() const { return printUInt_; }

    /**
     * Get volta_print_float function
     *
     * Signature: void volta_print_float(double value)
     * Prints a floating-point number to stdout.
     */
    llvm::Function* getPrintFloat() const { return printFloat_; }

    /**
     * Get volta_print_bool function
     *
     * Signature: void volta_print_bool(i8 value)
     * Prints a boolean (true/false) to stdout.
     */
    llvm::Function* getPrintBool() const { return printBool_; }

    /**
     * Get volta_print_string function
     *
     * Signature: void volta_print_string(i8* str)
     * Prints a null-terminated C string to stdout.
     */
    llvm::Function* getPrintString() const { return printString_; }

    // ========================================================================
    // String Functions
    // ========================================================================

    /**
     * Get volta_string_new function
     *
     * Signature: VoltaString* volta_string_new(i8* data, size_t length)
     * Creates a new Volta string from C string data.
     */
    llvm::Function* getStringNew() const { return stringNew_; }

    /**
     * Get volta_string_length function
     *
     * Signature: i64 volta_string_length(VoltaString* str)
     * Returns the length of a Volta string.
     */
    llvm::Function* getStringLength() const { return stringLength_; }

    /**
     * Get volta_string_eq function
     *
     * Signature: i1 volta_string_eq(VoltaString* a, VoltaString* b)
     * Compares two strings for equality.
     */
    llvm::Function* getStringEq() const { return stringEq_; }

    /**
     * Get volta_string_cmp function
     *
     * Signature: i32 volta_string_cmp(VoltaString* a, VoltaString* b)
     * Lexicographically compares two strings.
     * Returns: <0 if a<b, 0 if a==b, >0 if a>b
     */
    llvm::Function* getStringCmp() const { return stringCmp_; }

    /**
     * Get volta_string_concat function
     *
     * Signature: VoltaString* volta_string_concat(VoltaString* a, VoltaString* b)
     * Concatenates two strings, returns new string.
     */
    llvm::Function* getStringConcat() const { return stringConcat_; }

    // ========================================================================
    // Array Functions
    // ========================================================================

    /**
     * Get volta_array_new function
     *
     * Signature: VoltaArray* volta_array_new(size_t capacity)
     * Creates a new empty array with given capacity.
     */
    llvm::Function* getArrayNew() const { return arrayNew_; }

    /**
     * Get volta_array_length function
     *
     * Signature: i64 volta_array_length(VoltaArray* arr)
     * Returns the length of an array.
     */
    llvm::Function* getArrayLength() const { return arrayLength_; }

    /**
     * Get volta_array_push function
     *
     * Signature: void volta_array_push(VoltaArray* arr, void* elem)
     * Appends an element to the end of an array.
     */
    llvm::Function* getArrayPush() const { return arrayPush_; }

    /**
     * Get volta_array_get function
     *
     * Signature: void* volta_array_get(VoltaArray* arr, i64 index)
     * Gets element at index (with bounds checking).
     */
    llvm::Function* getArrayGet() const { return arrayGet_; }

    /**
     * Get volta_array_set function
     *
     * Signature: void volta_array_set(VoltaArray* arr, i64 index, void* elem)
     * Sets element at index (with bounds checking).
     */
    llvm::Function* getArraySet() const { return arraySet_; }

    // ========================================================================
    // Math Functions (for future use)
    // ========================================================================

    // TODO: Add when implementing math module
    // llvm::Function* getMathSqrt() const { return mathSqrt_; }
    // llvm::Function* getMathPow() const { return mathPow_; }
    // etc.

private:
    /**
     * Declare all runtime functions in the module
     *
     * Called once during construction.
     */
    void declareAll();

    /**
     * Helper to create a function declaration
     *
     * @param name Function name (e.g., "volta_print_int")
     * @param returnType LLVM return type
     * @param paramTypes Vector of LLVM parameter types
     * @return Declared function
     */
    llvm::Function* declareFunction(
        const char* name,
        llvm::Type* returnType,
        std::vector<llvm::Type*> paramTypes
    );

    // LLVM infrastructure (not owned)
    llvm::Module* module_;
    llvm::LLVMContext* context_;

    // Cached function pointers (owned by module_)
    // Memory Management
    llvm::Function* gcAlloc_;
    llvm::Function* gcCollect_;

    // Print Functions
    llvm::Function* printInt_;
    llvm::Function* printUInt_;
    llvm::Function* printFloat_;
    llvm::Function* printBool_;
    llvm::Function* printString_;

    // String Functions
    llvm::Function* stringNew_;
    llvm::Function* stringLength_;
    llvm::Function* stringEq_;
    llvm::Function* stringCmp_;
    llvm::Function* stringConcat_;

    // Array Functions
    llvm::Function* arrayNew_;
    llvm::Function* arrayLength_;
    llvm::Function* arrayPush_;
    llvm::Function* arrayGet_;
    llvm::Function* arraySet_;
};

} // namespace volta::llvm_codegen
