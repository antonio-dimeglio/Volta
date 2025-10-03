#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "semantic/Type.hpp"

using namespace volta::ir;
using namespace volta::semantic;

// ============================================================================
// Test: Instruction Base Class
// ============================================================================

TEST_CASE("Instruction - Opcode Names", "[ir][instruction]") {
    SECTION("Arithmetic opcodes") {
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Add)) == "add");
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Sub)) == "sub");
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Mul)) == "mul");
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Div)) == "div");
    }

    SECTION("Memory opcodes") {
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Alloca)) == "alloca");
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Load)) == "load");
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Store)) == "store");
    }

    SECTION("Control flow opcodes") {
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Br)) == "br");
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Ret)) == "ret");
    }

    SECTION("PHI opcode") {
        REQUIRE(std::string(Instruction::getOpcodeName(Instruction::Opcode::Phi)) == "phi");
    }
}

TEST_CASE("Instruction - Type Queries", "[ir][instruction]") {
    auto intType = Type::getInt();
    auto c1 = Constant::getInt(10);
    auto c2 = Constant::getInt(20);

    SECTION("isBinaryOp") {
        BinaryInstruction add(Instruction::Opcode::Add, c1.get(), c2.get());
        REQUIRE(add.isBinaryOp());
        REQUIRE_FALSE(add.isTerminator());
        REQUIRE_FALSE(add.isMemoryOp());
    }

    SECTION("isTerminator - Branch") {
        // We'll test this with actual BasicBlock later
        // For now, just verify the opcode classification works
    }

    SECTION("isMemoryOp") {
        AllocaInstruction alloca(intType);
        REQUIRE(alloca.isMemoryOp());
        REQUIRE_FALSE(alloca.isBinaryOp());
        REQUIRE_FALSE(alloca.isTerminator());
    }
}

// ============================================================================
// Test: BinaryInstruction
// ============================================================================

TEST_CASE("BinaryInstruction - Add", "[ir][instruction][binary]") {
    auto c1 = Constant::getInt(10);
    auto c2 = Constant::getInt(20);

    BinaryInstruction add(Instruction::Opcode::Add, c1.get(), c2.get(), "%0");

    SECTION("Opcode is Add") {
        REQUIRE(add.getOpcode() == Instruction::Opcode::Add);
    }

    SECTION("Has 2 operands") {
        REQUIRE(add.getNumOperands() == 2);
    }

    SECTION("Operands are correct") {
        REQUIRE(add.getOperand(0) == c1.get());
        REQUIRE(add.getOperand(1) == c2.get());
        REQUIRE(add.getLHS() == c1.get());
        REQUIRE(add.getRHS() == c2.get());
    }

    SECTION("Name is set") {
        REQUIRE(add.getName() == "%0");
    }

    SECTION("Type matches operands") {
        REQUIRE(add.getType()->isInt());
    }
}

TEST_CASE("BinaryInstruction - toString", "[ir][instruction][binary]") {
    auto c1 = Constant::getInt(5);
    auto c2 = Constant::getInt(3);

    BinaryInstruction mul(Instruction::Opcode::Mul, c1.get(), c2.get(), "%result");

    std::string str = mul.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains result name") {
        REQUIRE(str.find("%result") != std::string::npos);
    }

    SECTION("Contains opcode") {
        REQUIRE(str.find("mul") != std::string::npos);
    }

    // Format should be: "%result = mul i64 %operand1, %operand2"
}

TEST_CASE("BinaryInstruction - Float Operations", "[ir][instruction][binary]") {
    auto f1 = Constant::getFloat(3.14);
    auto f2 = Constant::getFloat(2.71);

    BinaryInstruction fadd(Instruction::Opcode::FAdd, f1.get(), f2.get(), "%f");

    SECTION("Opcode is FAdd") {
        REQUIRE(fadd.getOpcode() == Instruction::Opcode::FAdd);
    }

    SECTION("Type is float") {
        REQUIRE(fadd.getType()->isFloat());
    }
}

// ============================================================================
// Test: CompareInstruction
// ============================================================================

TEST_CASE("CompareInstruction - Integer Compare", "[ir][instruction][compare]") {
    auto c1 = Constant::getInt(10);
    auto c2 = Constant::getInt(20);

    CompareInstruction cmp(Instruction::Opcode::ICmpLT, c1.get(), c2.get(), "%cond");

    SECTION("Opcode is ICmpLT") {
        REQUIRE(cmp.getOpcode() == Instruction::Opcode::ICmpLT);
    }

    SECTION("Return type is bool") {
        REQUIRE(cmp.getType()->isBool());
    }

    SECTION("Has 2 operands") {
        REQUIRE(cmp.getNumOperands() == 2);
    }

    SECTION("Operands are correct") {
        REQUIRE(cmp.getLHS() == c1.get());
        REQUIRE(cmp.getRHS() == c2.get());
    }
}

