#pragma once
#include <memory>
#include <string>
#include <vector>
#include "error/ErrorTypes.hpp"

namespace volta::ast {

// Forward declarations
struct Block;
struct Type;

struct Expression {
    virtual ~Expression() = default;
    volta::errors::SourceLocation location;

    Expression(volta::errors::SourceLocation location)
        : location(location) {}
};

struct IdentifierExpression : Expression {
    std::string name;

    IdentifierExpression(std::string name, volta::errors::SourceLocation location)
        : Expression(location), name(std::move(name)) {}
};

// Literals
struct IntegerLiteral : Expression {
    int64_t value;

    IntegerLiteral(int64_t value, volta::errors::SourceLocation location)
        : Expression(location), value(value) {}
};

struct FloatLiteral : Expression {
    double value;

    FloatLiteral(double value, volta::errors::SourceLocation location)
        : Expression(location), value(value) {}
};

struct StringLiteral : Expression {
    std::string value;

    StringLiteral(std::string value, volta::errors::SourceLocation location)
        : Expression(location), value(std::move(value)) {}
};

struct BooleanLiteral : Expression {
    bool value;

    BooleanLiteral(bool value, volta::errors::SourceLocation location)
        : Expression(location), value(value) {}
};

struct NoneLiteral : Expression {
    NoneLiteral(volta::errors::SourceLocation location)
        : Expression(location) {}
};

struct SomeLiteral : Expression {
    std::unique_ptr<Expression> value;

    SomeLiteral(std::unique_ptr<Expression> value, volta::errors::SourceLocation location)
        : Expression(location), value(std::move(value)) {}
};

struct ArrayLiteral : Expression {
    std::vector<std::unique_ptr<Expression>> elements;

    ArrayLiteral(std::vector<std::unique_ptr<Expression>> elements, volta::errors::SourceLocation location)
        : Expression(location), elements(std::move(elements)) {}
};

struct TupleLiteral : Expression {
    std::vector<std::unique_ptr<Expression>> elements;

    TupleLiteral(std::vector<std::unique_ptr<Expression>> elements, volta::errors::SourceLocation location)
        : Expression(location), elements(std::move(elements)) {}
};

struct FieldInit {
    std::unique_ptr<IdentifierExpression> identifier;
    std::unique_ptr<Expression> value;

    FieldInit(std::unique_ptr<IdentifierExpression> identifier, std::unique_ptr<Expression> value)
        : identifier(std::move(identifier)), value(std::move(value)) {}
};

struct StructLiteral : Expression {
    std::unique_ptr<IdentifierExpression> structName;
    std::vector<std::unique_ptr<FieldInit>> fields;

    StructLiteral(
        std::unique_ptr<IdentifierExpression> structName,
        std::vector<std::unique_ptr<FieldInit>> fields,
        volta::errors::SourceLocation location
    ) : Expression(location),
        structName(std::move(structName)),
        fields(std::move(fields)) {}
};

// Binary operators
struct BinaryExpression : Expression {
    enum class Operator {
        // Arithmetic
        Add, Subtract, Multiply, Divide, Modulo, Power,
        // Comparison
        Equal, NotEqual, Less, Greater, LessEqual, GreaterEqual,
        // Logical
        LogicalAnd, LogicalOr,
        // Range
        Range, RangeInclusive,
        // Assignment
        Assign, AddAssign, SubtractAssign, MultiplyAssign, DivideAssign
    };

    std::unique_ptr<Expression> left;
    Operator op;
    std::unique_ptr<Expression> right;

    BinaryExpression(
        std::unique_ptr<Expression> left,
        Operator op,
        std::unique_ptr<Expression> right,
        volta::errors::SourceLocation location
    ) : Expression(location),
        left(std::move(left)),
        op(op),
        right(std::move(right)) {}
};

// Unary operators
struct UnaryExpression : Expression {
    enum class Operator {
        Not,
        Negate
    };

    Operator op;
    std::unique_ptr<Expression> operand;

    UnaryExpression(
        Operator op,
        std::unique_ptr<Expression> operand,
        volta::errors::SourceLocation location
    ) : Expression(location),
        op(op),
        operand(std::move(operand)) {}
};

// Call expression: func(args)
struct CallExpression : Expression {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpression(
        std::unique_ptr<Expression> callee,
        std::vector<std::unique_ptr<Expression>> arguments,
        volta::errors::SourceLocation location
    ) : Expression(location),
        callee(std::move(callee)),
        arguments(std::move(arguments)) {}
};

// Index expression: array[index]
struct IndexExpression : Expression {
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;

    IndexExpression(
        std::unique_ptr<Expression> object,
        std::unique_ptr<Expression> index,
        volta::errors::SourceLocation location
    ) : Expression(location),
        object(std::move(object)),
        index(std::move(index)) {}
};

// Slice expression: array[start:end]
struct SliceExpression : Expression {
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> start;  // nullptr for [:end]
    std::unique_ptr<Expression> end;    // nullptr for [start:]

