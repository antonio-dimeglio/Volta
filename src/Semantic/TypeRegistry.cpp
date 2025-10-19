#include "../../include/Semantic/TypeRegistry.hpp"
#include "../../include/Parser/AST.hpp"
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <stdexcept>
#include <limits>
#include <iostream>

namespace Semantic {


TypeRegistry::TypeRegistry() {

    i8_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::I8);
    i16_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::I16);
    i32_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::I32);
    i64_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::I64);
    u8_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::U8);
    u16_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::U16);
    u32_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::U32);
    u64_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::U64);
    f32_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::F32);
    f64_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::F64);
    bool_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::Bool);
    char_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::Char);
    string_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::String);
    void_type = std::make_unique<PrimitiveType>(PrimitiveTypeKind::Void);


    type_widths[PrimitiveTypeKind::I8] = 8;
    type_widths[PrimitiveTypeKind::I16] = 16;
    type_widths[PrimitiveTypeKind::I32] = 32;
    type_widths[PrimitiveTypeKind::I64] = 64;
    type_widths[PrimitiveTypeKind::U8] = 8;
    type_widths[PrimitiveTypeKind::U16] = 16;
    type_widths[PrimitiveTypeKind::U32] = 32;
    type_widths[PrimitiveTypeKind::U64] = 64;
    type_widths[PrimitiveTypeKind::F32] = 32;
    type_widths[PrimitiveTypeKind::F64] = 64;
}


const PrimitiveType* TypeRegistry::getPrimitive(PrimitiveTypeKind kind) const {
    switch (kind) {
        case PrimitiveTypeKind::I8:     return i8_type.get();
        case PrimitiveTypeKind::I16:    return i16_type.get();
        case PrimitiveTypeKind::I32:    return i32_type.get();
        case PrimitiveTypeKind::I64:    return i64_type.get();
        case PrimitiveTypeKind::U8:     return u8_type.get();
        case PrimitiveTypeKind::U16:    return u16_type.get();
        case PrimitiveTypeKind::U32:    return u32_type.get();
        case PrimitiveTypeKind::U64:    return u64_type.get();
        case PrimitiveTypeKind::F32:    return f32_type.get();
        case PrimitiveTypeKind::F64:    return f64_type.get();
        case PrimitiveTypeKind::Bool:   return bool_type.get();
        case PrimitiveTypeKind::Char:   return char_type.get();
        case PrimitiveTypeKind::String: return string_type.get();
        case PrimitiveTypeKind::Void:   return void_type.get();
    }
    throw std::runtime_error("Invalid primitive type kind");
}

const ArrayType* TypeRegistry::internArray(const Type* element_type, int size) {
    std::unique_ptr<Type> owned_element;

    if (auto* prim = dynamic_cast<const PrimitiveType*>(element_type)) {
        owned_element = std::make_unique<PrimitiveType>(prim->kind);
    } else if (auto* arr = dynamic_cast<const ArrayType*>(element_type)) {
        const ArrayType* interned_inner = internArray(arr->element_type.get(), arr->size);
        owned_element = std::make_unique<ArrayType>(
            cloneType(interned_inner->element_type.get()),
            interned_inner->size
        );
    } else {
        throw std::runtime_error("Unsupported element type for array interning");
    }

    auto new_arr = std::make_unique<ArrayType>(std::move(owned_element), size);


    auto it = array_types.find(new_arr);
    if (it != array_types.end()) {
    
        return it->get();
    }


    const ArrayType* result = new_arr.get();
    array_types.insert(std::move(new_arr));
    return result;
}

const GenericType* TypeRegistry::internGeneric(const std::string& name,
                                               const std::vector<const Type*>& type_params) {

    std::vector<std::unique_ptr<Type>> cloned_params;
    for (const Type* param : type_params) {

        if (auto* prim = dynamic_cast<const PrimitiveType*>(param)) {
            cloned_params.push_back(std::make_unique<PrimitiveType>(prim->kind));
        } else if (dynamic_cast<const OpaqueType*>(param)) {
            cloned_params.push_back(std::make_unique<OpaqueType>());
        } else {
            throw std::runtime_error("Generic type parameters other than primitives and opaque not yet supported");
        }
    }


    auto new_gen = std::make_unique<GenericType>(name, std::move(cloned_params));


    auto it = generic_types.find(new_gen);
    if (it != generic_types.end()) {
    
        return it->get();
    }


    const GenericType* result = new_gen.get();
    generic_types.insert(std::move(new_gen));
    return result;
}

