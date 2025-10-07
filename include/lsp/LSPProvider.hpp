#pragma once

#include <string>
#include <optional>
#include <memory>
#include <vector>
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "semantic/SymbolTable.hpp"
#include "error/ErrorTypes.hpp"

namespace volta {
namespace lsp {

/**
 * Position in a source file (1-indexed lines, 0-indexed columns to match editors)
 */
struct Position {
    size_t line;    // 1-indexed (as shown in editor)
    size_t column;  // 0-indexed (character position)

    Position(size_t line, size_t column) : line(line), column(column) {}
};

/**
 * Symbol information for LSP hover/completion
 */
struct SymbolInfo {
    std::string kind;        // "function", "variable", "struct", "parameter", etc.
    std::string name;
    std::string type;        // Type as string (e.g., "int", "fn(int, int) -> int")
    std::string signature;   // For functions: full signature
    bool isMutable;          // For variables

    // Source location
    volta::errors::SourceLocation location;

    SymbolInfo() : isMutable(false) {}
};

/**
 * Definition location information
 */
struct DefinitionInfo {
    std::string name;
    std::string kind;
    volta::errors::SourceLocation location;
};

/**
 * Document symbol (for outline view)
 */
struct DocumentSymbol {
    std::string name;
    std::string kind;
    volta::errors::SourceLocation location;
    std::optional<std::string> signature;
    std::vector<DocumentSymbol> children; // For struct fields, etc.
};

/**
 * LSPProvider provides Language Server Protocol information
 * by querying the AST and semantic analyzer
 */
class LSPProvider {
public:
    LSPProvider(const volta::ast::Program* program,
                volta::semantic::SemanticAnalyzer* analyzer);

    // Get information about symbol at position (for hover)
    std::optional<SymbolInfo> getSymbolInfo(const std::string& filename,
                                            const Position& pos);

    // Get definition location (for go-to-definition)
    std::optional<DefinitionInfo> getDefinition(const std::string& filename,
                                                const Position& pos);

    // Get all symbols in document (for outline view)
    std::vector<DocumentSymbol> getDocumentSymbols(const std::string& filename);

private:
    const volta::ast::Program* program_;
    volta::semantic::SemanticAnalyzer* analyzer_;

    // Helper: Find statement at position
    const volta::ast::Statement* findStatementAtPosition(const Position& pos);

    // Helper: Find expression at position
    const volta::ast::Expression* findExpressionAtPosition(const Position& pos,
                                                           const volta::ast::Statement* stmt);

    // Helper: Recursively find deepest expression at position
    const volta::ast::Expression* findDeepestExpression(const Position& pos,
                                                        const volta::ast::Expression* expr);

    // Helper: Check if position is within a location range
    bool positionInRange(const Position& pos, const volta::errors::SourceLocation& loc);

    // Extract info from different node types
    SymbolInfo extractFunctionInfo(const volta::ast::FnDeclaration* func);
    SymbolInfo extractVariableInfo(const volta::ast::VarDeclaration* var);
    SymbolInfo extractStructInfo(const volta::ast::StructDeclaration* structDecl);
    SymbolInfo extractIdentifierInfo(const volta::ast::IdentifierExpression* ident);

    // Type to string conversion
    std::string typeToString(const semantic::Type* type);
    std::string typeToString(const ast::Type* type);

    // Collect symbols for document outline
    void collectSymbolsFromStatement(const volta::ast::Statement* stmt,
                                    std::vector<DocumentSymbol>& symbols);
};

/**
 * JSON output helpers
 */
void outputSuccessJSON(const SymbolInfo& info);
void outputDefinitionJSON(const DefinitionInfo& info);
void outputSymbolsJSON(const std::vector<DocumentSymbol>& symbols);
void outputErrorJSON(const std::string& code, const std::string& message);

}} // namespace volta::lsp