TEST_CASE("CompareInstruction - Equality", "[ir][instruction][compare]") {
    auto c1 = Constant::getInt(42);
    auto c2 = Constant::getInt(42);

    CompareInstruction eq(Instruction::Opcode::ICmpEQ, c1.get(), c2.get(), "%eq");

    REQUIRE(eq.getOpcode() == Instruction::Opcode::ICmpEQ);
    REQUIRE(eq.getType()->isBool());
}

TEST_CASE("CompareInstruction - toString", "[ir][instruction][compare]") {
    auto c1 = Constant::getInt(5);
    auto c2 = Constant::getInt(10);

    CompareInstruction cmp(Instruction::Opcode::ICmpGE, c1.get(), c2.get(), "%c");

    std::string str = cmp.toString();
    REQUIRE_FALSE(str.empty());
    // Should contain "icmp" and the predicate
    REQUIRE(str.find("icmp") != std::string::npos);
}

// ============================================================================
// Test: AllocaInstruction
// ============================================================================

TEST_CASE("AllocaInstruction - Basic", "[ir][instruction][memory]") {
    auto intType = Type::getInt();
    AllocaInstruction alloca(intType, "%x");

    SECTION("Opcode is Alloca") {
        REQUIRE(alloca.getOpcode() == Instruction::Opcode::Alloca);
    }

    SECTION("Allocated type is correct") {
        REQUIRE(alloca.getAllocatedType() == intType);
    }

    SECTION("Result type is pointer") {
        // The alloca returns a pointer to the allocated type
        REQUIRE(alloca.getType()->isPointer());
    }

    SECTION("Has no operands") {
        REQUIRE(alloca.getNumOperands() == 0);
    }

    SECTION("Name is set") {
        REQUIRE(alloca.getName() == "%x");
    }
}

TEST_CASE("AllocaInstruction - toString", "[ir][instruction][memory]") {
    auto intType = Type::getInt();
    AllocaInstruction alloca(intType, "%ptr");

    std::string str = alloca.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains alloca keyword") {
        REQUIRE(str.find("alloca") != std::string::npos);
    }

    SECTION("Contains result name") {
        REQUIRE(str.find("%ptr") != std::string::npos);
    }

    // Format: "%ptr = alloca i64"
}

// ============================================================================
// Test: LoadInstruction
// ============================================================================

TEST_CASE("LoadInstruction - Basic", "[ir][instruction][memory]") {
    auto intType = Type::getInt();
    AllocaInstruction alloca(intType, "%ptr");

    LoadInstruction load(&alloca, "%val");

    SECTION("Opcode is Load") {
        REQUIRE(load.getOpcode() == Instruction::Opcode::Load);
    }

    SECTION("Has 1 operand (pointer)") {
        REQUIRE(load.getNumOperands() == 1);
    }

    SECTION("Pointer operand is correct") {
        REQUIRE(load.getPointer() == &alloca);
        REQUIRE(load.getOperand(0) == &alloca);
    }

    SECTION("Name is set") {
        REQUIRE(load.getName() == "%val");
    }
}

TEST_CASE("LoadInstruction - toString", "[ir][instruction][memory]") {
    auto intType = Type::getInt();
    AllocaInstruction alloca(intType, "%x");
    LoadInstruction load(&alloca, "%0");

    std::string str = load.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains load keyword") {
        REQUIRE(str.find("load") != std::string::npos);
    }

    // Format: "%0 = load i64, ptr %x"
}

// ============================================================================
// Test: StoreInstruction
// ============================================================================

TEST_CASE("StoreInstruction - Basic", "[ir][instruction][memory]") {
    auto intType = Type::getInt();
    auto value = Constant::getInt(42);
    AllocaInstruction alloca(intType, "%ptr");

    StoreInstruction store(value.get(), &alloca);

    SECTION("Opcode is Store") {
        REQUIRE(store.getOpcode() == Instruction::Opcode::Store);
    }

    SECTION("Has 2 operands") {
        REQUIRE(store.getNumOperands() == 2);
    }

    SECTION("Value operand is correct") {
        REQUIRE(store.getValue() == value.get());
        REQUIRE(store.getOperand(0) == value.get());
    }

    SECTION("Pointer operand is correct") {
        REQUIRE(store.getPointer() == &alloca);
        REQUIRE(store.getOperand(1) == &alloca);
    }

    SECTION("Type is void (no return value)") {
        REQUIRE(store.getType()->isVoid());
    }
}

