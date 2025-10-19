#pragma once
#include <string>

namespace Semantic {

// Converts a file path to a module name
// Example: "std/io.vlt" → "std.io"
//          "./utils.vlt" → "utils"
//          "math/advanced.vlt" → "math.advanced"
std::string fileToModule(std::string path);

// Converts a module name to a file path
// Example: "std.io" → "std/io.vlt"
//          "utils" → "utils.vlt"
//          "math.advanced" → "math/advanced.vlt"
std::string moduleToFile(const std::string& moduleName);

}  // namespace Semantic
