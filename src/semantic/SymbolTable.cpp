#include "semantic/SymbolTable.hpp"

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
    auto& currScope = scopes_.back();  // Must be reference!
    auto it = currScope.symbols.find(name);

    if (it != currScope.symbols.end()) {
        return false;
    }

    currScope.symbols.insert({name, std::move(symbol)});

    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto& currScope = it->symbols;  // Reference!
        auto jt = currScope.find(name);
        if (jt != currScope.end()) {
            return &(jt->second);
        }
    }

    return nullptr;
}

Symbol* SymbolTable::lookupInCurrentScope(const std::string& name) {
    auto& currScope = scopes_.back().symbols;  // Reference!
    auto it = currScope.find(name);
    if (it != currScope.end()) {
        return &(it->second);
    }

    return nullptr;
}

bool SymbolTable::isMutable(const std::string& name) {
    // TODO: Lookup the symbol and return its isMutable flag
    // Return false if not found
    auto symbol = lookup(name);

    if (symbol) {
        return symbol->isMutable;
    } else {
        return false;
    }
}

}
