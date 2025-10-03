# IR Value System - Learning Guide

## What You've Been Given

### Files Created
1. **[include/IR/Value.hpp](include/IR/Value.hpp)** - Complete API with documentation
2. **[src/IR/Value.cpp](src/IR/Value.cpp)** - Stub implementation with TODOs
3. **[tests/test_ir_value.cpp](tests/test_ir_value.cpp)** - Comprehensive test suite
4. **[include/IR/Instruction.hpp](include/IR/Instruction.hpp)** - Minimal forward declaration

## Your Learning Task

Implement all the TODOs in `src/IR/Value.cpp` to make the tests pass.

### How to Work

```bash
# Build and run tests
make test

# Or just build the test executable
make bin/test_volta

# Run only IR value tests (after building)
./bin/test_volta --gtest_filter=ValueTest.*
```

## Understanding the Value System

### Core Concepts

#### 1. **SSA (Static Single Assignment)**
Every Value represents a single definition. Once created, it cannot be redefined.

Example:
```
%1 = add i64 %a, %b    // %1 is defined once
%2 = mul i64 %1, 2     // %1 is used here
```

#### 2. **Use-Def Chains**
- Each Value tracks all its **uses** (where it's used)
- Each Use tracks the **value** being used and the **user** (instruction)
- This enables efficient optimization passes

Example flow:
```
Value(%1) -> Use1(in ADD instruction) -> Use2(in MUL instruction)
```

#### 3. **Type Safety**
Every value has a type from `semantic::Type`. This ensures type correctness during IR construction.

#### 4. **Value Hierarchy**

```
Value (abstract base)
├── Constant (compile-time known values)
│   ├── ConstantInt (42, -100, 0)
│   ├── ConstantFloat (3.14, -2.5)
│   ├── ConstantBool (true, false)
│   ├── ConstantString ("hello")
│   ├── ConstantNone (None for Option types)
│   └── UndefValue (uninitialized)
├── Argument (function parameters: %arg0, %arg1)
├── Instruction (computed values: %1, %2, %3)
└── GlobalVariable (global variables: @counter)
```

### Implementation Hints

#### Value Base Class

1. **Constructor**: Just initialize the fields. The ID is auto-incremented.

2. **replaceAllUsesWith(Value* newValue)**:
   ```cpp
   // Pseudocode:
   // - Make a copy of uses_ (it will be modified during iteration)
   // - For each use in the copy:
   //   - Call use->setValue(newValue)
   ```

3. **addUse/removeUse**:
   ```cpp
   // addUse: push to uses_ vector
   // removeUse: use std::remove + erase idiom
   ```

4. **Type checking methods** (isConstant, isInstruction, etc.):
   ```cpp
   // Check if kind_ is in the appropriate range
   // Example: isConstant() checks if kind_ >= ConstantInt && kind_ <= UndefValue
   ```

#### Use Class

1. **Constructor**:
   ```cpp
   // Store value_ and user_
   // Register this use with the value: value_->addUse(this)
   ```

2. **Destructor**:
   ```cpp
   // Unregister this use from the value: value_->removeUse(this)
   ```

3. **setValue(Value* newValue)**:
   ```cpp
   // Remove this use from old value_
   // Update value_ to newValue
   // Add this use to new value_
   ```

#### Constants

1. **Factory methods** (ConstantInt::get, etc.):
   ```cpp
   // For now: just create and return new instance
   // Later optimization: implement constant interning
   return new ConstantInt(value, type);
   ```

2. **toString() methods**:
   - ConstantInt: `"i64 42"` or just `"42"`
   - ConstantFloat: `"f64 3.14"` or `"3.14"`
   - ConstantBool: `"true"` or `"false"`
   - ConstantString: `"\"hello\""`
   - ConstantNone: `"None"`
   - UndefValue: `"undef"`

   Choose a format you like - the tests just check it's not empty!

#### Argument

1. **toString()**:
   ```cpp
   // If has name: return "%" + name
   // Else: return "%arg" + std::to_string(argNo_)
   ```

#### GlobalVariable

1. **toString()**:
   ```cpp
   // Global variables use @ prefix
   return "@" + name_;
   ```

### Testing Strategy

Run tests incrementally:

```bash
# Test just constant integers
./bin/test_volta --gtest_filter=ValueTest.ConstantInt*

# Test all constants
./bin/test_volta --gtest_filter=ValueTest.Constant*

# Test use-def chains (after implementing Use class)
./bin/test_volta --gtest_filter=ValueTest.*Uses*

# Run everything
./bin/test_volta --gtest_filter=ValueTest.*
```

### Common Pitfalls

1. **Use-Def Chain Cycles**: When implementing `Use::setValue()`, make sure to:
   - Remove from old value FIRST
   - Update the pointer
   - Add to new value LAST

2. **Iterator Invalidation**: In `replaceAllUsesWith()`, make a COPY of uses_ before iterating, since setValue() will modify the original vector.

3. **Memory Management**: For now, constants are created with `new` and not deleted. Later, you'll implement an arena allocator. Don't worry about leaks in tests.

4. **Type Checking**: The `classof()` methods use LLVM-style RTTI. They check the ValueKind, not dynamic_cast.

## Advanced Topics (Optional)

### Constant Interning

Later, you can optimize constants by caching them:

```cpp
static std::unordered_map<int64_t, ConstantInt*> intCache_;

ConstantInt* ConstantInt::get(int64_t value, std::shared_ptr<semantic::Type> type) {
    auto it = intCache_.find(value);
    if (it != intCache_.end()) {
        return it->second;  // Return cached
    }
    auto* ci = new ConstantInt(value, type);
    intCache_[value] = ci;
    return ci;
}
```

### SSA Naming

The `id_` field enables pretty-printing in SSA form:
```
%0 = add i64 %arg0, %arg1
%1 = mul i64 %0, 2
%2 = ret i64 %1
```

Each value gets a unique %N identifier based on its ID.

## What's Next?

After implementing and testing Value:

1. **Type System** - Adapt semantic types for IR
2. **Instruction** - Full instruction hierarchy (Add, Sub, Load, Store, etc.)
3. **BasicBlock** - CFG building blocks
4. **Function** - Container for basic blocks
5. **Module** - Top-level IR container

## Getting Feedback

Once you've implemented the TODOs:

1. Run the tests: `make test`
2. Fix any failures
3. Come back and show me your implementation
4. I'll review it and provide feedback on:
   - Correctness
   - Code style
   - Efficiency
   - Edge cases you might have missed

## Key Learning Outcomes

By implementing this, you'll understand:
- ✓ SSA form and why it's powerful
- ✓ Use-def chains for optimization
- ✓ LLVM-style RTTI without dynamic_cast overhead
- ✓ Factory pattern for constants
- ✓ Type-safe IR construction
- ✓ Memory management strategies (now and with arenas later)

Good luck! Take your time, read the comments in Value.hpp, and don't hesitate to experiment.
