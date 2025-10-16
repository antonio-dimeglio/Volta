#include <gtest/gtest.h>
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "HIR/Lowering.hpp"
#include "Parser/AST.hpp"
#include "Error/Error.hpp"

class HIRLoweringTest : public ::testing::Test {
protected:
    DiagnosticManager diag;

    Program lowerSource(const std::string& source) {
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();

        Parser parser(tokens, diag);
        auto ast = parser.parseProgram();

        HIRLowering lowering;
        return lowering.lower(std::move(*ast));
    }
};

// ==================== COMPOUND ASSIGNMENT LOWERING ====================

TEST_F(HIRLoweringTest, PlusEqualLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x += 5; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);

    // Check it's been lowered to x = x + 5
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Plus);

    auto* lhs = dynamic_cast<Variable*>(binExpr->lhs.get());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->token.lexeme, "x");
}

TEST_F(HIRLoweringTest, MinusEqualLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x -= 3; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());

    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Minus);
}

TEST_F(HIRLoweringTest, MultEqualLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x *= 2; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());

    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Mult);
}

TEST_F(HIRLoweringTest, DivEqualLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x /= 2; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());

    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Div);
}

TEST_F(HIRLoweringTest, ModuloEqualLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x %= 3; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());

    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Modulo);
}

// ==================== INCREMENT/DECREMENT LOWERING ====================

TEST_F(HIRLoweringTest, IncrementLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 0; x++; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);

    // Check it's been lowered to x = x + 1
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Plus);

    auto* literal = dynamic_cast<Literal*>(binExpr->rhs.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_EQ(literal->token.lexeme, "1");
}

TEST_F(HIRLoweringTest, DecrementLowering) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x--; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);

    // Check it's been lowered to x = x - 1
    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Minus);

    auto* literal = dynamic_cast<Literal*>(binExpr->rhs.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_EQ(literal->token.lexeme, "1");
}

TEST_F(HIRLoweringTest, MultipleIncrements) {
    auto hir = lowerSource(R"(
        fn test() -> i32 {
            let mut count: i32 = 0;
            count++;
            count++;
            count++;
            return count;
        }
    )");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());

    // Check all three increments are lowered
    for (int i = 1; i <= 3; i++) {
        auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[i].get());
        auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
        ASSERT_NE(assignment, nullptr);

        auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
        EXPECT_EQ(binExpr->op, TokenType::Plus);
    }
}

// ==================== NESTED EXPRESSION LOWERING ====================

TEST_F(HIRLoweringTest, CompoundAssignmentInExpression) {
    auto hir = lowerSource(R"(
        fn test() -> i32 {
            let mut x: i32 = 5;
            let mut y: i32 = 3;
            x += y * 2;
            return x;
        }
    )");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[2].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);

    // x = x + (y * 2)
    auto* outerBinExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
    ASSERT_NE(outerBinExpr, nullptr);
    EXPECT_EQ(outerBinExpr->op, TokenType::Plus);

    // Check the nested multiplication is preserved
    auto* innerBinExpr = dynamic_cast<BinaryExpr*>(outerBinExpr->rhs.get());
    ASSERT_NE(innerBinExpr, nullptr);
    EXPECT_EQ(innerBinExpr->op, TokenType::Mult);
}

// ==================== CONTROL FLOW LOWERING ====================

TEST_F(HIRLoweringTest, CompoundAssignInIfStatement) {
    auto hir = lowerSource(R"(
        fn test() -> void {
            let mut x: i32 = 10;
            if true {
                x += 5;
            }
        }
    )");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* ifStmt = dynamic_cast<IfStmt*>(fnDecl->body[1].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(ifStmt->thenBody[0].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);
}

TEST_F(HIRLoweringTest, IncrementInWhileLoop) {
    auto hir = lowerSource(R"(
        fn test() -> void {
            let mut i: i32 = 0;
            while i < 10 {
                i++;
            }
        }
    )");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* whileStmt = dynamic_cast<WhileStmt*>(fnDecl->body[1].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(whileStmt->thenBody[0].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);

    auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
    EXPECT_EQ(binExpr->op, TokenType::Plus);
}

// ==================== NON-SUGARED CODE PRESERVATION ====================

