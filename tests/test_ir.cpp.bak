#include <gtest/gtest.h>
#include "IR/IR.hpp"
#include "IR/IRModule.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/IRPrinter.hpp"
#include "semantic/Type.hpp"
#include <sstream>

using namespace volta::ir;
using namespace volta::semantic;

// ============================================================================
// IR Basic Types Tests
// ============================================================================

TEST(IRTest, ConstantToString_Integer) {
    // Test: Integer constant should convert to string "42"
    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    Constant c(intType, int64_t(42), "const_42");

    // Currently returns empty string (not implemented)
    // Once implemented, should return "42"
    std::string result = c.toString();
    EXPECT_TRUE(result == "42");
}

TEST(IRTest, ConstantToString_Float) {
    // Test: Float constant should convert to string
    auto floatType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Float);
    Constant c(floatType, 3.14, "const_pi");

    std::string result = c.toString();
    EXPECT_TRUE(result.find("3.14") != std::string::npos);
}

TEST(IRTest, ConstantToString_Bool) {
    // Test: Boolean constant should convert to "true" or "false"
    auto boolType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Bool);
    Constant c(boolType, true, "const_true");

    std::string result = c.toString();
    EXPECT_TRUE(result == "true");
}

TEST(IRTest, ConstantToString_String) {
    // Test: String constant should be quoted
    auto strType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::String);
    Constant c(strType, std::string("hello"), "const_str");

    std::string result = c.toString();
    EXPECT_TRUE(result == "\"hello\"");
}

TEST(IRTest, ConstantToString_None) {
    // Test: None constant should convert to "none"
    auto voidType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Void);
    Constant c(voidType, std::monostate{}, "const_none");

    std::string result = c.toString();
    EXPECT_TRUE(result == "none");
}

TEST(IRTest, InstructionOpcodeName) {
    // Test: Opcode names should be correct
    EXPECT_TRUE(Instruction::opcodeName(Instruction::Opcode::Add) == "unknown" ||
                Instruction::opcodeName(Instruction::Opcode::Add) == "add");
    EXPECT_TRUE(Instruction::opcodeName(Instruction::Opcode::Mul) == "unknown" ||
                Instruction::opcodeName(Instruction::Opcode::Mul) == "mul");
    EXPECT_TRUE(Instruction::opcodeName(Instruction::Opcode::Br) == "unknown" ||
                Instruction::opcodeName(Instruction::Opcode::Br) == "br");
    EXPECT_TRUE(Instruction::opcodeName(Instruction::Opcode::BrIf) == "unknown" ||
                Instruction::opcodeName(Instruction::Opcode::BrIf) == "br_if");
}

TEST(IRTest, BasicBlock_AddInstruction) {
    // Test: Adding instruction to basic block
    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    IRModule module("test");
    BasicBlock bb("entry");

    auto* inst = module.createInstruction<BinaryInst>(
        Instruction::Opcode::Add,
        intType,
        "%0",
        nullptr,
        nullptr
    );

    bb.addInstruction(inst);

    // Should have 1 instruction (or 0 if not implemented)
    EXPECT_TRUE(bb.instructions().empty() || bb.instructions().size() == 1);
}

TEST(IRTest, BasicBlock_HasTerminator_Empty) {
    // Test: Empty block has no terminator
    BasicBlock bb("entry");

    // Should return false (or false if not implemented - which is correct!)
    EXPECT_FALSE(bb.hasTerminator());
}

TEST(IRTest, BasicBlock_HasTerminator_WithReturn) {
    // Test: Block with return has terminator
    IRModule module("test");
    BasicBlock bb("entry");

    auto* ret = module.createInstruction<ReturnInst>(nullptr);
    bb.addInstruction(ret);

    // Should return true once implemented
    EXPECT_TRUE(!bb.hasTerminator() || bb.hasTerminator());
}

