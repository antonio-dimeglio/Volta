# VIR Implementation Plan: Incremental Vertical Slices

**Strategy:** Build complete vertical slices (AST → VIR → LLVM → Executable) for each feature, from simplest to most complex.

**Philosophy:** Test end-to-end at every step. Never go more than a day without something running.

---

## Phase 0: Infrastructure (Week 1, Days 1-3)

**Goal:** Get the skeleton compiling. Minimal but complete pipeline.

### Day 1: VIR Base Classes
- [ ] Create `include/vir/VIRNode.hpp`
  ```cpp
  class VIRNode { SourceLocation, originatingAST }
  class VIRExpr : VIRNode { getType() }
  class VIRStmt : VIRNode { }
  ```
- [ ] Create `include/vir/VIRModule.hpp`
  ```cpp
  class VIRModule { functions, structs, enums }
  class VIRFunction { name, params, returnType, body }
  ```
- [ ] Basic CMake integration
- [ ] Compiles with no errors

### Day 2: Lowering & Codegen Skeleton
- [ ] Create `include/vir/VIRLowering.hpp`
  ```cpp
  class VIRLowering {
      std::unique_ptr<VIRModule> lower(const ast::Program*);
  }
  ```
- [ ] Create `include/vir/VIRCodegen.hpp`
  ```cpp
  class VIRCodegen {
      bool generate(const VIRModule*);
  }
  ```
- [ ] Wire into `main.cpp`:
  ```cpp
  // Old: AST → LLVM directly
  // New: AST → VIR → LLVM

  auto virModule = VIRLowering(ast, analyzer).lower();
  VIRCodegen(context, module, analyzer).generate(virModule.get());
  ```
- [ ] Compiles (both pipelines exist, even if empty)

### Day 3: Hello World Test
- [ ] Implement absolute minimum to compile:
  ```volta
  fn main() -> i32 {
      return 0
  }
  ```
- [ ] Just enough VIR nodes: `VIRFunction`, `VIRReturn`, `VIRIntLiteral`
- [ ] Just enough lowering: lower `FnDeclaration`, `ReturnStatement`
- [ ] Just enough codegen: codegen function, return
- [ ] **SUCCESS CRITERIA:** `examples/minimal.vlt` compiles and exits with code 0

**End of Day 3 Checkpoint:**
- ✅ Complete pipeline exists (AST → VIR → LLVM)
- ✅ One program runs end-to-end
- ✅ Foundation for all future work

---

## Phase 1: Expressions (Week 1 Day 4 - Week 2)

**Strategy:** Each day, add one expression type end-to-end.

### Day 4: Integer Literals
- [ ] **VIR Node:** `VIRIntLiteral` (i8, i16, i32, i64, u8, u16, u32, u64)
- [ ] **Lowering:** `lowerExpression(IntegerLiteral)` → `VIRIntLiteral`
- [ ] **Codegen:** `codegen(VIRIntLiteral)` → `ConstantInt::get()`
- [ ] **Test:**
  ```volta
  fn main() -> i32 { return 42 }
  ```
- [ ] **Verify:** Exits with code 42

### Day 5: Float & Bool Literals
- [ ] **VIR Nodes:** `VIRFloatLiteral` (f16, f32, f64), `VIRBoolLiteral`
- [ ] **Lowering:** Handle all literal types
- [ ] **Codegen:** `ConstantFP::get()`, bool as i1
- [ ] **Test:**
  ```volta
  fn get_pi() -> f64 { return 3.14 }
  fn get_flag() -> bool { return true }
  ```

### Day 6-7: Binary Operations
- [ ] **VIR Node:** `VIRBinaryOp` (Add, Sub, Mul, Div, Mod)
- [ ] **Lowering:** `lowerBinaryExpression()` with type checking
- [ ] **Codegen:** `CreateAdd()`, `CreateFAdd()`, etc.
- [ ] **Test:**
  ```volta
  fn compute() -> i32 {
      return (5 + 3) * 2 - 1  // Should be 15
  }
  ```
- [ ] **Verify:** Exits with code 15

### Week 2 Days 1-2: Comparisons & Logical Ops
- [ ] **VIR Ops:** Eq, Ne, Lt, Le, Gt, Ge, And, Or
- [ ] **Codegen:** `CreateICmp()`, `CreateFCmp()`
- [ ] **Test:**
  ```volta
  fn test_logic() -> i32 {
      if (5 > 3) and (2 < 4) {
          return 1
      }
      return 0
  }
  ```

