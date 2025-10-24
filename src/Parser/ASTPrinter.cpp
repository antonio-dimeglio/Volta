#include "Parser/ASTPrinter.hpp"
#include "Parser/ASTVisitor.hpp"
#include "Parser/AST.hpp"
#include "HIR/HIR.hpp"
#include <sstream>

// Forward declaration
void printHIRNodeHelper(const Stmt* stmt, std::ostream& os, int indent);

// Internal visitor class that does the actual printing
class ASTPrinterVisitor : public RecursiveASTVisitor {
    std::ostream& out;
    int indentLevel = 0;
    const int indentSize = 2;

    [[nodiscard]] std::string getIndent() const {
        return std::string(static_cast<size_t>(indentLevel * indentSize), ' ');
    }

    void printType(const Type::Type* type) {
        if (type != nullptr) {
            out << type->toString();
        } else {
            out << "<no type>";
        }
    }

    std::string exprToString(const Expr* expr);
    std::string rangeToString(const Range* node);

    // Override traverseStmt to handle HIR nodes
    void traverseStmtMixed(Stmt* stmt) {
        // Regular AST node - use visitor
        stmt->accept(*this);
    }

public:
    explicit ASTPrinterVisitor(std::ostream& out) : out(out) {}

    // Statement visitors
    void visitVarDecl(VarDecl& node) override {
        out << getIndent() << "VarDecl: ";
        if (node.mutable_) { out << "mut ";
}
        out << node.name.lexeme;

        if (node.typeAnnotation != nullptr) {
            out << " : ";
            printType(node.typeAnnotation);
        }

        if (node.initValue) {
            out << " = " << exprToString(node.initValue.get());
        }
    }

    void visitFnDecl(FnDecl& node) override {
        out << getIndent() << "FnDecl: " << node.name;
        if (node.isGeneric()) {
            out << "<";
            for (size_t i = 0; i < node.typeParamaters.size(); ++i) {
                out << node.typeParamaters[i].lexeme << (i < node.typeParamaters.size() - 1 ? ", " : "");
            }
            out << ">";
        }
        out << "(";

        for (size_t i = 0; i < node.params.size(); ++i) {
            if (i > 0) { out << ", ";
}
            out << node.params[i].toString();
        }

        out << ") -> ";
        printType(node.returnType);
        out << "\n";

        out << getIndent() << "{\n";
        indentLevel++;
        for (auto& stmt : node.body) {
            traverseStmtMixed(stmt.get());
            out << "\n";
        }
        indentLevel--;
        out << getIndent() << "}";
    }

    void visitStructDecl(StructDecl& node) override {
        out << getIndent();
        if (node.isPublic) { out << "pub "; }
        out << "struct " << node.name.lexeme;
        if (node.isGeneric()) {
            out << "<";
            for (size_t i = 0; i < node.typeParamaters.size(); ++i) {
                out << node.typeParamaters[i].lexeme << (i < node.typeParamaters.size() - 1 ? ", " : "");
            }
            out << ">";
        }
        out << " {\n";

        indentLevel++;
        for (const auto& field : node.fields) {
            out << getIndent();
            if (field.isPublic) { out << "pub "; }
            out << field.name.lexeme << ": ";
            printType(field.type);
            out << "\n";
        }

        // Print methods
        for (const auto& method : node.methods) {
            out << "\n" << getIndent();
            if (method->isPublic) { out << "pub ";
}
            out << "fn " << method->name << "(";
            for (size_t i = 0; i < method->params.size(); i++) {
                if (i > 0) { out << ", ";
}
                out << method->params[i].name << ": ";
                printType(method->params[i].type);
            }
            out << ") -> ";
            printType(method->returnType);
            out << " { ... }\n";
        }

        indentLevel--;
        out << getIndent() << "}";
    }

    void visitReturnStmt(ReturnStmt& node) override {
        out << getIndent() << "Return";
        if (node.value) {
            out << ": " << exprToString(node.value.get());
        }
    }