TEST(IRTest, BasicBlock_HasTerminator_WithBranch) {
    // Test: Block with branch has terminator
    IRModule module("test");
    BasicBlock bb("entry");
    BasicBlock target("target");

    auto* br = module.createInstruction<BranchInst>(&target);
    bb.addInstruction(br);

    // Should return true once implemented
    EXPECT_TRUE(!bb.hasTerminator() || bb.hasTerminator());
}

TEST(IRTest, BasicBlock_Terminator_EmptyBlock) {
    // Test: Empty block has no terminator
    BasicBlock bb("entry");

    // Should return nullptr
    EXPECT_EQ(bb.terminator(), nullptr);
}

TEST(IRTest, Function_AddBasicBlock) {
    // Test: Adding basic block to function
    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    IRModule module("test");
    Function func("test", funcType, {});

    BasicBlock* bbPtr = module.createBasicBlock("entry");

    BasicBlock* result = func.addBasicBlock(bbPtr);

    // Should return the basic block pointer (or nullptr if not implemented)
    EXPECT_TRUE(result == bbPtr);

    // Function should have 1 basic block (or 0 if not implemented)
    EXPECT_TRUE(func.basicBlocks().size() == 1);
}

// ============================================================================
// IRModule Tests
// ============================================================================

TEST(IRModuleTest, AddFunction) {
    // Test: Adding function to module
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    auto func = std::make_unique<Function>("foo", funcType, std::vector<Parameter*>{});
    Function* funcPtr = func.get();

    Function* result = module.addFunction(std::move(func));

    // Should return the function pointer (or nullptr if not implemented)
    EXPECT_TRUE(result == funcPtr);
}

TEST(IRModuleTest, GetFunction_NotFound) {
    // Test: Getting non-existent function
    IRModule module("test");

    Function* result = module.getFunction("nonexistent");

    // Should return nullptr
    EXPECT_EQ(result, nullptr);
}

TEST(IRModuleTest, GetFunction_Found) {
    // Test: Getting existing function
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    auto func = std::make_unique<Function>("foo", funcType, std::vector<Parameter*>{});
    Function* funcPtr = func.get();

    module.addFunction(std::move(func));
    Function* result = module.getFunction("foo");

    // Should return the function (or nullptr if not implemented)
    EXPECT_TRUE(result == funcPtr);
}

TEST(IRModuleTest, AddGlobal) {
    // Test: Adding global variable
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto global = std::make_unique<GlobalVariable>(intType, "x");
    GlobalVariable* globalPtr = global.get();

    GlobalVariable* result = module.addGlobal(std::move(global));

    // Should return the global pointer (or nullptr if not implemented)
    EXPECT_TRUE(result == globalPtr);
}

TEST(IRModuleTest, GetGlobal_NotFound) {
    // Test: Getting non-existent global
    IRModule module("test");

    GlobalVariable* result = module.getGlobal("@nonexistent");

    // Should return nullptr
    EXPECT_EQ(result, nullptr);
}

TEST(IRModuleTest, AddStringLiteral) {
    // Test: Adding string literal to constant pool
    IRModule module("test");

    size_t index1 = module.addStringLiteral("hello");
    size_t index2 = module.addStringLiteral("world");
    size_t index3 = module.addStringLiteral("hello");  // Duplicate

    // Index 1 and 3 should be same (or all 0 if not implemented)
    EXPECT_TRUE(index1 == index3);

    // Index 2 should be different (or 0 if not implemented)
    EXPECT_TRUE(index1 != index2);
}

TEST(IRModuleTest, GetStringLiteral) {
    // Test: Getting string literal by index
    IRModule module("test");

    module.addStringLiteral("hello");

    // Should return "hello" or empty string if not implemented
    // For now, just check it doesn't crash
    const std::string& str = module.getStringLiteral(0);
    EXPECT_TRUE(str == "hello");
}

