#pragma once

#include "AST.hpp"
#include "../HIR/HIR.hpp"
#include <iostream>
#include <string>

class ASTPrinter {
public:
    ASTPrinter() = default;

    // Print the entire program
    void print(const Program& program, std::ostream& os = std::cout);
    void print(const HIR::HIRProgram& program, std::ostream& os = std::cout);
};