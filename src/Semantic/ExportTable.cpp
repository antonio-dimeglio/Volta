#include "Semantic/ExportTable.hpp"
#include "Semantic/ModuleUtils.hpp"
#include "Parser/AST.hpp"
#include <sstream>

namespace Semantic {

bool ExportTable::addExport(const std::string& moduleName, const std::string& symbolName) {
    auto& exports = modules[moduleName];
    if (exports.contains(symbolName)) {
        return false;
    }
    exports.insert(std::move(symbolName));
    return true;
}

bool ExportTable::hasExport(const std::string& moduleName, const std::string& symbolName) const {
    auto it = modules.find(moduleName);
    if (it == modules.end()) return false;
    return it->second.contains(symbolName);
}

const std::unordered_set<std::string> ExportTable::getExports(const std::string& moduleName) const {
    static const std::unordered_set<std::string> empty;
    auto it = modules.find(moduleName);
    return (it != modules.end()) ? it->second : empty;
}

std::string ExportTable::dump() const {
    std::ostringstream oss;
    oss << "ExportTable {\n";
    for (const auto& [module, symbols] : modules) {
        oss << "  \"" << module << "\": {";
        bool first = true;
        for (const auto& sym : symbols) {
            if (!first) oss << ", ";
            first = false;
            oss << sym;
        }
        oss << "}\n";
    }
    oss << "}";
    return oss.str();
}

bool ExportTable::moduleExists(const std::string& moduleName) const {
    return modules.find(moduleName) != modules.end();
}

void ExportTable::collectExportsFromAST(const Program& ast, const std::string& moduleName) {
    for (const auto& stmt : ast.statements) {
        // Check if it's a function declaration
        if (auto* fnDecl = dynamic_cast<const FnDecl*>(stmt.get())) {
            if (fnDecl->isPublic) {
                addExport(moduleName, fnDecl->name);
            }
        }

        // TODO: Add support for pub struct, pub enum when implemented
    }
}

}  // namespace Semantic