#include "Semantic/FunctionRegistry.hpp"

namespace Semantic {

void FunctionRegistry::registerFunction(const std::string& name,
                                       const std::vector<FunctionParameter>& params,
                                       const Type::Type* returnType,
                                       const std::string& moduleName,
                                       bool isExtern) {
    functions[name].emplace_back(name, params, returnType, moduleName, isExtern);
}

void FunctionRegistry::collectFromHIR(const HIR::HIRProgram& hir, const std::string& moduleName) {
    for (const auto& stmt : hir.statements) {
        if (auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(stmt.get())) {
            // Skip generic function templates (they have unresolved types)
            bool isGeneric = false;
            if (fnDecl->returnType->kind == Type::TypeKind::Unresolved) {
                isGeneric = true;
            }
            for (const auto& param : fnDecl->params) {
                if (param.type->kind == Type::TypeKind::Unresolved) {
                    isGeneric = true;
                    break;
                }
            }
            if (isGeneric) {
                continue;  // Skip generic templates
            }

            // Convert HIR::Param to FunctionParameter
            std::vector<FunctionParameter> funcParams;
            funcParams.reserve(fnDecl->params.size());
for (const auto& param : fnDecl->params) {
                funcParams.emplace_back(param.name, param.type);
            }
            // Only register functions (extern declarations are also functions)
            registerFunction(fnDecl->name, funcParams, fnDecl->returnType, moduleName, fnDecl->isExtern);
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
    return it->second.data();  // Return first overload for now
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