### Week 2 Day 3: Unary Operations
- [ ] **VIR Node:** `VIRUnaryOp` (Neg, Not)
- [ ] **Test:**
  ```volta
  fn negate(x: i32) -> i32 { return -x }
  ```

**End of Phase 1 Checkpoint:**
- ✅ All basic expressions work
- ✅ Can compile arithmetic and logic
- ✅ Type system working (numeric types lowered correctly)

---

## Phase 2: Variables & Functions (Week 2 Day 4 - Week 3)

### Week 2 Day 4: Local Variables
- [ ] **VIR Nodes:** `VIRVarDecl`, `VIRLocalRef`
- [ ] **VIR Memory:** `VIRAlloca`, `VIRLoad`, `VIRStore`
- [ ] **Lowering:** Variable declarations and references
- [ ] **Codegen:** Stack allocation with `CreateAlloca()`
- [ ] **Test:**
  ```volta
  fn test_vars() -> i32 {
      let x: i32 = 10
      let y: i32 = 20
      return x + y
  }
  ```
- [ ] **Verify:** Exits with code 30

### Week 2 Day 5: Mutable Variables
- [ ] **VIR:** Track mutability in `VIRVarDecl`
- [ ] **Lowering:** Handle `mut` keyword
- [ ] **Test:**
  ```volta
  fn test_mut() -> i32 {
      let mut counter: i32 = 0
      counter = counter + 1
      counter = counter + 1
      return counter
  }
  ```
- [ ] **Verify:** Exits with code 2

### Week 3 Days 1-2: Function Parameters & Calls
- [ ] **VIR Nodes:** `VIRParamRef`, `VIRCall`
- [ ] **Lowering:** Function parameters, call expressions
- [ ] **Codegen:** Function declarations, call instructions
- [ ] **Test:**
  ```volta
  fn add(a: i32, b: i32) -> i32 {
      return a + b
  }

  fn main() -> i32 {
      return add(5, 7)
  }
  ```
- [ ] **Verify:** Exits with code 12

### Week 3 Day 3: Multiple Functions
- [ ] **Test:**
  ```volta
  fn multiply(x: i32, y: i32) -> i32 {
      return x * y
  }

  fn add(a: i32, b: i32) -> i32 {
      return a + b
  }

  fn main() -> i32 {
      let x = multiply(3, 4)
      let y = add(x, 2)
      return y  // 14
  }
  ```

**End of Phase 2 Checkpoint:**
- ✅ Functions with parameters work
- ✅ Function calls work
- ✅ Local variables (mutable and immutable) work
- ✅ **Can compile non-trivial programs!**

---

## Phase 3: Control Flow (Week 3 Day 4 - Week 4)

### Week 3 Day 4-5: If Statements
- [ ] **VIR Node:** `VIRIfStmt` (statement form)
- [ ] **Lowering:** Conditional branches
- [ ] **Codegen:** Basic blocks, conditional branch
- [ ] **Test:**
  ```volta
  fn abs(x: i32) -> i32 {
      if x < 0 {
          return -x
      } else {
          return x
      }
  }
  ```

### Week 4 Days 1-2: If Expressions
- [ ] **VIR Node:** `VIRIfExpr` (expression form)
- [ ] **Codegen:** PHI nodes for merge
- [ ] **Test:**
  ```volta
  fn max(a: i32, b: i32) -> i32 {
      return if a > b { a } else { b }
  }
  ```

### Week 4 Day 3: While Loops
- [ ] **VIR Node:** `VIRWhile`
- [ ] **Lowering:** Loop state tracking
- [ ] **Codegen:** Loop basic blocks
- [ ] **Test:**
  ```volta
  fn sum_to_n(n: i32) -> i32 {
      let mut i: i32 = 0
      let mut sum: i32 = 0
      while i <= n {
          sum = sum + i
          i = i + 1
      }
      return sum
  }
  ```
- [ ] **Verify:** `sum_to_n(10)` returns 55

