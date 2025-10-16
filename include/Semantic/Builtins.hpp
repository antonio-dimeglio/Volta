#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../Parser/Type.hpp"

// Forward declarations
namespace llvm {
    class LLVMContext;
    class Type;
    class FunctionType;
}

namespace Semantic {
    class TypeRegistry;
}

/**
 * Volta Builtin Functions Registry
 *
 * This module provides a decoupled registry for builtin functions (like println, panic, len).
 * Builtins are functions that are implicitly available in all Volta programs without imports.
 *
 * Design Principles:
 * - DECOUPLED: No dependencies on SemanticAnalyzer, Codegen, or HIR/MIR
 * - STATELESS: Pure query interface, no side effects
 * - DUAL INTERFACE: Can return both Volta types (for semantic analysis) and LLVM types (for codegen)
 * - INJECTED: Components receive BuiltinRegistry as a dependency, not a global
 *
 * Usage:
 * 1. Create BuiltinRegistry with TypeRegistry dependency
 * 2. Inject into SemanticAnalyzer for type checking builtin calls
 * 3. Inject into Codegen for generating runtime function calls
 */
namespace Builtins {

/**
 * Builtin parameter specification
 * Represents a single parameter in a builtin function signature
 */
struct BuiltinParam {
    std::string name;           // Parameter name (for documentation/debugging)
    const Type* volta_type;     // Volta type (for semantic analysis)

    BuiltinParam(const std::string& name, const Type* volta_type)
        : name(name), volta_type(volta_type) {}
};

/**
 * Builtin function signature
 * Complete specification of a builtin function's type signature
 */
struct BuiltinSignature {
    std::string name;                   // Function name (e.g., "println")
    std::string runtime_name;           // Runtime function name (e.g., "volta_println")
    std::vector<BuiltinParam> params;   // Parameters
    const Type* return_type;            // Return type (Volta type)
    bool is_variadic;                   // True for variadic functions (e.g., printf-style)
    bool is_generic;                    // True for generic functions (e.g., len<T>)

    BuiltinSignature(const std::string& name,
                    const std::string& runtime_name,
                    const std::vector<BuiltinParam>& params,
                    const Type* return_type,
                    bool is_variadic = false,
                    bool is_generic = false)
        : name(name), runtime_name(runtime_name), params(params),
          return_type(return_type), is_variadic(is_variadic), is_generic(is_generic) {}
};

/**
 * Builtin Registry
 *
 * Central registry for all builtin functions in Volta.
 * Provides query interface for both semantic analysis (Volta types) and codegen (LLVM types).
 *
 * Thread-safe: Yes (read-only after construction)
 * Ownership: Does not own types (types are owned by TypeRegistry)
 */
class BuiltinRegistry {
public:
    /**
     * Construct builtin registry with type registry dependency
     * @param type_registry Reference to type registry (for looking up Volta types)
     */
    explicit BuiltinRegistry(const Semantic::TypeRegistry& type_registry);

    // Disable copy/move (singleton-like usage pattern)
    BuiltinRegistry(const BuiltinRegistry&) = delete;
    BuiltinRegistry& operator=(const BuiltinRegistry&) = delete;

    // ========================================================================
    // Query Interface (for both SemanticAnalyzer and Codegen)
    // ========================================================================

    /**
     * Check if a function name is a builtin
     * @param name Function name to check
     * @return true if builtin, false otherwise
     */
    bool isBuiltin(const std::string& name) const;

    /**
     * Look up a builtin signature by name
     * @param name Function name
     * @return Pointer to signature, or nullptr if not found
     */
    const BuiltinSignature* lookup(const std::string& name) const;

    /**
     * Get all registered builtin names
     * @return Vector of builtin function names
     */
    std::vector<std::string> getAllNames() const;

    /**
     * Get the runtime name for a builtin (for codegen)
     * @param name Volta function name (e.g., "println")
     * @return Runtime function name (e.g., "volta_println"), or empty string if not found
     */
    std::string getRuntimeName(const std::string& name) const;

    // ========================================================================
    // LLVM Interface (for Codegen)
    // ========================================================================

    /**
     * Convert a builtin signature to an LLVM FunctionType
     * This allows codegen to declare external functions with correct signatures.
     *
     * @param name Builtin function name
     * @param context LLVM context for type creation
     * @return LLVM FunctionType, or nullptr if builtin not found
     */
    llvm::FunctionType* toLLVMFunctionType(const std::string& name,
                                          llvm::LLVMContext& context) const;

    /**
     * Get LLVM parameter types for a builtin
     * @param name Builtin function name
     * @param context LLVM context for type creation
     * @return Vector of LLVM parameter types (empty if not found)
     */
    std::vector<llvm::Type*> getLLVMParamTypes(const std::string& name,
                                               llvm::LLVMContext& context) const;

    /**
     * Get LLVM return type for a builtin
     * @param name Builtin function name
     * @param context LLVM context for type creation
     * @return LLVM return type, or nullptr if not found
     */
    llvm::Type* getLLVMReturnType(const std::string& name,
                                  llvm::LLVMContext& context) const;

private:
    // Reference to type registry (for Volta type lookups)
    const Semantic::TypeRegistry& type_registry;

    // Builtin function registry (name -> signature)
    std::unordered_map<std::string, BuiltinSignature> builtins;

    /**
     * Register all builtin functions
     * Called during construction to populate the registry.
     */
    void registerBuiltins();

    /**
     * Helper: Register a single builtin
     */
    void registerBuiltin(const BuiltinSignature& signature);

    // ========================================================================
    // Builtin Registration Helpers
    // ========================================================================

    /**
     * Register I/O builtins (println, print)
     */
    void registerIOBuiltins();

    /**
     * Register panic/error builtins (panic, assert)
     */
    void registerPanicBuiltins();

    /**
     * Register array/collection builtins (len)
     */
    void registerArrayBuiltins();

    /**
     * Register conversion builtins (to_string for various types)
     */
    void registerConversionBuiltins();

    /**
     * Register debug builtins (dbg, sizeof)
     */
    void registerDebugBuiltins();
};

} // namespace Builtins