    void visitIfStmt(IfStmt& node) override {
        out << getIndent() << "If: " << exprToString(node.condition.get()) << "\n";
        out << getIndent() << "Then:\n";

        indentLevel++;
        for (auto& stmt : node.thenBody) {
            traverseStmt(stmt.get());
            out << "\n";
        }
        indentLevel--;

        if (!node.elseBody.empty()) {
            out << getIndent() << "Else:\n";
            indentLevel++;
            for (auto& stmt : node.elseBody) {
                traverseStmt(stmt.get());
                out << "\n";
            }
            indentLevel--;
        }
    }

    void visitWhileStmt(WhileStmt& node) override {
        out << getIndent() << "While: " << exprToString(node.condition.get()) << "\n";
        out << getIndent() << "{\n";

        indentLevel++;
        for (auto& stmt : node.thenBody) {
            traverseStmt(stmt.get());
            out << "\n";
        }
        indentLevel--;

        out << getIndent() << "}";
    }

    void visitForStmt(ForStmt& node) override {
        out << getIndent() << "For: " << node.var->token.lexeme
            << " in " << rangeToString(static_cast<Range*>(node.range.get())) << "\n";
        out << getIndent() << "{\n";

        indentLevel++;
        for (auto& stmt : node.body) {
            traverseStmt(stmt.get());
            out << "\n";
        }
        indentLevel--;

        out << getIndent() << "}";
    }

    void visitExprStmt(ExprStmt& node) override {
        out << getIndent() << "ExprStmt: " << exprToString(node.expr.get());
    }

    void visitBlockStmt(BlockStmt& node) override {
        out << getIndent() << "Block:\n";
        out << getIndent() << "{\n";

        indentLevel++;
        for (auto& stmt : node.statements) {
            traverseStmt(stmt.get());
            out << "\n";
        }
        indentLevel--;

        out << getIndent() << "}";
    }

    void visitBreakStmt(BreakStmt& /* node */) override {
        out << getIndent() << "Break";
    }

    void visitContinueStmt(ContinueStmt& /* node */) override {
        out << getIndent() << "Continue";
    }

    void visitExternBlock(ExternBlock& node) override {
        out << getIndent() << "ExternBlock(\"" << node.abi << "\"):\n";
        out << getIndent() << "{\n";

        indentLevel++;
        for (auto& fn : node.declarations) {
            out << getIndent() << "ExternFnDecl: " << fn->name << "(";
            for (size_t i = 0; i < fn->params.size(); ++i) {
                if (i > 0) { out << ", ";
}
                out << fn->params[i].toString();
            }

            out << ") -> ";
            printType(fn->returnType);
            out << "\n";
        }
        indentLevel--;

        out << getIndent() << "}";
    }

    void visitImportStmt(ImportStmt& node) override {
        out << getIndent() << "Import(\"" << node.modulePath << "\")";
        if (!node.importedItems.empty()) {
            out << " {";
            for (size_t i = 0; i < node.importedItems.size(); ++i) {
                if (i > 0) { out << ", ";
}
                out << node.importedItems[i];
            }
            out << "}";
        }
        out << "\n";
    }
};

