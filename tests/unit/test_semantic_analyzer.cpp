#include <gtest/gtest.h>
#include "../helpers/test_utils.hpp"

using namespace VoltaTest;

// ============================================================================
// Type Compatibility and Implicit Casting Tests
// ============================================================================

class SemanticAnalyzerTest : public ::testing::Test {
protected:
    bool hasSemanticError(const std::string& source) {
        return !passesSemanticAnalysis(source);
    }
};

// ----------------------------------------------------------------------------
// NO Implicit Casting Between Signed and Unsigned
// ----------------------------------------------------------------------------

TEST_F(SemanticAnalyzerTest, NoImplicitCastSignedToUnsigned) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 42;
    let y: u32 = x;  // ERROR: Cannot implicitly cast i32 to u32
    return 0;
}
)";
    // This should fail semantic analysis
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, NoImplicitCastUnsignedToSigned) {
    std::string source = R"(
fn main() -> i32 {
    let x: u32 = 42;
    let y: i32 = x;  // ERROR: Cannot implicitly cast u32 to i32
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, NoImplicitCastInFunctionCall) {
    std::string source = R"(
fn takes_unsigned(x: u32) -> i32 {
    return 0;
}

fn main() -> i32 {
    let signed_val: i32 = 42;
    takes_unsigned(signed_val);  // ERROR: Cannot pass i32 to u32 parameter
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ----------------------------------------------------------------------------
// NO Narrowing Conversions (Large to Small Integer Types)
// ----------------------------------------------------------------------------

TEST_F(SemanticAnalyzerTest, NoImplicitNarrowingI64ToI32) {
    std::string source = R"(
fn main() -> i32 {
    let x: i64 = 1000;
    let y: i32 = x;  // ERROR: Cannot narrow i64 to i32
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, NoImplicitNarrowingI32ToI8) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 200;
    let y: i8 = x;  // ERROR: Cannot narrow i32 to i8
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, NoImplicitNarrowingU64ToU16) {
    std::string source = R"(
fn main() -> i32 {
    let x: u64 = 1000;
    let y: u16 = x;  // ERROR: Cannot narrow u64 to u16
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ----------------------------------------------------------------------------
// YES Widening Conversions (Small to Large Integer Types - SAME SIGNEDNESS)
// ----------------------------------------------------------------------------

TEST_F(SemanticAnalyzerTest, AllowWideningI8ToI32) {
    std::string source = R"(
fn main() -> i32 {
    let x: i8 = 10;
    let y: i32 = x;  // OK: Can widen i8 to i32
    return y;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, AllowWideningI16ToI64) {
    std::string source = R"(
fn main() -> i32 {
    let x: i16 = 100;
    let y: i64 = x;  // OK: Can widen i16 to i64
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, AllowWideningU8ToU32) {
    std::string source = R"(
fn main() -> i32 {
    let x: u8 = 10;
    let y: u32 = x;  // OK: Can widen u8 to u32
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ----------------------------------------------------------------------------
// Integer Literal Range Checking
// ----------------------------------------------------------------------------

TEST_F(SemanticAnalyzerTest, I8LiteralInRange) {
    std::string source = R"(
fn main() -> i32 {
    let x: i8 = 127;   // OK: Max i8 value
    let y: i8 = -128;  // OK: Min i8 value
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, I8LiteralOutOfRange) {
    std::string source = R"(
fn main() -> i32 {
    let x: i8 = 128;  // ERROR: 128 exceeds i8 range (max 127)
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, I8LiteralUnderflow) {
    std::string source = R"(
fn main() -> i32 {
    let x: i8 = -129;  // ERROR: -129 below i8 range (min -128)
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, U8CannotBeNegative) {
    std::string source = R"(
fn main() -> i32 {
    let x: u8 = -1;  // ERROR: Unsigned cannot hold negative values
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, U8LiteralInRange) {
    std::string source = R"(
fn main() -> i32 {
    let x: u8 = 0;    // OK: Min u8 value
    let y: u8 = 255;  // OK: Max u8 value
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, U8LiteralOutOfRange) {
    std::string source = R"(
fn main() -> i32 {
    let x: u8 = 256;  // ERROR: 256 exceeds u8 range (max 255)
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, I16LiteralRange) {
    std::string source = R"(
fn main() -> i32 {
    let x: i16 = 32767;   // OK: Max i16
    let y: i16 = -32768;  // OK: Min i16
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, I16LiteralOutOfRange) {
    std::string source = R"(
fn main() -> i32 {
    let x: i16 = 32768;  // ERROR: Exceeds i16 max
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ----------------------------------------------------------------------------
// Return Statement Type Checking with Literals
// ----------------------------------------------------------------------------

TEST_F(SemanticAnalyzerTest, ReturnLiteralInRange) {
    std::string source = R"(
fn get_byte() -> i8 {
    return 100;  // OK: 100 fits in i8
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ReturnLiteralOutOfRange) {
    std::string source = R"(
fn get_byte() -> i8 {
    return 200;  // ERROR: 200 exceeds i8 range
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ReturnNegativeLiteralToUnsigned) {
    std::string source = R"(
fn get_unsigned() -> u32 {
    return -1;  // ERROR: Cannot return negative to unsigned
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ReturnWrongTypeVariable) {
    std::string source = R"(
fn returns_i32() -> i32 {
    let x: i64 = 100;
    return x;  // ERROR: Cannot implicitly narrow i64 to i32
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ----------------------------------------------------------------------------
// Float Type Checking
// ----------------------------------------------------------------------------

TEST_F(SemanticAnalyzerTest, NoImplicitFloatToInt) {
    std::string source = R"(
fn main() -> i32 {
    let x: f32 = 3.14;
    let y: i32 = x;  // ERROR: Cannot implicitly cast float to int
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, NoImplicitIntToFloat) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 42;
    let y: f32 = x;  // ERROR: Cannot implicitly cast int to float
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, NoNarrowingF64ToF32) {
    std::string source = R"(
fn main() -> i32 {
    let x: f64 = 3.14159265359;
    let y: f32 = x;  // ERROR: Cannot narrow f64 to f32
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, AllowWideningF32ToF64) {
    std::string source = R"(
fn main() -> i32 {
    let x: f32 = 3.14;
    let y: f64 = x;  // OK: Can widen f32 to f64
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ============================================================================
// Variable Scoping and Shadowing
// ============================================================================

TEST_F(SemanticAnalyzerTest, VariableShadowingNotAllowed) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    let x: i32 = 20;  // OK: Shadowing is allowed
    return x;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, UndeclaredVariableError) {
    std::string source = R"(
fn main() -> i32 {
    return y;  // ERROR: Variable 'y' not declared
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, VariableUsedBeforeDeclaration) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = y;  // ERROR: 'y' used before declaration
    let y: i32 = 10;
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, BlockScopingValid) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    if true {
        let y: i32 = 20;
        let z: i32 = x + y;  // OK: x is visible in nested scope
    }
    return x;  // OK: x still in scope
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, VariableOutOfScope) {
    std::string source = R"(
fn main() -> i32 {
    if true {
        let y: i32 = 20;
    }
    return y;  // ERROR: y is out of scope
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ============================================================================
// Mutability Checking
// ============================================================================

TEST_F(SemanticAnalyzerTest, CannotAssignToImmutableVariable) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    x = 20;  // ERROR: Cannot assign to immutable variable
    return x;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, CanAssignToMutableVariable) {
    std::string source = R"(
fn main() -> i32 {
    let mut x: i32 = 10;
    x = 20;  // OK: x is mutable
    return x;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, CannotPassImmutableAsReference) {
    std::string source = R"(
fn modify(x: mut ref i32) {
    x = 100;
}

fn main() -> i32 {
    let x: i32 = 10;
    modify(x);  // ERROR: Cannot pass immutable variable as mut ref
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, CanPassMutableAsReference) {
    std::string source = R"(
fn modify(x: mut ref i32) {
    x = 100;
}

fn main() -> i32 {
    let mut x: i32 = 10;
    modify(x);  // OK: x is mutable
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, CanPassImmutableAsConstRef) {
    std::string source = R"(
fn read_only(x: ref i32) -> i32 {
    return x;
}

fn main() -> i32 {
    let x: i32 = 10;
    return read_only(x);  // OK: const ref can accept immutable
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ============================================================================
// Function Signature Validation
// ============================================================================

TEST_F(SemanticAnalyzerTest, UndeclaredFunctionCall) {
    std::string source = R"(
fn main() -> i32 {
    return foo();  // ERROR: Function 'foo' not declared
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, WrongNumberOfArguments) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> i32 {
    return add(10);  // ERROR: Expected 2 arguments, got 1
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, WrongArgumentType) {
    std::string source = R"(
fn takes_int(x: i32) -> i32 {
    return x;
}

fn main() -> i32 {
    let f: f32 = 3.14;
    return takes_int(f);  // ERROR: Cannot pass f32 to i32 parameter
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, RecursionAllowed) {
    std::string source = R"(
fn factorial(n: i32) -> i32 {
    if n <= 1 {
        return 1;
    }
    return n * factorial(n - 1);  // OK: Recursive call
}

fn main() -> i32 {
    return factorial(5);
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, DuplicateFunctionDefinition) {
    std::string source = R"(
fn foo() -> i32 {
    return 42;
}

fn foo() -> i32 {  // ERROR: Duplicate function definition
    return 100;
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ============================================================================
// Return Statement Validation
// ============================================================================

TEST_F(SemanticAnalyzerTest, MissingReturnStatement) {
    std::string source = R"(
fn must_return() -> i32 {
    let x: i32 = 10;
    // ERROR: Missing return statement
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ReturnInVoidFunction) {
    std::string source = R"(
fn no_return() -> void {
    return;  // OK: Can return void
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ReturnValueInVoidFunction) {
    std::string source = R"(
fn no_return() -> void {
    return 42;  // ERROR: Cannot return value in void function
}

fn main() -> i32 {
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, VoidFunctionWithoutReturn) {
    std::string source = R"(
fn void_func() -> void {
    let x: i32 = 10;
    // OK: void functions don't require return
}

fn main() -> i32 {
    void_func();
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ============================================================================
// Array Type Checking
// ============================================================================

TEST_F(SemanticAnalyzerTest, ArrayIndexOutOfBoundsLiteral) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1, 2, 3, 4, 5];
    return arr[10];  // ERROR: Index 10 out of bounds for array of size 5
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ArrayTypeMismatch) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1.0, 2.0, 3.0, 4.0, 5.0];  // ERROR: f32 elements in i32 array
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ArraySizeMismatch) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1, 2, 3];  // ERROR: Expected 5 elements, got 3
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ArrayIndexNonInteger) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1, 2, 3, 4, 5];
    let idx: f32 = 2.5;
    return arr[idx];  // ERROR: Array index must be integer
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ============================================================================
// Struct Type Checking
// ============================================================================

TEST_F(SemanticAnalyzerTest, StructFieldDoesNotExist) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32
}

fn main() -> i32 {
    let p := Point { x: 10.0, y: 20.0 };
    let z := p.z;  // ERROR: Field 'z' does not exist on Point
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, StructFieldTypeMismatch) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32
}

fn main() -> i32 {
    let p := Point { x: 10, y: 20 };  // ERROR: i32 literals, expected f32
    return 0;
}
)";
    // This might be OK if integer literals can be promoted to float
    // Adjust based on language spec
}

TEST_F(SemanticAnalyzerTest, AccessPrivateField) {
    std::string source = R"(
struct Point {
    x: f32,  // Private field
    pub y: f32
}

fn main() -> i32 {
    let p := Point { x: 10.0, y: 20.0 };
    let val := p.x;  // ERROR: Cannot access private field 'x'
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, CanAccessPublicField) {
    std::string source = R"(
struct Point {
    pub x: f32,
    pub y: f32
}

fn main() -> i32 {
    let p := Point { x: 10.0, y: 20.0 };
    let val := p.x;  // OK: x is public
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ============================================================================
// Operator Type Checking
// ============================================================================

TEST_F(SemanticAnalyzerTest, ArithmeticOnIncompatibleTypes) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    let y: f32 = 3.14;
    let z := x + y;  // ERROR: Cannot add i32 and f32
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ComparisonSignedUnsigned) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = -10;
    let y: u32 = 10;
    if x < y {  // ERROR or WARNING: Comparing signed with unsigned
        return 1;
    }
    return 0;
}
)";
    // Might be warning instead of error - adjust based on spec
    EXPECT_TRUE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, LogicalOperatorOnNonBool) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    if x and true {  // ERROR: 'and' requires bool operands
        return 1;
    }
    return 0;
}
)";
    EXPECT_TRUE(hasSemanticError(source));
}

// ============================================================================
// Type Inference Tests
// ============================================================================

TEST_F(SemanticAnalyzerTest, TypeInferenceFromLiteral) {
    std::string source = R"(
fn main() -> i32 {
    let x := 42;  // Infer as i32 (default integer type)
    let y: i32 = x;  // OK: x is i32
    return y;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, TypeInferenceFromExpression) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    let y: i32 = 20;
    let z := x + y;  // Infer as i32
    return z;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ============================================================================
// Edge Cases and Complex Scenarios
// ============================================================================

TEST_F(SemanticAnalyzerTest, ChainedComparisons) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 5;
    let y: i32 = 10;
    let z: i32 = 15;
    if x < y and y < z {  // OK: Proper boolean logic
        return 1;
    }
    return 0;
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

TEST_F(SemanticAnalyzerTest, ComplexNestedScopes) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    if true {
        let y: i32 = 20;
        if true {
            let z: i32 = x + y;  // OK: x and y visible
        }
    }
    return x;  // OK: x still in scope
}
)";
    EXPECT_FALSE(hasSemanticError(source));
}

// ============================================================================
// Future Features (Disabled)
// ============================================================================

TEST_F(SemanticAnalyzerTest, DISABLED_EnumValueTypeCheck) {
    SKIP_UNIMPLEMENTED_FEATURE("enum semantic analysis");
}

TEST_F(SemanticAnalyzerTest, DISABLED_VariantPatternExhaustiveness) {
    SKIP_UNIMPLEMENTED_FEATURE("variant pattern exhaustiveness checking");
}

TEST_F(SemanticAnalyzerTest, DISABLED_MatchExhaustiveness) {
    SKIP_UNIMPLEMENTED_FEATURE("match exhaustiveness checking");
}
