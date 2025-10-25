#include "AST.hpp"
#include <sstream>

std::string FnCall::toString() const {
    std::ostringstream oss;
    oss << "FnCall(" << name;
    if (isGeneric()) {
        oss << "<";
        for (size_t i = 0; i < typeArgs.size(); ++i) {
            oss << typeArgs[i]->toString() << (i < typeArgs.size() - 1 ? ", " : "");
        }
        oss << ">";
    }
    oss << "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) { oss << ", ";
}
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
    std::ostringstream oss;
    oss << "Var(" << token.lexeme;
    if (!typeArgs.empty()) {
        oss << "<";
        for (size_t i = 0; i < typeArgs.size(); ++i) {
            oss << typeArgs[i]->toString() << (i < typeArgs.size() - 1 ? ", " : "");
        }
        oss << ">";
    }
    oss << ")";
    return oss.str();
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
            if (i > 0) { oss << ", ";
}
            oss << elements[i]->toString();
        }
        oss << "]";
    }
    return oss.str();
}

std::string AddrOf::toString() const {
    return "addrof " + operand->toString();
}

std::string SizeOf::toString() const {
    return "sizeof<" + (type ? type->toString() : "unknown") + ">";
}

std::string IndexExpr::toString() const {
    std::string idx = ""; 
    size_t i = 0;
    for (auto& node : index) {
        idx += node.get()->toString() + (i < index.size() - 1 ? ", " : "");
        i++;
    }
    return "Index(" + array->toString() + "[" + idx + "])";
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

std::string StructLiteral::toString() const {
    std::ostringstream oss;
    oss << structName.lexeme;
    if (isGeneric()) {
        oss << "<";
        for (size_t i = 0; i < typeArgs.size(); ++i) {
            oss << typeArgs[i]->toString() << (i < typeArgs.size() - 1 ? ", " : "");
        }
        oss << ">";
    }
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << fields[i].first.lexeme << ": " << fields[i].second->toString();
    }
    oss << " }";
    return oss.str();
}

std::string FieldAccess::toString() const {
    return object->toString() + "." + fieldName.lexeme;
}

std::string StaticMethodCall::toString() const {
    std::ostringstream oss;
    oss << typeName.lexeme;
    if (!typeArgs.empty()) {
        oss << "<";
        for (size_t i = 0; i < typeArgs.size(); ++i) {
            oss << typeArgs[i]->toString() << (i < typeArgs.size() - 1 ? ", " : "");
        }
        oss << ">";
    }
    oss << "." << methodName.lexeme << "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) { oss << ", ";
}
        oss << args[i]->toString();
    }
    oss << ")";
    return oss.str();
}

std::string InstanceMethodCall::toString() const {
    std::ostringstream oss;
    oss << object->toString() << "." << methodName.lexeme << "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) { oss << ", ";
}
        oss << args[i]->toString();
    }
    oss << ")";
    return oss.str();
}

std::string Param::toString() const {
    std::string typeStr = type->toString();
    if (isMutRef) {
        return "mut ref " + name + ": " + typeStr;
    } else if (isRef) {
        return "ref " + name + ": " + typeStr;
    } else if (isMutable) {
        return name + ": mut " + typeStr;
    } else {
        return name + ": " + typeStr;
    }
}

std::string StructField::toString() const {
    std::string fieldName = name.lexeme;
    std::string typeStr = type->toString();

    if (isPublic) {
        return "pub " + fieldName + ": " + typeStr;
    } else {
        return fieldName + ": " + typeStr;
    }
}

std::string VarDecl::toString() const {
    std::ostringstream oss;
    oss << "VarDecl(";
    if (mutable_) { oss << "mut ";
}
    oss << name.lexeme;
    if (typeAnnotation != nullptr) {
        oss << ": " << typeAnnotation->toString();
    }
    if (initValue) {
        oss << " = " << initValue->toString();
    }
    oss << ")";
    return oss.str();
}

std::string FnDecl::toString() const {
    std::ostringstream oss;


    oss << "FnDecl(" << name << ")";
    if (isGeneric()) {
        oss << "<";
        for (size_t i = 0; i < typeParamaters.size(); ++i) {
            oss << typeParamaters[i].lexeme << (i < typeParamaters.size() - 1 ? ", " : "");
        }
        oss << ">";
    }
    oss << "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << params[i].toString();
    }    
    oss << ")" << " -> " << returnType->toString();
    return oss.str();
}

std::string StructDecl::toString() const {
    std::ostringstream oss;
    oss << "Struct(" << name.lexeme << ")\n";
    if (isGeneric()) {
        oss << "<";
        for (size_t i = 0; i < typeParamaters.size(); ++i) {
            oss << typeParamaters[i].lexeme << (i < typeParamaters.size() - 1 ? ", " : "");
        }
        oss << ">";
    }
    for (size_t i = 0; i < fields.size(); i++) {
        if (i > 0) { oss << ", "; }
        oss << fields[i].toString();
    }
    if (!methods.empty()) {
        oss << "\n  Methods: " << methods.size();
    }
    oss << ")";
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

std::string ImportStmt::toString() const {
    std::ostringstream oss;
    oss << "Import(\"" << modulePath << "\"";
    if (!importedItems.empty()) {
        oss << ", {";
        for (size_t i = 0; i < importedItems.size(); ++i) {
            if (i > 0) { oss << ", ";
}
            oss << importedItems[i];
        }
        oss << "}";
    }
    oss << ")";
    return oss.str();
}

std::string Program::toString() const {
    return "Program(" + std::to_string(statements.size()) + " statements)";
}