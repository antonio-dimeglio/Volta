#include "Semantic/ModuleUtils.hpp"
#include <algorithm>

namespace Semantic {

std::string fileToModule(std::string path) {
    // Remove leading "./" or ".\" if present
    if (path.rfind("./", 0) == 0 || path.rfind(".\\", 0) == 0)
        path = path.substr(2);

    // Remove file extension (.vlt)
    size_t dotPos = path.rfind('.');
    if (dotPos != std::string::npos)
        path = path.substr(0, dotPos);

    // Replace path separators with dots
    std::replace(path.begin(), path.end(), '/', '.');
    std::replace(path.begin(), path.end(), '\\', '.');

    return path;
}

std::string moduleToFile(const std::string& moduleName) {
    std::string path = moduleName;

    // Replace dots with path separators
    std::replace(path.begin(), path.end(), '.', '/');

    // Add .vlt extension
    path += ".vlt";

    return path;
}

}  // namespace Semantic
