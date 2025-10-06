#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <new>
#include <vector>
#include <type_traits>

namespace volta::ir {

/**
 * Arena - Fast bump-pointer memory allocator
 *
 * Design Philosophy:
 * - Allocate objects in contiguous memory chunks
 * - No individual deallocations (bulk free on arena destruction)
 * - 10-100x faster than malloc/free
 * - Better cache locality
 * - Eliminates use-after-free bugs
 *
 * Key Concepts:
 * - Chunk: Fixed-size memory block (default 1MB)
 * - Bump Pointer: Current allocation position
 * - Alignment: Objects must be aligned to their natural alignment
 * - Placement New: Construct object at specific memory address
 *
 * Usage:
 *   Arena arena(1024 * 1024);  // 1MB chunks
 *   auto* obj = arena.allocate<MyClass>(arg1, arg2);
 *   // Use obj...
 *   // No delete! Arena destructor frees everything
 */
class Arena {
public:
    /**
     * Create arena with specified chunk size
     * @param chunkSize Size of each memory chunk (default 1MB)
     */
    explicit Arena(size_t chunkSize = 1024 * 1024);

    /**
     * Destructor - frees all allocated chunks
     */
    ~Arena();

    // Disable copy (arena owns unique memory)
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // Allow move
    Arena(Arena&& other) noexcept;
    Arena& operator=(Arena&& other) noexcept;

    /**
     * Allocate and construct object of type T
     * @param args Constructor arguments for T
     * @return Pointer to constructed object
     */
    template<typename T, typename... Args>
    T* allocate(Args&&... args);

    /**
     * Allocate raw memory without constructing object
     * @param size Number of bytes to allocate
     * @param alignment Required alignment (must be power of 2)
     * @return Pointer to allocated memory
     *
     * LEARNING TIP: Use this for arrays or raw buffers
     */
    void* allocateRaw(size_t size, size_t alignment);

    /**
     * Reset arena - free all chunks
     * LEARNING TIP: Useful for reusing arena across multiple compilation units
     */
    void reset();

    /**
     * Get total bytes allocated (including waste from alignment)
     */
    size_t getBytesAllocated() const { return totalAllocated_; }

    /**
     * Get total bytes used (actual object sizes)
     */
    size_t getBytesUsed() const { return totalUsed_; }

    /**
     * Get number of chunks allocated
     */
    size_t getNumChunks() const;

private:
    /**
     * Chunk - A contiguous block of memory
     */
    struct Chunk {
        uint8_t* memory;    // Pointer to chunk memory
        size_t size;        // Total size of chunk
        size_t used;        // Bytes used in this chunk
        Chunk* next;        // Linked list of chunks

        Chunk(size_t sz);
        ~Chunk();
    };

    /**
     * Allocate a new chunk and link it to the list
     */
    void allocateChunk();

    /**
     * Align a pointer to required alignment
     * @param ptr Pointer to align
     * @param alignment Required alignment (must be power of 2)
     * @return Aligned pointer
     *
     */
    static uintptr_t alignPointer(uintptr_t ptr, size_t alignment);

    Chunk* currentChunk_;       // Current chunk being allocated from
    Chunk* firstChunk_;         // First chunk (for cleanup)
    size_t chunkSize_;          // Size of each chunk
    size_t totalAllocated_;     // Total bytes allocated from system
    size_t totalUsed_;          // Total bytes actually used by objects

    // Track allocated objects for proper destruction
    // Store pairs of (destructor_function, object_pointer)
    std::vector<std::pair<void(*)(void*), void*>> destructors_;
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename T, typename... Args>
T* Arena::allocate(Args&&... args) {
    // Allocate raw memory
    void* ptr = allocateRaw(sizeof(T), alignof(T));

    // Construct object using placement new
    T* obj = new (ptr) T(std::forward<Args>(args)...);

    // Register destructor (only if T has a non-trivial destructor)
    if constexpr (!std::is_trivially_destructible_v<T>) {
        destructors_.push_back({
            [](void* p) { static_cast<T*>(p)->~T(); },
            obj
        });
    }

    return obj;
}

} // namespace volta::ir
