#include <gtest/gtest.h>
#include "../include/Lexer/Lexer.hpp"
#include "../include/Parser/Parser.hpp"
#include "../include/HIR/Lowering.hpp"
#include "../include/Type/TypeRegistry.hpp"
#include "../include/Semantic/SemanticAnalyzer.hpp"
#include "../include/Error/Error.hpp"
#include <memory>
#include <string>

class SemanticAnalyzerTest : public ::testing::Test {
protected:
    bool analyze(const std::string& source) {
        Type::TypeRegistry typeRegistry;
        DiagnosticManager diag;
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();
        if (diag.hasErrors()) return false;
        Parser parser(tokens, typeRegistry, diag);
        auto astProgram = parser.parseProgram();
        if (diag.hasErrors()) return false;
        HIRLowering lowering(typeRegistry);
        auto hirProgram = lowering.lower(std::move(*astProgram));
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

// ==================== EDGE CASES ====================

TEST_F(SemanticAnalyzerTest, UndefinedVariableError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: i32 = y; }"));
}

TEST_F(SemanticAnalyzerTest, UndefinedFunctionError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x := foo(1, 2); }"));
}

TEST_F(SemanticAnalyzerTest, FunctionArgumentCountMismatch) {
    EXPECT_TRUE(hasErrors(R"(
        fn add(x: i32, y: i32) -> i32 { return x + y; }
        fn test() -> void { let result := add(1); }
    )"));
}

TEST_F(SemanticAnalyzerTest, FunctionArgumentTypeMismatch) {
    EXPECT_TRUE(hasErrors(R"(
        fn add(x: i32, y: i32) -> i32 { return x + y; }
        fn test() -> void { let result := add(true, false); }
    )"));
}

TEST_F(SemanticAnalyzerTest, ArithmeticOnBoolError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: bool = true + false; }"));
}

TEST_F(SemanticAnalyzerTest, LogicalOnIntError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { let x: i32 = 1 and 2; }"));
}

TEST_F(SemanticAnalyzerTest, IfConditionMustBeBoolean) {
    EXPECT_TRUE(hasErrors("fn test() -> void { if (42) { } }"));
}

TEST_F(SemanticAnalyzerTest, WhileConditionMustBeBoolean) {
    EXPECT_TRUE(hasErrors("fn test() -> void { while (42) { } }"));
}

TEST_F(SemanticAnalyzerTest, ContinueOutsideLoopError) {
    EXPECT_TRUE(hasErrors("fn test() -> void { continue; }"));
}

TEST_F(SemanticAnalyzerTest, ContinueInsideLoop) {
    EXPECT_TRUE(analyze("fn test() -> void { while (true) { continue; } }"));
}

TEST_F(SemanticAnalyzerTest, NestedScopes) {
    EXPECT_TRUE(analyze(R"(
        fn test() -> void {
            let x: i32 = 10;
            if (true) {
                let x: i32 = 20;
            }
            let y: i32 = x;
        }
    )"));
}

TEST_F(SemanticAnalyzerTest, ArrayIndexingWithInteger) {
    EXPECT_TRUE(analyze("fn test() -> void { let arr: [i32; 5] = [1, 2, 3, 4, 5]; let x := arr[2]; }"));
}

TEST_F(SemanticAnalyzerTest, MultiDimensionalArrayFlattened) {
    EXPECT_TRUE(analyze(R"(
        fn test() -> void {
            let matrix: [[i32; 2]; 2] = [[1, 2], [3, 4]];
            let x := matrix[0][1];
        }
    )"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
