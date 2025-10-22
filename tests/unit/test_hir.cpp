#include <gtest/gtest.h>
#include "../../include/HIR/HIR.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include "../../include/Lexer/Token.hpp"
#include <memory>

using namespace HIR;
using namespace Type;

// ============================================================================
// HIR Node Creation Tests
// ============================================================================

class HIRTest : public ::testing::Test {
protected:
    TypeRegistry types;

    HIRTest() {}

    Token makeToken(const std::string& lexeme, TokenType tt = TokenType::Identifier) {
        return Token(tt, 1, 1, lexeme);
    }
};

// ============================================================================
// Variable Declaration Tests
// ============================================================================

TEST_F(HIRTest, VarDeclImmutable) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto varDecl = std::make_unique<HIRVarDecl>(
        makeToken("x"),
        i32Type,
        nullptr,  // No initializer
        false,    // Immutable
        1, 1
    );

    EXPECT_EQ(varDecl->name.lexeme, "x");
    EXPECT_EQ(varDecl->typeAnnotation, i32Type);
    EXPECT_FALSE(varDecl->mutable_);
    EXPECT_EQ(varDecl->initValue, nullptr);
}

TEST_F(HIRTest, VarDeclMutable) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto varDecl = std::make_unique<HIRVarDecl>(
        makeToken("x"),
        i32Type,
        nullptr,
        true,  // Mutable
        1, 1
    );

    EXPECT_TRUE(varDecl->mutable_);
}

TEST_F(HIRTest, VarDeclWithInitializer) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto literal = std::make_unique<Literal>(makeToken("42", TokenType::Integer));

    auto varDecl = std::make_unique<HIRVarDecl>(
        makeToken("x"),
        i32Type,
        std::move(literal),
        false,
        1, 1
    );

    EXPECT_NE(varDecl->initValue, nullptr);
}

// ============================================================================
// Function Declaration Tests
// ============================================================================

TEST_F(HIRTest, FnDeclNoParams) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    std::vector<HIR::Param> params;
    std::vector<std::unique_ptr<HIRStmt>> body;

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "main",
        std::move(params),
        i32Type,
        std::move(body),
        false,  // Not extern
        false,  // Not public
        1, 1
    );

    EXPECT_EQ(fnDecl->name, "main");
    EXPECT_EQ(fnDecl->returnType, i32Type);
    EXPECT_EQ(fnDecl->params.size(), 0);
    EXPECT_FALSE(fnDecl->isExtern);
    EXPECT_FALSE(fnDecl->isPublic);
}

TEST_F(HIRTest, FnDeclWithParams) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* i64Type = types.getPrimitive(PrimitiveKind::I64);

    std::vector<HIR::Param> params;
    params.emplace_back("a", i32Type, false, false);
    params.emplace_back("b", i64Type, false, false);

    std::vector<std::unique_ptr<HIRStmt>> body;

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "add",
        std::move(params),
        i64Type,
        std::move(body),
        false, false, 1, 1
    );

    EXPECT_EQ(fnDecl->name, "add");
    EXPECT_EQ(fnDecl->params.size(), 2);
    EXPECT_EQ(fnDecl->params[0].name, "a");
    EXPECT_EQ(fnDecl->params[0].type, i32Type);
    EXPECT_EQ(fnDecl->params[1].name, "b");
    EXPECT_EQ(fnDecl->params[1].type, i64Type);
}

TEST_F(HIRTest, FnDeclWithRefParam) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    std::vector<HIR::Param> params;
    params.emplace_back("x", i32Type, true, false);  // ref (immutable reference)

    std::vector<std::unique_ptr<HIRStmt>> body;

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "read",
        std::move(params),
        types.getPrimitive(PrimitiveKind::Void),
        std::move(body),
        false, false, 1, 1
    );

    EXPECT_TRUE(fnDecl->params[0].isRef);
    EXPECT_FALSE(fnDecl->params[0].isMutRef);
}

TEST_F(HIRTest, FnDeclWithMutRefParam) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    std::vector<HIR::Param> params;
    params.emplace_back("x", i32Type, true, true);  // mut ref

    std::vector<std::unique_ptr<HIRStmt>> body;

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "modify",
        std::move(params),
        types.getPrimitive(PrimitiveKind::Void),
        std::move(body),
        false, false, 1, 1
    );

    EXPECT_TRUE(fnDecl->params[0].isRef);
    EXPECT_TRUE(fnDecl->params[0].isMutRef);
}

