#include <gtest/gtest.h>
#include "vm/BytecodeCompiler.hpp"
#include "IR/Module.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"

using namespace volta;
using namespace volta::vm;
using namespace volta::ir;

// ============================================================================
// Test Fixture
// ============================================================================

class BytecodeCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("test_module");
        builder = std::make_unique<IRBuilder>(*module);
    }

    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder> builder;
};

// ============================================================================
// Basic Compilation Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileEmptyFunction) {
    // Create function: fn test() -> void { ret void }
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    builder->createRet(nullptr);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    // Verify
    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());

    // Should have 4 native functions (print_i64, print_f64, print_bool, print_str) + 1 user function
    EXPECT_EQ(bytecode->getFunctionCount(), 5);
}

TEST_F(BytecodeCompilerTest, CompileSimpleReturn) {
    // Create function: fn test() -> int { ret 42 }
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* const42 = builder->getInt(42);
    builder->createRet(const42);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    // Verify
    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());

    // Check constant pool
    EXPECT_EQ(bytecode->getIntConstant(0), 42);
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileAddition) {
    // Create function: fn add(a: int, b: int) -> int { ret a + b }
    auto* func = builder->createFunction("add", builder->getIntType(),
                                         {builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* a = func->getParam(0);
    auto* b = func->getParam(1);
    auto* sum = builder->createAdd(a, b, "sum");
    builder->createRet(sum);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    // Verify
    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileAllArithmeticOps) {
    // Test Add, Sub, Mul, Div, Rem
    auto* func = builder->createFunction("arithmetic", builder->getIntType(),
                                         {builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* a = func->getParam(0);
    auto* b = func->getParam(1);

    auto* add = builder->createAdd(a, b);
    auto* sub = builder->createSub(add, b);
    auto* mul = builder->createMul(sub, b);
    auto* div = builder->createDiv(mul, b);
    auto* rem = builder->createRem(div, b);
    builder->createRet(rem);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

// ============================================================================
// Comparison Operations Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileComparisons) {
    auto* func = builder->createFunction("compare", builder->getBoolType(),
                                         {builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* a = func->getParam(0);
    auto* b = func->getParam(1);

    auto* eq = builder->createEq(a, b);
    auto* ne = builder->createNe(a, b);
    auto* lt = builder->createLt(a, b);
    auto* result = builder->createAnd(eq, ne);
    builder->createRet(result);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

// ============================================================================
// Control Flow Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileUnconditionalBranch) {
    // fn test() -> int {
    //   br block2
    // block2:
    //   ret 42
    // }
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* block1 = func->getEntryBlock();
    auto* block2 = builder->createBasicBlock("block2", func);

    builder->setInsertionPoint(block1);
    builder->createBr(block2);

    builder->setInsertionPoint(block2);
    auto* const42 = builder->getInt(42);
    builder->createRet(const42);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileConditionalBranch) {
    // fn test(cond: bool) -> int {
    //   br cond, trueBlock, falseBlock
    // trueBlock:
    //   ret 1
    // falseBlock:
    //   ret 0
    // }
    auto* func = builder->createFunction("test", builder->getIntType(), {builder->getBoolType()});
    auto* entry = func->getEntryBlock();
    auto* trueBlock = builder->createBasicBlock("trueBlock", func);
    auto* falseBlock = builder->createBasicBlock("falseBlock", func);

    builder->setInsertionPoint(entry);
    auto* cond = func->getParam(0);
    builder->createCondBr(cond, trueBlock, falseBlock);

    builder->setInsertionPoint(trueBlock);
    auto* const1 = builder->getInt(1);
    builder->createRet(const1);

    builder->setInsertionPoint(falseBlock);
    auto* const0 = builder->getInt(0);
    builder->createRet(const0);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

// ============================================================================
// Register Allocation Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, RegisterAllocationWithConstants) {
    // fn test() -> int {
    //   let a = 10
    //   let b = 20
    //   ret a + b
    // }
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* const10 = builder->getInt(10);
    auto* const20 = builder->getInt(20);
    auto* sum = builder->createAdd(const10, const20);
    builder->createRet(sum);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());

    // Check constant pool has both values
    EXPECT_EQ(bytecode->getIntConstant(0), 10);
    EXPECT_EQ(bytecode->getIntConstant(1), 20);
}

TEST_F(BytecodeCompilerTest, RegisterAllocationWithParameters) {
    // fn test(a: int, b: int, c: int) -> int {
    //   ret a + b + c
    // }
    auto* func = builder->createFunction("test", builder->getIntType(),
                                         {builder->getIntType(), builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* a = func->getParam(0);
    auto* b = func->getParam(1);
    auto* c = func->getParam(2);

    auto* sum1 = builder->createAdd(a, b);
    auto* sum2 = builder->createAdd(sum1, c);
    builder->createRet(sum2);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());

    // Check function metadata
    auto funcInfo = bytecode->getFunction(4);  // Index 0-3 are print functions
    EXPECT_EQ(funcInfo.getParamCount(), 3);
    EXPECT_GE(funcInfo.getRegisterCount(), 3);  // At least 3 for params
}

// ============================================================================
// Function Call Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileFunctionCall) {
    // fn callee(x: int) -> int { ret x }
    // fn caller() -> int { ret callee(42) }

    // Create callee
    auto* callee = builder->createFunction("callee", builder->getIntType(), {builder->getIntType()});
    auto* calleeEntry = callee->getEntryBlock();
    builder->setInsertionPoint(calleeEntry);
    auto* param = callee->getParam(0);
    builder->createRet(param);

    // Create caller
    auto* caller = builder->createFunction("caller", builder->getIntType(), {});
    auto* callerEntry = caller->getEntryBlock();
    builder->setInsertionPoint(callerEntry);
    auto* const42 = builder->getInt(42);
    auto* result = builder->createCall(callee, {const42});
    builder->createRet(result);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
    EXPECT_EQ(bytecode->getFunctionCount(), 6);  // 4 print functions + callee + caller
}

// ============================================================================
// Constant Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileBooleanConstants) {
    auto* func = builder->createFunction("test", builder->getBoolType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* constTrue = builder->getBool(true);
    auto* constFalse = builder->getBool(false);
    auto* result = builder->createAnd(constTrue, constFalse);
    builder->createRet(result);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(BytecodeCompilerTest, TooManyParameters) {
    // Create function with 256 parameters (should fail)
    std::vector<std::shared_ptr<IRType>> paramTypes;
    for (int i = 0; i < 256; i++) {
        paramTypes.push_back(builder->getIntType());
    }

    auto* func = builder->createFunction("test", builder->getIntType(), paramTypes);
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    auto* const0 = builder->getInt(0);
    builder->createRet(const0);

    BytecodeCompiler compiler;

    // Should throw exception
    EXPECT_THROW(compiler.compile(std::move(module)), std::runtime_error);
}

TEST_F(BytecodeCompilerTest, ComplexControlFlow) {
    // fn test(x: int) -> int {
    //   if (x > 0) {
    //     if (x > 10) { ret 2 }
    //     else { ret 1 }
    //   } else {
    //     ret 0
    //   }
    // }
    auto* func = builder->createFunction("test", builder->getIntType(), {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    auto* thenBlock = builder->createBasicBlock("then", func);
    auto* elseBlock = builder->createBasicBlock("else", func);
    auto* innerThen = builder->createBasicBlock("innerThen", func);
    auto* innerElse = builder->createBasicBlock("innerElse", func);

    builder->setInsertionPoint(entry);
    auto* x = func->getParam(0);
    auto* const0 = builder->getInt(0);
    auto* cond1 = builder->createGt(x, const0);
    builder->createCondBr(cond1, thenBlock, elseBlock);

    builder->setInsertionPoint(thenBlock);
    auto* const10 = builder->getInt(10);
    auto* cond2 = builder->createGt(x, const10);
    builder->createCondBr(cond2, innerThen, innerElse);

    builder->setInsertionPoint(innerThen);
    auto* const2 = builder->getInt(2);
    builder->createRet(const2);

    builder->setInsertionPoint(innerElse);
    auto* const1 = builder->getInt(1);
    builder->createRet(const1);

    builder->setInsertionPoint(elseBlock);
    builder->createRet(const0);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

// ============================================================================
// Runtime Functions Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, PrintFunctionRegistered) {
    auto* func = builder->createFunction("test", builder->getVoidType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    builder->createRet(nullptr);

    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);

    // Check print functions are registered at indices 0-3
    auto printFunc0 = bytecode->getFunction(0);
    EXPECT_EQ(printFunc0.getName(), "print_i64");
    EXPECT_EQ(printFunc0.getParamCount(), 1);
    EXPECT_TRUE(printFunc0.isNative());

    auto printFunc3 = bytecode->getFunction(3);
    EXPECT_EQ(printFunc3.getName(), "print_str");
    EXPECT_EQ(printFunc3.getParamCount(), 1);
    EXPECT_TRUE(printFunc3.isNative());
}

// ============================================================================
// If-Else-If Chain Integration Tests (from full parsing pipeline)
// ============================================================================

#include "lexer/lexer.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "IR/IRGenerator.hpp"
#include "error/ErrorReporter.hpp"
#include "IR/IRPrinter.hpp"
#include <sstream>
#include <iomanip>

class BytecodeIfElseChainTest : public ::testing::Test {
protected:
    std::unique_ptr<BytecodeModule> compileSource(const std::string& source) {
        using namespace volta::lexer;
        using namespace volta::parser;
        using namespace volta::semantic;
        using namespace volta::errors;

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

        // Print IR for debugging
        ir::IRPrinter printer;
        std::cout << "\n=== IR ===\n";
        std::cout << printer.printModule(*module) << std::endl;

        // 5. Bytecode Compilation
        BytecodeCompiler compiler;
        auto bytecode = compiler.compile(std::move(module));

        return bytecode;
    }
};

TEST_F(BytecodeIfElseChainTest, SimpleIfElse) {
    std::string source = R"(
fn test(x: int) -> int {
    if x > 0 {
        return 1
    } else {
        return 0
    }
}
    )";

    auto bytecode = compileSource(source);
    ASSERT_NE(bytecode, nullptr);

    std::cout << "\n=== SimpleIfElse Bytecode ===\n";
    std::cout << "Function count: " << bytecode->getFunctionCount() << std::endl;

    EXPECT_TRUE(bytecode->verify());

    // Get the test function
    vm::index funcIdx = bytecode->getFunctionIndex("test");
    auto funcInfo = bytecode->getFunction(funcIdx);

    std::cout << "Function: " << funcInfo.getName() << std::endl;
    std::cout << "  Params: " << (int)funcInfo.getParamCount() << std::endl;
    std::cout << "  Registers: " << (int)funcInfo.getRegisterCount() << std::endl;
    std::cout << "  Code size: " << funcInfo.getCodeLength() << " bytes" << std::endl;

    EXPECT_EQ(funcInfo.getParamCount(), 1);
    EXPECT_GE(funcInfo.getRegisterCount(), 1);
}

TEST_F(BytecodeIfElseChainTest, SingleElseIf) {
    std::string source = R"(
fn test(x: int) -> int {
    if x < 0 {
        return -1
    } else if x == 0 {
        return 0
    } else {
        return 1
    }
}
    )";

    auto bytecode = compileSource(source);
    ASSERT_NE(bytecode, nullptr);

    std::cout << "\n=== SingleElseIf Bytecode ===\n";
    std::cout << "Function count: " << bytecode->getFunctionCount() << std::endl;

    EXPECT_TRUE(bytecode->verify());

    vm::index funcIdx = bytecode->getFunctionIndex("test");
    auto funcInfo = bytecode->getFunction(funcIdx);

    std::cout << "Function: " << funcInfo.getName() << std::endl;
    std::cout << "  Params: " << (int)funcInfo.getParamCount() << std::endl;
    std::cout << "  Registers: " << (int)funcInfo.getRegisterCount() << std::endl;
    std::cout << "  Code size: " << funcInfo.getCodeLength() << " bytes" << std::endl;

    EXPECT_EQ(funcInfo.getParamCount(), 1);
    EXPECT_GE(funcInfo.getRegisterCount(), 1);
}

TEST_F(BytecodeIfElseChainTest, MultipleElseIfs) {
    std::string source = R"(
fn test(x: int) -> int {
    if x < 0 {
        return -1
    } else if x == 0 {
        return 0
    } else if x == 1 {
        return 1
    } else if x == 2 {
        return 2
    } else {
        return 999
    }
}
    )";

    auto bytecode = compileSource(source);
    ASSERT_NE(bytecode, nullptr);

    std::cout << "\n=== MultipleElseIfs Bytecode ===\n";
    EXPECT_TRUE(bytecode->verify());

    vm::index funcIdx = bytecode->getFunctionIndex("test");
    auto funcInfo = bytecode->getFunction(funcIdx);

    std::cout << "Function: " << funcInfo.getName() << std::endl;
    std::cout << "  Code size: " << funcInfo.getCodeLength() << " bytes" << std::endl;
}

TEST_F(BytecodeIfElseChainTest, ElseIfWithAssignments) {
    std::string source = R"(
fn test(x: int) -> int {
    result: mut int = 0
    if x < 0 {
        result = -1
    } else if x == 0 {
        result = 0
    } else {
        result = 1
    }
    return result
}
    )";

    auto bytecode = compileSource(source);
    ASSERT_NE(bytecode, nullptr);

    std::cout << "\n=== ElseIfWithAssignments Bytecode ===\n";
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeIfElseChainTest, ClassifyFunction) {
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
    )";

    auto bytecode = compileSource(source);
    ASSERT_NE(bytecode, nullptr);

    std::cout << "\n=== ClassifyFunction Bytecode ===\n";
    EXPECT_TRUE(bytecode->verify());

    // Detailed bytecode inspection
    vm::index funcIdx = bytecode->getFunctionIndex("classify");
    auto funcInfo = bytecode->getFunction(funcIdx);

    std::cout << "\n=== Bytecode Details ===\n";
    std::cout << "Function: " << funcInfo.getName() << std::endl;
    std::cout << "  Offset: " << funcInfo.getCodeOffset() << std::endl;
    std::cout << "  Size: " << funcInfo.getCodeLength() << " bytes" << std::endl;
    std::cout << "  Params: " << (int)funcInfo.getParamCount() << std::endl;
    std::cout << "  Registers: " << (int)funcInfo.getRegisterCount() << std::endl;

    // Print bytecode hex dump
    std::cout << "\n  Bytecode (hex):" << std::endl;
    uint32_t offset = funcInfo.getCodeOffset();
    for (uint32_t i = 0; i < funcInfo.getCodeLength(); i++) {
        if (i % 16 == 0) std::cout << "    " << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)bytecode->readByte(offset + i) << " ";
        if ((i + 1) % 16 == 0 || i == funcInfo.getCodeLength() - 1) std::cout << std::endl;
    }
    std::cout << std::dec;
}

// ============================================================================
// Struct Operations Tests
// ============================================================================

TEST_F(BytecodeCompilerTest, CompileSimpleStructCreation) {
    // Test: Point { x: 3.0, y: 4.0 }
    // Creates struct type, allocates, and inserts fields

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getFloatType(),
        builder->getFloatType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("test", structType, {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    // Allocate struct
    auto* structAlloc = builder->createGCAlloc(structType, "point");

    // Insert field 0 (x = 3.0)
    auto* f3 = builder->getFloat(3.0);
    auto* struct1 = builder->createInsertValue(structAlloc, f3, 0, "");

    // Insert field 1 (y = 4.0)
    auto* f4 = builder->getFloat(4.0);
    auto* struct2 = builder->createInsertValue(struct1, f4, 1, "");

    builder->createRet(struct2);

    // Should throw because GCAlloc not yet implemented
    BytecodeCompiler compiler;
    EXPECT_THROW({
        auto bytecode = compiler.compile(std::move(module));
    }, std::runtime_error);
}

TEST_F(BytecodeCompilerTest, CompileStructFieldAccess) {
    // Test: point.x (extract field 0)

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getFloatType(),
        builder->getFloatType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("getX", builder->getFloatType(), {structType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* param = func->getParam(0);

    // Extract field 0
    auto* x = builder->createExtractValue(param, 0, "x");
    builder->createRet(x);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());

    // Verify STRUCT_GET instruction was emitted
    // (We can't execute yet, but bytecode generation should work)
}

TEST_F(BytecodeCompilerTest, CompileStructFieldUpdate) {
    // Test: point with x = newValue (insert field 0)

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getFloatType(),
        builder->getFloatType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("setX", structType,
                                         {structType, builder->getFloatType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* param = func->getParam(0);
    auto* newX = func->getParam(1);

    // Insert new value at field 0
    auto* updated = builder->createInsertValue(param, newX, 0, "");
    builder->createRet(updated);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileNestedStructAccess) {
    // Test: line.start.x (nested field access)
    // Line { start: Point, end: Point }
    // Point { x: float, y: float }

    std::vector<std::shared_ptr<IRType>> pointFields = {
        builder->getFloatType(),
        builder->getFloatType()
    };
    auto pointType = std::make_shared<IRStructType>(pointFields);

    std::vector<std::shared_ptr<IRType>> lineFields = {
        pointType,
        pointType
    };
    auto lineType = std::make_shared<IRStructType>(lineFields);

    auto* func = builder->createFunction("getStartX", builder->getFloatType(), {lineType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* line = func->getParam(0);

    // Extract start (field 0) - returns Point
    auto* start = builder->createExtractValue(line, 0, "start");

    // Extract x (field 0 of Point) - returns float
    auto* x = builder->createExtractValue(start, 0, "x");

    builder->createRet(x);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileStructWithMixedTypes) {
    // Test: Person { age: int, height: float, name: str }
    // Mix of different types

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getIntType(),
        builder->getFloatType(),
        builder->getStringType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("getAge", builder->getIntType(), {structType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* person = func->getParam(0);
    auto* age = builder->createExtractValue(person, 0, "age");

    builder->createRet(age);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileStructMultipleUpdates) {
    // Test: Update multiple fields in sequence
    // point.x = 1.0, point.y = 2.0

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getFloatType(),
        builder->getFloatType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("updateBoth", structType, {structType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* point = func->getParam(0);

    // Update x to 1.0
    auto* f1 = builder->getFloat(1.0);
    auto* point1 = builder->createInsertValue(point, f1, 0, "");

    // Update y to 2.0
    auto* f2 = builder->getFloat(2.0);
    auto* point2 = builder->createInsertValue(point1, f2, 1, "");

    builder->createRet(point2);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileEmptyStruct) {
    // Edge case: Struct with no fields (Unit type)

    std::vector<std::shared_ptr<IRType>> fieldTypes = {};
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("makeUnit", structType, {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* emptyStruct = builder->createGCAlloc(structType, "unit");
    builder->createRet(emptyStruct);

    // Should throw - GCAlloc not implemented
    BytecodeCompiler compiler;
    EXPECT_THROW({
        auto bytecode = compiler.compile(std::move(module));
    }, std::runtime_error);
}

TEST_F(BytecodeCompilerTest, CompileLargeStruct) {
    // Edge case: Struct with many fields (10+)

    std::vector<std::shared_ptr<IRType>> fieldTypes;
    for (int i = 0; i < 15; i++) {
        fieldTypes.push_back(builder->getIntType());
    }
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("getField7", builder->getIntType(), {structType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* bigStruct = func->getParam(0);
    auto* field7 = builder->createExtractValue(bigStruct, 7, "field7");

    builder->createRet(field7);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileStructPassThrough) {
    // Test: Return struct parameter unchanged (no operations)

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getIntType(),
        builder->getIntType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("identity", structType, {structType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* param = func->getParam(0);
    builder->createRet(param);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}

TEST_F(BytecodeCompilerTest, CompileStructExtractInsertSameField) {
    // Edge case: Extract and immediately re-insert the same field
    // value = point.x; point.x = value (no-op semantically)

    std::vector<std::shared_ptr<IRType>> fieldTypes = {
        builder->getFloatType(),
        builder->getFloatType()
    };
    auto structType = std::make_shared<IRStructType>(fieldTypes);

    auto* func = builder->createFunction("noOp", structType, {structType});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);

    auto* point = func->getParam(0);

    // Extract x
    auto* x = builder->createExtractValue(point, 0, "x");

    // Insert it back
    auto* newPoint = builder->createInsertValue(point, x, 0, "");

    builder->createRet(newPoint);

    // Compile
    BytecodeCompiler compiler;
    auto bytecode = compiler.compile(std::move(module));

    ASSERT_NE(bytecode, nullptr);
    EXPECT_TRUE(bytecode->verify());
}