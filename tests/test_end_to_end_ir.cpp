#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "lexer/lexer.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "IR/IRGenerator.hpp"
#include "IR/IRPrinter.hpp"
#include "IR/Verifier.hpp"
#include "error/ErrorReporter.hpp"

using namespace volta;
using namespace volta::lexer;
using namespace volta::parser;
using namespace volta::semantic;
using namespace volta::ir;
using namespace volta::errors;

/**
 * End-to-End IR Generation Tests
 *
 * Tests the complete compilation pipeline:
 * Source Code → Lexer → Parser → Semantic Analysis → IR Generation
 */

class EndToEndIRTest : public ::testing::Test {
protected:
    std::unique_ptr<ir::Module> compileSource(const std::string& source) {
        // 1. Lex
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // 2. Parse
        Parser parser(tokens);
        auto ast = parser.parseProgram();
        if (!ast) {
            std::cerr << "Parse failed\n";
            return nullptr;
        }

        // 3. Semantic Analysis
        ErrorReporter semanticReporter;
        SemanticAnalyzer analyzer(semanticReporter);
        analyzer.registerBuiltins();

        if (!analyzer.analyze(*ast)) {
            std::cerr << "Semantic errors:\n";
            semanticReporter.printErrors(std::cerr);
            return nullptr;
        }

        // 4. IR Generation
        auto module = generateIR(*ast, analyzer, "test_module");
        if (!module) {
            std::cerr << "IR generation failed\n";
            return nullptr;
        }

        // 5. Verify IR
        Verifier verifier;
        if (!verifier.verify(*module)) {
            std::cerr << "IR verification failed:\n";
            for (const auto& error : verifier.getErrors()) {
                std::cerr << "  - " << error << "\n";
            }
            return nullptr;
        }

        return module;
    }

    std::string getIRAsString(const ir::Module& module) {
        IRPrinter printer;
        return printer.printModule(module);
    }
};

// ============================================================================
// Basic Function Tests
// ============================================================================

TEST_F(EndToEndIRTest, SimpleFunction) {
    std::string source = R"(
fn add(a: int, b: int) -> int {
    return a + b
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("add");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getNumParams(), 2);
    EXPECT_EQ(func->getReturnType()->kind(), IRType::Kind::I64);
}

TEST_F(EndToEndIRTest, FunctionWithLocalVariable) {
    std::string source = R"(
fn calculate(x: int) -> int {
    result: int = x * 2
    return result
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("calculate");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getNumParams(), 1);
}

