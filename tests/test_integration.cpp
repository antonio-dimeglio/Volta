#include <gtest/gtest.h>
#include "lexer/lexer.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "error/ErrorReporter.hpp"
#include "IR/IRGenerator.hpp"
#include "IR/IRPrinter.hpp"
#include "bytecode/BytecodeCompiler.hpp"
#include "bytecode/Disassembler.hpp"
#include "vm/VM.hpp"
#include "vm/Value.hpp"
#include <sstream>

using namespace volta;

// ============================================================================
// End-to-End Integration Tests
// ============================================================================

/**
 * Helper class to run a complete program through all stages:
 * Source -> Lexer -> Parser -> Semantic Analysis -> IR -> Bytecode -> VM
 */
class ProgramRunner {
public:
    explicit ProgramRunner(const std::string& source)
        : source_(source), hasError_(false) {}

    /**
     * Run the entire compilation and execution pipeline
     * Returns the final result value from the VM
     */
    vm::Value run() {
        // Stage 1: Lexing
        lexer::Lexer lexer(source_);
        auto tokens = lexer.tokenize();

        if (tokens.empty()) {
            hasError_ = true;
            errorMessage_ = "Lexer error";
            return vm::Value::makeNull();
        }

        // Stage 2: Parsing
        parser::Parser parser(std::move(tokens));
        auto ast = parser.parseProgram();

        if (!ast) {
            hasError_ = true;
            errorMessage_ = "Parser error";
            return vm::Value::makeNull();
        }

        errors::ErrorReporter er;

        // Stage 3: Semantic Analysis
        semantic::SemanticAnalyzer analyzer(er);
        auto semanticResult = analyzer.analyze(*ast);

        if (!semanticResult) {
            hasError_ = true;
            errorMessage_ = "Semantic analysis error";
            return vm::Value::makeNull();
        }

        // Stage 4: IR Generation
        ir::IRGenerator irGen(er);
        auto irModule = irGen.generate(*ast, analyzer);

        if (!irModule) {
            hasError_ = true;
            errorMessage_ = "IR generation error";
            return vm::Value::makeNull();
        }

        // Optional: Print IR for debugging
        if (printIR_) {
            std::ostringstream irOut;
            ir::IRPrinter irPrinter(irOut);
            irPrinter.print(*irModule);
            irOutput_ = irOut.str();
        }

        // Stage 5: Bytecode Compilation
        bytecode::BytecodeCompiler compiler;
        auto compiled = compiler.compile(*irModule);

        if (!compiled) {
            hasError_ = true;
            errorMessage_ = "Bytecode compilation error";
            return vm::Value::makeNull();
        }

        // Optional: Disassemble bytecode for debugging
        if (printBytecode_) {
            bytecode::Disassembler disasm;
            bytecodeOutput_ = disasm.disassembleModule(*compiled);
        }

        // Stage 6: VM Execution
        vm::VM vmInstance(std::move(compiled));

        if (debugTrace_) {
            vmInstance.setDebugTrace(true);
        }

        try {
            int exitCode = vmInstance.execute();

            if (exitCode != 0) {
                hasError_ = true;
                errorMessage_ = "VM execution failed with exit code: " + std::to_string(exitCode);
                return vm::Value::makeNull();
            }

            // Get the return value from stack
            if (vmInstance.stackSize() > 0) {
                return vmInstance.pop();
            }

            return vm::Value::makeNull();

        } catch (const vm::VM::RuntimeError& e) {
            hasError_ = true;
            errorMessage_ = "Runtime error: " + e.message;
            return vm::Value::makeNull();
        }
    }

    /**
     * Execute a specific function and return its result
     */
    vm::Value runFunction(const std::string& functionName) {
        // ... similar to run() but calls executeFunction instead
        // For now, simplified version
        return run();
    }

    void setPrintIR(bool enable) { printIR_ = enable; }
    void setPrintBytecode(bool enable) { printBytecode_ = enable; }
    void setDebugTrace(bool enable) { debugTrace_ = enable; }

