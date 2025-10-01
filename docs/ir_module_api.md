# Volta IR Module API Reference

## Overview

The IR (Intermediate Representation) module converts the semantically-analyzed AST into a lower-level, SSA-like representation that's easier to optimize and translate to bytecode.

## Module Structure

```
include/IR/
├── IR.hpp           - Core IR types (Value, Instruction, BasicBlock, Function)
├── IRModule.hpp     - Container for entire program's IR
├── IRBuilder.hpp    - Helper for constructing IR
├── IRGenerator.hpp  - AST → IR converter
└── IRPrinter.hpp    - Debug printer for IR
```

---

## Core Concepts

### 1. Values (`Value`)

Everything in IR is a **value** - a typed entity that can be used as an operand:

```cpp
class Value {
    enum class Kind {
        Instruction,    // %0 = add %1, %2
        Parameter,      // Function parameter
        Constant,       // 42, 3.14, "hello"
        GlobalVariable, // @global_var
        BasicBlock      // bb0 (used as branch target)
    };

    Kind kind() const;
    std::shared_ptr<semantic::Type> type() const;
    std::string name() const;
};
```

**Key types of values:**
- **Instructions**: Results of operations (`%0`, `%1`, etc.)
- **Constants**: Literals (`42`, `3.14`, `true`, `"hello"`)
- **Parameters**: Function arguments
- **Globals**: Global variables

### 2. Instructions (`Instruction`)

Instructions perform operations and produce values:

```cpp
class Instruction : public Value {
    enum class Opcode {
        // Arithmetic
        Add, Sub, Mul, Div, Mod, Neg,

        // Comparison
        Eq, Ne, Lt, Le, Gt, Ge,

        // Logical
        And, Or, Not,

        // Memory
        Alloc, Load, Store, GetField, SetField, GetElement, SetElement,

        // Control flow
        Br, BrIf, Ret, Call, CallForeign,

        // Special
        Phi
    };

    Opcode opcode() const;
    std::vector<Value*> operands() const;
};
```

**Instruction types:**
- **BinaryInst**: `%result = add %left, %right`
- **UnaryInst**: `%result = neg %operand`
- **LoadInst**: `%result = load [%address]`
- **StoreInst**: `store %value, [%address]`
- **BranchInst**: `br bb_target`
- **BranchIfInst**: `br_if %cond, bb_then, bb_else`
- **ReturnInst**: `ret %value` or `ret`
- **CallInst**: `%result = call @function, %arg1, %arg2`

### 3. Basic Blocks (`BasicBlock`)

A basic block is a sequence of instructions with:
- **Single entry**: Can only be entered at the start
- **Single exit**: Last instruction is a terminator (branch/return)
- **No internal branches**: Executes sequentially

```cpp
class BasicBlock : public Value {
    void addInstruction(std::unique_ptr<Instruction> inst);
    const std::vector<std::unique_ptr<Instruction>>& instructions() const;
    bool hasTerminator() const;
    Instruction* terminator() const;
};
```

**Example:**
```
bb0:
    %0 = add %a, %b
    %1 = mul %0, 2
    br_if %1, bb1, bb2
```

### 4. Functions (`Function`)

Functions contain basic blocks:

```cpp
class Function {
    const std::string& name() const;
    const std::vector<std::unique_ptr<BasicBlock>>& basicBlocks() const;
    const std::vector<std::unique_ptr<Parameter>>& parameters() const;

    BasicBlock* addBasicBlock(std::unique_ptr<BasicBlock> block);
    BasicBlock* entryBlock() const;
};
```

**Example:**
```
function @factorial(%n: int) -> int:
  bb0:
    %0 = le %n, 1
    br_if %0, bb1, bb2

  bb1:
    ret 1

  bb2:
    %1 = sub %n, 1
    %2 = call @factorial, %1
    %3 = mul %n, %2
    ret %3
```

### 5. Module (`IRModule`)

Container for the entire program:

```cpp
class IRModule {
    Function* addFunction(std::unique_ptr<Function> function);
    GlobalVariable* addGlobal(std::unique_ptr<GlobalVariable> global);
    size_t addStringLiteral(const std::string& str);

    Function* getFunction(const std::string& name) const;
    GlobalVariable* getGlobal(const std::string& name) const;

    bool verify(std::ostream& errors) const;
};
```

---

## IRBuilder API

The `IRBuilder` is a **helper class** that makes building IR much easier. It automatically:
- Generates unique names (`%0`, `%1`, `%2`, ...)
- Inserts instructions into the current block
- Manages context

### Basic Usage

```cpp
IRBuilder builder;

// Create function
auto func = builder.createFunction("add", funcType);
Function* funcPtr = func.get();

// Create entry block
auto entry = builder.createBasicBlock("entry", funcPtr);
builder.setInsertPoint(entry.get());

// Build instructions
Value* sum = builder.createAdd(param0, param1);
builder.createRet(sum);
```

### Arithmetic Operations

