#include <gtest/gtest.h>
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Parser/AST.hpp"
#include "Type/TypeRegistry.hpp"
#include "Error/Error.hpp"

class ParserTest : public ::testing::Test {
protected:
    DiagnosticManager diag;
    Type::TypeRegistry typeRegistry;

    std::unique_ptr<Program> parse(const std::string& source) {
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();

        if (diag.hasErrors()) {
            return nullptr;
        }

        Parser parser(tokens, typeRegistry, diag);
        return parser.parseProgram();
    }
};

// ==================== BASIC FUNCTION TESTS ====================

TEST_F(ParserTest, SimpleFunctionDeclaration) {
    auto program = parse("fn foo() -> i32 { return 42; }");

    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->statements.size(), 1);

    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    EXPECT_EQ(fnDecl->name, "foo");
    EXPECT_EQ(fnDecl->params.size(), 0);
    EXPECT_EQ(fnDecl->body.size(), 1);
}

TEST_F(ParserTest, FunctionWithParameters) {
    auto program = parse("fn add(a: i32, b: i32) -> i32 { return a + b; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    EXPECT_EQ(fnDecl->params.size(), 2);
    EXPECT_EQ(fnDecl->params[0].name, "a");
    EXPECT_EQ(fnDecl->params[1].name, "b");
}

TEST_F(ParserTest, FunctionWithRefParameters) {
    auto program = parse("fn modify(ref x: i32, mut ref y: i32) -> void { }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    EXPECT_TRUE(fnDecl->params[0].isRef);
    EXPECT_FALSE(fnDecl->params[0].isMutRef);

    EXPECT_FALSE(fnDecl->params[1].isRef);
    EXPECT_TRUE(fnDecl->params[1].isMutRef);
}

TEST_F(ParserTest, FunctionWithoutReturnType) {
    auto program = parse("fn noreturn() { let x: i32 = 5; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    // Should default to void
    auto* primType = dynamic_cast<PrimitiveType*>(fnDecl->returnType.get());
    ASSERT_NE(primType, nullptr);
    EXPECT_EQ(primType->kind, PrimitiveTypeKind::Void);
}

// ==================== VARIABLE DECLARATION TESTS ====================

TEST_F(ParserTest, ImmutableVariableDeclaration) {
    auto program = parse("fn test() -> void { let x: i32 = 10; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* varDecl = dynamic_cast<VarDecl*>(fnDecl->body[0].get());

    ASSERT_NE(varDecl, nullptr);
    EXPECT_FALSE(varDecl->mutable_);
    EXPECT_EQ(varDecl->name.lexeme, "x");
}

TEST_F(ParserTest, MutableVariableDeclaration) {
    auto program = parse("fn test() -> void { let mut x: i32 = 10; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* varDecl = dynamic_cast<VarDecl*>(fnDecl->body[0].get());

    ASSERT_NE(varDecl, nullptr);
    EXPECT_TRUE(varDecl->mutable_);
}

TEST_F(ParserTest, VariableWithoutInitializer) {
    auto program = parse("fn test() -> void { let x: i32; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* varDecl = dynamic_cast<VarDecl*>(fnDecl->body[0].get());

    ASSERT_NE(varDecl, nullptr);
    EXPECT_EQ(varDecl->init_value, nullptr);
}

// ==================== COMPOUND ASSIGNMENT TESTS ====================

TEST_F(ParserTest, CompoundAdditionAssignment) {
    auto program = parse("fn test() -> void { let mut x: i32 = 10; x += 5; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    ASSERT_NE(exprStmt, nullptr);

    auto* compAssign = dynamic_cast<CompoundAssign*>(exprStmt->expr.get());
    ASSERT_NE(compAssign, nullptr);
    EXPECT_EQ(compAssign->op, TokenType::PlusEqual);
}

TEST_F(ParserTest, IncrementExpression) {
    auto program = parse("fn test() -> void { let mut x: i32 = 0; x++; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    ASSERT_NE(exprStmt, nullptr);

    auto* inc = dynamic_cast<Increment*>(exprStmt->expr.get());
    ASSERT_NE(inc, nullptr);
    EXPECT_EQ(inc->var->token.lexeme, "x");
}

TEST_F(ParserTest, DecrementExpression) {
    auto program = parse("fn test() -> void { let mut x: i32 = 10; x--; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    ASSERT_NE(exprStmt, nullptr);

    auto* dec = dynamic_cast<Decrement*>(exprStmt->expr.get());
    ASSERT_NE(dec, nullptr);
    EXPECT_EQ(dec->var->token.lexeme, "x");
}

TEST_F(ParserTest, AllCompoundAssignments) {
    auto program = parse(R"(
        fn test() -> void {
            let mut x: i32 = 10;
            x += 1;
            x -= 2;
            x *= 3;
            x /= 4;
            x %= 5;
        }
    )");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());

    // Check each compound assignment
    auto* comp1 = dynamic_cast<CompoundAssign*>(
        dynamic_cast<ExprStmt*>(fnDecl->body[1].get())->expr.get());
    EXPECT_EQ(comp1->op, TokenType::PlusEqual);

    auto* comp2 = dynamic_cast<CompoundAssign*>(
        dynamic_cast<ExprStmt*>(fnDecl->body[2].get())->expr.get());
    EXPECT_EQ(comp2->op, TokenType::MinusEqual);

    auto* comp3 = dynamic_cast<CompoundAssign*>(
        dynamic_cast<ExprStmt*>(fnDecl->body[3].get())->expr.get());
    EXPECT_EQ(comp3->op, TokenType::MultEqual);

    auto* comp4 = dynamic_cast<CompoundAssign*>(
        dynamic_cast<ExprStmt*>(fnDecl->body[4].get())->expr.get());
    EXPECT_EQ(comp4->op, TokenType::DivEqual);

    auto* comp5 = dynamic_cast<CompoundAssign*>(
        dynamic_cast<ExprStmt*>(fnDecl->body[5].get())->expr.get());
    EXPECT_EQ(comp5->op, TokenType::ModuloEqual);
}

// ==================== CONTROL FLOW TESTS ====================

TEST_F(ParserTest, IfStatement) {
    auto program = parse("fn test() -> void { if true { return; } }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* ifStmt = dynamic_cast<IfStmt*>(fnDecl->body[0].get());

    ASSERT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->condition, nullptr);
    EXPECT_FALSE(ifStmt->thenBody.empty());
    EXPECT_TRUE(ifStmt->elseBody.empty());
}

TEST_F(ParserTest, IfElseStatement) {
    auto program = parse("fn test() -> i32 { if true { return 1; } else { return 0; } }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* ifStmt = dynamic_cast<IfStmt*>(fnDecl->body[0].get());

    ASSERT_NE(ifStmt, nullptr);
    EXPECT_FALSE(ifStmt->thenBody.empty());
    EXPECT_FALSE(ifStmt->elseBody.empty());
}

TEST_F(ParserTest, WhileLoop) {
    auto program = parse("fn test() -> void { while true { break; } }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* whileStmt = dynamic_cast<WhileStmt*>(fnDecl->body[0].get());

    ASSERT_NE(whileStmt, nullptr);
    EXPECT_NE(whileStmt->condition, nullptr);
    EXPECT_FALSE(whileStmt->thenBody.empty());
}

TEST_F(ParserTest, BreakStatement) {
    auto program = parse("fn test() -> void { while true { break; } }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* whileStmt = dynamic_cast<WhileStmt*>(fnDecl->body[0].get());
    auto* breakStmt = dynamic_cast<BreakStmt*>(whileStmt->thenBody[0].get());

    ASSERT_NE(breakStmt, nullptr);
}

TEST_F(ParserTest, ContinueStatement) {
    auto program = parse("fn test() -> void { while true { continue; } }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* whileStmt = dynamic_cast<WhileStmt*>(fnDecl->body[0].get());
    auto* continueStmt = dynamic_cast<ContinueStmt*>(whileStmt->thenBody[0].get());

    ASSERT_NE(continueStmt, nullptr);
}

// ==================== EXPRESSION TESTS ====================

TEST_F(ParserTest, BinaryExpression) {
    auto program = parse("fn test() -> i32 { return 5 + 3; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* binExpr = dynamic_cast<BinaryExpr*>(retStmt->value.get());

    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Plus);
}

TEST_F(ParserTest, UnaryExpression) {
    auto program = parse("fn test() -> i32 { return -5; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* unExpr = dynamic_cast<UnaryExpr*>(retStmt->value.get());

    ASSERT_NE(unExpr, nullptr);
    EXPECT_EQ(unExpr->op, TokenType::Minus);
}

TEST_F(ParserTest, FunctionCall) {
    auto program = parse("fn test() -> i32 { return foo(1, 2, 3); }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* fnCall = dynamic_cast<FnCall*>(retStmt->value.get());

    ASSERT_NE(fnCall, nullptr);
    EXPECT_EQ(fnCall->name, "foo");
    EXPECT_EQ(fnCall->args.size(), 3);
}

TEST_F(ParserTest, ArrayLiteralElements) {
    auto program = parse("fn test() -> void { let arr: i32 = [1, 2, 3]; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* varDecl = dynamic_cast<VarDecl*>(fnDecl->body[0].get());
    auto* arrLit = dynamic_cast<ArrayLiteral*>(varDecl->init_value.get());

    ASSERT_NE(arrLit, nullptr);
    EXPECT_EQ(arrLit->elements.size(), 3);
    EXPECT_EQ(arrLit->repeat_value, nullptr);
}

TEST_F(ParserTest, ArrayLiteralRepeat) {
    auto program = parse("fn test() -> void { let arr: i32 = [0; 5]; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* varDecl = dynamic_cast<VarDecl*>(fnDecl->body[0].get());
    auto* arrLit = dynamic_cast<ArrayLiteral*>(varDecl->init_value.get());

    ASSERT_NE(arrLit, nullptr);
    EXPECT_EQ(arrLit->elements.size(), 0);
    EXPECT_NE(arrLit->repeat_value, nullptr);
    EXPECT_EQ(*arrLit->repeat_count, 5);
}

// ==================== OPERATOR PRECEDENCE TESTS ====================

TEST_F(ParserTest, OperatorPrecedenceMultiplication) {
    auto program = parse("fn test() -> i32 { return 2 + 3 * 4; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* addExpr = dynamic_cast<BinaryExpr*>(retStmt->value.get());

    ASSERT_NE(addExpr, nullptr);
    EXPECT_EQ(addExpr->op, TokenType::Plus);

    // Right side should be multiplication
    auto* multExpr = dynamic_cast<BinaryExpr*>(addExpr->rhs.get());
    ASSERT_NE(multExpr, nullptr);
    EXPECT_EQ(multExpr->op, TokenType::Mult);
}

TEST_F(ParserTest, OperatorPrecedenceComparison) {
    auto program = parse("fn test() -> bool { return 5 + 3 == 8; }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* cmpExpr = dynamic_cast<BinaryExpr*>(retStmt->value.get());

    ASSERT_NE(cmpExpr, nullptr);
    EXPECT_EQ(cmpExpr->op, TokenType::EqualEqual);

    // Left side should be addition
    auto* addExpr = dynamic_cast<BinaryExpr*>(cmpExpr->lhs.get());
    ASSERT_NE(addExpr, nullptr);
    EXPECT_EQ(addExpr->op, TokenType::Plus);
}

// ==================== EDGE CASE TESTS ====================

TEST_F(ParserTest, EmptyFunction) {
    auto program = parse("fn empty() -> void { }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    EXPECT_EQ(fnDecl->body.size(), 0);
}

TEST_F(ParserTest, NestedFunctionCalls) {
    auto program = parse("fn test() -> i32 { return foo(bar(baz())); }");

    ASSERT_NE(program, nullptr);
    auto* fnDecl = dynamic_cast<FnDecl*>(program->statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* outerCall = dynamic_cast<FnCall*>(retStmt->value.get());

    ASSERT_NE(outerCall, nullptr);
    EXPECT_EQ(outerCall->name, "foo");

    auto* middleCall = dynamic_cast<FnCall*>(outerCall->args[0].get());
    ASSERT_NE(middleCall, nullptr);
    EXPECT_EQ(middleCall->name, "bar");

    auto* innerCall = dynamic_cast<FnCall*>(middleCall->args[0].get());
    ASSERT_NE(innerCall, nullptr);
    EXPECT_EQ(innerCall->name, "baz");
}

TEST_F(ParserTest, ComplexExpression) {
    auto program = parse("fn test() -> i32 { return (2 + 3) * (4 - 1); }");

    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
