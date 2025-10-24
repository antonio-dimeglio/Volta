#pragma once

#include "MIR/MIR.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace MIR {

// ============================================================================
// Escape Analysis for MIR
// ============================================================================
// Determines which allocations can be stack-allocated vs heap-allocated
//
// Algorithm:
// 1. Find all Alloca instructions (undecided allocations)
// 2. Track how pointers from these allocas are used
// 3. Mark allocations that "escape" (must be on heap)
// 4. Transform Alloca → SAlloca (stack) or HAlloca (heap)
//
// A value ESCAPES if:
// - Returned from function
// - Stored to heap memory
// - Passed to function call
// - Derived pointer escapes
//
// Conservative approach: When in doubt, use heap.
// ============================================================================

// Escape status for a value
enum class EscapeStatus : uint8_t {
    DoesNotEscape,  // Safe for stack allocation
    Escapes,        // Must be heap allocated
    Unknown         // Not yet analyzed
};

// Reason why a value escapes (for debugging/reporting)
enum class EscapeReason : uint8_t {
    DoesNotEscape,
    TooLarge,               // Size exceeds threshold
    ReturnedFromFunction,   // Returned via ret instruction
    StoredToHeap,           // Stored to heap-allocated memory
    PassedToFunction,       // Passed as argument to function call
    DerivedPointerEscapes,  // A pointer derived from this value escapes
    Unknown
};

// Information about a single allocation site
struct AllocationInfo {
    std::string allocaName;      // Name of the alloca result (e.g., "%ptr")
    size_t size;                 // Size in bytes
    const Type::Type* type;      // Type being allocated
    EscapeStatus status;         // Does it escape?
    EscapeReason reason;         // Why does it escape?

    AllocationInfo()
        : size(0), type(nullptr), status(EscapeStatus::Unknown),
          reason(EscapeReason::Unknown) {}

    AllocationInfo(std::string name, size_t sz, const Type::Type* ty)
        : allocaName(std::move(name)), size(sz), type(ty),
          status(EscapeStatus::Unknown), reason(EscapeReason::Unknown) {}
};

// Tracks escape information for all values in a function
class EscapeInfo {
private:
    // Maps allocation name → allocation information
    std::unordered_map<std::string, AllocationInfo> allocations;

    // Maps any value name → escape status (for tracking derived pointers)
    std::unordered_map<std::string, EscapeStatus> valueStatus;

    // Stack allocation size threshold (bytes)
    size_t sizeThreshold;

public:
    explicit EscapeInfo(size_t threshold = 64) : sizeThreshold(threshold) {}

    // Register an allocation site
    void addAllocation(const std::string& name, size_t size, const Type::Type* type);

    // Mark a value as escaping
    void markEscape(const std::string& valueName, EscapeReason reason);

    // Mark a value as not escaping
    void markDoesNotEscape(const std::string& valueName);

    // Check if a value escapes
    bool escapes(const std::string& valueName) const;

    // Get escape status for a value
    EscapeStatus getStatus(const std::string& valueName) const;

    // Get allocation info (returns nullptr if not an allocation)
    const AllocationInfo* getAllocationInfo(const std::string& name) const;

    // Get all allocations
    const std::unordered_map<std::string, AllocationInfo>& getAllocations() const {
        return allocations;
    }

    // Clear all data
    void clear();

    // Get size threshold
    size_t getSizeThreshold() const { return sizeThreshold; }

    // Set size threshold
    void setSizeThreshold(size_t threshold) { sizeThreshold = threshold; }

    // Debug: print escape info
    void dump() const;
};

// Main escape analysis pass
class EscapeAnalyzer {
private:
    EscapeInfo& escapeInfo;

    // Worklist for iterative escape propagation
    std::vector<std::string> worklist;

    // Track which values have been processed
    std::unordered_set<std::string> processed;

public:
    explicit EscapeAnalyzer(EscapeInfo& info) : escapeInfo(info) {};

    // Analyze a single function
    void analyze(Function& function);

private:
    // ===== Phase 1: Find Allocations =====

    // Find all Alloca instructions and register them
    void findAllocations(Function& function);

    // ===== Phase 2: Mark Immediate Escapes =====

    // Mark values that obviously escape (returns, function calls)
    void markImmediateEscapes(Function& function);

    // Check if instruction causes escape
    void checkInstruction(const Instruction& inst);

    // Check if terminator causes escape
    void checkTerminator(const Terminator& term);

    // ===== Phase 3: Propagate Escapes =====

    // Propagate escapes through use-def chains
    // If %b = load %a and %b escapes, then %a escapes
    void propagateEscapes(Function& function);

    // Find all values derived from a given value
    std::vector<std::string> findDerivedValues(const Function& function,
                                               const std::string& valueName);

    // ===== Phase 4: Size Check =====

    // Mark allocations that are too large
    void checkSizes();

    // ===== Helper Methods =====

    // Add value to worklist if not already processed
    void addToWorklist(const std::string& valueName);

    // Check if value is from an allocation
    bool isAllocation(const std::string& valueName) const;

    // Get the root allocation for a derived pointer
    // E.g., if %ptr2 = GEP %ptr1, returns %ptr1's allocation
    std::string getRootAllocation(const std::string& valueName) const;

    // Get size of a type
    size_t getTypeSize(const Type::Type* type) const;
};

// Transforms Alloca instructions into SAlloca or HAlloca based on escape analysis
class AllocationTransformer {
private:
    const EscapeInfo& escapeInfo;

public:
    explicit AllocationTransformer(const EscapeInfo& info);

    // Transform all Alloca instructions in a function
    void transform(Function& function);

private:
    // Transform a single instruction
    void transformInstruction(Instruction& inst);

    // Decide allocation kind based on escape info
    Opcode decideAllocationKind(const std::string& allocaName) const;
};

} // namespace MIR
