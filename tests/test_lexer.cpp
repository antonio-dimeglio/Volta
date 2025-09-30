#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include <gtest/gtest.h>

using namespace volta::lexer;

// Test fixture for Lexer tests
class LexerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LexerTest, SimpleTokens) {
    Lexer lexer("1 + 2");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].lexeme, "1");
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);

    EXPECT_EQ(tokens[1].type, TokenType::PLUS);
    EXPECT_EQ(tokens[1].lexeme, "+");

    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].lexeme, "2");

    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerTest, Keywords) {
    Lexer lexer("fn if else while for return");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::FUNCTION);
    EXPECT_EQ(tokens[1].type, TokenType::IF);
    EXPECT_EQ(tokens[2].type, TokenType::ELSE);
    EXPECT_EQ(tokens[3].type, TokenType::WHILE);
    EXPECT_EQ(tokens[4].type, TokenType::FOR);
    EXPECT_EQ(tokens[5].type, TokenType::RETURN);
    EXPECT_EQ(tokens[6].type, TokenType::END_OF_FILE);
}

TEST_F(LexerTest, Identifiers) {
    Lexer lexer("x my_var camelCase _private");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "x");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "my_var");
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme, "camelCase");
    EXPECT_EQ(tokens[3].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].lexeme, "_private");
}

TEST_F(LexerTest, Strings) {
    Lexer lexer("\"hello world\" \"test\\n\" \"quote: \\\"hi\\\"\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "hello world");
    EXPECT_EQ(tokens[1].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[1].lexeme, "test\n");
    EXPECT_EQ(tokens[2].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme, "quote: \"hi\"");
}

TEST_F(LexerTest, Integers) {
    Lexer lexer("0 42 123 999");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].lexeme, "0");
    EXPECT_EQ(tokens[1].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[1].lexeme, "42");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].lexeme, "123");
    EXPECT_EQ(tokens[3].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[3].lexeme, "999");
}

TEST_F(LexerTest, RealNumbers) {
    Lexer lexer("3.14 0.5 2.0");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::REAL);
    EXPECT_EQ(tokens[0].lexeme, "3.14");
    EXPECT_EQ(tokens[1].type, TokenType::REAL);
    EXPECT_EQ(tokens[1].lexeme, "0.5");
    EXPECT_EQ(tokens[2].type, TokenType::REAL);
    EXPECT_EQ(tokens[2].lexeme, "2.0");
}

TEST_F(LexerTest, ScientificNotation) {
    Lexer lexer("1.5e10 2e-3 3.0e+5");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::REAL);
    EXPECT_EQ(tokens[0].lexeme, "1.5e10");
    EXPECT_EQ(tokens[1].type, TokenType::REAL);
    EXPECT_EQ(tokens[1].lexeme, "2e-3");
    EXPECT_EQ(tokens[2].type, TokenType::REAL);
    EXPECT_EQ(tokens[2].lexeme, "3.0e+5");
}

TEST_F(LexerTest, Operators) {
    Lexer lexer("+ - * / % ** == != < > <= >=");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 13);
    EXPECT_EQ(tokens[0].type, TokenType::PLUS);
    EXPECT_EQ(tokens[1].type, TokenType::MINUS);
    EXPECT_EQ(tokens[2].type, TokenType::MULT);
    EXPECT_EQ(tokens[3].type, TokenType::DIV);
    EXPECT_EQ(tokens[4].type, TokenType::MODULO);
    EXPECT_EQ(tokens[5].type, TokenType::POWER);
    EXPECT_EQ(tokens[6].type, TokenType::EQUALS);
    EXPECT_EQ(tokens[7].type, TokenType::NOT_EQUALS);
    EXPECT_EQ(tokens[8].type, TokenType::LT);
    EXPECT_EQ(tokens[9].type, TokenType::GT);
    EXPECT_EQ(tokens[10].type, TokenType::LEQ);
    EXPECT_EQ(tokens[11].type, TokenType::GEQ);
}

TEST_F(LexerTest, AssignmentOperators) {
    Lexer lexer("= := += -= *= /=");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::ASSIGN);
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::PLUS_ASSIGN);
    EXPECT_EQ(tokens[3].type, TokenType::MINUS_ASSIGN);
    EXPECT_EQ(tokens[4].type, TokenType::MULT_ASSIGN);
    EXPECT_EQ(tokens[5].type, TokenType::DIV_ASSIGN);
}

TEST_F(LexerTest, Delimiters) {
    Lexer lexer("( ) [ ] { } , ; : .");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 11);
    EXPECT_EQ(tokens[0].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[1].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[2].type, TokenType::LSQUARE);
    EXPECT_EQ(tokens[3].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[4].type, TokenType::LBRACE);
    EXPECT_EQ(tokens[5].type, TokenType::RBRACE);
    EXPECT_EQ(tokens[6].type, TokenType::COMMA);
    EXPECT_EQ(tokens[7].type, TokenType::SEMICOLON);
    EXPECT_EQ(tokens[8].type, TokenType::COLON);
    EXPECT_EQ(tokens[9].type, TokenType::DOT);
}

TEST_F(LexerTest, Arrows) {
    Lexer lexer("-> =>");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::ARROW);
    EXPECT_EQ(tokens[0].lexeme, "->");
    EXPECT_EQ(tokens[1].type, TokenType::MATCH_ARROW);
    EXPECT_EQ(tokens[1].lexeme, "=>");
}

TEST_F(LexerTest, Comments) {
    Lexer lexer("x := 42 # this is a comment\ny := 10");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7); // x, :=, 42, y, :=, 10, EOF
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "x");
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].lexeme, "42");
    EXPECT_EQ(tokens[3].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].lexeme, "y");
}

TEST_F(LexerTest, LineAndColumnTracking) {
    Lexer lexer("x := 42\ny := 10");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);
    EXPECT_EQ(tokens[1].line, 1);
    EXPECT_EQ(tokens[1].column, 3);
    EXPECT_EQ(tokens[3].line, 2);
    EXPECT_EQ(tokens[3].column, 1);
}

TEST_F(LexerTest, ComplexExpression) {
    Lexer lexer("fn add(a: int, b: int) -> int { return a + b }");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, TokenType::FUNCTION);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "add");
    EXPECT_EQ(tokens[2].type, TokenType::LPAREN);
    EXPECT_EQ(tokens.back().type, TokenType::END_OF_FILE);
}

TEST_F(LexerTest, UnterminatedString) {
    Lexer lexer("\"hello");
    auto tokens = lexer.tokenize();

    EXPECT_TRUE(lexer.getErrorReporter().hasErrors());
    EXPECT_EQ(lexer.getErrorReporter().getErrorCount(), 1);
}

TEST_F(LexerTest, InvalidScientificNotation) {
    Lexer lexer("1e");
    auto tokens = lexer.tokenize();

    EXPECT_TRUE(lexer.getErrorReporter().hasErrors());
    EXPECT_EQ(lexer.getErrorReporter().getErrorCount(), 1);
}