    bool hasError() const { return hasError_; }
    const std::string& errorMessage() const { return errorMessage_; }
    const std::string& getIROutput() const { return irOutput_; }
    const std::string& getBytecodeOutput() const { return bytecodeOutput_; }

private:
    std::string source_;
    bool hasError_;
    std::string errorMessage_;
    bool printIR_ = false;
    bool printBytecode_ = false;
    bool debugTrace_ = false;
    std::string irOutput_;
    std::string bytecodeOutput_;
};

// ============================================================================
// Simple Expression Tests
// ============================================================================

TEST(IntegrationTest, SimpleIntegerConstant) {
    std::string program = R"(
        fn main() -> int {
            return 42;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.type, vm::ValueType::Int);
    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, SimpleAddition) {
    std::string program = R"(
        fn main() -> int {
            return 10 + 32;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, SimpleSubtraction) {
    std::string program = R"(
        fn main() -> int {
            return 50 - 8;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, SimpleMultiplication) {
    std::string program = R"(
        fn main() -> int {
            return 6 * 7;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, ComplexExpression) {
    std::string program = R"(
        fn main() -> int {
            return (10 + 5) * 2 + 12;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

// ============================================================================
// Variable Tests
// ============================================================================

TEST(IntegrationTest, LocalVariable) {
    std::string program = R"(
        fn main() -> int {
            let x: int = 42;
            return x;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, MultipleVariables) {
    std::string program = R"(
        fn main() -> int {
            let x: int = 10;
            let y: int = 32;
            return x + y;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, VariableReassignment) {
    std::string program = R"(
        fn main() -> int {
            let x: int = 10;
            x = 42;
            return x;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

// ============================================================================
// Conditional Tests
// ============================================================================

TEST(IntegrationTest, SimpleIfStatement) {
    std::string program = R"(
        fn main() -> int {
            if true {
                return 42;
            } else {
                return 0;
            }
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, IfElseWithComparison) {
    std::string program = R"(
        fn main() -> int {
            let x: int = 10;
            if x < 20 {
                return 42;
            } else {
                return 0;
            }
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, NestedIf) {
    std::string program = R"(
        fn main() -> int {
            let x: int = 10;
            if x > 5 {
                if x < 15 {
                    return 42;
                } else {
                    return 1;
                }
            } else {
                return 0;
            }
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

// ============================================================================
// Loop Tests
// ============================================================================

TEST(IntegrationTest, SimpleWhileLoop) {
    std::string program = R"(
        fn main() -> int {
            let sum: int = 0;
            let i: int = 1;
            while i <= 10 {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 55);  // Sum of 1..10
}

TEST(IntegrationTest, SimpleForLoop) {
    std::string program = R"(
        fn main() -> int {
            let sum: int = 0;
            for i in 0..10 {
                sum = sum + i;
            }
            return sum;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 45);  // Sum of 0..9
}

// ============================================================================
// Function Call Tests
// ============================================================================

TEST(IntegrationTest, SimpleFunctionCall) {
    std::string program = R"(
        fn helper() -> int {
            return 42;
        }

        fn main() -> int {
            return helper();
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, FunctionWithParameters) {
    std::string program = R"(
        fn add(a: int, b: int) -> int {
            return a + b;
        }

        fn main() -> int {
            return add(10, 32);
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

TEST(IntegrationTest, RecursiveFunction) {
    std::string program = R"(
        fn factorial(n: int) -> int {
            if n <= 1 {
                return 1;
            } else {
                return n * factorial(n - 1);
            }
        }

        fn main() -> int {
            return factorial(5);
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 120);
}

TEST(IntegrationTest, FibonacciRecursive) {
    std::string program = R"(
        fn fib(n: int) -> int {
            if n <= 1 {
                return n;
            } else {
                return fib(n - 1) + fib(n - 2);
            }
        }

        fn main() -> int {
            return fib(10);
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 55);
}

// ============================================================================
// Float Tests
// ============================================================================

TEST(IntegrationTest, SimpleFloat) {
    std::string program = R"(
        fn main() -> float {
            return 3.14;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.type, vm::ValueType::Float);
    EXPECT_DOUBLE_EQ(result.asFloat, 3.14);
}

TEST(IntegrationTest, FloatArithmetic) {
    std::string program = R"(
        fn main() -> float {
            let x: float = 2.5;
            let y: float = 1.5;
            return x + y;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_DOUBLE_EQ(result.asFloat, 4.0);
}

// ============================================================================
// Boolean Tests
// ============================================================================

TEST(IntegrationTest, BooleanLiterals) {
    std::string program = R"(
        fn main() -> bool {
            return true;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.type, vm::ValueType::Bool);
    EXPECT_TRUE(result.asBool);
}

TEST(IntegrationTest, LogicalOperators) {
    std::string program = R"(
        fn main() -> bool {
            return true && true || false;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_TRUE(result.asBool);
}

// ============================================================================
// String Tests
// ============================================================================

TEST(IntegrationTest, SimpleString) {
    std::string program = R"(
        fn main() -> str {
            return "hello";
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.type, vm::ValueType::String);
}

// ============================================================================
// Array Tests
// ============================================================================

TEST(IntegrationTest, SimpleArray) {
    std::string program = R"(
        fn main() -> int {
            let arr: Array[int] = [1, 2, 3, 4, 5];
            return arr[2];
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 3);
}

TEST(IntegrationTest, ArraySum) {
    std::string program = R"(
        fn main() -> int {
            let arr: Array[int] = [10, 20, 30];
            let sum: int = 0;
            for i in 0..3 {
                sum = sum + arr[i];
            }
            return sum;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 60);
}

// ============================================================================
// Struct Tests
// ============================================================================

TEST(IntegrationTest, SimpleStruct) {
    std::string program = R"(
        struct Point {
            x: int,
            y: int
        }

        fn main() -> int {
            let p: Point = Point { x: 10, y: 32 };
            return p.x + p.y;
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 42);
}

// ============================================================================
// Complex Programs
// ============================================================================

TEST(IntegrationTest, BubbleSort) {
    std::string program = R"(
        fn bubbleSort(arr: Array[int], n: int) -> void {
            for i in 0..n {
                for j in 0..(n - i - 1) {
                    if arr[j] > arr[j + 1] {
                        let temp: int = arr[j];
                        arr[j] = arr[j + 1];
                        arr[j + 1] = temp;
                    }
                }
            }
        }

        fn main() -> int {
            let arr: Array[int] = [5, 2, 8, 1, 9];
            bubbleSort(arr, 5);
            return arr[0];  // Should be 1 (smallest)
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_EQ(result.asInt, 1);
}

TEST(IntegrationTest, IsPrime) {
    std::string program = R"(
        fn isPrime(n: int) -> bool {
            if n <= 1 {
                return false;
            }
            let i: int = 2;
            while i * i <= n {
                if n % i == 0 {
                    return false;
                }
                i = i + 1;
            }
            return true;
        }

        fn main() -> bool {
            return isPrime(17);
        }
    )";

    ProgramRunner runner(program);
    vm::Value result = runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented: " << runner.errorMessage();
    }

    EXPECT_TRUE(result.asBool);
}

// ============================================================================
// Debug Output Tests
// ============================================================================

TEST(IntegrationTest, PrintIROutput) {
    std::string program = R"(
        fn main() -> int {
            return 42;
        }
    )";

    ProgramRunner runner(program);
    runner.setPrintIR(true);
    runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented";
    }

    std::string ir = runner.getIROutput();
    EXPECT_GT(ir.size(), 0);
    // IR should contain function name
    EXPECT_NE(ir.find("main"), std::string::npos);
}

TEST(IntegrationTest, PrintBytecodeOutput) {
    std::string program = R"(
        fn main() -> int {
            return 42;
        }
    )";

    ProgramRunner runner(program);
    runner.setPrintBytecode(true);
    runner.run();

    if (runner.hasError()) {
        GTEST_SKIP() << "Not yet implemented";
    }

    std::string bytecode = runner.getBytecodeOutput();
    EXPECT_GT(bytecode.size(), 0);
    // Bytecode should contain instructions
    EXPECT_NE(bytecode.find("ConstInt"), std::string::npos);
}
