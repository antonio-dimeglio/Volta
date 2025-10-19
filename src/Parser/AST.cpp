#include "AST.hpp"
#include <sstream>

std::string FnCall::toString() const {
    std::ostringstream oss;
    oss << "FnCall(" << name << "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << args[i]->toString();
    }
    oss << "))";
    return oss.str();
}

std::string BinaryExpr::toString() const {
    std::ostringstream oss;
    oss << "(" << lhs->toString() << " " << tokenTypeToString(op) << " " << rhs->toString() << ")";
    return oss.str();
}

std::string UnaryExpr::toString() const {
    std::ostringstream oss;
    oss << "(" << tokenTypeToString(op) << " " << operand->toString() << ")";
    return oss.str();
}

std::string Literal::toString() const {
    return "Literal(" + token.lexeme + ")";
}

std::string Variable::toString() const {
    return "Var(" + token.lexeme + ")";
}

std::string Assignment::toString() const {
    return "Assignment(" + lhs->toString() + " = " + value->toString() + ")";
}

std::string GroupingExpr::toString() const {
    return "(group " + expr->toString() + ")";
}

std::string ArrayLiteral::toString() const {
    std::ostringstream oss;
    if (repeat_value) {
        oss << "[" << repeat_value->toString() << "; " << *repeat_count << "]";
    } else {
        oss << "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << elements[i]->toString();
        }
        oss << "]";
    }
    return oss.str();
}

std::string AddrOf::toString() const {
    return "addrof " + operand->toString();
}

std::string IndexExpr::toString() const {
    return "Index(" + array->toString() + "[" + index->toString() + "])";
}

std::string CompoundAssign::toString() const {
    return "CompoundAssign(" + var->token.lexeme + " " + tokenTypeToString(op) + " " + value->toString() + ")";
}

std::string Increment::toString() const {
    return "Increment(" + var->token.lexeme + "++)";
}

std::string Decrement::toString() const {
    return "Decrement(" + var->token.lexeme + "--)";
}

std::string Range::toString() const {
    return "Range(" + from->toString() + ", " + to->toString() + ")";
}

std::string Param::toString() const {
    std::string type_str = type->toString();
    if (isMutRef) {
        return "mut ref " + name + ": " + type_str;
    } else if (isRef) {
        return "ref " + name + ": " + type_str;
    } else {
        return name + ": " + type_str;
    }
}

std::string VarDecl::toString() const {
    std::ostringstream oss;
    oss << "VarDecl(";
    if (mutable_) oss << "mut ";
    oss << name.lexeme;
    if (type_annotation) {
        oss << ": " << type_annotation->toString();
    }
    if (init_value) {
        oss << " = " << init_value->toString();
    }
    oss << ")";
    return oss.str();
}

std::string FnDecl::toString() const {
    std::ostringstream oss;
    oss << "FnDecl(" << name << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << params[i].toString();
    }
    oss << ") -> " << returnType->toString() << ")";
    return oss.str();
}

std::string ReturnStmt::toString() const {
    if (value) {
        return "Return(" + value->toString() + ")";
    }
    return "Return()";
}

std::string IfStmt::toString() const {
    std::string else_str = elseBody.empty() ? "" : " else {...}";
    return "IfStmt(if " + condition->toString() + " {...}" + else_str + ")";
}

std::string ExprStmt::toString() const {
    return "ExprStmt(" + expr->toString() + ")";
}

std::string WhileStmt::toString() const {
    return "WhileStmt(" + condition->toString() + ")";
}

std::string ForStmt::toString() const {
    return "ForStmt(" + range->toString() + ")";
}

std::string BlockStmt::toString() const {
    return "BlockStmt(" + std::to_string(statements.size()) + " statements)";
}

std::string BreakStmt::toString() const {
    return "Break";
}

std::string ContinueStmt::toString() const {
    return "Continue";
}

std::string ExternBlock::toString() const {
    std::ostringstream oss;
    oss << "ExternBlock(\"" << abi << "\", " << declarations.size() << " declarations)";
    return oss.str();
}

// ==================== PROGRAM ====================

std::string Program::toString() const {
    return "Program(" + std::to_string(statements.size()) + " statements)";
}