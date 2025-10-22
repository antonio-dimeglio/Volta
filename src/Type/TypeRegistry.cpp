#include "Type/TypeRegistry.hpp"

namespace Type {

TypeRegistry::TypeRegistry() {
    auto createPrimitive = [this](PrimitiveKind kind) -> PrimitiveType* {
        auto type = std::make_unique<PrimitiveType>(kind);
        PrimitiveType* ptr = type.get();
        ownedTypes.push_back(std::move(type));
        return ptr;
    };

    primitiveCache[PrimitiveKind::I8]     = createPrimitive(PrimitiveKind::I8);
    primitiveCache[PrimitiveKind::I16]    = createPrimitive(PrimitiveKind::I16);
    primitiveCache[PrimitiveKind::I32]    = createPrimitive(PrimitiveKind::I32);
    primitiveCache[PrimitiveKind::I64]    = createPrimitive(PrimitiveKind::I64);
    primitiveCache[PrimitiveKind::U8]     = createPrimitive(PrimitiveKind::U8);
    primitiveCache[PrimitiveKind::U16]    = createPrimitive(PrimitiveKind::U16);
    primitiveCache[PrimitiveKind::U32]    = createPrimitive(PrimitiveKind::U32);
    primitiveCache[PrimitiveKind::U64]    = createPrimitive(PrimitiveKind::U64);
    primitiveCache[PrimitiveKind::F32]    = createPrimitive(PrimitiveKind::F32);
    primitiveCache[PrimitiveKind::F64]    = createPrimitive(PrimitiveKind::F64);
    primitiveCache[PrimitiveKind::Bool]   = createPrimitive(PrimitiveKind::Bool);
    primitiveCache[PrimitiveKind::Void]   = createPrimitive(PrimitiveKind::Void);
    primitiveCache[PrimitiveKind::String] = createPrimitive(PrimitiveKind::String);
}

const PrimitiveType* TypeRegistry::getPrimitive(PrimitiveKind kind) {
    return primitiveCache.at(kind);
}

const ArrayType* TypeRegistry::getArray(const Type* elementType, int size) {
    ArrayTypeKey key{elementType, size};
    auto it = arrayCache.find(key);
    if (it != arrayCache.end()) {
        return it->second;
    }

    auto arrayType = std::make_unique<ArrayType>(elementType, size);
    ArrayType* ptr = arrayType.get();
    ownedTypes.push_back(std::move(arrayType));
    arrayCache[key] = ptr;
    return ptr;
}

const PointerType* TypeRegistry::getPointer(const Type* pointeeType) {
    auto it = pointerCache.find(pointeeType);
    if (it != pointerCache.end()) {
        return it->second;
    }

    auto ptrType = std::make_unique<PointerType>(pointeeType);
    PointerType* ptr = ptrType.get();
    ownedTypes.push_back(std::move(ptrType));
    pointerCache[pointeeType] = ptr;
    return ptr;
}

const GenericType* TypeRegistry::getGeneric(const std::string& name, 
                                            const std::vector<const Type*>& typeParams) {
    GenericTypeKey key{name, typeParams};

    auto it = genericCache.find(key);
    if (it != genericCache.end()) {
        return it->second;
    }

    auto genType = std::make_unique<GenericType>(name, typeParams);
    GenericType* ptr = genType.get();
    ownedTypes.push_back(std::move(genType));
    genericCache[key] = ptr;
    return ptr;
}

const OpaqueType* TypeRegistry::getOpaque() {
    if (!opaqueType) {
        auto opaque = std::make_unique<OpaqueType>();
        opaqueType = opaque.get();
        ownedTypes.push_back(std::move(opaque));
    }
    return opaqueType;
}

const UnresolvedType* TypeRegistry::getUnresolved(const std::string& name) {
    auto it = unresolvedCache.find(name);
    if (it != unresolvedCache.end()) {
        return it->second;
    }

    auto unresolvedType = std::make_unique<UnresolvedType>(name);
    UnresolvedType* ptr = unresolvedType.get();
    ownedTypes.push_back(std::move(unresolvedType));
    unresolvedCache[name] = ptr;
    return ptr;
}

const Type* TypeRegistry::parseTypeName(const std::string& name) {
    if (name == "i8")     return getPrimitive(PrimitiveKind::I8);
    if (name == "i16")    return getPrimitive(PrimitiveKind::I16);
    if (name == "i32")    return getPrimitive(PrimitiveKind::I32);
    if (name == "i64")    return getPrimitive(PrimitiveKind::I64);
    if (name == "u8")     return getPrimitive(PrimitiveKind::U8);
    if (name == "u16")    return getPrimitive(PrimitiveKind::U16);
    if (name == "u32")    return getPrimitive(PrimitiveKind::U32);
    if (name == "u64")    return getPrimitive(PrimitiveKind::U64);
    if (name == "f32")    return getPrimitive(PrimitiveKind::F32);
    if (name == "f64")    return getPrimitive(PrimitiveKind::F64);
    if (name == "bool")   return getPrimitive(PrimitiveKind::Bool);
    if (name == "void")   return getPrimitive(PrimitiveKind::Void);
    if (name == "str")    return getPrimitive(PrimitiveKind::String);

    // Check if it's a registered struct type
    auto structIt = structCache.find(name);
    if (structIt != structCache.end()) {
        return structIt->second;
    }

    return nullptr;
}

StructType* TypeRegistry::registerStruct(const std::string& name,
                                          const std::vector<FieldInfo>& fields) {
    // Check if already exists
    auto it = structCache.find(name);
    if (it != structCache.end()) {
        // If it exists as a stub (no fields), update it with full definition
        StructType* existing = it->second;
        if (existing->fields.empty() && !fields.empty()) {
            existing->fields = fields;
            return existing;
        }
        return nullptr;  // Already fully registered
    }

    // Create new struct type
    auto structType = std::make_unique<StructType>(name, fields);
    StructType* ptr = structType.get();
    ownedTypes.push_back(std::move(structType));
    structCache[name] = ptr;
    return ptr;
}

StructType* TypeRegistry::registerStructStub(const std::string& name) {
    // Check if already exists
    auto it = structCache.find(name);
    if (it != structCache.end()) {
        return it->second;  // Return existing (stub or full)
    }

    // Create new struct stub (no fields yet)
    auto structType = std::make_unique<StructType>(name, std::vector<FieldInfo>{});
    StructType* ptr = structType.get();
    ownedTypes.push_back(std::move(structType));
    structCache[name] = ptr;
    return ptr;
}

StructType* TypeRegistry::getStruct(const std::string& name) {
    auto it = structCache.find(name);
    if (it != structCache.end()) {
        return it->second;
    }
    return nullptr;
}

bool TypeRegistry::hasStruct(const std::string& name) const {
    return structCache.find(name) != structCache.end();
}

} // namespace Type

