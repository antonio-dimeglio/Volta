#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include <gtest/gtest.h>

using namespace volta::lexer;

/**
 * Tests for Missing Tokenization Features
 *
 * These tests are currently EXPECTED TO FAIL.
 * They document what needs to be implemented in the lexer.
 *
 * Run with: make test
 *
 * As you implement each feature, these tests should start passing!
 */

class LexerMissingFeaturesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// 1. STRING LITERALS
// ============================================================================

TEST_F(LexerMissingFeaturesTest, StringLiteral_Simple) {
    Lexer lexer("\"hello world\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2) << "Expected string literal + EOF";
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "hello world"); // Without quotes
    EXPECT_EQ(tokens[1].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, StringLiteral_Empty) {
    Lexer lexer("\"\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "");
    EXPECT_EQ(tokens[1].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, StringLiteral_WithEscapes) {
    Lexer lexer("\"hello\\nworld\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    // Lexer should process escape sequences
    EXPECT_EQ(tokens[0].lexeme, "hello\nworld");
}

TEST_F(LexerMissingFeaturesTest, StringLiteral_CommonEscapes) {
    // Test: \n \t \r \\ \"
    Lexer lexer("\"line1\\nline2\\ttab\\r\\\\backslash\\\"quote\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "line1\nline2\ttab\r\\backslash\"quote");
}

TEST_F(LexerMissingFeaturesTest, StringLiteral_InExpression) {
    Lexer lexer("name := \"Alice\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "name");
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme, "Alice");
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, StringLiteral_Multiple) {
    Lexer lexer("\"first\" + \"second\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme, "first");
    EXPECT_EQ(tokens[1].type, TokenType::PLUS);
    EXPECT_EQ(tokens[2].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme, "second");
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

// ============================================================================
// 2. MULTI-LINE COMMENTS
// ============================================================================

TEST_F(LexerMissingFeaturesTest, MultiLineComment_Simple) {
    Lexer lexer("#[\nThis is a comment\n]#");
    auto tokens = lexer.tokenize();

    // Comment should be ignored, only EOF
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, MultiLineComment_WithCode) {
    Lexer lexer("x := 5 #[ comment here ]# y := 10");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // x
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 5
    EXPECT_EQ(tokens[3].type, TokenType::IDENTIFIER); // y
    EXPECT_EQ(tokens[4].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[5].type, TokenType::INTEGER); // 10
    EXPECT_EQ(tokens[6].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, MultiLineComment_MultipleLines) {
    std::string code = "#[\n"
                       "Line 1 of comment\n"
                       "Line 2 of comment\n"
                       "Line 3 of comment\n"
                       "]#\n"
                       "x := 42";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // x
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 42
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, MultiLineComment_Nested) {
    // Volta spec doesn't specify if nested comments are supported
    // This test assumes they are NOT nested (simpler implementation)
    Lexer lexer("#[ outer #[ inner ]# still outer ]# code");
    auto tokens = lexer.tokenize();

    // Should have: "still outer ]# code" then EOF
    // Or if nested: just "code" then EOF
    // For now, assume non-nested (simpler)
    ASSERT_GE(tokens.size(), 1);
    EXPECT_EQ(tokens[tokens.size()-1].type, TokenType::END_OF_FILE);
}

// ============================================================================
// 3. POWER OPERATOR (**)
// ============================================================================

TEST_F(LexerMissingFeaturesTest, PowerOperator_Simple) {
    Lexer lexer("2 ** 3");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER); // 2
    EXPECT_EQ(tokens[1].type, TokenType::POWER);
    EXPECT_EQ(tokens[1].lexeme, "**");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 3
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, PowerOperator_Expression) {
    Lexer lexer("x**2 + y**3");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // x
    EXPECT_EQ(tokens[1].type, TokenType::POWER);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 2
    EXPECT_EQ(tokens[3].type, TokenType::PLUS);
    EXPECT_EQ(tokens[4].type, TokenType::IDENTIFIER); // y
    EXPECT_EQ(tokens[5].type, TokenType::POWER);
    EXPECT_EQ(tokens[6].type, TokenType::INTEGER); // 3
    EXPECT_EQ(tokens[7].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, PowerOperator_NotConfusedWithMult) {
    // ** should be POWER, not two MULT tokens
    Lexer lexer("2**3");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[1].type, TokenType::POWER);
    EXPECT_NE(tokens[1].type, TokenType::MULT);
}

// ============================================================================
// 4. COMPOUND ASSIGNMENT OPERATORS (+=, -=, *=, /=)
// ============================================================================

TEST_F(LexerMissingFeaturesTest, CompoundAssign_PlusEquals) {
    Lexer lexer("x += 5");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // x
    EXPECT_EQ(tokens[1].type, TokenType::PLUS_ASSIGN);
    EXPECT_EQ(tokens[1].lexeme, "+=");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 5
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, CompoundAssign_MinusEquals) {
    Lexer lexer("count -= 1");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::MINUS_ASSIGN);
    EXPECT_EQ(tokens[1].lexeme, "-=");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, CompoundAssign_MultEquals) {
    Lexer lexer("value *= 2");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::MULT_ASSIGN);
    EXPECT_EQ(tokens[1].lexeme, "*=");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, CompoundAssign_DivEquals) {
    Lexer lexer("result /= 10");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::DIV_ASSIGN);
    EXPECT_EQ(tokens[1].lexeme, "/=");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, CompoundAssign_NotConfusedWithSeparateOps) {
    // += should be one token, not + and =
    Lexer lexer("x+=5");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[1].type, TokenType::PLUS_ASSIGN);
    EXPECT_NE(tokens[1].type, TokenType::PLUS);
}

// ============================================================================
// 5. RANGE OPERATORS (.., ..=)
// ============================================================================

TEST_F(LexerMissingFeaturesTest, RangeOperator_Exclusive) {
    Lexer lexer("0..10");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER); // 0
    EXPECT_EQ(tokens[1].type, TokenType::RANGE);
    EXPECT_EQ(tokens[1].lexeme, "..");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 10
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, RangeOperator_Inclusive) {
    Lexer lexer("1..=100");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER); // 1
    EXPECT_EQ(tokens[1].type, TokenType::INCLUSIVE_RANGE);
    EXPECT_EQ(tokens[1].lexeme, "..=");
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 100
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, RangeOperator_WithVariables) {
    Lexer lexer("start..end");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // start
    EXPECT_EQ(tokens[1].type, TokenType::RANGE);
    EXPECT_EQ(tokens[2].type, TokenType::IDENTIFIER); // end
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, RangeOperator_InForLoop) {
    Lexer lexer("for i in 0..10");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::FOR);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER); // i
    EXPECT_EQ(tokens[2].type, TokenType::IN);
    EXPECT_EQ(tokens[3].type, TokenType::INTEGER); // 0
    EXPECT_EQ(tokens[4].type, TokenType::RANGE);
    EXPECT_EQ(tokens[5].type, TokenType::INTEGER); // 10
    EXPECT_EQ(tokens[6].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, RangeOperator_NotConfusedWithDots) {
    // .. should be RANGE, not two DOT tokens
    Lexer lexer("0..5");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[1].type, TokenType::RANGE);
    EXPECT_NE(tokens[1].type, TokenType::DOT);
}

