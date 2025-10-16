#include <gtest/gtest.h>
#include "../include/Lexer/Lexer.hpp"
#include "../include/Parser/Parser.hpp"
#include "../include/HIR/Lowering.hpp"
#include "../include/Semantic/TypeRegistry.hpp"
#include "../include/Semantic/SemanticAnalyzer.hpp"
#include "../include/Error/Error.hpp"
#include <memory>
#include <string>

class SemanticAnalyzerTest : public ::testing::Test {
protected:
    bool analyze(const std::string& source) {
        DiagnosticManager diag;
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();
        if (diag.hasErrors()) return false;
        Parser parser(tokens, diag);
        auto astProgram = parser.parseProgram();
        if (diag.hasErrors()) return false;
        HIRLowering lowering;
        auto hirProgram = lowering.lower(std::move(*astProgram));
        Semantic::TypeRegistry typeRegistry;
        Semantic::SemanticAnalyzer analyzer(typeRegistry, diag);
        analyzer.analyzeProgram(hirProgram);
        return !diag.hasErrors();
    }
    bool hasErrors(const std::string& source) { return !analyze(source); }
};

TEST_F(SemanticAnalyzerTest, SimpleVariableDeclaration) {
    EXPECT_TRUE(analyze("fn test() -> void { let x: i32 = 42; }"));
}

TEST_F(SemanticAnalyzerTest, TypeMismatchInAssignment) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: i32 = true; }"));
}

TEST_F(SemanticAnalyzerTest, ImmutableVariableCannotBeAssigned) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: i32 = 10; x = 20; }"));
}

TEST_F(SemanticAnalyzerTest, MutableVariableCanBeAssigned) {
    EXPECT_TRUE(analyze("fn test() -> void { let mut x: i32 = 10; x = 20; }"));
}

TEST_F(SemanticAnalyzerTest, LogicalOperatorsWithBool) {
    EXPECT_TRUE(analyze("fn test() -> void { let x: bool = true and false; let y: bool = true or false; }"));
}

TEST_F(SemanticAnalyzerTest, LogicalNotOnBoolean) {
    EXPECT_TRUE(analyze("fn test() -> void { let x: bool = not true; }"));
}

TEST_F(SemanticAnalyzerTest, VariableRedeclarationInSameScopeError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: i32 = 10; let x: i32 = 20; }"));
}

TEST_F(SemanticAnalyzerTest, ReturnTypeChecking) {
    EXPECT_TRUE(analyze("fn get_number() -> i32 { return 42; }"));
    EXPECT_TRUE(hasErrors("fn get_number() -> i32 { return true; }"));
}

TEST_F(SemanticAnalyzerTest, FunctionCallTypeChecking) {
    EXPECT_TRUE(analyze(R"(
        fn add(x: i32, y: i32) -> i32 { return x + y; }
        fn test() -> void { let result = add(10, 20); }
    )"));
}

TEST_F(SemanticAnalyzerTest, CompoundAssignmentDesugaring) {
    EXPECT_TRUE(analyze("fn test() -> void { let mut x: i32 = 10; x += 5; x++; }"));
}

TEST_F(SemanticAnalyzerTest, BreakOutsideLoopError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { break; }"));
}

TEST_F(SemanticAnalyzerTest, BreakInsideLoop) {
    EXPECT_TRUE(analyze("fn test() -> void { while (true) { break; } }"));
}

// ==================== ARRAY TESTS ====================

TEST_F(SemanticAnalyzerTest, ArrayTypeInference) {
    EXPECT_TRUE(analyze("fn test() -> void { let x := [1, 2, 3]; }"));
}

TEST_F(SemanticAnalyzerTest, ArrayExplicitTypeMatching) {
    EXPECT_TRUE(analyze("fn test() -> void { let x: [i32; 3] = [1, 2, 3]; }"));
}

TEST_F(SemanticAnalyzerTest, ArrayRepeatSyntax) {
    EXPECT_TRUE(analyze("fn test() -> void { let x: [i32; 5] = [0; 5]; }"));
}

TEST_F(SemanticAnalyzerTest, ArraySizeMismatch) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: [i32; 10] = [1, 2, 3]; }"));
}

TEST_F(SemanticAnalyzerTest, ArrayTypeMismatch) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: [i32; 3] = [1.0, 2.0, 3.0]; }"));
}

TEST_F(SemanticAnalyzerTest, ArrayElementTypeMismatch) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x := [1, 2, true]; }"));
}

TEST_F(SemanticAnalyzerTest, NestedArrayTypes) {
    EXPECT_TRUE(analyze("fn test() -> void { let x: [[i32; 2]; 3] = [[1, 2]; 3]; }"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
