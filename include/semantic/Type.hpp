#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace volta::semantic {

// Forward declarations
class TypeVariable;

/**
 * Semantic type representation (resolved, comparable)
 * Different from ast::Type (which is just parsed syntax)
 */
class Type {
public:
    enum class Kind {
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        F8, F16, F32, F64,
        Bool, String, Void,
        Array, Option, Tuple,
        Function, Struct, TypeVariable, Enum, Unknown
    };

    virtual ~Type() = default;
    virtual Kind kind() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type* other) const = 0;

    // Type trait helpers
    bool isNumeric() const {
        return kind() == Kind::I8 || kind() == Kind::I16 || kind() == Kind::I32 || kind() == Kind::I64 ||
               kind() == Kind::U8 || kind() == Kind::U16 || kind() == Kind::U32 || kind() == Kind::U64 ||
               kind() == Kind::F8 || kind() == Kind::F16 || kind() == Kind::F32 || kind() == Kind::F64;
    }

    bool isSignedInteger() const {
        return kind() == Kind::I8 || kind() == Kind::I16 || kind() == Kind::I32 || kind() == Kind::I64;
    }

    bool isUnsignedInteger() const {
        return kind() == Kind::U8 || kind() == Kind::U16 || kind() == Kind::U32 || kind() == Kind::U64;
    }

    bool isInteger() const {
        return isSignedInteger() || isUnsignedInteger();
    }

    bool isFloat() const {
        return kind() == Kind::F8 || kind() == Kind::F16 || kind() == Kind::F32 || kind() == Kind::F64;
    }

    bool isComparable() const {
        // Numbers, booleans, and strings support comparison operators
        return isNumeric() || kind() == Kind::Bool || kind() == Kind::String;
    }

    bool isIndexable() const {
        // Arrays and matrices support indexing
        return kind() == Kind::Array;
    }
};

class PrimitiveType : public Type {
public:
    enum class PrimitiveKind {
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        F8, F16, F32, F64,
        Bool, String, Void
    };

    explicit PrimitiveType(PrimitiveKind kind) : kind_(kind) {}

    Kind kind() const override {
        switch (kind_) {
            case PrimitiveKind::I8: return Kind::I8;
            case PrimitiveKind::I16: return Kind::I16;
            case PrimitiveKind::I32: return Kind::I32;
            case PrimitiveKind::I64: return Kind::I64;
            case PrimitiveKind::U8: return Kind::U8;
            case PrimitiveKind::U16: return Kind::U16;
            case PrimitiveKind::U32: return Kind::U32;
            case PrimitiveKind::U64: return Kind::U64;
            case PrimitiveKind::F8: return Kind::F8;
            case PrimitiveKind::F16: return Kind::F16;
            case PrimitiveKind::F32: return Kind::F32;
            case PrimitiveKind::F64: return Kind::F64;
            case PrimitiveKind::Bool: return Kind::Bool;
            case PrimitiveKind::String: return Kind::String;
            case PrimitiveKind::Void: return Kind::Void;
        }
        return Kind::Unknown;  // Should never reach here
    }

    std::string toString() const override {
        switch (kind_) {
            case PrimitiveKind::I8: return "i8";
            case PrimitiveKind::I16: return "i16";
            case PrimitiveKind::I32: return "i32";
            case PrimitiveKind::I64: return "i64";
            case PrimitiveKind::U8: return "u8";
            case PrimitiveKind::U16: return "u16";
            case PrimitiveKind::U32: return "u32";
            case PrimitiveKind::U64: return "u64";
            case PrimitiveKind::F8: return "f8";
            case PrimitiveKind::F16: return "f16";
            case PrimitiveKind::F32: return "f32";
            case PrimitiveKind::F64: return "f64";
            case PrimitiveKind::Bool: return "bool";
            case PrimitiveKind::String: return "str";
            case PrimitiveKind::Void: return "void";
        }
        return "unknown";  // Should never reach here
    }

    bool equals(const Type* other) const override {
        if (auto* prim = dynamic_cast<const PrimitiveType*>(other)) {
            return kind_ == prim->kind_;
        }
        return false;
    }

    PrimitiveKind primitiveKind() const { return kind_; }

private:
    PrimitiveKind kind_;
};

