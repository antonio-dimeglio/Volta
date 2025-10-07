#include "lsp/LSPProvider.hpp"
#include <iostream>
#include <sstream>

namespace volta {
namespace lsp {

LSPProvider::LSPProvider(const volta::ast::Program* program,
                         volta::semantic::SemanticAnalyzer* analyzer)
    : program_(program), analyzer_(analyzer) {}

bool LSPProvider::positionInRange(const Position& pos, 
                                  const volta::errors::SourceLocation& loc) {
    if (pos.line != loc.line) return false;
    return pos.column >= loc.column && pos.column < (loc.column + loc.length);
}

const volta::ast::Statement* LSPProvider::findStatementAtPosition(const Position& pos) {
    if (!program_) return nullptr;
    for (const auto& stmt : program_->statements) {
        // Check if position is on the same line as the statement
        if (stmt->location.line == pos.line) {
            return stmt.get();
        }
    }
    return nullptr;
}

const volta::ast::Expression* LSPProvider::findExpressionAtPosition(
    const Position& pos, const volta::ast::Statement* stmt) {
    // Check ExpressionStatement
    if (auto* exprStmt = dynamic_cast<const volta::ast::ExpressionStatement*>(stmt)) {
        return findDeepestExpression(pos, exprStmt->expr.get());
    }

    // Check VarDeclaration initializer
    if (auto* varDecl = dynamic_cast<const volta::ast::VarDeclaration*>(stmt)) {
        if (varDecl->initializer) {
            return findDeepestExpression(pos, varDecl->initializer.get());
        }
    }

    return nullptr;
}

const volta::ast::Expression* LSPProvider::findDeepestExpression(
    const Position& pos, const volta::ast::Expression* expr) {
    if (!expr) return nullptr;

    // Check if position is on the same line
    if (expr->location.line != pos.line) {
        return nullptr;
    }

    // Try to find a deeper match in sub-expressions first

    // CallExpression: check callee and arguments
    if (auto* call = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        // Check callee first (function name)
        if (auto* deeper = findDeepestExpression(pos, call->callee.get())) {
            return deeper;
        }
        // Check arguments
        for (const auto& arg : call->arguments) {
            if (auto* deeper = findDeepestExpression(pos, arg.get())) {
                return deeper;
            }
        }
        // If position is within this call expression's range, return it
        if (positionInRange(pos, call->location)) {
            return call;
        }
    }

    // BinaryExpression: check left and right
    if (auto* binary = dynamic_cast<const volta::ast::BinaryExpression*>(expr)) {
        if (auto* deeper = findDeepestExpression(pos, binary->left.get())) {
            return deeper;
        }
        if (auto* deeper = findDeepestExpression(pos, binary->right.get())) {
            return deeper;
        }
        if (positionInRange(pos, binary->location)) {
            return binary;
        }
    }

    // UnaryExpression: check operand
    if (auto* unary = dynamic_cast<const volta::ast::UnaryExpression*>(expr)) {
        if (auto* deeper = findDeepestExpression(pos, unary->operand.get())) {
            return deeper;
        }
        if (positionInRange(pos, unary->location)) {
            return unary;
        }
    }

    // For leaf nodes (like IdentifierExpression, literals), check if position is in range
    if (positionInRange(pos, expr->location)) {
        return expr;
    }

    // If we're on the same line and after the start of this expression,
    // return it as a fallback (for when location.length is not accurate)
    if (expr->location.line == pos.line && pos.column >= expr->location.column) {
        return expr;
    }

    // Position not in this expression
    return nullptr;
}

std::string LSPProvider::typeToString(const semantic::Type* type) {
    if (!type) return "unknown";
    return type->toString();
}

std::string LSPProvider::typeToString(const ast::Type* type) {
    if (!type) return "unknown";
    return "type";
}

SymbolInfo LSPProvider::extractFunctionInfo(const volta::ast::FnDeclaration* func) {
    SymbolInfo info;
    info.kind = "function";
    info.name = func->identifier;
    info.location = func->location;
    info.isMutable = false;

    // Build parameter list string
    std::ostringstream paramStr;
    for (size_t i = 0; i < func->parameters.size(); i++) {
        const auto& param = func->parameters[i];

        // Convert AST type to string
        std::string typeStr;
        if (auto* primType = dynamic_cast<const volta::ast::PrimitiveType*>(param->type.get())) {
            switch (primType->kind) {
                case volta::ast::PrimitiveType::Kind::Int: typeStr = "int"; break;
                case volta::ast::PrimitiveType::Kind::Float: typeStr = "float"; break;
                case volta::ast::PrimitiveType::Kind::Bool: typeStr = "bool"; break;
                case volta::ast::PrimitiveType::Kind::Str: typeStr = "str"; break;
                case volta::ast::PrimitiveType::Kind::Void: typeStr = "void"; break;
            }
        } else {
            typeStr = "type";
        }

        paramStr << param->identifier << ": " << typeStr;
        if (i < func->parameters.size() - 1) paramStr << ", ";
    }

    // Build return type string
    std::string returnTypeStr;
    if (auto* primType = dynamic_cast<const volta::ast::PrimitiveType*>(func->returnType.get())) {
        switch (primType->kind) {
            case volta::ast::PrimitiveType::Kind::Int: returnTypeStr = "int"; break;
            case volta::ast::PrimitiveType::Kind::Float: returnTypeStr = "float"; break;
            case volta::ast::PrimitiveType::Kind::Bool: returnTypeStr = "bool"; break;
            case volta::ast::PrimitiveType::Kind::Str: returnTypeStr = "str"; break;
            case volta::ast::PrimitiveType::Kind::Void: returnTypeStr = "void"; break;
        }
    } else {
        returnTypeStr = "type";
    }

    // Build full signature
    info.signature = "fn " + func->identifier + "(" + paramStr.str() + ") -> " + returnTypeStr;
    info.type = "fn(" + paramStr.str() + ") -> " + returnTypeStr;

    return info;
}

SymbolInfo LSPProvider::extractVariableInfo(const volta::ast::VarDeclaration* var) {
    SymbolInfo info;
    info.kind = "variable";
    info.name = var->identifier;
    info.location = var->location;
    info.isMutable = false;
    
    // Try to get type from the initializer expression
    if (var->initializer) {
        auto type = analyzer_->getExpressionType(var->initializer.get());
        if (type) {
            info.type = typeToString(type.get());
            info.signature = var->identifier + ": " + info.type;
        }
    }
    
    return info;
}

SymbolInfo LSPProvider::extractStructInfo(const volta::ast::StructDeclaration* structDecl) {
    SymbolInfo info;
    info.kind = "struct";
    info.name = structDecl->identifier;
    info.location = structDecl->location;
    info.type = structDecl->identifier;
    info.signature = "struct " + structDecl->identifier;
    info.isMutable = false;
    return info;
}

SymbolInfo LSPProvider::extractIdentifierInfo(const volta::ast::IdentifierExpression* ident) {
    SymbolInfo info;
    info.kind = "identifier";
    info.name = ident->name;
    info.location = ident->location;
    
    // Get type from expression type map
    auto type = analyzer_->getExpressionType(ident);
    if (type) {
        info.type = typeToString(type.get());
        info.signature = ident->name + ": " + info.type;
    }
    
    return info;
}

std::optional<SymbolInfo> LSPProvider::getSymbolInfo(const std::string& filename,
                                                      const Position& pos) {
    const volta::ast::Statement* stmt = findStatementAtPosition(pos);
    if (!stmt) return std::nullopt;

    // First, try to find expressions within the statement
    // This handles function calls, identifiers in expressions, etc.
    const volta::ast::Expression* expr = findExpressionAtPosition(pos, stmt);
    if (expr) {
        // Handle identifier expressions - look up function by name
        if (auto* ident = dynamic_cast<const volta::ast::IdentifierExpression*>(expr)) {
            std::string identName = ident->name;

            // Search all statements for function with this name
            for (const auto& s : program_->statements) {
                if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(s.get())) {
                    if (fnDecl->identifier == identName) {
                        return extractFunctionInfo(fnDecl);
                    }
                }
            }

            // Not a function, fall back to identifier info
            return extractIdentifierInfo(ident);
        }

        // Handle call expressions - extract callee and look it up
        if (auto* call = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
            if (auto* callee = dynamic_cast<const volta::ast::IdentifierExpression*>(call->callee.get())) {
                std::string funcName = callee->name;

                // Search all statements for function with this name
                for (const auto& s : program_->statements) {
                    if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(s.get())) {
                        if (fnDecl->identifier == funcName) {
                            return extractFunctionInfo(fnDecl);
                        }
                    }
                }
            }
        }
    }

