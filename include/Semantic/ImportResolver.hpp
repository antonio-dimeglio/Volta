#pragma once
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
struct Program;
class DiagnosticManager;

namespace Semantic {

class ExportTable;

// Maps: module name → (symbol name → source module)
// Example: "main" → {"add" → "examples.math"}
// Meaning: module "main" imports symbol "add" from "examples.math"
using ImportMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

// Validates all import statements in the given compilation units
// Returns true if all imports are valid, false if there are errors
// Errors are reported through the DiagnosticManager
bool validateImports(
    const std::vector<std::pair<std::string, const Program*>>& units,
    const ExportTable& exportTable,
    DiagnosticManager& diag
);

// Builds a map of all imports across compilation units
// Returns ImportMap that tells which symbols each module imports from where
ImportMap buildImportMap(
    const std::vector<std::pair<std::string, const Program*>>& units
);

// Detects circular dependencies in the import graph
// Returns true if cycles are found, false otherwise
// Errors are reported through the DiagnosticManager
bool detectCircularDependencies(
    const ImportMap& importMap,
    DiagnosticManager& diag
);

}  // namespace Semantic