const Type* TypeRegistry::internFromAST(const Type* ast_type) {
    if (!ast_type) {
        return nullptr;
    }


    if (auto* prim = dynamic_cast<const PrimitiveType*>(ast_type)) {
        return getPrimitive(prim->kind);
    }


    if (auto* arr = dynamic_cast<const ArrayType*>(ast_type)) {
        const Type* interned_element = internFromAST(arr->element_type.get());
        return internArray(interned_element, arr->size);
    }


    if (auto* gen = dynamic_cast<const GenericType*>(ast_type)) {
        std::vector<const Type*> interned_params;
        for (const auto& param : gen->type_params) {
            interned_params.push_back(internFromAST(param.get()));
        }
        return internGeneric(gen->name, interned_params);
    }

    // Opaque type - we don't intern it, just use a singleton-like behavior
    // Since all opaque types are identical, we can return a static instance
    if (dynamic_cast<const OpaqueType*>(ast_type)) {
        // For now, treat opaque as void* - return the primitive i8 pointer equivalent
        // This is a placeholder - ideally we'd have a dedicated opaque type instance
        static std::unique_ptr<OpaqueType> opaque_singleton = std::make_unique<OpaqueType>();
        return opaque_singleton.get();
    }

    return nullptr;
}


bool TypeRegistry::isValidType(const Type* type) const {
    if (!type) {
        return false;
    }

    if (type == i8_type.get() || type == i16_type.get() || type == i32_type.get() ||
        type == i64_type.get() || type == u8_type.get() || type == u16_type.get() ||
        type == u32_type.get() || type == u64_type.get() || type == f32_type.get() ||
        type == f64_type.get() || type == bool_type.get() || type == char_type.get() ||
        type == string_type.get() || type == void_type.get()) {
        return true;
    }


    for (const auto& arr : array_types) {
        if (arr.get() == type) {
            return true;
        }
    }


    for (const auto& gen : generic_types) {
        if (gen.get() == type) {
            return true;
        }
    }

    // Check if it's the opaque type singleton
    if (dynamic_cast<const OpaqueType*>(type)) {
        return true;
    }

    return false;
}


bool TypeRegistry::areTypesCompatible(const Type* target, const Type* source) const {
    if (!target || !source) return false;


    if (target == source) return true;


    auto* targetPrimitive = dynamic_cast<const PrimitiveType*>(target);
    auto* sourcePrimitive = dynamic_cast<const PrimitiveType*>(source);
    auto* targetGeneric = dynamic_cast<const GenericType*>(target);
    auto* sourceGeneric = dynamic_cast<const GenericType*>(source);

    // Special case: str is compatible with Ptr<u8> for FFI
    if (targetGeneric && sourcePrimitive) {
        if (targetGeneric->name == "Ptr" &&
            targetGeneric->type_params.size() == 1 &&
            sourcePrimitive->kind == PrimitiveTypeKind::String) {
            auto* innerType = dynamic_cast<const PrimitiveType*>(targetGeneric->type_params[0].get());
            if (innerType && innerType->kind == PrimitiveTypeKind::U8) {
                return true;
            }
        }
    }

    if (targetGeneric && sourceGeneric) {
        return target->equals(source);
    }

    if ((targetGeneric || sourceGeneric) || (!targetPrimitive || !sourcePrimitive)) {
        return false;
    } 

    if (isInteger(source) && isInteger(target)) {
    
        if ((isUnsigned(source) && isUnsigned(target)) ||
            (isSigned(source) && isSigned(target))) {
            return getTypeWidth(targetPrimitive->kind) >= getTypeWidth(sourcePrimitive->kind);
        }
    }


    if (isFloat(source) && isFloat(target)) {
        return getTypeWidth(targetPrimitive->kind) >= getTypeWidth(sourcePrimitive->kind);
    }

    return false;
}

bool TypeRegistry::canCastExplicitly(const Type* from_type, const Type* to_type) const {


    (void)from_type;
    (void)to_type;
    return false;
}


const Type* TypeRegistry::inferBinaryOp(const Type* left_type, TokenType op,
                                        const Type* right_type) {
    if (!left_type || !right_type) return nullptr;


    if (op == TokenType::Plus || op == TokenType::Minus || op == TokenType::Mult ||
        op == TokenType::Div || op == TokenType::Modulo) {

    
        if (!isNumeric(left_type) || !isNumeric(right_type)) {
            return nullptr;
        }

    
        if (left_type != right_type) {
            return nullptr;
        }

    
        if (op == TokenType::Modulo && !isInteger(left_type)) {
            return nullptr;
        }

    
        return left_type;
    }


    if (op == TokenType::EqualEqual || op == TokenType::NotEqual ||
        op == TokenType::LessThan || op == TokenType::LessEqual ||
        op == TokenType::GreaterThan || op == TokenType::GreaterEqual) {

    
        if (op == TokenType::EqualEqual || op == TokenType::NotEqual) {
            if (left_type == right_type) {
                return bool_type.get();
            }
            return nullptr;
        }

    
        if (isNumeric(left_type) && isNumeric(right_type) && left_type == right_type) {
            return bool_type.get();
        }

        return nullptr;
    }


    if (op == TokenType::And || op == TokenType::Or) {
        if (left_type == bool_type.get() && right_type == bool_type.get()) {
            return bool_type.get();
        }
        return nullptr;
    }


    return nullptr;
}

