#include "Semantic/TypeSubstitution.hpp"
#include <stdexcept>

namespace Volta {

TypeSubstitution::TypeSubstitution(
    const std::vector<Token>& typeParams,
    const std::vector<const Type::Type*>& typeArgs
) {
    if (typeParams.size() != typeArgs.size()) {
        throw std::runtime_error("Type parameter count mismatch during substitution");
    }

    for (size_t i = 0; i < typeParams.size(); ++i) {
        substitutions[typeParams[i].lexeme] = typeArgs[i];
    }
}

const Type::Type* TypeSubstitution::substitute(const Type::Type* type) const {
    if (!type) {
        return nullptr;
    }

    // Check if this is an unresolved type that matches a type parameter
    if (auto* unresolved = dynamic_cast<const Type::UnresolvedType*>(type)) {
        auto it = substitutions.find(unresolved->name);
        if (it != substitutions.end()) {
            return it->second;  // Replace T with i32, etc.
        }
        return type;  // Not a type parameter, leave as-is
    }

    // For pointer types, substitute the pointee
    if (auto* ptrType = dynamic_cast<const Type::PointerType*>(type)) {
        // Note: This requires TypeRegistry access to create new pointer types
        // For now, we'll assume the type is already properly constructed
        return type;
    }

    // For array types, substitute the element type
    if (auto* arrType = dynamic_cast<const Type::ArrayType*>(type)) {
        // Similar to pointers - would need TypeRegistry
        return type;
    }

    // For generic types (e.g., Pair<T, U>), recursively substitute type arguments
    if (auto* genType = dynamic_cast<const Type::GenericType*>(type)) {
        std::vector<const Type::Type*> substitutedParams;
        bool changed = false;
        for (const auto* param : genType->typeParams) {
            const Type::Type* substitutedParam = substitute(param);
            substitutedParams.push_back(substitutedParam);
            if (substitutedParam != param) {
                changed = true;
            }
        }

        // If any type parameter was substituted, create a new GenericType
        if (changed) {
            return new Type::GenericType(genType->name, substitutedParams);
        }
        return type;
    }

    // Concrete types pass through unchanged
    return type;
}

std::unique_ptr<FnDecl> TypeSubstitution::substituteFnDecl(const FnDecl* fn) const {
    // Clone the function with type substitution
    auto newFn = std::make_unique<FnDecl>(
        fn->name,
        std::vector<Token>(),  // No type parameters in monomorphized version
        substituteParams(fn->params),
        substitute(fn->returnType),
        substituteStmtList(fn->body),
        fn->isExtern,
        fn->isPublic,
        fn->line,
        fn->column
    );

    return newFn;
}

std::unique_ptr<StructDecl> TypeSubstitution::substituteStructDecl(const StructDecl* structDecl) const {
    return std::make_unique<StructDecl>(
        structDecl->isPublic,
        structDecl->name,
        std::vector<Token>(),  // No type parameters in monomorphized version
        substituteFields(structDecl->fields),
        substituteMethods(structDecl->methods)
    );
}

std::vector<Param> TypeSubstitution::substituteParams(const std::vector<Param>& params) const {
    std::vector<Param> result;
    result.reserve(params.size());

    for (const auto& param : params) {
        result.emplace_back(
            param.name,
            substitute(param.type),
            param.isRef,
            param.isMutRef,
            param.isMutable
        );
    }

    return result;
}

std::vector<StructField> TypeSubstitution::substituteFields(const std::vector<StructField>& fields) const {
    std::vector<StructField> result;
    result.reserve(fields.size());

    for (const auto& field : fields) {
        result.emplace_back(
            field.isPublic,
            field.name,
            substitute(field.type)
        );
    }

    return result;
}

std::vector<std::unique_ptr<Stmt>> TypeSubstitution::substituteStmtList(
    const std::vector<std::unique_ptr<Stmt>>& stmts
) const {
    std::vector<std::unique_ptr<Stmt>> result;
    result.reserve(stmts.size());

    for (const auto& stmt : stmts) {
        result.push_back(substituteStmt(stmt.get()));
    }

    return result;
}

std::vector<std::unique_ptr<FnDecl>> TypeSubstitution::substituteMethods(
    const std::vector<std::unique_ptr<FnDecl>>& methods
) const {
    std::vector<std::unique_ptr<FnDecl>> result;
    result.reserve(methods.size());

    for (const auto& method : methods) {
        result.push_back(substituteFnDecl(method.get()));
    }

    return result;
}

// Statement substitution
std::unique_ptr<Stmt> TypeSubstitution::substituteStmt(const Stmt* stmt) const {
    if (auto* varDecl = dynamic_cast<const VarDecl*>(stmt)) {
        return std::make_unique<VarDecl>(
            varDecl->mutable_,
            varDecl->name,
            substitute(varDecl->typeAnnotation),
            varDecl->initValue ? substituteExpr(varDecl->initValue.get()) : nullptr
        );
    }

    if (auto* retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        return std::make_unique<ReturnStmt>(
            retStmt->value ? substituteExpr(retStmt->value.get()) : nullptr,
            retStmt->line,
            retStmt->column
        );
    }

    if (auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        return std::make_unique<IfStmt>(
            substituteExpr(ifStmt->condition.get()),
            substituteStmtList(ifStmt->thenBody),
            substituteStmtList(ifStmt->elseBody),
            ifStmt->line,
            ifStmt->column
        );
    }

    if (auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        return std::make_unique<WhileStmt>(
            substituteExpr(whileStmt->condition.get()),
            substituteStmtList(whileStmt->thenBody),
            whileStmt->line,
            whileStmt->column
        );
    }

    if (auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        return std::make_unique<ForStmt>(
            std::make_unique<Variable>(forStmt->var->token),
            std::unique_ptr<Range>(dynamic_cast<Range*>(substituteExpr(forStmt->range.get()).release())),
            substituteStmtList(forStmt->body),
            forStmt->line,
            forStmt->column
        );
    }

    if (auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        return std::make_unique<ExprStmt>(
            substituteExpr(exprStmt->expr.get()),
            exprStmt->line,
            exprStmt->column
        );
    }

    if (auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        return std::make_unique<BlockStmt>(
            substituteStmtList(blockStmt->statements),
            blockStmt->line,
            blockStmt->column
        );
    }

    if (dynamic_cast<const BreakStmt*>(stmt) != nullptr) {
        return std::make_unique<BreakStmt>();
    }

    if (dynamic_cast<const ContinueStmt*>(stmt) != nullptr) {
        return std::make_unique<ContinueStmt>();
    }

    // Unsupported statement type - this shouldn't happen
    throw std::runtime_error("Unsupported statement type in type substitution");
}

// Expression substitution
std::unique_ptr<Expr> TypeSubstitution::substituteExpr(const Expr* expr) const {
    if (auto* literal = dynamic_cast<const Literal*>(expr)) {
        return std::make_unique<Literal>(literal->token);
    }

    if (auto* var = dynamic_cast<const Variable*>(expr)) {
        return std::make_unique<Variable>(var->token, var->typeArgs);
    }

    if (auto* binExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        return std::make_unique<BinaryExpr>(
            substituteExpr(binExpr->lhs.get()),
            substituteExpr(binExpr->rhs.get()),
            binExpr->op,
            binExpr->line,
            binExpr->column
        );
    }

    if (auto* unExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        return std::make_unique<UnaryExpr>(
            substituteExpr(unExpr->operand.get()),
            unExpr->op,
            unExpr->line,
            unExpr->column
        );
    }

    if (auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        std::vector<std::unique_ptr<Expr>> newArgs;
        for (const auto& arg : fnCall->args) {
            newArgs.push_back(substituteExpr(arg.get()));
        }
        return std::make_unique<FnCall>(
            fnCall->name,
            fnCall->typeArgs,  // Keep type args as-is (they're concrete types)
            std::move(newArgs),
            fnCall->line,
            fnCall->column
        );
    }

    if (auto* assignment = dynamic_cast<const Assignment*>(expr)) {
        return std::make_unique<Assignment>(
            substituteExpr(assignment->lhs.get()),
            substituteExpr(assignment->value.get()),
            assignment->line,
            assignment->column
        );
    }

    if (auto* grouping = dynamic_cast<const GroupingExpr*>(expr)) {
        return std::make_unique<GroupingExpr>(
            substituteExpr(grouping->expr.get()),
            grouping->line,
            grouping->column
        );
    }

    if (auto* arrayLit = dynamic_cast<const ArrayLiteral*>(expr)) {
        if (arrayLit->repeat_value) {
            return std::make_unique<ArrayLiteral>(
                substituteExpr(arrayLit->repeat_value.get()),
                *arrayLit->repeat_count,
                arrayLit->line,
                arrayLit->column
            );
        } else {
            std::vector<std::unique_ptr<Expr>> newElements;
            for (const auto& elem : arrayLit->elements) {
                newElements.push_back(substituteExpr(elem.get()));
            }
            return std::make_unique<ArrayLiteral>(
                std::move(newElements),
                arrayLit->line,
                arrayLit->column
            );
        }
    }

    if (auto* indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        std::vector<std::unique_ptr<Expr>> newIndices;
        for (const auto& idx : indexExpr->index) {
            newIndices.push_back(substituteExpr(idx.get()));
        }
        return std::make_unique<IndexExpr>(
            substituteExpr(indexExpr->array.get()),
            std::move(newIndices),
            indexExpr->line,
            indexExpr->column
        );
    }

    if (auto* addrOf = dynamic_cast<const AddrOf*>(expr)) {
        return std::make_unique<AddrOf>(
            substituteExpr(addrOf->operand.get()),
            addrOf->line,
            addrOf->column
        );
    }

    if (auto* compAssign = dynamic_cast<const CompoundAssign*>(expr)) {
        return std::make_unique<CompoundAssign>(
            std::unique_ptr<Variable>(dynamic_cast<Variable*>(substituteExpr(compAssign->var.get()).release())),
            substituteExpr(compAssign->value.get()),
            compAssign->op,
            compAssign->line,
            compAssign->column
        );
    }

    if (auto* inc = dynamic_cast<const Increment*>(expr)) {
        return std::make_unique<Increment>(
            std::unique_ptr<Variable>(dynamic_cast<Variable*>(substituteExpr(inc->var.get()).release())),
            inc->line,
            inc->column
        );
    }

    if (auto* dec = dynamic_cast<const Decrement*>(expr)) {
        return std::make_unique<Decrement>(
            std::unique_ptr<Variable>(dynamic_cast<Variable*>(substituteExpr(dec->var.get()).release())),
            dec->line,
            dec->column
        );
    }

    if (auto* range = dynamic_cast<const Range*>(expr)) {
        return std::make_unique<Range>(
            substituteExpr(range->from.get()),
            substituteExpr(range->to.get()),
            range->inclusive,
            range->line,
            range->column
        );
    }

    if (auto* structLit = dynamic_cast<const StructLiteral*>(expr)) {
        std::vector<std::pair<Token, std::unique_ptr<Expr>>> newFields;
        for (const auto& [fieldName, fieldValue] : structLit->fields) {
            newFields.emplace_back(fieldName, substituteExpr(fieldValue.get()));
        }

        // Substitute type arguments
        std::vector<const Type::Type*> newTypeArgs;
        for (const auto* typeArg : structLit->typeArgs) {
            newTypeArgs.push_back(substitute(typeArg));
        }

        auto result = std::make_unique<StructLiteral>(
            structLit->structName,
            newTypeArgs,
            std::move(newFields),
            structLit->line,
            structLit->column
        );

        // Substitute the resolved type if it exists
        if (structLit->resolvedType) {
            result->resolvedType = substitute(structLit->resolvedType);
        }

        return result;
    }

    if (auto* fieldAccess = dynamic_cast<const FieldAccess*>(expr)) {
        return std::make_unique<FieldAccess>(
            substituteExpr(fieldAccess->object.get()),
            fieldAccess->fieldName,
            fieldAccess->line,
            fieldAccess->column
        );
    }

    if (auto* staticCall = dynamic_cast<const StaticMethodCall*>(expr)) {
        std::vector<std::unique_ptr<Expr>> newArgs;
        for (const auto& arg : staticCall->args) {
            newArgs.push_back(substituteExpr(arg.get()));
        }
        return std::make_unique<StaticMethodCall>(
            staticCall->typeName,
            staticCall->typeArgs,
            staticCall->methodName,
            std::move(newArgs),
            staticCall->line,
            staticCall->column
        );
    }

    if (auto* instanceCall = dynamic_cast<const InstanceMethodCall*>(expr)) {
        std::vector<std::unique_ptr<Expr>> newArgs;
        for (const auto& arg : instanceCall->args) {
            newArgs.push_back(substituteExpr(arg.get()));
        }
        return std::make_unique<InstanceMethodCall>(
            substituteExpr(instanceCall->object.get()),
            instanceCall->methodName,
            std::move(newArgs),
            instanceCall->line,
            instanceCall->column
        );
    }

    // Unsupported expression type
    throw std::runtime_error("Unsupported expression type in type substitution");
}

} // namespace Volta
