#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "semantic/Type.hpp"

using namespace volta::ir;
using namespace volta::semantic;

// ============================================================================
// Test: BasicBlock Creation
// ============================================================================

TEST_CASE("BasicBlock - Basic Creation", "[ir][basicblock]") {
    BasicBlock bb("entry");

    SECTION("Name is set") {
        REQUIRE(bb.getName() == "entry");
    }

    SECTION("Initially empty") {
        REQUIRE(bb.empty());
        REQUIRE(bb.size() == 0);
    }

    SECTION("No predecessors initially") {
        REQUIRE(bb.getPredecessors().empty());
    }

    SECTION("No successors initially") {
        REQUIRE(bb.getSuccessors().empty());
    }

    SECTION("No terminator initially") {
        REQUIRE_FALSE(bb.hasTerminator());
        REQUIRE(bb.getTerminator() == nullptr);
    }
}

// ============================================================================
// Test: Adding Instructions
// ============================================================================

TEST_CASE("BasicBlock - Add Instructions", "[ir][basicblock]") {
    BasicBlock bb("test");

    auto c1 = Constant::getInt(10);
    auto c2 = Constant::getInt(20);

    SECTION("Add single instruction") {
        auto add = std::make_unique<BinaryInstruction>(
            Instruction::Opcode::Add, c1.get(), c2.get(), "%0"
        );

        bb.addInstruction(std::move(add));

        REQUIRE_FALSE(bb.empty());
        REQUIRE(bb.size() == 1);
    }

    SECTION("Add multiple instructions") {
        auto add = std::make_unique<BinaryInstruction>(
            Instruction::Opcode::Add, c1.get(), c2.get(), "%0"
        );
        auto mul = std::make_unique<BinaryInstruction>(
            Instruction::Opcode::Mul, c1.get(), c2.get(), "%1"
        );

        bb.addInstruction(std::move(add));
        bb.addInstruction(std::move(mul));

        REQUIRE(bb.size() == 2);
    }

    SECTION("Instruction parent is set") {
        auto add = std::make_unique<BinaryInstruction>(
            Instruction::Opcode::Add, c1.get(), c2.get(), "%0"
        );
        Instruction* addPtr = add.get();

        bb.addInstruction(std::move(add));

        REQUIRE(addPtr->getParent() == &bb);
    }
}

TEST_CASE("BasicBlock - Get First/Last Instruction", "[ir][basicblock]") {
    BasicBlock bb("test");

    auto c1 = Constant::getInt(10);
    auto c2 = Constant::getInt(20);

    auto add = std::make_unique<BinaryInstruction>(
        Instruction::Opcode::Add, c1.get(), c2.get(), "%0"
    );
    auto mul = std::make_unique<BinaryInstruction>(
        Instruction::Opcode::Mul, c1.get(), c2.get(), "%1"
    );

    Instruction* addPtr = add.get();
    Instruction* mulPtr = mul.get();

    bb.addInstruction(std::move(add));
    bb.addInstruction(std::move(mul));

    SECTION("First instruction") {
        REQUIRE(bb.getFirstInstruction() == addPtr);
    }

    // Note: getTerminator() should return last instruction ONLY if it's a terminator
    // We'll test this in the next section
}

// ============================================================================
// Test: Terminators
// ============================================================================

TEST_CASE("BasicBlock - Terminator", "[ir][basicblock]") {
    BasicBlock bb("test");

    SECTION("Block without terminator") {
        auto c1 = Constant::getInt(10);
        auto c2 = Constant::getInt(20);
        auto add = std::make_unique<BinaryInstruction>(
            Instruction::Opcode::Add, c1.get(), c2.get(), "%0"
        );

        bb.addInstruction(std::move(add));

        REQUIRE_FALSE(bb.hasTerminator());
        REQUIRE(bb.getTerminator() == nullptr);
    }

    SECTION("Block with return terminator") {
        auto value = Constant::getInt(42);
        auto ret = std::make_unique<ReturnInstruction>(value.get());
        Instruction* retPtr = ret.get();

        bb.addInstruction(std::move(ret));

        REQUIRE(bb.hasTerminator());
        REQUIRE(bb.getTerminator() == retPtr);
    }

    SECTION("Block with branch terminator") {
        BasicBlock target("target");
        auto br = std::make_unique<BranchInstruction>(&target);
        Instruction* brPtr = br.get();

        bb.addInstruction(std::move(br));

        REQUIRE(bb.hasTerminator());
        REQUIRE(bb.getTerminator() == brPtr);
    }
}