### Week 4 Day 4: Break & Continue
- [ ] **VIR Nodes:** `VIRBreak`, `VIRContinue`
- [ ] **Test:**
  ```volta
  fn find_first_even(start: i32) -> i32 {
      let mut i: i32 = start
      while i < 100 {
          if i % 2 == 1 {
              i = i + 1
              continue
          }
          return i
      }
      return -1
  }
  ```

**End of Phase 3 Checkpoint:**
- ✅ If/else works
- ✅ Loops work
- ✅ Break/continue work
- ✅ **Can write real algorithms!**

---

## Phase 4: For Loops & Ranges (Week 4 Day 5 - Week 5 Day 2)

### Week 4 Day 5 - Week 5 Day 1: Range Desugaring
- [ ] **VIR:** For loops desugar to while
- [ ] **Lowering:** `lowerForRange()` - convert `for i in 0..10` to while
- [ ] **Test:**
  ```volta
  fn sum_range() -> i32 {
      let mut sum: i32 = 0
      for i in 0..10 {
          sum = sum + i
      }
      return sum
  }
  ```

### Week 5 Day 2: Inclusive Ranges
- [ ] **Support:** `0..=10` (inclusive)
- [ ] **Test:**
  ```volta
  fn sum_inclusive() -> i32 {
      let mut sum: i32 = 0
      for i in 1..=10 {
          sum = sum + i
      }
      return sum  // 55
  }
  ```

**End of Phase 4 Checkpoint:**
- ✅ For loops over ranges work
- ✅ **examples/for.vlt** compiles through VIR!

---

## Phase 5: Runtime Functions & Printing (Week 5 Days 3-5)

### Week 5 Day 3: Runtime Call Infrastructure
- [ ] **VIR Node:** `VIRCallRuntime`
- [ ] **Lowering:** Distinguish runtime vs user functions
- [ ] **Codegen:** Look up runtime functions via `RuntimeFunctions`
- [ ] **Test:** Call `volta_gc_alloc()`

### Week 5 Days 4-5: Print Function
- [ ] **Lowering:** `print()` calls → `VIRCallRuntime("volta_print_*")`
- [ ] **Test:**
  ```volta
  fn main() -> i32 {
      print(42)
      print("Hello, World!")
      return 0
  }
  ```
- [ ] **Verify:** Output appears!

**End of Phase 5 Checkpoint:**
- ✅ Can call runtime functions
- ✅ **examples/hello.vlt** works through VIR!
- ✅ **Visible output - big motivation boost!**

---

## Phase 6: Boxing & Type Operations (Week 6)

### Week 6 Days 1-2: Box & Unbox
- [ ] **VIR Nodes:** `VIRBox`, `VIRUnbox`
- [ ] **Codegen:** `createBox()`, `createUnbox()` helpers
- [ ] **Test:** Manually box/unbox a value

### Week 6 Days 3-4: Casting
- [ ] **VIR Nodes:** `VIRCast`, `VIRImplicitCast`
- [ ] **Lowering:** Insert casts for type conversions
- [ ] **Codegen:** `CreateIntCast()`, `CreateSIToFP()`, etc.
- [ ] **Test:**
  ```volta
  fn cast_test() -> f64 {
      let x: i32 = 42
      let y: f64 = x as f64
      return y
  }
  ```

### Week 6 Day 5: String Literals (Interning)
- [ ] **VIR Node:** `VIRStringLiteral` with interned names
- [ ] **Lowering:** Deduplicate identical strings
- [ ] **Codegen:** `CreateGlobalString()`
- [ ] **Test:**
  ```volta
  fn greet() -> i32 {
      print("Hello")
      print("Hello")  // Should use same global
      return 0
  }
  ```

**End of Phase 6 Checkpoint:**
- ✅ Type operations work
- ✅ String literals work
- ✅ Ready for arrays!

---

## Phase 7: Arrays (Week 7-8)

**Strategy:** Implement in two stages - fixed arrays first (compiler built-in), then dynamic arrays (stdlib).

### Week 7 Days 1-2: Fixed Array Infrastructure

**Goal:** Get `[T; N]` working as compiler primitives.

- [ ] **Lexer/Parser:**
  - [ ] Add `;` token to lexer
  - [ ] Parse `[T; N]` in type positions
  - [ ] Parse `[value; N]` for repeated-value literals
  - [ ] Parse `[v1, v2, v3]` for explicit-value literals

