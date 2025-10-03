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

bool SymbolTable::declare(const std::string& name, Symbol symbol) {
    auto& currScope = scopes_.back();  
    auto it = currScope.symbols.find(name);

    if (it != currScope.symbols.end()) {
        return false;
    }

    currScope.symbols.insert({name, std::move(symbol)});

    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto& currScope = it->symbols;
        auto jt = currScope.find(name);
        if (jt != currScope.end()) {
            return jt->second;
        }
    }

    return std::nullopt;
}

std::optional<Symbol> SymbolTable::lookupInCurrentScope(const std::string& name) {
    auto& currScope = scopes_.back().symbols;
    auto it = currScope.find(name);
    if (it != currScope.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool SymbolTable::isMutable(const std::string& name) {
    auto symbol = lookup(name);
    assert(symbol.has_value() && "Symbol must exist before checking mutability");
    return symbol->isMutable;
}

}
