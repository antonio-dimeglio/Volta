#include <gtest/gtest.h>
#include "semantic/SemanticAnalyzer.hpp"
#include "semantic/Type.hpp"
#include "parser/Parser.hpp"
#include "lexer/lexer.hpp"
#include "error/ErrorReporter.hpp"
#include <memory>
#include <sstream>

// Don't use 'using namespace' to avoid ambiguity between ast::Type and semantic::Type
namespace semantic = volta::semantic;
namespace ast = volta::ast;
using volta::parser::Parser;
using volta::lexer::Lexer;
using volta::errors::ErrorReporter;

class SemanticAnalyzerTest : public ::testing::Test {
protected:
    std::unique_ptr<ErrorReporter> errorReporter;
    std::unique_ptr<semantic::SemanticAnalyzer> analyzer;

    void SetUp() override {
        errorReporter = std::make_unique<ErrorReporter>();
        analyzer = std::make_unique<semantic::SemanticAnalyzer>(*errorReporter);
    }

    std::unique_ptr<ast::Program> parseProgram(const std::string& source) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        return parser.parseProgram();
    }

    bool analyzeSource(const std::string& source) {
        auto program = parseProgram(source);
        return analyzer->analyze(*program);
    }
};

// ============================================================================
// Type System Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, PrimitiveTypeEquality) {
    auto int1 = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
    auto int2 = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
    auto float1 = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Float);

    EXPECT_TRUE(int1->equals(int2.get()));
    EXPECT_FALSE(int1->equals(float1.get()));
}

TEST_F(SemanticAnalyzerTest, ArrayTypeEquality) {
    auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
    auto floatType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Float);

    auto arrayInt1 = std::make_shared<semantic::ArrayType>(intType);
    auto arrayInt2 = std::make_shared<semantic::ArrayType>(intType);
    auto arrayFloat = std::make_shared<semantic::ArrayType>(floatType);

    EXPECT_TRUE(arrayInt1->equals(arrayInt2.get()));
    EXPECT_FALSE(arrayInt1->equals(arrayFloat.get()));
}

TEST_F(SemanticAnalyzerTest, FunctionTypeEquality) {
    auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
    auto floatType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Float);

    std::vector<std::shared_ptr<semantic::Type>> params1 = {intType, intType};
    std::vector<std::shared_ptr<semantic::Type>> params2 = {intType, floatType};

    auto fn1 = std::make_shared<semantic::FunctionType>(params1, intType);
    auto fn2 = std::make_shared<semantic::FunctionType>(params1, intType);
    auto fn3 = std::make_shared<semantic::FunctionType>(params2, intType);

    EXPECT_TRUE(fn1->equals(fn2.get()));
    EXPECT_FALSE(fn1->equals(fn3.get()));
}