TEST_F(HIRTest, FnDeclExtern) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    std::vector<HIR::Param> params;
    std::vector<std::unique_ptr<HIRStmt>> body;

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "printf",
        std::move(params),
        i32Type,
        std::move(body),
        true,   // Extern
        false,
        1, 1
    );

    EXPECT_TRUE(fnDecl->isExtern);
}

TEST_F(HIRTest, FnDeclPublic) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    std::vector<HIR::Param> params;
    std::vector<std::unique_ptr<HIRStmt>> body;

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "api_function",
        std::move(params),
        i32Type,
        std::move(body),
        false,
        true,   // Public
        1, 1
    );

    EXPECT_TRUE(fnDecl->isPublic);
}

// ============================================================================
// Control Flow Statement Tests
// ============================================================================

TEST_F(HIRTest, ReturnStmtWithValue) {
    auto literal = std::make_unique<Literal>(makeToken("42", TokenType::Integer));
    auto returnStmt = std::make_unique<HIRReturnStmt>(std::move(literal), 1, 1);

    EXPECT_NE(returnStmt->value, nullptr);
}

TEST_F(HIRTest, ReturnStmtVoid) {
    auto returnStmt = std::make_unique<HIRReturnStmt>(nullptr, 1, 1);

    EXPECT_EQ(returnStmt->value, nullptr);
}

TEST_F(HIRTest, IfStmtWithoutElse) {
    auto condition = std::make_unique<Literal>(makeToken("true", TokenType::True_));
    std::vector<std::unique_ptr<HIRStmt>> thenBody;

    auto ifStmt = std::make_unique<HIRIfStmt>(
        std::move(condition),
        std::move(thenBody),
        std::vector<std::unique_ptr<HIRStmt>>(),  // Empty else
        1, 1
    );

    EXPECT_NE(ifStmt->condition, nullptr);
    EXPECT_EQ(ifStmt->elseBody.size(), 0);
}

TEST_F(HIRTest, IfStmtWithElse) {
    auto condition = std::make_unique<Literal>(makeToken("true", TokenType::True_));
    std::vector<std::unique_ptr<HIRStmt>> thenBody;
    std::vector<std::unique_ptr<HIRStmt>> elseBody;

    // Add a statement to else body
    elseBody.push_back(std::make_unique<HIRReturnStmt>(nullptr, 1, 1));

    auto ifStmt = std::make_unique<HIRIfStmt>(
        std::move(condition),
        std::move(thenBody),
        std::move(elseBody),
        1, 1
    );

    EXPECT_EQ(ifStmt->elseBody.size(), 1);
}

TEST_F(HIRTest, WhileStmt) {
    auto condition = std::make_unique<Literal>(makeToken("true", TokenType::True_));
    std::vector<std::unique_ptr<HIRStmt>> body;

    auto whileStmt = std::make_unique<HIRWhileStmt>(
        std::move(condition),
        std::move(body),
        nullptr,  // No increment (not a desugared for loop)
        1, 1
    );

    EXPECT_NE(whileStmt->condition, nullptr);
    EXPECT_EQ(whileStmt->increment, nullptr);
}

TEST_F(HIRTest, WhileStmtWithIncrement) {
    auto condition = std::make_unique<Literal>(makeToken("true", TokenType::True_));
    auto increment = std::make_unique<Literal>(makeToken("1", TokenType::Integer));
    std::vector<std::unique_ptr<HIRStmt>> body;

    auto whileStmt = std::make_unique<HIRWhileStmt>(
        std::move(condition),
        std::move(body),
        std::move(increment),  // Desugared for loop
        1, 1
    );

    EXPECT_NE(whileStmt->increment, nullptr);
}

TEST_F(HIRTest, BreakStmt) {
    auto breakStmt = std::make_unique<HIRBreakStmt>(1, 1);
    EXPECT_EQ(breakStmt->line, 1);
    EXPECT_EQ(breakStmt->column, 1);
}

TEST_F(HIRTest, ContinueStmt) {
    auto continueStmt = std::make_unique<HIRContinueStmt>(1, 1);
    EXPECT_EQ(continueStmt->line, 1);
    EXPECT_EQ(continueStmt->column, 1);
}