// ============================================================================
// Test: CFG - Predecessors and Successors
// ============================================================================

TEST_CASE("BasicBlock - Single Predecessor/Successor", "[ir][basicblock][cfg]") {
    BasicBlock entry("entry");
    BasicBlock exit("exit");

    SECTION("Add predecessor") {
        exit.addPredecessor(&entry);

        REQUIRE(exit.getPredecessors().size() == 1);
        REQUIRE(exit.getPredecessors()[0] == &entry);
        REQUIRE(exit.hasSinglePredecessor());
        REQUIRE(exit.getSinglePredecessor() == &entry);
    }

    SECTION("Add successor") {
        entry.addSuccessor(&exit);

        REQUIRE(entry.getSuccessors().size() == 1);
        REQUIRE(entry.getSuccessors()[0] == &exit);
        REQUIRE(entry.hasSingleSuccessor());
        REQUIRE(entry.getSingleSuccessor() == &exit);
    }

    SECTION("Connect both ways") {
        entry.addSuccessor(&exit);
        exit.addPredecessor(&entry);

        REQUIRE(entry.getSuccessors()[0] == &exit);
        REQUIRE(exit.getPredecessors()[0] == &entry);
    }
}

TEST_CASE("BasicBlock - Multiple Predecessors/Successors", "[ir][basicblock][cfg]") {
    BasicBlock entry("entry");
    BasicBlock then("then");
    BasicBlock else_("else");
    BasicBlock merge("merge");

    SECTION("If-else diamond pattern") {
        // entry -> then, else
        entry.addSuccessor(&then);
        entry.addSuccessor(&else_);

        // then -> merge
        then.addSuccessor(&merge);
        then.addPredecessor(&entry);

        // else -> merge
        else_.addSuccessor(&merge);
        else_.addPredecessor(&entry);

        // merge has two predecessors
        merge.addPredecessor(&then);
        merge.addPredecessor(&else_);

        REQUIRE(entry.getSuccessors().size() == 2);
        REQUIRE(merge.getPredecessors().size() == 2);

        REQUIRE_FALSE(entry.hasSingleSuccessor());
        REQUIRE_FALSE(merge.hasSinglePredecessor());

        REQUIRE(merge.getSinglePredecessor() == nullptr);
    }
}

TEST_CASE("BasicBlock - Remove Predecessor/Successor", "[ir][basicblock][cfg]") {
    BasicBlock entry("entry");
    BasicBlock then("then");
    BasicBlock else_("else");

    entry.addSuccessor(&then);
    entry.addSuccessor(&else_);

    SECTION("Remove successor") {
        entry.removeSuccessor(&then);

        REQUIRE(entry.getSuccessors().size() == 1);
        REQUIRE(entry.getSuccessors()[0] == &else_);
    }

    SECTION("Remove all successors") {
        entry.removeSuccessor(&then);
        entry.removeSuccessor(&else_);

        REQUIRE(entry.getSuccessors().empty());
    }
}

// ============================================================================
// Test: CFG - Real Control Flow Example
// ============================================================================

TEST_CASE("BasicBlock - If Statement CFG", "[ir][basicblock][cfg]") {
    // Volta code:
    //   if x > 10 {
    //       y = x * 2
    //   } else {
    //       y = x + 5
    //   }
    //   return y

    BasicBlock entry("entry");
    BasicBlock thenBlock("then");
    BasicBlock elseBlock("else");
    BasicBlock merge("merge");

    // Build CFG
    entry.addSuccessor(&thenBlock);
    entry.addSuccessor(&elseBlock);

    thenBlock.addPredecessor(&entry);
    thenBlock.addSuccessor(&merge);

    elseBlock.addPredecessor(&entry);
    elseBlock.addSuccessor(&merge);

    merge.addPredecessor(&thenBlock);
    merge.addPredecessor(&elseBlock);

    // Verify structure
    REQUIRE(entry.getSuccessors().size() == 2);
    REQUIRE(thenBlock.getPredecessors().size() == 1);
    REQUIRE(elseBlock.getPredecessors().size() == 1);
    REQUIRE(merge.getPredecessors().size() == 2);

    // Add actual instructions
    auto c10 = Constant::getInt(10);
    auto cond = std::make_unique<CompareInstruction>(
        Instruction::Opcode::ICmpGT, c10.get(), c10.get(), "%cond"
    );
    auto condBr = std::make_unique<CondBranchInstruction>(
        cond.get(), &thenBlock, &elseBlock
    );

    entry.addInstruction(std::move(cond));
    entry.addInstruction(std::move(condBr));

    REQUIRE(entry.hasTerminator());
    REQUIRE(entry.size() == 2);
}

