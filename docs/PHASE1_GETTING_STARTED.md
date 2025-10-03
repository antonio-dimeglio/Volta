# Phase 1: IR Foundation - Getting Started

Welcome to Phase 1 of building Volta's state-of-the-art IR! This guide will help you implement your first IR functions.

## What You'll Build in Phase 1

- **Value system**: SSA values, constants, parameters, globals
- **Instruction system**: All IR opcodes and instruction types
- **BasicBlock system**: Control flow graph nodes

## Architecture Overview

```
IRModule                 (top level - contains everything)
  ├─ GlobalVariable[]    (module-level variables)
  └─ Function[]          (functions in the module)
       └─ BasicBlock[]   (CFG nodes)
            └─ Instruction[]  (SSA instructions)
                 └─ Value (operands)
```

## Files Created For You

### Headers (include/IR/)
- **Value.hpp** - Base Value class, Constant, Parameter, GlobalVariable
- **Instruction.hpp** - All instruction types (Add, Load, Store, Phi, etc.)
- **BasicBlock.hpp** - Control flow graph nodes
- **Function.hpp** - Function container
- **IRModule.hpp** - Top-level IR module
- **IRBuilder.hpp** - High-level API (Phase 2)
- **IRGenerator.hpp** - AST→IR converter (Phase 4)
- **IRPrinter.hpp** - Debug printer
- **SSABuilder.hpp** - SSA construction (Phase 3)

### Implementation (src/IR/)
- All corresponding .cpp files with **empty function skeletons**

### Tests (tests/)
- **test_ir_values.cpp** - Tests for Value, Constant, Parameter, GlobalVariable
- **test_ir_instructions.cpp** - Tests for all instruction types
- **test_ir_basicblocks.cpp** - Tests for BasicBlock and CFG

## Your First Task: Implement Value.cpp

### Step 1: Read the Tests

Open [tests/test_ir_values.cpp](../tests/test_ir_values.cpp) and read through the test cases. They tell you exactly what each function should do.

Example test:
```cpp
TEST_CASE("Constant - Integer", "[ir][constant]") {
    auto intConst = Constant::getInt(42);

    REQUIRE(intConst != nullptr);
    REQUIRE(intConst->getKind() == Value::Kind::Constant);
    REQUIRE(intConst->getType()->isInt());

    SECTION("Value is stored correctly") {
        auto value = std::get<int64_t>(intConst->getValue());
        REQUIRE(value == 42);
    }
}
```

This tells you: `Constant::getInt(42)` should create a Constant with value 42, type int, and kind Constant.

### Step 2: Implement the Function

Open [src/IR/Value.cpp](../src/IR/Value.cpp) and find the `Constant::getInt` function:

```cpp
std::shared_ptr<Constant> Constant::getInt(int64_t value) {
    // TODO: Create and return a Constant representing an integer.
    // - Type should be semantic::Type::getInt()
    // - Name can be empty (constants don't need SSA names)
    // - Value should be stored in the ConstantValue variant
    return nullptr;
}
```

**Your Implementation:**
```cpp
std::shared_ptr<Constant> Constant::getInt(int64_t value) {
    auto intType = semantic::Type::getInt();
    return std::make_shared<Constant>(intType, value);
}
```

That's it! The TODO comment tells you what to do, you just implement it.

### Step 3: Run the Tests

```bash
# From Volta root directory
make test_ir_values
./bin/test_ir_values
```

If you implemented it correctly, you'll see:
```
✓ Constant - Integer
✓ Value is stored correctly
```

## Implementation Order (Suggested)

### Week 1: Values

1. **Value::toString()** - Simple getter
2. **Value::addUse() / removeUse()** - Vector operations
3. **Constant::getInt() / getFloat() / getBool() / getString() / getNone()** - Factory methods
4. **Constant::toString()** - Format constants for printing
5. **Parameter::toString()** - Format parameters
6. **GlobalVariable::toString()** - Format globals

Run tests after each function!

### Week 2: Instructions

1. **Instruction::getOpcodeName()** - Map opcodes to strings
2. **Instruction::isTerminator() / isBinaryOp() / isCompare()** - Type queries
3. **BinaryInstruction::getOperand() / setOperand()** - Operand access
4. **BinaryInstruction::toString()** - Format binary ops
5. **CompareInstruction** - Similar to BinaryInstruction
6. **AllocaInstruction::toString()**
7. **LoadInstruction::getOperand() / toString()**
8. **StoreInstruction::getOperand() / toString()**
9. **BranchInstruction::toString()**
10. **CondBranchInstruction::toString()**
11. **ReturnInstruction::toString()**
12. **CallInstruction::getOperand() / toString()**
13. **PhiInstruction** - The most complex! Do this last.

### Week 3: BasicBlocks

1. **BasicBlock::addInstruction()** - Vector push_back
2. **BasicBlock::getFirstInstruction() / getTerminator()**
3. **BasicBlock::hasTerminator()** - Check last instruction
4. **BasicBlock::addPredecessor() / addSuccessor()** - CFG management
5. **BasicBlock::removePredecessor() / removeSuccessor()**
6. **BasicBlock::getSinglePredecessor() / getSingleSuccessor()**
7. **BasicBlock::dominates()** - Dominator tree walk
8. **BasicBlock::addToDominanceFrontier()**
9. **BasicBlock::toString()** - Format entire block

