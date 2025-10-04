#include "IR/Arena.hpp"
#include <cstdlib>
#include <cassert>

namespace volta::ir {

// ============================================================================
// Chunk Implementation
// ============================================================================

Arena::Chunk::Chunk(size_t sz)
    : memory(new uint8_t[sz]),
      size(sz),
      used(0),
      next(nullptr) {
}

Arena::Chunk::~Chunk() {
    delete[] memory;
}

// ============================================================================
// Arena Implementation
// ============================================================================

Arena::Arena(size_t chunkSize)
    : currentChunk_(nullptr),
      firstChunk_(nullptr),
      chunkSize_(chunkSize),
      totalAllocated_(0),
      totalUsed_(0) {
    allocateChunk();
}

Arena::~Arena() {
    reset();
}

Arena::Arena(Arena&& other) noexcept
    : currentChunk_(other.currentChunk_),
      firstChunk_(other.firstChunk_),
      chunkSize_(other.chunkSize_),
      totalAllocated_(other.totalAllocated_),
      totalUsed_(other.totalUsed_) {
    other.currentChunk_ = nullptr;
    other.firstChunk_ = nullptr;
    other.totalAllocated_ = 0;
    other.totalUsed_ = 0;
}

Arena& Arena::operator=(Arena&& other) noexcept {
    if (this != &other) {
        reset();
        currentChunk_ = other.currentChunk_;
        firstChunk_ = other.firstChunk_;
        chunkSize_ = other.chunkSize_;
        totalAllocated_ = other.totalAllocated_;
        totalUsed_ = other.totalUsed_;

        other.currentChunk_ = nullptr;
        other.firstChunk_ = nullptr;
        other.totalAllocated_ = 0;
        other.totalUsed_ = 0;
    }
    return *this;
}

void* Arena::allocateRaw(size_t size, size_t alignment) {
    // TODO: Implement raw allocation
    // This is the CORE method - implement carefully!

    // Step 1: Validate alignment (must be power of 2)
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be power of 2");

    // Step 0: Ensure we have a current chunk (handles post-reset case)
    if (!currentChunk_) {
        allocateChunk();
    }

    // Step 2: Get current position in chunk
    uintptr_t current = (uintptr_t)(currentChunk_->memory + currentChunk_->used);

    // Step 3: Align the pointer
    uintptr_t aligned = alignPointer(current, alignment);
    size_t padding = aligned - current;

    // Step 4: Check if we have enough space
    size_t totalNeeded = currentChunk_->used + padding + size;
    if (totalNeeded > currentChunk_->size) {
        // If allocation is larger than chunk size, allocate special large chunk
        if (size > chunkSize_) {
            auto* largeChunk = new Chunk(size + alignment);
            largeChunk->next = currentChunk_;
            if (!firstChunk_) {
                firstChunk_ = largeChunk;
            }
            // Insert into list but don't make it current (keep allocating from normal chunks)
            largeChunk->next = firstChunk_;
            firstChunk_ = largeChunk;
            totalAllocated_ += size + alignment;

            // Allocate from this large chunk
            uintptr_t largeStart = (uintptr_t)largeChunk->memory;
            uintptr_t largeAligned = alignPointer(largeStart, alignment);
            largeChunk->used = (largeAligned - largeStart) + size;
            totalUsed_ += size;
            return (void*)largeAligned;
        }

        // Normal case: allocate new regular chunk
        allocateChunk();
        // Recalculate with new chunk
        current = (uintptr_t)(currentChunk_->memory + currentChunk_->used);
        aligned = alignPointer(current, alignment);
        padding = aligned - current;
    }

    // Step 5: Allocate from current chunk
    void* ptr = (void*)aligned;
    currentChunk_->used += padding + size;
    totalUsed_ += size;

    return ptr;
}

void Arena::allocateChunk() {
    auto* chunk = new Chunk(chunkSize_);
    if (!firstChunk_) {
        firstChunk_ = chunk;
    }
    if (currentChunk_) {
        chunk->next = currentChunk_;
    }
    currentChunk_ = chunk;
    totalAllocated_ += chunkSize_;
}

void Arena::reset() {
    // TODO: Free all chunks and reset arena
    // HINT: Similar to destructor
    // HINT: But also reset currentChunk_, firstChunk_, counters

    // Minimal implementation
    Chunk* chunk = firstChunk_;
    while (chunk) {
        Chunk* next = chunk->next;
        delete chunk;
        chunk = next;
    }
    currentChunk_ = nullptr;
    firstChunk_ = nullptr;
    totalAllocated_ = 0;
    totalUsed_ = 0;
}

size_t Arena::getNumChunks() const {
    size_t count = 0;
    Chunk* chunk = firstChunk_;
    while (chunk) {
        count++;
        chunk = chunk->next;
    }
    return count;
}

uintptr_t Arena::alignPointer(uintptr_t ptr, size_t alignment) {
    return (ptr + alignment - 1) & ~(alignment - 1);
}

} // namespace volta::ir
