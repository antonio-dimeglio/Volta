#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include "error/ErrorReporter.hpp"
#include "error/ErrorTypes.hpp"
#include "Type.hpp"

namespace volta::semantic {

// Symbol information stored in the symbol table
struct Symbol {
    std::string name;
    std::shared_ptr<Type> type;
    bool isMutable;
    volta::errors::SourceLocation location;

    Symbol(std::string name,
           std::shared_ptr<Type> type,
           bool isMutable,
           volta::errors::SourceLocation location)
        : name(std::move(name)),
          type(std::move(type)),
          isMutable(isMutable),
          location(location) {}
};

/**
 * SymbolTable manages nested scopes and symbol lookup.
 * Uses a stack of scopes (global → function → block).
 */
class SymbolTable {
public:
    SymbolTable();

    // Scope management
    void enterScope();
    void exitScope();
    int scopeDepth() const { return scopes_.size(); }

    // Symbol operations
    bool declare(const std::string& name, Symbol symbol);
    std::optional<Symbol> lookup(const std::string& name);
    std::optional<Symbol> lookupInCurrentScope(const std::string& name);

    // Overload resolution
    std::vector<Symbol> lookupAllOverloads(const std::string& name);
    std::vector<Symbol> lookupAllOverloadsInCurrentScope(const std::string& name);
    std::optional<Symbol> lookupOverload(const std::string& name,
                                         const std::vector<std::shared_ptr<Type>>& argTypes);

    // Check if variable is mutable (for assignment checking)
    bool isMutable(const std::string& name);

private:
    struct Scope {
        std::unordered_map<std::string, std::vector<Symbol>> symbols;
    };
    bool signaturesMatch(const Type* t1, const Type* t2);

    std::vector<Scope> scopes_;
};

} // namespace volta::semantic