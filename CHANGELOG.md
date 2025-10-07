# Changelog

All notable changes to Volta will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned for 1.0
- Enums (algebraic data types)
- Pattern matching on enums
- Option[T] as built-in enum
- Result[T,E] for error handling
- Structs with methods
- Tuples
- Array methods (map, filter, reduce)
- String methods (len, contains, split)
- Break/continue in loops

## [0.1.0-alpha] - 2025-10-07

### Added
- Complete compiler pipeline (Lexer → Parser → Semantic → IR → Bytecode → VM)
- Basic types (int, float, bool, str)
- Arrays with indexing
- Variables (immutable by default, `mut` for mutable)
- Type inference with `:=` syntax
- Functions with explicit type signatures
- If/else expressions and statements
- While loops
- For loops with range operators (`0..5`, `0..=5`)
- Pattern matching (parser support only)
- Print builtin (overloaded for int, float, bool, str)
- Register-based VM
- Generational garbage collector
- IR optimization passes (constant folding, DCE, algebraic simplification)
- Comprehensive test suite (660 passing tests)
- LSP server for editor integration
- Documentation with MkDocs

### Implemented
- ✅ Lexer with full tokenization
- ✅ Parser with expression precedence
- ✅ Semantic analysis with type checking
- ✅ SSA-form IR generation
- ✅ IR optimization pipeline
- ✅ Bytecode compiler
- ✅ Register-based VM with call stack
- ✅ Generational GC (nursery + old gen)
- ✅ Basic builtin functions

### Known Limitations
- No pattern matching execution (parser only)
- No enum types
- No struct types
- No tuple types
- No Option[T] operations
- No standard library beyond print()
- No module system
- Break/continue not implemented in VM

### Performance
- Executes simple arithmetic and loops
- GC passes 660 tests including stress tests
- Optimization passes functional

---

## Release Categories

### Added
New features and functionality

### Changed
Changes to existing functionality

### Deprecated
Features that will be removed in future versions

### Removed
Features that have been removed

### Fixed
Bug fixes

### Security
Security-related fixes

---

## Version History Template

```markdown
## [X.Y.Z] - YYYY-MM-DD

### Added
- New feature 1
- New feature 2

### Changed
- Modified feature 1
- Updated feature 2

### Fixed
- Bug fix 1
- Bug fix 2

### Performance
- Optimization 1
- Optimization 2
```

---

## Upcoming Milestones

### 1.0 MVP (Target: Q2 2026)
Core language features for self-hosting:
- Enums with pattern matching
- Structs with methods
- Result[T,E] error handling
- Basic standard library

### 1.5 (Target: Q3 2026)
Enhanced language features:
- Module system
- Imports
- Better error messages
- More stdlib functions

### 2.0 Self-Hosted (Target: Q4 2026)
Compiler written in Volta:
- Volta compiler compiles itself
- All features working
- Performance competitive

### 3.0 Scientific Computing (Target: 2027)
Original vision realized:
- Array operations
- Math library
- Stats library
- JIT compilation
- Parallel operations

---

[Unreleased]: https://github.com/yourusername/volta/compare/v0.1.0-alpha...HEAD
[0.1.0-alpha]: https://github.com/yourusername/volta/releases/tag/v0.1.0-alpha
