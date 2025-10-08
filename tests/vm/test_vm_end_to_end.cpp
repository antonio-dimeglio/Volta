#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "lexer/lexer.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "IR/IRGenerator.hpp"
#include "IR/Verifier.hpp"
#include "error/ErrorReporter.hpp"
#include "vm/BytecodeCompiler.hpp"
#include "vm/VM.hpp"
#include "vm/Value.hpp"

using namespace volta;
using namespace volta::lexer;
using namespace volta::parser;
using namespace volta::semantic;
using namespace volta::errors;

// Don't use 'using namespace' for ir and vm - they both have 'vm::Value'
// Use qualified names instead: vm::Value and ir::vm::Value

/**
 * End-to-End vm::VM Tests
 *
 * Tests the complete compilation and execution pipeline:
 * Source Code → Lexer → Parser → Semantic Analysis → IR Generation → Bytecode → vm::VM Execution
 */

class VMEndToEndTest : public ::testing::Test {
protected:
    /**
     * Compile and execute Volta source code
     * Returns the vm::Value returned by the executed function
     */
    vm::Value executeSource(const std::string& source, const std::string& functionName = "main") {
        // 1. Lex
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // 2. Parse
        Parser parser(tokens);
        auto ast = parser.parseProgram();
        if (!ast) {
            throw std::runtime_error("Parse failed");
        }

        // 3. Semantic Analysis
        ErrorReporter semanticReporter;
        SemanticAnalyzer analyzer(semanticReporter);
        analyzer.registerBuiltins();

        if (!analyzer.analyze(*ast)) {
            std::ostringstream oss;
            semanticReporter.printErrors(oss);
            throw std::runtime_error("Semantic errors:\n" + oss.str());
        }

        // 4. IR Generation
        auto module = ir::generateIR(*ast, analyzer, "test_module");
        if (!module) {
            throw std::runtime_error("IR generation failed");
        }

        // 5. Verify IR
        ir::Verifier verifier;
        if (!verifier.verify(*module)) {
            std::ostringstream oss;
            for (const auto& error : verifier.getErrors()) {
                oss << "  - " << error << "\n";
            }
            throw std::runtime_error("IR verification failed:\n" + oss.str());
        }

        // 6. Compile to Bytecode
        vm::BytecodeCompiler compiler;
        auto bytecodeModule = compiler.compile(std::move(module));

        // 7. Execute in VM
        vm::VM vm;
        return vm.execute(*bytecodeModule, functionName);
    }

    /**
     * Execute with arguments
     */
    vm::Value executeSourceWithArgs(const std::string& source, const std::string& functionName,
                                  const std::vector<vm::Value>& args) {
        // Same pipeline as executeSource, but with args
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parseProgram();
        if (!ast) {
            throw std::runtime_error("Parse failed");
        }

        ErrorReporter semanticReporter;
        SemanticAnalyzer analyzer(semanticReporter);
        analyzer.registerBuiltins();

        if (!analyzer.analyze(*ast)) {
            std::ostringstream oss;
            semanticReporter.printErrors(oss);
            throw std::runtime_error("Semantic errors:\n" + oss.str());
        }

        auto module = ir::generateIR(*ast, analyzer, "test_module");
        if (!module) {
            throw std::runtime_error("IR generation failed");
        }

        ir::Verifier verifier;
        if (!verifier.verify(*module)) {
            std::ostringstream oss;
            for (const auto& error : verifier.getErrors()) {
                oss << "  - " << error << "\n";
            }
            throw std::runtime_error("IR verification failed:\n" + oss.str());
        }

        vm::BytecodeCompiler compiler;
        auto bytecodeModule = compiler.compile(std::move(module));

        vm::VM vm;
        return vm.execute(*bytecodeModule, functionName, args);
    }
};

// ============================================================================
// Basic Arithmetic Tests
// ============================================================================