const Type* TypeRegistry::inferUnaryOp(TokenType op, const Type* operand_type) {
    if (!operand_type) return nullptr;


    if (op == TokenType::Minus) {
        if (isSigned(operand_type) || isFloat(operand_type)) {
            return operand_type;
        }
        return nullptr;
    }


    if (op == TokenType::Not) {
        if (operand_type == bool_type.get()) {
            return operand_type;
        }
        return nullptr;
    }

    return nullptr;
}


const Type* TypeRegistry::inferLiteral(const Literal* literal) {
    if (!literal) return nullptr;

    TokenType tt = literal->token.tt;

    if (tt == TokenType::Integer) return i32_type.get();
    if (tt == TokenType::Float) return f32_type.get();
    if (tt == TokenType::True_ || tt == TokenType::False_) return bool_type.get();
    if (tt == TokenType::String) return string_type.get();

    // Unknown literal type
    return nullptr;
}

bool TypeRegistry::validateLiteralBounds(const Literal* literal, const Type* target_type) {
    if (!literal || !target_type) return false;

    TokenType tt = literal->token.tt;
    const std::string& lexeme = literal->token.lexeme;


    auto* prim = dynamic_cast<const PrimitiveType*>(target_type);



    if (tt == TokenType::Integer) {
        if (!isInteger(target_type)) return false;

        try {
            long long value = std::stoll(lexeme);

        
            switch (prim->kind) {
                case PrimitiveTypeKind::I8:
                    return value >= -128 && value <= 127;
                case PrimitiveTypeKind::I16:
                    return value >= -32768 && value <= 32767;
                case PrimitiveTypeKind::I32:
                    return value >= -2147483648LL && value <= 2147483647LL;
                case PrimitiveTypeKind::I64:
                
                case PrimitiveTypeKind::U8:
                    return value >= 0 && value <= 255;
                case PrimitiveTypeKind::U16:
                    return value >= 0 && value <= 65535;
                case PrimitiveTypeKind::U32:
                    return value >= 0 && value <= 4294967295LL;
                case PrimitiveTypeKind::U64:
                
                default:
                    return false;
            }
        } catch (...) {
            return false;
        }
    }


    if (tt == TokenType::Float) {
        if (!isFloat(target_type)) return false;

        try {
            double value = std::stod(lexeme);

        
            switch (prim->kind) {
                case PrimitiveTypeKind::F32:
                    return value >= -std::numeric_limits<float>::max() &&
                           value <= std::numeric_limits<float>::max();
                case PrimitiveTypeKind::F64:
                
                default:
                    return false;
            }
        } catch (...) {
            return false;
        }
    }


    if (tt == TokenType::True_ || tt == TokenType::False_) {
        return prim->kind == PrimitiveTypeKind::Bool;
    }


    if (tt == TokenType::String) {
        return prim->kind == PrimitiveTypeKind::String;
    }

    return false;
}


bool TypeRegistry::isInteger(const Type* type) const {
    auto* prim = dynamic_cast<const PrimitiveType*>(type);
    if (!prim) return false;

    return prim->kind == PrimitiveTypeKind::I8 || prim->kind == PrimitiveTypeKind::I16 ||
           prim->kind == PrimitiveTypeKind::I32 || prim->kind == PrimitiveTypeKind::I64 ||
           prim->kind == PrimitiveTypeKind::U8 || prim->kind == PrimitiveTypeKind::U16 ||
           prim->kind == PrimitiveTypeKind::U32 || prim->kind == PrimitiveTypeKind::U64;
}

bool TypeRegistry::isSigned(const Type* type) const {
    auto* prim = dynamic_cast<const PrimitiveType*>(type);
    if (!prim) return false;

    return prim->kind == PrimitiveTypeKind::I8 || prim->kind == PrimitiveTypeKind::I16 ||
           prim->kind == PrimitiveTypeKind::I32 || prim->kind == PrimitiveTypeKind::I64;
}

bool TypeRegistry::isUnsigned(const Type* type) const {
    auto* prim = dynamic_cast<const PrimitiveType*>(type);
    if (!prim) return false;

    return prim->kind == PrimitiveTypeKind::U8 || prim->kind == PrimitiveTypeKind::U16 ||
           prim->kind == PrimitiveTypeKind::U32 || prim->kind == PrimitiveTypeKind::U64;
}

bool TypeRegistry::isFloat(const Type* type) const {
    auto* prim = dynamic_cast<const PrimitiveType*>(type);
    if (!prim) return false;

    return prim->kind == PrimitiveTypeKind::F32 || prim->kind == PrimitiveTypeKind::F64;
}

