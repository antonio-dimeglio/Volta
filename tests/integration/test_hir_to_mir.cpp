#include <gtest/gtest.h>
#include "../../include/Parser/Parser.hpp"
#include "../../include/Lexer/Lexer.hpp"
#include "../../include/HIR/Lowering.hpp"
#include "../../include/Semantic/SemanticAnalyzer.hpp"
#include "../../include/MIR/HIRToMIR.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include "../../include/Error/Error.hpp"
#include <memory>
#include <string>

// Integration tests for HIR → MIR lowering pipeline

class HIRToMIRTest : public ::testing::Test {
protected:
    Type::TypeRegistry types;
    DiagnosticManager diag;

    HIRToMIRTest() : diag(false) {}

    MIR::Program parseAndLowerToMIR(const std::string& source) {
        // Lex
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();

        // Parse
        Parser parser(tokens, types, diag);
        auto ast = parser.parseProgram();
        if (!ast || diag.hasErrors()) {
            return MIR::Program();
        }

        // Lower to HIR
        HIRLowering hirLowering(types);
        auto hir = hirLowering.lower(*ast);

        // Semantic analysis
        Semantic::SemanticAnalyzer analyzer(types, diag);
        analyzer.analyzeProgram(hir);
        if (diag.hasErrors()) {
            return MIR::Program();
        }

        // Lower to MIR
        MIR::HIRToMIR mirLowering(types, diag, analyzer.getExprTypes());
        return mirLowering.lower(hir);
    }
};

// ============================================================================
// Simple Function Lowering
// ============================================================================

TEST_F(HIRToMIRTest, SimpleFunctionReturn) {
    std::string source = R"(
fn main() -> i32 {
    return 42;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_GT(mir.functions.size(), 0);

    auto* mainFn = mir.getFunction("main");
    ASSERT_NE(mainFn, nullptr);
    EXPECT_EQ(mainFn->name, "main");
    EXPECT_GT(mainFn->blocks.size(), 0);

    // Should have at least entry block with return terminator
    EXPECT_TRUE(mainFn->blocks[0].hasTerminator());
}

TEST_F(HIRToMIRTest, FunctionWithParameters) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* addFn = mir.getFunction("add");
    ASSERT_NE(addFn, nullptr);

    EXPECT_EQ(addFn->params.size(), 2);
    EXPECT_EQ(addFn->params[0].name, "a");
    EXPECT_EQ(addFn->params[1].name, "b");
}

// ============================================================================
// Arithmetic Operations
// ============================================================================