// ============================================================================
// 6. ARRAY LITERAL SYNTAX ([, ])
// ============================================================================

TEST_F(LexerMissingFeaturesTest, ArrayLiteral_Empty) {
    Lexer lexer("[]");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::LSQUARE);
    EXPECT_EQ(tokens[0].lexeme, "[");
    EXPECT_EQ(tokens[1].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[1].lexeme, "]");
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, ArrayLiteral_Simple) {
    Lexer lexer("[1, 2, 3]");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].type, TokenType::LSQUARE);
    EXPECT_EQ(tokens[1].type, TokenType::INTEGER); // 1
    EXPECT_EQ(tokens[2].type, TokenType::COMMA);
    EXPECT_EQ(tokens[3].type, TokenType::INTEGER); // 2
    EXPECT_EQ(tokens[4].type, TokenType::COMMA);
    EXPECT_EQ(tokens[5].type, TokenType::INTEGER); // 3
    EXPECT_EQ(tokens[6].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[7].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, ArrayLiteral_WithSpaces) {
    Lexer lexer("[ 1 , 2 , 3 ]");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].type, TokenType::LSQUARE);
    EXPECT_EQ(tokens[1].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].type, TokenType::COMMA);
    EXPECT_EQ(tokens[3].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[4].type, TokenType::COMMA);
    EXPECT_EQ(tokens[5].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[6].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[7].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, ArrayLiteral_Assignment) {
    Lexer lexer("numbers := [10, 20, 30]");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 10);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // numbers
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::LSQUARE);
    // ... elements ...
    EXPECT_EQ(tokens[8].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[9].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, ArrayIndexing) {
    Lexer lexer("arr[0]");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER); // arr
    EXPECT_EQ(tokens[1].type, TokenType::LSQUARE);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER); // 0
    EXPECT_EQ(tokens[3].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[4].type, TokenType::END_OF_FILE);
}

// ============================================================================
// 7. NEW KEYWORDS (match, struct, import, Some, None, type)
// ============================================================================

TEST_F(LexerMissingFeaturesTest, Keyword_Match) {
    Lexer lexer("match x");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::MATCH);
    EXPECT_EQ(tokens[0].lexeme, "match");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER); // x
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, Keyword_Struct) {
    Lexer lexer("struct Point");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::STRUCT);
    EXPECT_EQ(tokens[0].lexeme, "struct");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER); // Point
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, Keyword_Import) {
    Lexer lexer("import math");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::IMPORT);
    EXPECT_EQ(tokens[0].lexeme, "import");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER); // math
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