    // If no expression matched, check if we're hovering on the statement itself
    if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt)) {
        return extractFunctionInfo(fnDecl);
    }
    else if (auto* structDecl = dynamic_cast<const volta::ast::StructDeclaration*>(stmt)) {
        return extractStructInfo(structDecl);
    }
    // Don't fall back to VarDeclaration - expressions should be found within the initializer

    return std::nullopt;
}

std::optional<DefinitionInfo> LSPProvider::getDefinition(const std::string& filename,
                                                         const Position& pos) {
    auto info = getSymbolInfo(filename, pos);
    if (!info) return std::nullopt;
    
    DefinitionInfo defInfo;
    defInfo.name = info->name;
    defInfo.kind = info->kind;
    defInfo.location = info->location;
    return defInfo;
}

std::vector<DocumentSymbol> LSPProvider::getDocumentSymbols(const std::string& filename) {
    std::vector<DocumentSymbol> symbols;
    if (!program_) return symbols;
    
    for (const auto& stmt : program_->statements) {
        collectSymbolsFromStatement(stmt.get(), symbols);
    }
    return symbols;
}

void LSPProvider::collectSymbolsFromStatement(const volta::ast::Statement* stmt,
                                             std::vector<DocumentSymbol>& symbols) {
    if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt)) {
        DocumentSymbol sym;
        sym.name = fnDecl->identifier;
        sym.kind = "function";
        sym.location = fnDecl->location;
        sym.signature = "fn " + fnDecl->identifier;
        symbols.push_back(sym);
    }
    else if (auto* structDecl = dynamic_cast<const volta::ast::StructDeclaration*>(stmt)) {
        DocumentSymbol sym;
        sym.name = structDecl->identifier;
        sym.kind = "struct";
        sym.location = structDecl->location;
        sym.signature = "struct " + structDecl->identifier;
        symbols.push_back(sym);
    }
}

