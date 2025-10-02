#include "../include/vm/GC.hpp"
#include "../include/vm/VM.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace volta::vm {

// ========== Constructor/Destructor ==========

GarbageCollector::GarbageCollector(VM* vm, const GCConfig& config)
    : vm_(vm), config_(config) {
    youngGenObjects_.reserve(1024);  
    oldGenObjects_.reserve(512);
    
    rememberedSet_.reserve(256);
    
    stats_ = GCStats{};
    
    if (logging_) {
        std::cout << "[GC] Initialized with young gen max: " 
                  << config_.youngGenMaxSize / 1024 << "KB, "
                  << "old gen max: " << config_.oldGenMaxSize / 1024 << "KB\n";
    }
}

GarbageCollector::~GarbageCollector() {
    // Free all objects in both generations
    for (void* obj : youngGenObjects_) {
        free(obj);
    }
    for (void* obj : oldGenObjects_) {
        free(obj);
    }
}

// ========== Allocation ==========

StructObject* GarbageCollector::allocateStruct(uint32_t typeId, uint32_t fieldCount) {
    // Calculate size: GCHeader + StructObject with fieldCount fields
    // StructObject already includes ObjectHeader, so we need:
    // sizeof(GCHeader) + sizeof(ObjectHeader) + fieldCount * sizeof(Value)
    // But StructObject has fields[1] which accounts for 1 Value, so:
    // sizeof(GCHeader) + sizeof(StructObject) + (fieldCount > 0 ? fieldCount - 1 : 0) * sizeof(Value)

    size_t gcHeaderSize = sizeof(GCHeader);
    size_t baseStructSize = sizeof(StructObject);  // Includes ObjectHeader + 1 Value
    size_t extraFieldsSize = (fieldCount > 0 ? fieldCount - 1 : 0) * sizeof(Value);
    size_t totalSize = gcHeaderSize + baseStructSize + extraFieldsSize;

    if (!collecting_ && shouldCollectMinor()) {
        if (logging_) {
            std::cout << "[GC] Young gen threshold reached, triggering minor GC\n";
        }
        collectMinor();
    }

    void* memory = malloc(totalSize);
    if (!memory) {
        // Out of memory - try a major GC to free space
        if (!collecting_) {
            if (logging_) {
                std::cout << "[GC] Allocation failed, triggering major GC\n";
            }
            collectMajor();
            
            // Retry allocation after GC
            memory = malloc(totalSize);
            if (!memory) {
                // Still can't allocate - fail
                return nullptr;
            }
        } else {
            // Already collecting - can't recurse
            return nullptr;
        }
    }

    GCHeader* gcHeader = static_cast<GCHeader*>(memory);
    gcHeader->marked = 0;           // Not marked initially
    gcHeader->generation = 0;       // Start in young generation
    gcHeader->age = 0;              // Age 0 (just born)
    gcHeader->next = nullptr;       // No next pointer yet


    uint8_t* headerPtr = static_cast<uint8_t*>(memory) + gcHeaderSize;
    ObjectHeader* objHeader = reinterpret_cast<ObjectHeader*>(headerPtr);
    objHeader->kind = ObjectHeader::ObjectKind::Struct;
    objHeader->size = totalSize;
    objHeader->typeId = typeId;
    
    // 6. Get pointer to the StructObject (same as ObjectHeader location)
    StructObject* obj = reinterpret_cast<StructObject*>(headerPtr);
    
    // 7. Initialize fields to null
    for (uint32_t i = 0; i < fieldCount; i++) {
        obj->fields[i] = Value::makeNull();
    }
    
    // 8. Track this object in young generation
    youngGenObjects_.push_back(memory);  // Store the base pointer (with GCHeader)
    
    // 9. Update statistics
    stats_.youngGenSize += totalSize;
    stats_.youngGenObjects++;
    stats_.totalAllocated += totalSize;
    
    if (logging_) {
        std::cout << "[GC] Allocated struct (typeId=" << typeId 
                  << ", fields=" << fieldCount 
                  << ", size=" << totalSize << " bytes) in young gen\n";
    }
    
    return obj;
}

