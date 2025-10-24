#include "MIR/EscapeAnalysis.hpp"
#include <iostream>
#include <cassert>

namespace MIR {


void EscapeInfo::addAllocation(const std::string& name, size_t size, const Type::Type* type) {
    allocations[name] = AllocationInfo(name, size, type);
}

void EscapeInfo::markEscape(const std::string& valueName, EscapeReason reason) {
    valueStatus[valueName] = (reason == EscapeReason::DoesNotEscape)
                            ? EscapeStatus::DoesNotEscape
                            : EscapeStatus::Escapes;

    if (allocations.contains(valueName)) {
        allocations.at(valueName).reason = reason;
        allocations.at(valueName).status = valueStatus[valueName];
    }
}

void EscapeInfo::markDoesNotEscape(const std::string& valueName) {
    markEscape(valueName, EscapeReason::DoesNotEscape);
}

bool EscapeInfo::escapes(const std::string& valueName) const {
    auto it = valueStatus.find(valueName);
    if (it != valueStatus.end()) {
        return it->second == EscapeStatus::Escapes;
    }

    // Fallback: check allocations
    if (allocations.contains(valueName)) {
        auto reason = allocations.at(valueName).reason;
        // Unknown and DoesNotEscape both mean it doesn't escape
        return (reason != EscapeReason::DoesNotEscape && reason != EscapeReason::Unknown);
    }

    return false;
}

EscapeStatus EscapeInfo::getStatus(const std::string& valueName) const {
    auto it = valueStatus.find(valueName);
    if (it != valueStatus.end()) {
        return it->second;
    }

    // Fallback: check allocations
    if (allocations.contains(valueName)) {
        return allocations.at(valueName).status;
    }

    return EscapeStatus::Unknown;
}

const AllocationInfo* EscapeInfo::getAllocationInfo(const std::string& name) const {
    auto it = allocations.find(name);
    return it == allocations.end() ? nullptr : &(it->second);
}

void EscapeInfo::clear() {
    allocations.clear();
    valueStatus.clear();
}

void EscapeInfo::dump() const {
    // TODO: Print escape information for debugging
    std::cout << "=== Escape Analysis Results ===" << std::endl;
    std::cout << "Size threshold: " << sizeThreshold << " bytes" << std::endl;
    // Print allocations...
}

// ============================================================================
// EscapeAnalyzer Implementation
// ============================================================================

void EscapeAnalyzer::analyze(Function& function) {
    findAllocations(function);
    checkSizes();
    markImmediateEscapes(function);
    propagateEscapes(function);

    // Mark any remaining allocations that don't escape as safe
    for (const auto& [name, allocInfo] : escapeInfo.getAllocations()) {
        // Check valueStatus first (most up-to-date)
        if (escapeInfo.getStatus(name) == EscapeStatus::Unknown) {
            escapeInfo.markDoesNotEscape(name);
        }
    }
}

void EscapeAnalyzer::findAllocations(Function& function) {
    for (const auto& block: function.blocks) {
        for (const auto& instr: block.instructions) {
            if (instr.opcode == Opcode::Alloca) {
                std::string allocName = instr.result.name;
                const Type::Type* allocType = instr.result.type;

                const Type::Type* actualType = allocType;
                if (allocType->kind == Type::TypeKind::Pointer) {
                    auto* ptrType = static_cast<const Type::PointerType*>(allocType);
                    actualType = ptrType->pointeeType;
                }

                size_t size = getTypeSize(actualType);
                escapeInfo.addAllocation(allocName, size, actualType);
            }
        }
    }
}

void EscapeAnalyzer::markImmediateEscapes(Function& function) {
    for (const auto& block : function.blocks) {
        for (const auto& inst : block.instructions) {
            checkInstruction(inst);
        }
        checkTerminator(block.terminator);
    }
}

void EscapeAnalyzer::checkInstruction(const Instruction& inst) {
    if (inst.opcode == Opcode::Call) {
        // All arguments escape (conservative)
        for (const auto& arg : inst.operands) {
            if (!arg.isConstant()) {
                escapeInfo.markEscape(arg.name, EscapeReason::PassedToFunction);
            }
        }
    }
    if (inst.opcode == Opcode::Store) {
        if (inst.operands.size() >= 2) {
            if (!inst.operands[0].isConstant()) {
                escapeInfo.markEscape(inst.operands[0].name,
                                    EscapeReason::StoredToHeap);
            }
        }
    }
}

void EscapeAnalyzer::checkTerminator(const Terminator& term) {
    if (term.kind == TerminatorKind::Return) {
        for (const auto& val : term.operands) {
            if (!val.isConstant()) {
                escapeInfo.markEscape(val.name, EscapeReason::ReturnedFromFunction);
            }
        }
    }
}

void EscapeAnalyzer::propagateEscapes(Function& function) {
    bool changed = true;
    int iteration = 0;
    const int MAX_ITERATIONS = 100; // Safety limit

    while (changed && iteration < MAX_ITERATIONS) {
        changed = false;
        iteration++;

        for (const auto& block : function.blocks) {
            for (const auto& inst : block.instructions) {

                // Pattern 1: GEP/GetFieldPtr - backward propagation
                // If result escapes, operand must escape
                if (inst.opcode == Opcode::GetElementPtr ||
                    inst.opcode == Opcode::GetFieldPtr) {

                    if (escapeInfo.escapes(inst.result.name)) {
                        // The derived pointer escapes
                        // Mark the base pointer as escaping
                        if (!inst.operands.empty()) {
                            const auto& base = inst.operands[0];
                            if (!base.isConstant() && !escapeInfo.escapes(base.name)) {
                                escapeInfo.markEscape(base.name,
                                    EscapeReason::DerivedPointerEscapes);
                                changed = true;
                            }
                        }
                    }
                }

                // Pattern 2: Forward propagation for GEP/GetFieldPtr
                // If base escapes, derived pointer escapes
                if (inst.opcode == Opcode::GetElementPtr ||
                    inst.opcode == Opcode::GetFieldPtr) {

                    if (!inst.operands.empty()) {
                        const auto& base = inst.operands[0];
                        if (!base.isConstant() && escapeInfo.escapes(base.name)) {
                            // Base escapes, so derived must escape
                            if (!escapeInfo.escapes(inst.result.name)) {
                                escapeInfo.markEscape(inst.result.name,
                                    EscapeReason::DerivedPointerEscapes);
                                changed = true;
                            }
                        }
                    }
                }

                // Pattern 3a: Store - if destination escapes, value escapes
                if (inst.opcode == Opcode::Store && inst.operands.size() >= 2) {
                    const auto& value = inst.operands[0];
                    const auto& dest = inst.operands[1];

                    if (!dest.isConstant() && escapeInfo.escapes(dest.name)) {
                        // Storing to escaping location
                        if (!value.isConstant() && !escapeInfo.escapes(value.name)) {
                            escapeInfo.markEscape(value.name,
                                EscapeReason::StoredToHeap);
                            changed = true;
                        }
                    }
                }

                // Pattern 3b: Store - if value escapes, destination escapes (backward)
                if (inst.opcode == Opcode::Store && inst.operands.size() >= 2) {
                    const auto& value = inst.operands[0];
                    const auto& dest = inst.operands[1];

                    if (!value.isConstant() && escapeInfo.escapes(value.name)) {
                        // Storing escaping value
                        if (!dest.isConstant() && !escapeInfo.escapes(dest.name)) {
                            escapeInfo.markEscape(dest.name,
                                EscapeReason::StoredToHeap);
                            changed = true;
                        }
                    }
                }

                // Pattern 4: Load - forward propagation
                // If loading from escaping pointer, result might escape
                // (Conservative: if source escapes, loaded value escapes)
                if (inst.opcode == Opcode::Load) {
                    if (!inst.operands.empty()) {
                        const auto& src = inst.operands[0];
                        if (!src.isConstant() && escapeInfo.escapes(src.name)) {
                            if (!escapeInfo.escapes(inst.result.name)) {
                                escapeInfo.markEscape(inst.result.name,
                                    EscapeReason::DerivedPointerEscapes);
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

std::vector<std::string> EscapeAnalyzer::findDerivedValues(
    const Function& function,
    const std::string& valueName) {
    // TODO: Find all values derived from valueName
    // Hint: Look for Load, GEP, GetFieldPtr that use valueName
    return {};  // Placeholder
}

void EscapeAnalyzer::checkSizes() {
    for (const auto& [name, allocInfo] : escapeInfo.getAllocations()) {
        if (allocInfo.size > escapeInfo.getSizeThreshold()) {
            escapeInfo.markEscape(name, EscapeReason::TooLarge);
        }
    }
}

void EscapeAnalyzer::addToWorklist(const std::string& valueName) {
    worklist.push_back(valueName);
}

bool EscapeAnalyzer::isAllocation(const std::string& valueName) const {
    // TODO: Check if value is from an allocation
    return false;  // Placeholder
}

std::string EscapeAnalyzer::getRootAllocation(const std::string& valueName) const {
    // TODO: Get the root allocation for a derived pointer
    // Hint: For now, just return valueName (you can improve this later)
    return valueName;  // Placeholder
}

size_t EscapeAnalyzer::getTypeSize(const Type::Type* type) const {
    if (!type) return 0;

    switch (type->kind) {
        case Type::TypeKind::Primitive: {
            auto* prim = static_cast<const Type::PrimitiveType*>(type);
            switch (prim->kind) {
                case Type::PrimitiveKind::I8:
                case Type::PrimitiveKind::U8:
                case Type::PrimitiveKind::Bool:
                    return 1;
                case Type::PrimitiveKind::I16:
                case Type::PrimitiveKind::U16:
                    return 2;
                case Type::PrimitiveKind::I32:
                case Type::PrimitiveKind::U32:
                case Type::PrimitiveKind::F32:
                    return 4;
                case Type::PrimitiveKind::I64:
                case Type::PrimitiveKind::U64:
                case Type::PrimitiveKind::F64:
                    return 8;
                case Type::PrimitiveKind::String:
                    return 8;  // Pointer size
                default:
                    return 0;
            }
        }
        case Type::TypeKind::Pointer:
            return 8;  // 64-bit pointer
        case Type::TypeKind::Array: {
            auto* arr = static_cast<const Type::ArrayType*>(type);
            // Calculate total number of elements: product of all dimensions
            size_t totalElements = 1;
            for (int dim : arr->dimensions) {
                totalElements *= dim;
            }
            return totalElements * getTypeSize(arr->elementType);
        }
        case Type::TypeKind::Struct: {
            auto* structType = static_cast<const Type::StructType*>(type);
            size_t totalSize = 0;
            for (const auto& field : structType->fields) {
                totalSize += getTypeSize(field.type);
            }
            return totalSize;
        }
        default:
            return 0;
    }
}

// ============================================================================
// AllocationTransformer Implementation
// ============================================================================

AllocationTransformer::AllocationTransformer(const EscapeInfo& info)
    : escapeInfo(info) {
    // TODO: Implement constructor
}

void AllocationTransformer::transform(Function& function) {
    for (auto& block : function.blocks) {
        for (auto& inst : block.instructions) {
            transformInstruction(inst);
        }
    }
}

void AllocationTransformer::transformInstruction(Instruction& inst) {
    if (inst.opcode == Opcode::Alloca) {
        inst.opcode = decideAllocationKind(inst.result.name);
    }
}

Opcode AllocationTransformer::decideAllocationKind(const std::string& allocaName) const {
    if (escapeInfo.escapes(allocaName)) {
        return Opcode::HAlloca;
    } else {
        return Opcode::SAlloca;
    }
}

} // namespace MIR
