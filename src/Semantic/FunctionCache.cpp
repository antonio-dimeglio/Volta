#include "Semantic/FunctionCache.hpp"

namespace Semantic {

FunctionSignatureCache::FunctionSignatureCache(const FnDecl* fn, TypeRegistry& typeRegistry)
    : name(fn->name) {
    for (const auto& param : fn->params) {
        const Type* internedType = typeRegistry.internFromAST(param.type.get());
        params.emplace_back(param.name, internedType, param.isRef, param.isMutRef);
    }
    returnType = typeRegistry.internFromAST(fn->returnType.get());
}

void addImportedFunctionsToSymbolTable(
    SymbolTable& symbolTable,
    TypeRegistry& typeRegistry,
    const std::string& currentModule,
    const ImportMap& importMap,
    const FunctionCache& functionCache,
    DiagnosticManager& diag
) {
    if (importMap.count(currentModule) == 0) {
        return;  // No imports for this module
    }

    const auto& imports = importMap.at(currentModule);

    for (const auto& [symbolName, sourceModule] : imports) {
        // Look up the function in the cache
        if (functionCache.count(sourceModule) == 0 || functionCache.at(sourceModule).count(symbolName) == 0) {
            diag.error("Imported function '" + symbolName + "' not found in module '" + sourceModule + "'", 0, 0);
            continue;
        }

        const FunctionSignatureCache& cached = functionCache.at(sourceModule).at(symbolName);

        // Use cached signature directly (types already interned)
        FunctionSignature signature(cached.params, cached.returnType);

        // Add to symbol table as an imported function
        symbolTable.addFunction(cached.name, signature);
    }
}

}  // namespace Semantic
