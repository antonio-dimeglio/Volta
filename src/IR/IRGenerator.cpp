#include "IR/IRGenerator.hpp"

namespace volta::ir {

// ============================================================================
// Main Entry Point
// ============================================================================

std::unique_ptr<IRModule> IRGenerator::generate(
    const volta::ast::Program& program,
    const volta::semantic::SemanticAnalyzer& analyzer) {
    analyzer_ = &analyzer;
    module_ = std::make_unique<IRModule>("main");

    // Register built-in functions
    registerBuiltinFunctions();

    generateProgram(program);
    if (reporter_.hasErrors()) return nullptr;

    return std::move(module_);
}

// ============================================================================
// Top-Level Generation
// ============================================================================

void IRGenerator::generateProgram(const volta::ast::Program& program) {

    // First pass - collect all function and struct declarations
    // This allows forward references (calling functions before they're defined)
    for (const auto& stmt : program.statements) {
        if (auto* funcDecl = dynamic_cast<volta::ast::FnDeclaration*>(stmt.get())) {
            generateFunction(*funcDecl);
        } else if (auto* structDecl = dynamic_cast<volta::ast::StructDeclaration*>(stmt.get())) {
            generateStruct(*structDecl);
        }
    }

    // Second pass - generate global variables
    for (const auto& stmt : program.statements) {
        if (auto* varDecl = dynamic_cast<volta::ast::VarDeclaration*>(stmt.get())) {
            generateGlobal(*varDecl);
        }
    }

    // Third pass - collect top-level statements (that aren't function/struct/var declarations)
    // These will be wrapped in a special __init__ function
    std::vector<const volta::ast::Statement*> topLevelStmts;
    for (const auto& stmt : program.statements) {
        if (!dynamic_cast<volta::ast::FnDeclaration*>(stmt.get()) &&
            !dynamic_cast<volta::ast::StructDeclaration*>(stmt.get()) &&
            !dynamic_cast<volta::ast::VarDeclaration*>(stmt.get())) {
            topLevelStmts.push_back(stmt.get());
        }
    }

    // If there are top-level statements, create an __init__ function for them
    if (!topLevelStmts.empty()) {
        generateInitFunction(topLevelStmts);
    }
}

void IRGenerator::generateFunction(const volta::ast::FnDeclaration& funcDecl) {
    // Resolve parameter types
    std::vector<std::shared_ptr<semantic::Type>> paramTypes;
    for (const auto& param : funcDecl.parameters) {
        paramTypes.push_back(resolveType(param->type.get()));
    }

    // Resolve return type
    auto returnType = resolveType(funcDecl.returnType.get());

    // Create function type
    auto funcType = std::make_shared<semantic::FunctionType>(paramTypes, returnType);

    // Determine function name (qualified for methods)
    std::string functionName = funcDecl.identifier;
    if (funcDecl.isMethod) {
        functionName = funcDecl.receiverType + "." + funcDecl.identifier;
    }

    // Create IR function
    auto function = builder_.createFunction(functionName, funcType);

    // Add function to module
    module_->addFunction(std::move(function));

    // Get raw pointer to the function we just added
    Function* funcPtr = module_->functions().back().get();

    // Register in function map (use qualified name for methods)
    functionMap_[functionName] = funcPtr;

    // Set as current function
    currentFunction_ = funcPtr;

    // Create entry basic block
    auto entryBB = builder_.createBasicBlock("entry", currentFunction_);
    currentFunction_->addBasicBlock(std::move(entryBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());

    // Create allocas for parameters and store parameter values
    for (size_t i = 0; i < funcDecl.parameters.size(); i++) {
        auto* param = currentFunction_->parameters()[i].get();
        auto* alloca = builder_.createAlloc(param->type());
        builder_.createStore(param, alloca);
        declareVariable(funcDecl.parameters[i]->identifier, alloca);
    }

    // Generate function body
    if (funcDecl.body) {
        // Block body
        for (const auto& stmt : funcDecl.body->statements) {
            generateStatement(stmt.get());
        }
    } else if (funcDecl.expressionBody) {
        // Expression body
        Value* result = generateExpression(funcDecl.expressionBody.get());
        builder_.createRet(result);
    }

    // Add implicit return if function doesn't end with one
    if (!builder_.insertPoint()->hasTerminator()) {
        if (returnType->kind() == semantic::Type::Kind::Void) {
            builder_.createRetVoid();
        } else {
            // This should have been caught by semantic analysis
            error("Function must end with a return statement", funcDecl.location);
        }
    }

    // Clear variable map for next function
    variableMap_.clear();

    // Reset current function
    currentFunction_ = nullptr;
}

void IRGenerator::generateStruct(const volta::ast::StructDeclaration& structDecl) {
    // Structs are just type information, no IR code needed
    // The semantic analyzer already has the struct type definitions
    // Nothing to generate here
}

void IRGenerator::generateInitFunction(const std::vector<const volta::ast::Statement*>& stmts) {
    // Create a void type for the init function
    auto voidType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void);

    // Create function type with no parameters
    std::vector<std::shared_ptr<semantic::Type>> paramTypes;
    auto funcType = std::make_shared<semantic::FunctionType>(paramTypes, voidType);

    // Create IR function for __init__
    auto function = builder_.createFunction("__init__", funcType);

    // Add function to module
    module_->addFunction(std::move(function));

    // Get raw pointer to the function we just added
    Function* funcPtr = module_->functions().back().get();

    // Register in function map
    functionMap_["__init__"] = funcPtr;

    // Set as current function
    currentFunction_ = funcPtr;

    // Create entry basic block
    auto entryBB = builder_.createBasicBlock("entry", currentFunction_);
    currentFunction_->addBasicBlock(std::move(entryBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());

    // Generate all top-level statements
    for (const auto* stmt : stmts) {
        generateStatement(stmt);
    }

    // Add return void at the end
    if (!builder_.insertPoint()->hasTerminator()) {
        builder_.createRetVoid();
    }

    // Clear variable map
    variableMap_.clear();

    // Reset current function
    currentFunction_ = nullptr;
}

void IRGenerator::generateGlobal(const volta::ast::VarDeclaration& varDecl) {
    // Resolve the variable's type
    std::shared_ptr<semantic::Type> type;
    if (varDecl.typeAnnotation) {
        type = resolveType(varDecl.typeAnnotation->type.get());
    } else {
        // Type inference: get type from initializer
        type = analyzer_->getExpressionType(varDecl.initializer.get());
    }

    // For global variables with initializers, we need to evaluate them
    // In a full implementation, initializers must be constant expressions
    // For now, we'll create globals without initializers (they'll be initialized at runtime)

    std::shared_ptr<Constant> initializer = nullptr;

    // Try to evaluate constant initializers
    if (varDecl.initializer) {
        // Only handle literal initializers for now
        if (auto* intLit = dynamic_cast<volta::ast::IntegerLiteral*>(varDecl.initializer.get())) {
            auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
            initializer = std::make_shared<Constant>(intType, intLit->value);
        } else if (auto* floatLit = dynamic_cast<volta::ast::FloatLiteral*>(varDecl.initializer.get())) {
            auto floatType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Float);
            initializer = std::make_shared<Constant>(floatType, floatLit->value);
        } else if (auto* boolLit = dynamic_cast<volta::ast::BooleanLiteral*>(varDecl.initializer.get())) {
            auto boolType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Bool);
            initializer = std::make_shared<Constant>(boolType, boolLit->value);
        } else if (auto* strLit = dynamic_cast<volta::ast::StringLiteral*>(varDecl.initializer.get())) {
            auto strType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::String);
            initializer = std::make_shared<Constant>(strType, strLit->value);
        }
        // For non-constant initializers, we'll need to initialize them in a special init function
        // or at the start of main - this is left as future work
    }

    // Create global variable
    auto global = std::make_unique<GlobalVariable>(type, varDecl.identifier, initializer);

    // Add to module and get pointer
    GlobalVariable* globalPtr = module_->addGlobal(std::move(global));

    // Register in variable map so functions can access it
    declareVariable(varDecl.identifier, globalPtr);
}

