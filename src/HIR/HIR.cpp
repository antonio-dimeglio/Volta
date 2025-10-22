#include "HIR/HIR.hpp"

namespace HIR {

void HIRVarDecl::accept(HIRStmtVisitor& visitor) {
    visitor.visitVarDecl(*this);
}

void HIRExprStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitExprStmt(*this);
}

void HIRReturnStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitReturnStmt(*this);
}

void HIRIfStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitIfStmt(*this);
}

void HIRWhileStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitWhileStmt(*this);
}

void HIRBlockStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitBlockStmt(*this);
}

void HIRBreakStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitBreakStmt(*this);
}

void HIRContinueStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitContinueStmt(*this);
}

void HIRFnDecl::accept(HIRStmtVisitor& visitor) {
    visitor.visitFnDecl(*this);
}

void HIRExternBlock::accept(HIRStmtVisitor& visitor) {
    visitor.visitExternBlock(*this);
}

void HIRImportStmt::accept(HIRStmtVisitor& visitor) {
    visitor.visitImportStmt(*this);
}

void HIRStructDecl::accept(HIRStmtVisitor& visitor) {
    visitor.visitStructDecl(*this);
}

} // namespace HIR