// ============================================================================
// Test: Dominance
// ============================================================================

TEST_CASE("BasicBlock - Dominance", "[ir][basicblock][dominance]") {
    // Simple linear CFG: entry -> middle -> exit
    BasicBlock entry("entry");
    BasicBlock middle("middle");
    BasicBlock exit("exit");

    entry.addSuccessor(&middle);
    middle.addPredecessor(&entry);
    middle.addSuccessor(&exit);
    exit.addPredecessor(&middle);

    SECTION("Set immediate dominator") {
        middle.setImmediateDominator(&entry);
        exit.setImmediateDominator(&middle);

        REQUIRE(middle.getImmediateDominator() == &entry);
        REQUIRE(exit.getImmediateDominator() == &middle);
    }

    SECTION("Dominance check") {
        middle.setImmediateDominator(&entry);
        exit.setImmediateDominator(&middle);

        // entry dominates everything
        REQUIRE(entry.dominates(&middle));
        REQUIRE(entry.dominates(&exit));

        // middle dominates exit but not entry
        REQUIRE(middle.dominates(&exit));
        REQUIRE_FALSE(middle.dominates(&entry));

        // exit dominates nothing else
        REQUIRE_FALSE(exit.dominates(&entry));
        REQUIRE_FALSE(exit.dominates(&middle));
    }
}

TEST_CASE("BasicBlock - Dominance Frontier", "[ir][basicblock][dominance]") {
    // If-else diamond: entry -> [then, else] -> merge
    BasicBlock entry("entry");
    BasicBlock then("then");
    BasicBlock else_("else");
    BasicBlock merge("merge");

    // Build CFG
    entry.addSuccessor(&then);
    entry.addSuccessor(&else_);
    then.addSuccessor(&merge);
    else_.addSuccessor(&merge);

    // Set dominators (this would normally be computed by analysis pass)
    then.setImmediateDominator(&entry);
    else_.setImmediateDominator(&entry);
    merge.setImmediateDominator(&entry);

    SECTION("Dominance frontier of 'then' is 'merge'") {
        // 'then' dominates no other block's predecessors except 'merge'
        then.addToDominanceFrontier(&merge);

        auto& df = then.getDominanceFrontier();
        REQUIRE(df.size() == 1);
        REQUIRE(df[0] == &merge);
    }

    SECTION("Dominance frontier of 'else' is 'merge'") {
        else_.addToDominanceFrontier(&merge);

        auto& df = else_.getDominanceFrontier();
        REQUIRE(df.size() == 1);
        REQUIRE(df[0] == &merge);
    }

    // This tells us: "PHI nodes for variables defined in then/else need to be placed in merge"
}

// ============================================================================
// Test: toString
// ============================================================================

TEST_CASE("BasicBlock - toString", "[ir][basicblock]") {
    BasicBlock bb("entry");

    auto c1 = Constant::getInt(10);
    auto c2 = Constant::getInt(20);

    auto add = std::make_unique<BinaryInstruction>(
        Instruction::Opcode::Add, c1.get(), c2.get(), "%0"
    );
    auto ret = std::make_unique<ReturnInstruction>(c1.get());

    bb.addInstruction(std::move(add));
    bb.addInstruction(std::move(ret));

    std::string str = bb.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains block name") {
        REQUIRE(str.find("entry") != std::string::npos);
    }

    // Format should be:
    // entry:
    //     %0 = add i64 ...
    //     ret i64 ...
}
