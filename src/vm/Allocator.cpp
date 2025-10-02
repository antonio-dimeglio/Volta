#include "../include/vm/Allocator.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/mman.h>

namespace volta::vm {

// ============================================================================
// Arena Implementation
// ============================================================================

Arena::Arena(size_t size) : size_(size) {
    // Allocate memory for the arena from the OS
    base_ = static_cast<uint8_t*>(malloc(size));
    if (!base_) {
        throw std::bad_alloc();
    }
    current_ = base_;
}

Arena::~Arena() {
    if (base_) {
        free(base_);
    }
}

void* Arena::allocate(size_t size, size_t alignment) {
    // Align the current pointer to the requested alignment
    uintptr_t currentAddr = reinterpret_cast<uintptr_t>(current_);
    uintptr_t alignedAddr = (currentAddr + alignment - 1) & ~(alignment - 1);
    size_t alignmentPadding = alignedAddr - currentAddr;

    // Check if we have enough space
    size_t totalSize = alignmentPadding + size;
    if (used() + totalSize > capacity()) {
        return nullptr;  // Arena is full
    }

    // Bump the pointer and return the aligned address
    current_ = reinterpret_cast<uint8_t*>(alignedAddr);
    void* result = current_;
    current_ += size;

    return result;
}

void Arena::reset() {
    // Reset the arena to its initial state (all memory available)
    current_ = base_;
}

bool Arena::contains(void* ptr) const {
    uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(base_);
    uintptr_t endAddr = baseAddr + size_;

    return ptrAddr >= baseAddr && ptrAddr < endAddr;
}

// ============================================================================
// FreeListAllocator Implementation
// ============================================================================

FreeListAllocator::~FreeListAllocator() {
    // Free all OS-allocated blocks
    for (void* block : osBlocks_) {
        ::free(block);
    }
    osBlocks_.clear();
}

void* FreeListAllocator::allocate(size_t size) {
    // Add header size to allocation
    size_t totalSize = size + sizeof(FreeBlock);

    // Try to find a suitable free block
    FreeBlock* block = findFreeBlock(totalSize);

    if (block) {
        // Found a free block, remove it from the free list
        size_t sizeClass = getSizeClass(block->size);

        // Remove from free list
        FreeBlock** head = &freeLists_[sizeClass];
        FreeBlock* prev = nullptr;
        FreeBlock* current = *head;

        while (current && current != block) {
            prev = current;
            current = current->next;
        }

        if (current == block) {
            if (prev) {
                prev->next = block->next;
            } else {
                *head = block->next;
            }
        }

        totalFree_ -= block->size;
        totalAllocated_ += block->size;

        // Split block if it's much larger than needed
        if (block->size >= totalSize + sizeof(FreeBlock) + 32) {
            splitBlock(block, totalSize);
        }

        return reinterpret_cast<uint8_t*>(block) + sizeof(FreeBlock);
    }

    // No suitable block found, allocate from OS
    return allocateFromOS(totalSize);
}

void FreeListAllocator::free(void* ptr, size_t size) {
    if (!ptr) return;

    // Get the block header
    FreeBlock* block = reinterpret_cast<FreeBlock*>(
        static_cast<uint8_t*>(ptr) - sizeof(FreeBlock)
    );

    block->size = size + sizeof(FreeBlock);

    // Add to appropriate free list
    size_t sizeClass = getSizeClass(block->size);
    block->next = freeLists_[sizeClass];
    freeLists_[sizeClass] = block;

    totalAllocated_ -= block->size;
    totalFree_ += block->size;
}

void* FreeListAllocator::allocateFromOS(size_t size) {
    // Round up to reasonable chunk size (at least 4KB)
    size_t allocSize = std::max(size, size_t(4096));

    void* memory = malloc(allocSize);
    if (!memory) {
        return nullptr;
    }

    osBlocks_.push_back(memory);

    FreeBlock* block = static_cast<FreeBlock*>(memory);
    block->size = allocSize;
    block->next = nullptr;

    totalAllocated_ += allocSize;

    // If we allocated more than needed, add remainder to free list
    if (allocSize > size + sizeof(FreeBlock) + 32) {
        FreeBlock* remainder = reinterpret_cast<FreeBlock*>(
            reinterpret_cast<uint8_t*>(block) + size
        );
        remainder->size = allocSize - size;
        remainder->next = nullptr;

        size_t sizeClass = getSizeClass(remainder->size);
        remainder->next = freeLists_[sizeClass];
        freeLists_[sizeClass] = remainder;

        totalFree_ += remainder->size;
        totalAllocated_ -= remainder->size;

        block->size = size;
    }

    return reinterpret_cast<uint8_t*>(block) + sizeof(FreeBlock);
}

void FreeListAllocator::compact() {
    mergeBlocks();
}

void FreeListAllocator::printStats(std::ostream& out) const {
    out << "FreeListAllocator Statistics:\n";
    out << "  Total allocated: " << totalAllocated_ << " bytes\n";
    out << "  Total free: " << totalFree_ << " bytes\n";
    out << "  OS blocks: " << osBlocks_.size() << "\n";

    out << "  Size classes:\n";
    for (size_t i = 0; i < NUM_SIZE_CLASSES; i++) {
        int count = 0;
        FreeBlock* block = freeLists_[i];
        while (block) {
            count++;
            block = block->next;
        }
        if (count > 0) {
            out << "    Class " << i << " (" << getSizeClassSize(i) << " bytes): "
                << count << " blocks\n";
        }
    }
}

size_t FreeListAllocator::getSizeClass(size_t size) const {
    // Size classes: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096+ bytes
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    if (size <= 2048) return 7;
    return 8;  // 4096+
}

size_t FreeListAllocator::getSizeClassSize(size_t sizeClass) const {
    const size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    return sizes[std::min(sizeClass, NUM_SIZE_CLASSES - 1)];
}

FreeBlock* FreeListAllocator::findFreeBlock(size_t size) {
    // Search appropriate size class and larger classes
    size_t sizeClass = getSizeClass(size);

    for (size_t i = sizeClass; i < NUM_SIZE_CLASSES; i++) {
        FreeBlock* block = freeLists_[i];
        while (block) {
            if (block->size >= size) {
                return block;
            }
            block = block->next;
        }
    }

    return nullptr;
}

void FreeListAllocator::splitBlock(FreeBlock* block, size_t size) {
    size_t remainderSize = block->size - size;

    if (remainderSize < sizeof(FreeBlock) + 16) {
        return;  // Remainder too small to split
    }

    // Create new block from remainder
    FreeBlock* remainder = reinterpret_cast<FreeBlock*>(
        reinterpret_cast<uint8_t*>(block) + size
    );
    remainder->size = remainderSize;

    // Add remainder to free list
    size_t sizeClass = getSizeClass(remainderSize);
    remainder->next = freeLists_[sizeClass];
    freeLists_[sizeClass] = remainder;

    totalFree_ += remainderSize;

    // Update original block size
    block->size = size;
}

void FreeListAllocator::mergeBlocks() {
    // Simple implementation: would need to track block addresses for proper merging
    // This is a placeholder for a more sophisticated implementation
    // In a real implementation, we'd sort blocks by address and merge adjacent ones
}

// ============================================================================
// SlabAllocator Implementation
// ============================================================================

SlabAllocator::SlabAllocator(size_t objectSize, size_t objectsPerSlab)
    : objectSize_(std::max(objectSize, sizeof(void*))),
      objectsPerSlab_(objectsPerSlab),
      currentSlab_(nullptr),
      slabList_(nullptr),
      freeList_(nullptr),
      allocatedCount_(0),
      freeCount_(0) {

    slabSize_ = objectSize_ * objectsPerSlab_ + sizeof(Slab);
}

SlabAllocator::~SlabAllocator() {
    // Free all slabs
    Slab* slab = slabList_;
    while (slab) {
        Slab* next = slab->next;
        freeSlab(slab);
        slab = next;
    }
}

void* SlabAllocator::allocate() {
    // If free list is empty, allocate a new slab
    if (!freeList_) {
        Slab* newSlab = allocateSlab();
        if (!newSlab) {
            return nullptr;
        }
    }

    // Pop from free list
    void* result = freeList_;
    freeList_ = *reinterpret_cast<void**>(freeList_);

    allocatedCount_++;
    freeCount_--;

    return result;
}

void SlabAllocator::free(void* ptr) {
    if (!ptr) return;

    // Push onto free list
    *reinterpret_cast<void**>(ptr) = freeList_;
    freeList_ = ptr;

    allocatedCount_--;
    freeCount_++;
}

void SlabAllocator::releaseEmptySlabs() {
    // Walk slab list and free completely empty slabs
    Slab* slab = slabList_;
    Slab* prev = nullptr;

    while (slab) {
        if (slab->freeCount == objectsPerSlab_) {
            // Slab is completely empty, free it
            Slab* next = slab->next;

            if (prev) {
                prev->next = next;
            } else {
                slabList_ = next;
            }

            if (currentSlab_ == slab) {
                currentSlab_ = slabList_;
            }

            freeCount_ -= slab->freeCount;
            freeSlab(slab);
            slab = next;
        } else {
            prev = slab;
            slab = slab->next;
        }
    }
}

SlabAllocator::Slab* SlabAllocator::allocateSlab() {
    // Allocate memory for slab
    void* memory = malloc(slabSize_);
    if (!memory) {
        return nullptr;
    }

    Slab* slab = new Slab();
    slab->memory = static_cast<uint8_t*>(memory);
    slab->freeCount = objectsPerSlab_;
    slab->next = slabList_;

    slabList_ = slab;
    currentSlab_ = slab;

    // Initialize free list within slab
    initializeSlabFreeList(slab);

    return slab;
}

void SlabAllocator::freeSlab(Slab* slab) {
    if (slab) {
        if (slab->memory) {
            ::free(slab->memory);
        }
        delete slab;
    }
}

void SlabAllocator::initializeSlabFreeList(Slab* slab) {
    // Initialize free list by linking all objects in the slab
    uint8_t* obj = slab->memory;

    for (size_t i = 0; i < objectsPerSlab_; i++) {
        void** slot = reinterpret_cast<void**>(obj);
        *slot = freeList_;
        freeList_ = obj;
        obj += objectSize_;
        freeCount_++;
    }
}

// ============================================================================
// PageAllocator Implementation
// ============================================================================

PageAllocator::~PageAllocator() {
    // Free all allocated pages
    for (const PageInfo& page : pages_) {
        munmap(page.address, page.size);
    }
    pages_.clear();
}

void* PageAllocator::allocate(size_t size) {
    // Round up to page boundary
    size_t allocSize = roundToPage(size);

    // Allocate using mmap (anonymous mapping)
    void* memory = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (memory == MAP_FAILED) {
        return nullptr;
    }

    // Track the allocation
    pages_.push_back({memory, allocSize});
    totalAllocated_ += allocSize;

    return memory;
}

void PageAllocator::free(void* ptr, size_t size) {
    if (!ptr) return;

    // Round up to page boundary
    size_t allocSize = roundToPage(size);

    // Unmap the pages
    munmap(ptr, allocSize);

    // Remove from tracking
    auto it = std::find_if(pages_.begin(), pages_.end(),
        [ptr](const PageInfo& page) { return page.address == ptr; });

    if (it != pages_.end()) {
        totalAllocated_ -= it->size;
        pages_.erase(it);
    }
}

size_t PageAllocator::pageSize() {
    static size_t cachedPageSize = 0;
    if (cachedPageSize == 0) {
        cachedPageSize = sysconf(_SC_PAGESIZE);
    }
    return cachedPageSize;
}

size_t PageAllocator::roundToPage(size_t size) {
    size_t ps = pageSize();
    return (size + ps - 1) & ~(ps - 1);
}

} // namespace volta::vm