#include <gtest/gtest.h>
#include "../../include/Parser/Parser.hpp"
#include "../../include/Lexer/Lexer.hpp"
#include "../../include/HIR/Lowering.hpp"
#include "../../include/Semantic/SemanticAnalyzer.hpp"
#include "../../include/MIR/HIRToMIR.hpp"
#include "../../include/MIR/MIRVerifier.hpp"
#include "../../include/Codegen/MIRToLLVM.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include "../../include/Error/Error.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>

// Comprehensive integration tests for full compilation pipeline:
// Source → Lex → Parse → AST → HIR → Semantic Analysis → MIR → LLVM IR

class FullPipelineTest : public ::testing::Test {
protected:
    Type::TypeRegistry types;
    DiagnosticManager diag;
    std::unique_ptr<llvm::LLVMContext> llvmContext;

    FullPipelineTest() : diag(false) {
        llvmContext = std::make_unique<llvm::LLVMContext>();
    }

    std::unique_ptr<llvm::Module> compileToLLVM(const std::string& source, const std::string& moduleName = "test_module") {
        // Phase 1: Lexical Analysis
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();
        if (diag.hasErrors()) {
            return nullptr;
        }

        // Phase 2: Parsing (AST generation)
        Parser parser(tokens, types, diag);
        auto ast = parser.parseProgram();
        if (!ast || diag.hasErrors()) {
            return nullptr;
        }

        // Phase 3: HIR Lowering
        HIRLowering hirLowering(types);
        auto hir = hirLowering.lower(*ast);
        if (diag.hasErrors()) {
            return nullptr;
        }

        // Phase 4: Semantic Analysis
        Semantic::SemanticAnalyzer analyzer(types, diag);
        analyzer.analyzeProgram(hir);
        if (diag.hasErrors()) {
            return nullptr;
        }

        // Phase 5: MIR Lowering
        MIR::HIRToMIR mirLowering(types, diag, analyzer.getExprTypes());
        auto mir = mirLowering.lower(hir);
        if (diag.hasErrors()) {
            return nullptr;
        }

        // Phase 6: MIR Verification (optional but recommended)
        MIR::MIRVerifier verifier(diag);
        if (!verifier.verify(mir)) {
            return nullptr;
        }

        // Phase 7: LLVM IR Codegen
        auto module = std::make_unique<llvm::Module>(moduleName, *llvmContext);
        Codegen::MIRToLLVM codegen(*llvmContext, *module, types);
        codegen.lower(mir);

        // Phase 8: LLVM IR Verification
        std::string error;
        llvm::raw_string_ostream errorStream(error);
        if (llvm::verifyModule(*module, &errorStream)) {
            diag.error("LLVM IR verification failed: " + error, 0, 0);
            return nullptr;
        }

        return module;
    }

    bool hasFunction(llvm::Module* module, const std::string& name) {
        return module->getFunction(name) != nullptr;
    }

    llvm::Function* getFunction(llvm::Module* module, const std::string& name) {
        return module->getFunction(name);
    }
};

// ============================================================================
// Simple Programs - End to End
// ============================================================================

TEST_F(FullPipelineTest, HelloWorldProgram) {
    std::string source = R"(
fn main() -> i32 {
    return 0;
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "main"));

    auto* mainFn = getFunction(module.get(), "main");
    ASSERT_NE(mainFn, nullptr);
    EXPECT_EQ(mainFn->arg_size(), 0);
}

TEST_F(FullPipelineTest, SimpleArithmetic) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> i32 {
    return add(10, 20);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "add"));
    EXPECT_TRUE(hasFunction(module.get(), "main"));

    auto* addFn = getFunction(module.get(), "add");
    ASSERT_NE(addFn, nullptr);
    EXPECT_EQ(addFn->arg_size(), 2);
}

// ============================================================================
// Control Flow Programs
// ============================================================================

