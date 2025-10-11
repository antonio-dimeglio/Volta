# VIR File Structure

Created: October 10, 2025
Status: Infrastructure Ready for Implementation

## Directory Structure

```
Volta/
├── include/vir/          # VIR header files
│   ├── VIRNode.hpp       # Base classes (VIRNode, VIRExpr, VIRStmt)
│   ├── VIRExpression.hpp # All expression node types
│   ├── VIRStatement.hpp  # All statement node types
│   ├── VIRModule.hpp     # Module, Function, Struct, Enum declarations
│   ├── VIRLowering.hpp   # AST → VIR transformation
│   ├── VIRCodegen.hpp    # VIR → LLVM IR + RuntimeFunctions
│   └── VIRBuilder.hpp    # Helper for constructing VIR nodes
│
├── src/vir/             # VIR implementation files
│   ├── VIRNode.cpp       # Base class implementations
│   ├── VIRExpression.cpp # Expression constructors
│   ├── VIRStatement.cpp  # Statement constructors
│   ├── VIRModule.cpp     # Module implementations
│   ├── VIRLowering.cpp   # AST lowering logic
│   ├── VIRCodegen.cpp    # LLVM codegen logic
│   └── VIRBuilder.cpp    # Builder implementations
│
├── include/runtime/     # Runtime library (KEPT!)
│   └── volta_runtime.h  # C runtime declarations
│
├── src/runtime/         # Runtime library (KEPT!)
│   └── volta_runtime.c  # C runtime implementation
│
├── examples/            # Volta example programs (KEPT!)
│   ├── hello.vlt
│   ├── factorial.vlt
│   ├── for.vlt
│   ├── while.vlt
│   ├── array_simple.vlt
│   ├── array_complex.vlt
│   └── map_test.vlt
│
└── docs/                # Documentation
    ├── vir_design.md                # Complete VIR design spec
    ├── vir_implementation_plan.md   # 14-week incremental plan
    └── vir_file_structure.md        # This file
```

## What Was Removed

```
❌ include/llvm/LLVMCodegen.hpp      (Old direct AST → LLVM codegen)
❌ include/llvm/RuntimeFunctions.hpp  (Replaced by VIRCodegen.hpp)
❌ src/llvm/LLVMCodegen.cpp          (Replaced by VIR pipeline)
❌ src/llvm/RuntimeFunctions.cpp     (Replaced by VIR pipeline)
```

## What Was Kept

```
✅ include/runtime/volta_runtime.h   (C runtime library - language agnostic)
✅ src/runtime/volta_runtime.c       (C runtime implementation)
✅ examples/*.vlt                    (All Volta example programs)
✅ include/ast/*                     (AST remains unchanged)
✅ include/semantic/*                (Semantic analyzer remains unchanged)
✅ src/ast/*                         (AST implementations remain)
✅ src/semantic/*                    (Semantic analyzer remains)
```

## What Was Created

### Header Files (7 files)

1. **VIRNode.hpp** - Base classes for all VIR nodes
   - `VIRNode` - Base with source location tracking
   - `VIRExpr` - Base for expressions (produce values)
   - `VIRStmt` - Base for statements (side effects)

2. **VIRExpression.hpp** - All expression types (40+ classes)
   - Literals: Int, Float, Bool, String
   - References: Local, Param, Global
   - Operations: Binary, Unary
   - Type ops: Box, Unbox, Cast, ImplicitCast
   - Calls: Call, CallRuntime, CallIndirect
   - **VIRWrapFunction** - The key innovation for HOFs!
   - Memory: Alloca, Load, Store
   - Control flow: IfExpr
   - Structs: StructNew, StructGet, StructSet
   - Arrays: ArrayNew, ArrayGet, ArraySet
   - Safety: BoundsCheck

3. **VIRStatement.hpp** - All statement types
   - VIRBlock
   - VIRVarDecl
   - VIRReturn
   - VIRIfStmt
   - VIRWhile
   - VIRFor
   - VIRBreak, VIRContinue
   - VIRExprStmt