// Removed: Some and None are now enum variants, not keywords
// They will be tested as part of enum functionality

TEST_F(LexerMissingFeaturesTest, Keyword_Enum) {
    Lexer lexer("enum Option");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::ENUM);
    EXPECT_EQ(tokens[0].lexeme, "enum");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER); // Option
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, Keyword_Type) {
    Lexer lexer("type Vector");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::TYPE);
    EXPECT_EQ(tokens[0].lexeme, "type");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, Keywords_AllNew) {
    Lexer lexer("match struct import enum type");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TokenType::MATCH);
    EXPECT_EQ(tokens[1].type, TokenType::STRUCT);
    EXPECT_EQ(tokens[2].type, TokenType::IMPORT);
    EXPECT_EQ(tokens[3].type, TokenType::ENUM);
    EXPECT_EQ(tokens[4].type, TokenType::TYPE);
    EXPECT_EQ(tokens[5].type, TokenType::END_OF_FILE);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(LexerMissingFeaturesTest, Integration_StringsAndArrays) {
    Lexer lexer("names := [\"Alice\", \"Bob\", \"Charlie\"]");
    auto tokens = lexer.tokenize();

    // names := [ "Alice" , "Bob" , "Charlie" ] EOF
    ASSERT_EQ(tokens.size(), 10);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::INFER_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::LSQUARE);
    EXPECT_EQ(tokens[3].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[4].type, TokenType::COMMA);
    EXPECT_EQ(tokens[5].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[6].type, TokenType::COMMA);
    EXPECT_EQ(tokens[7].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[8].type, TokenType::RSQUARE);
    EXPECT_EQ(tokens[9].type, TokenType::END_OF_FILE);
}

TEST_F(LexerMissingFeaturesTest, Integration_RangeWithPower) {
    Lexer lexer("for i in 0..10 { x := i**2 }");
    auto tokens = lexer.tokenize();

    // Should properly tokenize range and power
    bool hasRange = false;
    bool hasPower = false;

    for (const auto& token : tokens) {
        if (token.type == TokenType::RANGE) hasRange = true;
        if (token.type == TokenType::POWER) hasPower = true;
    }

    EXPECT_TRUE(hasRange) << "Should tokenize .. as RANGE";
    EXPECT_TRUE(hasPower) << "Should tokenize ** as POWER";
}

TEST_F(LexerMissingFeaturesTest, Integration_MatchWithOption) {
    Lexer lexer("match result { Some(x) => x, None => 0 }");
    auto tokens = lexer.tokenize();

    // Should have MATCH keyword, Some and None are now regular identifiers (enum variants)
    bool hasMatch = false;
    bool hasSome = false;
    bool hasNone = false;

    for (const auto& token : tokens) {
        if (token.type == TokenType::MATCH) hasMatch = true;
        if (token.type == TokenType::IDENTIFIER && token.lexeme == "Some") hasSome = true;
        if (token.type == TokenType::IDENTIFIER && token.lexeme == "None") hasNone = true;
    }

    EXPECT_TRUE(hasMatch) << "Should have MATCH keyword";
    EXPECT_TRUE(hasSome) << "Should have Some identifier (enum variant)";
    EXPECT_TRUE(hasNone) << "Should have None identifier (enum variant)";
}

TEST_F(LexerMissingFeaturesTest, Integration_EnumDeclaration) {
    Lexer lexer("enum Option { Some(int), None }");
    auto tokens = lexer.tokenize();

    // Check for enum keyword and basic structure
    bool hasEnum = false;
    bool hasOption = false;
    bool hasSome = false;
    bool hasNone = false;

    for (const auto& token : tokens) {
        if (token.type == TokenType::ENUM) hasEnum = true;
        if (token.type == TokenType::IDENTIFIER && token.lexeme == "Option") hasOption = true;
        if (token.type == TokenType::IDENTIFIER && token.lexeme == "Some") hasSome = true;
        if (token.type == TokenType::IDENTIFIER && token.lexeme == "None") hasNone = true;
    }

    EXPECT_TRUE(hasEnum) << "Should have ENUM keyword";
    EXPECT_TRUE(hasOption) << "Should have Option identifier";
    EXPECT_TRUE(hasSome) << "Should have Some identifier (variant)";
    EXPECT_TRUE(hasNone) << "Should have None identifier (variant)";
}

TEST_F(LexerMissingFeaturesTest, Integration_CompoundAssignWithComment) {
    Lexer lexer("counter += 1 #[ increment counter ]#");
    auto tokens = lexer.tokenize();

    // Comment should be ignored
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, TokenType::PLUS_ASSIGN);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[3].type, TokenType::END_OF_FILE);
}
