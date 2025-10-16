#pragma once

#include "../Parser/AST.hpp"
#include <memory>
#include <vector>

// HIR (High-level Intermediate Representation)
// This is a desugared version of the AST where:
// - Compound assignments (+=, -=, etc.) are expanded to simple assignments
// - Increment/decrement (++, --) are expanded to assignments
// - For loops will be expanded to while loops (future)
// - Other syntactic sugar is removed

// For now, HIR uses the same node types as AST
// But compound assignments, increment, and decrement are desugared
// In the future, this will be a separate IR with its own node types
namespace HIR {

// Type alias for now - HIR reuses AST nodes
using Program = ::Program;
using Stmt = ::Stmt;
using Expr = ::Expr;
using Type = ::Type;

} // namespace HIR
