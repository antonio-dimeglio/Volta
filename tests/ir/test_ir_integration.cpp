#include <gtest/gtest.h>
#include <memory>
#include "IR/Module.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRPrinter.hpp"
#include "IR/OptimizationPass.hpp"
#include "IR/Verifier.hpp"

using namespace volta::ir;

/**
 * Integration Test Suite
 *
 * Tests the full IR pipeline: IR construction → verification → optimization
 * These tests build complex IR programs programmatically and ensure soundness.
 */

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("integration_test");
        builder = std::make_unique<IRBuilder>(*module);
    }

    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder> builder;
    
    // Helper: Verify IR is well-formed
    bool verifyModule() {
        Verifier verifier;
        return verifier.verify(*module);
    }
};

// ============================================================================
// Basic Arithmetic Programs
// ============================================================================

TEST_F(IntegrationTest, SimpleAdditionProgram) {
    // Build: fn add(a: i64, b: i64) -> i64 { return a + b; }
    auto* func = builder->createFunction("add", builder->getIntType(), 
                                         {builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    
    auto* a = func->getParam(0);
    auto* b = func->getParam(1);
    auto* sum = builder->createAdd(a, b, "sum");
    builder->createRet(sum);
    
    // Verify
    EXPECT_TRUE(verifyModule());
    
    // Check structure
    EXPECT_EQ(func->getNumParams(), 2);
    EXPECT_EQ(entry->getNumInstructions(), 2); // add + ret
}

TEST_F(IntegrationTest, ComplexArithmeticProgram) {
    // Build: fn calc(x: i64) -> i64 { return (x + 5) * 2 - 10; }
    auto* func = builder->createFunction("calc", builder->getIntType(), {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    
    auto* x = func->getParam(0);
    auto* five = builder->getInt(5);
    auto* two = builder->getInt(2);
    auto* ten = builder->getInt(10);
    
    auto* add_result = builder->createAdd(x, five);
    auto* mul_result = builder->createMul(add_result, two);
    auto* final_result = builder->createSub(mul_result, ten);
    builder->createRet(final_result);
    
    EXPECT_TRUE(verifyModule());
    EXPECT_EQ(entry->getNumInstructions(), 4); // 3 ops + ret
}

// ============================================================================
// Control Flow Programs
// ============================================================================

TEST_F(IntegrationTest, IfElseProgram) {
    // Build: fn max(a: i64, b: i64) -> i64 { if (a > b) return a; else return b; }
    auto* func = builder->createFunction("max", builder->getIntType(),
                                         {builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    auto* thenBB = module->createBasicBlock("if.then", func);
    auto* elseBB = module->createBasicBlock("if.else", func);
    
    builder->setInsertionPoint(entry);
    auto* a = func->getParam(0);
    auto* b = func->getParam(1);
    auto* cond = builder->createGt(a, b);
    builder->createCondBr(cond, thenBB, elseBB);
    
    builder->setInsertionPoint(thenBB);
    builder->createRet(a);
    
    builder->setInsertionPoint(elseBB);
    builder->createRet(b);
    
    EXPECT_TRUE(verifyModule());
    EXPECT_EQ(func->getNumBlocks(), 3);
}

TEST_F(IntegrationTest, WhileLoopProgram) {
    // Build: fn count(n: i64) -> i64 {
    //   let mut i = 0;
    //   while (i < n) { i = i + 1; }
    //   return i;
    // }
    auto* func = builder->createFunction("count", builder->getIntType(), {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    auto* loopHeader = module->createBasicBlock("loop.header", func);
    auto* loopBody = module->createBasicBlock("loop.body", func);
    auto* loopExit = module->createBasicBlock("loop.exit", func);
    
    builder->setInsertionPoint(entry);
    auto* i_alloca = builder->createAlloca(builder->getIntType(), "i");
    builder->createStore(builder->getInt(0), i_alloca);
    builder->createBr(loopHeader);
    
    builder->setInsertionPoint(loopHeader);
    auto* i_val = builder->createLoad(i_alloca, "i.val");
    auto* n = func->getParam(0);
    auto* cond = builder->createLt(i_val, n);
    builder->createCondBr(cond, loopBody, loopExit);
    
    builder->setInsertionPoint(loopBody);
    auto* i_inc = builder->createAdd(i_val, builder->getInt(1));
    builder->createStore(i_inc, i_alloca);
    builder->createBr(loopHeader);
    
    builder->setInsertionPoint(loopExit);
    auto* result = builder->createLoad(i_alloca, "result");
    builder->createRet(result);
    
    EXPECT_TRUE(verifyModule());
    EXPECT_EQ(func->getNumBlocks(), 4);
}

// ============================================================================
// Function Calls
// ============================================================================

TEST_F(IntegrationTest, FunctionCallProgram) {
    // Build two functions: helper and main
    auto* helper = builder->createFunction("helper", builder->getIntType(), {builder->getIntType()});
    auto* helperEntry = helper->getEntryBlock();
    builder->setInsertionPoint(helperEntry);
    auto* x = helper->getParam(0);
    auto* doubled = builder->createMul(x, builder->getInt(2));
    builder->createRet(doubled);
    
    auto* main = builder->createFunction("main", builder->getIntType(), {});
    auto* mainEntry = main->getEntryBlock();
    builder->setInsertionPoint(mainEntry);
    auto* result = builder->createCall(helper, {builder->getInt(21)});
    builder->createRet(result);
    
    EXPECT_TRUE(verifyModule());
    EXPECT_EQ(module->getNumFunctions(), 2);
}

TEST_F(IntegrationTest, RecursiveFunctionProgram) {
    // Build: fn factorial(n: i64) -> i64 {
    //   if (n <= 1) return 1;
    //   return n * factorial(n - 1);
    // }
    auto* func = builder->createFunction("factorial", builder->getIntType(), {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    auto* baseCaseBB = module->createBasicBlock("base.case", func);
    auto* recursiveBB = module->createBasicBlock("recursive", func);
    
    builder->setInsertionPoint(entry);
    auto* n = func->getParam(0);
    auto* cond = builder->createLe(n, builder->getInt(1));
    builder->createCondBr(cond, baseCaseBB, recursiveBB);
    
    builder->setInsertionPoint(baseCaseBB);
    builder->createRet(builder->getInt(1));
    
    builder->setInsertionPoint(recursiveBB);
    auto* n_minus_1 = builder->createSub(n, builder->getInt(1));
    auto* rec_result = builder->createCall(func, {n_minus_1});
    auto* final_result = builder->createMul(n, rec_result);
    builder->createRet(final_result);
    
    EXPECT_TRUE(verifyModule());
}

// ============================================================================
// Complex Programs
// ============================================================================

TEST_F(IntegrationTest, FibonacciProgram) {
    // Build recursive fibonacci
    auto* func = builder->createFunction("fib", builder->getIntType(), {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    auto* baseCaseBB = module->createBasicBlock("base.case", func);
    auto* recursiveBB = module->createBasicBlock("recursive", func);
    
    builder->setInsertionPoint(entry);
    auto* n = func->getParam(0);
    auto* cond = builder->createLe(n, builder->getInt(1));
    builder->createCondBr(cond, baseCaseBB, recursiveBB);
    
    builder->setInsertionPoint(baseCaseBB);
    builder->createRet(n);
    
    builder->setInsertionPoint(recursiveBB);
    auto* n_1 = builder->createSub(n, builder->getInt(1));
    auto* n_2 = builder->createSub(n, builder->getInt(2));
    auto* fib_n1 = builder->createCall(func, {n_1});
    auto* fib_n2 = builder->createCall(func, {n_2});
    auto* result = builder->createAdd(fib_n1, fib_n2);
    builder->createRet(result);
    
    EXPECT_TRUE(verifyModule());
}

TEST_F(IntegrationTest, GCDProgram) {
    // Build: fn gcd(a: i64, b: i64) -> i64 {
    //   while (b != 0) {
    //     let temp = b;
    //     b = a % b;
    //     a = temp;
    //   }
    //   return a;
    // }
    auto* func = builder->createFunction("gcd", builder->getIntType(),
                                         {builder->getIntType(), builder->getIntType()});
    auto* entry = func->getEntryBlock();
    auto* loopHeader = module->createBasicBlock("loop.header", func);
    auto* loopBody = module->createBasicBlock("loop.body", func);
    auto* loopExit = module->createBasicBlock("loop.exit", func);
    
    builder->setInsertionPoint(entry);
    auto* a_alloca = builder->createAlloca(builder->getIntType(), "a");
    auto* b_alloca = builder->createAlloca(builder->getIntType(), "b");
    builder->createStore(func->getParam(0), a_alloca);
    builder->createStore(func->getParam(1), b_alloca);
    builder->createBr(loopHeader);
    
    builder->setInsertionPoint(loopHeader);
    auto* b_val = builder->createLoad(b_alloca, "b.val");
    auto* cond = builder->createNe(b_val, builder->getInt(0));
    builder->createCondBr(cond, loopBody, loopExit);
    
    builder->setInsertionPoint(loopBody);
    auto* temp_alloca = builder->createAlloca(builder->getIntType(), "temp");
    builder->createStore(b_val, temp_alloca);
    auto* a_val = builder->createLoad(a_alloca, "a.val");
    auto* new_b = builder->createRem(a_val, b_val);
    builder->createStore(new_b, b_alloca);
    auto* temp_val = builder->createLoad(temp_alloca, "temp.val");
    builder->createStore(temp_val, a_alloca);
    builder->createBr(loopHeader);
    
    builder->setInsertionPoint(loopExit);
    auto* result = builder->createLoad(a_alloca, "result");
    builder->createRet(result);
    
    EXPECT_TRUE(verifyModule());
}

// ============================================================================
// Optimization Integration
// ============================================================================

TEST_F(IntegrationTest, ConstantFoldingIntegration) {
    // Build function with constant operations that should be folded
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    
    // Build: return 5 + 3 * 2
    auto* five = builder->getInt(5);
    auto* three = builder->getInt(3);
    auto* two = builder->getInt(2);
    auto* mul = builder->createMul(three, two);
    auto* add = builder->createAdd(five, mul);
    builder->createRet(add);
    
    // Before optimization
    EXPECT_GT(entry->getNumInstructions(), 1);
    
    // Run constant folding
    ConstantFoldingPass cfPass;
    bool changed = cfPass.runOnModule(*module);
    
    EXPECT_TRUE(changed);
    EXPECT_TRUE(verifyModule());
}

TEST_F(IntegrationTest, Mem2RegIntegration) {
    // Build function with alloca that can be promoted
    auto* func = builder->createFunction("test", builder->getIntType(), {});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    
    auto* x = builder->createAlloca(builder->getIntType(), "x");
    builder->createStore(builder->getInt(42), x);
    auto* val = builder->createLoad(x, "val");
    builder->createRet(val);
    
    // Before mem2reg
    EXPECT_GT(entry->getNumInstructions(), 1);
    
    // Run mem2reg
    Mem2RegPass mem2reg;
    bool changed = mem2reg.runOnModule(*module);
    
    EXPECT_TRUE(changed);
    EXPECT_TRUE(verifyModule());
}

TEST_F(IntegrationTest, FullOptimizationPipeline) {
    // Build a complex function and run full optimization pipeline
    auto* func = builder->createFunction("complex", builder->getIntType(), {builder->getIntType()});
    auto* entry = func->getEntryBlock();
    builder->setInsertionPoint(entry);
    
    auto* x = func->getParam(0);
    
    // Create some redundant allocas
    auto* temp1 = builder->createAlloca(builder->getIntType(), "temp1");
    builder->createStore(x, temp1);
    auto* val1 = builder->createLoad(temp1);
    
    // Add some constant expressions
    auto* const_result = builder->createAdd(builder->getInt(10), builder->getInt(20));
    
    // Use the values
    auto* result = builder->createAdd(val1, const_result);
    builder->createRet(result);
    
    EXPECT_TRUE(verifyModule());
    
    // Run optimization pipeline
    PassManager pm;
    pm.addPass(std::make_unique<ConstantFoldingPass>());
    pm.addPass(std::make_unique<Mem2RegPass>());
    pm.addPass(std::make_unique<DeadCodeEliminationPass>());
    
    bool changed = pm.run(*module);
    EXPECT_TRUE(changed);
    EXPECT_TRUE(verifyModule());
}

// ============================================================================
// SSA Form Verification
// ============================================================================

TEST_F(IntegrationTest, PhiNodeCorrectness) {
    // Build diamond CFG that should generate phi nodes
    auto* func = builder->createFunction("diamond", builder->getIntType(), {builder->getBoolType()});
    auto* entry = func->getEntryBlock();
    auto* thenBB = module->createBasicBlock("if.then", func);
    auto* elseBB = module->createBasicBlock("if.else", func);
    auto* mergeBB = module->createBasicBlock("if.merge", func);
    
    builder->setInsertionPoint(entry);
    auto* cond = func->getParam(0);
    auto* x = builder->createAlloca(builder->getIntType(), "x");
    builder->createCondBr(cond, thenBB, elseBB);
    
    builder->setInsertionPoint(thenBB);
    builder->createStore(builder->getInt(10), x);
    builder->createBr(mergeBB);
    
    builder->setInsertionPoint(elseBB);
    builder->createStore(builder->getInt(20), x);
    builder->createBr(mergeBB);
    
    builder->setInsertionPoint(mergeBB);
    auto* result = builder->createLoad(x, "result");
    builder->createRet(result);
    
    EXPECT_TRUE(verifyModule());
    
    // After mem2reg, merge block should have phi
    Mem2RegPass mem2reg;
    mem2reg.runOnModule(*module);

    EXPECT_TRUE(verifyModule());
}

// ============================================================================
// If-Else-If Chain Tests (from full parsing pipeline)
// ============================================================================

#include "lexer/lexer.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "IR/IRGenerator.hpp"
#include "error/ErrorReporter.hpp"
#include <sstream>

class IRIfElseChainTest : public ::testing::Test {
protected:
    std::unique_ptr<Module> generateIRFromSource(const std::string& source) {
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
        auto module = generateIR(*ast, analyzer, "test_module");
        if (!module) {
            throw std::runtime_error("IR generation failed");
        }

        return module;
    }
    
    bool verifyIRModule(const Module& module) {
        Verifier verifier;
        if (!verifier.verify(module)) {
            std::cerr << "IR verification errors:\n";
            for (const auto& error : verifier.getErrors()) {
                std::cerr << "  - " << error << "\n";
            }
            return false;
        }
        return true;
    }

    Function* getFunction(Module& module, const std::string& name) {
        for (auto* func : module.getFunctions()) {
            if (func->getName() == name) {
                return func;
            }
        }
        return nullptr;
    }
};

TEST_F(IRIfElseChainTest, SimpleIfElse) {
    std::string source = R"(
fn test(x: int) -> int {
    if x > 0 {
        return 1
    } else {
        return 0
    }
}
    )";

    auto module = generateIRFromSource(source);
    ASSERT_TRUE(module);

    std::cout << "\n=== SimpleIfElse ===\n";

    EXPECT_TRUE(verifyIRModule(*module));

    auto* func = getFunction(*module, "test");
    ASSERT_NE(func, nullptr);
    EXPECT_GE(func->getNumBlocks(), 2);
}

TEST_F(IRIfElseChainTest, SingleElseIf) {
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

    auto module = generateIRFromSource(source);
    ASSERT_TRUE(module);

    std::cout << "\n=== SingleElseIf ===\n";

    EXPECT_TRUE(verifyIRModule(*module));

    auto* func = getFunction(*module, "test");
    ASSERT_NE(func, nullptr);

    std::cout << "Number of blocks: " << func->getNumBlocks() << std::endl;
    EXPECT_GE(func->getNumBlocks(), 4);
}

TEST_F(IRIfElseChainTest, MultipleElseIfs) {
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

    auto module = generateIRFromSource(source);
    ASSERT_TRUE(module);

    std::cout << "\n=== MultipleElseIfs ===\n";

    EXPECT_TRUE(verifyIRModule(*module));

    auto* func = getFunction(*module, "test");
    ASSERT_NE(func, nullptr);

    std::cout << "Number of blocks: " << func->getNumBlocks() << std::endl;
}

TEST_F(IRIfElseChainTest, ElseIfWithAssignments) {
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

    auto module = generateIRFromSource(source);
    ASSERT_TRUE(module);

    std::cout << "\n=== ElseIfWithAssignments ===\n";

    EXPECT_TRUE(verifyIRModule(*module));

    auto* func = getFunction(*module, "test");
    ASSERT_NE(func, nullptr);
    EXPECT_GE(func->getNumBlocks(), 4);
}

TEST_F(IRIfElseChainTest, NestedIfElseIf) {
    std::string source = R"(
fn test(x: int, y: int) -> int {
    if x < 0 {
        if y < 0 {
            return -2
        } else {
            return -1
        }
    } else if x == 0 {
        return 0
    } else {
        return 1
    }
}
    )";

    auto module = generateIRFromSource(source);
    ASSERT_TRUE(module);

    std::cout << "\n=== NestedIfElseIf ===\n";

    EXPECT_TRUE(verifyIRModule(*module));
}

TEST_F(IRIfElseChainTest, DetailedBlockAnalysis) {
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

    auto module = generateIRFromSource(source);
    ASSERT_TRUE(module);

    std::cout << "\n=== DetailedBlockAnalysis ===\n";

    EXPECT_TRUE(verifyIRModule(*module));

    auto* func = getFunction(*module, "classify");
    ASSERT_NE(func, nullptr);

    // Detailed block analysis
    auto& blocks = func->getBlocks();
    std::cout << "\n=== Block Structure Analysis ===\n";
    std::cout << "Total blocks: " << blocks.size() << std::endl;

    for (size_t i = 0; i < blocks.size(); i++) {
        auto* block = blocks[i];
        std::cout << "\nBlock " << i << " (" << block << "):\n";
        std::cout << "  Predecessors: " << block->getNumPredecessors() << "\n";
        std::cout << "  Successors: " << block->getSuccessors().size() << "\n";
        std::cout << "  Instructions: " << block->getInstructions().size() << "\n";

        if (block->hasTerminator()) {
            auto* term = block->getTerminator();
            std::cout << "  Terminator: ";

            switch (term->getOpcode()) {
                case Instruction::Opcode::Ret:
                    std::cout << "RET";
                    if (static_cast<ReturnInst*>(term)->hasReturnValue()) {
                        std::cout << " (with value)";
                    }
                    break;
                case Instruction::Opcode::Br:
                    std::cout << "BR (unconditional)";
                    break;
                case Instruction::Opcode::CondBr:
                    std::cout << "CONDBR (conditional)";
                    break;
                default:
                    std::cout << "OTHER";
            }
            std::cout << "\n";
        } else {
            std::cout << "  Terminator: NONE (ERROR!)\n";
        }

        if (!block->getSuccessors().empty()) {
            std::cout << "  Successor blocks:\n";
            for (auto* succ : block->getSuccessors()) {
                for (size_t j = 0; j < blocks.size(); j++) {
                    if (blocks[j] == succ) {
                        std::cout << "    -> Block " << j << "\n";
                        break;
                    }
                }
            }
        }
    }
}
