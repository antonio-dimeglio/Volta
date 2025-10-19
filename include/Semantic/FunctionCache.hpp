#pragma once
#include "Semantic/SymbolTable.hpp"
#include "Semantic/TypeRegistry.hpp"
#include "Semantic/ImportResolver.hpp"
#include "Parser/AST.hpp"
#include "Error/Error.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace Semantic {

// Cached function signature (interned types, survives AST destruction)
struct FunctionSignatureCache {
    std::string name;
    std::vector<FunctionParameter> params;
    const Type* returnType;

    FunctionSignatureCache(const FnDecl* fn, TypeRegistry& typeRegistry);
};

using FunctionCache = std::unordered_map<std::string, std::unordered_map<std::string, FunctionSignatureCache>>;

// Add imported function declarations to the symbol table
void addImportedFunctionsToSymbolTable(
    SymbolTable& symbolTable,
    TypeRegistry& typeRegistry,
    const std::string& currentModule,
    const ImportMap& importMap,
    const FunctionCache& functionCache,
    DiagnosticManager& diag
);

}  // namespace Semantic
