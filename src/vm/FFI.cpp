#include "vm/FFI.hpp"
#include "vm/VM.hpp"
#include <ffi.h>
#include <dlfcn.h>  // POSIX: dlopen, dlsym, dlclose
#include <stdexcept>
#include "string.h"
#include <iostream>

// For libffi integration (install with: apt-get install libffi-dev)
// #include <ffi.h>

namespace volta::vm {

FFIManager::FFIManager(VM* vm) : vm_(vm) {
    // TODO: Initialize FFI manager
    //
    // Implementation:
    // Nothing special needed - constructor just stores VM pointer
    // Library loading happens via load_library() calls
}
void close_library(void* handle) {
#if defined(_WIN32)
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

FFIManager::~FFIManager() {
    for (void* handle : library_handles_) {
        close_library(handle);
    }
}

// ============================================================================
// Library Loading
// ============================================================================

bool FFIManager::load_library(const std::string& path) {
    // Note: RTLD_LAZY means symbols are resolved on first use (faster loading)

    #if defined(_WIN32)
        HMODULE handle = LoadLibraryA(path.c_str());
        if (!handle) { 
            std::cerr << "Failed to load library " << path << ": " << "\n";// What can I do here?
        } 
    #else 
        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
           const char* error = dlerror();
           std::cerr << "Failed to load library " << path << ": " << error << "\n";
           return false;
        }
    #endif
    library_handles_.push_back(handle);
    return true;
}

void FFIManager::load_math_library() {
    #ifdef __APPLE__
        // macOS: libm is part of libSystem
        load_library("libSystem.dylib");
    #elif __linux__
        load_library("libm.so.6") || load_library("libm.so");
    #elif _WIN32
        load_library("msvcrt.dll");
    #endif

    // YOUR CODE HERE
}

void FFIManager::load_blas_library() {
    // TODO: Load BLAS library
    //
    // Implementation:
    // Try common BLAS library names in order:
    // 1. OpenBLAS (most common):
    if (load_library("libopenblas.so")) return;  // Linux
    if (load_library("libopenblas.dylib")) return;  // macOS
    // 2. Standard BLAS:
   if (load_library("libblas.so")) return;
   if (load_library("libblas.dylib")) return;
    // 3. macOS Accelerate framework:
    #ifdef __APPLE__
       if (load_library("/System/Library/Frameworks/Accelerate.framework/Accelerate")) return;
    #endif
    
    std::cerr << "Failed to find BLAS definition." << "\n";
}

// ============================================================================
// Function Registration
// ============================================================================

bool FFIManager::register_function(
    const std::string& name,
    const std::string& symbol_name,
    const FFISignature& signature
) {
    void* func_ptr = nullptr;
    for (void* handle : library_handles_) {
        func_ptr = dlsym(handle, symbol_name.c_str());
        if (func_ptr) break; 
    }
    
    if (!func_ptr) {
        std::cerr << "FFI: Symbol not found: " << symbol_name << std::endl;
        return false;
    }
    
    FFIFunction ffi_func;
    ffi_func.function_ptr = func_ptr;
    ffi_func.signature = signature;
    ffi_func.signature.symbol_name = symbol_name;  
    ffi_func.name = name;

    functions_[name] = ffi_func;
    return true;
}

bool FFIManager::register_simple_function(
    const std::string& name,
    const std::vector<FFIType>& param_types,
    FFIType return_type
) {
    FFISignature sig;
    sig.param_types = param_types;
    sig.return_type = return_type;
    sig.symbol_name = name;
    return register_function(name, name, sig);

    return false;
}

bool FFIManager::has_function(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

// ============================================================================
// Function Calling
// ============================================================================

Value FFIManager::call(const std::string& name, const std::vector<Value>& args) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw std::runtime_error("FFI: Unknown function: " + name);
    }
    const FFIFunction& func = it->second;
    
    if (args.size() != func.signature.param_types.size()) {
        throw std::runtime_error("FFI: Wrong number of arguments");
    }
    
    std::vector<void*> c_args;
    for (size_t i = 0; i < args.size(); i++) {
        void* c_arg = marshal_to_c(args[i], func.signature.param_types[i]);
        c_args.push_back(c_arg);
    }
    
    vm_->gc().pause();
    
    void* result = call_with_libffi(func.function_ptr, func.signature, c_args);
    
    vm_->gc().resume();
    
    Value volta_result = marshal_from_c(result, func.signature.return_type);
    
    free_marshaled_args(c_args, func.signature);
    