TEST_F(VMEndToEndTest, SimpleReturn) {
    std::string source = R"(
fn main() -> int {
    return 42
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

TEST_F(VMEndToEndTest, SimpleAddition) {
    std::string source = R"(
fn main() -> int {
    return 5 + 3
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 8);
}

TEST_F(VMEndToEndTest, ArithmeticOperations) {
    std::string source = R"(
fn main() -> int {
    return 10 - 3 * 2 + 8 / 4
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    // 10 - (3 * 2) + (8 / 4) = 10 - 6 + 2 = 6
    EXPECT_EQ(result.as.as_i64, 6);
}

TEST_F(VMEndToEndTest, FloatingPointArithmetic) {
    std::string source = R"(
fn main() -> float {
    return 3.14 + 2.86
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_NEAR(result.as.as_f64, 6.0, 0.0001);
}

TEST_F(VMEndToEndTest, MixedTypeArithmetic) {
    std::string source = R"(
fn main() -> float {
    return 5 + 2.5
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_NEAR(result.as.as_f64, 7.5, 0.0001);
}

// ============================================================================
// Function Calls with Parameters
// ============================================================================

TEST_F(VMEndToEndTest, FunctionWithParameters) {
    std::string source = R"(
fn add(a: int, b: int) -> int {
    return a + b
}

fn main() -> int {
    return add(5, 3)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 8);
}

TEST_F(VMEndToEndTest, DirectFunctionCall) {
    std::string source = R"(
fn multiply(x: int, y: int) -> int {
    return x * y
}
    )";

    std::vector<vm::Value> args = {
        vm::Value::makeInt(6),
        vm::Value::makeInt(7)
    };

    vm::Value result = executeSourceWithArgs(source, "multiply", args);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

TEST_F(VMEndToEndTest, NestedFunctionCalls) {
    std::string source = R"(
fn add(a: int, b: int) -> int {
    return a + b
}

fn double(x: int) -> int {
    return x * 2
}

fn main() -> int {
    return double(add(3, 4))
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 14); // (3 + 4) * 2 = 14
}

// ============================================================================
// Local Variables
// ============================================================================

TEST_F(VMEndToEndTest, LocalVariables) {
    std::string source = R"(
fn main() -> int {
    x: int = 10
    y: int = 20
    return x + y
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 30);
}

TEST_F(VMEndToEndTest, LocalVariableReassignment) {
    std::string source = R"(
fn main() -> int {
    x: mut int = 5
    x = x + 10
    return x
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 15);
}

// ============================================================================
// Comparisons and Booleans
// ============================================================================

TEST_F(VMEndToEndTest, BooleanLiterals) {
    std::string source = R"(
fn main() -> bool {
    return true
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::BOOL);
    EXPECT_TRUE(result.as.as_b);
}

TEST_F(VMEndToEndTest, EqualityComparison) {
    std::string source = R"(
fn main() -> bool {
    return 5 == 5
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::BOOL);
    EXPECT_TRUE(result.as.as_b);
}

TEST_F(VMEndToEndTest, InequalityComparison) {
    std::string source = R"(
fn main() -> bool {
    return 3 < 7
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::BOOL);
    EXPECT_TRUE(result.as.as_b);
}

TEST_F(VMEndToEndTest, LogicalOperations) {
    std::string source = R"(
fn main() -> bool {
    return true and false
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::BOOL);
    EXPECT_FALSE(result.as.as_b);
}

// ============================================================================
// Control Flow - If Statements
// ============================================================================

TEST_F(VMEndToEndTest, SimpleIfTrue) {
    std::string source = R"(
fn main() -> int {
    if true {
        return 42
    }
    return 0
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

TEST_F(VMEndToEndTest, SimpleIfFalse) {
    std::string source = R"(
fn main() -> int {
    if false {
        return 42
    }
    return 99
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 99);
}

TEST_F(VMEndToEndTest, IfElseStatement) {
    std::string source = R"(
fn max(a: int, b: int) -> int {
    if a > b {
        return a
    } else {
        return b
    }
}

fn main() -> int {
    return max(10, 5)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 10);
}

TEST_F(VMEndToEndTest, IfElseIfChain) {
    std::string source = R"(
fn classify(x: int) -> int {
    if x < 0 {
        return -1
    } else if x == 0 {
        return 0
    } else {
        return 1
    }
}

fn main() -> int {
    return classify(5)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 1);
}

// ============================================================================
// Control Flow - While Loops
// ============================================================================

TEST_F(VMEndToEndTest, WhileLoop) {
    std::string source = R"(
fn main() -> int {
    sum: mut int = 0
    i: mut int = 1
    while i <= 5 {
        sum = sum + i
        i = i + 1
    }
    return sum
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 15); // 1 + 2 + 3 + 4 + 5 = 15
}

TEST_F(VMEndToEndTest, NestedWhileLoops) {
    std::string source = R"(
fn main() -> int {
    sum: mut int = 0
    i: mut int = 1
    while i <= 3 {
        j: mut int = 1
        while j <= 2 {
            sum = sum + 1
            j = j + 1
        }
        i = i + 1
    }
    return sum
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 6); // 3 iterations * 2 inner iterations = 6
}

// ============================================================================
// Recursion
// ============================================================================

TEST_F(VMEndToEndTest, SimpleRecursion) {
    std::string source = R"(
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fn main() -> int {
    return factorial(5)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 120); // 5! = 120
}

TEST_F(VMEndToEndTest, FibonacciRecursion) {
    std::string source = R"(
fn fib(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

fn main() -> int {
    return fib(7)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 13); // fib(7) = 13
}

// ============================================================================
// Type Conversions
// ============================================================================

TEST_F(VMEndToEndTest, IntToFloatConversion) {
    std::string source = R"(
fn main() -> float {
    x: int = 42
    return x as float
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_NEAR(result.as.as_f64, 42.0, 0.0001);
}

TEST_F(VMEndToEndTest, FloatToIntConversion) {
    std::string source = R"(
fn main() -> int {
    x: float = 3.14
    return x as int
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 3); // Truncation
}

// ============================================================================
// Complex Programs
// ============================================================================

TEST_F(VMEndToEndTest, ComplexCalculation) {
    std::string source = R"(
fn square(x: int) -> int {
    return x * x
}

fn sumOfSquares(a: int, b: int) -> int {
    return square(a) + square(b)
}

fn main() -> int {
    return sumOfSquares(3, 4)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 25); // 3^2 + 4^2 = 9 + 16 = 25
}

TEST_F(VMEndToEndTest, CounterWithMultipleFunctions) {
    std::string source = R"(
fn increment(x: int) -> int {
    return x + 1
}

fn decrement(x: int) -> int {
    return x - 1
}

fn main() -> int {
    counter: mut int = 0
    counter = increment(counter)
    counter = increment(counter)
    counter = increment(counter)
    counter = decrement(counter)
    return counter
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 2); // 0 + 1 + 1 + 1 - 1 = 2
}

TEST_F(VMEndToEndTest, GreatestCommonDivisor) {
    std::string source = R"(
fn gcd(a: int, b: int) -> int {
    a_copy: mut int = a
    b_copy: mut int = b
    while b_copy != 0 {
        temp: int = b_copy
        b_copy = a_copy % b_copy
        a_copy = temp
    }
    return a_copy
}

fn main() -> int {
    return gcd(48, 18)
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 6); // GCD(48, 18) = 6
}

// ============================================================================
// String and GC Tests
// ============================================================================

TEST_F(VMEndToEndTest, StringConstantAllocation) {
    // Test that string constants are allocated via GC
    std::string source = R"(
fn get_message() -> str {
    message: str = "Hello from GC!"
    return message
}

fn main() -> int {
    get_message()
    return 42
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

TEST_F(VMEndToEndTest, MultipleStringAllocations) {
    // Test multiple string allocations to trigger potential GC
    std::string source = R"(
fn create_strings() -> int {
    s1: str = "First string"
    s2: str = "Second string"
    s3: str = "Third string"
    s4: str = "Fourth string"
    s5: str = "Fifth string"
    return 100
}

fn main() -> int {
    return create_strings()
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 100);
}

TEST_F(VMEndToEndTest, StringsAcrossFunctionCalls) {
    // Test that strings survive across function calls
    std::string source = R"(
fn helper() -> str {
    return "helper string"
}

fn main() -> int {
    s1: str = "main string"
    s2: str = helper()
    s3: str = "another string"
    return 999
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 999);
}

TEST_F(VMEndToEndTest, StringPrintIntegration) {
    // Test string printing through the print builtin
    // This test verifies the complete pipeline: allocation -> GC -> printing
    std::string source = R"(
fn main() -> int {
    message: str = "Test message"
    print(message)
    return 0
}
    )";

    // This should not crash - strings should be GC-managed correctly
    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 0);
}

// ============================================================================
// Struct Tests
// ============================================================================

TEST_F(VMEndToEndTest, SimpleStructCreation) {
    // Basic struct creation and field access
    std::string source = R"(
struct Point {
    x: float,
    y: float
}

fn main() -> float {
    p := Point { x: 3.0, y: 4.0 }
    return p.x
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_DOUBLE_EQ(result.as.as_f64, 3.0);
}

TEST_F(VMEndToEndTest, StructFieldUpdate) {
    // Struct field assignment (immutable semantics)
    std::string source = R"(
struct Point {
    x: float,
    y: float
}

fn main() -> float {
    p: mut Point = Point { x: 3.0, y: 4.0 }
    p.x = 10.0
    return p.x
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_DOUBLE_EQ(result.as.as_f64, 10.0);
}

TEST_F(VMEndToEndTest, StructMultipleFieldUpdates) {
    // Update multiple fields sequentially
    std::string source = R"(
struct Point {
    x: float,
    y: float
}

fn main() -> float {
    p: mut Point = Point { x: 1.0, y: 2.0 }
    p.x = 5.0
    p.y = 7.0
    return p.x + p.y
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_DOUBLE_EQ(result.as.as_f64, 12.0);
}

TEST_F(VMEndToEndTest, StructWithMixedTypes) {
    // Struct with int, float, and bool fields
    std::string source = R"(
struct Data {
    count: int,
    value: float,
    flag: bool
}

fn main() -> int {
    d := Data { count: 42, value: 3.14, flag: true }
    return d.count
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

TEST_F(VMEndToEndTest, StructWithBoolField) {
    // Access boolean field from struct
    std::string source = R"(
struct Config {
    enabled: bool,
    count: int
}

fn main() -> int {
    c := Config { enabled: true, count: 10 }
    return c.count
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 10);
}

TEST_F(VMEndToEndTest, NestedStructAccess) {
    // Struct containing another struct
    std::string source = R"(
struct Inner {
    value: int
}

struct Outer {
    inner: Inner,
    extra: int
}

fn main() -> int {
    i := Inner { value: 42 }
    o := Outer { inner: i, extra: 100 }
    return o.inner.value
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 42);
}

TEST_F(VMEndToEndTest, StructFieldArithmetic) {
    // Perform arithmetic on struct fields
    std::string source = R"(
struct Rectangle {
    width: float,
    height: float
}

fn main() -> float {
    r := Rectangle { width: 5.0, height: 3.0 }
    return r.width * r.height
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_DOUBLE_EQ(result.as.as_f64, 15.0);
}

TEST_F(VMEndToEndTest, StructSwapFields) {
    // Swap two fields using a temporary
    std::string source = R"(
struct Pair {
    a: int,
    b: int
}

fn main() -> int {
    p: mut Pair = Pair { a: 10, b: 20 }
    temp := p.a
    p.a = p.b
    p.b = temp
    return p.a
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 20);
}

TEST_F(VMEndToEndTest, StructManyFields) {
    // Edge case: struct with many fields (10+)
    std::string source = R"(
struct BigStruct {
    f0: int,
    f1: int,
    f2: int,
    f3: int,
    f4: int,
    f5: int,
    f6: int,
    f7: int,
    f8: int,
    f9: int
}

fn main() -> int {
    s := BigStruct {
        f0: 0, f1: 1, f2: 2, f3: 3, f4: 4,
        f5: 5, f6: 6, f7: 7, f8: 8, f9: 9
    }
    return s.f7
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 7);
}

TEST_F(VMEndToEndTest, StructZeroValues) {
    // Struct initialized with zero values
    std::string source = R"(
struct Point {
    x: float,
    y: float
}

fn main() -> float {
    p := Point { x: 0.0, y: 0.0 }
    return p.x + p.y
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_DOUBLE_EQ(result.as.as_f64, 0.0);
}

TEST_F(VMEndToEndTest, StructNegativeValues) {
    // Struct with negative values
    std::string source = R"(
struct Vector {
    x: int,
    y: int
}

fn main() -> int {
    v := Vector { x: -5, y: -10 }
    return v.x + v.y
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, -15);
}

TEST_F(VMEndToEndTest, StructFieldReassignment) {
    // Reassign same field multiple times
    std::string source = R"(
struct Counter {
    value: int
}

fn main() -> int {
    c: mut Counter = Counter { value: 0 }
    c.value = 1
    c.value = 2
    c.value = 3
    return c.value
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 3);
}

TEST_F(VMEndToEndTest, StructInConditional) {
    // Use struct field in conditional
    std::string source = R"(
struct Config {
    threshold: int
}

fn main() -> int {
    cfg := Config { threshold: 50 }
    if cfg.threshold > 40 {
        return 1
    }
    return 0
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 1);
}

TEST_F(VMEndToEndTest, StructInLoop) {
    // Access struct field in loop
    std::string source = R"(
struct Range {
    start: int,
    end: int
}

fn main() -> int {
    r := Range { start: 0, end: 5 }
    sum: mut int = 0
    i: mut int = r.start
    while i < r.end {
        sum = sum + i
        i = i + 1
    }
    return sum
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 10);  // 0+1+2+3+4 = 10
}

TEST_F(VMEndToEndTest, StructComparisonFields) {
    // Compare struct fields
    std::string source = R"(
struct Pair {
    a: int,
    b: int
}

fn main() -> int {
    p := Pair { a: 10, b: 20 }
    if p.a < p.b {
        return 1
    }
    return 0
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 1);
}

// Note: Array literal type inference not fully implemented yet
// This test is disabled until array type inference is complete
TEST_F(VMEndToEndTest, DISABLED_StructFieldAsArrayIndex) {
    // Use struct field as array index
    std::string source = R"(
struct Index {
    pos: int
}

fn main() -> int {
    arr: mut [int] = [10, 20, 30, 40, 50]
    idx := Index { pos: 2 }
    return arr[idx.pos]
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 30);
}

TEST_F(VMEndToEndTest, StructWithFloatArithmetic) {
    // Complex float arithmetic on struct fields
    std::string source = R"(
struct Circle {
    radius: float
}

fn main() -> float {
    c := Circle { radius: 2.0 }
    # Area = πr² (approximating π as 3.14)
    return 3.14 * c.radius * c.radius
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::FLOAT64);
    EXPECT_NEAR(result.as.as_f64, 12.56, 0.01);
}

TEST_F(VMEndToEndTest, StructIncrementField) {
    // Increment struct field value
    std::string source = R"(
struct Counter {
    count: int
}

fn main() -> int {
    c: mut Counter = Counter { count: 5 }
    c.count = c.count + 1
    return c.count
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 6);
}

TEST_F(VMEndToEndTest, StructFieldCopySemantics) {
    // Verify fields are copied, not referenced
    std::string source = R"(
struct Point {
    x: int,
    y: int
}

fn main() -> int {
    p1 := Point { x: 10, y: 20 }
    val := p1.x
    p1_mut: mut Point = p1
    p1_mut.x = 99
    # val should still be 10 (not 99)
    return val
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 10);
}

TEST_F(VMEndToEndTest, StructTwoInstances) {
    // Two separate struct instances
    std::string source = R"(
struct Point {
    x: int,
    y: int
}

fn main() -> int {
    p1 := Point { x: 1, y: 2 }
    p2 := Point { x: 3, y: 4 }
    return p1.x + p2.y
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 5);
}

TEST_F(VMEndToEndTest, StructFieldMax) {
    // Find max of two struct fields
    std::string source = R"(
struct Pair {
    a: int,
    b: int
}

fn main() -> int {
    p := Pair { a: 100, b: 200 }
    if p.a > p.b {
        return p.a
    }
    return p.b
}
    )";

    vm::Value result = executeSource(source);
    EXPECT_EQ(result.type, vm::ValueType::INT64);
    EXPECT_EQ(result.as.as_i64, 200);
}