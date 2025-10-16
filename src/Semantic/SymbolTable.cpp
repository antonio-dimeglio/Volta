#include "../../include/Semantic/SymbolTable.hpp"
#include "../../include/Semantic/TypeRegistry.hpp"
#include <stdexcept>
#include <iostream>

namespace Semantic {


SymbolTable::SymbolTable(const TypeRegistry& type_registry)
    : type_registry(type_registry) {
    scopes.push_back(Scope());
}


void SymbolTable::enterScope() {
    scopes.push_back(Scope());
}

void SymbolTable::exitScope() {
    if (scopes.size() <= 1) {
        throw std::runtime_error("Attempt to pop global scope");
    }
    scopes.pop_back();
}

Scope& SymbolTable::currentScope() {
    return scopes.back();
}

const Scope& SymbolTable::currentScope() const {
    return scopes.back();
}

bool SymbolTable::isGlobalScope() const {
    return scopes.size() == 1;
}

bool SymbolTable::define(const std::string& name, const Type* type, bool is_mut) {
    if (existsInCurrentScope(name)) {
        for (const auto& [varName, symbol] : currentScope()) {
        }
        return false;
    }

    if (!type_registry.isValidType(type)) {
        return false;
    }

    currentScope().emplace(name, Symbol(name, type, is_mut));
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool SymbolTable::existsInCurrentScope(const std::string& name) const {
    return currentScope().find(name) != currentScope().end();
}

bool SymbolTable::exists(const std::string& name) const {
    return lookup(name) != nullptr;
}





void SymbolTable::enterFunction() {
    enterScope();
}

void SymbolTable::exitFunction() {
    exitScope();
}

bool SymbolTable::addFunction(const std::string& name, const FunctionSignature& signature) {
    
    if (functionExists(name)) {
        return false;
    }

    
    for (const auto& param : signature.parameters) {
        if (!type_registry.isValidType(param.type)) {
            return false;
        }
    }

    
    if (!type_registry.isValidType(signature.return_type)) {
        return false;
    }

    
    functions.emplace(name, signature);
    return true;
}

const FunctionSignature* SymbolTable::lookupFunction(const std::string& name) const {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return &it->second;
    }
    return nullptr;
}

bool SymbolTable::functionExists(const std::string& name) const {
    return functions.find(name) != functions.end();
}





size_t SymbolTable::scopeDepth() const {
    return scopes.size();
}

}  // namespace Semantic


