#include "Semantic/FunctionRegistry.hpp"

namespace Semantic {

void FunctionRegistry::registerFunction(const std::string& name,
                                       const std::vector<FunctionParameter>& params,
                                       const Type::Type* returnType,
                                       const std::string& moduleName) {
    functions[name].emplace_back(name, params, returnType, moduleName);
}

void FunctionRegistry::collectFromHIR(const HIR::HIRProgram& hir, const std::string& moduleName) {
    for (const auto& stmt : hir.statements) {
        if (auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(stmt.get())) {
            // Convert HIR::Param to FunctionParameter
            std::vector<FunctionParameter> funcParams;
            for (const auto& param : fnDecl->params) {
                funcParams.emplace_back(param.name, param.type);
            }
            // Only register functions (extern declarations are also functions)
            registerFunction(fnDecl->name, funcParams, fnDecl->returnType, moduleName);
        }
    }
}

bool FunctionRegistry::hasFunction(const std::string& name) const {
    return functions.find(name) != functions.end();
}

const GlobalFunctionSignature* FunctionRegistry::getFunction(const std::string& name) const {
    auto it = functions.find(name);
    if (it == functions.end() || it->second.empty()) {
        return nullptr;
    }
    return &it->second[0];  // Return first overload for now
}

const std::vector<GlobalFunctionSignature>& FunctionRegistry::getFunctionOverloads(const std::string& name) const {
    static const std::vector<GlobalFunctionSignature> empty;
    auto it = functions.find(name);
    if (it == functions.end()) {
        return empty;
    }
    return it->second;
}

} // namespace Semantic
