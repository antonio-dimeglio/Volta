# Future-Proofing Roadmap: Advanced Features & Ecosystem Integration

**Date**: 2025-10-03
**Status**: Long-term Vision (Post-Stabilization)
**Prerequisites**: All P0 bugs fixed, system stable

---

## Introduction: Vision & Philosophy

After stabilization, Volta has a choice: remain a small, academic VM or evolve into a **competitive platform for scientific computing and concurrent programming**. This roadmap chooses the latter.

### Core Philosophy

**Don't Reinvent the Wheel**: The scientific computing ecosystem already has world-class libraries (BLAS, LAPACK, CUDA). Instead of reimplementing matrix multiplication, **wrap** these libraries with Volta's safe, high-level interface.

**Zero-Cost Abstractions**: High-level syntax should compile to the same machine code as hand-optimized C. If Volta's `A @ B` matrix multiplication is slower than calling BLAS directly, we've failed.

**Safety Without Sacrifice**: Provide memory safety, thread safety, and type safety **without** runtime overhead when possible.

---

## Table of Contents

1. [Scientific Computing Integration](#scientific-computing-integration)
2. [Concurrency & Parallelism](#concurrency--parallelism)
3. [Exception Handling System](#exception-handling-system)
4. [Advanced Type System Features](#advanced-type-system-features)
5. [Performance Optimizations](#performance-optimizations)
6. [Developer Experience](#developer-experience)
7. [Implementation Timeline](#implementation-timeline)

---

## Scientific Computing Integration

### The Problem

Python + NumPy dominates scientific computing, but has fundamental issues:
- **Performance**: Python interpreter is slow (~100x slower than C)
- **Memory**: No control over array layout, inefficient for large datasets
- **Concurrency**: Global Interpreter Lock (GIL) prevents multi-threading

Julia addresses performance but has a smaller ecosystem and longer compile times.

### Our Opportunity

Volta can provide:
- **Native performance**: Compile to efficient bytecode + JIT
- **Zero-copy FFI**: Call BLAS/LAPACK directly without marshalling overhead
- **True parallelism**: No GIL, real threads
- **Type safety**: Catch dimension mismatches at compile time

### 🎯 Phase 1: BLAS/LAPACK Integration (Weeks 1-4)

#### **What is BLAS/LAPACK?**

**BLAS** (Basic Linear Algebra Subprograms) provides optimized routines for:
- **Level 1**: Vector operations (dot product, scaling)
- **Level 2**: Matrix-vector operations
- **Level 3**: Matrix-matrix operations (the most important!)

**LAPACK** (Linear Algebra Package) builds on BLAS for:
- Solving linear systems (Ax = b)
- Eigenvalue/eigenvector computation
- Singular value decomposition (SVD)
- Matrix decompositions (QR, LU, Cholesky)

These libraries are **heavily optimized** (assembly, SIMD, cache-aware) and **ubiquitous** (every scientific computing stack uses them).

#### **Design Rationale: Foreign Function Interface (FFI)**

**Why FFI?** We need to call C functions from Volta. Options:
1. **Reimplement in Volta**: Months of work, unlikely to match BLAS performance
2. **Generate C and compile**: Slow compilation, poor error messages
3. **FFI with dynamic loading**: Best of both worlds ✅

**How FFI Works**:
1. **Dynamic library loading**: Use OS facilities (dlopen/LoadLibrary) to load BLAS at runtime
2. **Symbol resolution**: Find function pointers by name ("dgemm", "dgesv", etc.)
3. **Type-safe wrappers**: Wrap raw C function pointers in type-safe Volta API
4. **Automatic marshalling**: Convert Volta types (Matrix, Array) to C pointers

#### **Architecture Overview**

```
┌─────────────────────────────────────────────────┐
│ Volta User Code                                 │
│  A := Matrix.ones(1000, 1000)                   │
│  B := Matrix.rand(1000, 1000)                   │
│  C := A @ B  // Matrix multiplication           │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ Volta Standard Library (volta/numeric.vlt)      │
│  - Operator @ calls BLASBackend.gemm()          │
│  - Matrix type with metadata (rows, cols)       │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ BLAS FFI Layer (C++ in Volta runtime)           │
│  - Load libopenblas.so / libmkl.so              │
│  - Resolve symbols (dgemm, dgesv, etc.)         │
│  - Handle different BLAS implementations        │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│ Native BLAS Library (C/Fortran)                 │
│  - OpenBLAS, Intel MKL, Apple Accelerate        │
│  - Optimized assembly kernels                   │
│  - Multi-threaded implementations               │
└─────────────────────────────────────────────────┘
```

#### **Implementation Details**

**Week 1-2: Build the FFI Layer**

**Goal**: Load native libraries and resolve symbols at runtime.

**Why dynamic loading?**
- **Portability**: Different systems have different BLAS (OpenBLAS on Linux, Accelerate on macOS, MKL on Intel systems)
- **No compile-time dependencies**: Users don't need BLAS installed to compile Volta
- **Runtime selection**: Can benchmark and choose fastest implementation

**Key Design**:
```cpp
namespace volta::ffi {

// Wrapper around OS library loading (dlopen/LoadLibrary)
class NativeLibrary {
    void* handle_;  // OS handle to loaded library
    std::string path_;

public:
    // Load library from filesystem
    static std::shared_ptr<NativeLibrary> load(const std::string& path) {
        // Platform-specific loading:
        #ifdef _WIN32
            void* handle = LoadLibraryA(path.c_str());  // Windows
        #else
            void* handle = dlopen(path.c_str(), RTLD_LAZY);  // POSIX
        #endif

        if (!handle) {
            // Provide helpful error (e.g., "libopenblas.so not found")
            throw std::runtime_error("Failed to load " + path + ": " + getError());
        }

        auto lib = std::make_shared<NativeLibrary>();
        lib->handle_ = handle;
        lib->path_ = path;
        return lib;
    }

    // Get function pointer by name
    void* getSymbol(const std::string& name) {
        #ifdef _WIN32
            return GetProcAddress((HMODULE)handle_, name.c_str());
        #else
            return dlsym(handle_, name.c_str());
        #endif
    }

    // Type-safe function call wrapper (uses templates)
    template<typename ReturnType, typename... Args>
    ReturnType call(const std::string& funcName, Args... args) {
        // Get function pointer
        auto* funcPtr = reinterpret_cast<ReturnType(*)(Args...)>(
            getSymbol(funcName)
        );

        if (!funcPtr) {
            throw std::runtime_error("Function not found: " + funcName);
        }

        // Call it
        return funcPtr(args...);
    }
};

} // namespace volta::ffi
```

**Why this design?**
- **RAII**: Library handle cleaned up automatically
- **Type safety**: Template `call<>` checks argument types at compile time
- **Error handling**: Clear messages if library/symbol not found
- **Cross-platform**: Same API works on Windows, Linux, macOS

**Week 2-3: Wrap BLAS Functions**

**Goal**: Provide a clean C++ API for common BLAS operations.

**Challenge**: BLAS has confusing names (dgemm, sgemm, zgemm for different types) and column-major layout (Fortran convention).

**Solution**: Hide complexity behind clear API.

**Key Design**:
```cpp
namespace volta::ffi {

class BLASBackend {
    static std::shared_ptr<NativeLibrary> library_;

public:
    // Initialize with specific BLAS implementation
    static void initialize(Implementation impl = Implementation::OpenBLAS) {
        // Load appropriate library
        switch (impl) {
            case Implementation::OpenBLAS:
                library_ = NativeLibrary::load("libopenblas.so");  // Linux
                break;
            case Implementation::MKL:
                library_ = NativeLibrary::load("libmkl_rt.so");    // Intel
                break;
            case Implementation::Accelerate:
                library_ = NativeLibrary::load("/System/Library/Frameworks/"
                                              "Accelerate.framework/Accelerate");  // macOS
                break;
        }
    }

    // Matrix-matrix multiply: C = alpha * A * B + beta * C
    // This is the MOST IMPORTANT function in all of scientific computing!
    static void gemm(
        size_t m, size_t n, size_t k,      // Matrix dimensions
        double alpha,                       // Scalar multiplier
        const double* A,                    // Input matrix A (m x k)
        const double* B,                    // Input matrix B (k x n)
        double beta,                        // Scalar multiplier for C
        double* C                           // Output matrix C (m x n)
    ) {
        // BLAS uses column-major layout (Fortran convention)
        // We use row-major (C convention), so need to transpose

        // Trick: (AB)^T = B^T A^T, so we swap A and B
        library_->call<void>(
            "cblas_dgemm",           // Function name
            CblasRowMajor,           // We use row-major layout
            CblasNoTrans,            // Don't transpose A
            CblasNoTrans,            // Don't transpose B
            m, n, k,                 // Dimensions
            alpha,                   // Alpha scalar
            A, k,                    // A matrix and leading dimension
            B, n,                    // B matrix and leading dimension
            beta,                    // Beta scalar
            C, n                     // C matrix and leading dimension
        );
    }

    // Vector-vector dot product: result = x^T * y
    static double dot(size_t n, const double* x, const double* y) {
        return library_->call<double>(
            "cblas_ddot",   // Function name
            n,              // Vector length
            x, 1,           // x vector and stride
            y, 1            // y vector and stride
        );
    }

    // More functions: axpy, scal, gemv, etc.
};

} // namespace volta::ffi
```

**Why this design?**
- **Hide Fortran layout**: Users don't need to know about column-major
- **Multiple implementations**: Can switch BLAS at runtime
- **Type overloading**: Will add templates for float/double/complex later
- **Performance**: Zero overhead - direct function call

**Week 3-4: Integrate with Volta Language**

**Goal**: Make BLAS accessible from Volta code with clean syntax.

**Challenge**: How do we represent matrices in Volta's type system and bytecode?

**Solution**: Add Matrix type to semantic analyzer and generate efficient bytecode.

**Implementation Steps**:

1. **Add Matrix type to semantic analyzer**:
   ```cpp
   // In semantic/Type.hpp
   class MatrixType : public Type {
       std::shared_ptr<Type> elementType_;  // float or double

   public:
       MatrixType(std::shared_ptr<Type> elemType)
           : elementType_(std::move(elemType)) {}

       Kind kind() const override { return Kind::Matrix; }
       std::string toString() const override {
           return "Matrix[" + elementType_->toString() + "]";
       }

       std::shared_ptr<Type> elementType() const { return elementType_; }
   };
   ```

2. **Add operator overloading for `@`**:
   ```cpp
   // In semantic/SemanticAnalyzer.cpp
   std::shared_ptr<Type> SemanticAnalyzer::analyzeBinaryExpression(
       const ast::BinaryExpression* expr
   ) {
       // ...existing code...

       if (expr->op == ast::BinaryExpression::Operator::MatMul) {  // @
           auto leftType = analyzeExpression(expr->left.get());
           auto rightType = analyzeExpression(expr->right.get());

           // Check both are matrices
           auto* leftMatrix = dynamic_cast<const MatrixType*>(leftType.get());
           auto* rightMatrix = dynamic_cast<const MatrixType*>(rightType.get());

           if (!leftMatrix || !rightMatrix) {
               error("@ operator requires Matrix types");
               return std::make_shared<UnknownType>();
           }

           // Check dimensions match (left.cols == right.rows)
           // (This requires runtime checks or shape tracking in types)

           // Result is Matrix with left.rows x right.cols
           return leftType;  // Simplified - same type
       }
   }
   ```

3. **Generate bytecode for matrix operations**:
   ```cpp
   // In bytecode/BytecodeCompiler.cpp
   void BytecodeCompiler::compileBinaryExpression(
       const ir::BinaryExpression* expr, Chunk& chunk
   ) {
       // ...existing code...

       if (expr->op == ir::BinaryExpression::Operator::MatMul) {
           // Load operands
           emitLoadValue(expr->left(), chunk);
           emitLoadValue(expr->right(), chunk);

           // Call BLAS gemm via new opcode
           chunk.emitOpcode(Opcode::MatMul);
       }
   }
   ```

4. **Add VM opcode that calls BLAS**:
   ```cpp
   // In vm/VM.cpp
   void VM::execMatMul() {
       Value right = pop();
       Value left = pop();

       // Extract matrix objects
       MatrixObject* A = static_cast<MatrixObject*>(left.asObject);
       MatrixObject* B = static_cast<MatrixObject*>(right.asObject);

       // Allocate result matrix
       MatrixObject* C = gc_->allocateMatrix(A->rows, B->cols);

       // Call BLAS
       ffi::BLASBackend::gemm(
           A->rows, B->cols, A->cols,  // Dimensions (m, n, k)
           1.0,                         // alpha = 1
           A->data, B->data,            // Input matrices
           0.0,                         // beta = 0 (don't add to C)
           C->data                      // Output matrix
       );

       push(Value::makeObject(C));
   }
   ```

**Result**: Users can write:
```rust
A := Matrix.ones(1000, 1000)
B := Matrix.rand(1000, 1000)
C := A @ B  // Calls optimized BLAS under the hood!
```

**Performance**: This should be **as fast as calling BLAS from C**, with ~0% overhead.

---

### 🎯 Phase 2: NumPy-Style API (Weeks 5-8)

#### **The Problem**

BLAS gives us fast matrix multiplication, but NumPy provides a **rich ecosystem**:
- Array creation (`zeros`, `ones`, `arange`, `linspace`)
- Reshaping and transposition
- Reduction operations (`sum`, `mean`, `max`, `min`)
- Broadcasting (automatic shape alignment)
- Universal functions (element-wise operations)

#### **Design Philosophy**

**Don't copy NumPy's mistakes**: NumPy has grown organically over 20 years and has inconsistencies. We can learn from it and design a cleaner API.

**Key decisions**:
1. **Immutable by default**: NumPy arrays are mutable, leading to subtle bugs. Volta arrays should be immutable unless explicitly marked `mut`.
2. **Type-safe shapes**: NumPy doesn't track shapes at type level. We can use generics for compile-time shape checking (later).
3. **Explicit broadcasting**: Make it clear when broadcasting happens.

#### **Implementation**

**Volta Standard Library Module**:
```rust
// File: stdlib/numeric.vlt
module numeric

// ============================================================
// Array Creation Functions
// ============================================================

// Create array of zeros
// Example: zeros((3, 4)) creates 3x4 matrix of zeros
fn zeros(shape: Tuple[int, ...]) -> Array[float] {
    // Calculate total size from shape
    size: int = 1
    for dim in shape {
        size *= dim
    }

    // Allocate array
    arr := Array[float].allocate(size)

    // Initialize to zero (already done by GC, but explicit)
    for i in 0..size {
        arr[i] = 0.0
    }

    // Reshape to desired shape
    return arr.reshape(shape)
}

// Create array of ones
fn ones(shape: Tuple[int, ...]) -> Array[float] {
    arr := zeros(shape)
    for i in 0..arr.length {
        arr[i] = 1.0
    }
    return arr
}

// Create array with evenly spaced values
// Example: arange(0, 10, 2) -> [0, 2, 4, 6, 8]
fn arange(start: int, stop: int, step: int = 1) -> Array[int] {
    size := (stop - start + step - 1) / step  // Ceiling division
    arr := Array[int].allocate(size)

    value := start
    for i in 0..size {
        arr[i] = value
        value += step
    }

    return arr
}

// Create array with linearly spaced values
// Example: linspace(0.0, 1.0, 5) -> [0.0, 0.25, 0.5, 0.75, 1.0]
fn linspace(start: float, stop: float, num: int = 50) -> Array[float] {
    arr := Array[float].allocate(num)
    step := (stop - start) / (num - 1)

    for i in 0..num {
        arr[i] = start + i * step
    }

    return arr
}

// ============================================================
// Array Manipulation
// ============================================================

// Reshape array to new shape
// Example: [1,2,3,4,5,6].reshape((2,3)) -> [[1,2,3], [4,5,6]]
fn reshape[T](arr: Array[T], shape: Tuple[int, ...]) -> Array[T] {
    // Calculate total size from new shape
    newSize: int = 1
    for dim in shape {
        newSize *= dim
    }

    // Verify size matches
    if newSize != arr.length {
        throw ValueError("Cannot reshape array of size " + arr.length +
                        " into shape with size " + newSize)
    }

    // Create new array with same data but different shape metadata
    result := arr.copy()  // Copy data (or use view for zero-copy)
    result.shape = shape
    return result
}

// Transpose matrix
fn transpose[T](arr: Array[T]) -> Array[T] {
    // Only works for 2D arrays (matrices)
    if arr.ndim != 2 {
        throw ValueError("transpose requires 2D array")
    }

    rows := arr.shape[0]
    cols := arr.shape[1]

    result := Array[T].allocate(rows * cols)
    result.shape = (cols, rows)

    // Transpose: result[i, j] = arr[j, i]
    for i in 0..rows {
        for j in 0..cols {
            result[j * rows + i] = arr[i * cols + j]
        }
    }

    return result
}

// ============================================================
// Reduction Operations
// ============================================================

// Sum all elements or along axis
fn sum[T: Numeric](arr: Array[T], axis: Option[int] = None) -> T | Array[T] {
    match axis {
        None => {
            // Sum all elements
            result := T.zero()
            for elem in arr {
                result += elem
            }
            return result
        },
        Some(ax) => {
            // Sum along specific axis
            // ... more complex logic for multi-dimensional arrays
        }
    }
}

// Mean (average)
fn mean(arr: Array[float], axis: Option[int] = None) -> float | Array[float] {
    match axis {
        None => return sum(arr, None) / arr.length,
        Some(ax) => {
            sums := sum(arr, Some(ax))
            return sums / arr.shape[ax]
        }
    }
}

// ============================================================
// Broadcasting
// ============================================================

// Add two arrays with broadcasting
// Example: [1,2,3] + [10] -> [11,12,13]
fn broadcast_add[T: Numeric](a: Array[T], b: Array[T]) -> Array[T] {
    // Determine broadcasted shape
    result_shape := compute_broadcast_shape(a.shape, b.shape)

    // Allocate result
    result := Array[T].allocate(shape_size(result_shape))
    result.shape = result_shape

    // Perform broadcasted addition
    // (This requires complex indexing logic - see NumPy's implementation)
    for i in 0..result.length {
        a_idx := broadcast_index(i, result_shape, a.shape)
        b_idx := broadcast_index(i, result_shape, b.shape)
        result[i] = a[a_idx] + b[b_idx]
    }

    return result
}

// Helper: compute shape after broadcasting
fn compute_broadcast_shape(shape1: Tuple[int, ...],
                           shape2: Tuple[int, ...]) -> Tuple[int, ...] {
    // Broadcasting rules:
    // 1. If shapes have different lengths, pad with 1s on left
    // 2. For each dimension, result is max(dim1, dim2)
    // 3. Dimensions must be either equal or one of them must be 1

    // Example: (3, 1, 4) + (4,) -> (3, 1, 4) + (1, 1, 4) -> (3, 1, 4)

    maxLen := max(shape1.length, shape2.length)
    result := Tuple[int].empty(maxLen)

    // Iterate from right to left (broadcast from innermost dimension)
    for i in 0..maxLen {
        dim1 := i < shape1.length ? shape1[shape1.length - 1 - i] : 1
        dim2 := i < shape2.length ? shape2[shape2.length - 1 - i] : 1

        if dim1 != dim2 and dim1 != 1 and dim2 != 1 {
            throw ValueError("Shapes not compatible for broadcasting")
        }

        result[maxLen - 1 - i] = max(dim1, dim2)
    }

    return result
}

// ============================================================
// Universal Functions (ufuncs)
// ============================================================

// Apply function element-wise (vectorized)
// Uses SIMD when possible (see JIT section)
fn sin(arr: Array[float]) -> Array[float] {
    result := Array[float].allocate(arr.length)

    for i in 0..arr.length {
        result[i] = math.sin(arr[i])  // Calls C math library
    }

    return result
}

fn cos(arr: Array[float]) -> Array[float] { /* similar */ }
fn exp(arr: Array[float]) -> Array[float] { /* similar */ }
fn log(arr: Array[float]) -> Array[float] { /* similar */ }

// ============================================================
// Example Usage
// ============================================================

fn example() {
    // Create matrices
    A := ones((3, 3))
    B := arange(0, 9).reshape((3, 3))

    // Matrix multiplication (calls BLAS)
    C := A @ B

    // Element-wise operations
    D := sin(C)

    // Reductions
    total := sum(D)
    avg := mean(D)

    print("Total: " + total)
    print("Average: " + avg)
}
```

**Why This Design?**
- **Familiar API**: Scientists coming from NumPy can be productive immediately
- **Type-safe**: Volta's type system catches errors NumPy would miss
- **Performance**: Backed by BLAS for matrix operations, SIMD for element-wise
- **Immutable by default**: Safer for concurrent programs

---

### 🎯 Phase 3: GPU Acceleration (Weeks 9-16)

#### **The Problem**

Modern scientific computing is **GPU-bound**. A single NVIDIA A100 GPU can perform **~300 TFLOPS** (trillion floating-point operations per second), while a high-end CPU does ~1 TFLOPS. For large matrix multiplications, GPU is **100-300x faster**.

But GPU programming is **hard**:
- CUDA code is verbose and error-prone
- Memory management (CPU ↔ GPU transfers)
- Kernel optimization requires deep expertise

#### **Our Opportunity**

**Automatic dispatch**: Small arrays use CPU, large arrays use GPU automatically. User doesn't need to know CUDA!

**Implementation Strategy**

**Week 9-10: GPU Device Abstraction**

**Goal**: Create a cross-platform GPU interface supporting CUDA (NVIDIA), ROCm (AMD), Metal (Apple), and OpenCL (portable).

**Design**:
```cpp
namespace volta::gpu {

// Represents a GPU device
class GPUDevice {
    enum class Backend { CUDA, ROCm, Metal, OpenCL };

    Backend backend_;
    int deviceId_;
    void* deviceHandle_;  // Backend-specific handle

public:
    // Get current GPU (usually device 0)
    static GPUDevice& current() {
        static GPUDevice device;
        return device;
    }

    // List available GPUs
    static std::vector<GPUDevice> available() {
        std::vector<GPUDevice> devices;

        // Query CUDA devices
        #ifdef VOLTA_WITH_CUDA
        int count;
        cudaGetDeviceCount(&count);
        for (int i = 0; i < count; i++) {
            devices.emplace_back(Backend::CUDA, i);
        }
        #endif

        // Query ROCm devices
        #ifdef VOLTA_WITH_ROCM
        // Similar for ROCm
        #endif

        return devices;
    }

    // Allocate GPU memory
    void* allocate(size_t bytes) {
        void* ptr = nullptr;

        switch (backend_) {
            case Backend::CUDA:
                cudaMalloc(&ptr, bytes);
                break;
            case Backend::ROCm:
                hipMalloc(&ptr, bytes);  // ROCm's CUDA-compatible API
                break;
            // ... other backends
        }

        return ptr;
    }

    // Free GPU memory
    void free(void* ptr) {
        switch (backend_) {
            case Backend::CUDA: cudaFree(ptr); break;
            case Backend::ROCm: hipFree(ptr); break;
            // ... other backends
        }
    }

    // Copy CPU -> GPU
    void copyToDevice(void* dst, const void* src, size_t bytes) {
        switch (backend_) {
            case Backend::CUDA:
                cudaMemcpy(dst, src, bytes, cudaMemcpyHostToDevice);
                break;
            // ... other backends
        }
    }

    // Copy GPU -> CPU
    void copyToHost(void* dst, const void* src, size_t bytes) {
        switch (backend_) {
            case Backend::CUDA:
                cudaMemcpy(dst, src, bytes, cudaMemcpyDeviceToHost);
                break;
            // ... other backends
        }
    }

    // Launch kernel (simplified - real version more complex)
    void launch(const std::string& kernelName,
                dim3 grid, dim3 block,
                void** args, size_t sharedMem = 0) {
        // Launch kernel with given grid/block dimensions
        // This requires compiling CUDA kernels at runtime or ahead-of-time
    }
};

} // namespace volta::gpu
```

**Why This Design?**
- **Unified API**: Same code works on NVIDIA, AMD, Apple GPUs
- **Runtime selection**: Choose GPU based on what's available
- **Memory management**: RAII wrappers prevent leaks

**Week 11-12: Automatic Dispatch**

**Goal**: Decide CPU vs GPU automatically based on array size.

**Heuristic**:
- **Small arrays** (< 1M elements): Use CPU (overhead of GPU transfer not worth it)
- **Large arrays** (> 1M elements): Use GPU (transfer cost amortized)

**Design**:
```cpp
class ArrayDispatcher {
    static constexpr size_t GPU_THRESHOLD = 1000000;  // 1M elements

    static ArrayObject* add(ArrayObject* a, ArrayObject* b) {
        size_t elements = a->length;

        if (shouldUseGPU(elements)) {
            return gpuAdd(a, b);
        } else {
            return cpuAdd(a, b);
        }
    }

private:
    static bool shouldUseGPU(size_t elements) {
        // Use GPU if:
        // 1. Array is large enough (amortize transfer cost)
        // 2. GPU is available
        // 3. Operation is GPU-friendly (compute-bound, not memory-bound)

        if (elements < GPU_THRESHOLD) return false;

        if (GPUDevice::available().empty()) return false;

        return true;
    }

    static ArrayObject* gpuAdd(ArrayObject* a, ArrayObject* b) {
        auto& gpu = GPUDevice::current();

        // Allocate GPU memory
        void* d_a = gpu.allocate(a->length * sizeof(double));
        void* d_b = gpu.allocate(b->length * sizeof(double));
        void* d_c = gpu.allocate(a->length * sizeof(double));

        // Copy to GPU
        gpu.copyToDevice(d_a, a->data, a->length * sizeof(double));
        gpu.copyToDevice(d_b, b->data, b->length * sizeof(double));

        // Launch kernel (element-wise addition)
        dim3 block(256);  // 256 threads per block
        dim3 grid((a->length + 255) / 256);  // Enough blocks to cover array

        void* args[] = {&d_a, &d_b, &d_c, &a->length};
        gpu.launch("vectorAdd", grid, block, args);

        // Allocate result on CPU
        ArrayObject* result = gc_->allocateArray(a->length);

        // Copy result back
        gpu.copyToHost(result->data, d_c, a->length * sizeof(double));

        // Free GPU memory
        gpu.free(d_a);
        gpu.free(d_b);
        gpu.free(d_c);

        return result;
    }

    static ArrayObject* cpuAdd(ArrayObject* a, ArrayObject* b) {
        // Simple CPU version
        ArrayObject* result = gc_->allocateArray(a->length);
        for (size_t i = 0; i < a->length; i++) {
            result->elements[i] = Value::makeFloat(
                a->elements[i].asFloat + b->elements[i].asFloat
            );
        }
        return result;
    }
};
```

**Result**: User writes `A + B`, and Volta automatically uses GPU for large arrays! **Zero CUDA code**.

**Week 13-16: Matrix Multiplication on GPU**

**Why special treatment?** Matrix multiplication (gemm) is THE most important operation in deep learning and scientific computing. It's also highly parallelizable.

**Use cuBLAS**: NVIDIA's GPU-accelerated BLAS library. Orders of magnitude faster than CPU for large matrices.

**Design**:
```cpp
class BLASBackend {
    static cublasHandle_t cublasHandle_;

public:
    static void gemm_gpu(size_t m, size_t n, size_t k,
                        double alpha,
                        const double* A,  // CPU pointers
                        const double* B,
                        double beta,
                        double* C) {
        auto& gpu = GPUDevice::current();

        // Allocate GPU memory
        double* d_A = static_cast<double*>(gpu.allocate(m * k * sizeof(double)));
        double* d_B = static_cast<double*>(gpu.allocate(k * n * sizeof(double)));
        double* d_C = static_cast<double*>(gpu.allocate(m * n * sizeof(double)));

        // Copy matrices to GPU
        gpu.copyToDevice(d_A, A, m * k * sizeof(double));
        gpu.copyToDevice(d_B, B, k * n * sizeof(double));

        // Call cuBLAS gemm
        cublasDgemm(
            cublasHandle_,
            CUBLAS_OP_N, CUBLAS_OP_N,  // No transpose
            m, n, k,
            &alpha,
            d_A, m,  // Leading dimension
            d_B, k,
            &beta,
            d_C, m
        );

        // Copy result back
        gpu.copyToHost(C, d_C, m * n * sizeof(double));

        // Free GPU memory
        gpu.free(d_A);
        gpu.free(d_B);
        gpu.free(d_C);
    }
};
```

**Performance**: For 10,000 x 10,000 matrix multiplication:
- **CPU BLAS**: ~10 seconds
- **GPU cuBLAS**: ~0.1 seconds
- **100x speedup!**

---

## Concurrency & Parallelism

### The Problem

Modern CPUs have many cores (16-64 is common), but single-threaded code can't use them. Python's GIL prevents true parallelism. We need:
- **Async I/O**: Don't block on network/disk
- **Thread parallelism**: Utilize all cores
- **Data parallelism**: Automatically parallelize loops

### Design Philosophy

**Multiple concurrency models** for different use cases:
1. **Async/await**: For I/O-bound tasks (web servers, database queries)
2. **OS threads**: For CPU-bound parallel tasks
3. **Data parallelism**: For array operations (automatic)

### 🎯 Phase 1: Green Threads & Async/Await (Weeks 1-6)

#### **What are Green Threads?**

**OS threads** are managed by the operating system. They're:
- **Heavy**: 1-2 MB stack per thread
- **Slow context switch**: Kernel involvement
- **Limited**: Can't have 10,000 OS threads

**Green threads** (aka user-space threads, coroutines) are managed by the runtime:
- **Lightweight**: Few KB per thread
- **Fast context switch**: User-space only
- **Scalable**: Can have millions

#### **Design: M:N Threading**

**M green threads** run on **N OS threads**, where M >> N.

**Example**: 10,000 green threads on 8 OS threads.

**How it works**:
1. **Scheduler**: Assigns green threads to OS threads
2. **Work stealing**: Idle OS threads steal work from busy ones
3. **Yielding**: Green thread voluntarily gives up CPU

**Implementation**:
```cpp
namespace volta::runtime {

// Green thread (stackful coroutine)
class GreenThread {
    uint64_t id_;
    void* stack_;        // Separate 2MB stack
    Context context_;    // Saved CPU registers (RIP, RSP, RBP, etc.)
    ThreadState state_;  // Running, Suspended, Completed

public:
    // Spawn new green thread
    static GreenThread* spawn(std::function<void()> func) {
        auto* thread = new GreenThread();
        thread->id_ = nextThreadId();

        // Allocate stack (2MB)
        thread->stack_ = mmap(nullptr, 2 * 1024 * 1024,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS,
                             -1, 0);

        // Initialize context to run func
        initializeContext(&thread->context_, thread->stack_, func);

        thread->state_ = ThreadState::Ready;
        return thread;
    }

    // Yield CPU to scheduler
    void yield() {
        // Save current context
        saveContext(&context_);

        // Switch to scheduler
        Scheduler::current()->switchToScheduler();
    }

    // Resume execution
    void resume() {
        state_ = ThreadState::Running;
        restoreContext(&context_);  // Restore registers and jump
    }
};

// Work-stealing scheduler
class Scheduler {
    std::vector<WorkQueue> queues_;     // One queue per OS thread
    std::vector<std::thread> workers_;  // OS thread pool

public:
    Scheduler(size_t numWorkers) {
        queues_.resize(numWorkers);

        // Spawn OS threads
        for (size_t i = 0; i < numWorkers; i++) {
            workers_.emplace_back([this, i]() {
                this->workerLoop(i);
            });
        }
    }

    // Schedule green thread for execution
    void schedule(GreenThread* thread) {
        // Round-robin: assign to least-loaded queue
        size_t minIdx = 0;
        size_t minSize = queues_[0].size();

        for (size_t i = 1; i < queues_.size(); i++) {
            if (queues_[i].size() < minSize) {
                minSize = queues_[i].size();
                minIdx = i;
            }
        }

        queues_[minIdx].push(thread);
    }

private:
    // OS thread event loop
    void workerLoop(size_t workerId) {
        while (true) {
            // Try to get task from own queue
            GreenThread* thread = queues_[workerId].tryPop();

            // If empty, steal from other queues
            if (!thread) {
                thread = steal(workerId);
            }

            // No work available - sleep briefly
            if (!thread) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }

            // Execute green thread
            thread->resume();

            // If thread yielded or completed, continue loop
        }
    }

    // Work stealing: steal task from another queue
    GreenThread* steal(size_t thiefId) {
        // Try to steal from random victim
        for (size_t i = 0; i < queues_.size(); i++) {
            if (i == thiefId) continue;

            GreenThread* thread = queues_[i].trySteal();
            if (thread) return thread;
        }

        return nullptr;
    }
};

} // namespace volta::runtime
```

**Why This Design?**
- **Scalability**: Can have millions of green threads
- **Load balancing**: Work stealing prevents idle CPUs
- **Performance**: Context switch is just saving/restoring registers (< 100 ns)

**Volta Language Integration**:
```rust
// Async function
async fn fetch_url(url: str) -> str {
    // This function can be suspended
    response := await http.get(url)  // Yields while waiting for I/O
    return await response.text()     // Yields again
}

// Concurrent execution
async fn fetch_multiple(urls: Array[str]) -> Array[str] {
    // Spawn concurrent tasks (green threads)
    futures := urls.map(|url| async { fetch_url(url) })

    // Wait for all to complete
    return await Future.all(futures)
}

// Entry point
fn main() {
    runtime := AsyncRuntime.new()

    urls := ["http://example.com", "http://google.com", "http://github.com"]
    results := runtime.block_on(fetch_multiple(urls))

    for result in results {
        print(result)
    }
}
```

**How it works**:
1. `async fn` returns a `Future` (promise of value)
2. `await` suspends green thread until Future completes
3. Scheduler runs other green threads while waiting
4. When I/O completes, thread resumes

**Performance**: Can handle 100,000+ concurrent connections on a laptop!

---

## Exception Handling System

### The Problem

Errors happen. Current options:
- **Return codes**: Error-prone (easy to forget to check)
- **NULL returns**: No context about error
- **Assertions**: Crash the program

We need structured exception handling with:
- **Stack traces**: Where did error occur?
- **Type safety**: Catch specific error types
- **Zero cost when not thrown**: No overhead in happy path

### Design Philosophy

**Two-tier error handling**:
1. **Exceptions**: For unexpected errors (network timeout, file not found)
2. **Result<T, E>**: For expected errors (division by zero, parse failures)

**Why both?** Exceptions for rare errors (throw across many stack frames), Result for common errors (explicit handling).

### Implementation

**Week 1-2: Exception Infrastructure**

**Goal**: Capture stack trace, unwind stack, find handler.

**Key Design**:
```cpp
namespace volta::exceptions {

// Exception object
class Exception {
    std::string message_;
    std::vector<StackFrame> stackTrace_;  // Captured at throw site
    std::shared_ptr<Exception> cause_;     // Exception chaining

public:
    Exception(std::string msg) : message_(std::move(msg)) {
        captureStackTrace();  // Walk stack and record frames
    }

private:
    void captureStackTrace() {
        // Use platform APIs to walk stack
        #ifdef __linux__
            // Use backtrace() from execinfo.h
            void* buffer[128];
            int frames = backtrace(buffer, 128);

            char** symbols = backtrace_symbols(buffer, frames);
            for (int i = 0; i < frames; i++) {
                stackTrace_.push_back(parseSymbol(symbols[i]));
            }
            free(symbols);
        #endif

        #ifdef __APPLE__
            // Similar with backtrace_symbols
        #endif

        #ifdef _WIN32
            // Use CaptureStackBackTrace
        #endif
    }
};

// Unwinding: find catch handler for exception
class UnwindManager {
    struct LandingPad {
        size_t instructionOffset;  // Where try block starts
        size_t handlerOffset;      // Where catch handler is
        std::vector<std::string> catchTypes;  // Which exceptions to catch
    };

    // Map function name -> landing pads
    std::unordered_map<std::string, std::vector<LandingPad>> landingPads_;

public:
    // Register catch handler (done by compiler)
    void registerLandingPad(const std::string& funcName, LandingPad pad) {
        landingPads_[funcName].push_back(pad);
    }

    // Find handler when exception is thrown
    Option<LandingPad> findHandler(
        const std::string& funcName,
        size_t throwOffset,
        const std::string& exceptionType
    ) {
        auto it = landingPads_.find(funcName);
        if (it == landingPads_.end()) return None;

        // Find landing pad that contains throw site
        for (const auto& pad : it->second) {
            if (throwOffset >= pad.instructionOffset &&
                throwOffset < pad.handlerOffset) {

                // Check if it catches this exception type
                for (const auto& catchType : pad.catchTypes) {
                    if (catchType == exceptionType || catchType == "Exception") {
                        return Some(pad);
                    }
                }
            }
        }

        return None;
    }
};

} // namespace volta::exceptions
```

**Volta Language Integration**:
```rust
// Define custom exception types
exception NetworkError {
    message: str,
    code: int
}

exception FileNotFound {
    path: str
}

// Function that can throw
fn risky_operation() throws NetworkError {
    if connection_failed {
        throw NetworkError {
            message: "Connection timeout",
            code: 504
        }
    }
}

// Catching exceptions
fn handle_errors() {
    try {
        risky_operation()
        other_risky_operation()
    } catch e: NetworkError {
        // Handle network errors
        print("Network error: " + e.message + " (code: " + e.code + ")")
        retry()
    } catch e: FileNotFound {
        // Handle file errors
        print("File not found: " + e.path)
    } finally {
        // Always runs (even if exception thrown)
        cleanup()
    }
}
```

**How it works**:
1. `throw` captures stack trace and creates exception object
2. VM unwinds stack, looking for catch handler
3. For each function, check if it has landing pad (try block)
4. If catch type matches, jump to handler
5. If no handler found, propagate to caller
6. `finally` blocks run during unwinding

**Week 3-4: Result<T, E> Type**

**Goal**: Explicit error handling for expected failures.

**Design**:
```rust
// Result type (algebraic data type)
enum Result[T, E] {
    Ok(T),   // Success case
    Err(E)   // Error case
}

// Function that returns Result
fn divide(a: int, b: int) -> Result[int, str] {
    if b == 0 {
        return Err("Division by zero")
    }
    return Ok(a / b)
}

// Pattern matching on Result
fn use_result() {
    result := divide(10, 2)

    match result {
        Ok(val) => print("Result: " + val),
        Err(msg) => print("Error: " + msg)
    }
}

// Question mark operator (syntactic sugar)
fn chained_operations() -> Result[int, str] {
    a := divide(10, 2)?  // If Err, return early
    b := divide(a, 3)?   // Otherwise, unwrap Ok value
    return Ok(b)
}

// Unwrap with default value
fn with_default() {
    result := divide(10, 0)
    value := result.unwrap_or(0)  // 0 if Err
}
```

**Why Result<T, E>?**
- **Explicit**: Can't forget to check errors (compiler enforces)
- **Type-safe**: Error type is known
- **Zero cost**: No overhead (just a tagged union)
- **Composable**: Can chain operations with `?`

---

## Implementation Timeline

### **Year 1: Foundation**

**Q1** (Weeks 1-13):
1. **Week 1-2**: Fix all P0 bugs (critical stabilization)
2. **Week 3-4**: Complete test suite, add integration tests
3. **Week 5-8**: BLAS/LAPACK integration (FFI layer + wrappers)
4. **Week 9-13**: Green threads & async/await (concurrency basics)

**Q2** (Weeks 14-26):
1. **Week 14-17**: NumPy-style API (array creation, manipulation)
2. **Week 18-21**: Exception handling (try/catch, stack traces)
3. **Week 22-26**: Generics (type parameters, constraints)

**Q3** (Weeks 27-39):
1. **Week 27-32**: JIT compilation tier 1 (baseline compiler)
2. **Week 33-36**: Synchronization primitives (mutex, channels)
3. **Week 37-39**: Traits/interfaces (type classes)

**Q4** (Weeks 40-52):
1. **Week 40-47**: GPU acceleration (CUDA/ROCm, automatic dispatch)
2. **Week 48-52**: JIT optimization tier 2 (LLVM backend)

### **Year 2: Ecosystem & Optimization**

**Q1**: SIMD auto-vectorization, concurrent GC, REPL
**Q2**: Package manager, debugger integration, LSP
**Q3**: Advanced optimizations (inline caching, escape analysis)
**Q4**: Production hardening, benchmarks, documentation

---

## Success Metrics

**End of Year 1**:
- ✅ Scientific computing performance: 80% of NumPy/BLAS
- ✅ Concurrent programs: 90% parallel efficiency on 8 cores
- ✅ JIT speedup: 5-10x over interpreter
- ✅ Community: 100+ packages, 1000+ GitHub stars

**End of Year 2**:
- ✅ GPU performance: Competitive with CuPy/PyTorch
- ✅ Adoption: 10+ production deployments
- ✅ Ecosystem: Package manager, LSP, REPL, debugger
- ✅ Performance: Can replace Python for most scientific computing

---

## Conclusion

This roadmap transforms Volta from a **teaching VM** into a **production-ready scientific computing platform**. The key is:

1. **Leverage existing infrastructure**: BLAS, CUDA, LLVM
2. **Start simple**: BLAS integration before GPU
3. **Iterate based on feedback**: Community drives priorities
4. **Maintain safety**: Type safety, memory safety, thread safety
5. **Ship incrementally**: Release early, release often

**The opportunity**: There's a gap in the market for a language that combines:
- **Python's ease of use** (clean syntax, REPL, batteries-included)
- **Julia's performance** (JIT, native code generation)
- **Rust's safety** (ownership, type safety, no GC pauses)

Volta can fill that gap for scientific computing, becoming the **go-to language for high-performance numerical work** while maintaining safety and productivity.

**Next Steps**:
1. Complete stabilization (fix remaining bugs)
2. Implement BLAS integration prototype
3. Benchmark against NumPy
4. Gather community feedback
5. Iterate!
