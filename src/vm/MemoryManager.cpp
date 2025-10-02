#include "../include/vm/MemoryManager.hpp"
#include "../include/vm/Value.hpp"
#include <iostream>
#include <cstring>

namespace volta::vm {

// ========== Constructor/Destructor ==========

MemoryManager::MemoryManager(VM* vm, const GCConfig& config)
    : vm_(vm) {
    // Initialize garbage collector
    gc_ = std::make_unique<GarbageCollector>(vm, config);

    // Initialize allocators
    youngArena_ = std::make_unique<Arena>(config.youngGenMaxSize);
    oldAllocator_ = std::make_unique<FreeListAllocator>();
    largeAllocator_ = std::make_unique<PageAllocator>();
}

MemoryManager::~MemoryManager() {
    // Cleanup is handled by unique_ptr destructors
}

// ========== Allocation Interface ==========

StructObject* MemoryManager::allocateStruct(uint32_t typeId, uint32_t fieldCount) {
    size_t size = calculateStructSize(fieldCount);

    void* memory = allocate(size, ObjectHeader::ObjectKind::Struct, typeId);
    if (!memory) {
        return nullptr;
    }

    StructObject* obj = static_cast<StructObject*>(memory);

    // Initialize fields to null
    for (uint32_t i = 0; i < fieldCount; i++) {
        obj->fields[i] = Value::makeNull();
    }

    return obj;
}

ArrayObject* MemoryManager::allocateArray(uint32_t length) {
    size_t size = calculateArraySize(length);

    // Large arrays go to page allocator
    ObjectHeader::ObjectKind kind = ObjectHeader::ObjectKind::Array;
    void* memory = allocate(size, kind, 0);

    if (!memory) {
        return nullptr;
    }

    ArrayObject* arr = static_cast<ArrayObject*>(memory);
    arr->length = length;

    // Initialize elements to null
    for (uint32_t i = 0; i < length; i++) {
        arr->elements[i] = Value::makeNull();
    }

    return arr;
}

void* MemoryManager::allocate(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    if (logging_) {
        std::cout << "[MemoryManager] Allocating " << size << " bytes\n";
    }

    // Check if we should trigger GC before allocation
    checkGCTrigger(size);

    void* memory = nullptr;

    // Choose allocator based on size
    if (size >= LARGE_OBJECT_THRESHOLD) {
        // Large objects go to page allocator
        memory = allocateLarge(size, kind, typeId);
    } else {
        // Try young generation first (fast path)
        memory = allocateYoung(size, kind, typeId);

        // If young gen is full, try with GC
        if (!memory) {
            memory = allocateWithGC(size, kind, typeId);
        }
    }

    if (memory) {
        allocationCount_++;
    }

    return memory;
}

// ========== Garbage Collection Control ==========

size_t MemoryManager::collect() {
    if (!gc_) return 0;

    if (logging_) {
        std::cout << "[MemoryManager] Triggering GC collection\n";
    }

    // Trigger appropriate collection based on memory pressure
    const GCConfig& config = gc_->config();
    float oldUsage = static_cast<float>(oldAllocBytes_) / static_cast<float>(config.oldGenMaxSize);

    if (oldUsage >= config.oldGenThreshold) {
        return collectMajor();
    } else {
        return collectMinor();
    }
}

size_t MemoryManager::collectMinor() {
    if (!gc_) return 0;

    if (logging_) {
        std::cout << "[MemoryManager] Minor collection\n";
    }

    size_t freed = gc_->collectMinor();

    // Reset young arena after collection
    if (youngArena_) {
        // Note: Objects that survived should have been promoted
        // This is a simplified implementation
    }

    return freed;
}

size_t MemoryManager::collectMajor() {
    if (!gc_) return 0;

    if (logging_) {
        std::cout << "[MemoryManager] Major collection\n";
    }

    return gc_->collectMajor();
}

// ========== Memory Statistics ==========

size_t MemoryManager::totalAllocated() const {
    return youngAllocBytes_ + oldAllocBytes_ + largeAllocBytes_;
}

size_t MemoryManager::youngGenSize() const {
    return youngAllocBytes_;
}

size_t MemoryManager::oldGenSize() const {
    return oldAllocBytes_;
}

size_t MemoryManager::largeObjectSize() const {
    return largeAllocBytes_;
}

float MemoryManager::memoryUsagePercent() const {
    if (!gc_) return 0.0f;

    const GCConfig& config = gc_->config();
    size_t totalMax = config.totalMaxSize;

    if (totalMax == 0) return 0.0f;

    return static_cast<float>(totalAllocated()) / static_cast<float>(totalMax);
}

void MemoryManager::printStats(std::ostream& out) const {
    out << "MemoryManager Statistics:\n";
    out << "  Total allocations: " << allocationCount_ << "\n";
    out << "  Young generation: " << youngAllocBytes_ << " bytes\n";
    out << "  Old generation: " << oldAllocBytes_ << " bytes\n";
    out << "  Large objects: " << largeAllocBytes_ << " bytes\n";
    out << "  Total allocated: " << totalAllocated() << " bytes\n";
    out << "  Memory usage: " << (memoryUsagePercent() * 100.0f) << "%\n";

    if (gc_) {
        out << "\n";
        gc_->printStats(out);
    }
}

// ========== Configuration ==========

const GCConfig& MemoryManager::gcConfig() const {
    if (gc_) {
        return gc_->config();
    }
    static GCConfig defaultConfig;
    return defaultConfig;
}

void MemoryManager::setGCConfig(const GCConfig& config) {
    if (gc_) {
        gc_->setConfig(config);
    }

    // Resize allocators if needed
    // This is simplified - in production, you'd need to handle live objects
    if (config.youngGenMaxSize != youngArena_->capacity()) {
        youngArena_ = std::make_unique<Arena>(config.youngGenMaxSize);
    }
}

// ========== Write Barrier ==========

void MemoryManager::writeBarrier(void* oldGenObject, void* youngGenObject) {
    if (gc_) {
        gc_->writeBarrier(oldGenObject, youngGenObject);
    }
}

// ========== Debugging & Profiling ==========

bool MemoryManager::verifyHeap() const {
    // Basic heap verification
    // In a production system, this would check:
    // - All pointers are valid
    // - No corruption in object headers
    // - Free lists are consistent
    // - No memory leaks

    if (logging_) {
        std::cout << "[MemoryManager] Verifying heap integrity\n";
    }

    return true;
}

void MemoryManager::dumpHeap(std::ostream& out) const {
    out << "=== Heap Dump ===\n";

    if (youngArena_) {
        out << "Young Arena:\n";
        out << "  Used: " << youngArena_->used() << " / " << youngArena_->capacity() << " bytes\n";
    }

    if (oldAllocator_) {
        out << "\nOld Generation:\n";
        oldAllocator_->printStats(out);
    }

    if (largeAllocator_) {
        out << "\nLarge Objects:\n";
        out << "  Total: " << largeAllocator_->totalAllocated() << " bytes\n";
    }
}

void MemoryManager::resetCounters() {
    allocationCount_ = 0;
    youngAllocBytes_ = 0;
    oldAllocBytes_ = 0;
    largeAllocBytes_ = 0;

    if (gc_) {
        gc_->resetStats();
    }
}

// ========== Internal Allocation Helpers ==========

void* MemoryManager::allocateYoung(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    if (!youngArena_) {
        return nullptr;
    }

    void* memory = youngArena_->allocate(size);
    if (!memory) {
        return nullptr;
    }

    initializeObjectHeader(memory, size, kind, typeId, true);
    youngAllocBytes_ += size;

    return memory;
}

void* MemoryManager::allocateOld(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    if (!oldAllocator_) {
        return nullptr;
    }

    void* memory = oldAllocator_->allocate(size);
    if (!memory) {
        return nullptr;
    }

    initializeObjectHeader(memory, size, kind, typeId, false);
    oldAllocBytes_ += size;

    return memory;
}

void* MemoryManager::allocateLarge(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    if (!largeAllocator_) {
        return nullptr;
    }

    void* memory = largeAllocator_->allocate(size);
    if (!memory) {
        return nullptr;
    }

    initializeObjectHeader(memory, size, kind, typeId, false);
    largeAllocBytes_ += size;

    return memory;
}

void MemoryManager::initializeObjectHeader(void* obj, size_t size, ObjectHeader::ObjectKind kind,
                                            uint32_t typeId, bool isYoung) {
    if (!obj) return;

    ObjectHeader* header = static_cast<ObjectHeader*>(obj);
    header->kind = kind;
    header->size = static_cast<uint32_t>(size);
    header->typeId = typeId;

    // Set generation in GC metadata
    if (gc_) {
        setGeneration(obj, isYoung ? 0 : 1);
        setMarked(obj, false);
    }
}

void MemoryManager::checkGCTrigger(size_t requestedSize) {
    if (!autoGC_ || !gc_) {
        return;
    }

    // Check if we should trigger GC based on thresholds
    const GCConfig& config = gc_->config();

    // Check young generation threshold
    float youngUsage = static_cast<float>(youngAllocBytes_) / static_cast<float>(config.youngGenMaxSize);
    if (youngUsage >= config.youngGenThreshold) {
        if (logging_) {
            std::cout << "[MemoryManager] Young gen threshold reached ("
                      << (youngUsage * 100.0f) << "%)\n";
        }
        collectMinor();
    }

    // Check old generation threshold
    float oldUsage = static_cast<float>(oldAllocBytes_) / static_cast<float>(config.oldGenMaxSize);
    if (oldUsage >= config.oldGenThreshold) {
        if (logging_) {
            std::cout << "[MemoryManager] Old gen threshold reached ("
                      << (oldUsage * 100.0f) << "%)\n";
        }
        collectMajor();
    }
}

void* MemoryManager::allocateWithGC(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    // First attempt failed, try to GC and retry
    if (logging_) {
        std::cout << "[MemoryManager] Allocation failed, triggering GC\n";
    }

    collectMinor();

    // Retry young allocation
    void* memory = allocateYoung(size, kind, typeId);
    if (memory) {
        return memory;
    }

    // Still failed, try old generation
    if (logging_) {
        std::cout << "[MemoryManager] Young gen still full, using old gen\n";
    }

    return allocateOld(size, kind, typeId);
}

// ============================================================================
// Memory Utilities
// ============================================================================

size_t getObjectSize(void* obj) {
    if (!obj) return 0;
    ObjectHeader* header = static_cast<ObjectHeader*>(obj);
    return header->size;
}

ObjectHeader::ObjectKind getObjectKind(void* obj) {
    if (!obj) return ObjectHeader::ObjectKind::Struct;
    ObjectHeader* header = static_cast<ObjectHeader*>(obj);
    return header->kind;
}

bool isMarked(void* obj) {
    if (!obj) return false;
    // GC metadata would be stored after ObjectHeader
    // This is a simplified implementation
    return false;
}

void setMarked(void* obj, bool marked) {
    if (!obj) return;
    // GC metadata would be stored after ObjectHeader
    // This is a simplified implementation
}

uint8_t getGeneration(void* obj) {
    if (!obj) return 0;
    // GC metadata would be stored after ObjectHeader
    // This is a simplified implementation
    return 0;
}

void setGeneration(void* obj, uint8_t generation) {
    if (!obj) return;
    // GC metadata would be stored after ObjectHeader
    // This is a simplified implementation
}

uint8_t getAge(void* obj) {
    if (!obj) return 0;
    // GC metadata would be stored after ObjectHeader
    return 0;
}

void incrementAge(void* obj) {
    if (!obj) return;
    // GC metadata would be stored after ObjectHeader
}

size_t calculateStructSize(uint32_t fieldCount) {
    // ObjectHeader + array of Values
    // Note: StructObject has a flexible array member (fields[1])
    // So we need: sizeof(ObjectHeader) + sizeof(Value) * fieldCount
    return sizeof(ObjectHeader) + sizeof(Value) * fieldCount;
}

size_t calculateArraySize(uint32_t elementCount) {
    // ObjectHeader + length field + array of Values
    // ArrayObject has: ObjectHeader + uint32_t length + elements[1]
    return sizeof(ObjectHeader) + sizeof(uint32_t) + sizeof(Value) * elementCount;
}

} // namespace volta::vm