    SliceExpression(
        std::unique_ptr<Expression> object,
        std::unique_ptr<Expression> start,
        std::unique_ptr<Expression> end,
        volta::errors::SourceLocation location
    ) : Expression(location),
        object(std::move(object)),
        start(std::move(start)),
        end(std::move(end)) {}
};

// Member access: object.member
struct MemberExpression : Expression {
    std::unique_ptr<Expression> object;
    std::unique_ptr<IdentifierExpression> member;

    MemberExpression(
        std::unique_ptr<Expression> object,
        std::unique_ptr<IdentifierExpression> member,
        volta::errors::SourceLocation location
    ) : Expression(location),
        object(std::move(object)),
        member(std::move(member)) {}
};

// Method call: object.method(args)
struct MethodCallExpression : Expression {
    std::unique_ptr<Expression> object;
    std::unique_ptr<IdentifierExpression> method;
    std::vector<std::unique_ptr<Expression>> arguments;

    MethodCallExpression(
        std::unique_ptr<Expression> object,
        std::unique_ptr<IdentifierExpression> method,
        std::vector<std::unique_ptr<Expression>> arguments,
        volta::errors::SourceLocation location
    ) : Expression(location),
        object(std::move(object)),
        method(std::move(method)),
        arguments(std::move(arguments)) {}
};

// If expression: if condition { thenExpr } else { elseExpr }
struct IfExpression : Expression {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> thenExpr;
    std::unique_ptr<Expression> elseExpr;

    IfExpression(
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Expression> thenExpr,
        std::unique_ptr<Expression> elseExpr,
        volta::errors::SourceLocation location
    ) : Expression(location),
        condition(std::move(condition)),
        thenExpr(std::move(thenExpr)),
        elseExpr(std::move(elseExpr)) {}
};

// Pattern matching
struct Pattern {
    virtual ~Pattern() = default;
    volta::errors::SourceLocation location;

    Pattern(volta::errors::SourceLocation location)
        : location(location) {}
};

struct LiteralPattern : Pattern {
    std::unique_ptr<Expression> literal;

    LiteralPattern(std::unique_ptr<Expression> literal, volta::errors::SourceLocation location)
        : Pattern(location), literal(std::move(literal)) {}
};

struct IdentifierPattern : Pattern {
    std::unique_ptr<IdentifierExpression> identifier;

    IdentifierPattern(std::unique_ptr<IdentifierExpression> identifier, volta::errors::SourceLocation location)
        : Pattern(location), identifier(std::move(identifier)) {}
};

struct TuplePattern : Pattern {
    std::vector<std::unique_ptr<Pattern>> elements;

    TuplePattern(std::vector<std::unique_ptr<Pattern>> elements, volta::errors::SourceLocation location)
        : Pattern(location), elements(std::move(elements)) {}
};

struct ConstructorPattern : Pattern {
    std::unique_ptr<IdentifierExpression> constructor;
    std::vector<std::unique_ptr<Pattern>> arguments;

    ConstructorPattern(
        std::unique_ptr<IdentifierExpression> constructor,
        std::vector<std::unique_ptr<Pattern>> arguments,
        volta::errors::SourceLocation location
    ) : Pattern(location),
        constructor(std::move(constructor)),
        arguments(std::move(arguments)) {}
};

struct WildcardPattern : Pattern {
    WildcardPattern(volta::errors::SourceLocation location)
        : Pattern(location) {}
};

struct MatchArm {
    std::unique_ptr<Pattern> pattern;
    std::unique_ptr<Expression> guard;  // nullptr if no guard
    std::unique_ptr<Expression> body;

    MatchArm(
        std::unique_ptr<Pattern> pattern,
        std::unique_ptr<Expression> guard,
        std::unique_ptr<Expression> body
    ) : pattern(std::move(pattern)),
        guard(std::move(guard)),
        body(std::move(body)) {}
};

struct MatchExpression : Expression {
    std::unique_ptr<Expression> value;
    std::vector<std::unique_ptr<MatchArm>> arms;

    MatchExpression(
        std::unique_ptr<Expression> value,
        std::vector<std::unique_ptr<MatchArm>> arms,
        volta::errors::SourceLocation location
    ) : Expression(location),
        value(std::move(value)),
        arms(std::move(arms)) {}
};

// Lambda expression
struct Parameter;

struct LambdaExpression : Expression {
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<Expression> body;        // nullptr if block body
    std::unique_ptr<Block> blockBody;        // nullptr if expression body

    LambdaExpression(
        std::vector<std::unique_ptr<Parameter>> parameters,
        std::unique_ptr<Type> returnType,
        std::unique_ptr<Expression> body,
        std::unique_ptr<Block> blockBody,
        volta::errors::SourceLocation location
    ) : Expression(location),
        parameters(std::move(parameters)),
        returnType(std::move(returnType)),
        body(std::move(body)),
        blockBody(std::move(blockBody)) {}
};

struct CastExpression : Expression {
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Type> targetType;

    CastExpression(
        std::unique_ptr<Expression> expression,
        std::unique_ptr<Type> targetType,
        volta::errors::SourceLocation location
    ) : Expression(location),
        expression(std::move(expression)),
        targetType(std::move(targetType)) {}
};

}