## Tips for Success

### 1. Read Tests First
The tests tell you exactly what to implement. They're your specification!

### 2. Start Simple
Begin with the easiest functions (getters, setters). Build confidence.

### 3. Use the Headers
The .hpp files have detailed comments explaining what each function does.

### 4. Run Tests Frequently
After implementing each function, run the tests. Don't wait until the end!

### 5. Check Your Work
Use the toString() methods to print IR and verify it looks correct.

### 6. Ask Questions
When stuck, check:
- Test cases (what is expected?)
- Header comments (what should this do?)
- LLVM documentation (how does LLVM do it?)

## Understanding SSA Form

Before implementing PhiInstruction, understand SSA:

### Example: Variable Assignment

**Volta code:**
```volta
x: mut int = 10
if condition {
    x = 20
} else {
    x = 30
}
print(x)
```

**IR (SSA form):**
```
entry:
    %x.0 = alloca i64
    store i64 10, ptr %x.0
    %cond = ...
    br i1 %cond, label %then, label %else

then:
    store i64 20, ptr %x.0
    br label %merge

else:
    store i64 30, ptr %x.0
    br label %merge

merge:
    %val.then = load i64, ptr %x.0  ; from then
    %val.else = load i64, ptr %x.0  ; from else
    %x.merged = phi i64 [ %val.then, %then ], [ %val.else, %else ]
    call @print(i64 %x.merged)
```

The **phi node** selects the correct value based on which predecessor executed!

## PHI Node Implementation

PhiInstruction is the most complex. Here's what you need:

```cpp
void PhiInstruction::addIncoming(Value* value, BasicBlock* block) {
    // 1. Add {value, block} to incomingValues_ vector
    // 2. Register this phi as a user of the value: value->addUse(this)
}

Value* PhiInstruction::getIncomingValueForBlock(BasicBlock* block) const {
    // 1. Loop through incomingValues_
    // 2. Find the entry where entry.block == block
    // 3. Return entry.value (or nullptr if not found)
}

std::string PhiInstruction::toString() const {
    // Format: "%name = phi type [ %val1, %block1 ], [ %val2, %block2 ]"
    //
    // Example: "%x.merged = phi i64 [ 20, %then ], [ 30, %else ]"
}
```

## Next Steps After Phase 1

Once all Phase 1 tests pass, you'll move to:

- **Phase 2: IRBuilder** - High-level API for creating instructions
- **Phase 3: SSABuilder** - Automatic PHI node placement
- **Phase 4: IRGenerator** - Convert AST to IR
- **Phase 5: Optimizations** - Make it fast!

## Common Pitfalls

### 1. Forgetting to Update Use-Def Chains
When setting operands, remember to:
```cpp
// Remove old use
if (oldValue) oldValue->removeUse(this);
// Add new use
if (newValue) newValue->addUse(this);
```

### 2. Not Checking for nullptr
Always validate pointers before dereferencing:
```cpp
if (!insertBlock_) {
    // error: no insert point set!
}
```

### 3. Memory Management
We use `std::unique_ptr` for ownership and raw pointers for references.
- BasicBlock **owns** Instructions (unique_ptr)
- Instructions **reference** Values (raw pointer)

### 4. SSA Value Names
Generate unique names with a counter:
```cpp
std::string getUniqueName() {
    return "%" + std::to_string(valueCounter_++);
}
```

## Resources

- **LLVM Language Reference**: https://llvm.org/docs/LangRef.html
- **SSA Book**: "SSA-based Compiler Design" (free PDF)
- **Braun et al. Paper**: Simple and Efficient Construction of SSA Form

## Your Progress Checklist

Phase 1 is complete when:

- [ ] All tests in test_ir_values.cpp pass
- [ ] All tests in test_ir_instructions.cpp pass
- [ ] All tests in test_ir_basicblocks.cpp pass
- [ ] You can manually create a simple function in IR
- [ ] You understand how PHI nodes work

## Example: Manual IR Construction

Try this after completing Phase 1:

```cpp
// Create a simple add function: fn add(a: int, b: int) -> int { return a + b }

auto intType = Type::getInt();

// Create parameters
auto param_a = std::make_unique<Parameter>(intType, "%a", 0);
auto param_b = std::make_unique<Parameter>(intType, "%b", 1);

std::vector<std::unique_ptr<Parameter>> params;
params.push_back(std::move(param_a));
params.push_back(std::move(param_b));

// Create function
Function fn("add", std::move(params), intType);

// Create entry block
BasicBlock* entry = fn.createBasicBlock("entry");

// Create add instruction
auto add = std::make_unique<BinaryInstruction>(
    Instruction::Opcode::Add,
    fn.getParameter(0),
    fn.getParameter(1),
    "%sum"
);

// Create return
auto ret = std::make_unique<ReturnInstruction>(add.get());

// Add to block
entry->addInstruction(std::move(add));
entry->addInstruction(std::move(ret));

// Print it!
std::cout << fn.toString() << std::endl;
```

Expected output:
```
define i64 @add(i64 %a, i64 %b) {
entry:
    %sum = add i64 %a, %b
    ret i64 %sum
}
```

If you can do this, Phase 1 is done! 🎉

---

**Ready to start? Open [src/IR/Value.cpp](../src/IR/Value.cpp) and begin!**