ArrayObject* GarbageCollector::allocateArray(uint32_t length) {
    // Calculate size: GCHeader + ArrayObject with length elements
    // ArrayObject already includes ObjectHeader + uint32_t length + elements[1]
    // So we need: sizeof(GCHeader) + sizeof(ArrayObject) + (length > 0 ? length - 1 : 0) * sizeof(Value)

    size_t gcHeaderSize = sizeof(GCHeader);
    size_t baseArraySize = sizeof(ArrayObject);  // Includes ObjectHeader + length + 1 Value
    size_t extraElementsSize = (length > 0 ? length - 1 : 0) * sizeof(Value);
    size_t totalSize = gcHeaderSize + baseArraySize + extraElementsSize;
    
    // 2. Check GC threshold
    if (!collecting_ && shouldCollectMinor()) {
        collectMinor();
    }
    
    // 3. Allocate
    void* memory = malloc(totalSize);
    if (!memory) {
        if (!collecting_) {
            collectMajor();
            memory = malloc(totalSize);
            if (!memory) return nullptr;
        } else {
            return nullptr;
        }
    }
    
    // 4. Initialize GC header
    GCHeader* gcHeader = static_cast<GCHeader*>(memory);
    gcHeader->marked = 0;
    gcHeader->generation = 0;
    gcHeader->age = 0;
    gcHeader->next = nullptr;
    
    // 5. Initialize ObjectHeader
    uint8_t* headerPtr = static_cast<uint8_t*>(memory) + gcHeaderSize;
    ObjectHeader* objHeader = reinterpret_cast<ObjectHeader*>(headerPtr);
    objHeader->kind = ObjectHeader::ObjectKind::Array;
    objHeader->size = totalSize;
    objHeader->typeId = 0;  // Arrays don't have typeIds
    
    // 6. Get ArrayObject pointer
    ArrayObject* arr = reinterpret_cast<ArrayObject*>(headerPtr);
    arr->length = length;
    
    // 7. Initialize elements
    for (uint32_t i = 0; i < length; i++) {
        arr->elements[i] = Value::makeNull();
    }
    
    // 8. Track in young generation
    youngGenObjects_.push_back(memory);
    
    // 9. Update stats
    stats_.youngGenSize += totalSize;
    stats_.youngGenObjects++;
    stats_.totalAllocated += totalSize;
    
    if (logging_) {
        std::cout << "[GC] Allocated array (length=" << length 
                  << ", size=" << totalSize << " bytes) in young gen\n";
    }
    
    return arr;
}

void* GarbageCollector::allocate(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    return nullptr;
}

// ========== Collection ==========

size_t GarbageCollector::collectMinor() {
    if (collecting_) return 0; // Prevent recursive collection

    collecting_ = true;
    auto startTime = std::chrono::steady_clock::now();

    if (logging_) {
        std::cout << "[GC] Minor collection started (young gen: "
                  << stats_.youngGenSize / 1024 << "KB, "
                  << stats_.youngGenObjects << " objects)\n";
    }

    // Mark phase: mark only young generation + remembered set
    mark(false);

    // Promote old objects before sweeping
    promoteObjects();

    // Sweep phase: free unmarked young generation objects
    size_t bytesFreed = sweep(false);

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    updateStats(true, bytesFreed, duration.count());

    if (logging_) {
        std::cout << "[GC] Minor collection completed: freed "
                  << bytesFreed / 1024 << "KB in "
                  << duration.count() << "ms\n";
        std::cout << "[GC] Young gen after: "
                  << stats_.youngGenSize / 1024 << "KB, "
                  << stats_.youngGenObjects << " objects\n";
    }

    collecting_ = false;
    return bytesFreed;
}