// ============================================================================
// Variable Declaration Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, SimpleVariableDeclaration) {
    EXPECT_TRUE(analyzeSource("x: int = 42"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, TypeInference) {
    EXPECT_TRUE(analyzeSource("x := 42"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, TypeMismatch) {
    EXPECT_FALSE(analyzeSource("x: int = 3.14"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MutableVariable) {
    EXPECT_TRUE(analyzeSource(R"(
        x: mut int = 5
        x = 10
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ImmutableVariableAssignment) {
    EXPECT_FALSE(analyzeSource(R"(
        x: int = 5
        x = 10
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, UndefinedVariable) {
    EXPECT_FALSE(analyzeSource("y = x + 5"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariableRedeclaration) {
    EXPECT_FALSE(analyzeSource(R"(
        x: int = 5
        x: int = 10
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

// ============================================================================
// Function Declaration Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, SimpleFunctionDeclaration) {
    EXPECT_TRUE(analyzeSource(R"(
        fn add(a: int, b: int) -> int {
            return a + b
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionWithExpressionBody) {
    EXPECT_TRUE(analyzeSource(R"(
        fn square(x: int) -> int { return x * x }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionReturnTypeMismatch) {
    EXPECT_FALSE(analyzeSource(R"(
        fn add(a: int, b: int) -> int {
            return 3.14
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionCallWithCorrectArgs) {
    EXPECT_TRUE(analyzeSource(R"(
        fn add(a: int, b: int) -> int {
            return a + b
        }
        result := add(5, 10)
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionCallWrongArgCount) {
    EXPECT_FALSE(analyzeSource(R"(
        fn add(a: int, b: int) -> int {
            return a + b
        }
        result := add(5)
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionCallWrongArgType) {
    EXPECT_FALSE(analyzeSource(R"(
        fn add(a: int, b: int) -> int {
            return a + b
        }
        result := add(5, 3.14)
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ForwardFunctionReference) {
    EXPECT_TRUE(analyzeSource(R"(
        fn main() -> int {
            return helper()
        }
        fn helper() -> int {
            return 42
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, RecursiveFunction) {
    EXPECT_TRUE(analyzeSource(R"(
        fn factorial(n: int) -> int {
            if n <= 1 {
                return 1
            }
            return n * factorial(n - 1)
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Control Flow Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, IfStatementWithBoolCondition) {
    EXPECT_TRUE(analyzeSource(R"(
        x := 5
        if x > 3 {
            y := 10
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, IfStatementNonBoolCondition) {
    EXPECT_FALSE(analyzeSource(R"(
        if 42 {
            y := 10
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, IfElseStatement) {
    EXPECT_TRUE(analyzeSource(R"(
        x := 5
        if x > 3 {
            y := 10
        } else {
            y := 20
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, WhileLoop) {
    EXPECT_TRUE(analyzeSource(R"(
        counter: mut int = 0
        while counter < 10 {
            counter = counter + 1
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, WhileLoopNonBoolCondition) {
    EXPECT_FALSE(analyzeSource(R"(
        while 42 {
            x := 1
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

// TEST_F(SemanticAnalyzerTest, ForLoopWithArray) {
//     EXPECT_TRUE(analyzeSource(R"(
//         numbers := [1, 2, 3, 4, 5]
//         for num in numbers {
//             x := num + 1
//         }
//     )"));
//     EXPECT_FALSE(errorReporter->hasErrors());
// }

TEST_F(SemanticAnalyzerTest, ForLoopWithRange) {
    EXPECT_TRUE(analyzeSource(R"(
        for i in 0..10 {
            x := i * 2
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Expression Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, ArithmeticOperations) {
    EXPECT_TRUE(analyzeSource(R"(
        a := 5 + 3
        b := 10 - 2
        c := 4 * 7
        d := 20 / 4
        e := 17 % 5
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ComparisonOperations) {
    EXPECT_TRUE(analyzeSource(R"(
        a := 5 > 3
        b := 10 < 20
        c := 5 >= 5
        d := 3 <= 10
        e := 5 == 5
        f := 5 != 3
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, LogicalOperations) {
    EXPECT_TRUE(analyzeSource(R"(
        a := true and false
        b := true or false
        c := not true
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, UnaryNegation) {
    EXPECT_TRUE(analyzeSource(R"(
        x := 5
        y := -x
        z := -3.14
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Scope Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, NestedScopes) {
    EXPECT_TRUE(analyzeSource(R"(
        x := 5
        if true {
            y := 10
            z := x + y
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariableOutOfScope) {
    EXPECT_FALSE(analyzeSource(R"(
        if true {
            x := 5
        }
        y := x + 10
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariableShadowing) {
    EXPECT_TRUE(analyzeSource(R"(
        x := 5
        if true {
            x := 10
            y := x
        }
        z := x
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionParameterScope) {
    EXPECT_TRUE(analyzeSource(R"(
        fn test(x: int) -> int {
            y := x + 5
            return y
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Return Statement Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, ReturnOutsideFunction) {
    EXPECT_FALSE(analyzeSource("return 42"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ReturnCorrectType) {
    EXPECT_TRUE(analyzeSource(R"(
        fn getValue() -> int {
            return 42
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ReturnWrongType) {
    EXPECT_FALSE(analyzeSource(R"(
        fn getValue() -> int {
            return "hello"
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

// ============================================================================
// Struct Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, StructDeclaration) {
    EXPECT_TRUE(analyzeSource(R"(
        struct Point {
            x: float,
            y: float
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Array Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, ArrayLiteral) {
    EXPECT_TRUE(analyzeSource(R"(
        numbers := [1, 2, 3, 4, 5]
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EmptyArray) {
    EXPECT_TRUE(analyzeSource(R"(
        empty: Array[int] = []
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Complex Integration Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, ComplexProgramWithMultipleFunctions) {
    EXPECT_TRUE(analyzeSource(R"(
        fn fibonacci(n: int) -> int {
            if n <= 1 {
                return n
            }
            return fibonacci(n - 1) + fibonacci(n - 2)
        }

        fn main() -> int {
            result := fibonacci(10)
            return result
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariablesAndFunctionsInteraction) {
    EXPECT_TRUE(analyzeSource(R"(
        fn square(x: int) -> int {
            return x * x
        }

        fn sumOfSquares(a: int, b: int) -> int {
            return square(a) + square(b)
        }

        result := sumOfSquares(3, 4)
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// TEST_F(SemanticAnalyzerTest, NestedControlFlow) {
//     EXPECT_TRUE(analyzeSource(R"(
//         fn findMax(arr: Array[int]) -> int {
//             max: mut int = 0
//             for num in arr {
//                 if num > max {
//                     max = num
//                 }
//             }
//             return max
//         }
//     )"));
//     EXPECT_FALSE(errorReporter->hasErrors());
// }
