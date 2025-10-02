#include "../include/vm/Allocator.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace volta::vm {

// ============================================================================
// Arena Implementation
// ============================================================================

Arena::Arena(size_t size) : size_(size) {
}

Arena::~Arena() {
}

void* Arena::allocate(size_t size, size_t alignment) {
    return nullptr;
}

void Arena::reset() {
}

bool Arena::contains(void* ptr) const {
    return false;
}

// ============================================================================
// FreeListAllocator Implementation
// ============================================================================

FreeListAllocator::~FreeListAllocator() {
}

void* FreeListAllocator::allocate(size_t size) {
    return nullptr;
}

void FreeListAllocator::free(void* ptr, size_t size) {
}

void* FreeListAllocator::allocateFromOS(size_t size) {
    return nullptr;
}

void FreeListAllocator::compact() {
}

void FreeListAllocator::printStats(std::ostream& out) const {
}

size_t FreeListAllocator::getSizeClass(size_t size) const {
    return 0;
}

size_t FreeListAllocator::getSizeClassSize(size_t sizeClass) const {
    return 0;
}

FreeBlock* FreeListAllocator::findFreeBlock(size_t size) {
    return nullptr;
}

void FreeListAllocator::splitBlock(FreeBlock* block, size_t size) {
}

void FreeListAllocator::mergeBlocks() {
}

// ============================================================================
// SlabAllocator Implementation
// ============================================================================

SlabAllocator::SlabAllocator(size_t objectSize, size_t objectsPerSlab)
    : objectSize_(objectSize), objectsPerSlab_(objectsPerSlab) {
}

SlabAllocator::~SlabAllocator() {
}

void* SlabAllocator::allocate() {
    return nullptr;
}

void SlabAllocator::free(void* ptr) {
}

void SlabAllocator::releaseEmptySlabs() {
}

SlabAllocator::Slab* SlabAllocator::allocateSlab() {
    return nullptr;
}

void SlabAllocator::freeSlab(Slab* slab) {
}

void SlabAllocator::initializeSlabFreeList(Slab* slab) {
}

// ============================================================================
// PageAllocator Implementation
// ============================================================================

PageAllocator::~PageAllocator() {
}

void* PageAllocator::allocate(size_t size) {
    return nullptr;
}

void PageAllocator::free(void* ptr, size_t size) {
}

size_t PageAllocator::pageSize() {
    return 4096;
}

size_t PageAllocator::roundToPage(size_t size) {
    return 0;
}

} // namespace volta::vm