TEST_F(EndToEndIRTest, FunctionWithTypeInference) {
    std::string source = R"(
fn square(n: int) -> int {
    result := n * n
    return result
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("square");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// Control Flow Tests
// ============================================================================

TEST_F(EndToEndIRTest, IfElseStatement) {
    std::string source = R"(
fn max(a: int, b: int) -> int {
    if (a > b) {
        return a
    } else {
        return b
    }
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("max");
    ASSERT_NE(func, nullptr);

    // Should have entry + then + else blocks
    EXPECT_GE(func->getNumBlocks(), 3);
}

TEST_F(EndToEndIRTest, WhileLoop) {
    std::string source = R"(
fn count_to(n: int) -> int {
    i: mut int = 0
    while (i < n) {
        i = i + 1
    }
    return i
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("count_to");
    ASSERT_NE(func, nullptr);

    // Should have entry + header + body + exit blocks
    EXPECT_GE(func->getNumBlocks(), 4);
}

TEST_F(EndToEndIRTest, NestedIfStatements) {
    std::string source = R"(
fn classify(x: int) -> int {
    if (x > 0) {
        if (x > 100) {
            return 2
        } else {
            return 1
        }
    } else {
        return 0
    }
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("classify");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// Arithmetic and Expression Tests
// ============================================================================

TEST_F(EndToEndIRTest, ComplexArithmetic) {
    std::string source = R"(
fn formula(x: int, y: int) -> int {
    return (x + y) * (x - y)
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("formula");
    ASSERT_NE(func, nullptr);
}

TEST_F(EndToEndIRTest, MultipleOperations) {
    std::string source = R"(
fn compute(a: int, b: int, c: int) -> int {
    x := a + b
    y := b * c
    z := x - y
    return z
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);
}

// ============================================================================
// Multiple Functions
// ============================================================================

TEST_F(EndToEndIRTest, MultipleFunctions) {
    std::string source = R"(
fn helper(x: int) -> int {
    return x * 2
}

fn main(n: int) -> int {
    return helper(n + 1)
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    EXPECT_NE(module->getFunction("helper"), nullptr);
    EXPECT_NE(module->getFunction("main"), nullptr);
    // Note: getNumFunctions() returns 3 because we auto-register 'print' builtin
    EXPECT_EQ(module->getNumFunctions(), 3);
}

TEST_F(EndToEndIRTest, RecursiveFunction) {
    std::string source = R"(
fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1
    } else {
        return n * factorial(n - 1)
    }
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("factorial");
    ASSERT_NE(func, nullptr);
}

// ============================================================================
// Type System Tests
// ============================================================================

TEST_F(EndToEndIRTest, BooleanTypes) {
    std::string source = R"(
fn is_positive(x: int) -> bool {
    return x > 0
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("is_positive");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getReturnType()->kind(), IRType::Kind::I1);
}

TEST_F(EndToEndIRTest, FloatTypes) {
    std::string source = R"(
fn add_floats(a: float, b: float) -> float {
    return a + b
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("add_floats");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getReturnType()->kind(), IRType::Kind::F64);
}

TEST_F(EndToEndIRTest, MixedTypes) {
    std::string source = R"(
fn process(x: int, flag: bool) -> int {
    if (flag) {
        return x * 2
    } else {
        return x
    }
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);
}

// ============================================================================
// Complex Programs
// ============================================================================

TEST_F(EndToEndIRTest, FibonacciIterative) {
    std::string source = R"(
fn fib(n: int) -> int {
    a: mut int = 0
    b: mut int = 1
    i: mut int = 0

    while (i < n) {
        temp := a + b
        a = b
        b = temp
        i = i + 1
    }

    return a
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    auto* func = module->getFunction("fib");
    ASSERT_NE(func, nullptr);
}

TEST_F(EndToEndIRTest, GCDAlgorithm) {
    std::string source = R"(
fn gcd(a: int, b: int) -> int {
    x: mut int = a
    y: mut int = b

    while (y != 0) {
        temp := y
        y = x % y
        x = temp
    }

    return x
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);
}

// ============================================================================
// IR Verification Tests
// ============================================================================

TEST_F(EndToEndIRTest, VerifySSAForm) {
    std::string source = R"(
fn test(x: int) -> int {
    a := x + 1
    b := a * 2
    return b
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    // Module should pass verification
    Verifier verifier;
    EXPECT_TRUE(verifier.verify(*module));
}

TEST_F(EndToEndIRTest, VerifyControlFlow) {
    std::string source = R"(
fn branch(x: int, y: int) -> int {
    if (x > y) {
        return x
    } else {
        return y
    }
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    Verifier verifier;
    EXPECT_TRUE(verifier.verify(*module));
}

// ============================================================================
// IR Output Format Tests
// ============================================================================

TEST_F(EndToEndIRTest, IROutputFormat) {
    std::string source = R"(
fn simple(x: int) -> int {
    return x + 1
}
    )";

    auto module = compileSource(source);
    ASSERT_NE(module, nullptr);

    std::string ir = getIRAsString(*module);

    // Check that IR contains expected elements
    EXPECT_NE(ir.find("define"), std::string::npos);
    EXPECT_NE(ir.find("simple"), std::string::npos);
    EXPECT_NE(ir.find("ret"), std::string::npos);
}
