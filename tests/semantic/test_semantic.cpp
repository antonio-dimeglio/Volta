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
    auto int1 = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::I32);
    auto int2 = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::I32);
    auto float1 = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::F32);

    EXPECT_TRUE(int1->equals(int2.get()));
    EXPECT_FALSE(int1->equals(float1.get()));
}

TEST_F(SemanticAnalyzerTest, ArrayTypeEquality) {
    auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::I32);
    auto floatType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::F32);

    auto arrayInt1 = std::make_shared<semantic::ArrayType>(intType);
    auto arrayInt2 = std::make_shared<semantic::ArrayType>(intType);
    auto arrayFloat = std::make_shared<semantic::ArrayType>(floatType);

    EXPECT_TRUE(arrayInt1->equals(arrayInt2.get()));
    EXPECT_FALSE(arrayInt1->equals(arrayFloat.get()));
}

TEST_F(SemanticAnalyzerTest, FunctionTypeEquality) {
    auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::I32);
    auto floatType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::F32);

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
    EXPECT_TRUE(analyzeSource("let x: i32 = 42"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, TypeInference) {
    EXPECT_TRUE(analyzeSource("let x := 42"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, TypeMismatch) {
    // With implicit numeric conversions, f32 -> i32 is allowed
    EXPECT_TRUE(analyzeSource("let x: i32 = 3.14"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MutableVariable) {
    EXPECT_TRUE(analyzeSource(R"(
        let mut x: i32 = 5
        x = 10
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ImmutableVariableAssignment) {
    EXPECT_FALSE(analyzeSource(R"(
        let x: i32 = 5
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
        let x: i32 = 5
        let x: i32 = 10
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

// ============================================================================
// Function Declaration Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, SimpleFunctionDeclaration) {
    EXPECT_TRUE(analyzeSource(R"(
        fn add(a: i32, b: i32) -> i32 {
            return a + b
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionWithExpressionBody) {
    EXPECT_TRUE(analyzeSource(R"(
        fn square(x: i32) -> i32 = x * x
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionReturnTypeMismatch) {
    // With implicit numeric conversions, f32 -> i32 is allowed
    EXPECT_TRUE(analyzeSource(R"(
        fn add(a: i32, b: i32) -> i32 {
            return 3.14
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionCallWithCorrectArgs) {
    EXPECT_TRUE(analyzeSource(R"(
        fn add(a: i32, b: i32) -> i32 {
            return a + b
        }
        let result := add(5, 10)
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionCallWrongArgCount) {
    EXPECT_FALSE(analyzeSource(R"(
        fn add(a: i32, b: i32) -> i32 {
            return a + b
        }
        let result := add(5)
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionCallWrongArgType) {
    EXPECT_FALSE(analyzeSource(R"(
        fn add(a: i32, b: i32) -> i32 {
            return a + b
        }
        let result := add(5, 3.14)
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ForwardFunctionReference) {
    EXPECT_TRUE(analyzeSource(R"(
        fn main() -> i32 {
            return helper()
        }
        fn helper() -> i32 {
            return 42
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, RecursiveFunction) {
    EXPECT_TRUE(analyzeSource(R"(
        fn factorial(n: i32) -> i32 {
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
        let x := 5
        if x > 3 {
            let y := 10
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, IfStatementNonBoolCondition) {
    EXPECT_FALSE(analyzeSource(R"(
        if 42 {
            let y := 10
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, IfElseStatement) {
    EXPECT_TRUE(analyzeSource(R"(
        let x := 5
        if x > 3 {
            let y := 10
        } else {
            let y := 20
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, WhileLoop) {
    EXPECT_TRUE(analyzeSource(R"(
        let mut counter: i32 = 0
        while counter < 10 {
            counter = counter + 1
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, WhileLoopNonBoolCondition) {
    EXPECT_FALSE(analyzeSource(R"(
        while 42 {
            let x := 1
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ForLoopWithArray) {
    EXPECT_TRUE(analyzeSource(R"(
        let numbers := [1, 2, 3, 4, 5]
        for num in numbers {
            let x := num + 1
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ForLoopWithRange) {
    EXPECT_TRUE(analyzeSource(R"(
        for i in 0..10 {
            let x := i * 2
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Expression Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, ArithmeticOperations) {
    EXPECT_TRUE(analyzeSource(R"(
        let a := 5 + 3
        let b := 10 - 2
        let c := 4 * 7
        let d := 20 / 4
        let e := 17 % 5
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ComparisonOperations) {
    EXPECT_TRUE(analyzeSource(R"(
        let a := 5 > 3
        let b := 10 < 20
        let c := 5 >= 5
        let d := 3 <= 10
        let e := 5 == 5
        let f := 5 != 3
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, LogicalOperations) {
    EXPECT_TRUE(analyzeSource(R"(
        let a := true and false
        let b := true or false
        let c := not true
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, UnaryNegation) {
    EXPECT_TRUE(analyzeSource(R"(
        let x := 5
        let y := -x
        let z := -3.14
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Scope Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, NestedScopes) {
    EXPECT_TRUE(analyzeSource(R"(
        let x := 5
        if true {
            let y := 10
            let z := x + y
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariableOutOfScope) {
    EXPECT_FALSE(analyzeSource(R"(
        if true {
            let x := 5
        }
        let y := x + 10
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariableShadowing) {
    EXPECT_TRUE(analyzeSource(R"(
        let x := 5
        if true {
            let x := 10
            let y := x
        }
        let z := x
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, FunctionParameterScope) {
    EXPECT_TRUE(analyzeSource(R"(
        fn test(x: i32) -> i32 {
            let y := x + 5
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
        fn getValue() -> i32 {
            return 42
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, ReturnWrongType) {
    EXPECT_FALSE(analyzeSource(R"(
        fn getValue() -> i32 {
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
        let numbers := [1, 2, 3, 4, 5]
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EmptyArray) {
    EXPECT_TRUE(analyzeSource(R"(
        let empty: Array[i32] = []
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Complex Integration Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, ComplexProgramWithMultipleFunctions) {
    EXPECT_TRUE(analyzeSource(R"(
        fn fibonacci(n: i32) -> i32 {
            if n <= 1 {
                return n
            }
            return fibonacci(n - 1) + fibonacci(n - 2)
        }

        fn main() -> i32 {
            let result := fibonacci(10)
            return result
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, VariablesAndFunctionsInteraction) {
    EXPECT_TRUE(analyzeSource(R"(
        fn square(x: i32) -> i32 {
            return x * x
        }

        fn sumOfSquares(a: i32, b: i32) -> i32 {
            return square(a) + square(b)
        }

        let result := sumOfSquares(3, 4)
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, NestedControlFlow) {
    EXPECT_TRUE(analyzeSource(R"(
        fn findMax(arr: Array[i32]) -> i32 {
            let mut max: i32 = 0
            for num in arr {
                if num > max {
                    max = num
                }
            }
            return max
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Enum Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, EnumDeclarationSimple) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Color { Red, Green, Blue }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EnumDeclarationWithData) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option { Some(i32), None }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EnumDeclarationGeneric) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option[T] { Some(T), None }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EnumDeclarationMultipleTypeParams) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Result[T, E] { Ok(T), Err(E) }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EnumVariantMultipleFields) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Message { Move(int, int), Write(str), Quit }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EnumConstructionSimple) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Color { Red, Green, Blue }

        fn test() -> i32 {
            let c := Color.Red
            return 0
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, EnumConstructionWithData) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option { Some(i32), None }

        fn test() -> i32 {
            let x := Option.Some(42)
            let y := Option.None
            return 0
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

// ============================================================================
// Match Statement Exhaustiveness Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, MatchExhaustiveEnum) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Color {
            Red,
            Green,
            Blue
        }

        fn test(c: Color) -> i32 {
            return match c {
                Color.Red => 1,
                Color.Green => 2,
                Color.Blue => 3
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchNonExhaustiveEnum) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Color {
            Red,
            Green,
            Blue
        }

        fn test(c: Color) -> i32 {
            return match c {
                Color.Red => 1,
                Color.Green => 2
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchEnumWithWildcard) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Status {
            Active,
            Inactive,
            Pending
        }

        fn test(s: Status) -> i32 {
            return match s {
                Status.Active => 1,
                _ => 0
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchExhaustiveBool) {
    EXPECT_TRUE(analyzeSource(R"(
        fn test(b: bool) -> str {
            return match b {
                true => "yes",
                false => "no"
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchNonExhaustiveBool) {
    EXPECT_FALSE(analyzeSource(R"(
        fn test(b: bool) -> str {
            return match b {
                true => "yes"
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchEnumWithData) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Result {
            Ok(i32),
            Err(str)
        }

        fn test(r: Result) -> i32 {
            return match r {
                Result.Ok(value) => value,
                Result.Err(_) => -1
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchEnumWithDataNonExhaustive) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Result {
            Ok(i32),
            Err(str)
        }

        fn test(r: Result) -> i32 {
            return match r {
                Result.Ok(value) => value
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchNestedEnums) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option {
            Some(i32),
            None
        }

        enum Result {
            Ok(Option),
            Err(str)
        }

        fn test(r: Result) -> i32 {
            return match r {
                Result.Ok(Option.Some(x)) => x,
                Result.Ok(Option.None) => 0,
                Result.Err(_) => -1
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchIntegerWithWildcard) {
    EXPECT_TRUE(analyzeSource(R"(
        fn test(x: i32) -> str {
            return match x {
                0 => "zero",
                1 => "one",
                _ => "other"
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchIntegerNonExhaustive) {
    EXPECT_FALSE(analyzeSource(R"(
        fn test(x: i32) -> str {
            return match x {
                0 => "zero",
                1 => "one"
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchTypeMismatch) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Color {
            Red,
            Green,
            Blue
        }

        fn test(c: Color) -> i32 {
            return match c {
                Color.Red => 1,
                Color.Green => "two",
                Color.Blue => 3
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchUnreachablePattern) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Color {
            Red,
            Green,
            Blue
        }

        fn test(c: Color) -> i32 {
            return match c {
                Color.Red => 1,
                _ => 0,
                Color.Green => 2
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

// ============================================================================
// Advanced Match Tests - Edge Cases
// ============================================================================

TEST_F(SemanticAnalyzerTest, MatchNestedMatchExpressions) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option {
            Some(i32),
            None
        }

        fn test(a: Option, b: Option) -> i32 {
            return match a {
                Option.Some(x) => match b {
                    Option.Some(y) => x + y,
                    Option.None => x
                },
                Option.None => match b {
                    Option.Some(y) => y,
                    Option.None => 0
                }
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchNestedMatchNonExhaustive) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Option {
            Some(i32),
            None
        }

        fn test(a: Option, b: Option) -> i32 {
            return match a {
                Option.Some(x) => match b {
                    Option.Some(y) => x + y
                },
                Option.None => 0
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchMultipleBindings) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Pair {
            Values(i32, i32),
            Empty
        }

        fn test(p: Pair) -> i32 {
            return match p {
                Pair.Values(a, b) => a + b,
                Pair.Empty => 0
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchDeeplyNestedEnums) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Inner {
            Val(i32),
            Empty
        }

        enum Middle {
            Has(Inner),
            None
        }

        enum Outer {
            Data(Middle),
            Nothing
        }

        fn test(o: Outer) -> i32 {
            return match o {
                Outer.Data(Middle.Has(Inner.Val(x))) => x,
                Outer.Data(Middle.Has(Inner.Empty)) => -1,
                Outer.Data(Middle.None) => -2,
                Outer.Nothing => -3
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchDeeplyNestedNonExhaustive) {
    // This tests nested pattern exhaustiveness
    // Missing: Outer.Data(Middle.Has(Inner.Empty))
    EXPECT_FALSE(analyzeSource(R"(
        enum Inner {
            Val(i32),
            Empty
        }

        enum Middle {
            Has(Inner),
            None
        }

        enum Outer {
            Data(Middle),
            Nothing
        }

        fn test(o: Outer) -> i32 {
            return match o {
                Outer.Data(Middle.Has(Inner.Val(x))) => x,
                Outer.Data(Middle.None) => -2,
                Outer.Nothing => -3
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchMissingTopLevelVariant) {
    // This tests top-level exhaustiveness which IS checked
    EXPECT_FALSE(analyzeSource(R"(
        enum Inner {
            Val(i32),
            Empty
        }

        enum Outer {
            Data(Inner),
            Nothing
        }

        fn test(o: Outer) -> i32 {
            return match o {
                Outer.Data(Inner.Val(x)) => x,
                Outer.Data(Inner.Empty) => -1
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchPatternShadowing) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option {
            Some(i32),
            None
        }

        fn test(opt: Option) -> i32 {
            let x := 100
            return match opt {
                Option.Some(x) => x,
                Option.None => x
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchWithWildcardInConstructor) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Pair {
            Values(i32, i32),
            Empty
        }

        fn test(p: Pair) -> i32 {
            return match p {
                Pair.Values(x, _) => x,
                Pair.Empty => 0
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchWithMultipleWildcards) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Color {
            Red,
            Green,
            Blue
        }

        fn test(c: Color) -> i32 {
            return match c {
                Color.Red => 1,
                _ => 2,
                _ => 3
            }
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchBoolWithWildcard) {
    EXPECT_TRUE(analyzeSource(R"(
        fn test(b: bool) -> str {
            return match b {
                true => "yes",
                _ => "no"
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchUsesPatternBindingInGuard) {
    EXPECT_TRUE(analyzeSource(R"(
        enum Option {
            Some(i32),
            None
        }

        fn test(opt: Option) -> i32 {
            return match opt {
                Option.Some(x) if x > 0 => x,
                Option.Some(x) => -x,
                Option.None => 0
            }
        }
    )"));
    EXPECT_FALSE(errorReporter->hasErrors());
}

TEST_F(SemanticAnalyzerTest, MatchPatternBindingScope) {
    EXPECT_FALSE(analyzeSource(R"(
        enum Option {
            Some(i32),
            None
        }

        fn test(opt: Option) -> i32 {
            match opt {
                Option.Some(x) => x,
                Option.None => 0
            }
            return x
        }
    )"));
    EXPECT_TRUE(errorReporter->hasErrors());
}