```cpp
Value* createAdd(Value* left, Value* right);
Value* createSub(Value* left, Value* right);
Value* createMul(Value* left, Value* right);
Value* createDiv(Value* left, Value* right);
Value* createMod(Value* left, Value* right);
Value* createNeg(Value* operand);
```

**Example:**
```cpp
// let result = (a + b) * 2
Value* sum = builder.createAdd(a, b);
Value* two = builder.getInt(2);
Value* result = builder.createMul(sum, two);
```

### Comparison Operations

```cpp
Value* createEq(Value* left, Value* right);   // ==
Value* createNe(Value* left, Value* right);   // !=
Value* createLt(Value* left, Value* right);   // <
Value* createLe(Value* left, Value* right);   // <=
Value* createGt(Value* left, Value* right);   // >
Value* createGe(Value* left, Value* right);   // >=
```

### Logical Operations

```cpp
Value* createAnd(Value* left, Value* right);  // &&
Value* createOr(Value* left, Value* right);   // ||
Value* createNot(Value* operand);             // !
```

### Memory Operations

```cpp
// Allocate local variable
Value* createAlloc(std::shared_ptr<semantic::Type> type);

// Load/store
Value* createLoad(std::shared_ptr<semantic::Type> type, Value* address);
void createStore(Value* value, Value* address);

// Struct field access
Value* createGetField(Value* object, size_t fieldIndex,
                     std::shared_ptr<semantic::Type> fieldType);
void createSetField(Value* object, size_t fieldIndex, Value* value);

// Array element access
Value* createGetElement(Value* array, Value* index,
                       std::shared_ptr<semantic::Type> elementType);
void createSetElement(Value* array, Value* index, Value* value);
```

**Example:**
```cpp
// let x: int = 5
Value* xAddr = builder.createAlloc(intType);
Value* five = builder.getInt(5);
builder.createStore(five, xAddr);

// Later: x + 2
Value* xVal = builder.createLoad(intType, xAddr);
Value* two = builder.getInt(2);
Value* result = builder.createAdd(xVal, two);
```

### Control Flow

```cpp
// Unconditional branch
void createBr(BasicBlock* target);

// Conditional branch
void createBrIf(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock);

// Return
void createRetVoid();
void createRet(Value* value);
```

**Example - If statement:**
```cpp
// if (x > 0) { y = 1 } else { y = 2 }

BasicBlock* thenBB = builder.createBasicBlock("then", func).get();
BasicBlock* elseBB = builder.createBasicBlock("else", func).get();
BasicBlock* mergeBB = builder.createBasicBlock("merge", func).get();

// Condition
Value* zero = builder.getInt(0);
Value* cond = builder.createGt(x, zero);
builder.createBrIf(cond, thenBB, elseBB);

// Then block
builder.setInsertPoint(thenBB);
builder.createStore(builder.getInt(1), y);
builder.createBr(mergeBB);

// Else block
builder.setInsertPoint(elseBB);
builder.createStore(builder.getInt(2), y);
builder.createBr(mergeBB);

// Continue after if
builder.setInsertPoint(mergeBB);
```

### Function Calls

```cpp
// Call Volta function
Value* createCall(Function* callee, const std::vector<Value*>& arguments);

// Call C function
Value* createCallForeign(const std::string& foreignName,
                        std::shared_ptr<semantic::Type> returnType,
                        const std::vector<Value*>& arguments);
```

**Example:**
```cpp
// let result = factorial(5)
Value* five = builder.getInt(5);
Value* result = builder.createCall(factorialFunc, {five});
```

### Constants

```cpp
Constant* getInt(int64_t value);
Constant* getFloat(double value);
Constant* getBool(bool value);
Constant* getString(const std::string& value);
Constant* getNone();
```

---

## IRGenerator API

Converts AST to IR:

```cpp
class IRGenerator {
public:
    explicit IRGenerator(ErrorReporter& reporter);

    // Main entry point
    std::unique_ptr<IRModule> generate(
        const ast::Program& program,
        const semantic::SemanticAnalyzer& analyzer);

private:
    // Top-level
    void generateFunction(const ast::FunctionDeclaration& funcDecl);
    void generateGlobal(const ast::VarDeclaration& varDecl);

    // Statements
    void generateStatement(const ast::Statement* stmt);
    void generateVarDeclaration(const ast::VarDeclaration& varDecl);
    void generateIfStatement(const ast::IfStatement& stmt);
    void generateWhileStatement(const ast::WhileStatement& stmt);
    void generateReturnStatement(const ast::ReturnStatement& stmt);

    // Expressions
    Value* generateExpression(const ast::Expression* expr);
    Value* generateBinaryExpression(const ast::BinaryExpression& expr);
    Value* generateCallExpression(const ast::CallExpression& expr);
    Value* generateIdentifier(const ast::Identifier& ident);
    // ... more expression types

    // Helpers
    void declareVariable(const std::string& name, Value* value);
    Value* lookupVariable(const std::string& name);
};
```