bool TypeRegistry::isNumeric(const Type* type) const {
    return isInteger(type) || isFloat(type);
}

bool TypeRegistry::isArray(const Type* type) const {
    return dynamic_cast<const ArrayType*>(type) != nullptr;
}

bool TypeRegistry::isGeneric(const Type* type) const {
    return dynamic_cast<const GenericType*>(type) != nullptr;
}


const Type* TypeRegistry::getArrayElementType(const Type* type) const {
    auto* arr = dynamic_cast<const ArrayType*>(type);
    if (!arr) return nullptr;



    if (auto* prim = dynamic_cast<const PrimitiveType*>(arr->element_type.get())) {
        return getPrimitive(prim->kind);
    }


    return arr->element_type.get();
}

int TypeRegistry::getArraySize(const Type* type) const {
    auto* arr = dynamic_cast<const ArrayType*>(type);
    return arr ? arr->size : -1;
}


int TypeRegistry::getTypeWidth(PrimitiveTypeKind kind) const {
    auto it = type_widths.find(kind);
    return (it != type_widths.end()) ? it->second : 0;
}


llvm::Type* TypeRegistry::toLLVMType(const Type* type, llvm::LLVMContext& ctx) const {
    if (!type) return nullptr;


    if (auto* prim = dynamic_cast<const PrimitiveType*>(type)) {
        switch (prim->kind) {
            case PrimitiveTypeKind::I8:     return llvm::Type::getInt8Ty(ctx);
            case PrimitiveTypeKind::I16:    return llvm::Type::getInt16Ty(ctx);
            case PrimitiveTypeKind::I32:    return llvm::Type::getInt32Ty(ctx);
            case PrimitiveTypeKind::I64:    return llvm::Type::getInt64Ty(ctx);
            case PrimitiveTypeKind::U8:     return llvm::Type::getInt8Ty(ctx);
            case PrimitiveTypeKind::U16:    return llvm::Type::getInt16Ty(ctx);
            case PrimitiveTypeKind::U32:    return llvm::Type::getInt32Ty(ctx);
            case PrimitiveTypeKind::U64:    return llvm::Type::getInt64Ty(ctx);
            case PrimitiveTypeKind::F32:    return llvm::Type::getFloatTy(ctx);
            case PrimitiveTypeKind::F64:    return llvm::Type::getDoubleTy(ctx);
            case PrimitiveTypeKind::Bool:   return llvm::Type::getInt1Ty(ctx);
            case PrimitiveTypeKind::Char:   return llvm::Type::getInt8Ty(ctx);
            case PrimitiveTypeKind::String:
            
                return llvm::PointerType::get(ctx, 0);
            case PrimitiveTypeKind::Void:   return llvm::Type::getVoidTy(ctx);
        }
    }


    if (auto* arr = dynamic_cast<const ArrayType*>(type)) {
        llvm::Type* elementType = toLLVMType(arr->element_type.get(), ctx);
        if (!elementType) return nullptr;
        return llvm::ArrayType::get(elementType, arr->size);
    }


    if (auto* gen = dynamic_cast<const GenericType*>(type)) {
        if (gen->name == "Ptr") {
            return llvm::PointerType::get(ctx, 0); // Opaque LLVM ptr.
        }

        // TODO: Add support for actual generics.
        return nullptr;
    }

    // Handle opaque type (should only appear inside Ptr<opaque>)
    if (dynamic_cast<const OpaqueType*>(type)) {
        // If we get here, opaque was used standalone (which is an error)
        // But for error recovery, treat it as a pointer
        return llvm::PointerType::get(ctx, 0);
    }

    return nullptr;
}

std::unique_ptr<Type> TypeRegistry::cloneType(const Type* type) const {
    // Clone a type recursively (needed for nested array interning)

    if (auto* prim = dynamic_cast<const PrimitiveType*>(type)) {
        return std::make_unique<PrimitiveType>(prim->kind);
    }

    if (auto* arr = dynamic_cast<const ArrayType*>(type)) {
        return std::make_unique<ArrayType>(
            cloneType(arr->element_type.get()),
            arr->size
        );
    }

    if (auto* gen = dynamic_cast<const GenericType*>(type)) {
        std::vector<std::unique_ptr<Type>> cloned_params;
        for (const auto& param : gen->type_params) {
            cloned_params.push_back(cloneType(param.get()));
        }
        return std::make_unique<GenericType>(gen->name, std::move(cloned_params));
    }

    if (dynamic_cast<const OpaqueType*>(type)) {
        return std::make_unique<OpaqueType>();
    }

    throw std::runtime_error("Unsupported type for cloning: " + type->toString());
}

}  // namespace Semantic