TEST_F(HIRTest, BlockStmt) {
    std::vector<std::unique_ptr<HIRStmt>> statements;
    statements.push_back(std::make_unique<HIRBreakStmt>(1, 1));
    statements.push_back(std::make_unique<HIRContinueStmt>(2, 1));

    auto blockStmt = std::make_unique<HIRBlockStmt>(std::move(statements), 1, 1);

    EXPECT_EQ(blockStmt->statements.size(), 2);
}

// ============================================================================
// Struct Declaration Tests
// ============================================================================

TEST_F(HIRTest, StructDeclNoMethods) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    auto* i64Type = types.getPrimitive(PrimitiveKind::I64);

    std::vector<StructField> fields;
    fields.push_back(StructField(false, makeToken("x"), i32Type));
    fields.push_back(StructField(false, makeToken("y"), i64Type));

    auto structDecl = std::make_unique<HIRStructDecl>(
        false,  // Not public
        makeToken("Point"),
        std::move(fields),
        1, 1
    );

    EXPECT_EQ(structDecl->name.lexeme, "Point");
    EXPECT_EQ(structDecl->fields.size(), 2);
    EXPECT_FALSE(structDecl->isPublic);
}

TEST_F(HIRTest, StructDeclWithMethods) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    std::vector<StructField> fields;
    fields.push_back(StructField(false, makeToken("x"), i32Type));

    std::vector<std::unique_ptr<HIRFnDecl>> methods;
    methods.push_back(std::make_unique<HIRFnDecl>(
        "get_x",
        std::vector<HIR::Param>(),
        i32Type,
        std::vector<std::unique_ptr<HIRStmt>>(),
        false, false, 1, 1
    ));

    auto structDecl = std::make_unique<HIRStructDecl>(
        false,
        makeToken("Point"),
        std::move(fields),
        std::move(methods),
        1, 1
    );

    EXPECT_EQ(structDecl->methods.size(), 1);
    EXPECT_EQ(structDecl->methods[0]->name, "get_x");
}

TEST_F(HIRTest, StructDeclPublic) {
    std::vector<StructField> fields;

    auto structDecl = std::make_unique<HIRStructDecl>(
        true,  // Public
        makeToken("PublicStruct"),
        std::move(fields),
        1, 1
    );

    EXPECT_TRUE(structDecl->isPublic);
}

// ============================================================================
// Extern Block Tests
// ============================================================================

TEST_F(HIRTest, ExternBlockWithFunctions) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    std::vector<std::unique_ptr<HIRFnDecl>> decls;
    decls.push_back(std::make_unique<HIRFnDecl>(
        "printf",
        std::vector<HIR::Param>(),
        i32Type,
        std::vector<std::unique_ptr<HIRStmt>>(),
        true,  // Extern
        false,
        1, 1
    ));

    auto externBlock = std::make_unique<HIRExternBlock>(std::move(decls), 1, 1);

    EXPECT_EQ(externBlock->declarations.size(), 1);
    EXPECT_EQ(externBlock->declarations[0]->name, "printf");
    EXPECT_TRUE(externBlock->declarations[0]->isExtern);
}

// ============================================================================
// Import Statement Tests
// ============================================================================

TEST_F(HIRTest, ImportStmtAll) {
    auto importStmt = std::make_unique<HIRImportStmt>(
        "std::math",
        std::vector<std::string>(),  // Empty = import all
        1, 1
    );

    EXPECT_EQ(importStmt->modulePath, "std::math");
    EXPECT_EQ(importStmt->symbols.size(), 0);
}

TEST_F(HIRTest, ImportStmtSpecific) {
    std::vector<std::string> symbols = {"sin", "cos", "tan"};

    auto importStmt = std::make_unique<HIRImportStmt>(
        "std::math",
        symbols,
        1, 1
    );

    EXPECT_EQ(importStmt->symbols.size(), 3);
    EXPECT_EQ(importStmt->symbols[0], "sin");
    EXPECT_EQ(importStmt->symbols[1], "cos");
    EXPECT_EQ(importStmt->symbols[2], "tan");
}

// ============================================================================
// HIR Program Tests
// ============================================================================

TEST_F(HIRTest, HIRProgramEmpty) {
    HIRProgram program;

    EXPECT_EQ(program.statements.size(), 0);
}

