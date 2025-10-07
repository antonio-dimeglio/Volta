#include <gtest/gtest.h>
#include "IR/Verifier.hpp"
#include "IR/Module.hpp"
#include "IR/IRBuilder.hpp"

using namespace volta::ir;

class VerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<Module>("test");
        verifier = std::make_unique<Verifier>();
    }
    std::unique_ptr<Module> module;
    std::unique_ptr<Verifier> verifier;
};

TEST_F(VerifierTest, EmptyModule_IsValid) {
    EXPECT_TRUE(verifier->verify(*module));
    EXPECT_FALSE(verifier->hasErrors());
}

TEST_F(VerifierTest, ValidFunction_Passes) {
    IRBuilder builder(*module);
    auto* func = builder.createFunction("test", builder.getVoidType(), {});
    builder.createRet();

    EXPECT_TRUE(verifier->verifyFunction(func));
}

TEST_F(VerifierTest, MissingTerminator_Fails) {
    IRBuilder builder(*module);
    auto* func = builder.createFunction("test", builder.getIntType(), {}, false);
    auto* block = module->createBasicBlock("entry", func);
    // No terminator!

    EXPECT_FALSE(verifier->verifyFunction(func));
    EXPECT_TRUE(verifier->hasErrors());
}

TEST_F(VerifierTest, WrongReturnType_Fails) {
    // Function returns int but returns void
    IRBuilder builder(*module);
    auto* func = builder.createFunction("test", builder.getIntType(), {});
    builder.createRet();  // Should return int, not void!

    EXPECT_FALSE(verifier->verifyFunction(func));
}

TEST_F(VerifierTest, CFGConsistency_ValidBranch) {
    IRBuilder builder(*module);
    auto* func = builder.createFunction("test", builder.getVoidType(), {});
    auto* target = builder.createBasicBlock("target");

    builder.createBr(target);
    builder.setInsertionPoint(target);
    builder.createRet();

    EXPECT_TRUE(verifier->verifyFunction(func));
}
