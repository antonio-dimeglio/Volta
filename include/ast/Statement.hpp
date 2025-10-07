#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "error/ErrorTypes.hpp"
#include "Expression.hpp"
#include "Type.hpp"

namespace volta::ast {

struct Statement {
    virtual ~Statement() = default;
    template<typename Visitor>
    auto accept(Visitor& visitor) const -> decltype(visitor.visit(*this)) { return visitor.visit(*this); }

    volta::errors::SourceLocation location;
    Statement(volta::errors::SourceLocation location) :
        location(location) {}
};

struct Program : Statement {
    std::vector<std::unique_ptr<Statement>> statements;

    Program(
        std::vector<std::unique_ptr<Statement>> statements,
        volta::errors::SourceLocation location
    ) : Statement(location), 
        statements(std::move(statements)) {}
};

struct ExpressionStatement: Statement {
    std::unique_ptr<Expression> expr;

    ExpressionStatement(
        std::unique_ptr<Expression> expr,
        volta::errors::SourceLocation location
    ) : Statement(location), 
        expr(std::move(expr)) {}
};

struct Block : Statement {
    std::vector<std::unique_ptr<Statement>> statements; 
    Block(
        std::vector<std::unique_ptr<Statement>> statements,
        volta::errors::SourceLocation location
    ) : Statement(location), 
        statements(std::move(statements)) {}
};

struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> thenBlock;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Block>>> elseIfClauses;
    std::unique_ptr<Block> elseBlock;
    
    IfStatement(
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Block> thenBlock,
        std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Block>>> elseIfClauses,
        std::unique_ptr<Block> elseBlock,
        volta::errors::SourceLocation location
    ) : Statement(location), 
        condition(std::move(condition)), 
        thenBlock(std::move(thenBlock)),
        elseIfClauses(std::move(elseIfClauses)),
        elseBlock(std::move(elseBlock)) {}
};

struct WhileStatement : Statement { 
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> thenBlock;
    
    WhileStatement(
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Block> thenBlock,
        volta::errors::SourceLocation location
    ) : Statement(location),
        condition(std::move(condition)),
        thenBlock(std::move(thenBlock)) {}
};

struct ForStatement : Statement {
    std::string identifier;
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Block> thenBlock;

    ForStatement(
        std::string identifier,
        std::unique_ptr<Expression> expression,
        std::unique_ptr<Block> thenBlock,
        volta::errors::SourceLocation location
    ) : Statement(location),
        identifier(identifier),
        expression(std::move(expression)),
        thenBlock(std::move(thenBlock)) {}
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expression> expression;

    ReturnStatement(
        std::unique_ptr<Expression> expression,
        volta::errors::SourceLocation location
    ) : Statement(location),
        expression(std::move(expression)) {}
};

struct BreakStatement : Statement {
    BreakStatement(volta::errors::SourceLocation location)
        : Statement(location) {}
};

struct ContinueStatement : Statement {
    ContinueStatement(volta::errors::SourceLocation location)
        : Statement(location) {}
};

struct ImportStatement : Statement {
    std::string identifier;

    ImportStatement(
        std::string identifier,
        volta::errors::SourceLocation location
    ) : Statement(location),
        identifier(identifier) {}
};

// Forward declarations for type system
struct Type;
struct TypeAnnotation;

struct VarDeclaration : Statement {
    std::string identifier;
    std::unique_ptr<TypeAnnotation> typeAnnotation;  // nullptr for := syntax
    std::unique_ptr<Expression> initializer;

    VarDeclaration(
        std::string identifier,
        std::unique_ptr<TypeAnnotation> typeAnnotation,
        std::unique_ptr<Expression> initializer,
        volta::errors::SourceLocation location
    ) : Statement(location),
        identifier(identifier),
        typeAnnotation(std::move(typeAnnotation)),
        initializer(std::move(initializer)) {}
};

struct Parameter {
    std::string identifier;  
    std::unique_ptr<Type> type;

    Parameter(
        std::string identifier,
        std::unique_ptr<Type> type
    ) : identifier(std::move(identifier)),
        type(std::move(type)) {}
};

struct FnDeclaration : Statement {
    std::string identifier;
    std::vector<std::string> typeParams;
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<Block> body;
    std::unique_ptr<Expression> expressionBody;

    // Method information (empty if regular function)
    std::string receiverType;  // e.g., "Point" for fn Point.distance()
    bool isMethod = false;

    FnDeclaration(
        std::string identifier,
        std::vector<std::string> typeParams,
        std::vector<std::unique_ptr<Parameter>> parameters,
        std::unique_ptr<Type> returnType,
        std::unique_ptr<Block> body,
        std::unique_ptr<Expression> expressionBody,
        volta::errors::SourceLocation location,
        std::string receiverType = "",
        bool isMethod = false
    ) : Statement(location),
        identifier(std::move(identifier)),
        typeParams(std::move(typeParams)),
        parameters(std::move(parameters)),
        returnType(std::move(returnType)),
        body(std::move(body)),
        expressionBody(std::move(expressionBody)),
        receiverType(std::move(receiverType)),
        isMethod(isMethod) {}
};

struct StructField {
    std::string identifier;
    std::unique_ptr<Type> type;

    StructField(
        std::string identifier,
        std::unique_ptr<Type> type
    ) : identifier(identifier),
        type(std::move(type)) {}
};

struct StructDeclaration : Statement {
    std::string identifier;
    std::vector<std::unique_ptr<StructField>> fields;

    StructDeclaration(
        std::string identifier,
        std::vector<std::unique_ptr<StructField>> fields,
        volta::errors::SourceLocation location
    ) : Statement(location),
        identifier(identifier),
        fields(std::move(fields)) {}
};

struct EnumVariant : Statement {
    std::string name;
    std::vector<std::unique_ptr<Type>> associatedTypes;  // Optional: types for variant data

    EnumVariant(
        std::string name,
        std::vector<std::unique_ptr<Type>> associatedTypes,
        volta::errors::SourceLocation location
    ) : Statement(location), name(std::move(name)),
        associatedTypes(std::move(associatedTypes)) {}
};

struct EnumDeclaration : Statement {
    std::string name;
    std::vector<std::string> typeParams; 
    std::vector<std::unique_ptr<EnumVariant>> variants;

    EnumDeclaration(
        std::string name,
        std::vector<std::string> typeParams,
        std::vector<std::unique_ptr<EnumVariant>> variants,
        volta::errors::SourceLocation location
    ) : Statement(location),
        name(name),
        typeParams(std::move(typeParams)),
        variants(std::move(variants)) {}
};


}