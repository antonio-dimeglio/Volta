#include <gtest/gtest.h>
#include "Lexer/Lexer.hpp"
#include "Error/Error.hpp"
#include <sstream>

// Test basic diagnostic reporting
TEST(DiagnosticManagerTest, BasicErrorReporting) {
    DiagnosticManager diag;

    diag.error("Test error message", 10, 5);

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 1);
    EXPECT_EQ(diag.getWarningCount(), 0);
}

TEST(DiagnosticManagerTest, WarningReporting) {
    DiagnosticManager diag;

    diag.warning("Test warning message", 20, 10);

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 0);
    EXPECT_EQ(diag.getWarningCount(), 1);
}

TEST(DiagnosticManagerTest, MixedDiagnostics) {
    DiagnosticManager diag;

    diag.error("First error", 1, 1);
    diag.warning("First warning", 2, 1);
    diag.error("Second error", 3, 1);
    diag.info("Info message", 4, 1);

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 2);
    EXPECT_EQ(diag.getWarningCount(), 1);
}

TEST(DiagnosticManagerTest, Clear) {
    DiagnosticManager diag;

    diag.error("Error", 1, 1);
    diag.warning("Warning", 2, 1);

    EXPECT_TRUE(diag.hasErrors());

    diag.clear();

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 0);
    EXPECT_EQ(diag.getWarningCount(), 0);
}

TEST(DiagnosticManagerTest, PrintOutput) {
    DiagnosticManager diag(false); // Disable colors for testing

    diag.error("Test error", 5, 10);

    std::ostringstream oss;
    diag.printAll(oss, "test.rv");

    std::string output = oss.str();
    EXPECT_NE(output.find("error"), std::string::npos);
    EXPECT_NE(output.find("Test error"), std::string::npos);
    EXPECT_NE(output.find("test.rv:5:10"), std::string::npos);
}

// Test Lexer with DiagnosticManager
TEST(LexerErrorTest, ValidInput) {
    DiagnosticManager diag;
    std::string source = "let x := 42;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_FALSE(diag.hasErrors());
    EXPECT_GT(tokens.size(), 0);
}

TEST(LexerErrorTest, MultipleDecimalPoints) {
    DiagnosticManager diag;
    std::string source = "let x := 3.14.159;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 1);
}

TEST(LexerErrorTest, UnterminatedString) {
    DiagnosticManager diag;
    std::string source = "let x := \"unterminated";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 1);
}

TEST(LexerErrorTest, UnterminatedRawString) {
    DiagnosticManager diag;
    std::string source = "let x := r\"unterminated";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.getErrorCount(), 1);
}

TEST(LexerErrorTest, InvalidSymbol) {
    DiagnosticManager diag;
    std::string source = "let x ! 42;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.getErrorCount(), 1);
}

TEST(LexerErrorTest, UnexpectedCharacter) {
    DiagnosticManager diag;
    std::string source = "let x := @;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.getErrorCount(), 1);
}

// Test comment handling
TEST(LexerCommentTest, SingleLineComment) {
    DiagnosticManager diag;
    std::string source = "let x := 42; // This is a comment\nlet y := 10;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_FALSE(diag.hasErrors());
    // Comments should be ignored, so we shouldn't have any comment tokens
    for (const auto& token : tokens) {
        EXPECT_NE(token.lexeme.find("//"), 0);
    }
}

TEST(LexerCommentTest, MultiLineComment) {
    DiagnosticManager diag;
    std::string source = "let x := 42; /* Multi\nline\ncomment */ let y := 10;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_FALSE(diag.hasErrors());
}

TEST(LexerCommentTest, OnlyComments) {
    DiagnosticManager diag;
    std::string source = "// Just a comment\n/* Another comment */";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_FALSE(diag.hasErrors());
    // Comments are skipped, lexer still produces tokens during scanning
    // The actual token count may vary, but there should be no errors
    EXPECT_GE(tokens.size(), 1); // At least EOF
}

// Test error recovery
TEST(LexerRecoveryTest, MultipleErrors) {
    DiagnosticManager diag;
    std::string source = "let x := 3.14.159; let y := @; let z := !;";
    Lexer lexer(source, diag);

    auto tokens = lexer.tokenize();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.getErrorCount(), 3);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
