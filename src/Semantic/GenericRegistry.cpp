#include "Semantic/GenericRegistry.hpp"
#include <sstream>

namespace Volta {

// Helper: Mangle a type name for use in monomorph names
static std::string mangleTypeName(const Type::Type* type);

void GenericRegistry::registerGenericFunction(const std::string& name, std::vector<Token> typeParams, FnDecl* astNode) {
    genericFunctions.emplace(name, GenericFunctionDef(name, std::move(typeParams), astNode));
}

void GenericRegistry::registerGenericStruct(const std::string& name, std::vector<Token> typeParams, StructDecl* astNode) {
    genericStructs.emplace(name, GenericStructDef(name, std::move(typeParams), astNode));
}

GenericFunctionDef* GenericRegistry::getGenericFunction(const std::string& name) {
    auto it = genericFunctions.find(name);
    if (it != genericFunctions.end()) {
        return &it->second;
    }
    return nullptr;
}

GenericStructDef* GenericRegistry::getGenericStruct(const std::string& name) {
    auto it = genericStructs.find(name);
    if (it != genericStructs.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GenericRegistry::isGenericFunction(const std::string& name) const {
    return genericFunctions.find(name) != genericFunctions.end();
}

bool GenericRegistry::isGenericStruct(const std::string& name) const {
    return genericStructs.find(name) != genericStructs.end();
}

std::string GenericRegistry::getOrCreateMonomorphName(
    const std::string& genericName,
    const std::vector<const Type::Type*>& typeArgs,
    bool isFunction
) {
    // Check if we already have this monomorph
    std::string existing = findMonomorphName(genericName, typeArgs, isFunction);
    if (!existing.empty()) {
        return existing;
    }

    // Generate new monomorph name
    std::string monomorphName = generateMonomorphName(genericName, typeArgs);

    // Record this instantiation
    if (isFunction) {
        functionInstances.emplace_back(genericName, typeArgs, monomorphName);
    } else {
        structInstances.emplace_back(genericName, typeArgs, monomorphName);
    }

    return monomorphName;
}

bool GenericRegistry::hasMonomorphInstance(
    const std::string& genericName,
    const std::vector<const Type::Type*>& typeArgs,
    bool isFunction
) const {
    return !findMonomorphName(genericName, typeArgs, isFunction).empty();
}

std::string GenericRegistry::findMonomorphName(
    const std::string& genericName,
    const std::vector<const Type::Type*>& typeArgs,
    bool isFunction
) const {
    const auto& instances = isFunction ? functionInstances : structInstances;

    for (const auto& instance : instances) {
        if (instance.genericName == genericName && typeArgsMatch(instance.typeArgs, typeArgs)) {
            return instance.monomorphName;
        }
    }

    return "";
}

std::string GenericRegistry::generateMonomorphName(
    const std::string& baseName,
    const std::vector<const Type::Type*>& typeArgs
) const {
    std::ostringstream oss;
    oss << baseName;

    for (const auto* typeArg : typeArgs) {
        oss << "$" << mangleTypeName(typeArg);
    }

    return oss.str();
}

bool GenericRegistry::typeArgsMatch(
    const std::vector<const Type::Type*>& a,
    const std::vector<const Type::Type*>& b
) const {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        // Simple pointer comparison (types should be interned in TypeRegistry)
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

// Helper function to mangle type names
static std::string mangleTypeName(const Type::Type* type) {
    if (auto* prim = dynamic_cast<const Type::PrimitiveType*>(type)) {
        switch (prim->kind) {
            case Type::PrimitiveKind::I8: return "i8";
            case Type::PrimitiveKind::I16: return "i16";
            case Type::PrimitiveKind::I32: return "i32";
            case Type::PrimitiveKind::I64: return "i64";
            case Type::PrimitiveKind::U8: return "u8";
            case Type::PrimitiveKind::U16: return "u16";
            case Type::PrimitiveKind::U32: return "u32";
            case Type::PrimitiveKind::U64: return "u64";
            case Type::PrimitiveKind::F32: return "f32";
            case Type::PrimitiveKind::F64: return "f64";
            case Type::PrimitiveKind::Bool: return "bool";
            case Type::PrimitiveKind::String: return "str";
            case Type::PrimitiveKind::Void: return "void";
        }
    } else if (auto* structType = dynamic_cast<const Type::StructType*>(type)) {
        return structType->name;
    } else if (auto* ptrType = dynamic_cast<const Type::PointerType*>(type)) {
        return "ptr$" + mangleTypeName(ptrType->pointeeType);
    } else if (auto* arrType = dynamic_cast<const Type::ArrayType*>(type)) {
        std::ostringstream oss;
        oss << "arr$" << mangleTypeName(arrType->elementType);
        for (int dim : arrType->dimensions) {
            oss << "$" << dim;
        }
        return oss.str();
    } else if (auto* genType = dynamic_cast<const Type::GenericType*>(type)) {
        std::ostringstream oss;
        oss << genType->name;
        for (const auto* arg : genType->typeParams) {
            oss << "$" << mangleTypeName(arg);
        }
        return oss.str();
    }

    return "unknown";
}

} // namespace Volta