// ============================================================================
// Statement Generation
// ============================================================================

void IRGenerator::generateStatement(const volta::ast::Statement* stmt) {
    if (auto* varDecl = dynamic_cast<const volta::ast::VarDeclaration*>(stmt)) {
        generateVarDeclaration(*varDecl);
    } else if (auto* exprStmt = dynamic_cast<const volta::ast::ExpressionStatement*>(stmt)) {
        generateExpressionStatement(*exprStmt);
    } else if (auto* ifStmt = dynamic_cast<const volta::ast::IfStatement*>(stmt)) {
        generateIfStatement(*ifStmt);
    } else if (auto* whStmt = dynamic_cast<const volta::ast::WhileStatement*>(stmt)) {
        generateWhileStatement(*whStmt);
    } else if (auto* forStmt = dynamic_cast<const volta::ast::ForStatement*>(stmt)) {
        generateForStatement(*forStmt);
    } else if (auto* retStmt = dynamic_cast<const volta::ast::ReturnStatement*>(stmt)) {
        generateReturnStatement(*retStmt);
    } else if (auto* blkStmt = dynamic_cast<const volta::ast::Block*>(stmt)) {
        generateBlock(*blkStmt);
    } 

}

void IRGenerator::generateVarDeclaration(const volta::ast::VarDeclaration& varDecl) {
    // Resolve the variable's type
    std::shared_ptr<semantic::Type> type;
    if (varDecl.typeAnnotation) {
        type = resolveType(varDecl.typeAnnotation->type.get());
    } else {
        // Type inference: get type from initializer
        type = analyzer_->getExpressionType(varDecl.initializer.get());
    }

    // Create alloca for the variable
    Value* alloca = builder_.createAlloc(type);

    // If there's an initializer, generate it and store
    if (varDecl.initializer) {
        Value* initValue = generateExpression(varDecl.initializer.get());
        builder_.createStore(initValue, alloca);
    }

    // Register the variable
    declareVariable(varDecl.identifier, alloca);
}