void outputSuccessJSON(const SymbolInfo& info) {
    std::cout << "{\n";
    std::cout << "  \"success\": true,\n";
    std::cout << "  \"result\": {\n";
    std::cout << "    \"kind\": \"" << info.kind << "\",\n";
    std::cout << "    \"name\": \"" << info.name << "\",\n";
    std::cout << "    \"type\": \"" << info.type << "\",\n";
    std::cout << "    \"signature\": \"" << info.signature << "\"\n";
    std::cout << "  },\n";
    std::cout << "  \"error\": null\n";
    std::cout << "}\n";
}

void outputDefinitionJSON(const DefinitionInfo& info) {
    std::cout << "{\n";
    std::cout << "  \"success\": true,\n";
    std::cout << "  \"result\": {\n";
    std::cout << "    \"name\": \"" << info.name << "\",\n";
    std::cout << "    \"kind\": \"" << info.kind << "\"\n";
    std::cout << "  },\n";
    std::cout << "  \"error\": null\n";
    std::cout << "}\n";
}

void outputSymbolsJSON(const std::vector<DocumentSymbol>& symbols) {
    std::cout << "{\n";
    std::cout << "  \"success\": true,\n";
    std::cout << "  \"result\": {\n";
    std::cout << "    \"symbols\": [\n";
    for (size_t i = 0; i < symbols.size(); i++) {
        const auto& sym = symbols[i];
        std::cout << "      {\n";
        std::cout << "        \"name\": \"" << sym.name << "\",\n";
        std::cout << "        \"kind\": \"" << sym.kind << "\"\n";
        std::cout << "      }";
        if (i < symbols.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "    ]\n";
    std::cout << "  },\n";
    std::cout << "  \"error\": null\n";
    std::cout << "}\n";
}

void outputErrorJSON(const std::string& code, const std::string& message) {
    std::cout << "{\n";
    std::cout << "  \"success\": false,\n";
    std::cout << "  \"result\": null,\n";
    std::cout << "  \"error\": {\n";
    std::cout << "    \"code\": \"" << code << "\",\n";
    std::cout << "    \"message\": \"" << message << "\"\n";
    std::cout << "  }\n";
    std::cout << "}\n";
}

}} // namespace volta::lsp
