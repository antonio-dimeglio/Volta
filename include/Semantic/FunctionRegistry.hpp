#pragma once

#include "Parser/AST.hpp"
#include "HIR/HIR.hpp"
#include "SymbolTable.hpp"
#include "Type/TypeRegistry.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace Semantic {

// Extended function signature with module information
struct GlobalFunctionSignature {
    std::string name;
    std::vector<FunctionParameter> parameters{};
    const Type::Type* return_type;
    std::string moduleName;  // Which module this function is defined in
    bool isExtern;           // Whether this is an extern function

    GlobalFunctionSignature(const std::string& name,
                           const std::vector<FunctionParameter>& params,
                           const Type::Type* returnType,
                           const std::string& moduleName,
                           bool isExtern = false)
        : name(name), parameters(params), return_type(returnType), moduleName(moduleName), isExtern(isExtern) {}
};

// Registry of all functions across all modules
class FunctionRegistry {
private:
    // Map: function name -> list of signatures (handles overloading in future)
    std::unordered_map<std::string, std::vector<GlobalFunctionSignature>> functions{};

public:
    // Add a function signature to the registry
    void registerFunction(const std::string& name,
                         const std::vector<FunctionParameter>& params,
                         const Type::Type* returnType,
                         const std::string& moduleName,
                         bool isExtern = false);

    // Collect all public functions from an HIR program
    void collectFromHIR(const HIR::HIRProgram& hir, const std::string& moduleName);

    // Check if a function exists
    [[nodiscard]] bool hasFunction(const std::string& name) const;

    // Get function signature (returns first match for now, handles overloading later)
    [[nodiscard]] const GlobalFunctionSignature* getFunction(const std::string& name) const;

    // Get all signatures for a function name
    [[nodiscard]] const std::vector<GlobalFunctionSignature>& getFunctionOverloads(const std::string& name) const;
};

} // namespace Semantic
