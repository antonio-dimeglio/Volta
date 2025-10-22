#include "Parser/ASTVisitor.hpp"
#include "Parser/AST.hpp"


void RecursiveASTVisitor::traverseExpr(Expr* expr) {
    if (expr != nullptr) {
        expr->accept(*this);
    }
}

void RecursiveASTVisitor::traverseStmt(Stmt* stmt) {
    if (stmt != nullptr) {
        stmt->accept(*this);
    }
}


void RecursiveASTVisitor::visitLiteral(Literal& node) {
}

void RecursiveASTVisitor::visitVariable(Variable& node) {
    // Variables have no children
}

void RecursiveASTVisitor::visitBinaryExpr(BinaryExpr& node) {
    traverseExpr(node.lhs.get());
    traverseExpr(node.rhs.get());
}

void RecursiveASTVisitor::visitUnaryExpr(UnaryExpr& node) {
    traverseExpr(node.operand.get());
}

void RecursiveASTVisitor::visitFnCall(FnCall& node) {
    for (auto& arg : node.args) {
        traverseExpr(arg.get());
    }
}

void RecursiveASTVisitor::visitAssignment(Assignment& node) {
    traverseExpr(node.lhs.get());
    traverseExpr(node.value.get());
}

void RecursiveASTVisitor::visitGroupingExpr(GroupingExpr& node) {
    traverseExpr(node.expr.get());
}

void RecursiveASTVisitor::visitArrayLiteral(ArrayLiteral& node) {
    for (auto& elem : node.elements) {
        traverseExpr(elem.get());
    }
    if (node.repeat_value) {
        traverseExpr(node.repeat_value.get());
    }
}

void RecursiveASTVisitor::visitIndexExpr(IndexExpr& node) {
    traverseExpr(node.array.get());
    traverseExpr(node.index.get());
}

void RecursiveASTVisitor::visitAddrOf(AddrOf& node) {
    traverseExpr(node.operand.get());
}

void RecursiveASTVisitor::visitCompoundAssign(CompoundAssign& node) {
    traverseExpr(node.var.get());
    traverseExpr(node.value.get());
}

void RecursiveASTVisitor::visitIncrement(Increment& node) {
    traverseExpr(node.var.get());
}

void RecursiveASTVisitor::visitDecrement(Decrement& node) {
    traverseExpr(node.var.get());
}

void RecursiveASTVisitor::visitRange(Range& node) {
    traverseExpr(node.from.get());
    traverseExpr(node.to.get());
}

void RecursiveASTVisitor::visitStructLiteral(StructLiteral& node) {
    for (auto& [fieldName, fieldValue] : node.fields) {
        traverseExpr(fieldValue.get());
    }
}

void RecursiveASTVisitor::visitFieldAccess(FieldAccess& node) {
    traverseExpr(node.object.get());
}

void RecursiveASTVisitor::visitStaticMethodCall(StaticMethodCall& node) {
    for (auto& arg : node.args) {
        traverseExpr(arg.get());
    }
}

void RecursiveASTVisitor::visitInstanceMethodCall(InstanceMethodCall& node) {
    traverseExpr(node.object.get());
    for (auto& arg : node.args) {
        traverseExpr(arg.get());
    }
}


void RecursiveASTVisitor::visitVarDecl(VarDecl& node) {
    if (node.initValue) {
        traverseExpr(node.initValue.get());
    }
}

void RecursiveASTVisitor::visitFnDecl(FnDecl& node) {
    for (auto& stmt : node.body) {
        traverseStmt(stmt.get());
    }
}

void RecursiveASTVisitor::visitStructDecl(StructDecl& node) {
    // No children to traverse for now (fields are data, not statements/expressions)
    // Methods will be added in Phase 3
}

void RecursiveASTVisitor::visitReturnStmt(ReturnStmt& node) {
    if (node.value) {
        traverseExpr(node.value.get());
    }
}

void RecursiveASTVisitor::visitIfStmt(IfStmt& node) {
    traverseExpr(node.condition.get());
    for (auto& stmt : node.thenBody) {
        traverseStmt(stmt.get());
    }
    for (auto& stmt : node.elseBody) {
        traverseStmt(stmt.get());
    }
}

void RecursiveASTVisitor::visitWhileStmt(WhileStmt& node) {
    traverseExpr(node.condition.get());
    for (auto& stmt : node.thenBody) {
        traverseStmt(stmt.get());
    }
}

void RecursiveASTVisitor::visitForStmt(ForStmt& node) {
    traverseExpr(node.var.get());
    traverseExpr(node.range.get());
    for (auto& stmt : node.body) {
        traverseStmt(stmt.get());
    }
}

void RecursiveASTVisitor::visitExprStmt(ExprStmt& node) {
    traverseExpr(node.expr.get());
}

void RecursiveASTVisitor::visitBlockStmt(BlockStmt& node) {
    for (auto& stmt : node.statements) {
        traverseStmt(stmt.get());
    }
}

void RecursiveASTVisitor::visitBreakStmt(BreakStmt& node) {
    // No children
}

void RecursiveASTVisitor::visitContinueStmt(ContinueStmt& node) {
    // No children
}

void RecursiveASTVisitor::visitExternBlock(ExternBlock& node) {
    for (auto& decl : node.declarations) {
        traverseStmt(decl.get());
    }
}

void RecursiveASTVisitor::visitImportStmt(ImportStmt& node) {
    // No children
}
