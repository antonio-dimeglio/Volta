#include "vir/VIRLowering.hpp"
namespace volta::vir {

VIRLowering::VIRLowering(const ast::Program* program,
                          const semantic::SemanticAnalyzer* analyzer)
    : program_(program), analyzer_(analyzer) {}

// ============================================================================
// Public API
// ============================================================================

std::unique_ptr<VIRModule> VIRLowering::lower() {
    module_ = std::make_unique<VIRModule>("main");

    for (auto& stmt : program_->statements) {
        if (ast::FnDeclaration* fnDecl = dynamic_cast<ast::FnDeclaration*>(stmt.get())) {
            auto virFn = lowerFunctionDeclaration(fnDecl);
            module_->addFunction(std::move(virFn));
        } else {
            lowerStatement(stmt.get());
        }
    }

    return std::move(module_);
}

// ============================================================================
// Top-Level Lowering
// ============================================================================

void VIRLowering::monomorphizeGenerics() {
    // TODO: Implement in Phase 10
}

std::string VIRLowering::mangleName(const std::string& baseName,
                                     const std::vector<std::shared_ptr<volta::semantic::Type>>& typeArgs) {
    // TODO: Implement in Phase 10
    return baseName;
}

// ============================================================================
// Declaration Lowering
// ============================================================================

std::unique_ptr<VIRFunction> VIRLowering::lowerFunctionDeclaration(const volta::ast::FnDeclaration* fn) {
    std::vector<VIRParam> params;

    // For now, we kind of assume all functions are public.
    bool isPublic = true;

    for (auto& param : fn->parameters) {
        auto paramType = analyzer_->resolveTypeAnnotation(param->type.get());

        params.push_back(
            VIRParam(
                param->identifier,
                paramType,
                param->isMutable));
    }
    std::shared_ptr<semantic::Type> returnType = analyzer_->resolveTypeAnnotation(fn->returnType.get());
    std::unique_ptr<VIRBlock> body = lowerBlock(fn->body.get());

    return std::make_unique<VIRFunction>(
        fn->identifier,
        params,
        returnType,
        std::move(body),
        isPublic);
}

std::unique_ptr<VIRStructDecl> VIRLowering::lowerStructDeclaration(const volta::ast::StructDeclaration* structDecl) {
    // TODO: Implement in Phase 8
    return nullptr;
}

std::unique_ptr<VIREnumDecl> VIRLowering::lowerEnumDeclaration(const volta::ast::EnumDeclaration* enumDecl) {
    // TODO: Implement in Phase 9
    return nullptr;
}

// ============================================================================
// Statement Lowering
// ============================================================================

std::unique_ptr<VIRStmt> VIRLowering::lowerStatement(const volta::ast::Statement* stmt) {
    if (auto* ret = dynamic_cast<const ast::ReturnStatement*>(stmt)) {
        return lowerReturnStatement(ret);
    }
    if (auto* varDecl = dynamic_cast<const ast::VarDeclaration*>(stmt)) {
        return lowerVarDeclaration(varDecl);
    }
    if (auto* ifStmt = dynamic_cast<const ast::IfStatement*>(stmt)) {
        return lowerIfStatement(ifStmt);
    }
    if (auto* whileStmt = dynamic_cast<const ast::WhileStatement*>(stmt)) {
        return lowerWhileStatement(whileStmt);
    }
    if (auto* forStmt = dynamic_cast<const ast::ForStatement*>(stmt)) {
        return lowerForStatement(forStmt);
    }
    if (auto* breakStmt = dynamic_cast<const ast::BreakStatement*>(stmt)) {
        return lowerBreakStatement(breakStmt);
    }
    if (auto* continueStmt = dynamic_cast<const ast::ContinueStatement*>(stmt)) {
        return lowerContinueStatement(continueStmt);
    }

    // Handle ExpressionStatement (wraps expressions used as statements)
    if (auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(stmt)) {
        // Lower the inner expression
        auto expr = lowerExpression(exprStmt->expr.get());

        // Wrap in VIRExprStmt
        return std::make_unique<VIRExprStmt>(
            std::move(expr),
            stmt->location,
            nullptr
        );
    }

    // Unknown statement type
    return nullptr;
}

std::unique_ptr<VIRBlock> VIRLowering::lowerBlock(const volta::ast::Block* block) {
    auto virBlock = std::make_unique<VIRBlock>(
        block->location,
        nullptr);  // AST tracking not needed for minimal case

    for (auto& stmt : block->statements) {
        auto virStmt = lowerStatement(stmt.get());
        virBlock->addStatement(std::move(virStmt));
    }

    return virBlock;
}

std::unique_ptr<VIRStmt> VIRLowering::lowerVarDeclaration(const volta::ast::VarDeclaration* varDecl) {
    auto type = analyzer_->getExpressionType(varDecl->initializer.get());

    return std::make_unique<VIRVarDecl>(
        varDecl->identifier,
        lowerExpression(varDecl->initializer.get()),
        varDecl->isMutable,
        type,
        varDecl->location,
        nullptr // This should be passed correctly, as it provides useful info!!
    );
}

std::unique_ptr<VIRStmt> VIRLowering::lowerReturnStatement(const volta::ast::ReturnStatement* ret) {
    // If void return the expression is just gonna be a nullptr.
    return std::make_unique<VIRReturn>(
        !ret->expression ? nullptr : lowerExpression(ret->expression.get()),
        ret->location,
        nullptr  // AST tracking not needed for minimal case
    );
}

std::unique_ptr<VIRStmt> VIRLowering::lowerIfStatement(const volta::ast::IfStatement* ifStmt) {
    auto cond = lowerExpression(ifStmt->condition.get());
    auto thenBlock = lowerBlock(ifStmt->thenBlock.get());
    auto elseBlock = ifStmt->elseBlock ? lowerBlock(ifStmt->elseBlock.get()) : nullptr;

    return std::make_unique<VIRIfStmt>(
        std::move(cond), 
        std::move(thenBlock), 
        elseBlock ? std::move(elseBlock) : nullptr, 
        ifStmt->location, 
        nullptr);
}

std::unique_ptr<VIRStmt> VIRLowering::lowerWhileStatement(const volta::ast::WhileStatement* whileStmt) {
    auto cond = lowerExpression(whileStmt->condition.get());
    auto body = lowerBlock(whileStmt->thenBlock.get());
    return std::make_unique<VIRWhile>(
        std::move(cond),
        std::move(body),
        whileStmt->location,
        nullptr
    );
}

std::unique_ptr<VIRStmt> VIRLowering::lowerForStatement(const volta::ast::ForStatement* forStmt) {
    if (auto* binExpr = dynamic_cast<const ast::BinaryExpression*>(forStmt->expression.get())){
        if (binExpr->op == ast::BinaryExpression::Operator::Range || 
            binExpr->op == ast::BinaryExpression::Operator::RangeInclusive) {
            // Desugar range-based for loop to while loop
            return lowerForRange(forStmt);
        }
    }
    
    // TODO: Handle iterables
    return nullptr;
}

std::unique_ptr<VIRStmt> VIRLowering::lowerBreakStatement(const volta::ast::BreakStatement* breakStmt) {
    return std::make_unique<VIRBreak>(
        breakStmt->location,
        nullptr
    );
}

std::unique_ptr<VIRStmt> VIRLowering::lowerContinueStatement(const volta::ast::ContinueStatement* continueStmt) {
    return std::make_unique<VIRContinue>(
        continueStmt->location,
        nullptr
    );
}

std::unique_ptr<VIRStmt> VIRLowering::lowerForRange(const volta::ast::ForStatement* forStmt) {
    // Create VIRForRange node - codegen will handle the proper loop structure with increment block
    auto* rangeExpr = dynamic_cast<const ast::BinaryExpression*>(forStmt->expression.get());

    auto startValue = lowerExpression(rangeExpr->left.get());
    auto endValue = lowerExpression(rangeExpr->right.get());
    auto loopVarType = analyzer_->getExpressionType(rangeExpr->left.get());
    auto body = lowerBlock(forStmt->thenBlock.get());

    bool inclusive = (rangeExpr->op == ast::BinaryExpression::Operator::RangeInclusive);

    return std::make_unique<VIRForRange>(
        forStmt->identifier,
        std::move(startValue),
        std::move(endValue),
        inclusive,
        std::move(body),
        loopVarType,
        forStmt->location,
        nullptr
    );
}

std::unique_ptr<VIRBlock> VIRLowering::lowerForArray(const volta::ast::ForStatement* forStmt) {
    // TODO: Implement in Phase 5
    return nullptr;
}

// ============================================================================
// Expression Lowering
// ============================================================================

std::unique_ptr<VIRExpr> VIRLowering::lowerExpression(const volta::ast::Expression* expr) {
    if (auto* intLit = dynamic_cast<const ast::IntegerLiteral*>(expr)) {
        return lowerIntegerLiteral(intLit);
    }
    if (auto* boolLit = dynamic_cast<const ast::BooleanLiteral*>(expr)) {
        return lowerBooleanLiteral(boolLit);
    }
    if (auto* floatLit = dynamic_cast<const ast::FloatLiteral*>(expr)) {
        return lowerFloatLiteral(floatLit);
    }
    if (auto* strLit = dynamic_cast<const ast::StringLiteral*>(expr)) {
        return lowerStringLiteral(strLit);
    }
    if (auto* arrLit = dynamic_cast<const ast::ArrayLiteral*>(expr)) {
        return lowerArrayLiteral(arrLit);
    }
    if (auto* iden = dynamic_cast<const ast::IdentifierExpression*>(expr)) {
        return lowerIdentifierExpression(iden);
    }
    if (auto* bin = dynamic_cast<const ast::BinaryExpression*>(expr)) {
        return lowerBinaryExpression(bin);
    }
    if (auto* una = dynamic_cast<const ast::UnaryExpression*>(expr)) {
        return lowerUnaryExpression(una);
    }
    if (auto* call = dynamic_cast<const ast::CallExpression*>(expr)) {
        return lowerCallExpression(call);
    }
    if (auto* call = dynamic_cast<const ast::MethodCallExpression*>(expr)) {
        return lowerMethodCallExpression(call);
    }
    if (auto* ifExpr = dynamic_cast<const ast::IfExpression*>(expr)) {
        return lowerIfExpression(ifExpr);
    }
    if (auto* idx = dynamic_cast<const ast::IndexExpression*>(expr)) {
        return lowerIndexExpression(idx);
    }

    // for voids
    return nullptr;
}

// Literals

std::unique_ptr<VIRExpr> VIRLowering::lowerIntegerLiteral(const volta::ast::IntegerLiteral* lit) {
    auto type = analyzer_->getExpressionType(lit);
    return std::make_unique<VIRIntLiteral>(
        lit->value,
        type,
        lit->location,
        nullptr  // AST tracking not needed for minimal case
    );
}

std::unique_ptr<VIRExpr> VIRLowering::lowerFloatLiteral(const volta::ast::FloatLiteral* lit) {
    auto type = analyzer_->getExpressionType(lit);
    return std::make_unique<VIRFloatLiteral>(
        lit->value,
        type,
        lit->location,
        nullptr  // AST tracking not needed for minimal case
    );
}

std::unique_ptr<VIRExpr> VIRLowering::lowerBooleanLiteral(const volta::ast::BooleanLiteral* lit) {
    return std::make_unique<VIRBoolLiteral>(
        lit->value,
        lit->location,
        nullptr  // AST tracking not needed for minimal case
    );
}

std::unique_ptr<VIRExpr> VIRLowering::lowerStringLiteral(const volta::ast::StringLiteral* lit) {
    // TODO: Implement in Phase 3
    return nullptr;
}

std::unique_ptr<VIRExpr> VIRLowering::lowerArrayLiteral(const volta::ast::ArrayLiteral* lit) {
    // TODO: Implement in Phase 6
    return nullptr;
}

// Identifiers

std::unique_ptr<VIRExpr> VIRLowering::lowerIdentifierExpression(const volta::ast::IdentifierExpression* expr) {
    auto type = analyzer_->getExpressionType(expr);

    // Create reference to the variable (gives us the pointer)
    auto varRef = std::make_unique<VIRLocalRef>(
        expr->name,
        type,
        expr->location,
        nullptr
    );

    // Wrap in Load to get the value
    return std::make_unique<VIRLoad>(
        std::move(varRef),
        type,
        expr->location,
        nullptr
    );
}

// Operations

std::unique_ptr<VIRExpr> VIRLowering::lowerBinaryExpression(const volta::ast::BinaryExpression* expr) {
    using Op = volta::ast::BinaryExpression::Operator;

    if (expr->op == Op::Assign ||
        expr->op == Op::AddAssign ||
        expr->op == Op::SubtractAssign ||
        expr->op == Op::MultiplyAssign ||
        expr->op == Op::DivideAssign ||
        expr->op == Op::ModuloAssign) {
        return lowerAssignment(expr);
    }

    auto lhs = lowerExpression(expr->left.get());
    auto rhs = lowerExpression(expr->right.get());
    auto op = convertBinaryOp(expr->op);
    auto type = analyzer_->getExpressionType(expr);
    return std::make_unique<VIRBinaryOp>(
        op,
        std::move(lhs),
        std::move(rhs),
        type,
        expr->location,
        nullptr
    );
}

std::unique_ptr<VIRExpr> VIRLowering::lowerUnaryExpression(const volta::ast::UnaryExpression* expr) {
    auto operand = lowerExpression(expr->operand.get());
    auto op = convertUnaryOp(expr->op);
    auto type = analyzer_->getExpressionType(expr);
    return std::make_unique<VIRUnaryOp>(
        op,
        std::move(operand),
        type,
        expr->location,
        nullptr
    );
}

std::unique_ptr<VIRExpr> VIRLowering::lowerAssignment(const volta::ast::BinaryExpression* expr) {
    auto* identExpr = dynamic_cast<const ast::IdentifierExpression*>(expr->left.get());
    
    if (!identExpr) {
        // TODO: Handle other lvalues (array access, member access, etc.)
        // For now, only support simple identifiers
        return nullptr;  // or throw error
    }

    std::string varName = identExpr->name;
    auto rhs = lowerExpression(expr->right.get());

    if (expr->op == ast::BinaryExpression::Operator::Assign) {
        auto pointer = std::make_unique<VIRLocalRef>(
            varName,
            analyzer_->getExpressionType(expr->left.get()),
            expr->left->location,
            nullptr
        );
        
        return std::make_unique<VIRStore>(
            std::move(pointer),
            std::move(rhs),
            expr->location,
            nullptr
        );
    } else if (expr->op == ast::BinaryExpression::Operator::AddAssign ||
               expr->op == ast::BinaryExpression::Operator::SubtractAssign ||
               expr->op == ast::BinaryExpression::Operator::MultiplyAssign ||
               expr->op == ast::BinaryExpression::Operator::DivideAssign ||
               expr->op == ast::BinaryExpression::Operator::ModuloAssign) {
        // Compound assignment: x op= rhs  becomes  x = x op rhs

        auto varType = analyzer_->getExpressionType(expr->left.get());

        // Create pointer for loading current value
        auto loadPointer = std::make_unique<VIRLocalRef>(
            varName,
            varType,
            expr->left->location,
            nullptr
        );

        // Load current value
        auto currentValue = std::make_unique<VIRLoad>(
            std::move(loadPointer),
            varType,
            expr->left->location,
            nullptr
        );

        // Determine which binary operation to use
        VIRBinaryOpKind binOp;
        switch (expr->op) {
            case ast::BinaryExpression::Operator::AddAssign:
                binOp = VIRBinaryOpKind::Add;
                break;
            case ast::BinaryExpression::Operator::SubtractAssign:
                binOp = VIRBinaryOpKind::Sub;
                break;
            case ast::BinaryExpression::Operator::MultiplyAssign:
                binOp = VIRBinaryOpKind::Mul;
                break;
            case ast::BinaryExpression::Operator::DivideAssign:
                binOp = VIRBinaryOpKind::Div;
                break;
            case ast::BinaryExpression::Operator::ModuloAssign:
                binOp = VIRBinaryOpKind::Mod;
                break;
            default:
                binOp = VIRBinaryOpKind::Add; // Unreachable
                break;
        }

        // Create the operation: currentValue op rhs
        auto newValue = std::make_unique<VIRBinaryOp>(
            binOp,
            std::move(currentValue),
            std::move(rhs),
            varType,
            expr->location,
            nullptr
        );

        // Create pointer for storing result
        auto storePointer = std::make_unique<VIRLocalRef>(
            varName,
            varType,
            expr->left->location,
            nullptr
        );

        // Store the result back
        return std::make_unique<VIRStore>(
            std::move(storePointer),
            std::move(newValue),
            expr->location,
            nullptr
        );
    }

    // Unsupported assignment type
    return nullptr;
}

// Calls

std::unique_ptr<VIRExpr> VIRLowering::lowerCallExpression(const volta::ast::CallExpression* call) {
    auto* identCallee = dynamic_cast<const ast::IdentifierExpression*>(call->callee.get());

    if (!identCallee) return nullptr; // We do not yet support method calls.
    std::string funcName = identCallee->name;

    std::vector<std::unique_ptr<VIRExpr>> args;
    for (auto& arg : call->arguments) {
        args.push_back(lowerExpression(arg.get()));
    }

    auto returnType = analyzer_->getExpressionType(call);
    return std::make_unique<VIRCall>(funcName, std::move(args), returnType, call->location, nullptr);
}

std::unique_ptr<VIRExpr> VIRLowering::lowerMethodCallExpression(const volta::ast::MethodCallExpression* call) {
    // TODO: Implement in Phase 7
    return nullptr;
}

std::unique_ptr<VIRCall> VIRLowering::desugarMethodCall(const volta::ast::MethodCallExpression* call) {
    // TODO: Implement in Phase 7
    return nullptr;
}

// Control Flow

std::unique_ptr<VIRExpr> VIRLowering::lowerIfExpression(const volta::ast::IfExpression* expr) {
    // TODO: Currently only supports simple expressions in branches (ternary-operator style).
    // To support statement blocks with final values (like Rust), we need BlockExpression in AST.
    // Example that doesn't work yet: if x { let temp = 10; temp * 2 } else { 0 }
    return nullptr;
}

// Array operations

std::unique_ptr<VIRExpr> VIRLowering::lowerIndexExpression(const volta::ast::IndexExpression* expr) {
    // TODO: Implement in Phase 6
    return nullptr;
}

// ============================================================================
// Fixed Array Operations (Phase 7)
// ============================================================================

std::unique_ptr<VIRExpr> VIRLowering::lowerFixedArrayLiteral(const volta::ast::ArrayLiteral* lit) {
    // TODO: Implement in Phase 7 Week 7 Day 3
    // 1. Determine if this is a fixed array literal (check type annotation)
    // 2. Lower all element expressions
    // 3. Perform escape analysis to determine isStackAllocated
    // 4. Create VIRFixedArrayNew node with elements, size, and allocation strategy
    return nullptr;
}

std::unique_ptr<VIRExpr> VIRLowering::lowerFixedArrayGet(const volta::ast::IndexExpression* expr) {
    // TODO: Implement in Phase 7 Week 7 Day 3
    // 1. Lower the array expression
    // 2. Lower the index expression
    // 3. Insert VIRBoundsCheck around index
    // 4. Create VIRFixedArrayGet node
    return nullptr;
}

std::unique_ptr<VIRExpr> VIRLowering::lowerFixedArraySet(const volta::ast::IndexExpression* lhs,
                                                          std::unique_ptr<VIRExpr> value) {
    // TODO: Implement in Phase 7 Week 7 Day 3
    // 1. Lower the array expression
    // 2. Lower the index expression
    // 3. Insert VIRBoundsCheck around index
    // 4. Create VIRFixedArraySet node with array, index, and value
    return nullptr;
}

bool VIRLowering::shouldStackAllocate(const volta::ast::VarDeclaration* varDecl) {
    // TODO: Implement in Phase 7 Week 7 Day 3
    // Escape analysis criteria:
    // 1. Check if variable is returned from function
    // 2. Check if variable is captured in closure
    // 3. Check array size (threshold: 1KB or ~256 elements)
    // 4. Check if variable is passed by reference to functions that may store it
    // For now, default to heap allocation (conservative)
    return false;
}

// ============================================================================
// Helper Functions
// ============================================================================

std::unique_ptr<VIRExpr> VIRLowering::insertImplicitCast(std::unique_ptr<VIRExpr> expr,
                                                          std::shared_ptr<volta::semantic::Type> targetType) {
    // TODO: Implement in Phase 2
    return expr;
}

std::string VIRLowering::internString(const std::string& value) {
    auto it = internedStrings_.find(value);
    if (it != internedStrings_.end()) {
        return it->second;
    }

    std::string internedName = ".str." + std::to_string(stringCounter_++);
    internedStrings_[value] = internedName;
    return internedName;
}

std::string VIRLowering::generateTempName() {
    return ".tmp." + std::to_string(tempCounter_++);
}

std::shared_ptr<volta::semantic::Type> VIRLowering::getType(const volta::ast::Expression* expr) {
    return analyzer_->getExpressionType(expr);
}

VIRBinaryOpKind VIRLowering::convertBinaryOp(volta::ast::BinaryExpression::Operator op) {
    using ASTOp = volta::ast::BinaryExpression::Operator;

    switch (op) {
        // Arithmetic
        case ASTOp::Add:        return VIRBinaryOpKind::Add;
        case ASTOp::Subtract:   return VIRBinaryOpKind::Sub;
        case ASTOp::Multiply:   return VIRBinaryOpKind::Mul;
        case ASTOp::Divide:     return VIRBinaryOpKind::Div;
        case ASTOp::Modulo:     return VIRBinaryOpKind::Mod;

        // Comparison
        case ASTOp::Equal:          return VIRBinaryOpKind::Eq;
        case ASTOp::NotEqual:       return VIRBinaryOpKind::Ne;
        case ASTOp::Less:           return VIRBinaryOpKind::Lt;
        case ASTOp::LessEqual:      return VIRBinaryOpKind::Le;
        case ASTOp::Greater:        return VIRBinaryOpKind::Gt;
        case ASTOp::GreaterEqual:   return VIRBinaryOpKind::Ge;

        // Logical
        case ASTOp::LogicalAnd: return VIRBinaryOpKind::And;
        case ASTOp::LogicalOr:  return VIRBinaryOpKind::Or;

        // Unsupported in VIR (should be desugared or handled elsewhere)
        case ASTOp::Power:
        case ASTOp::Range:
        case ASTOp::RangeInclusive:
        case ASTOp::Assign:
        case ASTOp::AddAssign:
        case ASTOp::SubtractAssign:
        case ASTOp::MultiplyAssign:
        case ASTOp::DivideAssign:
        case ASTOp::ModuloAssign:
            // These should not appear in expression context or should be desugared
            // For now, return Add as a placeholder (will cause errors if hit)
            return VIRBinaryOpKind::Add;
    }

    return VIRBinaryOpKind::Add; // Unreachable, but silences warnings
}

VIRUnaryOpKind VIRLowering::convertUnaryOp(volta::ast::UnaryExpression::Operator op) {
    using ASTOp = volta::ast::UnaryExpression::Operator;

    switch (op) {
        case ASTOp::Negate: return VIRUnaryOpKind::Neg;
        case ASTOp::Not:    return VIRUnaryOpKind::Not;
    }

    return VIRUnaryOpKind::Neg; // Unreachable, but silences warnings
}

} // namespace volta::vir