std::string ASTPrinterVisitor::exprToString(const Expr* expr) {
    if (const auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        std::ostringstream oss;
        oss << fnCall->name;
        if (fnCall->isGeneric()) {
            oss << "<";
            for (size_t i = 0; i < fnCall->typeArgs.size(); ++i) {
                oss << fnCall->typeArgs[i]->toString() << (i < fnCall->typeArgs.size() - 1 ? ", " : "");
            }
            oss << ">";
        }
        oss << "(";
        for (size_t i = 0; i < fnCall->args.size(); ++i) {
            if (i > 0) { oss << ", ";
}
            oss << exprToString(fnCall->args[i].get());
        }
        oss << ")";
        return oss.str();
    } else if (const auto* binExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        std::ostringstream oss;
        oss << "(" << exprToString(binExpr->lhs.get())
            << " " << tokenTypeToString(binExpr->op)
            << " " << exprToString(binExpr->rhs.get()) << ")";
        return oss.str();
    } else if (const auto* unExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        std::ostringstream oss;
        oss << "(" << tokenTypeToString(unExpr->op) << " " << exprToString(unExpr->operand.get()) << ")";
        return oss.str();
    } else if (const auto* lit = dynamic_cast<const Literal*>(expr)) {
        return lit->token.lexeme;
    } else if (const auto* var = dynamic_cast<const Variable*>(expr)) {
        return var->token.lexeme;
    } else if (const auto* assign = dynamic_cast<const Assignment*>(expr)) {
        return exprToString(assign->lhs.get()) + " = " + exprToString(assign->value.get());
    } else if (const auto* group = dynamic_cast<const GroupingExpr*>(expr)) {
        return "(" + exprToString(group->expr.get()) + ")";
    } else if (const auto* arr = dynamic_cast<const ArrayLiteral*>(expr)) {
        std::ostringstream oss;
        if (arr->repeat_value) {
            oss << "[" << exprToString(arr->repeat_value.get()) << "; " << *arr->repeat_count << "]";
        } else {
            oss << "[";
            for (size_t i = 0; i < arr->elements.size(); ++i) {
                if (i > 0) { oss << ", ";
}
                oss << exprToString(arr->elements[i].get());
            }
            oss << "]";
        }
        return oss.str();
    } else if (const auto* idxExpr = dynamic_cast<const IndexExpr*>(expr)) {
        std::ostringstream oss;
        oss << exprToString(idxExpr->array.get()) << "[";
        for (size_t i = 0; i < idxExpr->index.size(); ++i) {
            if (i > 0) { oss << ", "; }
            oss << exprToString(idxExpr->index[i].get());
        }
        oss << "]";
        return oss.str();
    } else if (const auto* compAssign = dynamic_cast<const CompoundAssign*>(expr)) {
        return compAssign->var->token.lexeme + " " + tokenTypeToString(compAssign->op) + " " + exprToString(compAssign->value.get());
    } else if (const auto* inc = dynamic_cast<const Increment*>(expr)) {
        return inc->var->token.lexeme + "++";
    } else if (const auto* dec = dynamic_cast<const Decrement*>(expr)) {
        return dec->var->token.lexeme + "--";
    } else if (const auto* range = dynamic_cast<const Range*>(expr)) {
        return rangeToString(range);
    } else if (const auto* addrOf = dynamic_cast<const AddrOf*>(expr)) {
        return "ptr " + exprToString(addrOf->operand.get());
    } else if (const auto* staticCall = dynamic_cast<const StaticMethodCall*>(expr)) {
        std::ostringstream oss;
        oss << staticCall->typeName.lexeme << "::" << staticCall->methodName.lexeme << "(";
        for (size_t i = 0; i < staticCall->args.size(); ++i) {
            if (i > 0) { oss << ", ";
}
            oss << exprToString(staticCall->args[i].get());
        }
        oss << ")";
        return oss.str();
    } else if (const auto* instanceCall = dynamic_cast<const InstanceMethodCall*>(expr)) {
        std::ostringstream oss;
        oss << exprToString(instanceCall->object.get()) << "." << instanceCall->methodName.lexeme << "(";
        for (size_t i = 0; i < instanceCall->args.size(); ++i) {
            if (i > 0) { oss << ", ";
}
            oss << exprToString(instanceCall->args[i].get());
        }
        oss << ")";
        return oss.str();
    }

    return "<unknown expr>";
}

std::string ASTPrinterVisitor::rangeToString(const Range* node) {
    std::string op = node->inclusive ? "..=" : "..";
    return exprToString(node->from.get()) + op + exprToString(node->to.get());
}

// Helper to print expressions (reuse from ASTPrinterVisitor)
std::string exprToStringForHIR(const Expr* expr);