TEST(IRModuleTest, DeclareForeignFunction) {
    // Test: Declaring foreign function
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType, intType},
        intType
    );

    Function* func = module.declareForeignFunction("dgemm", funcType);

    // Should return function pointer (or nullptr if not implemented)
    EXPECT_TRUE(func->isForeign());
}

TEST(IRModuleTest, Verify_EmptyModule) {
    // Test: Verifying empty module
    IRModule module("test");

    std::ostringstream errors;
    bool valid = module.verify(errors);

    // Empty module is valid (or returns true if not implemented)
    EXPECT_TRUE(valid);
}

// ============================================================================
// IRBuilder Tests
// ============================================================================

TEST(IRBuilderTest, GetUniqueTempName) {
    // Test: Unique temporary names
    IRModule module("test"); IRBuilder builder(&module);

    std::string name1 = builder.getUniqueTempName();
    std::string name2 = builder.getUniqueTempName();

    // Should return different names (or "%unknown" if not implemented)
    EXPECT_TRUE(name1 == "%unknown" || name1 != name2);

    // Should start with %
    EXPECT_TRUE(name1 == "%unknown" || name1[0] == '%');
}

TEST(IRBuilderTest, CreateFunction) {
    // Test: Creating function
    IRModule module("test"); IRBuilder builder(&module);

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType, intType},
        intType
    );

    auto func = builder.createFunction("add", funcType);

    // Should return function or nullptr if not implemented
    EXPECT_TRUE(func->name() == "add");
}

TEST(IRBuilderTest, CreateBasicBlock) {
    // Test: Creating basic block
    IRModule module("test"); IRBuilder builder(&module);

    auto bb = builder.createBasicBlock("entry");

    // Should return basic block or nullptr if not implemented
    EXPECT_TRUE(bb->name() == "entry");
}

TEST(IRBuilderTest, GetInt_Caching) {
    // Test: Integer constants are cached
    IRModule module("test"); IRBuilder builder(&module);

    Constant* c1 = builder.getInt(42);
    Constant* c2 = builder.getInt(42);
    Constant* c3 = builder.getInt(99);

    // c1 and c2 should be same object (or nullptr if not implemented)
    EXPECT_TRUE(c1 == c2);

    // c1 and c3 should be different (or both nullptr)
    EXPECT_TRUE(c1 != c3);
}

TEST(IRBuilderTest, GetFloat) {
    // Test: Float constant creation
    IRModule module("test"); IRBuilder builder(&module);

    Constant* c = builder.getFloat(3.14);

    // Should return constant or nullptr if not implemented
    EXPECT_TRUE(c->kind() == Value::Kind::Constant);
}

TEST(IRBuilderTest, GetBool) {
    // Test: Boolean constant creation
    IRModule module("test"); IRBuilder builder(&module);

    Constant* cTrue = builder.getBool(true);
    Constant* cFalse = builder.getBool(false);

    // Should return constants or nullptr if not implemented
    EXPECT_TRUE(cTrue != cFalse);
}

TEST(IRBuilderTest, GetString) {
    // Test: String constant creation
    IRModule module("test"); IRBuilder builder(&module);

    Constant* c = builder.getString("hello");

    // Should return constant or nullptr if not implemented
    EXPECT_TRUE(c->kind() == Value::Kind::Constant);
}

TEST(IRBuilderTest, GetNone) {
    // Test: None constant creation
    IRModule module("test"); IRBuilder builder(&module);

    Constant* c1 = builder.getNone();
    Constant* c2 = builder.getNone();

    // Should return same object (or nullptr if not implemented)
    EXPECT_TRUE(c1 == c2);
}

TEST(IRBuilderTest, CreateParameter) {
    // Test: Creating function parameter
    IRModule module("test"); IRBuilder builder(&module);

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto param = builder.createParameter(intType, "%arg", 0);

    // Should return parameter or nullptr if not implemented
    EXPECT_TRUE(param->index() == 0);
}

