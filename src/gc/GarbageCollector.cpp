#include "gc/GarbageCollector.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace Volta {
namespace GC {

// ========== CONSTRUCTOR / DESTRUCTOR ==========

GarbageCollector::GarbageCollector(size_t nurserySize, size_t oldGenSize)
    : rootProvider(nullptr)
    , currentCollection(MINOR_COLLECTION)
    , numMinorGCs(0)
    , numMajorGCs(0)
    , totalAllocated(0)
{
    initHeapSpace(nursery, nurserySize);
    initHeapSpace(oldGen, oldGenSize);
    typeRegistry = nullptr;
}

GarbageCollector::~GarbageCollector() {
    freeHeapSpace(nursery);
    freeHeapSpace(oldGen);
}

// ========== SETUP ==========

void GarbageCollector::setRootProvider(GCRootProvider* provider) {
    rootProvider = provider;
}

void GarbageCollector::setTypeRegistry(TypeRegistry* registry) {
    typeRegistry = registry;
}
// ========== ALLOCATION ==========

StringObject* GarbageCollector::allocateString(size_t length) {
    size_t totalSize = stringObjectSize(length);
    void* ptr = allocate(totalSize);

    StringObject* obj = static_cast<StringObject*>(ptr);
    obj->header.type = ObjectType::OBJ_STRING;
    obj->header.generation = isInSpace(ptr, oldGen) ? GC::Generation::OLD : GC::Generation::NURSERY;
    obj->header.age = 0;
    obj->header.marked = 0;
    obj->header.size = totalSize;
    obj->header.forwarding = nullptr;

    obj->length = length;
    obj->data[length] = '\0';


    totalAllocated += totalSize;
    return obj;
}

ArrayObject* GarbageCollector::allocateArray(size_t elementCount, uint32_t elementTypeId, size_t elementSize) {
    size_t totalSize =  arrayObjectSize(elementCount, elementSize);
    void* ptr = allocate(totalSize);

    ArrayObject* obj = static_cast<ArrayObject*>(ptr);

    obj->header.type = ObjectType::OBJ_ARRAY;
    obj->header.generation = isInSpace(ptr, oldGen) ? GC::Generation::OLD : GC::Generation::NURSERY;
    obj->header.age = 0;
    obj->header.marked = 0;
    obj->header.size = totalSize;
    obj->header.forwarding = nullptr;

    obj->length = elementCount;
    obj->elementTypeId = elementTypeId;
    obj->padding = 0;
    std::memset(obj->elements, 0, elementCount * elementSize);
    totalAllocated += totalSize;
    return obj;
}

StructObject* GarbageCollector::allocateStruct(uint32_t structTypeId, size_t fieldDataSize) {
    size_t totalSize = structObjectSize(fieldDataSize);
    void* ptr = allocate(totalSize);

    StructObject* obj = static_cast<StructObject*>(ptr);

    obj->header.type = ObjectType::OBJ_STRUCT;
    obj->header.generation = isInSpace(ptr, oldGen) ? GC::Generation::OLD : GC::Generation::NURSERY;
    obj->header.age = 0;
    obj->header.marked = 0;
    obj->header.size = totalSize;
    obj->header.forwarding = nullptr;

    obj->structTypeId = structTypeId;
    obj->padding = 0;
    std::memset(obj->fields, 0, fieldDataSize);
    totalAllocated += totalSize;
    return obj;
}

EnumObject* GarbageCollector::allocateEnum(uint32_t variantTag, size_t variantDataSize) {
    size_t totalSize = enumObjectSize(variantDataSize);
    void* ptr = allocate(totalSize);

    EnumObject* obj = static_cast<EnumObject*>(ptr);

    obj->header.type = ObjectType::OBJ_ENUM;
    obj->header.generation = isInSpace(ptr, oldGen) ? GC::Generation::OLD : GC::Generation::NURSERY;
    obj->header.age = 0;
    obj->header.marked = 0;
    obj->header.size = totalSize;
    obj->header.forwarding = nullptr;

    obj->variantTag = variantTag;
    obj->padding = 0;
    std::memset(obj->data, 0, variantDataSize);
    totalAllocated += totalSize;
    return obj;
}

// ========== WRITE BARRIER ==========

void GarbageCollector::writeBarrier(Object* obj, Object* value) {
    if (!obj || !value) return;  // Null check
    if (obj->header.generation == Generation::OLD &&
        value->header.generation == Generation::NURSERY) rememberedSet.insert(obj);
}

// ========== GARBAGE COLLECTION ==========

void GarbageCollector::minorGC() {
    nursery.bump = nursery.toSpace;
    nursery.limit = (char*)nursery.toSpace + nursery.size;
    
    // Copy live objects (they go to to-space via bump allocation)
    auto roots = rootProvider->getRoots();
    for (Object** root : roots) {
        if (*root) {
            *root = copyObject(*root);
        }
    }
    
    for (Object* oldObj : rememberedSet) {
        traceObject(oldObj);
    }
    
    // Now swap (to-space with objects becomes from-space)
    swapSpaces(nursery);
    
    rememberedSet.clear();
    numMinorGCs++;
}

void GarbageCollector::majorGC() {
// Check BEFORE swapping/copying
    size_t oldGenUsedBefore = getOldGenUsed();
    
    // Prepare to-space for copying
    nursery.bump = nursery.toSpace;
    nursery.limit = (char*)nursery.toSpace + nursery.size;
    oldGen.bump = oldGen.toSpace;
    oldGen.limit = (char*)oldGen.toSpace + oldGen.size;
    
    // Copy all roots
    auto roots = rootProvider->getRoots();
    for (Object** root : roots) {
        if (*root) {
            *root = copyObject(*root);
        }
    }
    
    // Swap spaces (to-space with objects becomes from-space)
    swapSpaces(nursery);
    swapSpaces(oldGen);
    
    rememberedSet.clear();
    
    // Check if need to grow (use size AFTER collection)
    if (shouldGrowOldGen()) {
        growOldGen();
    }
    
    numMajorGCs++;
}

// ========== STATISTICS ==========

size_t GarbageCollector::getNurseryUsed() const {
    return static_cast<char*>(nursery.bump) - static_cast<char*>(nursery.fromSpace);
}

size_t GarbageCollector::getOldGenUsed() const {
    return static_cast<char*>(oldGen.bump) - static_cast<char*>(oldGen.fromSpace);
}

// ========== INTERNAL HELPERS ==========

void GarbageCollector::initHeapSpace(HeapSpace& space, size_t size) {
    space.size = size;
    space.fromSpace = malloc(size);      
    space.toSpace = malloc(size);      
    space.bump = space.fromSpace;
    space.limit = (char*)space.fromSpace + size;
    
    if (!space.fromSpace || !space.toSpace) {
        throw std::runtime_error("Failed to allocate heap space");
    }
}

void GarbageCollector::freeHeapSpace(HeapSpace& space) {
    free(space.fromSpace);
    free(space.toSpace);

    space.size = 0;
    space.fromSpace = nullptr;
    space.toSpace = nullptr;
    space.bump = nullptr;
    space.limit = nullptr;
}

void* GarbageCollector::tryAllocateNursery(size_t size) {
    auto newBump = static_cast<char*>(nursery.bump) + size;
    if (newBump > nursery.limit) return nullptr;
    auto oldPos = nursery.bump;
    nursery.bump = newBump;
    return oldPos;
}

void* GarbageCollector::allocateOldGen(size_t size) {
    auto newBump = static_cast<char*>(oldGen.bump) + size;
    if (newBump > oldGen.limit) throw std::runtime_error("Out of memory.");
    auto oldPos = oldGen.bump;
    oldGen.bump = newBump;
    return oldPos;
}

void* GarbageCollector::allocate(size_t size) {
    if (size > LARGE_OBJECT_THRESHOLD) return allocateOldGen(size);

    void* mem = tryAllocateNursery(size);
    if (mem) return mem;

    minorGC();
    mem = tryAllocateNursery(size);

    if (mem) return mem; 
    throw std::runtime_error("Out of memory");
}

Object* GarbageCollector::copyObject(Object* obj) {
    if (!obj) return nullptr;  // Null check
    if (obj->header.forwarding) return static_cast<Object*>(obj->header.forwarding);

    Object* newObj;

    if (obj->header.generation == Generation::NURSERY) {
        obj->header.age++;
        if (obj->header.age >= 2) {
            newObj = copyToOldGen(obj);
        } else {
            newObj = copyToNursery(obj);
        }
    } else {
        newObj = copyToOldGen(obj);
    }

    obj->header.forwarding = newObj;

    traceObject(newObj);
    return newObj;
}

Object* GarbageCollector::copyToNursery(Object* obj) {
    Object* newObj = (Object*)nursery.bump;
    nursery.bump = (char*)nursery.bump + obj->header.size;
    
    // Copy bytes
    memcpy(newObj, obj, obj->header.size);
    
    // Update header
    newObj->header.forwarding = nullptr;
    
    return newObj;
}

Object* GarbageCollector::copyToOldGen(Object* obj) {
    Object* newObj = (Object*)oldGen.bump;
    oldGen.bump = (char*)oldGen.bump + obj->header.size;
    
    // Copy bytes
    memcpy(newObj, obj, obj->header.size);
    
    // Update header
    newObj->header.forwarding = nullptr;
    newObj->header.generation = Generation::OLD;
    return newObj;
}

void GarbageCollector::traceObject(Object* obj) {
    if (!obj) return;  // Null check
    if (!typeRegistry) return;  // No type info available

    switch (obj->header.type) {
        case OBJ_STRING: break;
        case OBJ_ARRAY: {
            ArrayObject* arrObj = (ArrayObject*)obj;
            const TypeInfo* elemInfo = typeRegistry->getTypeInfo(arrObj->elementTypeId);
            if (elemInfo && elemInfo->kind == TypeInfo::OBJECT) {
                for (size_t i = 0; i < arrObj->length; i++) {
                    Object** slot = (Object**)&arrObj->elements[i * sizeof(Object*)];
                    if (*slot) *slot = copyObject(*slot);
                }
            }
            break;
        }
        case OBJ_STRUCT: {
            StructObject* strObj = (StructObject*)obj;
            const TypeInfo* structInfo = typeRegistry->getTypeInfo(strObj->structTypeId);
            if (structInfo) {
                for (size_t offset : structInfo->pointerFieldOffsets) {
                    Object** slot = (Object**)((char*)strObj->fields + offset);
                    if (*slot) {
                        *slot = copyObject(*slot);
                    }
                }
            }
            break;
        }
        case OBJ_ENUM: {
            // For now, enums are similar to structs - they can contain pointers
            // We'll need type registry support to know which fields are pointers
            // For simple cases (like Option[int]), there are no pointers to trace
            EnumObject* enumObj = (EnumObject*)obj;
            // TODO: Add type registry support for enum variants
            // For now, assume enums with primitive data (like Option[int]) have no pointers
            (void)enumObj;  // Suppress unused warning
            break;
        }
    }
}

void GarbageCollector::swapSpaces(HeapSpace& space) {
    auto temp = space.fromSpace;
    space.fromSpace = space.toSpace;
    space.toSpace = temp;
    // Don't touch bump - it's already correct!
    // Update limit to match new from-space
    space.limit = (char*)space.fromSpace + space.size;
}

void GarbageCollector::growOldGen() {
    size_t newSize = oldGen.size * 2;
    void* newFromSpace = malloc(newSize);
    void* newToSpace = malloc(newSize);

    if (!newFromSpace || !newToSpace) {
        throw std::runtime_error("Failed to grow old gen");
    }
    
    // Copy current used data to new space
    size_t usedBytes = getOldGenUsed();
    memcpy(newFromSpace, oldGen.fromSpace, usedBytes);
    
    // Free old spaces
    free(oldGen.fromSpace);
    free(oldGen.toSpace);
    
    // Update old gen structure
    oldGen.fromSpace = newFromSpace;
    oldGen.toSpace = newToSpace;
    oldGen.size = newSize;
    
    // Update bump pointer (point into new space)
    oldGen.bump = (char*)newFromSpace + usedBytes;
    oldGen.limit = (char*)newFromSpace + newSize;

}

bool GarbageCollector::isInSpace(void* ptr, const HeapSpace& space) const {
    return ptr >= space.fromSpace && ptr < space.limit;
}

bool GarbageCollector::shouldGrowOldGen() const {
    double usage = (double)getOldGenUsed() / (double)oldGen.size;
    return usage > OLD_GEN_GROWTH_THRESHOLD;
}

}
}
