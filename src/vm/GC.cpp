#include "vm/GC.hpp"
#include "vm/VM.hpp"
#include <iostream>

namespace volta::vm {

GarbageCollector::GarbageCollector(VM* vm)
    : vm_(vm), pause_count_(0), should_collect_(false), collection_count_(0) {
}

// ============================================================================
// Main GC Interface
// ============================================================================

void GarbageCollector::collect() {
    if (is_paused()) { should_collect_ = true; return; }
    size_t bytes_before = vm_->bytes_allocated_;
    mark_roots();
    process_gray_stack();
    sweep();
    vm_->next_gc_threshold_ = vm_->bytes_allocated_ * GC_HEAP_GROW_FACTOR;
    collection_count_++;
    // Only do this if specifically asked to.
    // std::cout << "GC: collected " << (bytes_before - vm_->bytes_allocated_)
    //             << " bytes, " << vm_->bytes_allocated_ << " remaining\n";
}

void GarbageCollector::pause() {
    // During C function calls, GC must not run because:
    // - C code may hold pointers to Volta objects
    // - GC might free objects C code is using
    // Note: Pause/resume can nest, so we use a counter
    pause_count_++;
}

void GarbageCollector::resume() {
    // TODO: Resume GC after pause
    //
    // Implementation:
    // 1. Decrement pause counter: 
    // 2. Check: 
    // 3. If fully resumed and collection was deferred:
       

    pause_count_--;
    if (pause_count_ < 0) throw std::runtime_error("GC resume without pause");
    if (pause_count_ == 0 && should_collect_) {
        should_collect_ = false;
        collect();
    }
}

bool GarbageCollector::is_paused() const {
    pause_count_ > 0;
}

// ============================================================================
// Statistics
// ============================================================================

size_t GarbageCollector::bytes_allocated() const {
    return vm_->bytes_allocated_;
}

size_t GarbageCollector::next_gc_threshold() const {
    return vm_->next_gc_threshold_;
}

size_t GarbageCollector::object_count() const {
    size_t count = 0;
    Object* obj = vm_->objects_;
    while (obj != nullptr) { count++; obj = obj->next; }
    return count;
}

size_t GarbageCollector::collection_count() const {
    return collection_count_;
}

// ============================================================================
// Mark Phase
// ============================================================================

void GarbageCollector::mark_roots() {
    if (vm_->current_frame_) {
        for (int i = 0; i < 256; i++) {
            mark_value(vm_->current_frame_->registers[i]);
        }

        mark_object((Object*)vm_->current_frame_->closure);
    }

    for (auto& frame : vm_->call_stack_) {
        for (int i = 0; i < 256; i++) {
            mark_value(frame.registers[i]);
        }
        mark_object((Object*)frame.closure);
    }
    
    for (auto& [name, value] : vm_->globals_) {
        mark_value(value);
    }

    for (auto& constant : vm_->constants_) {
        mark_value(constant);
    }
}

void GarbageCollector::mark_value(const Value& value) {
    if (!value.is_object()) return;
    mark_object(value.as_object());
}

void GarbageCollector::mark_object(Object* obj) {
    if (obj == nullptr) return;
    if (obj->marked) return;
    obj->marked = true;
    gray_stack_.push_back(obj);
}

void GarbageCollector::process_gray_stack() {
    // TODO: Process gray stack (mark children of marked objects)
    //
    // This is where we trace through object references.
    //
    // Implementation:
    while (!gray_stack_.empty()) {
        Object* obj = gray_stack_.back(); gray_stack_.pop_back();
        switch (obj->type) {
            case ObjectType::STRING:
                // Strings have no references
                break;
    
            case ObjectType::ARRAY: {
                ArrayObject* array = (ArrayObject*)obj;
                for (size_t i = 0; i < array->length; i++) {
                    mark_value(array->elements[i]);
                }
                break;
            }
    
            case ObjectType::STRUCT: {
                StructObject* struct_obj = (StructObject*)obj;
                for (size_t i = 0; i < struct_obj->field_count; i++) {
                    mark_value(struct_obj->fields[i]);
                }
                break;
            }
    
            case ObjectType::CLOSURE: {
                ClosureObject* closure = (ClosureObject*)obj;
                // Mark upvalues (captured variables)
                for (size_t i = 0; i < closure->upvalue_count; i++) {
                    mark_value(closure->upvalues[i]);
                }
                // Note: We don't mark closure->function because it's not a GC'd object
                //       (it's metadata, not a heap object)
                break;
            }
    
            case ObjectType::NATIVE_FUNCTION:
                // Native functions have no references
                break;
        }
    }

}

// ============================================================================
// Sweep Phase
// ============================================================================

void GarbageCollector::sweep() {
    Object** obj = &vm_->objects_;
    while (*obj != nullptr) {
        if ((*obj)->marked) {
            (*obj)->marked = false;  // Unmark for next cycle
            obj = &(*obj)->next;
        } else {
            Object* unreached = *obj;
            *obj = unreached->next;
            free_object(unreached);
        }
    }
}

void GarbageCollector::free_object(Object* obj) {
    size_t size = object_size(obj);
    vm_->bytes_allocated_ -= size;
    switch (obj->type) {
        case ObjectType::STRING:
            delete (StringObject*)obj;
            break;
        case ObjectType::ARRAY:
            delete (ArrayObject*)obj;
            break;
        case ObjectType::STRUCT:
            delete (StructObject*)obj;
            break;
        case ObjectType::CLOSURE:
            delete (ClosureObject*)obj;
            break;
        case ObjectType::NATIVE_FUNCTION:
            // Call destructor explicitly (for std::string)
            ((NativeFunctionObject*)obj)->~NativeFunctionObject();
            delete (NativeFunctionObject*)obj;
            break;
    }

}

} // namespace volta::vm