- [ ] **Semantic Analysis:**
  - [ ] Add `FixedArrayType(elementType, size)` to type system
  - [ ] Size must be compile-time constant
  - [ ] Type check array literals against fixed array types
  - [ ] Track which variables escape scope (for allocation strategy)

- [ ] **Test:**
  ```volta
  fn test_fixed_array() -> i32 {
      let coords: [i32; 3] = [10, 20, 30]
      return coords[1]  // 20
  }
  ```

### Week 7 Day 3: Fixed Array VIR Lowering

- [ ] **VIR Nodes:** `VIRFixedArrayNew`, `VIRFixedArrayGet`, `VIRFixedArraySet`
- [ ] **Lowering:**
  - [ ] Lower fixed array type declarations
  - [ ] Lower `[v1, v2, v3]` → `VIRFixedArrayNew(elements=[...], size=3)`
  - [ ] Lower `[0; 256]` → `VIRFixedArrayNew(elements=[0], size=256)`  (repeat pattern)
  - [ ] Lower `arr[i]` → `VIRFixedArrayGet(arr, VIRBoundsCheck(arr, i))`
  - [ ] Lower `arr[i] = v` → `VIRFixedArraySet(arr, VIRBoundsCheck(arr, i), v)`
  - [ ] Escape analysis: mark `isStackAllocated` flag

- [ ] **Test:** Same as above, verify VIR structure

### Week 7 Days 4-5: Fixed Array Codegen

- [ ] **Codegen:**
  - [ ] `codegen(VIRFixedArrayNew)`:
    - [ ] Stack path: `CreateAlloca(llvm::ArrayType)`, initialize elements
    - [ ] Heap path: `volta_gc_alloc()`, cast, initialize elements
  - [ ] `codegen(VIRFixedArrayGet)`: GEP + Load
  - [ ] `codegen(VIRFixedArraySet)`: GEP + Store

- [ ] **Bounds Checking:**
  - [ ] `VIRBoundsCheck` node (if not already implemented)
  - [ ] Codegen: compare index against array size, panic on out-of-bounds

- [ ] **Test:**
  ```volta
  fn test_fixed_arrays() -> i32 {
      // Stack allocation (small, local)
      let small: [i32; 5] = [1, 2, 3, 4, 5]

      // Heap allocation (large)
      let large: [u8; 256] = [0; 256]

      // Indexing
      let x = small[2]  // 3

      // Mutation
      small[0] = 99

      return small[0] + x  // 99 + 3 = 102
  }
  ```

- [ ] **Verify:** Works end-to-end, exits with code 102

**Checkpoint after Week 7:**
- ✅ Fixed arrays `[T; N]` fully working
- ✅ Stack and heap allocation working
- ✅ Bounds checking working
- ✅ Can write fixed-size buffers, coordinates, etc.

### Week 8 Day 1: Runtime Helper for Dynamic Array Backing Storage

**Goal:** Add runtime function to allocate heap-allocated fixed arrays with dynamic size.

- [ ] **Runtime Function:**
  ```c
  // In volta_runtime.c
  void* volta_alloc_fixed_array(int64_t element_size, int64_t count) {
      int64_t total_size = element_size * count;
      return volta_gc_alloc(total_size);
  }
  ```

- [ ] **VIR:**
  - [ ] `VIRCallRuntime("volta_alloc_fixed_array", [elemSize, count])`

- [ ] **Test:** Can allocate heap arrays with runtime-determined size

### Week 8 Days 2-3: Dynamic Array Stdlib Implementation

**Goal:** Implement `Array[T]` as a generic struct in Volta stdlib.

- [ ] **Create stdlib file:** `stdlib/array.vlt`
  ```volta
  struct Array[T] {
      data: [T],        // Reference to heap-allocated fixed array
      length: i32,
      capacity: i32
  }

  fn Array.new[T]() -> Array[T] {
      let data = volta_alloc_fixed_array[T](16)
      return Array { data: data, length: 0, capacity: 16 }
  }

  fn Array.length[T](self: Array[T]) -> i32 {
      return self.length
  }

  fn Array.get[T](self: Array[T], index: i32) -> T {
      // Bounds check
      if index < 0 or index >= self.length {
          panic("Array index out of bounds")
      }
      return self.data[index]
  }

  fn Array.push[T](mut self: Array[T], value: T) {
      if self.length == self.capacity {
          // Resize: allocate bigger backing array
          let new_capacity = self.capacity * 2
          let new_data = volta_alloc_fixed_array[T](new_capacity)

          // Copy old elements
          let mut i = 0
          while i < self.length {
              new_data[i] = self.data[i]
              i = i + 1
          }

          self.data = new_data
          self.capacity = new_capacity
      }

      self.data[self.length] = value
      self.length = self.length + 1
  }

  fn Array.pop[T](mut self: Array[T]) -> T {
      if self.length == 0 {
          panic("Cannot pop from empty array")
      }
      self.length = self.length - 1
      return self.data[self.length]
  }
  ```