4. **VIRModule.hpp** - Top-level structures
   - VIRFunction (with VIRParam)
   - VIRStructDecl (with VIRStructField)
   - VIREnumDecl (with VIREnumVariant)
   - VIRModule (compilation unit)

5. **VIRLowering.hpp** - AST → VIR transformation
   - Main `lower()` method
   - Monomorphization support
   - Method call desugaring
   - For loop desugaring
   - String interning
   - Helper utilities

6. **VIRCodegen.hpp** - VIR → LLVM IR
   - RuntimeFunctions (declares all volta_* functions)
   - VIRCodegen (mechanical translation to LLVM)
   - Type lowering
   - Boxing/unboxing helpers
   - Struct type registry

7. **VIRBuilder.hpp** - Convenience builders
   - Factory methods for all node types
   - Simplifies VIR construction
   - Makes lowering code cleaner

### Source Files (7 files)

All source files created with:
- Constructor implementations
- Getter implementations
- Stub methods (to be filled incrementally)
- TODO comments marking what phase implements each feature

1. **VIRNode.cpp** - Base class implementations
2. **VIRExpression.cpp** - All expression constructors (~500 lines)
3. **VIRStatement.cpp** - All statement constructors
4. **VIRModule.cpp** - Module/Function/Struct/Enum implementations
5. **VIRLowering.cpp** - Lowering stubs (to be implemented incrementally)
6. **VIRCodegen.cpp** - Codegen stubs (to be implemented incrementally)
7. **VIRBuilder.cpp** - Builder stubs (to be implemented as needed)

## Compilation Status

- ✅ All files created
- ⏸️ Files have stubs, not full implementations
- ⏸️ Will not compile yet (missing implementations)
- ⏸️ CMakeLists.txt needs updating

## Next Steps (Phase 0, Day 1)

1. **Update CMakeLists.txt**
   - Add VIR source files to build
   - Remove old LLVM codegen files
   - Link against LLVM

2. **Implement minimal VIR nodes**
   - VIRIntLiteral (fully)
   - VIRReturn (fully)
   - VIRFunction (fully)

3. **Wire into main.cpp**
   ```cpp
   // Old: AST → LLVM directly
   // New: AST → VIR → LLVM

   auto virModule = VIRLowering(ast, analyzer).lower();
   VIRCodegen(context, module, analyzer).generate(virModule.get());
   ```

4. **Test with minimal program**
   ```volta
   fn main() -> i32 {
       return 0
   }
   ```

## Key Design Decisions Embedded in Structure

1. **Separation of Concerns**
   - VIRLowering.hpp: AST transformation logic
   - VIRCodegen.hpp: LLVM code generation
   - VIRBuilder.hpp: Convenient construction
   - Clear boundaries between phases

2. **Incremental Implementation**
   - All stubs marked with TODOs
   - Each phase knows what to implement
   - Can test after each addition

3. **Runtime Kept Separate**
   - C runtime library unchanged
   - Language-agnostic
   - Will be called by VIR codegen

4. **Expression/Statement Split**
   - Expressions produce values
   - Statements produce side effects
   - Type checking built into structure

5. **Explicit Operations**
   - VIRBox/VIRUnbox explicitly visible
   - VIRCast vs VIRImplicitCast distinguished
   - VIRWrapFunction strategy explicit

## File Statistics

| Category | Files | Lines (with stubs) |
|----------|-------|-------------------|
| Headers  | 7     | ~1,500            |
| Sources  | 7     | ~800              |
| **Total**| **14**| **~2,300**        |

## Documentation

| Document | Purpose | Status |
|----------|---------|--------|
| vir_design.md | Complete design spec | ✅ Done |
| vir_implementation_plan.md | 14-week incremental plan | ✅ Done |
| vir_file_structure.md | This file | ✅ Done |

## Ready for Phase 0, Day 1

The infrastructure is complete! All files exist, compile targets are defined (once CMakeLists.txt is updated), and the incremental implementation path is clear.

**Next session:** Update build system and get first program compiling through VIR! 🚀