size_t GarbageCollector::collectMajor() {
    if (collecting_) return 0; // Prevent recursive collection

    collecting_ = true;
    auto startTime = std::chrono::steady_clock::now();

    if (logging_) {
        std::cout << "[GC] Major collection started (total heap: "
                  << (stats_.youngGenSize + stats_.oldGenSize) / 1024 << "KB)\n";
    }

    // Mark phase: mark entire heap
    mark(true);

    // Sweep phase: free unmarked objects in both generations
    size_t bytesFreed = sweep(true);

    // Clear remembered set after full collection
    rememberedSet_.clear();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    updateStats(false, bytesFreed, duration.count());

    if (logging_) {
        std::cout << "[GC] Major collection completed: freed "
                  << bytesFreed / 1024 << "KB in "
                  << duration.count() << "ms\n";
        std::cout << "[GC] Heap after: young="
                  << stats_.youngGenSize / 1024 << "KB, old="
                  << stats_.oldGenSize / 1024 << "KB\n";
    }

    collecting_ = false;
    return bytesFreed;
}

bool GarbageCollector::collectIfNeeded() {
    if (collecting_) return false;

    if (shouldCollectMajor()) {
        collectMajor();
        return true;
    } else if (shouldCollectMinor()) {
        collectMinor();
        return true;
    }

    return false;
}

void GarbageCollector::forceCollect() {
    collectMajor();
}

// ========== Write Barrier ==========

void GarbageCollector::writeBarrier(void* oldGenObject, void* youngGenObject) {
    if (!oldGenObject || !youngGenObject) return;

    // Check if this is actually an old->young reference
    if (isOld(oldGenObject) && isYoung(youngGenObject)) {
        // Add old object to remembered set
        rememberedSet_.insert(oldGenObject);
    }
}

// ========== GC Roots ==========

void GarbageCollector::addRoot(Value* root) {
    if (root) {
        roots_.push_back(root);
    }
}

void GarbageCollector::removeRoot(Value* root) {
    auto it = std::find(roots_.begin(), roots_.end(), root);
    if (it != roots_.end()) {
        roots_.erase(it);
    }
}

void GarbageCollector::clearRoots() {
    roots_.clear();
}

// ========== Configuration ==========

void GarbageCollector::setConfig(const GCConfig& config) {
    config_ = config;
}

// ========== Statistics & Debugging ==========

void GarbageCollector::resetStats() {
    stats_ = GCStats{};
}

bool GarbageCollector::isYoung(void* obj) const {
    if (!obj) return false;

    GCHeader* gcHeader = getGCHeader(obj);
    return gcHeader && gcHeader->generation == 0;
}

bool GarbageCollector::isOld(void* obj) const {
    if (!obj) return false;

    GCHeader* gcHeader = getGCHeader(obj);
    return gcHeader && gcHeader->generation == 1;
}

void GarbageCollector::printStats(std::ostream& out) const {
    out << "=== GC Statistics ===\n";
    out << "Collections:\n";
    out << "  Minor: " << stats_.minorCollections << "\n";
    out << "  Major: " << stats_.majorCollections << "\n";
    out << "\nMemory:\n";
    out << "  Young gen: " << stats_.youngGenSize / 1024 << " KB ("
        << stats_.youngGenObjects << " objects)\n";
    out << "  Old gen: " << stats_.oldGenSize / 1024 << " KB ("
        << stats_.oldGenObjects << " objects)\n";
    out << "  Total: " << (stats_.youngGenSize + stats_.oldGenSize) / 1024 << " KB\n";
    out << "\nLifetime:\n";
    out << "  Allocated: " << stats_.totalAllocated / 1024 << " KB\n";
    out << "  Freed: " << stats_.totalFreed / 1024 << " KB\n";
    out << "  Promoted: " << stats_.objectsPromoted << " objects\n";
    out << "\nPerformance:\n";
    out << "  Total GC time: " << stats_.totalGCTimeMs << " ms\n";
    out << "  Last GC time: " << stats_.lastGCTimeMs << " ms\n";
    if (stats_.minorCollections + stats_.majorCollections > 0) {
        uint64_t totalCollections = stats_.minorCollections + stats_.majorCollections;
        out << "  Average GC time: " << (stats_.totalGCTimeMs / totalCollections) << " ms\n";
    }
}

// ========== Mark Phase ==========

void GarbageCollector::mark(bool fullHeap) {
    // Mark all reachable objects starting from roots
    markRoots();

    // For minor GC, also mark from remembered set (old->young refs)
    if (!fullHeap) {
        markRememberedSet();
    }
}

