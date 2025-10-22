#include <gtest/gtest.h>
#include "../../include/Parser/Parser.hpp"
#include "../../include/Lexer/Lexer.hpp"
#include "../../include/HIR/Lowering.hpp"
#include "../../include/HIR/HIR.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include "../../include/Error/Error.hpp"
#include <memory>
#include <string>

// Integration tests for AST → HIR lowering pipeline

class ASTToHIRTest : public ::testing::Test {
protected:
    Type::TypeRegistry types;
    DiagnosticManager diag;

    ASTToHIRTest() : diag(false) {}

    HIR::HIRProgram parseAndLower(const std::string& source) {
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();

        Parser parser(tokens, types, diag);
        auto ast = parser.parseProgram();

        if (!ast || diag.hasErrors()) {
            return HIR::HIRProgram();
        }

        HIRLowering lowering(types);
        return lowering.lower(*ast);
    }

    bool successfullyLowered(const std::string& source) {
        auto hir = parseAndLower(source);
        return !diag.hasErrors() && hir.statements.size() > 0;
    }
};

// ============================================================================
// Variable Declaration Lowering
// ============================================================================

TEST_F(ASTToHIRTest, SimpleVariableDeclaration) {
    std::string source = R"(
fn main() -> i32 {
    let x: i32 = 42;
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 1);

    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    EXPECT_EQ(fnDecl->name, "main");
    EXPECT_EQ(fnDecl->body.size(), 2);  // VarDecl + Return
}

TEST_F(ASTToHIRTest, MutableVariableDeclaration) {
    std::string source = R"(
fn main() -> i32 {
    let mut x: i32 = 10;
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* varDecl = dynamic_cast<HIR::HIRVarDecl*>(fnDecl->body[0].get());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_TRUE(varDecl->mutable_);
}

TEST_F(ASTToHIRTest, TypeInferredVariable) {
    std::string source = R"(
fn main() -> i32 {
    let x := 42;
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* varDecl = dynamic_cast<HIR::HIRVarDecl*>(fnDecl->body[0].get());
    ASSERT_NE(varDecl, nullptr);
    EXPECT_NE(varDecl->initValue, nullptr);
}

// ============================================================================
// Function Declaration Lowering
// ============================================================================

TEST_F(ASTToHIRTest, SimpleFunctionDeclaration) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 1);

    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    EXPECT_EQ(fnDecl->name, "add");
    EXPECT_EQ(fnDecl->params.size(), 2);
    EXPECT_EQ(fnDecl->params[0].name, "a");
    EXPECT_EQ(fnDecl->params[1].name, "b");
}