class ArrayType : public Type {
public:
    explicit ArrayType(std::shared_ptr<Type> elementType)
        : elementType_(std::move(elementType)) {}

    Kind kind() const override { return Kind::Array; }

    std::string toString() const override {
        return "Array[" + elementType_->toString() + "]";
    }

    bool equals(const Type* other) const override {
        if (auto* arr = dynamic_cast<const ArrayType*>(other)) {
            return elementType_->equals(arr->elementType_.get());
        }
        return false;
    }

    std::shared_ptr<Type> elementType() const { return elementType_; }

private:
    std::shared_ptr<Type> elementType_;
};

class OptionType : public Type {
public:
    explicit OptionType(std::shared_ptr<Type> innerType)
        : innerType_(std::move(innerType)) {}

    Kind kind() const override { return Kind::Option; }

    std::string toString() const override {
        return "Option[" + innerType_->toString() + "]";
    }

    bool equals(const Type* other) const override {
        if (auto* opt = dynamic_cast<const OptionType*>(other)) {
            return innerType_->equals(opt->innerType_.get());
        }
        return false;
    }

    std::shared_ptr<Type> innerType() const { return innerType_; }

private:
    std::shared_ptr<Type> innerType_;
};

class TupleType : public Type {
public:
    explicit TupleType(std::vector<std::shared_ptr<Type>> elementTypes)
        : elementTypes_(std::move(elementTypes)) {}

