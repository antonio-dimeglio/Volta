#include "bytecode/ConstantPool.hpp"
#include <cstring>
#include <stdexcept>

namespace volta::bytecode {

uint32_t ConstantPoolBuilder::add_int(int64_t value) {
    auto it = int_map_.find(value);
    if (it != int_map_.end()) return it->second;
    uint32_t index = constants_.size();
    constants_.push_back(Constant::from_int(value));
    int_map_[value] = index;
    return index;
}

uint32_t ConstantPoolBuilder::add_float(double value) {
    auto it = float_map_.find(value);
    if (it != float_map_.end()) return it->second;
    uint32_t index = constants_.size();
    constants_.push_back(Constant::from_float(value));
    float_map_[value] = index;
    return index;
}

uint32_t ConstantPoolBuilder::add_string(const std::string& value) {
    auto it = string_map_.find(value);
    if (it != string_map_.end()) return it->second;
    uint32_t index = constants_.size();
    constants_.push_back(Constant::from_string(value));
    string_map_[value] = index;
    return index;
}

uint32_t ConstantPoolBuilder::add_bool(bool value) {
    auto it = bool_map_.find(value);
    if (it != bool_map_.end()) return it->second;
    uint32_t index = constants_.size();
    constants_.push_back(Constant::from_bool(value));
    bool_map_[value] = index;
    return index;
}

uint32_t ConstantPoolBuilder::add_none() {
    if (none_index_ != 0xFFFFFFFF) return none_index_;
    none_index_ = constants_.size();
    constants_.push_back(Constant::none());
    return none_index_;
}

const Constant& ConstantPoolBuilder::get(uint32_t index) const {
    if (index >= constants_.size()) throw std::runtime_error("IR tried to access illegal index");
    return constants_[index];
}

std::vector<uint8_t> ConstantPoolBuilder::serialize() const {
    std::vector<uint8_t> result;

    // Write count (big-endian)
    uint32_t count = constants_.size();
    result.push_back((count >> 24) & 0xFF);
    result.push_back((count >> 16) & 0xFF);
    result.push_back((count >> 8) & 0xFF);
    result.push_back(count & 0xFF);

    // Write each constant
    for (const auto& c : constants_) {
        result.push_back(static_cast<uint8_t>(c.type));

        switch (c.type) {
            case ConstantType::NONE:
                break;

            case ConstantType::BOOL: {
                bool val = std::get<bool>(c.value);
                result.push_back(val ? 1 : 0);
                break;
            }

            case ConstantType::INT: {
                int64_t val = std::get<int64_t>(c.value);
                uint64_t uval = *reinterpret_cast<uint64_t*>(&val);
                for (int i = 7; i >= 0; i--) {
                    result.push_back((uval >> (i * 8)) & 0xFF);
                }
                break;
            }

            case ConstantType::FLOAT: {
                double val = std::get<double>(c.value);
                uint64_t bits;
                std::memcpy(&bits, &val, sizeof(double));
                for (int i = 7; i >= 0; i--) {
                    result.push_back((bits >> (i * 8)) & 0xFF);
                }
                break;
            }

            case ConstantType::STRING: {
                const std::string& str = std::get<std::string>(c.value);
                uint32_t len = str.length();
                result.push_back((len >> 24) & 0xFF);
                result.push_back((len >> 16) & 0xFF);
                result.push_back((len >> 8) & 0xFF);
                result.push_back(len & 0xFF);
                result.insert(result.end(), str.begin(), str.end());
                break;
            }
        }
    }

    return result;
}

} // namespace volta::bytecode
