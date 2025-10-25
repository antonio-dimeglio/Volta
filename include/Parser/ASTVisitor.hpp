#pragma once

// Forward declarations (avoid circular includes)
struct Expr;
struct Stmt;

// Expression types
struct Literal;
struct Variable;
struct BinaryExpr;
struct UnaryExpr;
struct FnCall;
struct Assignment;
struct GroupingExpr;
struct ArrayLiteral;
struct IndexExpr;
struct AddrOf;
struct SizeOf;
struct CompoundAssign;
struct Increment;
struct Decrement;
struct Range;
struct StructLiteral;
struct FieldAccess;
struct StaticMethodCall;
struct InstanceMethodCall;

// Statement types
struct VarDecl;
struct FnDecl;
struct StructDecl;
struct ReturnStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct ExprStmt;
struct BlockStmt;
struct BreakStmt;
struct ContinueStmt;
struct ExternBlock;
struct ImportStmt;


template<typename ReturnType = void>
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    virtual ReturnType visitLiteral(Literal& node) = 0;
    virtual ReturnType visitVariable(Variable& node) = 0;
    virtual ReturnType visitBinaryExpr(BinaryExpr& node) = 0;
    virtual ReturnType visitUnaryExpr(UnaryExpr& node) = 0;
    virtual ReturnType visitFnCall(FnCall& node) = 0;
    virtual ReturnType visitStaticMethodCall(StaticMethodCall& node) = 0;
    virtual ReturnType visitInstanceMethodCall(InstanceMethodCall& node) = 0;
    virtual ReturnType visitAssignment(Assignment& node) = 0;
    virtual ReturnType visitGroupingExpr(GroupingExpr& node) = 0;
    virtual ReturnType visitArrayLiteral(ArrayLiteral& node) = 0;
    virtual ReturnType visitIndexExpr(IndexExpr& node) = 0;
    virtual ReturnType visitAddrOf(AddrOf& node) = 0;
    virtual ReturnType visitSizeOf(SizeOf& node) = 0;
    virtual ReturnType visitCompoundAssign(CompoundAssign& node) = 0;
    virtual ReturnType visitIncrement(Increment& node) = 0;
    virtual ReturnType visitDecrement(Decrement& node) = 0;
    virtual ReturnType visitRange(Range& node) = 0;
    virtual ReturnType visitStructLiteral(StructLiteral& node) = 0;
    virtual ReturnType visitFieldAccess(FieldAccess& node) = 0;
};

class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;

    virtual void visitVarDecl(VarDecl& node) = 0;
    virtual void visitFnDecl(FnDecl& node) = 0;
    virtual void visitStructDecl(StructDecl& node) = 0;
    virtual void visitReturnStmt(ReturnStmt& node) = 0;
    virtual void visitIfStmt(IfStmt& node) = 0;
    virtual void visitWhileStmt(WhileStmt& node) = 0;
    virtual void visitForStmt(ForStmt& node) = 0;
    virtual void visitExprStmt(ExprStmt& node) = 0;
    virtual void visitBlockStmt(BlockStmt& node) = 0;
    virtual void visitBreakStmt(BreakStmt& node) = 0;
    virtual void visitContinueStmt(ContinueStmt& node) = 0;
    virtual void visitExternBlock(ExternBlock& node) = 0;
    virtual void visitImportStmt(ImportStmt& node) = 0;
};


class RecursiveASTVisitor : public StmtVisitor, public ExprVisitor<void> {
public:
    void visitLiteral(Literal& node) override;
    void visitVariable(Variable& node) override;
    void visitBinaryExpr(BinaryExpr& node) override;
    void visitUnaryExpr(UnaryExpr& node) override;
    void visitFnCall(FnCall& node) override;
    void visitAssignment(Assignment& node) override;
    void visitGroupingExpr(GroupingExpr& node) override;
    void visitArrayLiteral(ArrayLiteral& node) override;
    void visitIndexExpr(IndexExpr& node) override;
    void visitAddrOf(AddrOf& node) override;
    void visitSizeOf(SizeOf& node) override;
    void visitCompoundAssign(CompoundAssign& node) override;
    void visitIncrement(Increment& node) override;
    void visitDecrement(Decrement& node) override;
    void visitRange(Range& node) override;
    void visitStructLiteral(StructLiteral& node) override;
    void visitFieldAccess(FieldAccess& node) override;
    void visitStaticMethodCall(StaticMethodCall& node) override;
    void visitInstanceMethodCall(InstanceMethodCall& node) override;

    void visitVarDecl(VarDecl& node) override;
    void visitFnDecl(FnDecl& node) override;
    void visitStructDecl(StructDecl& node) override;
    void visitReturnStmt(ReturnStmt& node) override;
    void visitIfStmt(IfStmt& node) override;
    void visitWhileStmt(WhileStmt& node) override;
    void visitForStmt(ForStmt& node) override;
    void visitExprStmt(ExprStmt& node) override;
    void visitBlockStmt(BlockStmt& node) override;
    void visitBreakStmt(BreakStmt& node) override;
    void visitContinueStmt(ContinueStmt& node) override;
    void visitExternBlock(ExternBlock& node) override;
    void visitImportStmt(ImportStmt& node) override;

protected:
    void traverseExpr(Expr* expr);
    void traverseStmt(Stmt* stmt);
};