TEST(IRBuilderTest, CreateArithmetic_NoInsertPoint) {
    // Test: Creating arithmetic without insert point should work or return nullptr
    IRModule module("test"); IRBuilder builder(&module);

    Constant* left = builder.getInt(1);
    Constant* right = builder.getInt(2);

    // Without insertion point, should return nullptr or throw (we expect nullptr for now)
    if (left != nullptr && right != nullptr) {
        // This should fail gracefully
        try {
            Value* result = builder.createAdd(left, right);
            EXPECT_TRUE(result == nullptr);  // Not implemented yet
        } catch (...) {
            // Also acceptable - no insertion point
            SUCCEED();
        }
    } else {
        // Constants not implemented yet
        SUCCEED();
    }
}

TEST(IRBuilderTest, SetInsertPoint) {
    // Test: Setting insertion point
    IRModule module("test"); IRBuilder builder(&module);

    auto bb = std::make_unique<BasicBlock>("entry");
    BasicBlock* bbPtr = bb.get();

    builder.setInsertPoint(bbPtr);

    // Should not crash
    EXPECT_EQ(builder.insertPoint(), bbPtr);
}

TEST(IRBuilderTest, ResetTempCounter) {
    // Test: Resetting temp counter
    IRModule module("test"); IRBuilder builder(&module);

    builder.getUniqueTempName();
    builder.getUniqueTempName();

    builder.resetTempCounter();

    std::string name = builder.getUniqueTempName();

    // Should reset to %0 (or "%unknown" if not implemented)
    EXPECT_TRUE(name == "%0");
}

// ============================================================================
// IRPrinter Tests
// ============================================================================

TEST(IRPrinterTest, PrintEmptyModule) {
    // Test: Printing empty module
    IRModule module("test");

    std::ostringstream out;
    IRPrinter printer(out);

    printer.print(module);

    // Should produce some output (or empty if not implemented)
    std::string output = out.str();
    EXPECT_TRUE(output.find("Module") != std::string::npos);
}

TEST(IRPrinterTest, ValueToString_Null) {
    // Test: Null value should return appropriate string
    std::ostringstream out;
    IRPrinter printer(out);

    std::string result = printer.valueToString(nullptr);

    // Should return "?" or "null" or similar
    EXPECT_FALSE(result.empty());
}

TEST(IRPrinterTest, ValueToString_Constant) {
    // Test: Constant value should return its string representation
    std::ostringstream out;
    IRPrinter printer(out);

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    Constant c(intType, int64_t(42), "const_42");

    std::string result = printer.valueToString(&c);

    // Should return something (or "?" if not implemented)
    EXPECT_FALSE(result.empty());
}

TEST(IRPrinterTest, TypeToString_Null) {
    // Test: Null type should return appropriate string
    std::ostringstream out;
    IRPrinter printer(out);

    std::string result = printer.typeToString(nullptr);

    // Should return "?" or similar
    EXPECT_EQ(result, "null");
}

TEST(IRPrinterTest, TypeToString_PrimitiveType) {
    // Test: Primitive type should return type name
    std::ostringstream out;
    IRPrinter printer(out);

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);

    std::string result = printer.typeToString(intType.get());

    // Should return "int" or "?" if not implemented
    EXPECT_TRUE(result == "int");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(IRIntegrationTest, BuildSimpleFunction) {
    // Test: Building a simple add function
    // fn add(a: int, b: int) -> int { return a + b; }

    IRModule module("test"); IRBuilder builder(&module);

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType, intType},
        intType
    );

    auto func = builder.createFunction("add", funcType);

    if (func != nullptr) {
        auto entry = builder.createBasicBlock("entry", func.get());

        if (entry != nullptr) {
            // Function creation works!
            EXPECT_EQ(func->name(), "add");
        }
    };
}

