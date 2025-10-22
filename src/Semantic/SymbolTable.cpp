#include "../../include/Semantic/SymbolTable.hpp"
#include <ranges>
#include <stdexcept>
#include <iostream>

namespace Semantic {


SymbolTable::SymbolTable() {
    scopes.emplace_back();
}


void SymbolTable::enterScope() {
    scopes.emplace_back();
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

bool SymbolTable::define(const std::string& name, const Type::Type* type, bool is_mut) {
    if (existsInCurrentScope(name)) {
        return false;
    }

    // Type validation is done by parser/type-checker, no need to validate here
    currentScope().emplace(name, Symbol(name, type, is_mut));
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    
    for (const auto & scope : std::ranges::reverse_view(scopes)) {
        auto found = scope.find(name);
        if (found != scope.end()) {
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

    // Type validation is done by parser/type-checker, no need to validate here
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


