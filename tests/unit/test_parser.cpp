#include <gtest/gtest.h>
#include "../helpers/test_utils.hpp"
#include "../../include/Parser/Parser.hpp"
#include "../../include/Type/TypeRegistry.hpp"

using namespace VoltaTest;

// Helper class for parser tests
class ParserTest : public ::testing::Test {
protected:
    Type::TypeRegistry types;
    DiagnosticManager diag;

    ParserTest() : diag(false) {} // No color for tests

    std::unique_ptr<Program> parse(const std::string& source) {
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();
        Parser parser(tokens, types, diag);
        return parser.parseProgram();
    }

    bool canParse(const std::string& source) {
        try {
            auto program = parse(source);
            return program != nullptr && !diag.hasErrors();
        } catch (...) {
            return false;
        }
    }
};

// ============================================================================
// Basic Program Structure Tests
// ============================================================================

TEST_F(ParserTest, EmptyProgram) {
    auto program = parse("");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->statements.size(), 0);
}

TEST_F(ParserTest, MinimalMainFunction) {
    std::string source = R"(
fn main() -> i32 {
    return 0;
}
)";
    auto program = parse(source);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(program->statements.size(), 1);
}

// ============================================================================
// Variable Declaration Tests
// ============================================================================

TEST_F(ParserTest, SimpleVariableDeclaration) {
    std::string source = "fn main() -> i32 { let x: i32 = 42; return 0; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, InferredVariableDeclaration) {
    std::string source = "fn main() -> i32 { let x := 42; return 0; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, MutableVariableDeclaration) {
    std::string source = "fn main() -> i32 { let mut x: i32 = 0; return 0; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, MutableInferredVariable) {
    std::string source = "fn main() -> i32 { let mut x := 10; return 0; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, VariableWithoutInitializer) {
    std::string source = "fn main() -> i32 { let x: i32; return 0; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, MultipleVariableDeclarations) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    let y: i32 = 20;
    let z := x + y;
    return z;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Function Declaration Tests
// ============================================================================

TEST_F(ParserTest, FunctionWithNoParameters) {
    std::string source = R"(
fn foo() -> i32 {
    return 42;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, FunctionWithSingleParameter) {
    std::string source = R"(
fn double(x: i32) -> i32 {
    return x * 2;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, FunctionWithMultipleParameters) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, FunctionWithReferenceParameter) {
    std::string source = R"(
fn modify(x: ref i32) -> i32 {
    return x;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, FunctionWithMutableReferenceParameter) {
    std::string source = R"(
fn modify(x: mut ref i32) {
    x = x + 1;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, PublicFunction) {
    std::string source = R"(
pub fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, VoidReturnFunction) {
    std::string source = R"(
fn print_hello() -> void {
    return;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Expression Tests
// ============================================================================

TEST_F(ParserTest, LiteralInteger) {
    std::string source = "fn main() -> i32 { return 42; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LiteralFloat) {
    std::string source = "fn main() -> f32 { return 3.14; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LiteralBoolean) {
    std::string source = "fn main() -> bool { return true; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LiteralString) {
    std::string source = R"(fn main() -> i32 { let s := "hello"; return 0; })";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, SimpleAddition) {
    std::string source = "fn main() -> i32 { return 2 + 3; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, SimpleSubtraction) {
    std::string source = "fn main() -> i32 { return 10 - 5; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, SimpleMultiplication) {
    std::string source = "fn main() -> i32 { return 4 * 5; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, SimpleDivision) {
    std::string source = "fn main() -> i32 { return 20 / 4; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, SimpleModulo) {
    std::string source = "fn main() -> i32 { return 10 % 3; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ComplexArithmeticExpression) {
    std::string source = "fn main() -> i32 { return 2 + 3 * 4 - 5 / 2; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ParenthesizedExpression) {
    std::string source = "fn main() -> i32 { return (2 + 3) * 4; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, NestedParentheses) {
    std::string source = "fn main() -> i32 { return ((2 + 3) * (4 - 1)); }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, UnaryMinus) {
    std::string source = "fn main() -> i32 { return -42; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, UnaryNot) {
    std::string source = "fn main() -> bool { return not true; }";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Comparison Expression Tests
// ============================================================================

TEST_F(ParserTest, EqualityComparison) {
    std::string source = "fn main() -> bool { return 5 == 5; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, InequalityComparison) {
    std::string source = "fn main() -> bool { return 5 != 3; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LessThanComparison) {
    std::string source = "fn main() -> bool { return 3 < 5; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LessEqualComparison) {
    std::string source = "fn main() -> bool { return 5 <= 5; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, GreaterThanComparison) {
    std::string source = "fn main() -> bool { return 10 > 5; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, GreaterEqualComparison) {
    std::string source = "fn main() -> bool { return 10 >= 10; }";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Logical Expression Tests
// ============================================================================

TEST_F(ParserTest, LogicalAnd) {
    std::string source = "fn main() -> bool { return true and false; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LogicalOr) {
    std::string source = "fn main() -> bool { return true or false; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ComplexLogicalExpression) {
    std::string source = "fn main() -> bool { return (x > 5 and x < 10) or y == 0; }";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Assignment Tests
// ============================================================================

TEST_F(ParserTest, SimpleAssignment) {
    std::string source = "fn main() -> i32 { let mut x := 0; x = 10; return x; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, CompoundPlusAssignment) {
    std::string source = "fn main() -> i32 { let mut x := 5; x += 10; return x; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, CompoundMinusAssignment) {
    std::string source = "fn main() -> i32 { let mut x := 20; x -= 5; return x; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, CompoundMultAssignment) {
    std::string source = "fn main() -> i32 { let mut x := 5; x *= 2; return x; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, CompoundDivAssignment) {
    std::string source = "fn main() -> i32 { let mut x := 20; x /= 4; return x; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, IncrementOperator) {
    std::string source = "fn main() -> i32 { let mut x := 0; x++; return x; }";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, DecrementOperator) {
    std::string source = "fn main() -> i32 { let mut x := 10; x--; return x; }";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Control Flow Tests - If Statements
// ============================================================================

TEST_F(ParserTest, SimpleIfStatement) {
    std::string source = R"(
fn main() -> i32 {
    if true {
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, IfElseStatement) {
    std::string source = R"(
fn main() -> i32 {
    if x > 0 {
        return 1;
    } else {
        return -1;
    }
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, IfElseIfElseChain) {
    std::string source = R"(
fn main() -> i32 {
    if x > 0 {
        return 1;
    } else if x < 0 {
        return -1;
    } else {
        return 0;
    }
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, NestedIfStatements) {
    std::string source = R"(
fn main() -> i32 {
    if x > 0 {
        if y > 0 {
            return 1;
        }
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, IfWithComplexCondition) {
    std::string source = R"(
fn main() -> i32 {
    if x > 5 and y < 10 or z == 0 {
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Control Flow Tests - While Loops
// ============================================================================

TEST_F(ParserTest, SimpleWhileLoop) {
    std::string source = R"(
fn main() -> i32 {
    let mut i := 0;
    while i < 10 {
        i++;
    }
    return i;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, WhileLoopWithBreak) {
    std::string source = R"(
fn main() -> i32 {
    let mut i := 0;
    while true {
        if i == 5 {
            break;
        }
        i++;
    }
    return i;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, WhileLoopWithContinue) {
    std::string source = R"(
fn main() -> i32 {
    let mut i := 0;
    let mut sum := 0;
    while i < 10 {
        i++;
        if i % 2 == 0 {
            continue;
        }
        sum += i;
    }
    return sum;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, NestedWhileLoops) {
    std::string source = R"(
fn main() -> i32 {
    let mut i := 0;
    while i < 10 {
        let mut j := 0;
        while j < 10 {
            j++;
        }
        i++;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Control Flow Tests - For Loops
// ============================================================================

TEST_F(ParserTest, SimpleForLoop) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..10 {
        i++;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ForLoopWithInclusiveRange) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..=10 {
        i++;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ForLoopWithBreak) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..10 {
        if i == 5 {
            break;
        }
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ForLoopWithContinue) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..10 {
        if i % 2 == 0 {
            continue;
        }
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, NestedForLoops) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..10 {
        for j in 0..10 {
            let k := i * j;
        }
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Array Tests
// ============================================================================

TEST_F(ParserTest, ArrayLiteralSimple) {
    std::string source = R"(
fn main() -> i32 {
    let arr := [1, 2, 3, 4, 5];
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ArrayLiteralEmpty) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 0] = [];
    return 0;
}
)";
    EXPECT_FALSE(canParse(source));
}

TEST_F(ParserTest, ArrayTypeAnnotation) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1, 2, 3, 4, 5];
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ArrayIndexing) {
    std::string source = R"(
fn main() -> i32 {
    let arr := [1, 2, 3, 4, 5];
    return arr[0];
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ArrayIndexAssignment) {
    std::string source = R"(
fn main() -> i32 {
    let mut arr := [1, 2, 3, 4, 5];
    arr[0] = 10;
    return arr[0];
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ArrayRepeatSyntax) {
    std::string source = R"(
fn main() -> i32 {
    let arr := [0; 10];
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Function Call Tests
// ============================================================================

TEST_F(ParserTest, SimpleFunctionCall) {
    std::string source = R"(
fn foo() -> i32 {
    return 42;
}

fn main() -> i32 {
    return foo();
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, FunctionCallWithArguments) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> i32 {
    return add(5, 10);
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, NestedFunctionCalls) {
    std::string source = R"(
fn double(x: i32) -> i32 {
    return x * 2;
}

fn main() -> i32 {
    return double(double(5));
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Struct Tests
// ============================================================================

TEST_F(ParserTest, SimpleStructDeclaration) {
    std::string source = R"(
struct Point {
    x: f32,
    y: f32
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, PublicStructDeclaration) {
    std::string source = R"(
pub struct Point {
    pub x: f32,
    pub y: f32
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructWithPrivateFields) {
    std::string source = R"(
pub struct Point {
    pub x: f32,
    y: f32
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructWithMethods) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32

    pub fn new(x: f32, y: f32) -> Point {
        return Point {x:x, y:y};
    }

    pub fn distance(self) -> f32 {
        return 0.0;
    }
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructLiteral) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32
}

fn main() -> i32 {
    let p := Point { x: 10.0, y: 20.0 };
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructFieldAccess) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32
}

fn main() -> f32 {
    let p := Point { x: 10.0, y: 20.0 };
    return p.x;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructStaticMethodCall) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32

    pub fn new(x: f32, y: f32) -> Point {
        return Point {x: x, y: y};
    }
}

fn main() -> i32 {
    let p := Point.new(10.0, 20.0);
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructInstanceMethodCall) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32

    pub fn distance(self) -> f32 {
        return 0.0;
    }
}

fn main() -> f32 {
    let p := Point { x: 10.0, y: 20.0 };
    return p.distance();
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Pointer Tests
// ============================================================================

TEST_F(ParserTest, PointerTypeAnnotation) {
    std::string source = R"(
fn main() -> i32 {
    let p: ptr<i32>;
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, OpaquePointerType) {
    std::string source = R"(
fn main() -> i32 {
    let p: ptr<opaque>;
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Extern Block Tests
// ============================================================================

TEST_F(ParserTest, SimpleExternBlock) {
    std::string source = R"(
extern "C" {
    fn puts(s: ptr<u8>) -> i32;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ExternBlockMultipleFunctions) {
    std::string source = R"(
extern "C" {
    fn malloc(size: usize) -> ptr<opaque>;
    fn free(ptr: ptr<opaque>);
    fn strlen(s: ptr<u8>) -> usize;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Import Tests
// ============================================================================

TEST_F(ParserTest, SimpleImport) {
    std::string source = R"(
import std.io.{print};
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, MultipleImports) {
    std::string source = R"(
import std.io.{print, println};
import std.math.{sqrt, abs};
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Complex Integration Tests
// ============================================================================

TEST_F(ParserTest, BubbleSortExample) {
    std::string source = R"(
fn bubble_sort(arr: mut [i32; 10]) -> [i32; 10] {
    for i in 0..10 {
        for j in 0..9 {
            if arr[j] > arr[j + 1] {
                let temp := arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    return arr;
}

fn main() -> i32 {
    let mut x: [i32; 10] = [3, 2, 7, 0, 9, 9, 10, 250, 4, 10];
    bubble_sort(x);
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, RecursiveFibonacci) {
    std::string source = R"(
fn fib(n: i32) -> i32 {
    if n <= 1 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn main() -> i32 {
    return fib(10);
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, CompleteStructExample) {
    std::string source = R"(
pub struct Point {
    pub x: f32,
    pub y: f32

    pub fn new(x: f32, y: f32) -> Point {
        return Point {x : x, y : y};
    }

    pub fn distance(self) -> f32 {
        return 0.0;
    }

    pub fn move_by(mut self, dx: f32, dy: f32) {
        self.x = self.x + dx;
        self.y = self.y + dy;
    }
}

fn main() -> i32 {
    let mut p := Point.new(10.0, 20.0);
    let d := p.distance();
    p.move_by(5.0, 5.0);
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Error Cases - Should Fail to Parse
// ============================================================================

TEST_F(ParserTest, MissingFunctionBody) {
    std::string source = "fn main() -> i32;";
    EXPECT_FALSE(canParse(source));
}

TEST_F(ParserTest, MissingSemicolon) {
    std::string source = "fn main() -> i32 { let x := 42 return x; }";
    EXPECT_FALSE(canParse(source));
}

TEST_F(ParserTest, UnmatchedBrace) {
    std::string source = "fn main() -> i32 { return 0;";
    EXPECT_FALSE(canParse(source));
}

TEST_F(ParserTest, InvalidVariableDeclaration) {
    std::string source = "fn main() -> i32 { let := 42; return 0; }";
    EXPECT_FALSE(canParse(source));
}

TEST_F(ParserTest, InvalidExpression) {
    std::string source = "fn main() -> i32 { return + * 5; }";
    EXPECT_FALSE(canParse(source));
}

// ============================================================================
// Future Features (Currently Skipped)
// ============================================================================

TEST_F(ParserTest, DISABLED_EnumDeclaration) {
    SKIP_UNIMPLEMENTED_FEATURE("enum declarations");
    std::string source = R"(
enum Color {
    Red,
    Green,
    Blue
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, DISABLED_EnumWithExplicitValues) {
    SKIP_UNIMPLEMENTED_FEATURE("enum with explicit values");
    std::string source = R"(
enum Status {
    Success = 0,
    Error = 1,
    Pending = 2
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, DISABLED_VariantDeclaration) {
    SKIP_UNIMPLEMENTED_FEATURE("variant declarations");
    std::string source = R"(
variant Option {
    Some(i32),
    None
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, DISABLED_MatchExpression) {
    SKIP_UNIMPLEMENTED_FEATURE("match expressions");
    std::string source = R"(
fn main() -> i32 {
    let x := 5;
    return match x {
        1 => 10,
        2 => 20,
        else => 0
    };
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, DISABLED_VariantWithMethods) {
    SKIP_UNIMPLEMENTED_FEATURE("variant with methods");
    std::string source = R"(
variant Expr {
    Number(i32),
    Add(Expr, Expr),
    Multiply(Expr, Expr)

    fn eval(self) -> i32 {
        return match self {
            Expr::Number(n) => n,
            Expr::Add(lhs, rhs) => lhs.eval() + rhs.eval(),
            Expr::Multiply(lhs, rhs) => lhs.eval() * rhs.eval(),
            else => 0
        };
    }
}
)";
    EXPECT_TRUE(canParse(source));
}

// ============================================================================
// Struct Literal Disambiguation Tests
// These tests ensure the parser correctly distinguishes struct literals
// from other constructs using lookahead (checking for "field:" pattern)
// ============================================================================

TEST_F(ParserTest, StructLiteralWithLowercaseName) {
    // Struct literals should work regardless of capitalization
    std::string source = R"(
struct point {
    pub x: f32,
    pub y: f32
}

fn main() -> i32 {
    let p := point { x: 10.0, y: 20.0 };
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, EmptyStructLiteral) {
    // Empty struct literal: Point {}
    std::string source = R"(
struct Point {
}

fn main() -> i32 {
    let p := Point {};
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, ChainedComparisonNotStructLiteral) {
    // Critical test: "y < z {" should NOT be parsed as struct literal
    // The { starts the if-body, not a struct literal
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 5;
    let y: i32 = 10;
    let z: i32 = 15;
    if x < y and y < z {
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, VariableFollowedByBlockNotStructLiteral) {
    // Variable followed by { should be treated as variable, not struct literal
    std::string source = R"(
fn main() -> i32 {
    let condition: bool = true;
    if condition {
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, LogicalAndWithComparisonAndBlock) {
    // More complex: multiple comparisons with logical operators before {
    std::string source = R"(
fn main() -> i32 {
    let a: i32 = 1;
    let b: i32 = 2;
    let c: i32 = 3;
    let d: i32 = 4;
    if a < b and c < d {
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructLiteralInIfCondition) {
    // Struct literal CAN appear inside if condition (as expression)
    std::string source = R"(
struct Point {
    pub x: i32,
    pub y: i32
}

fn check(p: Point) -> bool {
    return true;
}

fn main() -> i32 {
    if check(Point { x: 1, y: 2 }) {
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, StructLiteralInWhileCondition) {
    // Struct literal in while loop expression context
    std::string source = R"(
struct State {
    pub done: bool
}

fn is_done(s: State) -> bool {
    return s.done;
}

fn main() -> i32 {
    while is_done(State { done: false }) {
        break;
    }
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}

TEST_F(ParserTest, NestedStructLiteral) {
    // Nested struct literals
    std::string source = R"(
struct Inner {
    pub value: i32
}

struct Outer {
    pub inner: Inner
}

fn main() -> i32 {
    let o := Outer { inner: Inner { value: 42 } };
    return 0;
}
)";
    EXPECT_TRUE(canParse(source));
}
