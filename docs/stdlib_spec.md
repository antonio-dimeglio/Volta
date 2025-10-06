# Volta Standard Library Specification

**Version:** 0.1.0
**Status:** Design Document
**Target:** Scientific Computing & Numerical Analysis

---

## Table of Contents

1. [Philosophy](#philosophy)
2. [Architecture](#architecture)
3. [Core Modules](#core-modules)
4. [Scientific Computing Modules](#scientific-computing-modules)
5. [Implementation Strategy](#implementation-strategy)
6. [Module Details](#module-details)
7. [Usage Examples](#usage-examples)

---

## Philosophy

The Volta standard library is designed around these principles:

1. **Performance First**: Critical operations use optimized C++/BLAS/LAPACK
2. **Pythonic API**: Familiar to scientific Python users (NumPy/SciPy style)
3. **Type Safety**: Leverages Volta's static typing
4. **Immutability**: Default immutable operations (functional style)
5. **GPU-Ready**: First-class GPU acceleration support
6. **Composable**: Small, focused modules that work together

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    User Code                            │
│  import math, array, linalg, gpu                       │
└───────────────────────┬─────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────┐
│              Volta Standard Library                     │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐│
│  │ Pure Volta  │  │ Volta+C++    │  │ C++ Bindings   ││
│  │ (High Level)│  │ (Mid Level)  │  │ (Low Level)    ││
│  └─────────────┘  └──────────────┘  └────────────────┘│
└───────────────────────┬─────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────┐
│              Native Runtime Layer                       │
│  ┌──────┐  ┌──────┐  ┌────────┐  ┌──────────────────┐ │
│  │ BLAS │  │LAPACK│  │ CUDA/  │  │ Custom C++ Ops   │ │
│  │      │  │      │  │ OpenCL │  │ (String, I/O)    │ │
│  └──────┘  └──────┘  └────────┘  └──────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

---

## Core Modules

### Module Organization

```
stdlib/
├── prelude.volta          # Auto-imported (print, len, range, etc.)
├── core/
│   ├── option.volta       # Option[T] utilities
│   ├── result.volta       # Result[T, E] error handling
│   ├── iterator.volta     # Iterator protocol
│   └── functional.volta   # map, filter, reduce, compose
│
├── collections/
│   ├── array.volta        # Dynamic arrays
│   ├── dict.volta         # Hash maps
│   ├── set.volta          # Sets
│   └── tuple.volta        # Tuple utilities
│
├── string/
│   ├── string.volta       # String operations
│   ├── format.volta       # String formatting
│   └── regex.volta        # Regular expressions
│
├── io/
│   ├── io.volta           # Console I/O
│   ├── file.volta         # File operations
│   └── path.volta         # Path manipulation
│
├── math/
│   ├── math.volta         # Basic math (sin, cos, sqrt, etc.)
│   ├── random.volta       # Random number generation
│   └── constants.volta    # PI, E, etc.
│
├── numeric/
│   ├── array.volta        # NumPy-style arrays (BLAS backend)
│   ├── linalg.volta       # Linear algebra (LAPACK backend)
│   ├── stats.volta        # Statistics
│   ├── fft.volta          # FFT operations
│   └── special.volta      # Special functions (erf, gamma, etc.)
│
├── gpu/
│   ├── array.volta        # GPU arrays (CUDA/OpenCL)
│   ├── kernels.volta      # GPU kernel interface
│   └── transfer.volta     # CPU ↔ GPU transfers
│
└── time/
    ├── datetime.volta     # Date/time handling
    └── duration.volta     # Time durations
```

---

## Core Modules

### 1. Prelude (Auto-imported)

**File:** `stdlib/prelude.volta`

**Always available without import:**

```volta
# Basic I/O
fn print(msg: str) -> void
fn println(msg: str) -> void
fn input(prompt: str) -> str

# Type conversions
fn int(x: any) -> int
fn float(x: any) -> float
fn str(x: any) -> str
fn bool(x: any) -> bool

# Collection operations
fn len[T](arr: Array[T]) -> int
fn range(start: int, end: int) -> Iterator[int]
fn range(end: int) -> Iterator[int]  # 0..end

# Assertions
fn assert(condition: bool, msg: str) -> void
fn panic(msg: str) -> never

# Common types (re-exports)
type Option[T] = core.option.Option[T]
type Result[T, E] = core.result.Result[T, E]
```

**Implementation:**
- Mostly thin wrappers over C++ runtime functions
- `print`, `input` → C++ I/O
- Type conversions → C++ casting
- `len`, `range` → VM builtins

---

### 2. Option Types

**File:** `stdlib/core/option.volta`

```volta
# Option type (Some | None)
enum Option[T] {
    Some(T),
    None
}

# Methods
impl[T] Option[T] {
    fn is_some(self) -> bool
    fn is_none(self) -> bool

    fn unwrap(self) -> T  # Panics if None
    fn unwrap_or(self, default: T) -> T
    fn unwrap_or_else(self, f: fn() -> T) -> T

    fn map[U](self, f: fn(T) -> U) -> Option[U]
    fn map_or[U](self, default: U, f: fn(T) -> U) -> U
    fn and_then[U](self, f: fn(T) -> Option[U]) -> Option[U]

    fn ok_or[E](self, err: E) -> Result[T, E]
}
```

**Usage:**
```volta
import option

fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

result := divide(10.0, 2.0)
value := result.unwrap_or(0.0)  # 5.0

chained := divide(10.0, 2.0)
    .map(fn(x) -> x * 2.0)
    .unwrap_or(0.0)  # 10.0
```

---

### 3. Collections - Array

**File:** `stdlib/collections/array.volta`

**Basic operations** (pure Volta):

```volta
impl[T] Array[T] {
    # Construction
    fn new() -> Array[T]
    fn with_capacity(capacity: int) -> Array[T]
    fn filled(value: T, count: int) -> Array[T]

    # Access
    fn get(self, index: int) -> Option[T]
    fn first(self) -> Option[T]
    fn last(self) -> Option[T]
    fn len(self) -> int
    fn is_empty(self) -> bool

    # Modification (mutable)
    fn push(self: mut, item: T) -> void
    fn pop(self: mut) -> Option[T]
    fn insert(self: mut, index: int, item: T) -> void
    fn remove(self: mut, index: int) -> T
    fn clear(self: mut) -> void

    # Slicing (returns new array)
    fn slice(self, start: int, end: int) -> Array[T]

    # Functional operations
    fn map[U](self, f: fn(T) -> U) -> Array[U]
    fn filter(self, pred: fn(T) -> bool) -> Array[T]
    fn reduce[U](self, f: fn(U, T) -> U, initial: U) -> U
    fn fold[U](self, initial: U, f: fn(U, T) -> U) -> U  # Alias for reduce

    # Searching
    fn contains(self, item: T) -> bool  # Requires T: Eq
    fn find(self, pred: fn(T) -> bool) -> Option[T]
    fn find_index(self, pred: fn(T) -> bool) -> Option[int]

    # Iteration
    fn iter(self) -> Iterator[T]
    fn enumerate(self) -> Iterator[(int, T)]

    # Aggregation
    fn sum(self) -> T  # Requires T: Numeric
    fn product(self) -> T  # Requires T: Numeric
    fn min(self) -> Option[T]  # Requires T: Ord
    fn max(self) -> Option[T]  # Requires T: Ord

    # Sorting
    fn sort(self: mut) -> void  # In-place, requires T: Ord
    fn sorted(self) -> Array[T]  # Returns new sorted array
    fn sort_by(self: mut, cmp: fn(T, T) -> int) -> void

    # Joining
    fn join(self, separator: str) -> str  # Requires T: ToString
    fn concat(self, other: Array[T]) -> Array[T]

    # Zipping
    fn zip[U](self, other: Array[U]) -> Array[(T, U)]
}
```

**Implementation:**
- Most operations: Pure Volta (compiled to efficient bytecode)
- Push/pop/insert/remove: C++ for memory management
- Sort: C++ (uses `std::sort` or custom algorithm)

---

## Scientific Computing Modules

### 4. Math Module

**File:** `stdlib/math/math.volta`

**Wraps C standard math library (libm):**

```volta
# Module: math

# Constants
const PI: float = 3.141592653589793
const E: float = 2.718281828459045
const TAU: float = 6.283185307179586  # 2*PI
const INF: float = __builtin_inf()
const NAN: float = __builtin_nan()

# Trigonometric functions
fn sin(x: float) -> float      # C: sin()
fn cos(x: float) -> float      # C: cos()
fn tan(x: float) -> float      # C: tan()
fn asin(x: float) -> float     # C: asin()
fn acos(x: float) -> float     # C: acos()
fn atan(x: float) -> float     # C: atan()
fn atan2(y: float, x: float) -> float  # C: atan2()

# Hyperbolic functions
fn sinh(x: float) -> float
fn cosh(x: float) -> float
fn tanh(x: float) -> float

# Exponential and logarithmic
fn exp(x: float) -> float      # e^x
fn exp2(x: float) -> float     # 2^x
fn log(x: float) -> float      # Natural log
fn log10(x: float) -> float    # Base-10 log
fn log2(x: float) -> float     # Base-2 log

# Power functions
fn pow(base: float, exp: float) -> float
fn sqrt(x: float) -> float
fn cbrt(x: float) -> float     # Cube root

# Rounding
fn ceil(x: float) -> float
fn floor(x: float) -> float
fn round(x: float) -> float
fn trunc(x: float) -> float

# Other
fn abs(x: float) -> float
fn abs(x: int) -> int          # Overloaded
fn sign(x: float) -> float     # -1, 0, or 1
fn min(a: float, b: float) -> float
fn max(a: float, b: float) -> float
fn clamp(x: float, min: float, max: float) -> float

# Angle conversion
fn degrees(radians: float) -> float
fn radians(degrees: float) -> float

# Special checks
fn is_nan(x: float) -> bool
fn is_inf(x: float) -> bool
fn is_finite(x: float) -> bool
```

**Implementation:**
All functions are **thin C++ wrappers** over `<cmath>`:

```cpp
// src/vm/RuntimeFunctions.cpp
double volta_math_sin(double x) { return std::sin(x); }
double volta_math_cos(double x) { return std::cos(x); }
// etc.
```

---

### 5. Numeric Arrays (NumPy-style, BLAS-backed)

**File:** `stdlib/numeric/array.volta`

**This is the BIG one for scientific computing!**

```volta
# Module: numeric
# NumPy-style multidimensional arrays with BLAS backend

struct NdArray[T] {
    data: *T,          # Pointer to data (C++)
    shape: Array[int], # Dimensions
    strides: Array[int], # Memory layout
    dtype: Type        # Data type
}

# Construction
fn array[T](data: Array[T]) -> NdArray[T]
fn zeros(shape: Array[int]) -> NdArray[float]
fn ones(shape: Array[int]) -> NdArray[float]
fn full(shape: Array[int], value: float) -> NdArray[float]
fn eye(n: int) -> NdArray[float]  # Identity matrix
fn arange(start: float, stop: float, step: float) -> NdArray[float]
fn linspace(start: float, stop: float, num: int) -> NdArray[float]
fn logspace(start: float, stop: float, num: int) -> NdArray[float]

# Random arrays
fn random(shape: Array[int]) -> NdArray[float]  # Uniform [0, 1)
fn randn(shape: Array[int]) -> NdArray[float]   # Normal(0, 1)
fn randint(low: int, high: int, shape: Array[int]) -> NdArray[int]

impl[T] NdArray[T] {
    # Properties
    fn shape(self) -> Array[int]
    fn ndim(self) -> int      # Number of dimensions
    fn size(self) -> int      # Total number of elements
    fn dtype(self) -> Type

    # Indexing
    fn get(self, indices: Array[int]) -> T
    fn set(self: mut, indices: Array[int], value: T) -> void
    fn at(self, i: int) -> T  # 1D indexing (flattened)

    # Slicing (returns view, not copy)
    fn slice(self, ranges: Array[Range]) -> NdArray[T]

    # Reshaping
    fn reshape(self, new_shape: Array[int]) -> NdArray[T]
    fn flatten(self) -> NdArray[T]
    fn transpose(self) -> NdArray[T]  # Swap dimensions
    fn T(self) -> NdArray[T]          # Alias for transpose

    # Element-wise operations (broadcasts automatically)
    fn add(self, other: NdArray[T]) -> NdArray[T]
    fn sub(self, other: NdArray[T]) -> NdArray[T]
    fn mul(self, other: NdArray[T]) -> NdArray[T]  # Element-wise!
    fn div(self, other: NdArray[T]) -> NdArray[T]

    # Scalar operations
    fn add_scalar(self, scalar: T) -> NdArray[T]
    fn mul_scalar(self, scalar: T) -> NdArray[T]

    # Operator overloads (if Volta supports)
    # arr1 + arr2 → arr1.add(arr2)
    # arr * 2.0 → arr.mul_scalar(2.0)

    # Reductions (along axis)
    fn sum(self, axis: Option[int]) -> NdArray[T]
    fn mean(self, axis: Option[int]) -> NdArray[float]
    fn std(self, axis: Option[int]) -> NdArray[float]
    fn var(self, axis: Option[int]) -> NdArray[float]
    fn min(self, axis: Option[int]) -> NdArray[T]
    fn max(self, axis: Option[int]) -> NdArray[T]

    # Boolean operations
    fn all(self, axis: Option[int]) -> NdArray[bool]
    fn any(self, axis: Option[int]) -> NdArray[bool]

    # Comparison (element-wise, returns bool array)
    fn eq(self, other: NdArray[T]) -> NdArray[bool]
    fn lt(self, other: NdArray[T]) -> NdArray[bool]
    fn gt(self, other: NdArray[T]) -> NdArray[bool]

    # Boolean indexing
    fn where(self, condition: NdArray[bool]) -> NdArray[T]

    # Copying
    fn copy(self) -> NdArray[T]  # Deep copy
    fn view(self) -> NdArray[T]  # Shallow view (no copy)

    # Type conversion
    fn astype[U](self) -> NdArray[U]
    fn to_array(self) -> Array[T]  # Convert to regular Array
}

# Universal functions (ufuncs) - element-wise operations
fn sin(arr: NdArray[float]) -> NdArray[float]
fn cos(arr: NdArray[float]) -> NdArray[float]
fn exp(arr: NdArray[float]) -> NdArray[float]
fn log(arr: NdArray[float]) -> NdArray[float]
fn sqrt(arr: NdArray[float]) -> NdArray[float]
fn abs(arr: NdArray[float]) -> NdArray[float]
# ... all math functions vectorized

# Advanced operations
fn dot(a: NdArray[float], b: NdArray[float]) -> NdArray[float]  # BLAS
fn matmul(a: NdArray[float], b: NdArray[float]) -> NdArray[float]  # BLAS
fn outer(a: NdArray[float], b: NdArray[float]) -> NdArray[float]
fn inner(a: NdArray[float], b: NdArray[float]) -> float
```

**Implementation:**
- **Data storage:** C++ contiguous memory (aligned for SIMD)
- **Element-wise ops:** C++ with SIMD (SSE/AVX)
- **Matrix ops:** BLAS (cblas_dgemm, etc.)
- **Reductions:** C++ parallel loops (OpenMP)

**Backend selection:**
```cpp
// Compile-time backend selection
#ifdef USE_OPENBLAS
  #include <cblas.h>
#elif USE_MKL
  #include <mkl.h>
#elif USE_ATLAS
  #include <cblas.h>
#endif
```

---

### 6. Linear Algebra (LAPACK-backed)

**File:** `stdlib/numeric/linalg.volta`

**High-performance linear algebra:**

```volta
# Module: linalg
# Linear algebra operations backed by LAPACK

# Matrix decompositions
fn lu(A: NdArray[float]) -> (NdArray[float], NdArray[float])  # LU decomposition
fn qr(A: NdArray[float]) -> (NdArray[float], NdArray[float])  # QR decomposition
fn svd(A: NdArray[float]) -> (NdArray[float], NdArray[float], NdArray[float])  # SVD
fn eig(A: NdArray[float]) -> (NdArray[float], NdArray[float])  # Eigenvalues/vectors
fn cholesky(A: NdArray[float]) -> NdArray[float]  # Cholesky decomposition

# Solving linear systems
fn solve(A: NdArray[float], b: NdArray[float]) -> NdArray[float]  # Solve Ax = b
fn lstsq(A: NdArray[float], b: NdArray[float]) -> NdArray[float]  # Least squares

# Matrix properties
fn det(A: NdArray[float]) -> float  # Determinant
fn trace(A: NdArray[float]) -> float  # Trace
fn rank(A: NdArray[float]) -> int  # Matrix rank
fn cond(A: NdArray[float]) -> float  # Condition number
fn norm(x: NdArray[float], ord: Option[float]) -> float  # Various norms

# Matrix operations
fn inv(A: NdArray[float]) -> NdArray[float]  # Matrix inverse
fn pinv(A: NdArray[float]) -> NdArray[float]  # Pseudo-inverse (Moore-Penrose)

# Matrix factorizations for specialized solvers
fn lu_factor(A: NdArray[float]) -> LUFactorization
fn lu_solve(lu: LUFactorization, b: NdArray[float]) -> NdArray[float]

# Eigenvalue problems
fn eigvals(A: NdArray[float]) -> NdArray[float]  # Eigenvalues only (faster)
fn eigvalsh(A: NdArray[float]) -> NdArray[float]  # For Hermitian matrices

# Matrix functions
fn matrix_power(A: NdArray[float], n: int) -> NdArray[float]
fn matrix_exp(A: NdArray[float]) -> NdArray[float]
fn matrix_log(A: NdArray[float]) -> NdArray[float]
```

**Implementation:**
- **All functions:** Direct LAPACK calls
- **Wrapper:** Thin C++ layer for error handling
- **Memory:** Zero-copy where possible (pass pointers to LAPACK)

**Example C++ wrapper:**
```cpp
// src/vm/RuntimeLinalg.cpp
#include <lapacke.h>

NdArray* volta_linalg_solve(NdArray* A, NdArray* b) {
    // Extract data
    int n = A->shape[0];
    double* A_data = (double*)A->data;
    double* b_data = (double*)b->data;

    // Call LAPACK
    int* ipiv = new int[n];
    int info = LAPACKE_dgesv(
        LAPACK_ROW_MAJOR, n, 1,
        A_data, n, ipiv, b_data, 1
    );

    delete[] ipiv;

    if (info != 0) {
        throw LinAlgError("Singular matrix");
    }

    return b;  // Solution in b
}
```

---

### 7. Statistics

**File:** `stdlib/numeric/stats.volta`

```volta
# Module: stats
# Statistical functions

# Descriptive statistics
fn mean(data: NdArray[float]) -> float
fn median(data: NdArray[float]) -> float
fn mode(data: NdArray[float]) -> float
fn std(data: NdArray[float], ddof: int) -> float  # Standard deviation
fn var(data: NdArray[float], ddof: int) -> float  # Variance
fn quantile(data: NdArray[float], q: float) -> float
fn percentile(data: NdArray[float], p: float) -> float

# Correlation and covariance
fn corrcoef(x: NdArray[float], y: NdArray[float]) -> float
fn cov(x: NdArray[float], y: NdArray[float]) -> float
fn cov_matrix(data: NdArray[float]) -> NdArray[float]

# Distributions
fn normal(mean: float, std: float, size: int) -> NdArray[float]
fn uniform(low: float, high: float, size: int) -> NdArray[float]
fn exponential(scale: float, size: int) -> NdArray[float]
fn poisson(lam: float, size: int) -> NdArray[int]

# Hypothesis testing
fn ttest_ind(a: NdArray[float], b: NdArray[float]) -> (float, float)  # (statistic, p-value)
fn ttest_rel(a: NdArray[float], b: NdArray[float]) -> (float, float)
fn chisquare(observed: NdArray[float], expected: NdArray[float]) -> (float, float)

# Histograms
fn histogram(data: NdArray[float], bins: int) -> (NdArray[int], NdArray[float])
```

**Implementation:**
- Basic stats: Pure Volta or C++ loops
- Distributions: C++ with GSL (GNU Scientific Library) or Boost
- Hypothesis tests: C++ implementations

---

### 8. GPU Arrays

**File:** `stdlib/gpu/array.volta`

**GPU-accelerated arrays (CUDA/OpenCL):**

```volta
# Module: gpu
# GPU-accelerated arrays

struct GpuArray[T] {
    device_ptr: *T,   # GPU memory pointer
    shape: Array[int],
    device_id: int
}

# Construction (copies to GPU)
fn array[T](data: NdArray[T]) -> GpuArray[T]
fn zeros(shape: Array[int]) -> GpuArray[float]
fn ones(shape: Array[int]) -> GpuArray[float]

impl[T] GpuArray[T] {
    # Transfer operations
    fn to_cpu(self) -> NdArray[T]  # GPU → CPU
    fn to_gpu(arr: NdArray[T]) -> GpuArray[T]  # CPU → GPU

    # Element-wise operations (GPU kernels)
    fn add(self, other: GpuArray[T]) -> GpuArray[T]
    fn mul(self, other: GpuArray[T]) -> GpuArray[T]
    fn sub(self, other: GpuArray[T]) -> GpuArray[T]
    fn div(self, other: GpuArray[T]) -> GpuArray[T]

    # Reductions (parallel GPU kernels)
    fn sum(self) -> T
    fn mean(self) -> float
    fn max(self) -> T
    fn min(self) -> T

    # Matrix operations (cuBLAS/clBLAS)
    fn matmul(self, other: GpuArray[T]) -> GpuArray[T]
    fn dot(self, other: GpuArray[T]) -> GpuArray[T]
}

# GPU-accelerated functions
fn matmul_gpu(a: GpuArray[float], b: GpuArray[float]) -> GpuArray[float]
fn svd_gpu(A: GpuArray[float]) -> (GpuArray[float], GpuArray[float], GpuArray[float])

# Device management
fn device_count() -> int
fn set_device(device_id: int) -> void
fn get_device() -> int
fn synchronize() -> void  # Wait for GPU to finish

# Memory info
fn memory_info() -> (int, int)  # (free, total) in bytes
```

**Implementation:**
- **CUDA backend:** `cuBLAS`, `cuSOLVER` for linear algebra
- **OpenCL backend:** `clBLAS` for portability
- **Kernel JIT:** Compile simple kernels on-the-fly
- **Memory management:** Pool allocator for GPU memory

**Example C++ implementation:**
```cpp
// src/vm/RuntimeGPU.cpp
#include <cuda_runtime.h>
#include <cublas_v2.h>

GpuArray* volta_gpu_matmul(GpuArray* a, GpuArray* b) {
    cublasHandle_t handle;
    cublasCreate(&handle);

    float alpha = 1.0f, beta = 0.0f;
    GpuArray* result = allocate_gpu_array(/*...*/);

    cublasSgemm(
        handle, CUBLAS_OP_N, CUBLAS_OP_N,
        m, n, k,
        &alpha,
        (float*)a->device_ptr, lda,
        (float*)b->device_ptr, ldb,
        &beta,
        (float*)result->device_ptr, ldc
    );

    cublasDestroy(handle);
    return result;
}
```

---

### 9. FFT (Fast Fourier Transform)

**File:** `stdlib/numeric/fft.volta`

```volta
# Module: fft
# Fast Fourier Transform (backed by FFTW)

# 1D FFT
fn fft(x: NdArray[float]) -> NdArray[Complex]
fn ifft(x: NdArray[Complex]) -> NdArray[Complex]
fn rfft(x: NdArray[float]) -> NdArray[Complex]  # Real input
fn irfft(x: NdArray[Complex]) -> NdArray[float]

# 2D FFT
fn fft2(x: NdArray[float]) -> NdArray[Complex]
fn ifft2(x: NdArray[Complex]) -> NdArray[Complex]

# ND FFT
fn fftn(x: NdArray[float], axes: Option[Array[int]]) -> NdArray[Complex]
fn ifftn(x: NdArray[Complex], axes: Option[Array[int]]) -> NdArray[Complex]

# Frequency arrays
fn fftfreq(n: int, d: float) -> NdArray[float]
fn rfftfreq(n: int, d: float) -> NdArray[float]

# Convolution
fn convolve(a: NdArray[float], b: NdArray[float]) -> NdArray[float]
fn correlate(a: NdArray[float], b: NdArray[float]) -> NdArray[float]
```

**Implementation:**
- **Backend:** FFTW (fastest FFT in the West)
- **GPU version:** cuFFT for CUDA
- **Complex numbers:** Add `Complex` type to Volta

---

## Implementation Strategy

### Phase 1: Foundation (Month 1-2)

**Priority: Core modules**

1. ✅ Prelude (basic I/O, conversions)
2. ✅ Array (collections)
3. ✅ Math (basic math functions)
4. ✅ String operations
5. ✅ Option/Result types

**Implementation:**
- Most in pure Volta
- C++ wrappers for I/O, math

---

### Phase 2: Scientific Computing Basics (Month 3-4)

**Priority: NumPy-style arrays**

6. ✅ NdArray (multidimensional arrays)
7. ✅ Basic BLAS operations (dot, matmul)
8. ✅ Element-wise operations
9. ✅ Broadcasting
10. ✅ Stats module

**Implementation:**
- C++ NdArray class
- Link against OpenBLAS or MKL
- SIMD for element-wise ops

---

### Phase 3: Advanced Linear Algebra (Month 5-6)

**Priority: LAPACK integration**

11. ✅ Linear solvers
12. ✅ Matrix decompositions
13. ✅ Eigenvalue problems
14. ✅ Specialized solvers

**Implementation:**
- Direct LAPACK bindings
- Error handling wrappers

---

### Phase 4: GPU Acceleration (Month 7-8)

**Priority: GPU arrays**

15. ✅ GpuArray type
16. ✅ cuBLAS/clBLAS bindings
17. ✅ Memory management
18. ✅ CPU ↔ GPU transfers

**Implementation:**
- CUDA backend (NVIDIA)
- OpenCL backend (portable)
- Automatic kernel generation

---

### Phase 5: Advanced Features (Month 9+)

19. ✅ FFT module
20. ✅ Sparse arrays
21. ✅ Advanced optimizations
22. ✅ Parallel CPU operations (OpenMP)

---

## Module Details

### How Modules Are Loaded

**Directory structure:**
```
/usr/local/lib/volta/stdlib/
├── prelude.vbc           # Precompiled bytecode
├── math.vbc
├── numeric/
│   ├── array.vbc
│   └── linalg.vbc
└── gpu/
    └── array.vbc

/usr/local/lib/volta/native/
├── libvolta_math.so      # Native libraries
├── libvolta_blas.so
├── libvolta_lapack.so
└── libvolta_gpu.so
```

**Import resolution:**
```volta
import math              # Loads stdlib/math.vbc
import numeric.array     # Loads stdlib/numeric/array.vbc
import gpu               # Loads stdlib/gpu.vbc + libvolta_gpu.so
```

**In main.cpp:**
```cpp
// VM initialization
VM vm;
vm.setStdlibPath("/usr/local/lib/volta/stdlib");
vm.loadPrelude();  // Always loaded

// User code
vm.loadModule("math");
vm.loadModule("numeric.array");
```

---

### Native Function Registration

**In C++ runtime:**
```cpp
// src/vm/VM.cpp
void VM::registerStdlibFunctions() {
    // Math functions
    registerNative("math.sin", volta_math_sin);
    registerNative("math.cos", volta_math_cos);
    registerNative("math.sqrt", volta_math_sqrt);

    // Array operations (BLAS)
    registerNative("numeric.dot", volta_blas_dot);
    registerNative("numeric.matmul", volta_blas_gemm);

    // Linear algebra (LAPACK)
    registerNative("linalg.solve", volta_lapack_solve);
    registerNative("linalg.svd", volta_lapack_svd);

    // GPU operations (CUDA)
    registerNative("gpu.matmul", volta_cuda_gemm);
    registerNative("gpu.to_gpu", volta_cuda_copy_h2d);
}
```

**In Volta stdlib:**
```volta
# stdlib/math.volta
fn sin(x: float) -> float {
    return __builtin("math.sin", x)
}

# Or with special syntax:
fn sin(x: float) -> float = __native("math.sin")
```

---

## Usage Examples

### Example 1: Basic NumPy-style Operations

```volta
import numeric

# Create arrays
a := numeric.array([1.0, 2.0, 3.0, 4.0, 5.0])
b := numeric.array([5.0, 4.0, 3.0, 2.0, 1.0])

# Element-wise operations
c := a + b           # [6, 6, 6, 6, 6]
d := a * 2.0         # [2, 4, 6, 8, 10]

# Reductions
sum := a.sum()       # 15.0
mean := a.mean()     # 3.0
std := a.std()       # 1.414...

# 2D arrays
matrix := numeric.zeros([3, 3])
identity := numeric.eye(3)

# Matrix multiplication
result := numeric.matmul(matrix, identity)
```

---

### Example 2: Linear Algebra

```volta
import numeric
import linalg

# Solve linear system Ax = b
A := numeric.array([[3.0, 1.0], [1.0, 2.0]])
b := numeric.array([9.0, 8.0])

x := linalg.solve(A, b)
print(x)  # [2.0, 3.0]

# Eigenvalue decomposition
eigenvalues, eigenvectors := linalg.eig(A)

# SVD
U, S, Vt := linalg.svd(A)

# Matrix inverse
A_inv := linalg.inv(A)
```

---

### Example 3: Statistics

```volta
import numeric
import stats

# Generate data
data := stats.normal(mean: 0.0, std: 1.0, size: 1000)

# Compute statistics
mean := stats.mean(data)
std := stats.std(data)
median := stats.median(data)

# Hypothesis test
group1 := stats.normal(0.0, 1.0, 100)
group2 := stats.normal(0.5, 1.0, 100)

statistic, p_value := stats.ttest_ind(group1, group2)

if p_value < 0.05 {
    print("Groups are significantly different")
}
```

---

### Example 4: GPU Acceleration

```volta
import numeric
import gpu

# Create large matrices on CPU
A := numeric.random([1000, 1000])
B := numeric.random([1000, 1000])

# Copy to GPU
A_gpu := gpu.array(A)
B_gpu := gpu.array(B)

# Perform matrix multiplication on GPU (MUCH faster!)
C_gpu := gpu.matmul(A_gpu, B_gpu)

# Copy result back to CPU
C := C_gpu.to_cpu()

print("Result shape:", C.shape())
```

---

### Example 5: Complete Scientific Workflow

```volta
import numeric
import linalg
import stats
import gpu

fn least_squares_gpu(X: numeric.NdArray[float], y: numeric.NdArray[float])
    -> numeric.NdArray[float] {
    # Solve least squares problem on GPU
    # X: [n_samples, n_features]
    # y: [n_samples]

    # Copy to GPU
    X_gpu := gpu.array(X)
    y_gpu := gpu.array(y)

    # Compute X^T X and X^T y on GPU
    XtX := gpu.matmul(X_gpu.transpose(), X_gpu)
    Xty := gpu.matmul(X_gpu.transpose(), y_gpu)

    # Copy back to CPU for linear solve (or use GPU solver)
    XtX_cpu := XtX.to_cpu()
    Xty_cpu := Xty.to_cpu()

    # Solve normal equations
    beta := linalg.solve(XtX_cpu, Xty_cpu)

    return beta
}

# Generate synthetic data
X := numeric.random([10000, 100])
true_beta := numeric.randn([100])
noise := stats.normal(0.0, 0.1, 10000)
y := numeric.dot(X, true_beta) + noise

# Fit model
beta_hat := least_squares_gpu(X, y)

# Compute R²
y_pred := numeric.dot(X, beta_hat)
r2 := stats.r2_score(y, y_pred)

print("R² score:", r2)
```

---

## Build System Integration

### Linking External Libraries

**Makefile additions:**
```makefile
# BLAS/LAPACK
BLAS_FLAGS := -lopenblas  # or -lmkl_rt for Intel MKL
LAPACK_FLAGS := -llapacke

# GPU (optional)
ifdef USE_CUDA
    CUDA_FLAGS := -lcublas -lcusolver -lcufft
    CXXFLAGS += -DUSE_CUDA
endif

# FFT
FFT_FLAGS := -lfftw3

# Link all
LDFLAGS += $(BLAS_FLAGS) $(LAPACK_FLAGS) $(CUDA_FLAGS) $(FFT_FLAGS)
```

### CMake Alternative

```cmake
# CMakeLists.txt
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(FFTW3 REQUIRED)

option(USE_CUDA "Enable CUDA support" ON)
if(USE_CUDA)
    find_package(CUDA REQUIRED)
    target_link_libraries(volta ${CUDA_LIBRARIES} ${CUDA_CUBLAS_LIBRARIES})
endif()

target_link_libraries(volta
    ${BLAS_LIBRARIES}
    ${LAPACK_LIBRARIES}
    ${FFTW3_LIBRARIES}
)
```

---

## Testing Strategy

### Unit Tests for Each Module

```volta
# test/stdlib/test_array.volta
import test
import array

test("Array.map transforms elements") {
    arr := [1, 2, 3]
    doubled := arr.map(fn(x) -> x * 2)
    test.assert_eq(doubled, [2, 4, 6])
}

test("Array.sum computes total") {
    arr := [1, 2, 3, 4, 5]
    test.assert_eq(arr.sum(), 15)
}
```

### Performance Benchmarks

```volta
# bench/numeric_bench.volta
import bench
import numeric

bench("Matrix multiplication 1000x1000") {
    A := numeric.random([1000, 1000])
    B := numeric.random([1000, 1000])

    bench.run {
        C := numeric.matmul(A, B)
    }
}
```

---

## Documentation Generation

### Docstrings

```volta
/// Computes the dot product of two arrays using BLAS.
///
/// # Arguments
/// * `a` - First input array
/// * `b` - Second input array
///
/// # Returns
/// The dot product as a scalar
///
/// # Examples
/// ```volta
/// a := array([1.0, 2.0, 3.0])
/// b := array([4.0, 5.0, 6.0])
/// result := dot(a, b)  # 32.0
/// ```
///
/// # Performance
/// This function uses BLAS `cblas_ddot` for optimal performance.
fn dot(a: NdArray[float], b: NdArray[float]) -> float
```

**Generate HTML docs:**
```bash
volta doc stdlib/ --output docs/stdlib/
```

---

## Summary

This standard library specification provides:

1. ✅ **Complete API** for scientific computing (NumPy/SciPy equivalent)
2. ✅ **Performance** through BLAS/LAPACK/CUDA integration
3. ✅ **Clean separation** between Volta (high-level) and C++ (low-level)
4. ✅ **GPU support** as first-class citizen
5. ✅ **Pythonic API** familiar to scientific users
6. ✅ **Extensible** design for future additions

**Total API surface:** ~300-400 functions across 15+ modules

**Estimated implementation time:**
- Core modules: 2-3 months
- Scientific modules: 3-4 months
- GPU support: 2 months
- **Total: 6-9 months for complete stdlib**

---

**Next Steps:**
1. Implement Phase 1 (core modules)
2. Design NdArray C++ class
3. BLAS integration
4. Build module loading system

---

**Generated with:** Claude Code
**Last Updated:** 2025-10-06
**Status:** Design specification (not yet implemented)
