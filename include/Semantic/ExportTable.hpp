#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

// Forward declaration
struct Program;

namespace Semantic {
    
class ExportTable {
private:
    std::unordered_map<std::string, std::unordered_set<std::string>> modules;

public:
    ExportTable() = default;

    // Adds exported symbol to the export table. returns false if already present
    bool addExport(const std::string& moduleName, const std::string& symbolName);

    // Checks if table has already a matching symbol name, returns true if present
    bool hasExport(const std::string& moduleName, const std::string& symbolName) const;

    // returns all the export associated with a module
    const std::unordered_set<std::string> getExports(const std::string& moduleName) const;

    // Checks if a module exists in the export table
    bool moduleExists(const std::string& moduleName) const;

    // Collects all pub symbols from an AST and adds them to the export table
    void collectExportsFromAST(const Program& ast, const std::string& moduleName);

    // returns a string representing the export table internals.
    std::string dump() const;
};

} // namespace Semantic
