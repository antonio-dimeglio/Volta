#pragma once

#include <stdint.h>
#include <sstream>

namespace volta::vm {
#define GC_MARKED_BIT 0x01


enum class ObjectType {
    ARRAY,
    STRING,
    CLOSURE,
    OPTION_SOME,
    TUPLE,
    STRUCT
};


struct Object {
    ObjectType type;

    // GC Metadata
    uint8_t gc_flags;
    uint8_t generation;
    uint8_t age;

    // Allocation list
    Object* gc_next;

    inline bool isMarked() { return gc_flags & GC_MARKED_BIT; }
    inline void set_marked() { gc_flags |= GC_MARKED_BIT; }
    inline void clear_marked() { gc_flags &= ~GC_MARKED_BIT; }
};

enum class ValueType {
    INT64,
    FLOAT64,
    BOOL,
    OBJECT,
    NONE
};

struct Value {
    ValueType type;
    union {
        int64_t as_i64;
        double as_f64;
        bool as_b;
        Object* as_obj;
    } as;

    // Helper factory functions for clean initialization
    static inline Value makeInt(int64_t val) {
        Value v;
        v.type = ValueType::INT64;
        v.as.as_i64 = val;
        return v;
    }

    static inline Value makeFloat(double val) {
        Value v;
        v.type = ValueType::FLOAT64;
        v.as.as_f64 = val;
        return v;
    }

    static inline Value makeBool(bool val) {
        Value v;
        v.type = ValueType::BOOL;
        v.as.as_b = val;
        return v;
    }

    static inline Value makeObject(Object* obj) {
        Value v;
        v.type = ValueType::OBJECT;
        v.as.as_obj = obj;
        return v;
    }

    static inline Value makeNone() {
        Value v;
        v.type = ValueType::NONE;
        return v;
    }

    inline std::string toString() const {
        switch (type) {
            case ValueType::INT64:
                return std::to_string(as.as_i64);
            case ValueType::FLOAT64: {
                std::ostringstream oss;
                oss << as.as_f64;
                return oss.str();
            }
            case ValueType::BOOL:
                return as.as_b ? "true" : "false";
            case ValueType::OBJECT:
                if (as.as_obj == nullptr)
                    return "<null object>";
                switch (as.as_obj->type) {
                    case ObjectType::ARRAY:   return "<array>";
                    case ObjectType::STRING:  return "<string>";
                    case ObjectType::CLOSURE: return "<closure>";
                    case ObjectType::OPTION_SOME: return "<option>";
                    case ObjectType::TUPLE:   return "<tuple>";
                    case ObjectType::STRUCT:  return "<struct>";
                    default:                  return "<object>";
                }
            case ValueType::NONE:
                return "none";
            default:
                return "<unknown>";
        }
    }
};


} // namespace volta::vm
