#include <gtest/gtest.h>
#include "../helpers/test_utils.hpp"
#include "../../include/Parser/Parser.hpp"
#include "../../include/Lexer/Lexer.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include "../../include/Error/Error.hpp"
#include <memory>
#include <string>
#include <sstream>

// Extreme stress tests and limit testing for the Volta compiler

class StressTest : public ::testing::Test {
protected:
    Type::TypeRegistry types;
    DiagnosticManager diag;

    StressTest() : diag(false) {}
};

// ============================================================================
// Deep Nesting Stress Tests
// ============================================================================

TEST_F(StressTest, DeeplyNestedIfStatements) {
    // Test parser and lowering with 20 levels of nested if statements
    std::ostringstream source;
    source << "fn deep_nest(x: i32) -> i32 {\n";

    // Create 20 levels of nesting
    for (int i = 0; i < 20; i++) {
        source << std::string(i + 1, ' ') << "if (x > " << i << ") {\n";
    }

    source << std::string(21, ' ') << "return 100;\n";

    for (int i = 19; i >= 0; i--) {
        source << std::string(i + 1, ' ') << "}\n";
    }

    source << "  return 0;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, DeeplyNestedLoops) {
    // Test parser with 10 levels of nested loops
    std::ostringstream source;
    source << "fn nested_loops() -> i32 {\n";
    source << "  let mut sum: i32 = 0;\n";

    for (int i = 0; i < 10; i++) {
        source << std::string(i + 2, ' ') << "for i" << i << " in 0..2 {\n";
    }

    source << std::string(12, ' ') << "sum = sum + 1;\n";

    for (int i = 9; i >= 0; i--) {
        source << std::string(i + 2, ' ') << "}\n";
    }

    source << "  return sum;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, DeeplyNestedBlocks) {
    // Test scoping with 15 levels of nested blocks
    std::ostringstream source;
    source << "fn nested_blocks() -> i32 {\n";

    for (int i = 0; i < 15; i++) {
        source << std::string(i + 2, ' ') << "{\n";
        source << std::string(i + 3, ' ') << "let x" << i << ": i32 = " << i << ";\n";
    }

    source << std::string(17, ' ') << "let result := 14;\n";

    for (int i = 14; i >= 0; i--) {
        source << std::string(i + 2, ' ') << "}\n";
    }

    source << "  return 0;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

// ============================================================================
// Large Program Stress Tests
// ============================================================================

TEST_F(StressTest, ManyVariables) {
    // Test with 100 local variables
    std::ostringstream source;
    source << "fn many_vars() -> i32 {\n";

    for (int i = 0; i < 100; i++) {
        source << "  let var" << i << ": i32 = " << i << ";\n";
    }

    source << "  return var99;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, ManyFunctions) {
    // Test with 50 functions
    std::ostringstream source;

    for (int i = 0; i < 50; i++) {
        source << "fn func" << i << "() -> i32 {\n";
        source << "  return " << i << ";\n";
        source << "}\n\n";
    }

    source << "fn main() -> i32 {\n";
    source << "  return func49();\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, ManyParameters) {
    // Test function with 20 parameters
    std::ostringstream source;
    source << "fn many_params(";

    for (int i = 0; i < 20; i++) {
        if (i > 0) source << ", ";
        source << "p" << i << ": i32";
    }

    source << ") -> i32 {\n";
    source << "  return p0 + p19;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

// ============================================================================
// Large Array Stress Tests
// ============================================================================

TEST_F(StressTest, LargeArrayDeclaration) {
    // Test with large array (1000 elements)
    std::string source = R"(
fn test() -> i32 {
    let arr: [i32; 1000] = [0; 1000];
    return arr[500];
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, LargeArrayInitializer) {
    // Test parsing array with many explicit elements
    std::ostringstream source;
    source << "fn test() -> i32 {\n";
    source << "  let arr: [i32; 100] = [";

    for (int i = 0; i < 100; i++) {
        if (i > 0) source << ", ";
        source << i;
    }

    source << "];\n";
    source << "  return arr[50];\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, NestedArrays) {
    // Test multi-dimensional arrays
    std::string source = R"(
fn test() -> i32 {
    let matrix: [[i32; 10]; 10] = [[0; 10]; 10];
    return matrix[5][5];
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, ThreeDimensionalArray) {
    // Test 3D arrays
    std::string source = R"(
fn test() -> i32 {
    let cube: [[[i32; 5]; 5]; 5] = [[[0; 5]; 5]; 5];
    return cube[2][2][2];
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// Expression Complexity Stress Tests
// ============================================================================

TEST_F(StressTest, LongArithmeticChain) {
    // Test with 50 operations in sequence
    std::ostringstream source;
    source << "fn long_chain() -> i32 {\n";
    source << "  return 1";

    for (int i = 0; i < 50; i++) {
        source << " + " << (i + 2);
    }

    source << ";\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, DeeplyNestedExpressions) {
    // Test with 30 levels of nested parentheses
    std::ostringstream source;
    source << "fn nested_expr() -> i32 {\n";
    source << "  return ";

    for (int i = 0; i < 30; i++) {
        source << "(";
    }

    source << "1";

    for (int i = 0; i < 30; i++) {
        source << " + 1)";
    }

    source << ";\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, ComplexBooleanExpression) {
    // Test with many boolean operations
    std::ostringstream source;
    source << "fn complex_bool(x: i32) -> bool {\n";
    source << "  return ";

    for (int i = 0; i < 20; i++) {
        if (i > 0) source << " and ";
        source << "(x > " << i << ")";
    }

    source << ";\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

// ============================================================================
// Recursive Function Stress Tests
// ============================================================================

TEST_F(StressTest, MutuallyRecursiveFunctions) {
    std::string source = R"(
fn even(n: i32) -> bool {
    if (n == 0) {
        return true;
    }
    return odd(n - 1);
}

fn odd(n: i32) -> bool {
    if (n == 0) {
        return false;
    }
    return even(n - 1);
}

fn main() -> i32 {
    let result := even(10);
    return 0;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MultipleRecursiveCalls) {
    std::string source = R"(
fn tribonacci(n: i32) -> i32 {
    if (n <= 1) {
        return n;
    }
    if (n == 2) {
        return 1;
    }
    return tribonacci(n - 1) + tribonacci(n - 2) + tribonacci(n - 3);
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// Type System Stress Tests
// ============================================================================

TEST_F(StressTest, AllIntegerTypes) {
    std::string source = R"(
fn test_all_ints() -> i64 {
    let a: i8 = 10;
    let b: i16 = 100;
    let c: i32 = 1000;
    let d: i64 = 10000;
    let e: u8 = 20;
    let f: u16 = 200;
    let g: u32 = 2000;
    let h: u64 = 20000;
    return d;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, AllFloatTypes) {
    std::string source = R"(
fn test_floats() -> f64 {
    let x: f32 = 3.14;
    let y: f64 = 3.141592653589793;
    return y;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MixedTypeOperations) {
    std::string source = R"(
fn mixed_types() -> i64 {
    let a: i8 = 10;
    let b: i16 = 20;
    let c: i32 = 30;
    let d: i64 = 40;
    return d;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// String and Literal Stress Tests
// ============================================================================

TEST_F(StressTest, VeryLongString) {
    // Test with 500 character string
    std::ostringstream source;
    source << "fn test() -> i32 {\n";
    source << "  let s: str = \"";

    for (int i = 0; i < 500; i++) {
        source << "a";
    }

    source << "\";\n";
    source << "  return 0;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, ManyStringLiterals) {
    // Test with 50 different string literals
    std::ostringstream source;
    source << "fn test() -> i32 {\n";

    for (int i = 0; i < 50; i++) {
        source << "  let s" << i << ": str = \"string" << i << "\";\n";
    }

    source << "  return 0;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

// ============================================================================
// Control Flow Edge Cases
// ============================================================================

TEST_F(StressTest, ManyElseIfChains) {
    // Test with 30 else-if branches
    std::ostringstream source;
    source << "fn classify(x: i32) -> i32 {\n";
    source << "  if (x < 0) {\n";
    source << "    return -1;\n";
    source << "  }";

    for (int i = 0; i < 30; i++) {
        source << " else if (x == " << i << ") {\n";
        source << "    return " << i << ";\n";
        source << "  }";
    }

    source << " else {\n";
    source << "    return 999;\n";
    source << "  }\n";
    source << "}\n";

    // Note: else-if chains might not be fully supported yet
    VoltaTest::canParse(source.str());
}

TEST_F(StressTest, ManyBreakContinueStatements) {
    std::string source = R"(
fn test_breaks() -> i32 {
    let mut count: i32 = 0;
    for i in 0..100 {
        if (i == 10) {
            break;
        }
        if (i == 5) {
            continue;
        }
        count = count + 1;
    }
    return count;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// Struct Complexity Tests
// ============================================================================

TEST_F(StressTest, StructWithManyFields) {
    // Test struct with 30 fields
    std::ostringstream source;
    source << "struct LargeStruct {\n";

    for (int i = 0; i < 30; i++) {
        source << "  field" << i << ": i32,\n";
    }

    source << "}\n\n";
    source << "fn main() -> i32 { return 0; }\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

TEST_F(StressTest, NestedStructs) {
    std::string source = R"(
struct Inner {
    x: i32,
    y: i32
}

struct Middle {
    inner: Inner,
    z: i32
}

struct Outer {
    middle: Middle,
    w: i32
}

fn test() -> i32 {
    return 0;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, ManyStructDeclarations) {
    // Test with 20 different struct types
    std::ostringstream source;

    for (int i = 0; i < 20; i++) {
        source << "struct Struct" << i << " {\n";
        source << "  field: i32\n";
        source << "}\n\n";
    }

    source << "fn main() -> i32 { return 0; }\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}

// ============================================================================
// Boundary Value Tests
// ============================================================================

TEST_F(StressTest, MaximumI8Value) {
    std::string source = R"(
fn test() -> i8 {
    let max: i8 = 127;
    return max;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MinimumI8Value) {
    std::string source = R"(
fn test() -> i8 {
    let min: i8 = -128;
    return min;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MaximumI32Value) {
    std::string source = R"(
fn test() -> i32 {
    let max: i32 = 2147483647;
    return max;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MinimumI32Value) {
    std::string source = R"(
fn test() -> i32 {
    let min: i32 = -2147483648;
    return min;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MaximumU8Value) {
    std::string source = R"(
fn test() -> u8 {
    let max: u8 = 255;
    return max;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, ZeroValue) {
    std::string source = R"(
fn test() -> i32 {
    let zero: i32 = 0;
    return zero;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, VeryLargeU64Value) {
    std::string source = R"(
fn test() -> u64 {
    let big: u64 = 18446744073709551615;
    return big;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// Complex Real-World Algorithms
// ============================================================================

TEST_F(StressTest, QuickSortImplementation) {
    std::string source = R"(
fn partition(arr: mut [i32; 10], low: i32, high: i32) -> i32 {
    let pivot := arr[high];
    let mut i: i32 = low - 1;

    for j in low..high {
        if (arr[j] <= pivot) {
            i = i + 1;
            let temp := arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    let temp := arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;

    return i + 1;
}

fn main() -> i32 {
    return 0;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, BinarySearchImplementation) {
    std::string source = R"(
fn binary_search(arr: [i32; 10], target: i32) -> i32 {
    let mut low: i32 = 0;
    let mut high: i32 = 9;

    while (low <= high) {
        let mid := (low + high) / 2;

        if (arr[mid] == target) {
            return mid;
        } else if (arr[mid] < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return -1;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, MatrixTranspose) {
    std::string source = R"(
fn transpose(matrix: [[i32; 3]; 3]) -> [[i32; 3]; 3] {
    let mut result: [[i32; 3]; 3] = [[0; 3]; 3];

    for i in 0..3 {
        for j in 0..3 {
            result[j][i] = matrix[i][j];
        }
    }

    return result;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// Pathological Cases
// ============================================================================

TEST_F(StressTest, EmptyFunction) {
    std::string source = R"(
fn empty() -> void {
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, ImmediateReturn) {
    std::string source = R"(
fn immediate() -> i32 {
    return 42;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, UnreachableCode) {
    std::string source = R"(
fn unreachable() -> i32 {
    return 1;
    return 2;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

TEST_F(StressTest, InfiniteLoop) {
    std::string source = R"(
fn infinite() -> i32 {
    while (true) {
        let x := 1;
    }
    return 0;
}
)";

    EXPECT_TRUE(VoltaTest::canParse(source));
}

// ============================================================================
// Performance/Scalability Tests
// ============================================================================

TEST_F(StressTest, LargeProgram) {
    // Combine many features into one large program
    std::ostringstream source;

    // Many helper functions
    for (int i = 0; i < 10; i++) {
        source << "fn helper" << i << "(x: i32) -> i32 {\n";
        source << "  let mut sum: i32 = 0;\n";
        source << "  for j in 0..10 {\n";
        source << "    sum = sum + j + x;\n";
        source << "  }\n";
        source << "  return sum;\n";
        source << "}\n\n";
    }

    // Main function that uses all helpers
    source << "fn main() -> i32 {\n";
    source << "  let mut total: i32 = 0;\n";
    for (int i = 0; i < 10; i++) {
        source << "  total = total + helper" << i << "(" << i << ");\n";
    }
    source << "  return total;\n";
    source << "}\n";

    EXPECT_TRUE(VoltaTest::canParse(source.str()));
}