TEST_F(HIRLoweringTest, SimpleAssignmentUnchanged) {
    auto hir = lowerSource("fn test() -> void { let mut x: i32 = 10; x = 5; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[1].get());
    auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());

    ASSERT_NE(assignment, nullptr);

    // Should be a simple literal, not a binary expression
    auto* literal = dynamic_cast<Literal*>(assignment->value.get());
    ASSERT_NE(literal, nullptr);
    EXPECT_EQ(literal->token.lexeme, "5");
}

TEST_F(HIRLoweringTest, BinaryExpressionUnchanged) {
    auto hir = lowerSource("fn test() -> i32 { return 5 + 3; }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* binExpr = dynamic_cast<BinaryExpr*>(retStmt->value.get());

    ASSERT_NE(binExpr, nullptr);
    EXPECT_EQ(binExpr->op, TokenType::Plus);
}

TEST_F(HIRLoweringTest, FunctionCallUnchanged) {
    auto hir = lowerSource("fn test() -> i32 { return foo(1, 2); }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    auto* retStmt = dynamic_cast<ReturnStmt*>(fnDecl->body[0].get());
    auto* fnCall = dynamic_cast<FnCall*>(retStmt->value.get());

    ASSERT_NE(fnCall, nullptr);
    EXPECT_EQ(fnCall->name, "foo");
    EXPECT_EQ(fnCall->args.size(), 2);
}

// ==================== COMPLEX LOWERING SCENARIOS ====================

TEST_F(HIRLoweringTest, MultipleCompoundAssignments) {
    auto hir = lowerSource(R"(
        fn test() -> i32 {
            let mut x: i32 = 100;
            x += 10;
            x -= 5;
            x *= 2;
            x /= 3;
            x %= 7;
            return x;
        }
    )");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());

    // All compound assignments should be lowered to simple assignments
    TokenType expectedOps[] = {TokenType::Plus, TokenType::Minus, TokenType::Mult, TokenType::Div, TokenType::Modulo};

    for (int i = 0; i < 5; i++) {
        auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[i + 1].get());
        auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
        ASSERT_NE(assignment, nullptr);

        auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
        ASSERT_NE(binExpr, nullptr);
        EXPECT_EQ(binExpr->op, expectedOps[i]);
    }
}

TEST_F(HIRLoweringTest, MixedIncrementDecrement) {
    auto hir = lowerSource(R"(
        fn test() -> i32 {
            let mut counter: i32 = 50;
            counter++;
            counter++;
            counter--;
            counter++;
            counter--;
            counter--;
            return counter;
        }
    )");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());

    TokenType expectedOps[] = {
        TokenType::Plus,  // ++
        TokenType::Plus,  // ++
        TokenType::Minus, // --
        TokenType::Plus,  // ++
        TokenType::Minus, // --
        TokenType::Minus  // --
    };

    for (int i = 0; i < 6; i++) {
        auto* exprStmt = dynamic_cast<ExprStmt*>(fnDecl->body[i + 1].get());
        auto* assignment = dynamic_cast<Assignment*>(exprStmt->expr.get());
        ASSERT_NE(assignment, nullptr);

        auto* binExpr = dynamic_cast<BinaryExpr*>(assignment->value.get());
        ASSERT_NE(binExpr, nullptr);
        EXPECT_EQ(binExpr->op, expectedOps[i]);
    }
}

TEST_F(HIRLoweringTest, NestedControlFlowWithCompounds) {
    auto hir = lowerSource(R"(
        fn test() -> void {
            let mut x: i32 = 0;
            if true {
                x += 1;
                while x < 10 {
                    x++;
                    if x > 5 {
                        x *= 2;
                    }
                }
            }
        }
    )");

    EXPECT_FALSE(diag.hasErrors());

    // Just verify it lowered without errors
    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
}

// ==================== EDGE CASES ====================

TEST_F(HIRLoweringTest, EmptyFunctionUnchanged) {
    auto hir = lowerSource("fn test() -> void { }");

    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    EXPECT_EQ(fnDecl->body.size(), 0);
}

TEST_F(HIRLoweringTest, OnlyNonSugaredCode) {
    auto hir = lowerSource(R"(
        fn test() -> i32 {
            let x: i32 = 5;
            let y: i32 = 10;
            return x + y;
        }
    )");

    EXPECT_FALSE(diag.hasErrors());

    // Should pass through unchanged
    auto* fnDecl = dynamic_cast<FnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