    if (func.signature.return_type != FFIType::VOID) free(result);
    return volta_result;

    return Value::none();
}

// ============================================================================
// Type Marshaling
// ============================================================================

void* FFIManager::marshal_to_c(const Value& value, FFIType type) {

    // This converts Volta values to C-compatible memory.
    // IMPORTANT: Caller must free the returned pointer (except STRING/POINTER)!

    switch (type) {
        case FFIType::BOOL: {
            bool* ptr = new bool(value.as_bool());
            return ptr;
        }
    
        case FFIType::INT64: {
            int64_t* ptr = new int64_t(value.as_int());
            return ptr;
        }
    
        case FFIType::DOUBLE: {
            double* ptr = new double(value.to_number());  // Handles int→float
            return ptr;
        }
    
        case FFIType::STRING: {
            if (!value.is_object()) throw std::runtime_error("incorrect value for marshaling");
            StringObject* str = (StringObject*)value.as_object();
            return (void*)str->data;  // Points to Volta string (don't free!)
        }
    
        case FFIType::ARRAY_INT64: {
            if (!value.is_object()) throw std::runtime_error("incorrect value for marshaling");
            ArrayObject* arr = (ArrayObject*)value.as_object();
            int64_t* c_array = new int64_t[arr->length];
            for (size_t i = 0; i < arr->length; i++) {
                c_array[i] = arr->elements[i].as_int();
            }
            return c_array;
        }
    
        case FFIType::ARRAY_DOUBLE: {
            if (!value.is_object()) throw std::runtime_error("incorrect value for marshaling");
            ArrayObject* arr = (ArrayObject*)value.as_object();
            double* c_array = new double[arr->length];
            for (size_t i = 0; i < arr->length; i++) {
                c_array[i] = arr->elements[i].to_number();
            }
            return c_array;
        }
    
        case FFIType::POINTER: {
            if (!value.is_object()) throw std::runtime_error("incorrect value for marshaling");
            return (void*)value.as_object();
        }
    
        default:
            throw std::runtime_error("FFI: Unknown type for marshaling");
    }
}

Value FFIManager::marshal_from_c(void* c_value, FFIType type) {
    switch (type) {
        case FFIType::VOID:
            return Value::none();
    
        case FFIType::BOOL:
            return Value::bool_value(*(bool*)c_value);
    
        case FFIType::INT64:
            return Value::int_value(*(int64_t*)c_value);
    
        case FFIType::DOUBLE:
            return Value::float_value(*(double*)c_value);
    
        case FFIType::STRING: {
            // Create Volta string from C string
            const char* str = (const char*)c_value;
            StringObject* volta_str = StringObject::create(vm_, str, strlen(str));
            return Value::object_value(volta_str);
        }
    
        case FFIType::POINTER:
            return Value::object_value((Object*)c_value);
    
        case FFIType::ARRAY_INT64:
        case FFIType::ARRAY_DOUBLE:
            // Arrays as return values not supported (need size info)
            throw std::runtime_error("FFI: Arrays cannot be return values");
    
        default:
            throw std::runtime_error("FFI: Unknown type for marshaling");
    }
}

void FFIManager::free_marshaled_args(
    const std::vector<void*>& c_args,
    const FFISignature& signature
) {
    for (size_t i = 0; i < c_args.size(); i++) {
        FFIType type = signature.param_types[i];
        switch (type) {
            case FFIType::BOOL:
                delete (bool*)c_args[i];
                break;
            case FFIType::INT64:
                delete (int64_t*)c_args[i];
                break;
            case FFIType::DOUBLE:
                delete (double*)c_args[i];
                break;
            case FFIType::ARRAY_INT64:
                delete[] (int64_t*)c_args[i];
                break;
            case FFIType::ARRAY_DOUBLE:
                delete[] (double*)c_args[i];
                break;
            case FFIType::STRING:
            case FFIType::POINTER:
                // Don't free - points to Volta object
                break;
        }
    }
}

// ============================================================================
// Built-in FFI Functions
// ============================================================================

