#include "semantic/SymbolTable.hpp"
#include <cassert>

namespace volta::semantic {

SymbolTable::SymbolTable() {
    enterScope();
}

void SymbolTable::enterScope() {
    scopes_.push_back({});
}

void SymbolTable::exitScope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

bool SymbolTable::signaturesMatch(const Type* type1, const Type* type2) {
    // Both must be function types
    auto* fn1 = dynamic_cast<const FunctionType*>(type1);
    auto* fn2 = dynamic_cast<const FunctionType*>(type2);

    if (!fn1 || !fn2) {
        return false;
    }

    // Check if parameter counts match
    const auto& params1 = fn1->paramTypes();
    const auto& params2 = fn2->paramTypes();

    if (params1.size() != params2.size()) {
        return false;
    }

    // Check if each parameter type matches
    for (size_t i = 0; i < params1.size(); i++) {
        if (params1[i]->kind() != params2[i]->kind()) {
            return false;
        }
    }

    return true;
}

bool SymbolTable::declare(const std::string& name, Symbol symbol) {
    auto& currScope = scopes_.back();
    auto it = currScope.symbols.find(name);

    // Check if this symbol is a function
    bool isFunction = dynamic_cast<const FunctionType*>(symbol.type.get()) != nullptr;

    if (it != currScope.symbols.end()) {
        // Name already exists in this scope
        std::vector<Symbol>& existingSymbols = it->second;

        if (!isFunction) {
            // Variables/types cannot be overloaded
            return false;
        }

        // Check if this exact signature already exists
        for (const auto& existing : existingSymbols) {
            if (signaturesMatch(existing.type.get(), symbol.type.get())) {
                return false;  // Duplicate function signature
            }
        }

        // New overload - add to vector
        existingSymbols.push_back(std::move(symbol));
        return true;
    }

    // Name doesn't exist - create new entry with vector containing this symbol
    currScope.symbols.insert({name, std::vector<Symbol>{std::move(symbol)}});
    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto& currScope = it->symbols;
        auto jt = currScope.find(name);
        if (jt != currScope.end()) {
            // Found the name - return first symbol in the vector
            const std::vector<Symbol>& symbols = jt->second;
            if (!symbols.empty()) {
                return symbols[0];  // Return first overload (or only symbol for non-functions)
            }
        }
    }

    return std::nullopt;
}

std::optional<Symbol> SymbolTable::lookupInCurrentScope(const std::string& name) {
    auto& currScope = scopes_.back().symbols;
    auto it = currScope.find(name);
    if (it != currScope.end()) {
        const std::vector<Symbol>& symbols = it->second;
        if (!symbols.empty()) {
            return symbols[0];
        }
    }

    return std::nullopt;
}

std::vector<Symbol> SymbolTable::lookupAllOverloads(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto& currScope = it->symbols;
        auto jt = currScope.find(name);
        if (jt != currScope.end()) {
            return jt->second;  // Return all overloads
        }
    }

    return {};  // Empty vector if not found
}

std::vector<Symbol> SymbolTable::lookupAllOverloadsInCurrentScope(const std::string& name) {
    auto& currScope = scopes_.back().symbols;
    auto it = currScope.find(name);
    if (it != currScope.end()) {
        return it->second;  // Return all overloads in current scope
    }

    return {};  // Empty vector if not found
}

std::optional<Symbol> SymbolTable::lookupOverload(
    const std::string& name,
    const std::vector<std::shared_ptr<Type>>& argTypes) {

    auto overloads = lookupAllOverloads(name);

    // Try to find a matching overload
    for (const auto& candidate : overloads) {
        auto* fnType = dynamic_cast<const FunctionType*>(candidate.type.get());
        if (!fnType) continue;  // Skip non-functions

        const auto& paramTypes = fnType->paramTypes();

        // Check if argument count matches
        if (paramTypes.size() != argTypes.size()) {
            continue;
        }

        // Check if all argument types match
        bool matches = true;
        for (size_t i = 0; i < argTypes.size(); i++) {
            if (paramTypes[i]->kind() != argTypes[i]->kind()) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return candidate;
        }
    }

    return std::nullopt;  // No matching overload found
}

bool SymbolTable::isMutable(const std::string& name) {
    auto symbol = lookup(name);
    assert(symbol.has_value() && "Symbol must exist before checking mutability");
    return symbol->isMutable;
}

}