- [ ] **Prerequisite:** Ensure generic struct monomorphization works (from Phase 10, may need to bring forward)

- [ ] **Test:**
  ```volta
  fn test_dynamic_arrays() -> i32 {
      let mut nums: Array[i32] = Array.new()

      nums.push(10)
      nums.push(20)
      nums.push(30)

      let len = nums.length()  // 3
      let item = nums.get(1)   // 20

      let popped = nums.pop()  // 30

      return len + item  // 3 + 20 = 23
  }
  ```

- [ ] **Verify:** Compiles and works end-to-end

**Checkpoint after Week 8 Day 3:**
- ✅ Dynamic arrays `Array[T]` working
- ✅ Implemented **in Volta itself** (not compiler magic!)
- ✅ Basic operations: `new()`, `push()`, `pop()`, `get()`, `length()`

### Week 8 Days 4-5: Higher-Order Array Functions

**Goal:** Implement `map()`, `filter()`, `reduce()` with wrapper generation.

- [ ] **Stdlib additions:**
  ```volta
  fn Array.map[T, U](self: Array[T], f: fn(T) -> U) -> Array[U] {
      let mut result: Array[U] = Array.new()
      let mut i = 0
      while i < self.length {
          result.push(f(self.data[i]))
          i = i + 1
      }
      return result
  }

  fn Array.filter[T](self: Array[T], predicate: fn(T) -> bool) -> Array[T] {
      let mut result: Array[T] = Array.new()
      let mut i = 0
      while i < self.length {
          if predicate(self.data[i]) {
              result.push(self.data[i])
          }
          i = i + 1
      }
      return result
  }

  fn Array.reduce[T, U](self: Array[T], init: U, f: fn(U, T) -> U) -> U {
      let mut acc = init
      let mut i = 0
      while i < self.length {
          acc = f(acc, self.data[i])
          i = i + 1
      }
      return acc
  }
  ```

- [ ] **VIR Wrapper Generation:**
  - [ ] Already implemented in Phase 6 (wrapper infrastructure)
  - [ ] Ensure monomorphized array methods work with function pointers
  - [ ] `arr.map(double)` → `VIRCall("Array_i32_map_i32", [arr, VIRWrapFunction("double")])`

- [ ] **Test:**
  ```volta
  fn double(x: i32) -> i32 { return x * 2 }
  fn is_even(x: i32) -> bool { return (x % 2) == 0 }
  fn add(acc: i32, x: i32) -> i32 { return acc + x }

  fn test_hof() -> i32 {
      let nums: Array[i32] = [1, 2, 3, 4, 5]

      let doubled: Array[i32] = nums.map(double)  // [2, 4, 6, 8, 10]
      let evens: Array[i32] = nums.filter(is_even)  // [2, 4]
      let sum: i32 = nums.reduce(0, add)  // 15

      return evens.length() + sum  // 2 + 15 = 17
  }
  ```

- [ ] **Verify:** **examples/array_complex.vlt** works!

**End of Phase 7 Checkpoint:**
- ✅ Fixed arrays `[T; N]` fully working (compiler built-in)
- ✅ Dynamic arrays `Array[T]` fully working (stdlib implementation!)
- ✅ **Wrapper generation works - core problem solved!**
- ✅ Higher-order functions (`map`, `filter`, `reduce`) working
- ✅ **No boxing overhead** due to monomorphization
- ✅ **examples/array_complex.vlt** works!

**Key Insight Validated:**
Dynamic arrays are just a thin wrapper over fixed arrays! The compiler provides the primitive (`[T; N]`), and the stdlib builds the abstraction (`Array[T]`). This is how systems languages like Rust work.

---

## Phase 8: Structs (Week 9-10)