### Usage

```cpp
// After semantic analysis
IRGenerator generator(errorReporter);
auto irModule = generator.generate(ast, semanticAnalyzer);

if (irModule) {
    // Success! Print IR for debugging
    IRPrinter printer(std::cout);
    printer.print(*irModule);
}
```

---

## IRPrinter API

Pretty-print IR for debugging:

```cpp
class IRPrinter {
public:
    explicit IRPrinter(std::ostream& out);

    void print(const IRModule& module);
    void printFunction(const Function& function);
    void printBasicBlock(const BasicBlock& block);
    void printInstruction(const Instruction& inst);
};
```

### Usage

```cpp
IRPrinter printer(std::cout);
printer.print(*module);
```

**Output:**
```
function @factorial(%n: int) -> int:
  bb0:
    %0 = le %n, 1
    br_if %0, bb1, bb2

  bb1:
    ret 1

  bb2:
    %1 = sub %n, 1
    %2 = call @factorial, %1
    %3 = mul %n, %2
    ret %3
```

---

## Complete Example: Generating IR for a Function

Let's generate IR for this Volta function:

```volta
fn add(a: int, b: int) -> int {
    return a + b
}
```

### Using IRBuilder Directly

```cpp
IRBuilder builder;

// Create function type
auto intType = std::make_shared<semantic::PrimitiveType>(
    semantic::PrimitiveType::PrimitiveKind::Int);
auto funcType = std::make_shared<semantic::FunctionType>(
    std::vector<std::shared_ptr<semantic::Type>>{intType, intType},
    intType);

// Create function
auto func = builder.createFunction("add", funcType);
Function* funcPtr = func.get();

// Create parameters
auto param0 = builder.createParameter(intType, "%a", 0);
auto param1 = builder.createParameter(intType, "%b", 1);

// Create entry block
auto entry = builder.createBasicBlock("entry", funcPtr);
BasicBlock* entryPtr = entry.get();
funcPtr->addBasicBlock(std::move(entry));

builder.setInsertPoint(entryPtr);

// Generate: %0 = add %a, %b
Value* sum = builder.createAdd(param0.get(), param1.get());

// Generate: ret %0
builder.createRet(sum);
```

**Generated IR:**
```
function @add(%a: int, %b: int) -> int:
  entry:
    %0 = add %a, %b
    ret %0
```

---

## IR Verification

The `IRModule::verify()` method checks:
- All basic blocks end with terminators
- All branches target valid blocks
- All values are defined before use
- Type consistency

```cpp
std::ostringstream errors;
if (!module->verify(errors)) {
    std::cerr << "IR verification failed:\n" << errors.str();
}
```

---

## Implementation Strategy

### Phase 1: Implement Core IR Classes
1. Implement `IR.hpp` classes (Value, Instruction, BasicBlock, Function)
2. Implement `IRModule` container
3. Write tests for each class

### Phase 2: Implement IRBuilder
1. Implement IRBuilder helper methods
2. Test by manually constructing simple functions
3. Verify with IRPrinter

### Phase 3: Implement IRGenerator
1. Start with simple expressions (literals, binary ops)
2. Add control flow (if, while)
3. Add function calls
4. Add complex features (arrays, structs)

### Phase 4: Testing
1. Test IR generation for each AST node type
2. Compare printed IR with expected output
3. Run verification on all generated IR

---

## Key Design Decisions

### 1. SSA-Like but Simplified

We use **fresh variables** for each value but don't initially implement phi nodes:

```
// Instead of:
x1 = 1
if cond {
    x2 = 2
} else {
    x3 = 3
}
x4 = phi [x2, then], [x3, else]

// We do:
x_addr = alloc int
store 1, x_addr
if cond {
    store 2, x_addr
} else {
    store 3, x_addr
}
x_val = load x_addr
```

**Why?** Simpler to implement, still optimizable, can add phi nodes later.

### 2. Memory Model

Variables are allocated with `alloc` and accessed with `load`/`store`:

```
%x = alloc int       // Allocate space for x
store 5, %x          // x = 5
%val = load %x       // Read x
```

**Why?** Explicit memory operations make analysis easier, matches how real machines work.

### 3. Typed Values

Every value has a type from semantic analysis:

```cpp
Value* val = builder.createAdd(a, b);
val->type();  // Returns shared_ptr<semantic::Type>
```

**Why?** Type info flows through IR, enables type-based optimizations.

---

## Next Steps

1. **Implement IR.hpp** - Start with Value, Constant, Instruction base classes
2. **Implement specific instruction types** - BinaryInst, UnaryInst, etc.
3. **Implement BasicBlock and Function** - Container classes
4. **Implement IRModule** - Top-level container
5. **Implement IRBuilder** - Helper methods
6. **Implement IRPrinter** - For debugging
7. **Implement IRGenerator** - AST → IR conversion
8. **Write tests** - For each component

Each step should be tested before moving to the next!