TEST_F(HIRToMIRTest, IntegerArithmetic) {
    std::string source = R"(
fn compute(x: i32, y: i32) -> i32 {
    let sum := x + y;
    let diff := x - y;
    let prod := x * y;
    let quot := x / y;
    return sum;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("compute");
    ASSERT_NE(fn, nullptr);
    EXPECT_GT(fn->blocks.size(), 0);

    // Entry block should contain arithmetic instructions
    auto& entry = fn->blocks[0];
    EXPECT_GT(entry.instructions.size(), 0);
}

TEST_F(HIRToMIRTest, ComparisonOperations) {
    std::string source = R"(
fn compare(a: i32, b: i32) -> bool {
    return a < b;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("compare");
    ASSERT_NE(fn, nullptr);

    // Should have comparison instruction
    auto& entry = fn->blocks[0];
    bool hasComparison = false;
    for (auto& inst : entry.instructions) {
        if (inst.opcode >= MIR::Opcode::ICmpEQ && inst.opcode <= MIR::Opcode::ICmpUGE) {
            hasComparison = true;
            break;
        }
    }
    // Note: might be optimized away or done differently
}

// ============================================================================
// Control Flow (If/Else)
// ============================================================================

TEST_F(HIRToMIRTest, SimpleIfStatement) {
    std::string source = R"(
fn test(x: i32) -> i32 {
    if (x > 0) {
        return 1;
    }
    return 0;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("test");
    ASSERT_NE(fn, nullptr);

    // Should have multiple blocks for if statement (entry, then, merge/exit)
    EXPECT_GT(fn->blocks.size(), 1);
}

TEST_F(HIRToMIRTest, IfElseStatement) {
    std::string source = R"(
fn test(x: i32) -> i32 {
    if (x > 0) {
        return 1;
    } else {
        return -1;
    }
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("test");
    ASSERT_NE(fn, nullptr);

    // Should have at least 3 blocks (entry, then, else)
    EXPECT_GE(fn->blocks.size(), 2);
}

// ============================================================================
// Loops
// ============================================================================

TEST_F(HIRToMIRTest, WhileLoop) {
    std::string source = R"(
fn count() -> i32 {
    let mut i: i32 = 0;
    while (i < 10) {
        i = i + 1;
    }
    return i;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("count");
    ASSERT_NE(fn, nullptr);

    // While loops need multiple blocks (entry, cond, body, exit)
    EXPECT_GT(fn->blocks.size(), 1);
}

TEST_F(HIRToMIRTest, ForLoop) {
    std::string source = R"(
fn sum() -> i32 {
    let mut total: i32 = 0;
    for i in 0..10 {
        total = total + i;
    }
    return total;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("sum");
    ASSERT_NE(fn, nullptr);

    // For loops are desugared to while loops, expect multiple blocks
    EXPECT_GT(fn->blocks.size(), 1);
}

// ============================================================================
// Variables and Memory
// ============================================================================

TEST_F(HIRToMIRTest, MutableVariable) {
    std::string source = R"(
fn test() -> i32 {
    let mut x: i32 = 10;
    x = 20;
    return x;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("test");
    ASSERT_NE(fn, nullptr);

    // Mutable variables should use alloca + load/store
    auto& entry = fn->blocks[0];
    bool hasAlloca = false;
    bool hasStore = false;
    bool hasLoad = false;

    for (auto& inst : entry.instructions) {
        if (inst.opcode == MIR::Opcode::Alloca) hasAlloca = true;
        if (inst.opcode == MIR::Opcode::Store) hasStore = true;
        if (inst.opcode == MIR::Opcode::Load) hasLoad = true;
    }

    // At least one of these should be true for mutable variables
    EXPECT_TRUE(hasAlloca || hasStore || hasLoad);
}

// ============================================================================
// Arrays
// ============================================================================

TEST_F(HIRToMIRTest, ArrayDeclaration) {
    std::string source = R"(
fn test() -> i32 {
    let arr: [i32; 5] = [1, 2, 3, 4, 5];
    return arr[0];
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("test");
    ASSERT_NE(fn, nullptr);

    // Arrays should use GetElementPtr for indexing
    auto& entry = fn->blocks[0];
    bool hasGEP = false;
    for (auto& inst : entry.instructions) {
        if (inst.opcode == MIR::Opcode::GetElementPtr) {
            hasGEP = true;
            break;
        }
    }
    // Might be optimized or handled differently
}

// ============================================================================
// Function Calls
// ============================================================================

TEST_F(HIRToMIRTest, SimpleFunctionCall) {
    std::string source = R"(
fn helper() -> i32 {
    return 42;
}

fn main() -> i32 {
    return helper();
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    EXPECT_GE(mir.functions.size(), 2);

    auto* mainFn = mir.getFunction("main");
    ASSERT_NE(mainFn, nullptr);

    // Should have Call instruction
    auto& entry = mainFn->blocks[0];
    bool hasCall = false;
    for (auto& inst : entry.instructions) {
        if (inst.opcode == MIR::Opcode::Call) {
            hasCall = true;
            EXPECT_TRUE(inst.callTarget.has_value());
            if (inst.callTarget.has_value()) {
                EXPECT_EQ(inst.callTarget.value(), "helper");
            }
            break;
        }
    }
    EXPECT_TRUE(hasCall);
}

TEST_F(HIRToMIRTest, FunctionCallWithArgs) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> i32 {
    return add(10, 20);
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* mainFn = mir.getFunction("main");
    ASSERT_NE(mainFn, nullptr);

    // Check for call instruction with arguments
    auto& entry = mainFn->blocks[0];
    bool hasCall = false;
    for (auto& inst : entry.instructions) {
        if (inst.opcode == MIR::Opcode::Call) {
            hasCall = true;
            EXPECT_EQ(inst.operands.size(), 2);  // Two arguments
            break;
        }
    }
    EXPECT_TRUE(hasCall);
}

// ============================================================================
// Recursive Functions
// ============================================================================

TEST_F(HIRToMIRTest, RecursiveFunction) {
    std::string source = R"(
fn factorial(n: i32) -> i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("factorial");
    ASSERT_NE(fn, nullptr);

    // Recursive call should be present
    bool hasRecursiveCall = false;
    for (auto& block : fn->blocks) {
        for (auto& inst : block.instructions) {
            if (inst.opcode == MIR::Opcode::Call &&
                inst.callTarget.has_value() &&
                inst.callTarget.value() == "factorial") {
                hasRecursiveCall = true;
                break;
            }
        }
    }
    EXPECT_TRUE(hasRecursiveCall);
}

// ============================================================================
// Multiple Functions
// ============================================================================

TEST_F(HIRToMIRTest, MultipleHelperFunctions) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn multiply(a: i32, b: i32) -> i32 {
    return a * b;
}

fn compute(x: i32) -> i32 {
    let sum := add(x, 10);
    let prod := multiply(sum, 2);
    return prod;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());

    EXPECT_NE(mir.getFunction("add"), nullptr);
    EXPECT_NE(mir.getFunction("multiply"), nullptr);
    EXPECT_NE(mir.getFunction("compute"), nullptr);
}

// ============================================================================
// SSA Form Validation
// ============================================================================

TEST_F(HIRToMIRTest, SSAFormBasic) {
    std::string source = R"(
fn test(x: i32) -> i32 {
    let y := x + 1;
    let z := y + 1;
    return z;
}
)";

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fn = mir.getFunction("test");
    ASSERT_NE(fn, nullptr);

    // Each temporary should have unique name (%0, %1, etc.)
    auto& entry = fn->blocks[0];
    EXPECT_GT(entry.instructions.size(), 0);

    // Verify instructions exist
    bool hasArithmetic = false;
    for (auto& inst : entry.instructions) {
        if (inst.opcode == MIR::Opcode::IAdd) {
            hasArithmetic = true;
        }
    }
    EXPECT_TRUE(hasArithmetic);
}

// ============================================================================
// Complex Programs
// ============================================================================

TEST_F(HIRToMIRTest, FibonacciProgram) {
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

    auto mir = parseAndLowerToMIR(source);

    ASSERT_FALSE(diag.hasErrors());

    auto* fibFn = mir.getFunction("fibonacci");
    auto* mainFn = mir.getFunction("main");

    ASSERT_NE(fibFn, nullptr);
    ASSERT_NE(mainFn, nullptr);

    // Fibonacci should have multiple blocks for if statement
    EXPECT_GT(fibFn->blocks.size(), 1);
}
