#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Parser/AST.hpp"
#include "Type/Type.hpp"

namespace Volta {

// Represents a generic function definition before monomorphization
struct GenericFunctionDef {
    std::string name;
    std::vector<Token> typeParams;
    FnDecl* astNode;

    GenericFunctionDef(std::string name, std::vector<Token> typeParams, FnDecl* ast)
        : name(std::move(name)), typeParams(std::move(typeParams)), astNode(ast) {}
};

// Represents a generic struct definition before monomorphization
struct GenericStructDef {
    std::string name;
    std::vector<Token> typeParams;  // [T, U, V]
    StructDecl* astNode;

    GenericStructDef(std::string name, std::vector<Token> typeParams, StructDecl* ast)
        : name(std::move(name)), typeParams(std::move(typeParams)), astNode(ast) {}
};

// Tracks monomorphized instances to avoid duplicates
struct MonomorphInstance {
    std::string genericName;
    std::vector<const Type::Type*> typeArgs;
    std::string monomorphName;

    MonomorphInstance(std::string generic, std::vector<const Type::Type*> args, std::string monomorph)
        : genericName(std::move(generic)), typeArgs(std::move(args)), monomorphName(std::move(monomorph)) {}
};

class GenericRegistry {
    std::map<std::string, GenericFunctionDef> genericFunctions;
    std::map<std::string, GenericStructDef> genericStructs;

    std::vector<MonomorphInstance> functionInstances;
    std::vector<MonomorphInstance> structInstances;

public:
    void registerGenericFunction(const std::string& name, std::vector<Token> typeParams, FnDecl* astNode);
    void registerGenericStruct(const std::string& name, std::vector<Token> typeParams, StructDecl* astNode);

    GenericFunctionDef* getGenericFunction(const std::string& name);
    GenericStructDef* getGenericStruct(const std::string& name);

    bool isGenericFunction(const std::string& name) const;
    bool isGenericStruct(const std::string& name) const;

    std::string getOrCreateMonomorphName(
        const std::string& genericName,
        const std::vector<const Type::Type*>& typeArgs,
        bool isFunction
    );

    bool hasMonomorphInstance(
        const std::string& genericName,
        const std::vector<const Type::Type*>& typeArgs,
        bool isFunction
    ) const;

    std::string findMonomorphName(
        const std::string& genericName,
        const std::vector<const Type::Type*>& typeArgs,
        bool isFunction
    ) const;

private:

    std::string generateMonomorphName(
        const std::string& baseName,
        const std::vector<const Type::Type*>& typeArgs
    ) const;

    // Check if two type argument lists match
    bool typeArgsMatch(
        const std::vector<const Type::Type*>& a,
        const std::vector<const Type::Type*>& b
    ) const;
};

} // namespace Volta
