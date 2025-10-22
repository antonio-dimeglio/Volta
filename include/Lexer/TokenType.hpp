#pragma once 

#include <string>

#define TOKEN_TYPES(X) \
    /* Literals */ \
    X(Integer) \
    X(Float) \
    X(True_) \
    X(False_) \
    X(String) \
    /* Arithmetic */ \
    X(Plus) \
    X(Minus) \
    X(Mult) \
    X(Div) \
    X(Modulo) \
    X(Increment) \
    X(Decrement) \
    /* Comparison */ \
    X(EqualEqual) \
    X(NotEqual) \
    X(LessThan) \
    X(LessEqual) \
    X(GreaterThan) \
    X(GreaterEqual) \
    /* Logical */ \
    X(And) \
    X(Or) \
    X(Not) \
    /* Assignment */ \
    X(Assign) \
    X(InferAssign) \
    X(PlusEqual) \
    X(MinusEqual) \
    X(MultEqual) \
    X(DivEqual) \
    X(ModuloEqual) \
    /* Special */ \
    X(Identifier) \
    /* Range */ \
    X(Range) \
    X(InclusiveRange) \
    /* Keywords */ \
    X(Let) \
    X(Mut) \
    X(Ref) \
    X(Function) \
    X(Return) \
    X(If) \
    X(Else) \
    X(While) \
    X(For) \
    X(In) \
    X(Break) \
    X(Continue) \
    X(Match) \
    X(Struct) \
    X(Import) \
    X(As) \
    X(Type) \
    X(Enum) \
    X(Extern) \
    X(Opaque) \
    X(PtrKeyword) \
    X(Self_) \
    X(Impl) \
    X(Varargs) \
    X(Pub) \
    /* Delimiters */ \
    X(LParen) \
    X(RParen) \
    X(LSquare) \
    X(RSquare) \
    X(LBrace) \
    X(RBrace) \
    /* Misc */ \
    X(Colon) \
    X(DoubleColon) \
    X(Semicolon) \
    X(Arrow) \
    X(FatArrow) \
    X(Dot) \
    X(Comma) \
    /* EOF */ \
    X(EndOfFile) \
    /* Internal */ \
    X(Dummy)

enum class TokenType {
#define X(name) name,
    TOKEN_TYPES(X)
#undef X
};

inline const std::string tokenTypeToString(TokenType type) {
    switch (type) {
#define X(name) case TokenType::name: return std::string(#name);
        TOKEN_TYPES(X)
#undef X
        default: return std::string("Unknown");
    }
}