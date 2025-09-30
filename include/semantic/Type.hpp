#pragma once
#include <memory>
#include <string>
#include <vector>

namespace volta::semantic {

/**
 * Semantic type representation (resolved, comparable)
 * Different from ast::Type (which is just parsed syntax)
 */
class Type {
public:
    enum class Kind {
        Int, Float, Bool, String, Void,
        Array, Matrix, Option, Tuple,
        Function, Struct, TypeVariable, Unknown
    };

    virtual ~Type() = default;
    virtual Kind kind() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type* other) const = 0;
};

class PrimitiveType : public Type {
public:
    enum class PrimitiveKind { Int, Float, Bool, String, Void };

    explicit PrimitiveType(PrimitiveKind kind) : kind_(kind) {}

    Kind kind() const override {
        switch (kind_) {
            case PrimitiveKind::Int: return Kind::Int;
            case PrimitiveKind::Float: return Kind::Float;
            case PrimitiveKind::Bool: return Kind::Bool;
            case PrimitiveKind::String: return Kind::String;
            case PrimitiveKind::Void: return Kind::Void;
        }
        return Kind::Unknown;  // Should never reach here
    }

    std::string toString() const override {
        switch (kind_) {
            case PrimitiveKind::Int: return "int";
            case PrimitiveKind::Float: return "float";
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

class MatrixType : public Type {
public:
    explicit MatrixType(std::shared_ptr<Type> elementType)
        : elementType_(std::move(elementType)) {}

    Kind kind() const override { return Kind::Matrix; }

    std::string toString() const override {
        return "Matrix[" + elementType_->toString() + "]";
    }

    bool equals(const Type* other) const override {
        if (auto* mat = dynamic_cast<const MatrixType*>(other)) {
            return elementType_->equals(mat->elementType_.get());
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
                 std::shared_ptr<Type> returnType)
        : paramTypes_(std::move(paramTypes)),
          returnType_(std::move(returnType)) {}

    Kind kind() const override { return Kind::Function; }

    std::string toString() const override {
        std::string result = "fn(";
        for (size_t i = 0; i < paramTypes_.size(); ++i) {
            if (i > 0) result += ", ";
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
            return returnType_->equals(fn->returnType_.get());
        }
        return false;
    }

    const std::vector<std::shared_ptr<Type>>& paramTypes() const { return paramTypes_; }
    std::shared_ptr<Type> returnType() const { return returnType_; }

private:
    std::vector<std::shared_ptr<Type>> paramTypes_;
    std::shared_ptr<Type> returnType_;
};

class StructType : public Type {
public:
    struct Field {
        std::string name;
        std::shared_ptr<Type> type;

        Field(std::string name, std::shared_ptr<Type> type)
            : name(std::move(name)), type(std::move(type)) {}
    };

    StructType(std::string name, std::vector<Field> fields)
        : name_(std::move(name)), fields_(std::move(fields)) {}

    Kind kind() const override { return Kind::Struct; }

    std::string toString() const override {
        return name_;
    }

    bool equals(const Type* other) const override {
        if (auto* structType = dynamic_cast<const StructType*>(other)) {
            // Structural equality: same name and fields
            return name_ == structType->name_;
        }
        return false;
    }

    const std::string& name() const { return name_; }
    const std::vector<Field>& fields() const { return fields_; }

    // Lookup field by name
    const Field* getField(const std::string& fieldName) const {
        for (const auto& field : fields_) {
            if (field.name == fieldName) {
                return &field;
            }
        }
        return nullptr;
    }

private:
    std::string name_;
    std::vector<Field> fields_;
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

} // namespace volta::semantic