void IRGenerator::generateExpressionStatement(const volta::ast::ExpressionStatement& stmt) {
    generateExpression(stmt.expr.get());
}

void IRGenerator::generateIfStatement(const volta::ast::IfStatement& stmt) {
    // Generate condition expression
    Value* condValue = generateExpression(stmt.condition.get());

    // Create basic blocks
    auto thenBB = builder_.createBasicBlock("then", currentFunction_);
    auto elseBB = builder_.createBasicBlock("else", currentFunction_);
    auto mergeBB = builder_.createBasicBlock("merge", currentFunction_);

    // Emit conditional branch
    builder_.createBrIf(condValue, thenBB.get(), elseBB.get());

    // Generate then block
    currentFunction_->addBasicBlock(std::move(thenBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
    generateStatement(stmt.thenBlock.get());
    bool thenHasTerminator = builder_.insertPoint()->hasTerminator();
    if (!thenHasTerminator) {
        builder_.createBr(mergeBB.get());
    }

    // Generate else block
    currentFunction_->addBasicBlock(std::move(elseBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
    if (stmt.elseBlock != nullptr) {
        generateStatement(stmt.elseBlock.get());
    }
    bool elseHasTerminator = builder_.insertPoint()->hasTerminator();
    if (!elseHasTerminator) {
        builder_.createBr(mergeBB.get());
    }

    // Only add merge block if at least one branch doesn't have a terminator
    if (!thenHasTerminator || !elseHasTerminator) {
        currentFunction_->addBasicBlock(std::move(mergeBB));
        builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
    }
    // If both branches have terminators, the merge block is unreachable
    // Don't set insertion point - function ends here
}

void IRGenerator::generateWhileStatement(const volta::ast::WhileStatement& stmt) {
    // Create basic blocks
    auto condBB = builder_.createBasicBlock("cond", currentFunction_);
    auto bodyBB = builder_.createBasicBlock("body", currentFunction_);
    auto endBB = builder_.createBasicBlock("end", currentFunction_);

    // Emit branch to condition block
    builder_.createBr(condBB.get());

    // Generate condition block
    currentFunction_->addBasicBlock(std::move(condBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
    Value* condValue = generateExpression(stmt.condition.get());
    builder_.createBrIf(condValue, bodyBB.get(), endBB.get());

    // Generate body block
    currentFunction_->addBasicBlock(std::move(bodyBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
    BasicBlock* condBlock = currentFunction_->basicBlocks()[currentFunction_->basicBlocks().size() - 2].get();
    generateStatement(stmt.thenBlock.get());
    if (!builder_.insertPoint()->hasTerminator()) {
        builder_.createBr(condBlock);
    }

    // Continue at end block
    currentFunction_->addBasicBlock(std::move(endBB));
    builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
}

void IRGenerator::generateForStatement(const volta::ast::ForStatement& stmt) {
    if (auto* binExpr = dynamic_cast<volta::ast::BinaryExpression*>(stmt.expression.get())) {
        if (binExpr->op == volta::ast::BinaryExpression::Operator::Range ||
            binExpr->op == volta::ast::BinaryExpression::Operator::RangeInclusive) {

            // Generate range-based for loop
            // for i in start..end  =>
            //   i = start
            //   while i < end:
            //     body
            //     i = i + 1

            bool isInclusive = (binExpr->op == volta::ast::BinaryExpression::Operator::RangeInclusive);

            // Evaluate range bounds
            Value* startValue = generateExpression(binExpr->left.get());
            Value* endValue = generateExpression(binExpr->right.get());

            // Create loop variable alloca and initialize
            auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
            Value* loopVar = builder_.createAlloc(intType);
            builder_.createStore(startValue, loopVar);
            declareVariable(stmt.identifier, loopVar);

            // Create basic blocks
            auto condBB = builder_.createBasicBlock("for.cond", currentFunction_);
            auto bodyBB = builder_.createBasicBlock("for.body", currentFunction_);
            auto incBB = builder_.createBasicBlock("for.inc", currentFunction_);
            auto endBB = builder_.createBasicBlock("for.end", currentFunction_);

            // Branch to condition
            builder_.createBr(condBB.get());

            // Generate condition block
            currentFunction_->addBasicBlock(std::move(condBB));
            builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
            Value* currentValue = builder_.createLoad(intType, loopVar);
            Value* condValue;
            if (isInclusive) {
                condValue = builder_.createLe(currentValue, endValue);  // i <= end
            } else {
                condValue = builder_.createLt(currentValue, endValue);  // i < end
            }
            builder_.createBrIf(condValue, bodyBB.get(), endBB.get());

            // Generate body block
            currentFunction_->addBasicBlock(std::move(bodyBB));
            builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
            generateStatement(stmt.thenBlock.get());
            if (!builder_.insertPoint()->hasTerminator()) {
                builder_.createBr(incBB.get());
            }

            // Generate increment block
            currentFunction_->addBasicBlock(std::move(incBB));
            builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());
            Value* nextValue = builder_.createAdd(
                builder_.createLoad(intType, loopVar),
                builder_.getInt(1)
            );
            builder_.createStore(nextValue, loopVar);
            BasicBlock* condBlock = currentFunction_->basicBlocks()[currentFunction_->basicBlocks().size() - 4].get();
            builder_.createBr(condBlock);

            // Continue at end block
            currentFunction_->addBasicBlock(std::move(endBB));
            builder_.setInsertPoint(currentFunction_->basicBlocks().back().get());

            return;
        }
    }

    // Otherwise, iterate over array/collection
    // For now, array iteration is not fully implemented
    // This would require:
    // 1. Getting array length from type system or runtime
    // 2. Loading elements at each index
    // 3. Storing into the loop variable
    error("Array iteration in for loops not yet fully implemented", stmt.location);
}

void IRGenerator::generateReturnStatement(const volta::ast::ReturnStatement& stmt) {
    if (stmt.expression == nullptr) {
        builder_.createRetVoid();
    } else {
        Value* value = generateExpression(stmt.expression.get());
        builder_.createRet(value);
    }
}

void IRGenerator::generateBlock(const volta::ast::Block& block) {
    for (auto& stmt : block.statements) {
        generateStatement(stmt.get());
    }
}

// ============================================================================
// Expression Generation
// ============================================================================

Value* IRGenerator::generateExpression(const volta::ast::Expression* expr) {
    if (auto* binExpr = dynamic_cast<const volta::ast::BinaryExpression*>(expr)) {
        return generateBinaryExpression(*binExpr);
    } else if (auto* unrExpr = dynamic_cast<const volta::ast::UnaryExpression*>(expr)) {
        return generateUnaryExpression(*unrExpr);
    } else if (auto* callExpr = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        return generateCallExpression(*callExpr);
    } else if (auto* idxExpr = dynamic_cast<const volta::ast::IndexExpression*>(expr)) {
        return generateIndexExpression(*idxExpr);
    } else if (auto* slcExpr = dynamic_cast<const volta::ast::SliceExpression*>(expr)) {
        return generateSliceExpression(*slcExpr);
    } else if (auto* mbmrExpr = dynamic_cast<const volta::ast::MemberExpression*>(expr)) {
        return generateMemberExpression(*mbmrExpr);
    } else if (auto* mtclExpr = dynamic_cast<const volta::ast::MethodCallExpression*>(expr)) {
        return generateMethodCallExpression(*mtclExpr);
    } else if (auto* ifExpr = dynamic_cast<const volta::ast::IfExpression*>(expr)) {
        return generateIfExpression(*ifExpr);
    } else if (auto* mtcExpr = dynamic_cast<const volta::ast::MatchExpression*>(expr)) {
        return generateMatchExpression(*mtcExpr);
    } else if (auto* lmbExpr = dynamic_cast<const volta::ast::LambdaExpression*>(expr)) {
        return generateLambdaExpression(*lmbExpr);
    } else if (auto* idExpr = dynamic_cast<const volta::ast::IdentifierExpression*>(expr)) {
        return generateIdentifierExpression(*idExpr);
    }  else if (auto* intLit = dynamic_cast<const volta::ast::IntegerLiteral*>(expr)) {
        return generateIntegerLiteral(*intLit);
    } else if (auto* floatLit = dynamic_cast<const volta::ast::FloatLiteral*>(expr)) {
        return generateFloatLiteral(*floatLit);
    }  else if (auto* boolLit = dynamic_cast<const volta::ast::BooleanLiteral*>(expr)) {
        return generateBooleanLiteral(*boolLit);
    }  else if (auto* strLit = dynamic_cast<const volta::ast::StringLiteral*>(expr)) {
        return generateStringLiteral(*strLit);
    } else if (auto* arrLit = dynamic_cast<const volta::ast::ArrayLiteral*>(expr)) {
        return generateArrayLiteral(*arrLit);
    } else if (auto* tupLit = dynamic_cast<const volta::ast::TupleLiteral*>(expr)) {
        return generateTupleLiteral(*tupLit);
    } else if (auto* stcLit = dynamic_cast<const volta::ast::StructLiteral*>(expr)) {
        return generateStructLiteral(*stcLit);
    } else if (auto* someLit = dynamic_cast<const volta::ast::SomeLiteral*>(expr)) {
        return generateSomeLiteral(*someLit);
    } 

    return nullptr;
}

Value* IRGenerator::generateBinaryExpression(const volta::ast::BinaryExpression& expr) {
    using Op = volta::ast::BinaryExpression::Operator;

    // Handle assignment operators separately
    if (expr.op == Op::Assign || expr.op == Op::AddAssign || expr.op == Op::SubtractAssign ||
        expr.op == Op::MultiplyAssign || expr.op == Op::DivideAssign) {

        // Get the left-hand side address (must be a variable or index expression)
        Value* leftAddr = nullptr;
        if (auto* ident = dynamic_cast<volta::ast::IdentifierExpression*>(expr.left.get())) {
            leftAddr = lookupVariable(ident->name);
        } else {
            error("Left side of assignment must be a variable", expr.location);
            return nullptr;
        }

        // Generate right-hand side value
        Value* rightValue = generateExpression(expr.right.get());

        // For compound assignments, we need to load, operate, then store
        if (expr.op != Op::Assign) {
            Value* currentValue = builder_.createLoad(leftAddr->type(), leftAddr);
            switch (expr.op) {
                case Op::AddAssign:
                    rightValue = builder_.createAdd(currentValue, rightValue);
                    break;
                case Op::SubtractAssign:
                    rightValue = builder_.createSub(currentValue, rightValue);
                    break;
                case Op::MultiplyAssign:
                    rightValue = builder_.createMul(currentValue, rightValue);
                    break;
                case Op::DivideAssign:
                    rightValue = builder_.createDiv(currentValue, rightValue);
                    break;
                default:
                    break;
            }
        }

        builder_.createStore(rightValue, leftAddr);
        return rightValue;
    }

    // Handle range operators (these don't produce IR values, used in for loops)
    if (expr.op == Op::Range || expr.op == Op::RangeInclusive) {
        error("Range operators can only be used in for loop expressions", expr.location);
        return nullptr;
    }

    // For all other operators, generate both operands first
    Value* left = generateExpression(expr.left.get());
    Value* right = generateExpression(expr.right.get());

    // Dispatch based on operator
    switch (expr.op) {
        case Op::Add:
            return builder_.createAdd(left, right);
        case Op::Subtract:
            return builder_.createSub(left, right);
        case Op::Multiply:
            return builder_.createMul(left, right);
        case Op::Divide:
            return builder_.createDiv(left, right);
        case Op::Modulo:
            return builder_.createMod(left, right);
        case Op::Power:
            // Power might need special handling or a runtime call
            error("Power operator not yet implemented in IR generation", expr.location);
            return nullptr;
        case Op::Equal:
            return builder_.createEq(left, right);
        case Op::NotEqual:
            return builder_.createNe(left, right);
        case Op::Less:
            return builder_.createLt(left, right);
        case Op::Greater:
            return builder_.createGt(left, right);
        case Op::LessEqual:
            return builder_.createLe(left, right);
        case Op::GreaterEqual:
            return builder_.createGe(left, right);
        case Op::LogicalAnd:
            return builder_.createAnd(left, right);
        case Op::LogicalOr:
            return builder_.createOr(left, right);
        default:
            error("Unknown binary operator", expr.location);
            return nullptr;
    }
}

Value* IRGenerator::generateUnaryExpression(const volta::ast::UnaryExpression& expr) {
    Value* operand = generateExpression(expr.operand.get());

    switch (expr.op) {
        case volta::ast::UnaryExpression::Operator::Not:
            return builder_.createNot(operand);
        case volta::ast::UnaryExpression::Operator::Negate:
            return builder_.createNeg(operand);
        default:
            error("Unknown unary operator", expr.location);
            return nullptr;
    }
}

Value* IRGenerator::generateCallExpression(const volta::ast::CallExpression& expr) {
    // Get the function name (callee should be an identifier)
    auto* identExpr = dynamic_cast<volta::ast::IdentifierExpression*>(expr.callee.get());
    if (!identExpr) {
        error("Function callee must be an identifier", expr.location);
        return nullptr;
    }

    // Look up the function
    auto it = functionMap_.find(identExpr->name);
    if (it == functionMap_.end()) {
        error("Undefined function '" + identExpr->name + "'", expr.location);
        return nullptr;
    }
    Function* function = it->second;

    // Generate IR for each argument
    std::vector<Value*> args;
    for (const auto& arg : expr.arguments) {
        args.push_back(generateExpression(arg.get()));
    }

    // Emit call instruction
    return builder_.createCall(function, args);
}

Value* IRGenerator::generateIndexExpression(const volta::ast::IndexExpression& expr) {
    // TODO: Generate IR for array indexing: array[index]
    //
    // Steps:
    // 1. Generate IR for array expression
    // 2. Generate IR for index expression
    // 3. Emit: builder_.createGetElement(array, index, elementType)
    //
    // Note: You'll need to get the element type from the array type.

    // YOUR CODE HERE
    return nullptr;
}

Value* IRGenerator::generateSliceExpression(const volta::ast::SliceExpression& expr) {
    // TODO: Generate IR for array slicing: array[start:end]

    throw std::runtime_error("Slice expression unsupported for IR generation.");
}

Value* IRGenerator::generateMemberExpression(const volta::ast::MemberExpression& expr) {
    // For member access, we need the address (alloca) not the loaded value
    // Special case: if object is an identifier, get its alloca directly
    Value* object = nullptr;
    if (auto* identExpr = dynamic_cast<const volta::ast::IdentifierExpression*>(expr.object.get())) {
        object = lookupVariable(identExpr->name);
        if (!object) {
            error("Failed to look up variable '" + identExpr->name + "'", expr.location);
            return nullptr;
        }
    } else {
        // For other expressions, generate normally
        object = generateExpression(expr.object.get());
        if (!object) {
            error("Failed to generate object expression", expr.location);
            return nullptr;
        }
    }

    // Get the type of the object
    auto objectType = analyzer_->getExpressionType(expr.object.get());
    if (!objectType) {
        error("Could not determine object type", expr.location);
        return nullptr;
    }

    // Object must be a struct type
    auto* structType = dynamic_cast<const semantic::StructType*>(objectType.get());
    if (!structType) {
        error("Member access on non-struct type", expr.location);
        return nullptr;
    }

    // Find the field index
    const std::string& fieldName = expr.member->name;
    size_t fieldIndex = 0;
    bool found = false;
    const auto& fields = structType->fields();
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].name == fieldName) {
            fieldIndex = i;
            found = true;
            break;
        }
    }

    if (!found) {
        error("Struct '" + structType->name() + "' has no field '" + fieldName + "'", expr.location);
        return nullptr;
    }

    // Get the field type
    auto fieldType = fields[fieldIndex].type;

    // Emit get field instruction
    return builder_.createGetField(object, fieldIndex, fieldType);
}

Value* IRGenerator::generateMethodCallExpression(const volta::ast::MethodCallExpression& expr) {
    // Get the type of the object
    auto objectType = analyzer_->getExpressionType(expr.object.get());
    if (!objectType) {
        error("Could not determine object type for method call", expr.location);
        return nullptr;
    }

    // Object must be a struct type
    auto* structType = dynamic_cast<const semantic::StructType*>(objectType.get());
    if (!structType) {
        error("Method call on non-struct type", expr.location);
        return nullptr;
    }

    // Build qualified method name: "StructName.methodName"
    const std::string& methodName = expr.method->name;
    const std::string qualifiedName = structType->name() + "." + methodName;

    // Look up the method in the function map
    auto it = functionMap_.find(qualifiedName);
    if (it == functionMap_.end()) {
        error("Undefined method '" + qualifiedName + "'", expr.location);
        return nullptr;
    }
    Function* method = it->second;

    // Generate IR for the object (receiver) - this will be the first argument
    Value* receiver = generateExpression(expr.object.get());
    if (!receiver) {
        error("Failed to generate receiver for method call", expr.location);
        return nullptr;
    }

    // Generate IR for each argument
    std::vector<Value*> args;
    args.push_back(receiver);  // First argument is always 'self'
    for (const auto& arg : expr.arguments) {
        Value* argValue = generateExpression(arg.get());
        if (!argValue) {
            error("Failed to generate argument for method call", expr.location);
            return nullptr;
        }
        args.push_back(argValue);
    }

    // Emit call instruction
    return builder_.createCall(method, args);
}

Value* IRGenerator::generateIfExpression(const volta::ast::IfExpression& expr) {
    // TODO: Generate IR for if expression (if as expression)
    //
    // This is like if statement, but both branches must produce a value.
    // In proper SSA, you'd use a phi node to merge the values.
    // For simplified approach: use an alloca to store the result.
    //
    // Steps:
    // 1. Create result alloca
    // 2. Generate if/else branches (like if statement)
    // 3. In each branch, store the branch result to the alloca
    // 4. After merge, load from alloca and return

    // YOUR CODE HERE
    return nullptr;
}

Value* IRGenerator::generateMatchExpression(const volta::ast::MatchExpression& expr) {
    // TODO: Generate IR for match expression (pattern matching)

    throw std::runtime_error("Match expression unsupported for IR generation");
}

Value* IRGenerator::generateLambdaExpression(const volta::ast::LambdaExpression& expr) {
    // TODO: Generate IR for lambda expression after implementing closure support.

    throw std::runtime_error("Lambda expressions unsupported for IR generation");
}

Value* IRGenerator::generateIdentifierExpression(const volta::ast::IdentifierExpression& expr) {
    auto* addr = lookupVariable(expr.name);

    if (addr == nullptr) {
        error("could not find definition for " + expr.name, expr.location);
    }
    return builder_.createLoad(addr->type(), addr);
}

Value* IRGenerator::generateIntegerLiteral(const volta::ast::IntegerLiteral& lit) {
    return builder_.getInt(lit.value);
}

Value* IRGenerator::generateFloatLiteral(const volta::ast::FloatLiteral& lit) {
    return builder_.getFloat(lit.value);
}

Value* IRGenerator::generateStringLiteral(const volta::ast::StringLiteral& lit) {
    return  builder_.getString(lit.value);
}

Value* IRGenerator::generateBooleanLiteral(const volta::ast::BooleanLiteral& lit) {
    return builder_.getBool(lit.value);
}

Value* IRGenerator::generateNoneLiteral(const volta::ast::NoneLiteral& lit) {
    return builder_.getNone();
}

Value* IRGenerator::generateSomeLiteral(const volta::ast::SomeLiteral& lit) {
    // TODO: Generate IR for some literal (optional value)
    //
    // This requires option type support in IR.
    // For now, just generate the inner value.

    // YOUR CODE HERE
    return nullptr;
}

Value* IRGenerator::generateArrayLiteral(const volta::ast::ArrayLiteral& lit) {
    // TODO: Generate IR for array literal: [1, 2, 3]
    //
    // Steps:
    // 1. Determine array element type
    // 2. Create array allocation
    // 3. For each element:
    //    a. Generate IR for element expression
    //    b. Store into array at index i
    // 4. Return the array

    // YOUR CODE HERE
    return nullptr;
}

Value* IRGenerator::generateTupleLiteral(const volta::ast::TupleLiteral& lit) {
    // TODO: Generate IR for tuple literal: (1, "hello", true)
    //
    // Similar to array, but with heterogeneous types.
    // Can be represented as a struct with numbered fields.

    // YOUR CODE HERE
    return nullptr;
}

Value* IRGenerator::generateStructLiteral(const volta::ast::StructLiteral& lit) {
    // Get the struct type from semantic analysis
    auto structType = std::dynamic_pointer_cast<semantic::StructType>(
        analyzer_->getExpressionType(&lit)
    );

    if (!structType) {
        error("Expected struct type for struct literal", lit.location);
        return nullptr;
    }

    // Allocate memory for the struct
    Value* structAlloc = builder_.createAlloc(structType);

    // For each field in the literal, set the corresponding field in the struct
    for (const auto& fieldInit : lit.fields) {
        const std::string& fieldName = fieldInit->identifier->name;

        // Look up the field index in the struct type
        size_t fieldIndex = 0;
        bool found = false;
        const auto& structFields = structType->fields();
        for (size_t i = 0; i < structFields.size(); ++i) {
            if (structFields[i].name == fieldName) {
                fieldIndex = i;
                found = true;
                break;
            }
        }

        if (!found) {
            error("Unknown field '" + fieldName + "' in struct " + structType->name(),
                  lit.location);
            continue;
        }

        // Generate IR for the field value
        Value* fieldValue = generateExpression(fieldInit->value.get());
        if (!fieldValue) {
            continue;
        }

        // Set the field in the struct
        builder_.createSetField(structAlloc, fieldIndex, fieldValue);
    }

    return structAlloc;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::shared_ptr<semantic::Type> IRGenerator::resolveType(const volta::ast::Type* astType) {
    return analyzer_->resolveTypeAnnotation(astType);
}

void IRGenerator::declareVariable(const std::string& name, Value* value) {
    // TODO: Handle nested scopes
    variableMap_[name] = value;
}

Value* IRGenerator::lookupVariable(const std::string& name) {
    auto it = variableMap_.find(name);
    if (it != variableMap_.end()) {
        return it->second;
    }
    return nullptr;
}

void IRGenerator::error(const std::string& message, const volta::errors::SourceLocation& loc) {
    reporter_.reportError(std::make_unique<errors::CompilerError>(
        errors::CompilerError::Level::Error,
        message,
        loc
    ));
}

void IRGenerator::registerBuiltinFunctions() {
    auto voidType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void);
    auto intType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Int);
    auto boolType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Bool);
    auto strType = std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::String);

    // Register print function: print(string) -> void
    {
        std::vector<std::shared_ptr<semantic::Type>> params = {strType};
        auto funcType = std::make_shared<semantic::FunctionType>(params, voidType);
        auto func = builder_.createFunction("print", funcType);
        func->setForeign(true);
        module_->addFunction(std::move(func));
        functionMap_["print"] = module_->functions().back().get();
    }

    // Register println function: println(string) -> void
    {
        std::vector<std::shared_ptr<semantic::Type>> params = {strType};
        auto funcType = std::make_shared<semantic::FunctionType>(params, voidType);
        auto func = builder_.createFunction("println", funcType);
        func->setForeign(true);
        module_->addFunction(std::move(func));
        functionMap_["println"] = module_->functions().back().get();
    }

    // Register len function: len(string) -> int
    {
        std::vector<std::shared_ptr<semantic::Type>> params = {strType};
        auto funcType = std::make_shared<semantic::FunctionType>(params, intType);
        auto func = builder_.createFunction("len", funcType);
        func->setForeign(true);
        module_->addFunction(std::move(func));
        functionMap_["len"] = module_->functions().back().get();
    }

    // Register assert function: assert(bool) -> void
    {
        std::vector<std::shared_ptr<semantic::Type>> params = {boolType};
        auto funcType = std::make_shared<semantic::FunctionType>(params, voidType);
        auto func = builder_.createFunction("assert", funcType);
        func->setForeign(true);
        module_->addFunction(std::move(func));
        functionMap_["assert"] = module_->functions().back().get();
    }

    // Register type_of function: type_of(string) -> string
    {
        std::vector<std::shared_ptr<semantic::Type>> params = {strType};
        auto funcType = std::make_shared<semantic::FunctionType>(params, strType);
        auto func = builder_.createFunction("type_of", funcType);
        func->setForeign(true);
        module_->addFunction(std::move(func));
        functionMap_["type_of"] = module_->functions().back().get();
    }

    // Register to_string function: to_string(string) -> string
    {
        std::vector<std::shared_ptr<semantic::Type>> params = {strType};
        auto funcType = std::make_shared<semantic::FunctionType>(params, strType);
        auto func = builder_.createFunction("to_string", funcType);
        func->setForeign(true);
        module_->addFunction(std::move(func));
        functionMap_["to_string"] = module_->functions().back().get();
    }
}

} // namespace volta::ir
