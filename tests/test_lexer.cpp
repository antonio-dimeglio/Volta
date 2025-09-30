#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include <gtest/gtest.h>

// Test fixture for Lexer tests
class LexerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LexerTest, SimpleTokens) {
    volta::Lexer lexer("1 + 2");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[0].lexeme, "1");
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);

    EXPECT_EQ(tokens[1].type, volta::TokenType::PLUS);
    EXPECT_EQ(tokens[1].lexeme, "+");

    EXPECT_EQ(tokens[2].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[2].lexeme, "2");

    EXPECT_EQ(tokens[3].type, volta::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, Keywords) {
    volta::Lexer lexer("fn if else while for return");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, volta::TokenType::FUNCTION);
    EXPECT_EQ(tokens[1].type, volta::TokenType::IF);
    EXPECT_EQ(tokens[2].type, volta::TokenType::ELSE);
    EXPECT_EQ(tokens[3].type, volta::TokenType::WHILE);
    EXPECT_EQ(tokens[4].type, volta::TokenType::FOR);
    EXPECT_EQ(tokens[5].type, volta::TokenType::RETURN);
    EXPECT_EQ(tokens[6].type, volta::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, Identifiers) {
    volta::Lexer lexer("x my_var camelCase _private");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "x");
    EXPECT_EQ(tokens[1].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "my_var");
    EXPECT_EQ(tokens[2].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme, "camelCase");
    EXPECT_EQ(tokens[3].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].lexeme, "_private");
}

TEST_F(LexerTest, Strings) {
    volta::Lexer lexer("\"hello world\" \"test\\n\" \"quote: \\\"hi\\\"\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, volta::TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "hello world");
    EXPECT_EQ(tokens[1].type, volta::TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[1].lexeme, "test\n");
    EXPECT_EQ(tokens[2].type, volta::TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme, "quote: \"hi\"");
}

TEST_F(LexerTest, Integers) {
    volta::Lexer lexer("0 42 123 999");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[0].lexeme, "0");
    EXPECT_EQ(tokens[1].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[1].lexeme, "42");
    EXPECT_EQ(tokens[2].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[2].lexeme, "123");
    EXPECT_EQ(tokens[3].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[3].lexeme, "999");
}

TEST_F(LexerTest, RealNumbers) {
    volta::Lexer lexer("3.14 0.5 2.0");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, volta::TokenType::REAL);
    EXPECT_EQ(tokens[0].lexeme, "3.14");
    EXPECT_EQ(tokens[1].type, volta::TokenType::REAL);
    EXPECT_EQ(tokens[1].lexeme, "0.5");
    EXPECT_EQ(tokens[2].type, volta::TokenType::REAL);
    EXPECT_EQ(tokens[2].lexeme, "2.0");
}

TEST_F(LexerTest, ScientificNotation) {
    volta::Lexer lexer("1.5e10 2e-3 3.0e+5");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, volta::TokenType::REAL);
    EXPECT_EQ(tokens[0].lexeme, "1.5e10");
    EXPECT_EQ(tokens[1].type, volta::TokenType::REAL);
    EXPECT_EQ(tokens[1].lexeme, "2e-3");
    EXPECT_EQ(tokens[2].type, volta::TokenType::REAL);
    EXPECT_EQ(tokens[2].lexeme, "3.0e+5");
}

TEST_F(LexerTest, Operators) {
    volta::Lexer lexer("+ - * / % ** == != < > <= >=");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 13);
    EXPECT_EQ(tokens[0].type, volta::TokenType::PLUS);
    EXPECT_EQ(tokens[1].type, volta::TokenType::MINUS);
    EXPECT_EQ(tokens[2].type, volta::TokenType::MULT);
    EXPECT_EQ(tokens[3].type, volta::TokenType::DIV);
    EXPECT_EQ(tokens[4].type, volta::TokenType::MODULO);
    EXPECT_EQ(tokens[5].type, volta::TokenType::POWER);
    EXPECT_EQ(tokens[6].type, volta::TokenType::EQUALS);
    EXPECT_EQ(tokens[7].type, volta::TokenType::NOT_EQUALS);
    EXPECT_EQ(tokens[8].type, volta::TokenType::LT);
    EXPECT_EQ(tokens[9].type, volta::TokenType::GT);
    EXPECT_EQ(tokens[10].type, volta::TokenType::LEQ);
    EXPECT_EQ(tokens[11].type, volta::TokenType::GEQ);
}

