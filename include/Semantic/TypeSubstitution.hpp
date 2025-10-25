#pragma once

#include <string>
#include <map>
#include <memory>
#include "Parser/AST.hpp"
#include "Type/Type.hpp"

namespace Volta {

// Handles substitution of type parameters with concrete types during monomorphization
// Example: T -> i32, U -> f64
class TypeSubstitution {
    std::map<std::string, const Type::Type*> substitutions;

public:
    // Initialize with type parameter mappings
    // typeParams: [T, U], typeArgs: [i32, f64] -> {T: i32, U: f64}
    TypeSubstitution(
        const std::vector<Token>& typeParams,
        const std::vector<const Type::Type*>& typeArgs
    );

    // Substitute type parameters in a type
    const Type::Type* substitute(const Type::Type* type) const;

    // Clone and substitute AST nodes (deep copy with type substitution)
    std::unique_ptr<Expr> substituteExpr(const Expr* expr) const;
    std::unique_ptr<Stmt> substituteStmt(const Stmt* stmt) const;

    // Clone entire function/struct with substitution
    std::unique_ptr<FnDecl> substituteFnDecl(const FnDecl* fn) const;
    std::unique_ptr<StructDecl> substituteStructDecl(const StructDecl* structDecl) const;

private:
    // Helper: substitute in parameter list
    std::vector<Param> substituteParams(const std::vector<Param>& params) const;

    // Helper: substitute in struct fields
    std::vector<StructField> substituteFields(const std::vector<StructField>& fields) const;

    // Helper: substitute in statement list
    std::vector<std::unique_ptr<Stmt>> substituteStmtList(
        const std::vector<std::unique_ptr<Stmt>>& stmts
    ) const;

    // Helper: substitute in method list
    std::vector<std::unique_ptr<FnDecl>> substituteMethods(
        const std::vector<std::unique_ptr<FnDecl>>& methods
    ) const;
};

} // namespace Volta
