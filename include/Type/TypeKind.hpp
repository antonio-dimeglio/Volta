#pragma once 

namespace Type {

enum class PrimitiveKind {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    Bool,
    Void,
    String
};

enum class TypeKind {
    Primitive,
    Array,
    Pointer,
    Struct,
    Generic,
    Opaque,
    Unresolved  // For forward-declared types (e.g., struct names before semantic analysis)
};

};