// Helper to print HIR nodes (which don't use the visitor pattern)
void printHIRNodeHelper(const HIR::HIRStmt* stmt, std::ostream& os, int indent) {
    std::string indentStr(indent * 2, ' ');

    if (const auto* hirRet = dynamic_cast<const HIR::HIRReturnStmt*>(stmt)) {
        os << indentStr << "HIRReturn";
        if (hirRet->value) {
            os << ": " << exprToStringForHIR(hirRet->value.get());
        }
    } else if (const auto* hirIf = dynamic_cast<const HIR::HIRIfStmt*>(stmt)) {
        os << indentStr << "HIRIf: " << exprToStringForHIR(hirIf->condition.get()) << "\n";
        os << indentStr << "Then:\n";
        for (const auto& s : hirIf->thenBody) {
            printHIRNodeHelper(s.get(), os, indent + 1);
            os << "\n";
        }
        if (!hirIf->elseBody.empty()) {
            os << indentStr << "Else:\n";
            for (const auto& s : hirIf->elseBody) {
                printHIRNodeHelper(s.get(), os, indent + 1);
                os << "\n";
            }
        }
    } else if (const auto* hirWhile = dynamic_cast<const HIR::HIRWhileStmt*>(stmt)) {
        os << indentStr << "HIRWhile: " << exprToStringForHIR(hirWhile->condition.get()) << "\n";
        os << indentStr << "{\n";
        for (const auto& s : hirWhile->body) {
            printHIRNodeHelper(s.get(), os, indent + 1);
            os << "\n";
        }
        if (hirWhile->increment) {
            os << indentStr << "  [increment: " << exprToStringForHIR(hirWhile->increment.get()) << "]\n";
        }
        os << indentStr << "}";
    } else if (const auto* hirBlock = dynamic_cast<const HIR::HIRBlockStmt*>(stmt)) {
        os << indentStr << "HIRBlock:\n";
        os << indentStr << "{\n";
        for (const auto& s : hirBlock->statements) {
            printHIRNodeHelper(s.get(), os, indent + 1);
            os << "\n";
        }
        os << indentStr << "}";
    } else if (const auto* hirExpr = dynamic_cast<const HIR::HIRExprStmt*>(stmt)) {
        os << indentStr << "HIRExprStmt: " << exprToStringForHIR(hirExpr->expr.get());
    } else if (const auto* hirVarDecl = dynamic_cast<const HIR::HIRVarDecl*>(stmt)) {
        os << indentStr << "HIRVarDecl: " << hirVarDecl->name.lexeme;
        if (hirVarDecl->typeAnnotation != nullptr) {
            os << " : " << hirVarDecl->typeAnnotation->toString();
        }
        os << " = " << exprToStringForHIR(hirVarDecl->initValue.get());
    } else if (const auto* hirFnDecl = dynamic_cast<const HIR::HIRFnDecl*>(stmt)) {
        os << indentStr << "HIRFnDecl: " << hirFnDecl->name << "(";
        for (size_t i = 0; i < hirFnDecl->params.size(); i++) {
            if (i > 0) { os << ", ";
}
            const auto& param = hirFnDecl->params[i];
            if (param.isMutRef) {
                os << "mut ref " << param.name << ": " << param.type->toString();
            } else if (param.isRef) {
                os << "ref " << param.name << ": " << param.type->toString();
            } else if (param.isMutable) {
                os << param.name << ": mut " << param.type->toString();
            } else {
                os << param.name << ": " << param.type->toString();
            }
        }
        os << ") -> ";
        if (hirFnDecl->returnType != nullptr) {
            os << hirFnDecl->returnType->toString();
        } else {
            os << "void";
        }
        os << "\n" << indentStr << "{\n";
        for (const auto& s : hirFnDecl->body) {
            printHIRNodeHelper(s.get(), os, indent + 1);
            os << "\n";
        }
        os << indentStr << "}";
    } else if (const auto* hirExternBlock = dynamic_cast<const HIR::HIRExternBlock*>(stmt)) {
        os << indentStr << "HIRExternBlock:\n";
        for (const auto& fn : hirExternBlock->declarations) {
            printHIRNodeHelper(fn.get(), os, indent + 1);
            os << "\n";
        }
    } else if (const auto* hirImport = dynamic_cast<const HIR::HIRImportStmt*>(stmt)) {
        os << indentStr << "HIRImport from " << hirImport->modulePath << ": ";
        for (size_t i = 0; i < hirImport->symbols.size(); i++) {
            if (i > 0) { os << ", ";
}
            os << hirImport->symbols[i];
        }
    } else if (dynamic_cast<const HIR::HIRBreakStmt*>(stmt) != nullptr) {
        os << indentStr << "HIRBreak";
    } else if (dynamic_cast<const HIR::HIRContinueStmt*>(stmt) != nullptr) {
        os << indentStr << "HIRContinue";
    } else {
        os << indentStr << "Unknown HIR node";
    }
}

