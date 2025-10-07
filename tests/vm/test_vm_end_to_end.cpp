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