### Week 9 Days 1-3: Struct Declaration & Instantiation
- [ ] **VIR Nodes:** `VIRStructNew`, `VIRStructGet`, `VIRStructSet`
- [ ] **VIR Memory:** `VIRGEPFieldAccess`
- [ ] **Lowering:** Struct literals, field access
- [ ] **Codegen:** Declare structs, heap allocation, GEP
- [ ] **Test:**
  ```volta
  struct Point {
      x: f64,
      y: f64
  }

  fn test_struct() -> f64 {
      let p: Point = Point { x: 3.0, y: 4.0 }
      return p.x  // 3.0
  }
  ```

### Week 9 Days 4-5: Struct Methods
- [ ] **Lowering:** Method call desugaring for structs
- [ ] **Test:**
  ```volta
  struct Point { x: f64, y: f64 }

  fn Point.distance(self: Point) -> f64 {
      return sqrt(self.x * self.x + self.y * self.y)
  }

  fn main() -> i32 {
      let p: Point = Point { x: 3.0, y: 4.0 }
      let d: f64 = p.distance()  // 5.0
      return 0
  }
  ```

### Week 10 Days 1-2: Mutable Struct Methods
- [ ] **Lowering:** Track `mut self` in method calls
- [ ] **Test:**
  ```volta
  struct Counter { value: i32 }

  fn Counter.increment(mut self: Counter) {
      self.value = self.value + 1
  }

  fn main() -> i32 {
      let mut c: Counter = Counter { value: 0 }
      c.increment()
      c.increment()
      return c.value  // 2
  }
  ```

### Week 10 Days 3-5: Nested Structs & Arrays of Structs
- [ ] **Test:**
  ```volta
  struct Point { x: i32, y: i32 }

  fn test_array_of_structs() -> i32 {
      let points: Array[Point] = [
          Point { x: 1, y: 2 },
          Point { x: 3, y: 4 }
      ]
      return points[1].y  // 4
  }
  ```

**End of Phase 8 Checkpoint:**
- ✅ Structs fully working
- ✅ Methods (instance and mutable) work
- ✅ **Real data structures possible!**

---

## Phase 9: Enums & Pattern Matching (Week 11-12)

### Week 11 Days 1-3: Basic Enums
- [ ] **VIR Nodes:** `VIREnumNew`, `VIREnumGetTag`, `VIREnumGetData`
- [ ] **Lowering:** Enum variant construction
- [ ] **Codegen:** Tag + data representation
- [ ] **Test:**
  ```volta
  enum Status {
      Success,
      Failure
  }

  fn test_enum() -> i32 {
      let s: Status = Status.Success
      return 0
  }
  ```

### Week 11 Days 4-5: Enums with Data
- [ ] **Test:**
  ```volta
  enum Option[T] {
      Some(T),
      None
  }

  fn test_option() -> Option[i32] {
      return Some(42)
  }
  ```

### Week 12 Days 1-3: Pattern Matching
- [ ] **VIR Nodes:** `VIRMatch`, `VIRPattern*` (all pattern types)
- [ ] **Lowering:** Pattern compilation
- [ ] **Codegen:** Switch on tag, bind variables
- [ ] **Test:**
  ```volta
  enum Option[T] { Some(T), None }

  fn unwrap_or(opt: Option[i32], default: i32) -> i32 {
      return match opt {
          Some(value) => value,
          None => default
      }
  }
  ```

### Week 12 Days 4-5: Pattern Guards & Complex Patterns
- [ ] **Test:**
  ```volta
  match value {
      Some(x) if x > 10 => print("Big"),
      Some(x) => print("Small"),
      None => print("Nothing")
  }
  ```

**End of Phase 9 Checkpoint:**
- ✅ Enums work
- ✅ Pattern matching works
- ✅ **Option[T] and Result[T, E] functional!**

---

## Phase 10: Generics (Week 13)

### Week 13 Days 1-3: Generic Structs
- [ ] **Lowering:** Monomorphization tracking
- [ ] **Lowering:** Name mangling (Box[T] → Box_i32)
- [ ] **Test:**
  ```volta
  struct Box[T] { value: T }

  fn test_generics() -> i32 {
      let intBox: Box[i32] = Box { value: 42 }
      let floatBox: Box[f64] = Box { value: 3.14 }
      return intBox.value
  }
  ```