// Standalone expression printer for HIR (copy of the one in ASTPrinterVisitor)
std::string exprToStringForHIR(const Expr* expr) {
    if (const auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        std::ostringstream oss;
        oss << fnCall->name;
        if (fnCall->isGeneric()) {
            oss << "<";
            for (size_t i = 0; i < fnCall->typeArgs.size(); ++i) {
                oss << fnCall->typeArgs[i]->toString() << (i < fnCall->typeArgs.size() - 1 ? ", " : "");
            }
            oss << ">";
        }
        oss << "(";
        for (size_t i = 0; i < fnCall->args.size(); ++i) {
            if (i > 0) { oss << ", ";
}
            oss << exprToStringForHIR(fnCall->args[i].get());
        }
        oss << ")";
        return oss.str();
    } else if (const auto* binExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        std::ostringstream oss;
        oss << "(" << exprToStringForHIR(binExpr->lhs.get())
            << " " << tokenTypeToString(binExpr->op)
            << " " << exprToStringForHIR(binExpr->rhs.get()) << ")";
        return oss.str();
    } else if (const auto* unExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        std::ostringstream oss;
        oss << "(" << tokenTypeToString(unExpr->op) << " " << exprToStringForHIR(unExpr->operand.get()) << ")";
        return oss.str();
    } else if (const auto* lit = dynamic_cast<const Literal*>(expr)) {
        return lit->token.lexeme;
    } else if (const auto* var = dynamic_cast<const Variable*>(expr)) {
        return var->token.lexeme;
    } else if (const auto* assign = dynamic_cast<const Assignment*>(expr)) {
        return exprToStringForHIR(assign->lhs.get()) + " = " + exprToStringForHIR(assign->value.get());
    } else if (const auto* group = dynamic_cast<const GroupingExpr*>(expr)) {
        return "(" + exprToStringForHIR(group->expr.get()) + ")";
    } else if (const auto* arr = dynamic_cast<const ArrayLiteral*>(expr)) {
        std::ostringstream oss;
        if (arr->repeat_value) {
            oss << "[" << exprToStringForHIR(arr->repeat_value.get()) << "; " << *arr->repeat_count << "]";
        } else {
            oss << "[";
            for (size_t i = 0; i < arr->elements.size(); ++i) {
                if (i > 0) { oss << ", ";
}
                oss << exprToStringForHIR(arr->elements[i].get());
            }
            oss << "]";
        }
        return oss.str();
    } else if (const auto* idx = dynamic_cast<const IndexExpr*>(expr)) {
        std::ostringstream oss;
        oss << exprToStringForHIR(idx->array.get()) << "[";
        for (size_t i = 0; i < idx->index.size(); ++i) {
            if (i > 0) { oss << ", "; }
            oss << exprToStringForHIR(idx->index[i].get());
        }
        oss << "]";
        return oss.str();
    } else if (const auto* addrOf = dynamic_cast<const AddrOf*>(expr)) {
        return "ptr " + exprToStringForHIR(addrOf->operand.get());
    }

    return "<unknown expr>";
}

// Public API implementation
void ASTPrinter::print(const Program& program, std::ostream& os) {
    os << "Program (" << program.statements.size() << " statements):\n";

    for (const auto& stmt : program.statements) {
        // Regular AST node, use visitor
        ASTPrinterVisitor visitor(os);
        const_cast<Stmt*>(stmt.get())->accept(visitor);
        os << "\n";
    }
}

// Print HIR program
void ASTPrinter::print(const HIR::HIRProgram& program, std::ostream& os) {
    os << "HIR Program (" << program.statements.size() << " statements):\n";

    for (const auto& stmt : program.statements) {
        printHIRNodeHelper(stmt.get(), os, 0);
        os << "\n";
    }
}
