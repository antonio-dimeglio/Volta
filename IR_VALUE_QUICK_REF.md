# IR Value - Quick Reference Card

## Build & Test Commands

```bash
# Build and run all tests
make test

# Run only Value tests
./bin/test_volta --gtest_filter=ValueTest.*

# Run specific test
./bin/test_volta --gtest_filter=ValueTest.ConstantIntCreation

# Run tests with verbose output
./bin/test_volta --gtest_filter=ValueTest.* --gtest_verbose

# Clean and rebuild
make clean && make test
```

## Implementation Checklist

### Phase 1: Basic Infrastructure
- [ ] `Value::Value()` constructor
- [ ] `Value::isConstant()`
- [ ] `Value::isInstruction()`
- [ ] `Value::isArgument()`
- [ ] `Value::isGlobalValue()`

### Phase 2: Constants (Start Here!)
- [ ] `ConstantInt::get()` - Return `new ConstantInt(value, type)`
- [ ] `ConstantInt::toString()` - Return `std::to_string(value_)`
- [ ] `ConstantFloat::get()`
- [ ] `ConstantFloat::toString()`
- [ ] `ConstantBool::get()`, `getTrue()`, `getFalse()`
- [ ] `ConstantBool::toString()`
- [ ] `ConstantString::get()`
- [ ] `ConstantString::toString()`
- [ ] `ConstantNone::get()`
- [ ] `ConstantNone::toString()`
- [ ] `UndefValue::get()`
- [ ] `UndefValue::toString()`

### Phase 3: Arguments & Globals
- [ ] `Argument::toString()`
- [ ] `GlobalVariable::toString()`

### Phase 4: Use-Def Chains (Advanced)
- [ ] `Use::Use()` constructor
- [ ] `Use::~Use()` destructor
- [ ] `Use::setValue()`
- [ ] `Value::addUse()`
- [ ] `Value::removeUse()`
- [ ] `Value::replaceAllUsesWith()`

## Code Snippets

### ConstantInt::get()
```cpp
ConstantInt* ConstantInt::get(int64_t value, std::shared_ptr<semantic::Type> type) {
    return new ConstantInt(value, type);
}
```

### ConstantInt::toString()
```cpp
std::string ConstantInt::toString() const {
    return std::to_string(value_);
}
```

### Value::isConstant()
```cpp
bool Value::isConstant() const {
    return kind_ >= ValueKind::ConstantInt && kind_ <= ValueKind::UndefValue;
}
```

### Argument::toString()
```cpp
std::string Argument::toString() const {
    if (hasName()) {
        return "%" + getName();
    }
    return "%arg" + std::to_string(argNo_);
}
```

### Use::Use()
```cpp
Use::Use(Value* value, Instruction* user)
    : value_(value), user_(user) {
    if (value_) {
        value_->addUse(this);
    }
}
```

### Value::addUse()
```cpp
void Value::addUse(Use* use) {
    uses_.push_back(use);
}
```

### Value::removeUse()
```cpp
void Value::removeUse(Use* use) {
    uses_.erase(std::remove(uses_.begin(), uses_.end(), use), uses_.end());
}
```

### Value::replaceAllUsesWith()
```cpp
void Value::replaceAllUsesWith(Value* newValue) {
    // Make a copy since setValue will modify uses_
    auto usesCopy = uses_;
    for (Use* use : usesCopy) {
        use->setValue(newValue);
    }
}
```

### Use::setValue()
```cpp
void Use::setValue(Value* newValue) {
    if (value_) {
        value_->removeUse(this);
    }
    value_ = newValue;
    if (value_) {
        value_->addUse(this);
    }
}
```

## Test Progress Tracking

Run this to see how many tests pass:

```bash
./bin/test_volta --gtest_filter=ValueTest.* 2>&1 | grep -E "\[.*PASSED.*\]|\[.*FAILED.*\]" | tail -5
```

Target: 49 tests passing

## Common Errors

### "Expected: (ci) != (nullptr)"
→ Your factory method (e.g., `ConstantInt::get()`) is returning `nullptr` instead of a new object.

### "uses_.erase() fails"
→ Include `<algorithm>` for `std::remove`

### "Segmentation fault in Use destructor"
→ Check for null pointer before calling `value_->removeUse(this)`

### Circular reference in replaceAllUsesWith
→ Make a COPY of uses_ before iterating

## Files to Edit

- **src/IR/Value.cpp** - All your implementations go here
- No other files need modification!

## Review Criteria (When You Come Back)

I'll check:
1. ✓ All 49 tests pass
2. ✓ No memory leaks (we'll test with valgrind later)
3. ✓ Code follows C++ best practices
4. ✓ Proper null checking
5. ✓ Efficient use of data structures
6. ✓ Clean, readable code

## Tips

1. Start simple: Implement constants first (Phase 2)
2. Test incrementally: Run tests after each method
3. Read the header comments: They explain the design
4. Don't optimize prematurely: Get it working first
5. Use `std::to_string()` for number formatting
6. Use `"\"" + value_ + "\""` for string escaping

Good luck! 🚀