TEST_F(ASTToHIRTest, VoidFunctionDeclaration) {
    std::string source = R"(
fn print_hello() -> void {
    return;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* voidType = types.getPrimitive(Type::PrimitiveKind::Void);
    EXPECT_EQ(fnDecl->returnType, voidType);
}

TEST_F(ASTToHIRTest, FunctionWithRefParam) {
    std::string source = R"(
fn read_value(x: ref i32) -> void {
    return;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    ASSERT_EQ(fnDecl->params.size(), 1);
    EXPECT_TRUE(fnDecl->params[0].isRef);
    EXPECT_FALSE(fnDecl->params[0].isMutRef);
}

TEST_F(ASTToHIRTest, FunctionWithMutRefParam) {
    std::string source = R"(
fn modify_value(x: mut ref i32) -> void {
    return;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    ASSERT_EQ(fnDecl->params.size(), 1);
    EXPECT_TRUE(fnDecl->params[0].isRef);
    EXPECT_TRUE(fnDecl->params[0].isMutRef);
}

// ============================================================================
// Control Flow Lowering
// ============================================================================

TEST_F(ASTToHIRTest, IfStatementLowering) {
    std::string source = R"(
fn main() -> i32 {
    if (true) {
        return 1;
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* ifStmt = dynamic_cast<HIR::HIRIfStmt*>(fnDecl->body[0].get());
    ASSERT_NE(ifStmt, nullptr);
    EXPECT_NE(ifStmt->condition, nullptr);
    EXPECT_GT(ifStmt->thenBody.size(), 0);
}

TEST_F(ASTToHIRTest, IfElseStatementLowering) {
    std::string source = R"(
fn main() -> i32 {
    if (true) {
        return 1;
    } else {
        return 0;
    }
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* ifStmt = dynamic_cast<HIR::HIRIfStmt*>(fnDecl->body[0].get());
    ASSERT_NE(ifStmt, nullptr);
    EXPECT_GT(ifStmt->thenBody.size(), 0);
    EXPECT_GT(ifStmt->elseBody.size(), 0);
}

TEST_F(ASTToHIRTest, WhileLoopLowering) {
    std::string source = R"(
fn main() -> i32 {
    let mut i: i32 = 0;
    while (i < 10) {
        i = i + 1;
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* whileStmt = dynamic_cast<HIR::HIRWhileStmt*>(fnDecl->body[1].get());
    ASSERT_NE(whileStmt, nullptr);
    EXPECT_NE(whileStmt->condition, nullptr);
    EXPECT_GT(whileStmt->body.size(), 0);
}

TEST_F(ASTToHIRTest, ForLoopDesugaring) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..10 {
        let x := i;
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    // For loops should be desugared into while loops with increment
    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    // Check that for loop was lowered (should be a HIRWhileStmt or HIRBlockStmt)
    EXPECT_GT(fnDecl->body.size(), 0);
}

TEST_F(ASTToHIRTest, BreakStatementLowering) {
    std::string source = R"(
fn main() -> i32 {
    while (true) {
        break;
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* whileStmt = dynamic_cast<HIR::HIRWhileStmt*>(fnDecl->body[0].get());
    ASSERT_NE(whileStmt, nullptr);

    auto* breakStmt = dynamic_cast<HIR::HIRBreakStmt*>(whileStmt->body[0].get());
    ASSERT_NE(breakStmt, nullptr);
}

TEST_F(ASTToHIRTest, ContinueStatementLowering) {
    std::string source = R"(
fn main() -> i32 {
    while (true) {
        continue;
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* whileStmt = dynamic_cast<HIR::HIRWhileStmt*>(fnDecl->body[0].get());
    ASSERT_NE(whileStmt, nullptr);

    auto* continueStmt = dynamic_cast<HIR::HIRContinueStmt*>(whileStmt->body[0].get());
    ASSERT_NE(continueStmt, nullptr);
}

// ============================================================================
// Return Statement Lowering
// ============================================================================

TEST_F(ASTToHIRTest, ReturnStatementWithValue) {
    std::string source = R"(
fn get_value() -> i32 {
    return 42;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* returnStmt = dynamic_cast<HIR::HIRReturnStmt*>(fnDecl->body[0].get());
    ASSERT_NE(returnStmt, nullptr);
    EXPECT_NE(returnStmt->value, nullptr);
}

TEST_F(ASTToHIRTest, ReturnStatementVoid) {
    std::string source = R"(
fn do_nothing() -> void {
    return;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* returnStmt = dynamic_cast<HIR::HIRReturnStmt*>(fnDecl->body[0].get());
    ASSERT_NE(returnStmt, nullptr);
    EXPECT_EQ(returnStmt->value, nullptr);
}

// ============================================================================
// Array Operations Lowering
// ============================================================================

TEST_F(ASTToHIRTest, ArrayDeclarationLowering) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1, 2, 3, 4, 5];
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* varDecl = dynamic_cast<HIR::HIRVarDecl*>(fnDecl->body[0].get());
    ASSERT_NE(varDecl, nullptr);

    auto* arrayType = dynamic_cast<const Type::ArrayType*>(varDecl->typeAnnotation);
    ASSERT_NE(arrayType, nullptr);
    EXPECT_EQ(arrayType->size, 5);
}

TEST_F(ASTToHIRTest, ArrayIndexingLowering) {
    std::string source = R"(
fn main() -> i32 {
    let arr: [i32; 5] = [1, 2, 3, 4, 5];
    let x := arr[2];
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    EXPECT_EQ(fnDecl->body.size(), 3);  // arr decl, x decl, return
}

// ============================================================================
// Struct Declaration Lowering
// ============================================================================

TEST_F(ASTToHIRTest, StructDeclarationLowering) {
    std::string source = R"(
struct Point {
    x: i32,
    y: i32
}

fn main() -> i32 {
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 2);

    auto* structDecl = dynamic_cast<HIR::HIRStructDecl*>(hir.statements[0].get());
    ASSERT_NE(structDecl, nullptr);
    EXPECT_EQ(structDecl->name.lexeme, "Point");
    EXPECT_EQ(structDecl->fields.size(), 2);
}

TEST_F(ASTToHIRTest, PublicStructDeclarationLowering) {
    std::string source = R"(
pub struct Point {
    x: i32,
    y: i32
}

fn main() -> i32 {
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* structDecl = dynamic_cast<HIR::HIRStructDecl*>(hir.statements[0].get());
    ASSERT_NE(structDecl, nullptr);
    EXPECT_TRUE(structDecl->isPublic);
}

// ============================================================================
// Complex Program Lowering
// ============================================================================

TEST_F(ASTToHIRTest, FibonacciProgram) {
    std::string source = R"(
fn fibonacci(n: i32) -> i32 {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() -> i32 {
    let result := fibonacci(10);
    return result;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 2);

    auto* fibFn = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fibFn, nullptr);
    EXPECT_EQ(fibFn->name, "fibonacci");

    auto* mainFn = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[1].get());
    ASSERT_NE(mainFn, nullptr);
    EXPECT_EQ(mainFn->name, "main");
}

TEST_F(ASTToHIRTest, BubbleSortProgram) {
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
    let mut arr: [i32; 10] = [9, 8, 7, 6, 5, 4, 3, 2, 1, 0];
    arr = bubble_sort(arr);
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 2);

    auto* sortFn = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(sortFn, nullptr);
    EXPECT_EQ(sortFn->name, "bubble_sort");
}

// ============================================================================
// Expression Lowering
// ============================================================================

TEST_F(ASTToHIRTest, BinaryExpressionLowering) {
    std::string source = R"(
fn main() -> i32 {
    let x := 10 + 20;
    let y := x * 2;
    return y;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
    EXPECT_EQ(fnDecl->body.size(), 3);  // 2 var decls + return
}

TEST_F(ASTToHIRTest, FunctionCallLowering) {
    std::string source = R"(
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> i32 {
    let result := add(10, 20);
    return result;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 2);
}

// ============================================================================
// Nested Structures
// ============================================================================

TEST_F(ASTToHIRTest, NestedIfStatements) {
    std::string source = R"(
fn main() -> i32 {
    if (true) {
        if (false) {
            return 1;
        } else {
            return 2;
        }
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);

    auto* outerIf = dynamic_cast<HIR::HIRIfStmt*>(fnDecl->body[0].get());
    ASSERT_NE(outerIf, nullptr);
    EXPECT_GT(outerIf->thenBody.size(), 0);
}

TEST_F(ASTToHIRTest, NestedLoops) {
    std::string source = R"(
fn main() -> i32 {
    for i in 0..10 {
        for j in 0..10 {
            let x := i + j;
        }
    }
    return 0;
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    auto* fnDecl = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    ASSERT_NE(fnDecl, nullptr);
}

// ============================================================================
// Multiple Functions
// ============================================================================

TEST_F(ASTToHIRTest, MultipleFunctions) {
    std::string source = R"(
fn helper1() -> i32 {
    return 1;
}

fn helper2() -> i32 {
    return 2;
}

fn main() -> i32 {
    return helper1() + helper2();
}
)";

    auto hir = parseAndLower(source);

    ASSERT_FALSE(diag.hasErrors());
    ASSERT_EQ(hir.statements.size(), 3);

    auto* fn1 = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[0].get());
    auto* fn2 = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[1].get());
    auto* mainFn = dynamic_cast<HIR::HIRFnDecl*>(hir.statements[2].get());

    ASSERT_NE(fn1, nullptr);
    ASSERT_NE(fn2, nullptr);
    ASSERT_NE(mainFn, nullptr);

    EXPECT_EQ(fn1->name, "helper1");
    EXPECT_EQ(fn2->name, "helper2");
    EXPECT_EQ(mainFn->name, "main");
}
