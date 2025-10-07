#include "ast/ASTDumper.hpp"

namespace volta::ast {

// Main dump methods
std::string ASTDumper::dump(const Statement& stmt) {
    ss.str("");
    ss.clear();
    dumpStatement(stmt);
    return ss.str();
}

std::string ASTDumper::dump(const Expression& expr) {
    ss.str("");
    ss.clear();
    dumpExpression(expr);
    return ss.str();
}

std::string ASTDumper::dump(const Type& type) {
    ss.str("");
    ss.clear();
    dumpType(type);
    return ss.str();
}

std::string ASTDumper::dump(const Pattern& pattern) {
    ss.str("");
    ss.clear();
    dumpPattern(pattern);
    return ss.str();
}

// Statement dumpers
void ASTDumper::dumpStatement(const Statement& stmt) {
    if (auto* prog = dynamic_cast<const Program*>(&stmt)) {
        dumpProgram(*prog);
    } else if (auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        dumpExpressionStatement(*exprStmt);
    } else if (auto* block = dynamic_cast<const Block*>(&stmt)) {
        dumpBlock(*block);
    } else if (auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        dumpIfStatement(*ifStmt);
    } else if (auto* enumDecl = dynamic_cast<const EnumDeclaration*>(&stmt)) { 
        dumpEnumDeclaration(*enumDecl);
    } else if (auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        dumpWhileStatement(*whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        dumpForStatement(*forStmt);
    } else if (auto* retStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        dumpReturnStatement(*retStmt);
    } else if (auto* impStmt = dynamic_cast<const ImportStatement*>(&stmt)) {
        dumpImportStatement(*impStmt);
    } else if (auto* varDecl = dynamic_cast<const VarDeclaration*>(&stmt)) {
        dumpVarDeclaration(*varDecl);
    } else if (auto* fnDecl = dynamic_cast<const FnDeclaration*>(&stmt)) {
        dumpFnDeclaration(*fnDecl);
    } else if (auto* structDecl = dynamic_cast<const StructDeclaration*>(&stmt)) {
        dumpStructDeclaration(*structDecl);
    }
}

void ASTDumper::dumpProgram(const Program& prog) {
    indent();
    ss << "Program\n";
    increaseIndent();
    for (const auto& stmt : prog.statements) {
        dumpStatement(*stmt);
    }
    decreaseIndent();
}

void ASTDumper::dumpExpressionStatement(const ExpressionStatement& stmt) {
    indent();
    ss << "ExpressionStatement\n";
    increaseIndent();
    dumpExpression(*stmt.expr);
    decreaseIndent();
}

void ASTDumper::dumpBlock(const Block& block) {
    indent();
    ss << "Block\n";
    increaseIndent();
    for (const auto& stmt : block.statements) {
        dumpStatement(*stmt);
    }
    decreaseIndent();
}

void ASTDumper::dumpEnumDeclaration(const EnumDeclaration& stmt) {
    indent();
    ss << "EnumDeclaration: " << stmt.name;

    // Print generic type parameters if present
    if (!stmt.typeParams.empty()) {
        ss << "[";
        for (size_t i = 0; i < stmt.typeParams.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << stmt.typeParams[i];
        }
        ss << "]";
    }
    ss << "\n";

    increaseIndent();

    // Dump each variant
    for (const auto& variant : stmt.variants) {
        indent();
        ss << "Variant: " << variant->name;

        if (!variant->associatedTypes.empty()) {
            ss << "(";
            for (size_t i = 0; i < variant->associatedTypes.size(); ++i) {
                if (i > 0) ss << ", ";
                // Simplified type printing - just show it has types
                ss << "[type]";
            }
            ss << ")";
        }
        ss << "\n";
    }

    decreaseIndent();
}

void ASTDumper::dumpIfStatement(const IfStatement& stmt) {
    indent();
    ss << "IfStatement\n";
    increaseIndent();

    indent();
    ss << "condition:\n";
    increaseIndent();
    dumpExpression(*stmt.condition);
    decreaseIndent();

    indent();
    ss << "then:\n";
    increaseIndent();
    dumpBlock(*stmt.thenBlock);
    decreaseIndent();

    for (const auto& [cond, block] : stmt.elseIfClauses) {
        indent();
        ss << "else if:\n";
        increaseIndent();
        dumpExpression(*cond);
        dumpBlock(*block);
        decreaseIndent();
    }

    if (stmt.elseBlock) {
        indent();
        ss << "else:\n";
        increaseIndent();
        dumpBlock(*stmt.elseBlock);
        decreaseIndent();
    }

    decreaseIndent();
}

void ASTDumper::dumpWhileStatement(const WhileStatement& stmt) {
    indent();
    ss << "WhileStatement\n";
    increaseIndent();

    indent();
    ss << "condition:\n";
    increaseIndent();
    dumpExpression(*stmt.condition);
    decreaseIndent();

    indent();
    ss << "body:\n";
    increaseIndent();
    dumpBlock(*stmt.thenBlock);
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpForStatement(const ForStatement& stmt) {
    indent();
    ss << "ForStatement\n";
    increaseIndent();

    indent();
    ss << "identifier: " << stmt.identifier << "\n";

    indent();
    ss << "iterable:\n";
    increaseIndent();
    dumpExpression(*stmt.expression);
    decreaseIndent();

    indent();
    ss << "body:\n";
    increaseIndent();
    dumpBlock(*stmt.thenBlock);
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpReturnStatement(const ReturnStatement& stmt) {
    indent();
    ss << "ReturnStatement\n";
    if (stmt.expression) {
        increaseIndent();
        dumpExpression(*stmt.expression);
        decreaseIndent();
    }
}

void ASTDumper::dumpImportStatement(const ImportStatement& stmt) {
    indent();
    ss << "ImportStatement: " << stmt.identifier << "\n";
}

void ASTDumper::dumpVarDeclaration(const VarDeclaration& decl) {
    indent();
    ss << "VarDeclaration: " << decl.identifier << "\n";
    increaseIndent();

    if (decl.typeAnnotation) {
        indent();
        ss << "type:\n";
        increaseIndent();
        dumpTypeAnnotation(*decl.typeAnnotation);
        decreaseIndent();
    }

    indent();
    ss << "initializer:\n";
    increaseIndent();
    dumpExpression(*decl.initializer);
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpFnDeclaration(const FnDeclaration& decl) {
    indent();
    if (decl.isMethod) {
        ss << "MethodDeclaration: " << decl.receiverType << "." << decl.identifier << "\n";
    } else {
        ss << "FnDeclaration: " << decl.identifier << "\n";
    }
    increaseIndent();

    if (!decl.typeParams.empty()) {
        indent();
        ss << "type params: [";
        for (size_t i = 0; i < decl.typeParams.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << decl.typeParams[i];
        }
        ss << "]\n";
    }

    indent();
    ss << "parameters:\n";
    increaseIndent();
    for (const auto& param : decl.parameters) {
        indent();
        ss << param->identifier<< ": ";
        dumpType(*param->type);
        ss << "\n";
    }
    decreaseIndent();

    indent();
    ss << "return type:\n";
    increaseIndent();
    dumpType(*decl.returnType);
    ss << "\n";
    decreaseIndent();

    if (decl.body) {
        indent();
        ss << "body:\n";
        increaseIndent();
        dumpBlock(*decl.body);
        decreaseIndent();
    } else if (decl.expressionBody) {
        indent();
        ss << "expression body:\n";
        increaseIndent();
        dumpExpression(*decl.expressionBody);
        decreaseIndent();
    }

    decreaseIndent();
}

void ASTDumper::dumpStructDeclaration(const StructDeclaration& decl) {
    indent();
    ss << "StructDeclaration: " << decl.identifier << "\n";
    increaseIndent();

    indent();
    ss << "fields:\n";
    increaseIndent();
    for (const auto& field : decl.fields) {
        indent();
        ss << field->identifier << ": ";
        dumpType(*field->type);
        ss << "\n";
    }
    decreaseIndent();

    decreaseIndent();
}

// Expression dumpers
void ASTDumper::dumpExpression(const Expression& expr) {
    if (auto* ident = dynamic_cast<const IdentifierExpression*>(&expr)) {
        dumpIdentifier(*ident);
    } else if (auto* intLit = dynamic_cast<const IntegerLiteral*>(&expr)) {
        dumpIntegerLiteral(*intLit);
    } else if (auto* floatLit = dynamic_cast<const FloatLiteral*>(&expr)) {
        dumpFloatLiteral(*floatLit);
    } else if (auto* strLit = dynamic_cast<const StringLiteral*>(&expr)) {
        dumpStringLiteral(*strLit);
    } else if (auto* boolLit = dynamic_cast<const BooleanLiteral*>(&expr)) {
        dumpBooleanLiteral(*boolLit);
    } // Note: NoneLiteral and SomeLiteral removed - now enum variants
    else if (auto* arrLit = dynamic_cast<const ArrayLiteral*>(&expr)) {
        dumpArrayLiteral(*arrLit);
    } else if (auto* tupLit = dynamic_cast<const TupleLiteral*>(&expr)) {
        dumpTupleLiteral(*tupLit);
    } else if (auto* structLit = dynamic_cast<const StructLiteral*>(&expr)) {
        dumpStructLiteral(*structLit);
    } else if (auto* binExpr = dynamic_cast<const BinaryExpression*>(&expr)) {
        dumpBinaryExpression(*binExpr);
    } else if (auto* unExpr = dynamic_cast<const UnaryExpression*>(&expr)) {
        dumpUnaryExpression(*unExpr);
    } else if (auto* callExpr = dynamic_cast<const CallExpression*>(&expr)) {
        dumpCallExpression(*callExpr);
    } else if (auto* indexExpr = dynamic_cast<const IndexExpression*>(&expr)) {
        dumpIndexExpression(*indexExpr);
    } else if (auto* sliceExpr = dynamic_cast<const SliceExpression*>(&expr)) {
        dumpSliceExpression(*sliceExpr);
    } else if (auto* memExpr = dynamic_cast<const MemberExpression*>(&expr)) {
        dumpMemberExpression(*memExpr);
    } else if (auto* methExpr = dynamic_cast<const MethodCallExpression*>(&expr)) {
        dumpMethodCallExpression(*methExpr);
    } else if (auto* ifExpr = dynamic_cast<const IfExpression*>(&expr)) {
        dumpIfExpression(*ifExpr);
    } else if (auto* matchExpr = dynamic_cast<const MatchExpression*>(&expr)) {
        dumpMatchExpression(*matchExpr);
    } else if (auto* lambdaExpr = dynamic_cast<const LambdaExpression*>(&expr)) {
        dumpLambdaExpression(*lambdaExpr);
    }
}

void ASTDumper::dumpIdentifier(const IdentifierExpression& expr) {
    indent();
    ss << "Identifier: " << expr.name << "\n";
}

void ASTDumper::dumpIntegerLiteral(const IntegerLiteral& lit) {
    indent();
    ss << "IntegerLiteral: " << lit.value << "\n";
}

void ASTDumper::dumpFloatLiteral(const FloatLiteral& lit) {
    indent();
    ss << "FloatLiteral: " << lit.value << "\n";
}

void ASTDumper::dumpStringLiteral(const StringLiteral& lit) {
    indent();
    ss << "StringLiteral: \"" << lit.value << "\"\n";
}

void ASTDumper::dumpBooleanLiteral(const BooleanLiteral& lit) {
    indent();
    ss << "BooleanLiteral: " << (lit.value ? "true" : "false") << "\n";
}

// Note: dumpNoneLiteral and dumpSomeLiteral removed - now enum variants

void ASTDumper::dumpArrayLiteral(const ArrayLiteral& lit) {
    indent();
    ss << "ArrayLiteral\n";
    increaseIndent();
    for (const auto& elem : lit.elements) {
        dumpExpression(*elem);
    }
    decreaseIndent();
}

void ASTDumper::dumpTupleLiteral(const TupleLiteral& lit) {
    indent();
    ss << "TupleLiteral\n";
    increaseIndent();
    for (const auto& elem : lit.elements) {
        dumpExpression(*elem);
    }
    decreaseIndent();
}

void ASTDumper::dumpStructLiteral(const StructLiteral& lit) {
    indent();
    ss << "StructLiteral: " << lit.structName->name << "\n";
    increaseIndent();
    for (const auto& field : lit.fields) {
        indent();
        ss << field->identifier->name << ":\n";
        increaseIndent();
        dumpExpression(*field->value);
        decreaseIndent();
    }
    decreaseIndent();
}

void ASTDumper::dumpBinaryExpression(const BinaryExpression& expr) {
    indent();
    ss << "BinaryExpression: " << getOperatorString(expr.op) << "\n";
    increaseIndent();

    indent();
    ss << "left:\n";
    increaseIndent();
    dumpExpression(*expr.left);
    decreaseIndent();

    indent();
    ss << "right:\n";
    increaseIndent();
    dumpExpression(*expr.right);
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpUnaryExpression(const UnaryExpression& expr) {
    indent();
    ss << "UnaryExpression: " << getOperatorString(expr.op) << "\n";
    increaseIndent();
    dumpExpression(*expr.operand);
    decreaseIndent();
}

void ASTDumper::dumpCallExpression(const CallExpression& expr) {
    indent();
    ss << "CallExpression\n";
    increaseIndent();

    indent();
    ss << "callee:\n";
    increaseIndent();
    dumpExpression(*expr.callee);
    decreaseIndent();

    indent();
    ss << "arguments:\n";
    increaseIndent();
    for (const auto& arg : expr.arguments) {
        dumpExpression(*arg);
    }
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpIndexExpression(const IndexExpression& expr) {
    indent();
    ss << "IndexExpression\n";
    increaseIndent();

    indent();
    ss << "object:\n";
    increaseIndent();
    dumpExpression(*expr.object);
    decreaseIndent();

    indent();
    ss << "index:\n";
    increaseIndent();
    dumpExpression(*expr.index);
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpSliceExpression(const SliceExpression& expr) {
    indent();
    ss << "SliceExpression\n";
    increaseIndent();

    indent();
    ss << "object:\n";
    increaseIndent();
    dumpExpression(*expr.object);
    decreaseIndent();

    if (expr.start) {
        indent();
        ss << "start:\n";
        increaseIndent();
        dumpExpression(*expr.start);
        decreaseIndent();
    }

    if (expr.end) {
        indent();
        ss << "end:\n";
        increaseIndent();
        dumpExpression(*expr.end);
        decreaseIndent();
    }

    decreaseIndent();
}

void ASTDumper::dumpMemberExpression(const MemberExpression& expr) {
    indent();
    ss << "MemberExpression\n";
    increaseIndent();

    indent();
    ss << "object:\n";
    increaseIndent();
    dumpExpression(*expr.object);
    decreaseIndent();

    indent();
    ss << "member: " << expr.member->name << "\n";

    decreaseIndent();
}

void ASTDumper::dumpMethodCallExpression(const MethodCallExpression& expr) {
    indent();
    ss << "MethodCallExpression\n";
    increaseIndent();

    indent();
    ss << "object:\n";
    increaseIndent();
    dumpExpression(*expr.object);
    decreaseIndent();

    indent();
    ss << "method: " << expr.method->name << "\n";

    indent();
    ss << "arguments:\n";
    increaseIndent();
    for (const auto& arg : expr.arguments) {
        dumpExpression(*arg);
    }
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpIfExpression(const IfExpression& expr) {
    indent();
    ss << "IfExpression\n";
    increaseIndent();

    indent();
    ss << "condition:\n";
    increaseIndent();
    dumpExpression(*expr.condition);
    decreaseIndent();

    indent();
    ss << "then:\n";
    increaseIndent();
    dumpExpression(*expr.thenExpr);
    decreaseIndent();

    indent();
    ss << "else:\n";
    increaseIndent();
    dumpExpression(*expr.elseExpr);
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpMatchExpression(const MatchExpression& expr) {
    indent();
    ss << "MatchExpression\n";
    increaseIndent();

    indent();
    ss << "value:\n";
    increaseIndent();
    dumpExpression(*expr.value);
    decreaseIndent();

    indent();
    ss << "arms:\n";
    increaseIndent();
    for (const auto& arm : expr.arms) {
        indent();
        ss << "arm:\n";
        increaseIndent();

        indent();
        ss << "pattern:\n";
        increaseIndent();
        dumpPattern(*arm->pattern);
        decreaseIndent();

        if (arm->guard) {
            indent();
            ss << "guard:\n";
            increaseIndent();
            dumpExpression(*arm->guard);
            decreaseIndent();
        }

        indent();
        ss << "body:\n";
        increaseIndent();
        dumpExpression(*arm->body);
        decreaseIndent();

        decreaseIndent();
    }
    decreaseIndent();

    decreaseIndent();
}

void ASTDumper::dumpLambdaExpression(const LambdaExpression& expr) {
    indent();
    ss << "LambdaExpression\n";
    increaseIndent();

    indent();
    ss << "parameters:\n";
    increaseIndent();
    for (const auto& param : expr.parameters) {
        indent();
        ss << param->identifier << ": ";
        dumpType(*param->type);
        ss << "\n";
    }
    decreaseIndent();

    indent();
    ss << "return type:\n";
    increaseIndent();
    dumpType(*expr.returnType);
    ss << "\n";
    decreaseIndent();

    if (expr.body) {
        indent();
        ss << "body:\n";
        increaseIndent();
        dumpExpression(*expr.body);
        decreaseIndent();
    } else if (expr.blockBody) {
        indent();
        ss << "block body:\n";
        increaseIndent();
        dumpBlock(*expr.blockBody);
        decreaseIndent();
    }

    decreaseIndent();
}

// Type dumpers
void ASTDumper::dumpType(const Type& type) {
    if (auto* primType = dynamic_cast<const PrimitiveType*>(&type)) {
        dumpPrimitiveType(*primType);
    } else if (auto* compType = dynamic_cast<const CompoundType*>(&type)) {
        dumpCompoundType(*compType);
    } else if (auto* genType = dynamic_cast<const GenericType*>(&type)) {
        dumpGenericType(*genType);
    } else if (auto* funcType = dynamic_cast<const FunctionType*>(&type)) {
        dumpFunctionType(*funcType);
    } else if (auto* namedType = dynamic_cast<const NamedType*>(&type)) {
        dumpNamedType(*namedType);
    }
}

void ASTDumper::dumpPrimitiveType(const PrimitiveType& type) {
    ss << getPrimitiveTypeString(type.kind);
}

void ASTDumper::dumpCompoundType(const CompoundType& type) {
    ss << getCompoundTypeString(type.kind);
    if (!type.typeArgs.empty()) {
        ss << "[";
        for (size_t i = 0; i < type.typeArgs.size(); ++i) {
            if (i > 0) ss << ", ";
            dumpType(*type.typeArgs[i]);
        }
        ss << "]";
    }
}

void ASTDumper::dumpGenericType(const GenericType& type) {
    ss << type.identifier->name << "[";
    for (size_t i = 0; i < type.typeArgs.size(); ++i) {
        if (i > 0) ss << ", ";
        dumpType(*type.typeArgs[i]);
    }
    ss << "]";
}

void ASTDumper::dumpFunctionType(const FunctionType& type) {
    ss << "fn(";
    for (size_t i = 0; i < type.paramTypes.size(); ++i) {
        if (i > 0) ss << ", ";
        dumpType(*type.paramTypes[i]);
    }
    ss << ") -> ";
    dumpType(*type.returnType);
}

void ASTDumper::dumpNamedType(const NamedType& type) {
    ss << type.identifier->name;
}

void ASTDumper::dumpTypeAnnotation(const TypeAnnotation& annot) {
    indent();
    if (annot.isMutable) {
        ss << "mut ";
    }
    dumpType(*annot.type);
    ss << "\n";
}

// Pattern dumpers
void ASTDumper::dumpPattern(const Pattern& pattern) {
    if (auto* litPat = dynamic_cast<const LiteralPattern*>(&pattern)) {
        dumpLiteralPattern(*litPat);
    } else if (auto* idPat = dynamic_cast<const IdentifierPattern*>(&pattern)) {
        dumpIdentifierPattern(*idPat);
    } else if (auto* tupPat = dynamic_cast<const TuplePattern*>(&pattern)) {
        dumpTuplePattern(*tupPat);
    } else if (auto* consPat = dynamic_cast<const ConstructorPattern*>(&pattern)) {
        dumpConstructorPattern(*consPat);
    } else if (auto* wildPat = dynamic_cast<const WildcardPattern*>(&pattern)) {
        dumpWildcardPattern(*wildPat);
    }
}

void ASTDumper::dumpLiteralPattern(const LiteralPattern& pattern) {
    indent();
    ss << "LiteralPattern:\n";
    increaseIndent();
    dumpExpression(*pattern.literal);
    decreaseIndent();
}

void ASTDumper::dumpIdentifierPattern(const IdentifierPattern& pattern) {
    indent();
    ss << "IdentifierPattern: " << pattern.identifier->name << "\n";
}

void ASTDumper::dumpTuplePattern(const TuplePattern& pattern) {
    indent();
    ss << "TuplePattern\n";
    increaseIndent();
    for (const auto& elem : pattern.elements) {
        dumpPattern(*elem);
    }
    decreaseIndent();
}

void ASTDumper::dumpConstructorPattern(const ConstructorPattern& pattern) {
    indent();
    ss << "ConstructorPattern: " << pattern.constructor->name << "\n";
    if (!pattern.arguments.empty()) {
        increaseIndent();
        for (const auto& arg : pattern.arguments) {
            dumpPattern(*arg);
        }
        decreaseIndent();
    }
}

void ASTDumper::dumpWildcardPattern(const WildcardPattern& pattern) {
    indent();
    ss << "WildcardPattern: _\n";
}

// Helper methods
std::string ASTDumper::getOperatorString(BinaryExpression::Operator op) {
    switch (op) {
        case BinaryExpression::Operator::Add: return "+";
        case BinaryExpression::Operator::Subtract: return "-";
        case BinaryExpression::Operator::Multiply: return "*";
        case BinaryExpression::Operator::Divide: return "/";
        case BinaryExpression::Operator::Modulo: return "%";
        case BinaryExpression::Operator::Power: return "**";
        case BinaryExpression::Operator::Equal: return "==";
        case BinaryExpression::Operator::NotEqual: return "!=";
        case BinaryExpression::Operator::Less: return "<";
        case BinaryExpression::Operator::Greater: return ">";
        case BinaryExpression::Operator::LessEqual: return "<=";
        case BinaryExpression::Operator::GreaterEqual: return ">=";
        case BinaryExpression::Operator::LogicalAnd: return "and";
        case BinaryExpression::Operator::LogicalOr: return "or";
        case BinaryExpression::Operator::Range: return "..";
        case BinaryExpression::Operator::RangeInclusive: return "..=";
        case BinaryExpression::Operator::Assign: return "=";
        case BinaryExpression::Operator::AddAssign: return "+=";
        case BinaryExpression::Operator::SubtractAssign: return "-=";
        case BinaryExpression::Operator::MultiplyAssign: return "*=";
        case BinaryExpression::Operator::DivideAssign: return "/=";
        default: return "unknown";
    }
}

std::string ASTDumper::getOperatorString(UnaryExpression::Operator op) {
    switch (op) {
        case UnaryExpression::Operator::Not: return "not";
        case UnaryExpression::Operator::Negate: return "-";
        default: return "unknown";
    }
}

std::string ASTDumper::getPrimitiveTypeString(PrimitiveType::Kind kind) {
    switch (kind) {
        case PrimitiveType::Kind::Int: return "int";
        case PrimitiveType::Kind::Float: return "float";
        case PrimitiveType::Kind::Bool: return "bool";
        case PrimitiveType::Kind::Str: return "str";
        default: return "unknown";
    }
}

std::string ASTDumper::getCompoundTypeString(CompoundType::Kind kind) {
    switch (kind) {
        case CompoundType::Kind::Array: return "Array";
        case CompoundType::Kind::Matrix: return "Matrix";
        case CompoundType::Kind::Option: return "Option";
        case CompoundType::Kind::Tuple: return "Tuple";
        default: return "unknown";
    }
}

} // namespace volta::ast