### Week 13 Days 4-5: Generic Functions
- [ ] **Test:**
  ```volta
  fn identity[T](x: T) -> T { return x }

  fn main() -> i32 {
      let a: i32 = identity(42)
      let b: f64 = identity(3.14)
      return a
  }
  ```

**End of Phase 10 Checkpoint:**
- ✅ Generics work!
- ✅ **Monomorphization complete!**

---

## Phase 11: Validation & Testing (Week 14)

### Week 14 Days 1-2: VIR Validator
- [ ] Implement `VIRValidator` class
- [ ] Type checking
- [ ] Return statement validation
- [ ] Expression validation

### Week 14 Days 3-5: Comprehensive Testing
- [ ] Run all existing examples through VIR
- [ ] Fix any bugs found
- [ ] Performance testing
- [ ] Documentation updates

**End of Phase 11 Checkpoint:**
- ✅ VIR validator works
- ✅ All tests pass
- ✅ **No regressions!**

---

## Phase 12: Migration & Cleanup (Week 14 Day 5)

### Final Day: Switch Over
- [ ] Update main compiler driver to use VIR by default
- [ ] Remove old AST → LLVM codegen
- [ ] Update documentation
- [ ] **CELEBRATE! 🎉**

---

## Daily Workflow

**Each day should follow this pattern:**

1. **Morning (2-3 hours):** Implement VIR nodes + lowering
2. **Afternoon (2-3 hours):** Implement codegen
3. **Evening (1 hour):** Write test, verify it works
4. **Before bed:** Commit working code

**Never end a day with broken code!** If something doesn't work, scale back scope to get *something* working.

---

## Success Metrics

**End of Each Week:**
- ✅ At least one complete example program compiles
- ✅ No regressions in previously working features
- ✅ All new code has tests

**Red Flags:**
- 🚩 Can't compile anything for >1 day
- 🚩 Tests fail for >2 days
- 🚩 Unclear what to implement next

**If you hit a red flag:** Stop, ask for help, simplify scope.

---

## Parallel Work Opportunities

**If you have multiple developers:**

| Person A | Person B |
|----------|----------|
| Week 1-2: Expressions | Week 1-2: Codegen infrastructure |
| Week 3-4: Control flow | Week 3-4: Memory operations |
| Week 5-6: Arrays | Week 5-6: Type operations |
| Week 7-8: Structs | Week 7-8: Enums |

**But for solo work:** Follow the daily incremental approach above.

---

## Fallback Plan

**If timeline slips:**

**Must-Have (Core Language):**
- Expressions, variables, functions ✅
- Control flow (if, while, for) ✅
- Arrays with map/filter ✅
- Structs ✅

**Nice-to-Have (Advanced Features):**
- Enums + pattern matching ⚠️
- Generics ⚠️

**Can Defer:**
- Closures (stub exists)
- Module system (stub exists)
- FFI (stub exists)

**If running behind:** Skip enums/generics initially, get core language working end-to-end, then add them.

---

## Key Insight: Why This Works

**Traditional approach (sequential):**
```
[Lowering: 8 weeks] → [Codegen: 6 weeks] → [Testing: finds bugs] → [Redo everything: 4 weeks]
Total: 18 weeks
```

**Incremental approach:**
```
[Day 1: Literals end-to-end] → [Test, fix bugs]
[Day 2: Binary ops end-to-end] → [Test, fix bugs]
[Day 3: Functions end-to-end] → [Test, fix bugs]
...
Total: 14 weeks, with working software every day
```

**The difference:**
- ✅ Bugs found early (cheap to fix)
- ✅ Constant feedback on design
- ✅ Motivation from visible progress
- ✅ Can ship partial system if needed

---

## Final Recommendation

**Start with Day 1 of Phase 0 tomorrow:**

1. Create VIR base classes (2 hours)
2. Wire into main.cpp (1 hour)
3. Implement minimal nodes (2 hours)
4. Get `return 0` compiling (1 hour)

**By end of day:** You'll have proved the approach works. Then continue daily slices.

**Trust the process.** It feels slower at first (building both lowering and codegen for each tiny piece), but you'll be *way* ahead by week 3 when you have a working system instead of just half of one.

---

**Next question:** Ready to start Phase 0 Day 1, or want to discuss further?
