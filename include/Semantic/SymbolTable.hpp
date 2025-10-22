#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "../Type/Type.hpp"

// Forward declarations
namespace Semantic {
    class TypeRegistry;
}

namespace Semantic {

/**
 * Symbol: Represents a variable in a scope
 */
struct Symbol {
    std::string name;
    const Type::Type* type;  // Pointer to interned type
    bool is_mut;

    Symbol(const std::string& name, const Type::Type* type, bool is_mut)
        : name(name), type(type), is_mut(is_mut) {}
};

/**
 * FunctionParameter: Represents a function parameter with reference modes
 */
struct FunctionParameter {
    std::string name;
    const Type::Type* type;  // Pointer to interned type
    bool is_ref;       // Pass by reference
    bool is_mut_ref;   // Pass by mutable reference

    FunctionParameter(const std::string& name, const Type::Type* type,
                     bool is_ref = false, bool is_mut_ref = false)
        : name(name), type(type), is_ref(is_ref), is_mut_ref(is_mut_ref) {}
};

/**
 * FunctionSignature: Represents a function's type signature
 */
struct FunctionSignature {
    std::vector<FunctionParameter> parameters;
    const Type::Type* return_type;  // Pointer to interned type
    bool isExtern;                   // Whether this is an extern function

    FunctionSignature(const std::vector<FunctionParameter>& parameters,
                     const Type::Type* return_type,
                     bool isExtern = false)
        : parameters(parameters), return_type(return_type), isExtern(isExtern) {}
};

/**
 * Scope: Maps variable names to their symbols in a single scope
 */
using Scope = std::unordered_map<std::string, Symbol>;

/**
 * SymbolTable: Manages variable and function scopes for semantic analysis
 *
 * The SymbolTable is a ledger for all types and definitions.
 * It is the primary component that allows correct semantic analysis.
 */
class SymbolTable {
private:
    // Scope stack (scopes[0] is global scope)
    std::vector<Scope> scopes;

    // Function registry (name -> signature)
    std::unordered_map<std::string, FunctionSignature> functions;

public:
    SymbolTable();

    /**
     * Push a new scope onto the stack when entering a block
     * (function, loop, if statement, etc.)
     */
    void enterScope();

    /**
     * Pop the current scope from the stack when exiting a block
     * Throws if attempting to pop global scope
     */
    void exitScope();

    /**
     * Get the current (innermost) scope
     */
    Scope& currentScope();
    const Scope& currentScope() const;

    /**
     * Check if currently in global scope
     */
    bool isGlobalScope() const;

    /**
     * Add a variable to the current scope
     * Returns false if variable already exists in current scope
     */
    bool define(const std::string& name, const Type::Type* type, bool is_mut);

    /**
     * Look up a variable from current scope back to global scope
     * Returns nullptr if not found
     */
    const Symbol* lookup(const std::string& name) const;

    /**
     * Check if a variable exists in the current scope (not parent scopes)
     */
    bool existsInCurrentScope(const std::string& name) const;

    /**
     * Check if a variable exists in any scope
     */
    bool exists(const std::string& name) const;

    /**
     * Enter a new scope for a function
     * Note: Parameters are added separately by SemanticAnalyzer
     */
    void enterFunction();

    /**
     * Exit function scope
     */
    void exitFunction();

    /**
     * Add a function to the symbol table
     * Returns false if function already exists
     */
    bool addFunction(const std::string& name, const FunctionSignature& signature);

    /**
     * Look up a function by name
     * Returns nullptr if not found
     */
    const FunctionSignature* lookupFunction(const std::string& name) const;

    /**
     * Check if a function exists
     */
    bool functionExists(const std::string& name) const;

    /**
     * Get current scope depth (0 = global, 1 = first nested scope, etc.)
     */
    size_t scopeDepth() const;
};

} // namespace Semantic