void GarbageCollector::markValue(const Value& value) {
    // Only mark if it's an object reference
    if (value.type == ValueType::Object && value.asObject != nullptr) {
        markObject(value.asObject);
    }
}

void GarbageCollector::markObject(void* obj) {
    if (!obj) return;

    // Get the GC header for this object
    GCHeader* gcHeader = getGCHeader(obj);
    if (!gcHeader) return;

    // Already marked? Skip (prevents infinite loops on circular references)
    if (gcHeader->marked) return;

    // Mark this object as reachable
    gcHeader->marked = 1;

    // Get the object header to determine type
    ObjectHeader* objHeader = static_cast<ObjectHeader*>(obj);

    // Recursively mark all objects this object references
    if (objHeader->kind == ObjectHeader::ObjectKind::Struct) {
        StructObject* structObj = static_cast<StructObject*>(obj);

        // Calculate field count from size
        size_t totalHeaderSize = sizeof(GCHeader) + sizeof(ObjectHeader);
        size_t fieldsSize = objHeader->size - totalHeaderSize;
        size_t fieldCount = fieldsSize / sizeof(Value);

        // Mark all fields
        for (size_t i = 0; i < fieldCount; i++) {
            markValue(structObj->fields[i]);
        }
    } else if (objHeader->kind == ObjectHeader::ObjectKind::Array) {
        ArrayObject* arrayObj = static_cast<ArrayObject*>(obj);

        // Mark all elements
        for (uint32_t i = 0; i < arrayObj->length; i++) {
            markValue(arrayObj->elements[i]);
        }
    }
}

void GarbageCollector::markRoots() {
    // Mark evaluation stack
    const Value* stack = vm_->getStack();
    size_t stackTop = vm_->getStackTop();
    for (size_t i = 0; i < stackTop; i++) {
        markValue(stack[i]);
    }

    // Mark local stack (all active frames)
    const Value* localStack = vm_->getLocalStack();
    size_t localStackTop = vm_->getLocalStackTop();
    for (size_t i = 0; i < localStackTop; i++) {
        markValue(localStack[i]);
    }

    // Mark globals
    const Value* globals = vm_->getGlobals();
    size_t globalsSize = vm_->getGlobalsSize();
    for (size_t i = 0; i < globalsSize; i++) {
        markValue(globals[i]);
    }

    // Mark custom roots (if any were registered)
    for (Value* root : roots_) {
        if (root) {
            markValue(*root);
        }
    }
}

void GarbageCollector::markRememberedSet() {
    // Mark objects in remembered set (old gen objects that reference young gen)
    // This is critical for minor GC correctness
    for (void* oldGenObj : rememberedSet_) {
        markObject(oldGenObj);
    }
}

// ========== Sweep Phase ==========

size_t GarbageCollector::sweep(bool fullHeap) {
    size_t totalFreed = 0;

    // Sweep young generation
    totalFreed += sweepGeneration(youngGenObjects_, true);

    // If full heap, also sweep old generation
    if (fullHeap) {
        totalFreed += sweepGeneration(oldGenObjects_, false);
    }

    return totalFreed;
}

size_t GarbageCollector::sweepGeneration(std::vector<void*>& generation, bool isYoung) {
    size_t bytesFreed = 0;
    size_t objectsFreed = 0;

    // Iterate through all objects in this generation
    auto it = generation.begin();
    while (it != generation.end()) {
        void* basePtr = *it;
        GCHeader* gcHeader = static_cast<GCHeader*>(basePtr);

        // Get object header (skip past GCHeader)
        uint8_t* objPtr = static_cast<uint8_t*>(basePtr) + sizeof(GCHeader);
        ObjectHeader* objHeader = reinterpret_cast<ObjectHeader*>(objPtr);

        if (gcHeader->marked) {
            // Object is reachable - keep it alive
            // Reset mark bit for next collection
            gcHeader->marked = 0;

            // Increment age for young generation objects
            if (isYoung) {
                if (gcHeader->age < 63) { // Max age (6 bits)
                    gcHeader->age++;
                }
            }

            ++it; // Keep this object
        } else {
            // Object is unreachable - free it
            bytesFreed += objHeader->size;
            objectsFreed++;

            freeObject(basePtr);

            // Remove from generation list
            it = generation.erase(it);
        }
    }

    // Update statistics
    if (isYoung) {
        stats_.youngGenSize -= bytesFreed;
        stats_.youngGenObjects -= objectsFreed;
    } else {
        stats_.oldGenSize -= bytesFreed;
        stats_.oldGenObjects -= objectsFreed;
    }

    return bytesFreed;
}