    Kind kind() const override { return Kind::Tuple; }

    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < elementTypes_.size(); ++i) {
            if (i > 0) result += ", ";
            result += elementTypes_[i]->toString();
        }
        result += ")";
        return result;
    }

    bool equals(const Type* other) const override {
        if (auto* tup = dynamic_cast<const TupleType*>(other)) {
            if (elementTypes_.size() != tup->elementTypes_.size()) {
                return false;
            }
            for (size_t i = 0; i < elementTypes_.size(); ++i) {
                if (!elementTypes_[i]->equals(tup->elementTypes_[i].get())) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    const std::vector<std::shared_ptr<Type>>& elementTypes() const { return elementTypes_; }

private:
    std::vector<std::shared_ptr<Type>> elementTypes_;
};

class FunctionType : public Type {
public:
    FunctionType(std::vector<std::shared_ptr<Type>> paramTypes,
                 std::shared_ptr<Type> returnType,
                 std::vector<bool> paramMutability = {})
        : paramTypes_(std::move(paramTypes)),
          returnType_(std::move(returnType)),
          paramMutability_(std::move(paramMutability)) {
        // If mutability not specified, default all to false
        if (paramMutability_.empty() && !paramTypes_.empty()) {
            paramMutability_.resize(paramTypes_.size(), false);
        }
    }

    Kind kind() const override { return Kind::Function; }

    std::string toString() const override {
        std::string result = "fn(";
        for (size_t i = 0; i < paramTypes_.size(); ++i) {
            if (i > 0) result += ", ";
            if (i < paramMutability_.size() && paramMutability_[i]) {
                result += "mut ";
            }
            result += paramTypes_[i]->toString();
        }
        result += ") -> " + returnType_->toString();
        return result;
    }

    bool equals(const Type* other) const override {
        if (auto* fn = dynamic_cast<const FunctionType*>(other)) {
            if (paramTypes_.size() != fn->paramTypes_.size()) {
                return false;
            }
            for (size_t i = 0; i < paramTypes_.size(); ++i) {
                if (!paramTypes_[i]->equals(fn->paramTypes_[i].get())) {
                    return false;
                }
            }
            // Mutability must also match for function types to be equal
            if (paramMutability_ != fn->paramMutability_) {
                return false;
            }
            return returnType_->equals(fn->returnType_.get());
        }
        return false;
    }

    const std::vector<std::shared_ptr<Type>>& paramTypes() const { return paramTypes_; }
    std::shared_ptr<Type> returnType() const { return returnType_; }
    const std::vector<bool>& paramMutability() const { return paramMutability_; }

private:
    std::vector<std::shared_ptr<Type>> paramTypes_;
    std::shared_ptr<Type> returnType_;
    std::vector<bool> paramMutability_;
};

class StructType : public Type {
public:
    struct Field {
        std::string name;
        std::shared_ptr<Type> type;

        Field(std::string name, std::shared_ptr<Type> type)
            : name(std::move(name)), type(std::move(type)) {}
    };

    // Constructor for generic struct template (e.g., Pair[T])
    StructType(std::string name,
               std::vector<std::string> typeParams,
               std::vector<Field> fields)
        : name_(std::move(name)),
          typeParams_(std::move(typeParams)),
          typeArgs_(),  // No type arguments - this is the template
          fields_(std::move(fields)),
          isInstantiated_(false) {
        // Populate the field index for O(1) lookup
        for (size_t i = 0; i < fields_.size(); ++i) {
            fieldIndex_[fields_[i].name] = i;
        }
    }

    // Constructor for instantiated struct (e.g., Pair[int])
    StructType(std::string name,
               std::vector<std::string> typeParams,
               std::vector<std::shared_ptr<Type>> typeArgs,
               std::vector<Field> fields)
        : name_(std::move(name)),
          typeParams_(std::move(typeParams)),
          typeArgs_(std::move(typeArgs)),
          fields_(std::move(fields)),
          isInstantiated_(true) {
        // Populate the field index for O(1) lookup
        for (size_t i = 0; i < fields_.size(); ++i) {
            fieldIndex_[fields_[i].name] = i;
        }
    }

    // Legacy constructor for non-generic structs
    StructType(std::string name, std::vector<Field> fields)
        : name_(std::move(name)),
          typeParams_(),
          typeArgs_(),
          fields_(std::move(fields)),
          isInstantiated_(false) {
        // Populate the field index for O(1) lookup
        for (size_t i = 0; i < fields_.size(); ++i) {
            fieldIndex_[fields_[i].name] = i;
        }
    }

    Kind kind() const override { return Kind::Struct; }

    std::string toString() const override {
        std::string s = name_;

        // If instantiated, show concrete types (Pair[int])
        if (isInstantiated_ && !typeArgs_.empty()) {
            s += "[";
            for (size_t i = 0; i < typeArgs_.size(); ++i) {
                if (i > 0) s += ", ";
                s += typeArgs_[i]->toString();
            }
            s += "]";
        }
        // Otherwise show type parameters (Pair[T])
        else if (!typeParams_.empty()) {
            s += "[";
            for (size_t i = 0; i < typeParams_.size(); ++i) {
                if (i > 0) s += ", ";
                s += typeParams_[i];
            }
            s += "]";
        }
        return s;
    }

    bool equals(const Type* other) const override {
        if (auto* structType = dynamic_cast<const StructType*>(other)) {
            if (name_ != structType->name_) return false;

            // If both are instantiated, compare type arguments
            if (isInstantiated_ && structType->isInstantiated_) {
                if (typeArgs_.size() != structType->typeArgs_.size()) return false;
                for (size_t i = 0; i < typeArgs_.size(); ++i) {
                    if (!typeArgs_[i]->equals(structType->typeArgs_[i].get())) {
                        return false;
                    }
                }
                return true;
            }

            // If both are templates, compare type parameters
            if (!isInstantiated_ && !structType->isInstantiated_) {
                if (typeParams_.size() != structType->typeParams_.size()) return false;
                for (size_t i = 0; i < typeParams_.size(); ++i) {
                    if (typeParams_[i] != structType->typeParams_[i]) return false;
                }
                return true;
            }

            // One is instantiated, one is template - not equal
            return false;
        }
        return false;
    }

    const std::string& name() const { return name_; }
    const std::vector<Field>& fields() const { return fields_; }
    const std::vector<std::string>& typeParams() const { return typeParams_; }
    const std::vector<std::shared_ptr<Type>>& typeArgs() const { return typeArgs_; }
    bool isInstantiated() const { return isInstantiated_; }

    // Lookup field by name
    const Field* getField(const std::string& fieldName) const {
        auto it = fieldIndex_.find(fieldName);
        if (it != fieldIndex_.end()) {
            return &fields_[it->second];
        }
        return nullptr;
    }

    // Get field index by name (returns -1 if not found)
    int getFieldIndex(const std::string& fieldName) const {
        auto it = fieldIndex_.find(fieldName);
        if (it != fieldIndex_.end()) {
            return static_cast<int>(it->second);
        }
        return -1;
    }

    // Instantiate template with concrete types
    std::shared_ptr<StructType> instantiate(const std::vector<std::shared_ptr<Type>>& concreteTypes) const;

private:
    // Substitute type parameters in a type (defined after TypeVariable)
    std::shared_ptr<Type> substituteTypeParams(const std::shared_ptr<Type>& type,
                                               const std::vector<std::shared_ptr<Type>>& concreteTypes) const;

    std::string name_;
    std::vector<std::string> typeParams_;           // ["T"] for Pair[T] template
    std::vector<std::shared_ptr<Type>> typeArgs_;   // [intType] for Pair[int] instance
    std::vector<Field> fields_;
    std::unordered_map<std::string, size_t> fieldIndex_;
    bool isInstantiated_;
};

class TypeVariable : public Type {
public:
    explicit TypeVariable(std::string name)
        : name_(std::move(name)) {}

    Kind kind() const override { return Kind::TypeVariable; }

    std::string toString() const override {
        return name_;
    }

    bool equals(const Type* other) const override {
        if (auto* typeVar = dynamic_cast<const TypeVariable*>(other)) {
            return name_ == typeVar->name_;
        }
        return false;
    }

    const std::string& name() const { return name_; }

private:
    std::string name_;
};

// StructType method implementations (defined after TypeVariable)
inline std::shared_ptr<Type> StructType::substituteTypeParams(
    const std::shared_ptr<Type>& type,
    const std::vector<std::shared_ptr<Type>>& concreteTypes) const {
    // If it's a type variable (like T), replace it
    if (auto* typeVar = dynamic_cast<const TypeVariable*>(type.get())) {
        for (size_t i = 0; i < typeParams_.size(); ++i) {
            if (typeParams_[i] == typeVar->name()) {
                return concreteTypes[i];
            }
        }
    }
    // Otherwise return as-is (TODO: handle nested generics like Array[T])
    return type;
}

inline std::shared_ptr<StructType> StructType::instantiate(
    const std::vector<std::shared_ptr<Type>>& concreteTypes) const {
    if (concreteTypes.size() != typeParams_.size()) {
        return nullptr;  // Type argument count mismatch
    }

    // Substitute type parameters in fields
    std::vector<Field> instantiatedFields;
    for (const auto& field : fields_) {
        // Substitute type variables with concrete types
        auto substituted = substituteTypeParams(field.type, concreteTypes);
        instantiatedFields.push_back(Field{field.name, substituted});
    }

    return std::make_shared<StructType>(name_, typeParams_, concreteTypes, instantiatedFields);
}

class EnumType : public Type {
public:
    struct Variant {
        std::string name;
        std::vector<std::shared_ptr<Type>> associatedTypes;
        Variant(std::string name, std::vector<std::shared_ptr<Type>> associatedTypes)
            : name(name), associatedTypes(associatedTypes) {}
    };

    // Constructor for generic enum template (e.g., Option[T])
    EnumType(std::string name,
             std::vector<std::string> typeParams,
             std::vector<Variant> variants)
        : name_(std::move(name)),
          typeParams_(std::move(typeParams)),
          typeArgs_(),  // No type arguments - this is the template
          variants_(std::move(variants)),
          isInstantiated_(false) {}

    // Constructor for instantiated enum (e.g., Option[int])
    EnumType(std::string name,
             std::vector<std::string> typeParams,
             std::vector<std::shared_ptr<Type>> typeArgs,
             std::vector<Variant> variants)
        : name_(std::move(name)),
          typeParams_(std::move(typeParams)),
          typeArgs_(std::move(typeArgs)),
          variants_(std::move(variants)),
          isInstantiated_(true) {}

    Kind kind() const override { return Kind::Enum; }

    std::string toString() const override {
        std::string s = name_;

        // If instantiated, show concrete types (Option[int])
        if (isInstantiated_ && !typeArgs_.empty()) {
            s += "[";
            for (size_t i = 0; i < typeArgs_.size(); ++i) {
                if (i > 0) s += ", ";
                s += typeArgs_[i]->toString();
            }
            s += "]";
        }
        // Otherwise show type parameters (Option[T])
        else if (!typeParams_.empty()) {
            s += "[";
            for (size_t i = 0; i < typeParams_.size(); ++i) {
                if (i > 0) s += ", ";
                s += typeParams_[i];
            }
            s += "]";
        }
        return s;
    }

    bool equals(const Type* other) const override {
        if (auto* enumType = dynamic_cast<const EnumType*>(other)) {
            if (name_ != enumType->name_) return false;

            // If both are instantiated, compare type arguments
            if (isInstantiated_ && enumType->isInstantiated_) {
                if (typeArgs_.size() != enumType->typeArgs_.size()) return false;
                for (size_t i = 0; i < typeArgs_.size(); ++i) {
                    if (!typeArgs_[i]->equals(enumType->typeArgs_[i].get())) {
                        return false;
                    }
                }
                return true;
            }

            // If both are templates, compare type parameters
            if (!isInstantiated_ && !enumType->isInstantiated_) {
                if (typeParams_.size() != enumType->typeParams_.size()) return false;
                for (size_t i = 0; i < typeParams_.size(); ++i) {
                    if (typeParams_[i] != enumType->typeParams_[i]) return false;
                }
                return true;
            }

            // One is instantiated, one is template - not equal
            return false;
        }
        return false;
    }

    // Accessors
    const std::string& name() const { return name_; }
    const std::vector<Variant>& variants() const { return variants_; }
    const std::vector<std::string>& typeParams() const { return typeParams_; }
    const std::vector<std::shared_ptr<Type>>& typeArgs() const { return typeArgs_; }
    bool isInstantiated() const { return isInstantiated_; }

    // Lookup variant by name
    const Variant* getVariant(const std::string& variantName) const {
        for (const auto& variant : variants_) {
            if (variant.name == variantName) {
                return &variant;
            }
        }
        return nullptr;
    }

    // Create an instantiated version of this enum with concrete type arguments
    // Example: Option[T].instantiate({intType}) → Option[int]
    std::shared_ptr<EnumType> instantiate(const std::vector<std::shared_ptr<Type>>& concreteTypes) const {
        if (concreteTypes.size() != typeParams_.size()) {
            return nullptr;  // Type argument count mismatch
        }

        // Substitute type parameters in variant associated types
        std::vector<Variant> instantiatedVariants;
        for (const auto& variant : variants_) {
            std::vector<std::shared_ptr<Type>> instantiatedTypes;
            for (const auto& assocType : variant.associatedTypes) {
                // Substitute type variables with concrete types
                auto substituted = substituteTypeParams(assocType, concreteTypes);
                instantiatedTypes.push_back(substituted);
            }
            instantiatedVariants.push_back(Variant{variant.name, instantiatedTypes});
        }

        return std::make_shared<EnumType>(name_, typeParams_, concreteTypes, instantiatedVariants);
    }

private:
    // Substitute type parameters in a type
    std::shared_ptr<Type> substituteTypeParams(const std::shared_ptr<Type>& type,
                                               const std::vector<std::shared_ptr<Type>>& concreteTypes) const {
        // If it's a type variable (like T), replace it
        if (auto* typeVar = dynamic_cast<const TypeVariable*>(type.get())) {
            for (size_t i = 0; i < typeParams_.size(); ++i) {
                if (typeParams_[i] == typeVar->name()) {
                    return concreteTypes[i];
                }
            }
        }
        // Otherwise return as-is (TODO: handle nested generics like Array[T])
        return type;
    }

    std::string name_;
    std::vector<std::string> typeParams_;           // ["T"] for Option[T] template
    std::vector<std::shared_ptr<Type>> typeArgs_;   // [intType] for Option[int] instance
    std::vector<Variant> variants_;
    bool isInstantiated_;
};

class UnknownType : public Type {
public:
    UnknownType() = default;

    Kind kind() const override { return Kind::Unknown; }

    std::string toString() const override {
        return "<unknown>";
    }

    bool equals(const Type* /*other*/) const override {
        // Unknown types are never equal to anything (even other unknowns)
        return false;
    }
};

/**
 * Type cache for interning types to avoid duplicate allocations
 * Provides canonical instances of primitive and commonly used types
 */
class TypeCache {
public:
    TypeCache()
        : i8Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::I8)),
          i16Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::I16)),
          i32Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::I32)),
          i64Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::I64)),
          u8Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::U8)),
          u16Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::U16)),
          u32Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::U32)),
          u64Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::U64)),
          f8Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::F8)),
          f16Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::F16)),
          f32Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::F32)),
          f64Type_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::F64)),
          boolType_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool)),
          stringType_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String)),
          voidType_(std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Void)),
          unknownType_(std::make_shared<UnknownType>()) {}

    // Signed integer types
    std::shared_ptr<Type> getI8() const { return i8Type_; }
    std::shared_ptr<Type> getI16() const { return i16Type_; }
    std::shared_ptr<Type> getI32() const { return i32Type_; }
    std::shared_ptr<Type> getI64() const { return i64Type_; }

    // Unsigned integer types
    std::shared_ptr<Type> getU8() const { return u8Type_; }
    std::shared_ptr<Type> getU16() const { return u16Type_; }
    std::shared_ptr<Type> getU32() const { return u32Type_; }
    std::shared_ptr<Type> getU64() const { return u64Type_; }

    // Floating point types
    std::shared_ptr<Type> getF8() const { return f8Type_; }
    std::shared_ptr<Type> getF16() const { return f16Type_; }
    std::shared_ptr<Type> getF32() const { return f32Type_; }
    std::shared_ptr<Type> getF64() const { return f64Type_; }

    // Other primitive types
    std::shared_ptr<Type> getBool() const { return boolType_; }
    std::shared_ptr<Type> getString() const { return stringType_; }
    std::shared_ptr<Type> getVoid() const { return voidType_; }
    std::shared_ptr<Type> getUnknown() const { return unknownType_; }

    // Complex types - cache by structure
    std::shared_ptr<Type> getArrayType(std::shared_ptr<Type> elementType) const {
        std::string key = "Array[" + elementType->toString() + "]";
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        auto type = std::make_shared<ArrayType>(elementType);
        cache_[key] = type;
        return type;
    }

    std::shared_ptr<Type> getOptionType(std::shared_ptr<Type> innerType) const {
        std::string key = "Option[" + innerType->toString() + "]";
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        auto type = std::make_shared<OptionType>(innerType);
        cache_[key] = type;
        return type;
    }

    std::shared_ptr<Type> getTupleType(std::vector<std::shared_ptr<Type>> elementTypes) const {
        std::string key = "(";
        for (size_t i = 0; i < elementTypes.size(); ++i) {
            if (i > 0) key += ", ";
            key += elementTypes[i]->toString();
        }
        key += ")";

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        auto type = std::make_shared<TupleType>(std::move(elementTypes));
        cache_[key] = type;
        return type;
    }

    std::shared_ptr<Type> getFunctionType(std::vector<std::shared_ptr<Type>> paramTypes,
                                          std::shared_ptr<Type> returnType,
                                          std::vector<bool> paramMutability = {}) const {
        std::string key = "fn(";
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) key += ", ";
            if (!paramMutability.empty() && i < paramMutability.size() && paramMutability[i]) {
                key += "mut ";
            }
            key += paramTypes[i]->toString();
        }
        key += ") -> " + returnType->toString();

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        auto type = std::make_shared<FunctionType>(std::move(paramTypes), returnType, std::move(paramMutability));
        cache_[key] = type;
        return type;
    }

private:
    // Cached primitive types
    std::shared_ptr<Type> i8Type_;
    std::shared_ptr<Type> i16Type_;
    std::shared_ptr<Type> i32Type_;
    std::shared_ptr<Type> i64Type_;
    std::shared_ptr<Type> u8Type_;
    std::shared_ptr<Type> u16Type_;
    std::shared_ptr<Type> u32Type_;
    std::shared_ptr<Type> u64Type_;
    std::shared_ptr<Type> f8Type_;
    std::shared_ptr<Type> f16Type_;
    std::shared_ptr<Type> f32Type_;
    std::shared_ptr<Type> f64Type_;
    std::shared_ptr<Type> boolType_;
    std::shared_ptr<Type> stringType_;
    std::shared_ptr<Type> voidType_;
    std::shared_ptr<Type> unknownType_;

    // Complex type cache
    mutable std::unordered_map<std::string, std::shared_ptr<Type>> cache_;
};

} // namespace volta::semantic