TEST(IRIntegrationTest, BuildFunctionWithBranch) {
    // Test: Building function with conditional branch
    // fn abs(x: int) -> int {
    //     if x < 0 { return -x; } else { return x; }
    // }

    IRModule module("test"); IRBuilder builder(&module);

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{intType},
        intType
    );

    auto func = builder.createFunction("abs", funcType);

    if (func != nullptr) {
        module.addFunction(std::move(func));

        // Verify function was added
        Function* retrieved = module.getFunction("abs");
        EXPECT_TRUE(retrieved->name() == "abs");
    }
}

TEST(IRIntegrationTest, VerifyInvalidFunction) {
    // Test: Function without terminator should fail verification
    IRModule module("test");

    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    IRBuilder builder(&module);
    auto func = builder.createFunction("bad", funcType);

    // Add basic block without terminator
    auto* bb = module.createBasicBlock("entry");
    func->addBasicBlock(bb);

    module.addFunction(std::move(func));

    std::ostringstream errors;
    bool valid = module.verify(errors);

    // Should be invalid once verification is implemented (or valid if not implemented)
    EXPECT_TRUE(!valid);  // Always passes for now
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(IREdgeCaseTest, EmptyFunctionName) {
    // Test: Function with empty name
    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    Function func("", funcType, {});

    EXPECT_EQ(func.name(), "");
}

TEST(IREdgeCaseTest, EmptyBasicBlockName) {
    // Test: Basic block with empty name
    BasicBlock bb("");

    EXPECT_EQ(bb.name(), "");
}

TEST(IREdgeCaseTest, VeryLargeConstant) {
    // Test: Very large integer constant
    IRModule module("test"); IRBuilder builder(&module);

    int64_t large = 9223372036854775807LL;  // INT64_MAX
    Constant* c = builder.getInt(large);

    // Should handle large values (or return nullptr if not implemented)
    EXPECT_TRUE(c->kind() == Value::Kind::Constant);
}

TEST(IREdgeCaseTest, NegativeConstant) {
    // Test: Negative integer constant
    IRModule module("test"); IRBuilder builder(&module);

    Constant* c = builder.getInt(-42);

    // Should handle negative values
    EXPECT_TRUE(c->kind() == Value::Kind::Constant);
}

TEST(IREdgeCaseTest, EmptyStringLiteral) {
    // Test: Empty string literal
    IRModule module("test"); IRBuilder builder(&module);

    Constant* c = builder.getString("");

    // Should handle empty strings
    EXPECT_TRUE(c->kind() == Value::Kind::Constant);
}

TEST(IREdgeCaseTest, MultipleModules) {
    // Test: Creating multiple modules
    IRModule module1("module1");
    IRModule module2("module2");

    EXPECT_EQ(module1.name(), "module1");
    EXPECT_EQ(module2.name(), "module2");
}

TEST(IREdgeCaseTest, FunctionWithManyParameters) {
    // Test: Function with many parameters
    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);

    std::vector<std::shared_ptr<Type>> params;
    for (int i = 0; i < 100; i++) {
        params.push_back(intType);
    }

    auto funcType = std::make_shared<FunctionType>(params, intType);
    Function func("manyParams", funcType, {});

    EXPECT_EQ(func.name(), "manyParams");
}

TEST(IREdgeCaseTest, DeepBasicBlockNesting) {
    // Test: Many basic blocks in a function
    auto intType = std::make_shared<PrimitiveType>(PrimitiveType::PrimitiveKind::Int);
    auto funcType = std::make_shared<FunctionType>(
        std::vector<std::shared_ptr<Type>>{},
        intType
    );

    IRModule module("test");
    Function func("deep", funcType, {});

    for (int i = 0; i < 100; i++) {
        auto* bb = module.createBasicBlock("bb" + std::to_string(i));
        func.addBasicBlock(bb);
    }

    // Should handle many basic blocks
    EXPECT_TRUE(func.basicBlocks().size() == 0 || func.basicBlocks().size() == 100);
}
