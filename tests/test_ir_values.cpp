#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "IR/Value.hpp"
#include "semantic/Type.hpp"

using namespace volta::ir;
using namespace volta::semantic;

// ============================================================================
// Test: Value Base Class
// ============================================================================

TEST_CASE("Value - Basic Properties", "[ir][value]") {
    auto intType = Type::getInt();
    Value value(Value::Kind::Instruction, intType, "%0");

    SECTION("Kind is set correctly") {
        REQUIRE(value.getKind() == Value::Kind::Instruction);
    }

    SECTION("Type is set correctly") {
        REQUIRE(value.getType() == intType);
    }

    SECTION("Name is set correctly") {
        REQUIRE(value.getName() == "%0");
    }

    SECTION("Initially has no uses") {
        REQUIRE(value.getUses().empty());
        REQUIRE_FALSE(value.hasUses());
    }
}

TEST_CASE("Value - Use Tracking", "[ir][value]") {
    auto intType = Type::getInt();
    Value value(Value::Kind::Instruction, intType, "%0");

    // Mock instruction for testing (we'll use Value itself as placeholder)
    auto user1 = std::make_unique<Value>(Value::Kind::Instruction, intType, "%1");
    auto user2 = std::make_unique<Value>(Value::Kind::Instruction, intType, "%2");

    // Note: In real code, only Instructions can be users
    // For this test, we're just checking the use-tracking mechanism
    // TODO: Replace with actual Instruction when available
}

TEST_CASE("Value - toString", "[ir][value]") {
    auto intType = Type::getInt();

    SECTION("SSA value name") {
        Value value(Value::Kind::Instruction, intType, "%result");
        REQUIRE(value.toString() == "%result");
    }

    SECTION("Numbered SSA value") {
        Value value(Value::Kind::Instruction, intType, "%0");
        REQUIRE(value.toString() == "%0");
    }
}

// ============================================================================
// Test: Constants
// ============================================================================

TEST_CASE("Constant - Integer", "[ir][constant]") {
    auto intConst = Constant::getInt(42);

    REQUIRE(intConst != nullptr);
    REQUIRE(intConst->getKind() == Value::Kind::Constant);
    REQUIRE(intConst->getType()->isInt());

    SECTION("Value is stored correctly") {
        auto value = std::get<int64_t>(intConst->getValue());
        REQUIRE(value == 42);
    }

    SECTION("toString formats correctly") {
        // Expected format: "i64 42" or similar
        std::string str = intConst->toString();
        REQUIRE_FALSE(str.empty());
        // Should contain the value
        REQUIRE(str.find("42") != std::string::npos);
    }
}

TEST_CASE("Constant - Float", "[ir][constant]") {
    auto floatConst = Constant::getFloat(3.14);

    REQUIRE(floatConst != nullptr);
    REQUIRE(floatConst->getKind() == Value::Kind::Constant);
    REQUIRE(floatConst->getType()->isFloat());

    SECTION("Value is stored correctly") {
        auto value = std::get<double>(floatConst->getValue());
        REQUIRE(value == 3.14);
    }
}

TEST_CASE("Constant - Boolean", "[ir][constant]") {
    auto trueConst = Constant::getBool(true);
    auto falseConst = Constant::getBool(false);

    SECTION("True constant") {
        REQUIRE(trueConst != nullptr);
        REQUIRE(trueConst->getType()->isBool());
        auto value = std::get<bool>(trueConst->getValue());
        REQUIRE(value == true);
    }

    SECTION("False constant") {
        REQUIRE(falseConst != nullptr);
        auto value = std::get<bool>(falseConst->getValue());
        REQUIRE(value == false);
    }
}

TEST_CASE("Constant - String", "[ir][constant]") {
    auto strConst = Constant::getString("hello");

    REQUIRE(strConst != nullptr);
    REQUIRE(strConst->getType()->isString());

    SECTION("Value is stored correctly") {
        auto value = std::get<std::string>(strConst->getValue());
        REQUIRE(value == "hello");
    }
}