void GarbageCollector::freeObject(void* basePtr) {
    // basePtr points to the start of allocated memory (GCHeader)
    free(basePtr);
}

// ========== Promotion ==========

void GarbageCollector::promoteObjects() {
    std::vector<void*> toPromote;

    // Find young gen objects that should be promoted
    for (void* basePtr : youngGenObjects_) {
        GCHeader* gcHeader = static_cast<GCHeader*>(basePtr);

        // Only promote marked (live) objects that are old enough
        if (gcHeader->marked && gcHeader->age >= config_.promotionAge) {
            toPromote.push_back(basePtr);
        }
    }

    // Promote the selected objects
    for (void* basePtr : toPromote) {
        promoteObject(basePtr);
    }

    if (logging_ && !toPromote.empty()) {
        std::cout << "[GC] Promoted " << toPromote.size() << " objects to old generation\n";
    }
}

void GarbageCollector::promoteObject(void* basePtr) {
    GCHeader* gcHeader = static_cast<GCHeader*>(basePtr);

    // Get object header to get size
    uint8_t* objPtr = static_cast<uint8_t*>(basePtr) + sizeof(GCHeader);
    ObjectHeader* objHeader = reinterpret_cast<ObjectHeader*>(objPtr);

    // Update GC metadata
    gcHeader->generation = 1; // Move to old generation
    gcHeader->age = 0;        // Reset age

    // Move from young to old generation list
    auto it = std::find(youngGenObjects_.begin(), youngGenObjects_.end(), basePtr);
    if (it != youngGenObjects_.end()) {
        youngGenObjects_.erase(it);
        oldGenObjects_.push_back(basePtr);

        // Update statistics
        stats_.youngGenSize -= objHeader->size;
        stats_.youngGenObjects--;
        stats_.oldGenSize += objHeader->size;
        stats_.oldGenObjects++;
        stats_.objectsPromoted++;
    }
}

// ========== Helpers ==========

GCHeader* GarbageCollector::getGCHeader(void* obj) const {
    if (!obj) return nullptr;

    // obj points to ObjectHeader, but GCHeader is BEFORE it
    uint8_t* objPtr = static_cast<uint8_t*>(obj);
    uint8_t* gcHeaderPtr = objPtr - sizeof(GCHeader);
    return reinterpret_cast<GCHeader*>(gcHeaderPtr);
}

bool GarbageCollector::shouldCollect() const {
    return shouldCollectMinor() || shouldCollectMajor();
}

bool GarbageCollector::shouldCollectMinor() const {
    // Trigger if young gen exceeds threshold
    size_t threshold = static_cast<size_t>(config_.youngGenMaxSize * config_.youngGenThreshold);
    return stats_.youngGenSize >= threshold;
}

bool GarbageCollector::shouldCollectMajor() const {
    // Trigger if old gen exceeds threshold OR total heap is full
    size_t oldThreshold = static_cast<size_t>(config_.oldGenMaxSize * config_.oldGenThreshold);
    size_t totalThreshold = config_.totalMaxSize;

    return (stats_.oldGenSize >= oldThreshold) ||
           (stats_.youngGenSize + stats_.oldGenSize >= totalThreshold);
}

void GarbageCollector::updateStats(bool wasMinor, size_t bytesFreed, uint64_t durationMs) {
    if (wasMinor) {
        stats_.minorCollections++;
    } else {
        stats_.majorCollections++;
    }

    stats_.totalFreed += bytesFreed;
    stats_.totalGCTimeMs += durationMs;
    stats_.lastGCTimeMs = durationMs;
}

} // namespace volta::vm
