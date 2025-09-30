#pragma once
#include <string>
#include <sstream>
#include "Statement.hpp"
#include "Expression.hpp"
#include "Type.hpp"

namespace volta::ast {

class ASTDumper {
private:
    std::ostringstream ss;
    int indentLevel = 0;
    const std::string indentStr = "  ";

    void indent() {
        for (int i = 0; i < indentLevel; ++i) {
            ss << indentStr;
        }
    }

    void increaseIndent() { indentLevel++; }
    void decreaseIndent() { indentLevel--; }

public:
    std::string dump(const Statement& stmt);
    std::string dump(const Expression& expr);
    std::string dump(const Type& type);
    std::string dump(const Pattern& pattern);

    // Statement dumpers
    void dumpStatement(const Statement& stmt);
    void dumpProgram(const Program& prog);
    void dumpExpressionStatement(const ExpressionStatement& stmt);
    void dumpBlock(const Block& block);
    void dumpIfStatement(const IfStatement& stmt);
    void dumpWhileStatement(const WhileStatement& stmt);
    void dumpForStatement(const ForStatement& stmt);
    void dumpReturnStatement(const ReturnStatement& stmt);
    void dumpImportStatement(const ImportStatement& stmt);
    void dumpVarDeclaration(const VarDeclaration& decl);
    void dumpFnDeclaration(const FnDeclaration& decl);
    void dumpStructDeclaration(const StructDeclaration& decl);

    // Expression dumpers
    void dumpExpression(const Expression& expr);
    void dumpIdentifier(const IdentifierExpression& expr);
    void dumpIntegerLiteral(const IntegerLiteral& lit);
    void dumpFloatLiteral(const FloatLiteral& lit);
    void dumpStringLiteral(const StringLiteral& lit);
    void dumpBooleanLiteral(const BooleanLiteral& lit);
    void dumpNoneLiteral(const NoneLiteral& lit);
    void dumpSomeLiteral(const SomeLiteral& lit);
    void dumpArrayLiteral(const ArrayLiteral& lit);
    void dumpTupleLiteral(const TupleLiteral& lit);
    void dumpStructLiteral(const StructLiteral& lit);
    void dumpBinaryExpression(const BinaryExpression& expr);
    void dumpUnaryExpression(const UnaryExpression& expr);
    void dumpCallExpression(const CallExpression& expr);
    void dumpIndexExpression(const IndexExpression& expr);
    void dumpSliceExpression(const SliceExpression& expr);
    void dumpMemberExpression(const MemberExpression& expr);
    void dumpMethodCallExpression(const MethodCallExpression& expr);
    void dumpIfExpression(const IfExpression& expr);
    void dumpMatchExpression(const MatchExpression& expr);
    void dumpLambdaExpression(const LambdaExpression& expr);

    // Type dumpers
    void dumpType(const Type& type);
    void dumpPrimitiveType(const PrimitiveType& type);
    void dumpCompoundType(const CompoundType& type);
    void dumpGenericType(const GenericType& type);
    void dumpFunctionType(const FunctionType& type);
    void dumpNamedType(const NamedType& type);
    void dumpTypeAnnotation(const TypeAnnotation& annot);

    // Pattern dumpers
    void dumpPattern(const Pattern& pattern);
    void dumpLiteralPattern(const LiteralPattern& pattern);
    void dumpIdentifierPattern(const IdentifierPattern& pattern);
    void dumpTuplePattern(const TuplePattern& pattern);
    void dumpConstructorPattern(const ConstructorPattern& pattern);
    void dumpWildcardPattern(const WildcardPattern& pattern);

    // Helper methods
    std::string getOperatorString(BinaryExpression::Operator op);
    std::string getOperatorString(UnaryExpression::Operator op);
    std::string getPrimitiveTypeString(PrimitiveType::Kind kind);
    std::string getCompoundTypeString(CompoundType::Kind kind);
};

} // namespace volta::ast