TEST_CASE("Constant - None", "[ir][constant]") {
    auto noneConst = Constant::getNone();

    REQUIRE(noneConst != nullptr);
    // Type should be Option<void> or similar
    // For now, just check it's created successfully

    SECTION("Value is monostate") {
        REQUIRE(std::holds_alternative<std::monostate>(noneConst->getValue()));
    }
}

// ============================================================================
// Test: Parameters
// ============================================================================

TEST_CASE("Parameter - Creation", "[ir][parameter]") {
    auto intType = Type::getInt();
    Parameter param(intType, "%arg0", 0);

    SECTION("Kind is Parameter") {
        REQUIRE(param.getKind() == Value::Kind::Parameter);
    }

    SECTION("Type is correct") {
        REQUIRE(param.getType() == intType);
    }

    SECTION("Name is correct") {
        REQUIRE(param.getName() == "%arg0");
    }

    SECTION("Index is correct") {
        REQUIRE(param.getIndex() == 0);
    }
}

TEST_CASE("Parameter - Multiple Parameters", "[ir][parameter]") {
    auto intType = Type::getInt();
    auto floatType = Type::getFloat();

    Parameter param0(intType, "%a", 0);
    Parameter param1(floatType, "%b", 1);
    Parameter param2(intType, "%c", 2);

    REQUIRE(param0.getIndex() == 0);
    REQUIRE(param1.getIndex() == 1);
    REQUIRE(param2.getIndex() == 2);

    REQUIRE(param0.getName() == "%a");
    REQUIRE(param1.getName() == "%b");
    REQUIRE(param2.getName() == "%c");
}

TEST_CASE("Parameter - toString", "[ir][parameter]") {
    auto intType = Type::getInt();
    Parameter param(intType, "%x", 0);

    std::string str = param.toString();
    REQUIRE_FALSE(str.empty());
    // Should contain parameter name
    REQUIRE(str.find("%x") != std::string::npos);
}

// ============================================================================
// Test: GlobalVariable
// ============================================================================

TEST_CASE("GlobalVariable - Without Initializer", "[ir][global]") {
    auto intType = Type::getInt();
    GlobalVariable global(intType, "counter", nullptr, true);

    SECTION("Kind is GlobalVariable") {
        REQUIRE(global.getKind() == Value::Kind::GlobalVariable);
    }

    SECTION("Name has @ prefix") {
        REQUIRE(global.getName() == "@counter");
    }

    SECTION("Type is correct") {
        REQUIRE(global.getType() == intType);
    }

    SECTION("Is mutable") {
        REQUIRE(global.isMutable() == true);
    }

    SECTION("No initializer") {
        REQUIRE(global.getInitializer() == nullptr);
    }
}

TEST_CASE("GlobalVariable - With Initializer", "[ir][global]") {
    auto intType = Type::getInt();
    auto initializer = Constant::getInt(100);
    GlobalVariable global(intType, "value", initializer, false);

    SECTION("Has initializer") {
        REQUIRE(global.getInitializer() != nullptr);
        auto initValue = std::get<int64_t>(global.getInitializer()->getValue());
        REQUIRE(initValue == 100);
    }

    SECTION("Is immutable") {
        REQUIRE(global.isMutable() == false);
    }
}

TEST_CASE("GlobalVariable - toString", "[ir][global]") {
    auto intType = Type::getInt();
    auto initializer = Constant::getInt(42);
    GlobalVariable global(intType, "x", initializer, false);

    std::string str = global.toString();
    REQUIRE_FALSE(str.empty());

    SECTION("Contains global name") {
        REQUIRE(str.find("@x") != std::string::npos);
    }

    SECTION("Contains 'global' keyword") {
        REQUIRE(str.find("global") != std::string::npos);
    }
}