TEST_F(HIRTest, HIRProgramWithStatements) {
    std::vector<std::unique_ptr<HIRStmt>> stmts;

    stmts.push_back(std::make_unique<HIRBreakStmt>(1, 1));
    stmts.push_back(std::make_unique<HIRContinueStmt>(2, 1));

    HIRProgram program(std::move(stmts));

    EXPECT_EQ(program.statements.size(), 2);
}

// ============================================================================
// Expression Statement Tests
// ============================================================================

TEST_F(HIRTest, ExprStmt) {
    auto literal = std::make_unique<Literal>(makeToken("42", TokenType::Integer));
    auto exprStmt = std::make_unique<HIRExprStmt>(std::move(literal), 1, 1);

    EXPECT_NE(exprStmt->expr, nullptr);
}

// ============================================================================
// Param Tests
// ============================================================================

TEST_F(HIRTest, ParamByValue) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    HIR::Param param("x", i32Type, false, false);

    EXPECT_EQ(param.name, "x");
    EXPECT_EQ(param.type, i32Type);
    EXPECT_FALSE(param.isRef);
    EXPECT_FALSE(param.isMutRef);
}

TEST_F(HIRTest, ParamByRef) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    HIR::Param param("x", i32Type, true, false);

    EXPECT_TRUE(param.isRef);
    EXPECT_FALSE(param.isMutRef);
}

TEST_F(HIRTest, ParamByMutRef) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);
    HIR::Param param("x", i32Type, true, true);

    EXPECT_TRUE(param.isRef);
    EXPECT_TRUE(param.isMutRef);
}

// ============================================================================
// Complex HIR Structure Tests
// ============================================================================

TEST_F(HIRTest, CompleteFunction) {
    auto* i32Type = types.getPrimitive(PrimitiveKind::I32);

    // Build function: fn add(a: i32, b: i32) -> i32 { return a + b; }
    std::vector<HIR::Param> params;
    params.emplace_back("a", i32Type, false, false);
    params.emplace_back("b", i32Type, false, false);

    std::vector<std::unique_ptr<HIRStmt>> body;

    // Create return statement with binary expression
    auto varA = std::make_unique<Variable>(makeToken("a"));
    auto varB = std::make_unique<Variable>(makeToken("b"));
    auto addExpr = std::make_unique<BinaryExpr>(
        std::move(varA),
        std::move(varB),
        TokenType::Plus,
        2, 7
    );

    body.push_back(std::make_unique<HIRReturnStmt>(std::move(addExpr), 2, 1));

    auto fnDecl = std::make_unique<HIRFnDecl>(
        "add",
        std::move(params),
        i32Type,
        std::move(body),
        false, false, 1, 1
    );

    EXPECT_EQ(fnDecl->name, "add");
    EXPECT_EQ(fnDecl->params.size(), 2);
    EXPECT_EQ(fnDecl->body.size(), 1);

    // Verify the return statement exists
    auto* returnStmt = dynamic_cast<HIRReturnStmt*>(fnDecl->body[0].get());
    ASSERT_NE(returnStmt, nullptr);
    EXPECT_NE(returnStmt->value, nullptr);
}

TEST_F(HIRTest, NestedControlFlow) {
    auto* boolType = types.getPrimitive(PrimitiveKind::Bool);

    // Build: if (condition) { while (true) { break; } }
    std::vector<std::unique_ptr<HIRStmt>> whileBody;
    whileBody.push_back(std::make_unique<HIRBreakStmt>(3, 5));

    auto whileLoop = std::make_unique<HIRWhileStmt>(
        std::make_unique<Literal>(makeToken("true", TokenType::True_)),
        std::move(whileBody),
        nullptr,
        2, 5
    );

    std::vector<std::unique_ptr<HIRStmt>> ifBody;
    ifBody.push_back(std::move(whileLoop));

    auto ifStmt = std::make_unique<HIRIfStmt>(
        std::make_unique<Literal>(makeToken("true", TokenType::True_)),
        std::move(ifBody),
        std::vector<std::unique_ptr<HIRStmt>>(),
        1, 1
    );

    EXPECT_EQ(ifStmt->thenBody.size(), 1);

    auto* nestedWhile = dynamic_cast<HIRWhileStmt*>(ifStmt->thenBody[0].get());
    ASSERT_NE(nestedWhile, nullptr);
    EXPECT_EQ(nestedWhile->body.size(), 1);
}