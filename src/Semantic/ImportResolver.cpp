#include "Semantic/ImportResolver.hpp"
#include "Semantic/ExportTable.hpp"
#include "Parser/AST.hpp"
#include "Error/Error.hpp"
#include <sstream>
#include <algorithm>

namespace Semantic {

// Calculate Levenshtein distance between two strings
static size_t levenshteinDistance(const std::string& s1, const std::string& s2) {
    const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<size_t>> d(len1 + 1, std::vector<size_t>(len2 + 1));

    for (size_t i = 0; i <= len1; ++i) d[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) d[0][j] = j;

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({
                d[i - 1][j] + 1,      // deletion
                d[i][j - 1] + 1,      // insertion
                d[i - 1][j - 1] + cost // substitution
            });
        }
    }

    return d[len1][len2];
}

// Find the closest matching export symbol using Levenshtein distance
static std::string findClosestMatch(const std::string& symbol, const std::unordered_set<std::string>& exports) {
    if (exports.empty()) return "";

    std::string closest;
    size_t minDistance = SIZE_MAX;

    for (const auto& exportSym : exports) {
        size_t distance = levenshteinDistance(symbol, exportSym);
        if (distance < minDistance) {
            minDistance = distance;
            closest = exportSym;
        }
    }

    // Only suggest if the distance is reasonable (less than half the symbol length)
    if (minDistance <= symbol.length() / 2 + 1) {
        return closest;
    }

    return "";
}

std::string join(const std::unordered_set<std::string>& items, const std::string& delimiter) {
    std::ostringstream oss;
    bool first = true;
    for (const auto& item : items) {
        if (!first) oss << delimiter;
        first = false;
        oss << item;
    }
    return oss.str();
}

bool validateImports(
    const std::vector<std::pair<std::string, const Program*>>& units,
    const ExportTable& exportTable,
    DiagnosticManager& diag
) {
    bool hasErrors = false ;

    for (auto&[source, ast] : units) {
        for (auto& stmt : ast->statements) {
            if (auto* importStmt = dynamic_cast<const ImportStmt*>(stmt.get())) {
                if (!exportTable.moduleExists(importStmt->modulePath)){
                    diag.error("Module '" + importStmt->modulePath + "' not found. Did you forget to include it in the compilation?", importStmt->line, importStmt->column);
                    hasErrors = true;
                    continue;
                }

                for (auto const& symbol : importStmt->importedItems) {
                    if (!exportTable.hasExport(importStmt->modulePath, symbol)) {
                        auto availableExports = exportTable.getExports(importStmt->modulePath);

                        std::string errorMsg = "Symbol '" + symbol + "' is not exported by module '" + importStmt->modulePath + "'";

                        if (!availableExports.empty()) {
                            // Try to find a close match using Levenshtein distance
                            std::string suggestion = findClosestMatch(symbol, availableExports);

                            if (!suggestion.empty()) {
                                errorMsg += ". Did you mean '" + suggestion + "'?";
                            }

                            std::string exportList = join(availableExports, ", ");
                            errorMsg += " Available exports: {" + exportList + "}";
                        } else {
                            errorMsg += ". Module has no public exports";
                        }

                        diag.error(errorMsg, importStmt->line, importStmt->column);
                        hasErrors = true;
                    }
                }
            }
        }
    }


    return !hasErrors;
}

ImportMap buildImportMap(const std::vector<std::pair<std::string, const Program*>>& units) {
    ImportMap importMap;

    for (const auto& [moduleName, ast] : units) {
        // Walk through AST to find import statements
        for (const auto& stmt : ast->statements) {
            if (auto* importStmt = dynamic_cast<const ImportStmt*>(stmt.get())) {
                // Record each imported symbol
                for (const auto& symbol : importStmt->importedItems) {
                    importMap[moduleName][symbol] = importStmt->modulePath;
                }
            }
        }
    }

    return importMap;
}

// Helper function for DFS-based cycle detection
static bool detectCyclesDFS(
    const std::string& module,
    const ImportMap& importMap,
    std::unordered_set<std::string>& visiting,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& path,
    DiagnosticManager& diag
) {
    // If already fully visited, no cycle from this node
    if (visited.count(module) > 0) {
        return false;
    }

    // If currently visiting, we found a cycle
    if (visiting.count(module) > 0) {
        // Find where the cycle starts in the path
        auto cycleStart = std::find(path.begin(), path.end(), module);

        std::string cycleDesc = "Circular dependency detected: ";
        bool first = true;
        for (auto it = cycleStart; it != path.end(); ++it) {
            if (!first) cycleDesc += " → ";
            cycleDesc += *it;
            first = false;
        }
        cycleDesc += " → " + module;

        diag.error(cycleDesc, 0, 0);
        return true;
    }

    // Mark as visiting and add to path
    visiting.insert(module);
    path.push_back(module);

    // Check all modules this module imports from
    if (importMap.count(module) > 0) {
        const auto& imports = importMap.at(module);

        // Build set of unique modules we import from
        std::unordered_set<std::string> importedModules;
        for (const auto& [symbol, sourceModule] : imports) {
            importedModules.insert(sourceModule);
        }

        // Visit each imported module
        for (const auto& importedModule : importedModules) {
            if (detectCyclesDFS(importedModule, importMap, visiting, visited, path, diag)) {
                return true;
            }
        }
    }

    // Done visiting this module
    visiting.erase(module);
    path.pop_back();
    visited.insert(module);

    return false;
}

bool detectCircularDependencies(
    const ImportMap& importMap,
    DiagnosticManager& diag
) {
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    std::vector<std::string> path;

    // Check each module in the import map
    for (const auto& [module, imports] : importMap) {
        if (visited.count(module) == 0) {
            if (detectCyclesDFS(module, importMap, visiting, visited, path, diag)) {
                return true;  // Cycle found
            }
        }
    }

    return false;  // No cycles found
}

} // namespace Semantic