#include "ASTPrinter.hpp"
#include <sstream>

std::string ASTPrinter::getIndent() const {
    return std::string(indentLevel * indentSize, ' ');
}

void ASTPrinter::printIndented(const std::string& text) const {
    std::cout << getIndent() << text;
}

void ASTPrinter::print(const Program& program, std::ostream& os) const {
    os << "Program (" << program.statements.size() << " statements):\n";
    for (const auto& stmt : program.statements) {
        const_cast<ASTPrinter*>(this)->printStmt(stmt.get(), os, 1);
        os << "\n";
    }
}

void ASTPrinter::printStmt(const Stmt* stmt, std::ostream& os, int indent) {
    indentLevel = indent;

    if (auto* varDecl = dynamic_cast<const VarDecl*>(stmt)) {
        printVarDecl(varDecl, os, indent);
    } else if (auto* fnDecl = dynamic_cast<const FnDecl*>(stmt)) {
        printFnDecl(fnDecl, os, indent);
    } else if (auto* retStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        printReturnStmt(retStmt, os, indent);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        printIfStmt(ifStmt, os, indent);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        printWhileStmt(whileStmt, os, indent);
    } else if (auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        printExprStmt(exprStmt, os, indent);
    } else if (auto* breakStmt = dynamic_cast<const BreakStmt*>(stmt)) {
        printBreakStmt(breakStmt, os, indent);
    } else if (auto* continueStmt = dynamic_cast<const ContinueStmt*>(stmt)) {
        printContinueStmt(continueStmt, os, indent);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        printForStmt(forStmt, os, indent);
    } else if (auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        printBlockStmt(blockStmt, os, indent);
    } else if (auto* externBlock = dynamic_cast<const ExternBlock*>(stmt)) {
        printExternBlock(externBlock, os, indent);
    } else if (auto* importStmt = dynamic_cast<const ImportStmt*>(stmt)) {
        printImportStmt(importStmt, os, indent);  
    } else {
        os << getIndent() << "Unknown statement\n";
    }
}

void ASTPrinter::printExpr(const Expr* expr, std::ostream& os) const {
    os << exprToString(expr);
}

void ASTPrinter::printType(const Type* type, std::ostream& os) const {
    if (type) {
        os << type->toString();
    } else {
        os << "<no type>";
    }
}

// ==================== STATEMENT PRINTERS ====================

void ASTPrinter::printVarDecl(const VarDecl* node, std::ostream& os, int /* indent */) {
    os << getIndent() << "VarDecl: ";
    if (node->mutable_) os << "mut ";
    os << node->name.lexeme;

    if (node->type_annotation) {
        os << " : ";
        printType(node->type_annotation.get(), os);
    }

    if (node->init_value) {
        os << " = " << exprToString(node->init_value.get());
    }
}

void ASTPrinter::printFnDecl(const FnDecl* node, std::ostream& os, int indent) {
    os << getIndent() << "FnDecl: " << node->name << "(";

    for (size_t i = 0; i < node->params.size(); ++i) {
        if (i > 0) os << ", ";
        os << node->params[i].toString();
    }

    os << ") -> ";
    printType(node->returnType.get(), os);
    os << "\n";

    os << getIndent() << "{\n";
    for (const auto& stmt : node->body) {
        printStmt(stmt.get(), os, indent + 1);
        os << "\n";
    }
    os << getIndent() << "}";
}

void ASTPrinter::printReturnStmt(const ReturnStmt* node, std::ostream& os, int /* indent */) {
    os << getIndent() << "Return";
    if (node->value) {
        os << ": " << exprToString(node->value.get());
    }
}

void ASTPrinter::printIfStmt(const IfStmt* node, std::ostream& os, int indent) {
    os << getIndent() << "If: " << exprToString(node->condition.get()) << "\n";
    os << getIndent() << "Then:\n";

    for (const auto& stmt : node->thenBody) {
        printStmt(stmt.get(), os, indent + 1);
        os << "\n";
    }

    if (!node->elseBody.empty()) {
        os << getIndent() << "Else:\n";
        for (const auto& stmt : node->elseBody) {
            printStmt(stmt.get(), os, indent + 1);
            os << "\n";
        }
    }
}

void ASTPrinter::printWhileStmt(const WhileStmt* node, std::ostream& os, int indent) {
    os << getIndent() << "While: " << exprToString(node->condition.get()) << "\n";
    os << getIndent() << "{\n";

    for (const auto& stmt : node->thenBody) {
        printStmt(stmt.get(), os, indent + 1);
        os << "\n";
    }

    os << getIndent() << "}";
}

void ASTPrinter::printExprStmt(const ExprStmt* node, std::ostream& os, int /* indent */) {
    os << getIndent() << "ExprStmt: " << exprToString(node->expr.get());
}

void ASTPrinter::printBreakStmt(const BreakStmt* /* node */, std::ostream& os, int /* indent */) {
    os << getIndent() << "Break";
}

void ASTPrinter::printContinueStmt(const ContinueStmt* /* node */, std::ostream& os, int /* indent */) {
    os << getIndent() << "Continue";
}

void ASTPrinter::printForStmt(const ForStmt* node, std::ostream& os, int indent) {
    os << getIndent() << "For: " << node->var->token.lexeme << " in " << rangeToString(node->range.get()) << "\n";
    os << getIndent() << "{\n";

    for (const auto& stmt : node->body) {
        printStmt(stmt.get(), os, indent + 1);
        os << "\n";
    }

    os << getIndent() << "}";
}