TEST_F(FullPipelineTest, IfElseStatement) {
    std::string source = R"(
fn abs(x: i32) -> i32 {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}

fn main() -> i32 {
    return abs(-5);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* absFn = getFunction(module.get(), "abs");
    ASSERT_NE(absFn, nullptr);

    // Should have multiple basic blocks for if/else
    EXPECT_GT(absFn->size(), 1);
}

TEST_F(FullPipelineTest, WhileLoop) {
    std::string source = R"(
fn sum_to_n(n: i32) -> i32 {
    let mut sum: i32 = 0;
    let mut i: i32 = 1;
    while (i <= n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

fn main() -> i32 {
    return sum_to_n(10);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* sumFn = getFunction(module.get(), "sum_to_n");
    ASSERT_NE(sumFn, nullptr);

    // While loops create multiple basic blocks
    EXPECT_GT(sumFn->size(), 1);
}

TEST_F(FullPipelineTest, ForLoop) {
    std::string source = R"(
fn sum_range() -> i32 {
    let mut total: i32 = 0;
    for i in 0..10 {
        total = total + i;
    }
    return total;
}

fn main() -> i32 {
    return sum_range();
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "sum_range"));
}

TEST_F(FullPipelineTest, NestedLoops) {
    std::string source = R"(
fn nested_sum() -> i32 {
    let mut total: i32 = 0;
    for i in 0..5 {
        for j in 0..5 {
            total = total + i + j;
        }
    }
    return total;
}

fn main() -> i32 {
    return nested_sum();
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* fn = getFunction(module.get(), "nested_sum");
    ASSERT_NE(fn, nullptr);

    // Nested loops create many basic blocks
    EXPECT_GT(fn->size(), 2);
}

// ============================================================================
// Recursive Functions
// ============================================================================

TEST_F(FullPipelineTest, RecursiveFactorial) {
    std::string source = R"(
fn factorial(n: i32) -> i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn main() -> i32 {
    return factorial(5);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* factFn = getFunction(module.get(), "factorial");
    ASSERT_NE(factFn, nullptr);
    EXPECT_EQ(factFn->arg_size(), 1);
}

TEST_F(FullPipelineTest, RecursiveFibonacci) {
    std::string source = R"(
fn fibonacci(n: i32) -> i32 {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() -> i32 {
    return fibonacci(10);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "fibonacci"));
    EXPECT_TRUE(hasFunction(module.get(), "main"));
}

// ============================================================================
// Array Operations
// ============================================================================

TEST_F(FullPipelineTest, ArrayDeclarationAndAccess) {
    std::string source = R"(
fn test_arrays() -> i32 {
    let arr: [i32; 5] = [10, 20, 30, 40, 50];
    return arr[2];
}

fn main() -> i32 {
    return test_arrays();
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "test_arrays"));
}

TEST_F(FullPipelineTest, ArrayMutation) {
    std::string source = R"(
fn modify_array() -> i32 {
    let mut arr: [i32; 3] = [1, 2, 3];
    arr[0] = 10;
    arr[1] = 20;
    return arr[0] + arr[1];
}

fn main() -> i32 {
    return modify_array();
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* fn = getFunction(module.get(), "modify_array");
    ASSERT_NE(fn, nullptr);
}

// ============================================================================
// Multiple Functions & Complex Interactions
// ============================================================================

TEST_F(FullPipelineTest, MultipleFunctionsWithCalls) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn subtract(a: i32, b: i32) -> i32 {
    return a - b;
}

fn multiply(a: i32, b: i32) -> i32 {
    return a * b;
}

fn compute(x: i32, y: i32) -> i32 {
    let sum := add(x, y);
    let diff := subtract(x, y);
    return multiply(sum, diff);
}

fn main() -> i32 {
    return compute(10, 5);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "add"));
    EXPECT_TRUE(hasFunction(module.get(), "subtract"));
    EXPECT_TRUE(hasFunction(module.get(), "multiply"));
    EXPECT_TRUE(hasFunction(module.get(), "compute"));
    EXPECT_TRUE(hasFunction(module.get(), "main"));
}

// ============================================================================
// Stress Tests - Large Programs
// ============================================================================

TEST_F(FullPipelineTest, BubbleSortAlgorithm) {
    std::string source = R"(
fn bubble_sort(arr: mut [i32; 10]) -> [i32; 10] {
    for i in 0..10 {
        for j in 0..9 {
            if (arr[j] > arr[j + 1]) {
                let temp := arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    return arr;
}

fn main() -> i32 {
    let mut data: [i32; 10] = [9, 7, 5, 3, 1, 0, 2, 4, 6, 8];
    data = bubble_sort(data);
    return data[0];
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* sortFn = getFunction(module.get(), "bubble_sort");
    ASSERT_NE(sortFn, nullptr);

    // Complex algorithm should have many basic blocks
    EXPECT_GT(sortFn->size(), 3);
}

TEST_F(FullPipelineTest, MatrixMultiplication) {
    std::string source = R"(
fn matrix_mult(a: [i32; 4], b: [i32; 4]) -> [i32; 4] {
    let mut result: [i32; 4] = [0, 0, 0, 0];
    result[0] = a[0] * b[0] + a[1] * b[2];
    result[1] = a[0] * b[1] + a[1] * b[3];
    result[2] = a[2] * b[0] + a[3] * b[2];
    result[3] = a[2] * b[1] + a[3] * b[3];
    return result;
}

fn main() -> i32 {
    let a: [i32; 4] = [1, 2, 3, 4];
    let b: [i32; 4] = [5, 6, 7, 8];
    let c := matrix_mult(a, b);
    return c[0];
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    EXPECT_TRUE(hasFunction(module.get(), "matrix_mult"));
    EXPECT_TRUE(hasFunction(module.get(), "main"));
}

// ============================================================================
// Type System Stress Tests
// ============================================================================

TEST_F(FullPipelineTest, MixedIntegerTypes) {
    std::string source = R"(
fn test_types() -> i64 {
    let a: i8 = 10;
    let b: i16 = 100;
    let c: i32 = 1000;
    let d: i64 = 10000;
    return d;
}

fn main() -> i32 {
    let x := test_types();
    return 0;
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* testFn = getFunction(module.get(), "test_types");
    ASSERT_NE(testFn, nullptr);
}

TEST_F(FullPipelineTest, FloatOperations) {
    std::string source = R"(
fn compute_area(radius: f32) -> f32 {
    let pi: f32 = 3.14159;
    return pi * radius * radius;
}

fn main() -> i32 {
    let radius: f32 = 5.0;
    let area := compute_area(radius);
    return 0;
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* areaFn = getFunction(module.get(), "compute_area");
    ASSERT_NE(areaFn, nullptr);
}

// ============================================================================
// Complex Nested Structures
// ============================================================================

TEST_F(FullPipelineTest, DeeplyNestedIfElse) {
    std::string source = R"(
fn classify(x: i32) -> i32 {
    if (x < 0) {
        if (x < -100) {
            return -2;
        } else {
            return -1;
        }
    } else {
        if (x > 100) {
            return 2;
        } else {
            return 1;
        }
    }
}

fn main() -> i32 {
    return classify(50);
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    auto* classifyFn = getFunction(module.get(), "classify");
    ASSERT_NE(classifyFn, nullptr);

    // Deeply nested if/else creates many basic blocks
    EXPECT_GT(classifyFn->size(), 4);
}

// ============================================================================
// Pipeline Validation - Error Detection
// ============================================================================

TEST_F(FullPipelineTest, DetectTypeErrors) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 10;
    let y: bool = x;  // Type error: i32 cannot be assigned to bool
    return 0;
}
)";

    auto module = compileToLLVM(source);

    // This should fail semantic analysis
    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(diag.hasErrors());
}

TEST_F(FullPipelineTest, DetectUndefinedVariable) {
    std::string source = R"(
fn main() -> i32 {
    return undefined_var;  // Error: undefined variable
}
)";

    auto module = compileToLLVM(source);

    EXPECT_EQ(module, nullptr);
    EXPECT_TRUE(diag.hasErrors());
}

// ============================================================================
// Performance Test - Many Functions
// ============================================================================

TEST_F(FullPipelineTest, ManySmallFunctions) {
    std::string source = R"(
fn f1() -> i32 { return 1; }
fn f2() -> i32 { return 2; }
fn f3() -> i32 { return 3; }
fn f4() -> i32 { return 4; }
fn f5() -> i32 { return 5; }
fn f6() -> i32 { return 6; }
fn f7() -> i32 { return 7; }
fn f8() -> i32 { return 8; }
fn f9() -> i32 { return 9; }
fn f10() -> i32 { return 10; }

fn main() -> i32 {
    return f1() + f2() + f3() + f4() + f5() + f6() + f7() + f8() + f9() + f10();
}
)";

    auto module = compileToLLVM(source);

    ASSERT_NE(module, nullptr);
    ASSERT_FALSE(diag.hasErrors());

    // Should have all 11 functions
    for (int i = 1; i <= 10; i++) {
        EXPECT_TRUE(hasFunction(module.get(), "f" + std::to_string(i)));
    }
    EXPECT_TRUE(hasFunction(module.get(), "main"));
}
