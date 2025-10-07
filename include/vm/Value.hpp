#pragma once

#include <stdint.h>
#include <sstream>
#include "gc/Object.hpp"

namespace volta::vm {
#define GC_MARKED_BIT 0x01

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
        Volta::GC::Object* as_obj;
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

    static inline Value makeObject(Volta::GC::Object* obj) {
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
                
                return "<object>";
            case ValueType::NONE:
                return "none";
            default:
                return "<unknown>";
        }
    }
};


} // namespace volta::vm
