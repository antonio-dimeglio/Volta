#pragma once

#include <vector>

namespace Volta {
namespace GC {

struct Object;

/**
 * Interface for providing GC roots
 *
 * The VM (or any other component managing object references) must implement
 * this interface to provide the GC with roots for tracing.
 *
 * GC Roots are pointers to objects that are definitely live:
 * - Stack frame registers holding objects
 * - Global variables
 * - Runtime state (interned strings, etc.)
 *
 * The GC calls getRoots() at the beginning of each collection cycle.
 */
class GCRootProvider {
public:
    virtual ~GCRootProvider() = default;

    /**
     * Get all GC roots
     *
     * Returns a list of pointers-to-pointers. Each Object** in the returned
     * vector is a location that holds an object pointer that must be traced.
     *
     * IMPORTANT: The GC may update the pointed-to values during copying!
     *
     * Example:
     *   If a stack frame register holds an object at address 0x1000,
     *   and after GC it's moved to 0x2000, the GC will update the register:
     *     register.object_ptr = 0x2000
     *
     * @return Vector of pointers to object pointers (roots)
     */
    virtual std::vector<Object**> getRoots() = 0;
};

} // namespace GC
} // namespace Volta