TEST_CASE("StoreInstruction - toString", "[ir][instruction][memory]") {
    auto intType = Type::getInt();
    auto value = Constant::getInt(100);
    AllocaInstruction alloca(intType, "%x");

    StoreInstruction store(value.get(), &alloca);

    std::string str = store.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains store keyword") {
        REQUIRE(str.find("store") != std::string::npos);
    }

    // Format: "store i64 100, ptr %x"
}

// ============================================================================
// Test: ReturnInstruction
// ============================================================================

TEST_CASE("ReturnInstruction - With Value", "[ir][instruction][control]") {
    auto value = Constant::getInt(42);
    ReturnInstruction ret(value.get());

    SECTION("Opcode is Ret") {
        REQUIRE(ret.getOpcode() == Instruction::Opcode::Ret);
    }

    SECTION("Has 1 operand") {
        REQUIRE(ret.getNumOperands() == 1);
    }

    SECTION("Return value is correct") {
        REQUIRE(ret.getReturnValue() == value.get());
        REQUIRE(ret.getOperand(0) == value.get());
    }

    SECTION("Is a terminator") {
        REQUIRE(ret.isTerminator());
    }
}

TEST_CASE("ReturnInstruction - Void Return", "[ir][instruction][control]") {
    ReturnInstruction ret(nullptr);

    SECTION("Has no operands") {
        REQUIRE(ret.getNumOperands() == 0);
    }

    SECTION("Return value is null") {
        REQUIRE(ret.getReturnValue() == nullptr);
    }

    SECTION("toString shows 'ret void'") {
        std::string str = ret.toString();
        REQUIRE(str.find("ret") != std::string::npos);
        REQUIRE(str.find("void") != std::string::npos);
    }
}

// ============================================================================
// Test: PhiInstruction - THE HEART OF SSA!
// ============================================================================

TEST_CASE("PhiInstruction - Basic Creation", "[ir][instruction][phi]") {
    auto intType = Type::getInt();
    PhiInstruction phi(intType, "%merged");

    SECTION("Opcode is Phi") {
        REQUIRE(phi.getOpcode() == Instruction::Opcode::Phi);
    }

    SECTION("Type is correct") {
        REQUIRE(phi.getType() == intType);
    }

    SECTION("Initially has no incoming values") {
        REQUIRE(phi.getNumOperands() == 0);
        REQUIRE(phi.getIncomingValues().empty());
    }

    SECTION("Name is set") {
        REQUIRE(phi.getName() == "%merged");
    }
}

TEST_CASE("PhiInstruction - Adding Incoming Values", "[ir][instruction][phi]") {
    auto intType = Type::getInt();
    PhiInstruction phi(intType, "%x");

    auto val1 = Constant::getInt(10);
    auto val2 = Constant::getInt(20);

    // Create mock basic blocks (we'll use null for now, replace with real blocks later)
    BasicBlock* block1 = nullptr;  // TODO: Create actual blocks when BasicBlock is complete
    BasicBlock* block2 = nullptr;

    // phi.addIncoming(val1.get(), block1);
    // phi.addIncoming(val2.get(), block2);

    // SECTION("Has 2 incoming values") {
    //     REQUIRE(phi.getNumOperands() == 2);
    //     REQUIRE(phi.getIncomingValues().size() == 2);
    // }

    // SECTION("Can query by block") {
    //     REQUIRE(phi.getIncomingValueForBlock(block1) == val1.get());
    //     REQUIRE(phi.getIncomingValueForBlock(block2) == val2.get());
    // }

    // Note: This test needs BasicBlock to be fully implemented
    // We'll complete it in Phase 3
}

TEST_CASE("PhiInstruction - toString", "[ir][instruction][phi]") {
    auto intType = Type::getInt();
    PhiInstruction phi(intType, "%result");

    // TODO: Add incoming values when BasicBlock is ready

    std::string str = phi.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains phi keyword") {
        REQUIRE(str.find("phi") != std::string::npos);
    }

    // Format: "%result = phi i64 [ %val1, %block1 ], [ %val2, %block2 ]"
}