void FFIManager::register_math_functions() {
    // TODO: Register standard math functions
    //
    // Implementation:
    load_math_library();
    //
    // Register functions (all take double, return double):
    register_simple_function("sin", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("cos", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("tan", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("asin", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("acos", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("atan", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("sinh", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("cosh", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("tanh", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("exp", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("log", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("log10", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("sqrt", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("cbrt", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("floor", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("ceil", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("round", {FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("fabs", {FFIType::DOUBLE}, FFIType::DOUBLE);
    //
    // Two-argument functions:
    register_simple_function("pow", {FFIType::DOUBLE, FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("atan2", {FFIType::DOUBLE, FFIType::DOUBLE}, FFIType::DOUBLE);
    register_simple_function("fmod", {FFIType::DOUBLE, FFIType::DOUBLE}, FFIType::DOUBLE);

}

void FFIManager::register_blas_functions() {
    load_blas_library();
    
    // Register dgemm (note: Fortran adds '_' suffix to names):
    FFISignature dgemm_sig;
    dgemm_sig.param_types = {
        FFIType::STRING,        // transa
        FFIType::STRING,        // transb
        FFIType::INT64,         // m
        FFIType::INT64,         // n
        FFIType::INT64,         // k
        FFIType::DOUBLE,        // alpha
        FFIType::ARRAY_DOUBLE,  // A
        FFIType::INT64,         // lda
        FFIType::ARRAY_DOUBLE,  // B
        FFIType::INT64,         // ldb
        FFIType::DOUBLE,        // beta
        FFIType::ARRAY_DOUBLE,  // C (output)
        FFIType::INT64          // ldc
    };
    dgemm_sig.return_type = FFIType::VOID;
    register_function("dgemm", "dgemm_", dgemm_sig);
    
    // TODO: Add more BLAS functions (daxpy, ddot, dgemv, etc.)
}

void FFIManager::register_lapack_functions() {

    load_blas_library();  // LAPACK often bundled with BLAS
    
    FFISignature dgesv_sig;
    dgesv_sig.param_types = {
        FFIType::INT64,         // n
        FFIType::INT64,         // nrhs
        FFIType::ARRAY_DOUBLE,  // A
        FFIType::INT64,         // lda
        FFIType::ARRAY_INT64,   // ipiv (pivot indices)
        FFIType::ARRAY_DOUBLE,  // B (input/output)
        FFIType::INT64,         // ldb
        FFIType::INT64          // info (output)
    };
    dgesv_sig.return_type = FFIType::VOID;
    register_function("dgesv", "dgesv_", dgesv_sig);
}

// ============================================================================
// libffi Integration
// ============================================================================

void* FFIManager::call_with_libffi(
    void* func_ptr,
    const FFISignature& signature,
    const std::vector<void*>& c_args
) {
    // TODO: Call C function using libffi
    //
    // This is the magic that handles calling conventions!
    // libffi knows how to call functions on x86, x64, ARM, etc.
    //
    // Implementation:
    // 1. Setup argument types: // TODO: Fix this, its incorrect!
    // ffi_type** arg_types = new ffi_type*[c_args.size()];
    // for (size_t i = 0; i < c_args.size(); i++) {
    //     arg_types[i] = ffi_type_from_ffi_type(signature.param_types[i]);
    // }
    //
    // 2. Setup return type:
    // ffi_type* ret_type = ffi_type_from_ffi_type(signature.return_type);
    //
    // 3. Prepare call interface:
    //    ffi_cif cif;
    //    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI,
    //                                      c_args.size(), ret_type, arg_types);
    //    if (status != FFI_OK) throw error("ffi_prep_cif failed");
    //
    // 4. Allocate return value buffer:
    //    void* result = malloc(ret_type->size);
    //
    // 5. Call!
    //    ffi_call(&cif, FFI_FN(func_ptr), result, (void**)c_args.data());
    //
    // 6. Cleanup:
    //    delete[] arg_types;
    //
    // 7. Return result (caller will marshal and free):
    //    return result;
    //
    // Note: Install libffi with: apt-get install libffi-dev
    //       Link with: -lffi

    // YOUR CODE HERE
    return nullptr;
}

void* FFIManager::ffi_type_from_ffi_type(FFIType type) {
    switch (type) {
        case FFIType::VOID:    return &ffi_type_void;
        case FFIType::BOOL:    return &ffi_type_uint8;
        case FFIType::INT64:   return &ffi_type_sint64;
        case FFIType::DOUBLE:  return &ffi_type_double;
        case FFIType::STRING:  return &ffi_type_pointer;
        case FFIType::ARRAY_INT64:   return &ffi_type_pointer;
        case FFIType::ARRAY_DOUBLE:  return &ffi_type_pointer;
        case FFIType::POINTER: return &ffi_type_pointer;
        default: throw std::runtime_error("Unknown FFI type");
    }
}

} // namespace volta::vm
