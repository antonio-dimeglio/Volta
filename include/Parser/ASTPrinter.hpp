#pragma once

#include "AST.hpp"
#include <iostream>
#include <string>

class ASTPrinter {
private:
    int indentLevel = 0;
    const int indentSize = 2;

    std::string getIndent() const;
    void printIndented(const std::string& text) const;

public:
    ASTPrinter() = default;

    // Print the entire program
    void print(const Program& program, std::ostream& os = std::cout) const;

    // Print individual nodes
    void printStmt(const Stmt* stmt, std::ostream& os = std::cout, int indent = 0);
    void printExpr(const Expr* expr, std::ostream& os = std::cout) const;
    void printType(const Type* type, std::ostream& os = std::cout) const;

    // Specific node printers
    void printVarDecl(const VarDecl* node, std::ostream& os, int indent);
    void printFnDecl(const FnDecl* node, std::ostream& os, int indent);
    void printReturnStmt(const ReturnStmt* node, std::ostream& os, int indent);
    void printIfStmt(const IfStmt* node, std::ostream& os, int indent);
    void printWhileStmt(const WhileStmt* node, std::ostream& os, int indent);
    void printExprStmt(const ExprStmt* node, std::ostream& os, int indent);
    void printBreakStmt(const BreakStmt* node, std::ostream& os, int indent);
    void printContinueStmt(const ContinueStmt* node, std::ostream& os, int indent);
    void printForStmt(const ForStmt* node, std::ostream& os, int indent);
    void printBlockStmt(const BlockStmt* node, std::ostream& os, int indent);
    void printExternBlock(const ExternBlock* node, std::ostream& os, int indent);
    void printImportStmt(const ImportStmt* node, std::ostream& os, int indent);

    // Expression printers
    std::string exprToString(const Expr* expr) const;
    std::string fnCallToString(const FnCall* node) const;
    std::string binaryExprToString(const BinaryExpr* node) const;
    std::string unaryExprToString(const UnaryExpr* node) const;
    std::string literalToString(const Literal* node) const;
    std::string variableToString(const Variable* node) const;
    std::string assignmentToString(const Assignment* node) const;
    std::string groupingToString(const GroupingExpr* node) const;
    std::string arrayLiteralToString(const ArrayLiteral* node) const;
    std::string addrOfToString(const AddrOf* node) const;
    std::string indexExprToString(const IndexExpr* node) const;
    std::string compoundAssignToString(const CompoundAssign* node) const;
    std::string incrementToString(const Increment* node) const;
    std::string decrementToString(const Decrement* node) const;
    std::string rangeToString(const Range* node) const;
};