void ASTPrinter::printBlockStmt(const BlockStmt* node, std::ostream& os, int indent) {
    os << getIndent() << "Block:\n";
    os << getIndent() << "{\n";

    for (const auto& stmt : node->statements) {
        printStmt(stmt.get(), os, indent + 1);
        os << "\n";
    }

    os << getIndent() << "}";
}

void ASTPrinter::printExternBlock(const ExternBlock* node, std::ostream& os, int indent) {
    os << getIndent() << "ExternBlock(\"" << node->abi << "\"):\n";
    os << getIndent() << "{\n";

    for (const auto& fn : node->declarations) {
        indentLevel = indent + 1;
        os << getIndent() << "ExternFnDecl: " << fn->name << "(";

        for (size_t i = 0; i < fn->params.size(); ++i) {
            if (i > 0) os << ", ";
            os << fn->params[i].toString();
        }

        os << ") -> ";
        printType(fn->returnType.get(), os);
        os << "\n";
    }

    indentLevel = indent;
    os << getIndent() << "}";
}

void ASTPrinter::printImportStmt(const ImportStmt* node, std::ostream& os, int indent) {
    os << getIndent() << "Import(\"" << node->modulePath << "\")";
    if (!node->importedItems.empty()) {
        os << " {";
        for (size_t i = 0; i < node->importedItems.size(); ++i) {
            if (i > 0) os << ", ";
            os << node->importedItems[i];
        }
        os << "}";
    }
    os << "\n";
}

std::string ASTPrinter::exprToString(const Expr* expr) const {
    if (auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        return fnCallToString(fnCall);
    } else if (auto* binExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        return binaryExprToString(binExpr);
    } else if (auto* unExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        return unaryExprToString(unExpr);
    } else if (auto* lit = dynamic_cast<const Literal*>(expr)) {
        return literalToString(lit);
    } else if (auto* var = dynamic_cast<const Variable*>(expr)) {
        return variableToString(var);
    } else if (auto* assign = dynamic_cast<const Assignment*>(expr)) {
        return assignmentToString(assign);
    } else if (auto* group = dynamic_cast<const GroupingExpr*>(expr)) {
        return groupingToString(group);
    } else if (auto* arr = dynamic_cast<const ArrayLiteral*>(expr)) {
        return arrayLiteralToString(arr);
    } else if (auto* idx = dynamic_cast<const IndexExpr*>(expr)) {
        return indexExprToString(idx);
    } else if (auto* compAssign = dynamic_cast<const CompoundAssign*>(expr)) {
        return compoundAssignToString(compAssign);
    } else if (auto* inc = dynamic_cast<const Increment*>(expr)) {
        return incrementToString(inc);
    } else if (auto* dec = dynamic_cast<const Decrement*>(expr)) {
        return decrementToString(dec);
    } else if (auto* range = dynamic_cast<const Range*>(expr)) {
        return rangeToString(range);
    } else if (auto* addrOf = dynamic_cast<const AddrOf*>(expr)) {
        return addrOfToString(addrOf);
    }

    return "<unknown expr>";
}

std::string ASTPrinter::fnCallToString(const FnCall* node) const {
    std::ostringstream oss;
    oss << node->name << "(";
    for (size_t i = 0; i < node->args.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << exprToString(node->args[i].get());
    }
    oss << ")";
    return oss.str();
}

std::string ASTPrinter::binaryExprToString(const BinaryExpr* node) const {
    std::ostringstream oss;
    oss << "(" << exprToString(node->lhs.get())
        << " " << tokenTypeToString(node->op)
        << " " << exprToString(node->rhs.get()) << ")";
    return oss.str();
}

std::string ASTPrinter::unaryExprToString(const UnaryExpr* node) const {
    std::ostringstream oss;
    oss << "(" << tokenTypeToString(node->op) << " " << exprToString(node->operand.get()) << ")";
    return oss.str();
}

std::string ASTPrinter::literalToString(const Literal* node) const {
    return node->token.lexeme;
}

std::string ASTPrinter::variableToString(const Variable* node) const {
    return node->token.lexeme;
}

std::string ASTPrinter::assignmentToString(const Assignment* node) const {
    return exprToString(node->lhs.get()) + " = " + exprToString(node->value.get());
}

std::string ASTPrinter::groupingToString(const GroupingExpr* node) const {
    return "(" + exprToString(node->expr.get()) + ")";
}

std::string ASTPrinter::arrayLiteralToString(const ArrayLiteral* node) const {
    std::ostringstream oss;
    if (node->repeat_value) {
        oss << "[" << exprToString(node->repeat_value.get()) << "; " << *node->repeat_count << "]";
    } else {
        oss << "[";
        for (size_t i = 0; i < node->elements.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << exprToString(node->elements[i].get());
        }
        oss << "]";
    }
    return oss.str();
}

std::string ASTPrinter::addrOfToString(const AddrOf* node) const {
    return "ptr " + exprToString(node->operand.get());
}

std::string ASTPrinter::indexExprToString(const IndexExpr* node) const {
    return exprToString(node->array.get()) + "[" + exprToString(node->index.get()) + "]";
}

std::string ASTPrinter::compoundAssignToString(const CompoundAssign* node) const {
    return node->var->token.lexeme + " " + tokenTypeToString(node->op) + " " + exprToString(node->value.get());
}

std::string ASTPrinter::incrementToString(const Increment* node) const {
    return node->var->token.lexeme + "++";
}

std::string ASTPrinter::decrementToString(const Decrement* node) const {
    return node->var->token.lexeme + "--";
}

std::string ASTPrinter::rangeToString(const Range* node) const {
    std::string op = node->inclusive ? "..=" : "..";
    return exprToString(node->from.get()) + op + exprToString(node->to.get());
}