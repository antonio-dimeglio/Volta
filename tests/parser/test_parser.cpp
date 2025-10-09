#include "parser/Parser.hpp"
#include "lexer/lexer.hpp"
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "ast/Type.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace volta::parser;
using namespace volta::lexer;
using namespace volta::ast;

// Test fixture for Parser tests
class ParserTest : public ::testing::Test {
protected:
    std::unique_ptr<Parser> createParser(const std::string& source) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        return std::make_unique<Parser>(std::move(tokens));
    }
};

// ============================================================================
// Program Tests
// ============================================================================

TEST_F(ParserTest, ParseEmptyProgram) {
    auto parser = createParser("");
    auto program = parser->parseProgram();
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 0);
}

TEST_F(ParserTest, ParseSimpleProgram) {
    auto parser = createParser("let x := 42");
    auto program = parser->parseProgram();
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 1);
}

// ============================================================================
// Expression Tests
// ============================================================================

TEST_F(ParserTest, ParseIntegerLiteral) {
    auto parser = createParser("42");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* literal = dynamic_cast<IntegerLiteral*>(expr.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_EQ(literal->value, 42);
}

TEST_F(ParserTest, ParseFloatLiteral) {
    auto parser = createParser("3.14");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* literal = dynamic_cast<FloatLiteral*>(expr.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_DOUBLE_EQ(literal->value, 3.14);
}

TEST_F(ParserTest, ParseStringLiteral) {
    auto parser = createParser("\"hello world\"");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* literal = dynamic_cast<StringLiteral*>(expr.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_EQ(literal->value, "hello world");
}

TEST_F(ParserTest, ParseBooleanLiteral) {
    auto parser = createParser("true");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* literal = dynamic_cast<BooleanLiteral*>(expr.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_EQ(literal->value, true);
}

TEST_F(ParserTest, ParseIdentifier) {
    auto parser = createParser("variable_name");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* ident = dynamic_cast<IdentifierExpression*>(expr.get());
    ASSERT_NE(ident, nullptr);
    EXPECT_EQ(ident->name, "variable_name");
}

TEST_F(ParserTest, ParseBinaryExpression) {
    auto parser = createParser("1 + 2");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* binExpr = dynamic_cast<BinaryExpression*>(expr.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, BinaryExpression::Operator::Add);
}

TEST_F(ParserTest, ParseComplexBinaryExpression) {
    auto parser = createParser("1 + 2 * 3 - 4 / 2");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    // Just verify it parses without error
}

TEST_F(ParserTest, ParseUnaryExpression) {
    auto parser = createParser("-42");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* unaryExpr = dynamic_cast<UnaryExpression*>(expr.get());
    ASSERT_NE(unaryExpr, nullptr);
    EXPECT_EQ(unaryExpr->op, UnaryExpression::Operator::Negate);
}

TEST_F(ParserTest, ParseLogicalExpression) {
    auto parser = createParser("true and false or true");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    // Verify it parses as binary expression
    auto* binExpr = dynamic_cast<BinaryExpression*>(expr.get());
    ASSERT_NE(binExpr, nullptr);
}

TEST_F(ParserTest, ParseComparisonExpression) {
    auto parser = createParser("x < 10");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* binExpr = dynamic_cast<BinaryExpression*>(expr.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, BinaryExpression::Operator::Less);
}

TEST_F(ParserTest, ParseRangeExpression) {
    auto parser = createParser("1..10");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* binExpr = dynamic_cast<BinaryExpression*>(expr.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, BinaryExpression::Operator::Range);
}

TEST_F(ParserTest, ParseInclusiveRangeExpression) {
    auto parser = createParser("1..=10");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* binExpr = dynamic_cast<BinaryExpression*>(expr.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, BinaryExpression::Operator::RangeInclusive);
}

TEST_F(ParserTest, ParseCallExpression) {
    auto parser = createParser("func(1, 2, 3)");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* callExpr = dynamic_cast<CallExpression*>(expr.get());
    ASSERT_NE(callExpr, nullptr);
    EXPECT_EQ(callExpr->arguments.size(), 3);
}

TEST_F(ParserTest, ParseIndexExpression) {
    auto parser = createParser("arr[0]");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* indexExpr = dynamic_cast<IndexExpression*>(expr.get());
    ASSERT_NE(indexExpr, nullptr);
}

TEST_F(ParserTest, ParseSliceExpression) {
    auto parser = createParser("arr[1:5]");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* sliceExpr = dynamic_cast<SliceExpression*>(expr.get());
    ASSERT_NE(sliceExpr, nullptr);
}

TEST_F(ParserTest, ParseMemberExpression) {
    auto parser = createParser("obj.field");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* memberExpr = dynamic_cast<MemberExpression*>(expr.get());
    ASSERT_NE(memberExpr, nullptr);
}

TEST_F(ParserTest, ParseMethodCallExpression) {
    auto parser = createParser("obj.method(1, 2)");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* methodCall = dynamic_cast<MethodCallExpression*>(expr.get());
    ASSERT_NE(methodCall, nullptr);
    EXPECT_EQ(methodCall->arguments.size(), 2);
}

TEST_F(ParserTest, ParseArrayLiteral) {
    auto parser = createParser("[1, 2, 3, 4, 5]");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* arrayLit = dynamic_cast<ArrayLiteral*>(expr.get());
    ASSERT_NE(arrayLit, nullptr);
    EXPECT_EQ(arrayLit->elements.size(), 5);
}

TEST_F(ParserTest, ParseTupleLiteral) {
    auto parser = createParser("(1, 2, 3)");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* tupleLit = dynamic_cast<TupleLiteral*>(expr.get());
    ASSERT_NE(tupleLit, nullptr);
    EXPECT_EQ(tupleLit->elements.size(), 3);
}

TEST_F(ParserTest, ParseStructLiteral) {
    auto parser = createParser("Point { x: 10, y: 20 }");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
    auto* structLit = dynamic_cast<StructLiteral*>(expr.get());
    ASSERT_NE(structLit, nullptr);
    EXPECT_EQ(structLit->fields.size(), 2);
}

// ============================================================================
// Statement Tests
// ============================================================================

TEST_F(ParserTest, ParseExpressionStatement) {
    auto parser = createParser("x + 1");
    auto stmt = parser->parseExpressionStatement();
    ASSERT_NE(stmt, nullptr);
    EXPECT_NE(stmt->expr, nullptr);
}

TEST_F(ParserTest, ParseBlock) {
    auto parser = createParser("{ let x := 1\n let y := 2 }");
    auto block = parser->parseBlock();
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->statements.size(), 2);
}

TEST_F(ParserTest, ParseIfStatement) {
    auto parser = createParser("if x > 0 { let x := 1 }");
    // Consume 'if' token first since parseIfStatement expects it consumed
    auto program = createParser("if x > 0 { let x := 1 }");
    auto stmt = program->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* ifStmt = dynamic_cast<IfStatement*>(stmt.get());
    ASSERT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->condition, nullptr);
    EXPECT_NE(ifStmt->thenBlock, nullptr);
}

TEST_F(ParserTest, ParseIfElseStatement) {
    auto parser = createParser("if x > 0 { let x := 1 } else { let x := 0 }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* ifStmt = dynamic_cast<IfStatement*>(stmt.get());
    ASSERT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->elseBlock, nullptr);
}

TEST_F(ParserTest, ParseIfElseIfElseStatement) {
    auto parser = createParser("if x > 0 { let x := 1 } else if x < 0 { let x := 2 } else { let x := 0 }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* ifStmt = dynamic_cast<IfStatement*>(stmt.get());
    ASSERT_NE(ifStmt, nullptr);
    EXPECT_EQ(ifStmt->elseIfClauses.size(), 1);
    EXPECT_NE(ifStmt->elseBlock, nullptr);
}

TEST_F(ParserTest, ParseWhileStatement) {
    auto parser = createParser("while x > 0 { let x := x - 1 }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* whileStmt = dynamic_cast<WhileStatement*>(stmt.get());
    ASSERT_NE(whileStmt, nullptr);
    EXPECT_NE(whileStmt->condition, nullptr);
}

TEST_F(ParserTest, ParseForStatement) {
    auto parser = createParser("for i in 0..10 { let x := x + i }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* forStmt = dynamic_cast<ForStatement*>(stmt.get());
    ASSERT_NE(forStmt, nullptr);
    EXPECT_EQ(forStmt->identifier, "i");
}

TEST_F(ParserTest, ParseReturnStatement) {
    auto parser = createParser("return 42");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* retStmt = dynamic_cast<ReturnStatement*>(stmt.get());
    ASSERT_NE(retStmt, nullptr);
    EXPECT_NE(retStmt->expression, nullptr);
}

TEST_F(ParserTest, ParseReturnStatementEmpty) {
    auto parser = createParser("return }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* retStmt = dynamic_cast<ReturnStatement*>(stmt.get());
    ASSERT_NE(retStmt, nullptr);
    EXPECT_EQ(retStmt->expression, nullptr);
}

TEST_F(ParserTest, ParseImportStatement) {
    auto parser = createParser("import math");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* importStmt = dynamic_cast<ImportStatement*>(stmt.get());
    ASSERT_NE(importStmt, nullptr);
    EXPECT_EQ(importStmt->identifier, "math");
}

// ============================================================================
// Declaration Tests
// ============================================================================

TEST_F(ParserTest, ParseVarDeclarationWithType) {
    auto parser = createParser("let x: int = 42");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* decl = dynamic_cast<VarDeclaration*>(stmt.get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->identifier, "x");
    EXPECT_NE(decl->typeAnnotation, nullptr);
    EXPECT_NE(decl->initializer, nullptr);
    EXPECT_FALSE(decl->isMutable);
}

TEST_F(ParserTest, ParseVarDeclarationInferred) {
    auto parser = createParser("let x := 42");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* decl = dynamic_cast<VarDeclaration*>(stmt.get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->identifier, "x");
    EXPECT_EQ(decl->typeAnnotation, nullptr);
    EXPECT_NE(decl->initializer, nullptr);
    EXPECT_FALSE(decl->isMutable);
}

TEST_F(ParserTest, ParseVarDeclarationMutable) {
    auto parser = createParser("let mut x: int = 42");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* decl = dynamic_cast<VarDeclaration*>(stmt.get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->identifier, "x");
    EXPECT_NE(decl->typeAnnotation, nullptr);
    EXPECT_TRUE(decl->isMutable);
}

TEST_F(ParserTest, ParseFnDeclarationSimple) {
    auto parser = createParser("fn add(a: int, b: int) -> int { return a + b }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* fnDecl = dynamic_cast<FnDeclaration*>(stmt.get());
    ASSERT_NE(fnDecl, nullptr);
    EXPECT_EQ(fnDecl->identifier, "add");
    EXPECT_EQ(fnDecl->parameters.size(), 2);
    EXPECT_NE(fnDecl->returnType, nullptr);
    EXPECT_NE(fnDecl->body, nullptr);
}

TEST_F(ParserTest, ParseStructDeclaration) {
    auto parser = createParser("struct Point { x: int, y: int }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* structDecl = dynamic_cast<StructDeclaration*>(stmt.get());
    ASSERT_NE(structDecl, nullptr);
    EXPECT_EQ(structDecl->identifier, "Point");
    EXPECT_EQ(structDecl->fields.size(), 2);
}

TEST_F(ParserTest, ParseStructDeclarationEmpty) {
    auto parser = createParser("struct Empty { }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* structDecl = dynamic_cast<StructDeclaration*>(stmt.get());
    ASSERT_NE(structDecl, nullptr);
    EXPECT_EQ(structDecl->identifier, "Empty");
    EXPECT_EQ(structDecl->fields.size(), 0);
}

// ============================================================================
// Enum Declaration Tests
// ============================================================================

TEST_F(ParserTest, ParseEnumDeclarationSimple) {
    auto parser = createParser("enum Color { Red, Green, Blue }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* enumDecl = dynamic_cast<EnumDeclaration*>(stmt.get());
    ASSERT_NE(enumDecl, nullptr);
    EXPECT_EQ(enumDecl->name, "Color");
    EXPECT_EQ(enumDecl->typeParams.size(), 0);
    EXPECT_EQ(enumDecl->variants.size(), 3);
    EXPECT_EQ(enumDecl->variants[0]->name, "Red");
    EXPECT_EQ(enumDecl->variants[1]->name, "Green");
    EXPECT_EQ(enumDecl->variants[2]->name, "Blue");
    EXPECT_EQ(enumDecl->variants[0]->associatedTypes.size(), 0);
}

TEST_F(ParserTest, ParseEnumDeclarationWithData) {
    auto parser = createParser("enum Option { Some(int), None }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* enumDecl = dynamic_cast<EnumDeclaration*>(stmt.get());
    ASSERT_NE(enumDecl, nullptr);
    EXPECT_EQ(enumDecl->name, "Option");
    EXPECT_EQ(enumDecl->typeParams.size(), 0);
    EXPECT_EQ(enumDecl->variants.size(), 2);
    EXPECT_EQ(enumDecl->variants[0]->name, "Some");
    EXPECT_EQ(enumDecl->variants[0]->associatedTypes.size(), 1);
    EXPECT_EQ(enumDecl->variants[1]->name, "None");
    EXPECT_EQ(enumDecl->variants[1]->associatedTypes.size(), 0);
}

TEST_F(ParserTest, ParseEnumDeclarationGeneric) {
    auto parser = createParser("enum Option[T] { Some(T), None }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* enumDecl = dynamic_cast<EnumDeclaration*>(stmt.get());
    ASSERT_NE(enumDecl, nullptr);
    EXPECT_EQ(enumDecl->name, "Option");
    EXPECT_EQ(enumDecl->typeParams.size(), 1);
    EXPECT_EQ(enumDecl->typeParams[0], "T");
    EXPECT_EQ(enumDecl->variants.size(), 2);
    EXPECT_EQ(enumDecl->variants[0]->name, "Some");
    EXPECT_EQ(enumDecl->variants[0]->associatedTypes.size(), 1);
    EXPECT_EQ(enumDecl->variants[1]->name, "None");
}

TEST_F(ParserTest, ParseEnumDeclarationGenericMultipleParams) {
    auto parser = createParser("enum Result[T, E] { Ok(T), Err(E) }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* enumDecl = dynamic_cast<EnumDeclaration*>(stmt.get());
    ASSERT_NE(enumDecl, nullptr);
    EXPECT_EQ(enumDecl->name, "Result");
    EXPECT_EQ(enumDecl->typeParams.size(), 2);
    EXPECT_EQ(enumDecl->typeParams[0], "T");
    EXPECT_EQ(enumDecl->typeParams[1], "E");
    EXPECT_EQ(enumDecl->variants.size(), 2);
    EXPECT_EQ(enumDecl->variants[0]->name, "Ok");
    EXPECT_EQ(enumDecl->variants[1]->name, "Err");
}

TEST_F(ParserTest, ParseEnumDeclarationVariantMultipleTypes) {
    auto parser = createParser("enum Message { Move(int, int), Write(str) }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* enumDecl = dynamic_cast<EnumDeclaration*>(stmt.get());
    ASSERT_NE(enumDecl, nullptr);
    EXPECT_EQ(enumDecl->name, "Message");
    EXPECT_EQ(enumDecl->variants.size(), 2);
    EXPECT_EQ(enumDecl->variants[0]->name, "Move");
    EXPECT_EQ(enumDecl->variants[0]->associatedTypes.size(), 2);
    EXPECT_EQ(enumDecl->variants[1]->name, "Write");
    EXPECT_EQ(enumDecl->variants[1]->associatedTypes.size(), 1);
}

// ============================================================================
// Type Tests
// ============================================================================

TEST_F(ParserTest, ParsePrimitiveType) {
    auto parser = createParser("i32");
    auto type = parser->parseType();
    ASSERT_NE(type, nullptr);
    auto* primType = dynamic_cast<PrimitiveType*>(type.get());
    ASSERT_NE(primType, nullptr);
    EXPECT_EQ(primType->kind, PrimitiveType::Kind::I32);
}

TEST_F(ParserTest, ParseArrayType) {
    auto parser = createParser("Array[int]");
    auto type = parser->parseType();
    ASSERT_NE(type, nullptr);
    auto* compType = dynamic_cast<CompoundType*>(type.get());
    ASSERT_NE(compType, nullptr);
    EXPECT_EQ(compType->kind, CompoundType::Kind::Array);
}

TEST_F(ParserTest, ParseTupleType) {
    auto parser = createParser("(int, str, bool)");
    auto type = parser->parseType();
    ASSERT_NE(type, nullptr);
    auto* compType = dynamic_cast<CompoundType*>(type.get());
    ASSERT_NE(compType, nullptr);
    EXPECT_EQ(compType->kind, CompoundType::Kind::Tuple);
}

TEST_F(ParserTest, ParseFunctionType) {
    auto parser = createParser("fn(int, int) -> int");
    auto type = parser->parseType();
    ASSERT_NE(type, nullptr);
    auto* fnType = dynamic_cast<FunctionType*>(type.get());
    ASSERT_NE(fnType, nullptr);
    EXPECT_EQ(fnType->paramTypes.size(), 2);
}

TEST_F(ParserTest, ParseGenericType) {
    auto parser = createParser("Map[str, int]");
    auto type = parser->parseType();
    ASSERT_NE(type, nullptr);
    auto* genType = dynamic_cast<GenericType*>(type.get());
    ASSERT_NE(genType, nullptr);
}

TEST_F(ParserTest, ParseTypeAnnotation) {
    auto parser = createParser("mut int");
    auto annot = parser->parseTypeAnnotation();
    ASSERT_NE(annot, nullptr);
    EXPECT_TRUE(annot->isMutable);
}

// ============================================================================
// Pattern Tests
// ============================================================================

TEST_F(ParserTest, ParseLiteralPattern) {
    auto parser = createParser("42");
    auto pattern = parser->parsePattern();
    ASSERT_NE(pattern, nullptr);
    auto* litPattern = dynamic_cast<LiteralPattern*>(pattern.get());
    ASSERT_NE(litPattern, nullptr);
}

TEST_F(ParserTest, ParseIdentifierPattern) {
    auto parser = createParser("x");
    auto pattern = parser->parsePattern();
    ASSERT_NE(pattern, nullptr);
    auto* identPattern = dynamic_cast<IdentifierPattern*>(pattern.get());
    ASSERT_NE(identPattern, nullptr);
}

TEST_F(ParserTest, ParseWildcardPattern) {
    auto parser = createParser("_");
    auto pattern = parser->parsePattern();
    ASSERT_NE(pattern, nullptr);
    auto* wildcardPattern = dynamic_cast<WildcardPattern*>(pattern.get());
    ASSERT_NE(wildcardPattern, nullptr);
}

TEST_F(ParserTest, ParseTuplePattern) {
    auto parser = createParser("(x, y, z)");
    auto pattern = parser->parsePattern();
    ASSERT_NE(pattern, nullptr);
    auto* tuplePattern = dynamic_cast<TuplePattern*>(pattern.get());
    ASSERT_NE(tuplePattern, nullptr);
    EXPECT_EQ(tuplePattern->elements.size(), 3);
}

TEST_F(ParserTest, ParseConstructorPattern) {
    auto parser = createParser("Some(x)");
    auto pattern = parser->parsePattern();
    ASSERT_NE(pattern, nullptr);
    auto* ctorPattern = dynamic_cast<ConstructorPattern*>(pattern.get());
    ASSERT_NE(ctorPattern, nullptr);
}

// ============================================================================
// Complex Tests
// ============================================================================

TEST_F(ParserTest, ParseComplexProgram) {
    auto parser = createParser(R"(
        fn factorial(n: int) -> int {
            if n <= 1 {
                return 1
            } else {
                return n * factorial(n - 1)
            }
        }

        let x := factorial(5)
    )");
    auto program = parser->parseProgram();
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 2);
}

TEST_F(ParserTest, ParseNestedExpressions) {
    auto parser = createParser("((1 + 2) * (3 - 4)) / 5");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseChainedMethodCalls) {
    auto parser = createParser("obj.method1().method2().method3()");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

// ============================================================================
// Additional Feature Tests (May fail - features not yet implemented)
// ============================================================================

TEST_F(ParserTest, ParseMatchExpression) {
    auto parser = createParser(R"(
        match value {
            Some(x) => x,
            None => 0
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchExpressionMultiplePatterns) {
    auto parser = createParser(R"(
        match result {
            0 => "zero",
            1 => "one",
            _ => "other"
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseLambdaExpression) {
    auto parser = createParser("fn(x: int) -> int = x + 1");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseLambdaExpressionMultipleParams) {
    auto parser = createParser("fn(x: int, y: int) -> int = x + y");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseLambdaExpressionNoParams) {
    auto parser = createParser("fn() -> int = 42");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseSingleExpressionFunction) {
    auto parser = createParser("fn square(x: float) -> float = x * x");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseSingleExpressionFunctionNoReturn) {
    auto parser = createParser("fn increment(x: int) -> int = x + 1");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseOptionTypeSome) {
    auto parser = createParser("Some(42)");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseOptionTypeNone) {
    auto parser = createParser("None");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseOptionTypeInVariable) {
    auto parser = createParser("let value: Option[int] = Some(10)");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseTypeAlias) {
    auto parser = createParser("type Vector = Array[float]");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseTypeAliasTuple) {
    auto parser = createParser("type Point = (float, float)");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseTypeAliasFunction) {
    auto parser = createParser("type Callback = fn(int) -> int");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseGenericFunction) {
    auto parser = createParser("fn first[T](arr: Array[T]) -> T { return arr[0] }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseGenericFunctionMultipleParams) {
    auto parser = createParser("fn map[T, U](arr: Array[T], f: fn(T) -> U) -> Array[U] { }");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseGenericStruct) {
    auto parser = createParser(R"(
        struct Box[T] {
            value: T
        }
    )");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseArraySlicingBasic) {
    auto parser = createParser("arr[1:3]");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseArraySlicingOpenEnd) {
    auto parser = createParser("arr[1:]");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseArraySlicingOpenStart) {
    auto parser = createParser("arr[:3]");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseNestedGenerics) {
    auto parser = createParser("let value: Array[Array[int]] = [[1, 2], [3, 4]]");
    auto stmt = parser->parseStatement();
    ASSERT_NE(stmt, nullptr);
}

TEST_F(ParserTest, ParseComplexMatchWithGuards) {
    auto parser = createParser(R"(
        match pair {
            (x, y) if x > y => x,
            (x, y) => y
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseStructInstantiationNested) {
    auto parser = createParser("Person { name: \"Alice\", address: Address { street: \"Main St\" } }");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

// ============================================================================
// Match Statement Tests - Exhaustiveness
// ============================================================================

TEST_F(ParserTest, ParseMatchWithEnumExhaustive) {
    auto parser = createParser(R"(
        match color {
            Red => "red",
            Green => "green",
            Blue => "blue"
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithEnumAndWildcard) {
    auto parser = createParser(R"(
        match status {
            Active => 1,
            _ => 0
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithEnumVariantData) {
    auto parser = createParser(R"(
        match result {
            Ok(value) => value,
            Err(msg) => 0
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithNestedEnums) {
    auto parser = createParser(R"(
        match outer {
            Some(Ok(x)) => x,
            Some(Err(_)) => -1,
            None => 0
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithBoolExhaustive) {
    auto parser = createParser(R"(
        match is_valid {
            true => "yes",
            false => "no"
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithIntegerRanges) {
    auto parser = createParser(R"(
        match value {
            0 => "zero",
            1..10 => "small",
            _ => "large"
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithTuplePattern) {
    auto parser = createParser(R"(
        match point {
            (0, 0) => "origin",
            (x, 0) => "x-axis",
            (0, y) => "y-axis",
            (x, y) => "other"
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}

TEST_F(ParserTest, ParseMatchWithMultipleVariants) {
    auto parser = createParser(R"(
        match shape {
            Circle(r) => r * r,
            Rectangle(w, h) => w * h,
            Triangle(b, h) => b * h / 2
        }
    )");
    auto expr = parser->parseExpression();
    ASSERT_NE(expr, nullptr);
}