TEST_F(LexerTest, AssignmentOperators) {
    volta::Lexer lexer("= := += -= *= /=");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, volta::TokenType::ASSIGN);
    EXPECT_EQ(tokens[1].type, volta::TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, volta::TokenType::PLUS_ASSIGN);
    EXPECT_EQ(tokens[3].type, volta::TokenType::MINUS_ASSIGN);
    EXPECT_EQ(tokens[4].type, volta::TokenType::MULT_ASSIGN);
    EXPECT_EQ(tokens[5].type, volta::TokenType::DIV_ASSIGN);
}

TEST_F(LexerTest, Delimiters) {
    volta::Lexer lexer("( ) [ ] { } , ; : .");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 11);
    EXPECT_EQ(tokens[0].type, volta::TokenType::LPAREN);
    EXPECT_EQ(tokens[1].type, volta::TokenType::RPAREN);
    EXPECT_EQ(tokens[2].type, volta::TokenType::LSQUARE);
    EXPECT_EQ(tokens[3].type, volta::TokenType::RSQUARE);
    EXPECT_EQ(tokens[4].type, volta::TokenType::LBRACE);
    EXPECT_EQ(tokens[5].type, volta::TokenType::RBRACE);
    EXPECT_EQ(tokens[6].type, volta::TokenType::COMMA);
    EXPECT_EQ(tokens[7].type, volta::TokenType::SEMICOLON);
    EXPECT_EQ(tokens[8].type, volta::TokenType::COLON);
    EXPECT_EQ(tokens[9].type, volta::TokenType::DOT);
}

TEST_F(LexerTest, Arrows) {
    volta::Lexer lexer("-> =>");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, volta::TokenType::ARROW);
    EXPECT_EQ(tokens[0].lexeme, "->");
    EXPECT_EQ(tokens[1].type, volta::TokenType::MATCH_ARROW);
    EXPECT_EQ(tokens[1].lexeme, "=>");
}

TEST_F(LexerTest, Comments) {
    volta::Lexer lexer("x := 42 # this is a comment\ny := 10");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7); // x, :=, 42, y, :=, 10, EOF
    EXPECT_EQ(tokens[0].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "x");
    EXPECT_EQ(tokens[1].type, volta::TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, volta::TokenType::INTEGER);
    EXPECT_EQ(tokens[2].lexeme, "42");
    EXPECT_EQ(tokens[3].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].lexeme, "y");
}

TEST_F(LexerTest, LineAndColumnTracking) {
    volta::Lexer lexer("x := 42\ny := 10");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);
    EXPECT_EQ(tokens[1].line, 1);
    EXPECT_EQ(tokens[1].column, 3);
    EXPECT_EQ(tokens[3].line, 2);
    EXPECT_EQ(tokens[3].column, 1);
}

TEST_F(LexerTest, ComplexExpression) {
    volta::Lexer lexer("fn add(a: int, b: int) -> int { return a + b }");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, volta::TokenType::FUNCTION);
    EXPECT_EQ(tokens[1].type, volta::TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "add");
    EXPECT_EQ(tokens[2].type, volta::TokenType::LPAREN);
    EXPECT_EQ(tokens.back().type, volta::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, UnterminatedString) {
    volta::Lexer lexer("\"hello");
    EXPECT_THROW(lexer.tokenize(), std::runtime_error);
}

TEST_F(LexerTest, InvalidScientificNotation) {
    volta::Lexer lexer("1e");
    EXPECT_THROW(lexer.tokenize(), std::runtime_error);
}