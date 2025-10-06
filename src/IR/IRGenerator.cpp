#include "IR/IRGenerator.hpp"
#include "IR/IRType.hpp"
#include "semantic/SemanticAnalyzer.hpp"

namespace volta::ir {

// ============================================================================
// Constructor
// ============================================================================

IRGenerator::IRGenerator(Module& module)
    : module_(module),
      builder_(module),
      breakTarget_(nullptr),
      continueTarget_(nullptr),
      hasErrors_(false),
      currentFunction_(nullptr),
      analyzer_(nullptr) {
    symbolTable_.push_back({});
}

IRGenerator::IRGenerator(Module& module, const volta::semantic::SemanticAnalyzer* analyzer)
    : module_(module),
      builder_(module),
      breakTarget_(nullptr),
      continueTarget_(nullptr),
      hasErrors_(false),
      currentFunction_(nullptr),
      analyzer_(analyzer) {
    symbolTable_.push_back({});
}

// ============================================================================
// Top-Level Generation
// ============================================================================

bool IRGenerator::generate(const ast::Program* program) {
    if (!program) {
        reportError("Null program");
        return false;
    }

    // Separate top-level statements into declarations and executable statements
    std::vector<const ast::Statement*> executableStmts;

    for (const auto& stmt : program->statements) {
        // Check if this is a declaration (function or global variable)
        if (dynamic_cast<const ast::FnDeclaration*>(stmt.get()) ||
            dynamic_cast<const ast::VarDeclaration*>(stmt.get())) {
            // Generate declarations directly at module level
            generateStatement(stmt.get());
        } else {
            // Collect executable statements to wrap in __main
            executableStmts.push_back(stmt.get());
        }
    }

    // If there are executable top-level statements, wrap them in a __main function
    if (!executableStmts.empty()) {
        // Create __main function: fn __main() -> void
        auto* mainFunc = module_.createFunction(
            "__main",
            module_.getVoidType(),
            {} // No parameters
        );

        // Set up function context
        currentFunction_ = mainFunc;
        enterScope();

        // Create entry block
        auto* entryBlock = module_.createBasicBlock("entry", mainFunc);
        builder_.setInsertionPoint(entryBlock);

        // Generate all executable statements
        for (const auto* stmt : executableStmts) {
            generateStatement(stmt);
        }

        // Add return void if block is not already terminated
        if (!isBlockTerminated()) {
            builder_.createRet(nullptr);
        }

        // Clean up function context
        exitScope();
        currentFunction_ = nullptr;
    }

    return !hasErrors_;
}

// ============================================================================
// Statement Generation
// ============================================================================

void IRGenerator::generateStatement(const ast::Statement* stmt) {
    if (!stmt) return;

    // Dispatch to specific handler based on statement type
    if (auto* fnDecl = dynamic_cast<const ast::FnDeclaration*>(stmt)) {
        generateFunctionDecl(fnDecl);
    } else if (auto* varDecl = dynamic_cast<const ast::VarDeclaration*>(stmt)) {
        generateVarDecl(varDecl);
    } else if (auto* ifStmt = dynamic_cast<const ast::IfStatement*>(stmt)) {
        generateIfStmt(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const ast::WhileStatement*>(stmt)) {
        generateWhileStmt(whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ast::ForStatement*>(stmt)) {
        generateForStmt(forStmt);
    } else if (auto* retStmt = dynamic_cast<const ast::ReturnStatement*>(stmt)) {
        generateReturnStmt(retStmt);
    } else if (auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(stmt)) {
        generateExprStmt(exprStmt);
    } else if (auto* block = dynamic_cast<const ast::Block*>(stmt)) {
        generateBlock(block);
    } else {
        reportError("Unknown statement type");
    }
}

void IRGenerator::generateFunctionDecl(const ast::FnDeclaration* decl) {
    // 1. Lower return type from semantic analysis or AST
    if (!analyzer_) {
        reportError("Function '" + decl->identifier + "': No semantic analyzer available");
        return;
    }

    if (!decl->returnType) {
        reportError("Function '" + decl->identifier + "': Missing return type annotation");
        return;
    }

    auto semReturnType = analyzer_->resolveTypeAnnotation(decl->returnType.get());
    if (!semReturnType) {
        reportError("Function '" + decl->identifier + "': Failed to resolve return type");
        return;
    }

    std::shared_ptr<IRType> returnType = lowerType(semReturnType);
    if (!returnType) {
        reportError("Function '" + decl->identifier + "': Failed to lower return type to IR");
        return;
    }

    // 2. Lower parameter types from semantic analysis
    std::vector<std::shared_ptr<IRType>> paramTypes;
    for (const auto& param : decl->parameters) {
        if (!param->type) {
            reportError("Parameter '" + param->identifier + "' missing type annotation");
            return;
        }

        auto semType = analyzer_->resolveTypeAnnotation(param->type.get());
        if (!semType) {
            reportError("Failed to resolve type for parameter '" + param->identifier + "'");
            return;
        }

        auto irType = lowerType(semType);
        if (!irType) {
            reportError("Failed to lower type for parameter '" + param->identifier + "'");
            return;
        }

        paramTypes.push_back(irType);
    }

    // 3. Create function
    Function* func = builder_.createFunction(decl->identifier, returnType, paramTypes);
    currentFunction_ = func;

    // 4. Enter function scope
    enterScope();

    // 5. Create allocas for parameters and add to symbol table
    for (size_t i = 0; i < decl->parameters.size(); i++) {
        const auto& param = decl->parameters[i];
        Argument* arg = func->getParam(i);

        // Create alloca for parameter
        Value* alloca = builder_.createAlloca(arg->getType(), param->identifier);

        // Store parameter value to alloca
        builder_.createStore(arg, alloca);

        // Add to symbol table
        declareVariable(param->identifier, alloca);
    }

    // 6. Generate function body
    if (decl->body) {
        generateBlock(decl->body.get());
    }

    // 7. Add implicit return if missing
    // Check if we need to add an implicit return
    // For now, only add for void functions; type checker ensures non-void functions return
    if (returnType->kind() == IRType::Kind::Void) {
        // Check if entry block has terminator
        bool hasReturn = func->getEntryBlock() && func->getEntryBlock()->hasTerminator();
        if (!hasReturn && func->getEntryBlock()) {
            builder_.setInsertionPoint(func->getEntryBlock());
            builder_.createRet();
        }
    }
    // Note: Non-void functions are validated by semantic analyzer

    // 8. Exit function scope
    exitScope();
    currentFunction_ = nullptr;
}

void IRGenerator::generateVarDecl(const ast::VarDeclaration* decl) {
    // 1. Get type from semantic analysis or initializer
    std::shared_ptr<IRType> type;
    if (analyzer_ && decl->typeAnnotation && decl->typeAnnotation->type) {
        auto semType = analyzer_->resolveTypeAnnotation(decl->typeAnnotation->type.get());
        type = lowerType(semType);
    } else if (analyzer_ && decl->initializer) {
        auto semType = analyzer_->getExpressionType(decl->initializer.get());
        type = lowerType(semType);
    } else {
        type = builder_.getIntType();  // Fallback
    }

    // 2. Create alloca for variable
    Value* alloca = builder_.createAlloca(type, decl->identifier);

    // 3. Generate initializer (if present)
    if (decl->initializer) {
        Value* initValue = generateExpression(decl->initializer.get());
        if (initValue) {
            // 4. Store initializer to alloca
            builder_.createStore(initValue, alloca);
        }
    }

    // 5. Add to symbol table
    declareVariable(decl->identifier, alloca);
}

void IRGenerator::generateIfStmt(const ast::IfStatement* stmt) {
    // 1. Generate main if condition
    if (!stmt->condition) {
        reportError("If statement: condition is null in AST");
        return;
    }

    Value* cond = generateExpression(stmt->condition.get());
    if (!cond) {
        reportError("If statement: failed to generate condition");
        cond = builder_.getBool(false);  // Dummy value to continue
    }

    // 2. Create blocks for main if-then-else
    auto blocks = builder_.createIfThenElse(cond);

    // 3. Generate then block
    builder_.setInsertionPoint(blocks.thenBlock);
    if (stmt->thenBlock) {
        generateBlock(stmt->thenBlock.get());
    }
    if (!isBlockTerminated()) {
        builder_.createBr(blocks.mergeBlock);
    }

    // 4. Generate else-if chain SEQUENTIALLY
    builder_.setInsertionPoint(blocks.elseBlock);

    BasicBlock* currentElseBlock = blocks.elseBlock;
    BasicBlock* finalMerge = blocks.mergeBlock;

    for (size_t i = 0; i < stmt->elseIfClauses.size(); i++) {
        const auto& elseIfClause = stmt->elseIfClauses[i];

        // Generate condition in current else block
        builder_.setInsertionPoint(currentElseBlock);
        Value* elseIfCond = generateExpression(elseIfClause.first.get());
        if (!elseIfCond) {
            reportError("Else-if statement: failed to generate condition");
            elseIfCond = builder_.getBool(false);
        }

        // Create then and next else blocks
        BasicBlock* elseIfThen = builder_.createBasicBlock("elseif.then");
        BasicBlock* nextElse = builder_.createBasicBlock("elseif.else");

        // Create conditional branch IMMEDIATELY
        builder_.createCondBr(elseIfCond, elseIfThen, nextElse);

        // Generate then block
        builder_.setInsertionPoint(elseIfThen);
        generateBlock(elseIfClause.second.get());
        if (!isBlockTerminated()) {
            builder_.createBr(finalMerge);
        }

        // Move to next else block
        currentElseBlock = nextElse;
    }

    // 5. Generate final else (or just branch to merge)
    builder_.setInsertionPoint(currentElseBlock);
    if (stmt->elseBlock) {
        generateBlock(stmt->elseBlock.get());
    }
    if (!isBlockTerminated()) {
        builder_.createBr(finalMerge);
    }

    // 6. Continue in merge block
    builder_.setInsertionPoint(finalMerge);

    // Handle unreachable merge block
    // NOTE: If all branches return, the merge block is unreachable (0 predecessors).
    // We still need to give it a valid terminator for IR correctness.
    // The bytecode compiler will skip emitting code for unreachable blocks.
    if (finalMerge->getNumPredecessors() == 0) {
        if (currentFunction_ && currentFunction_->getReturnType()->kind() == IRType::Kind::Void) {
            builder_.createRet();
        } else if (currentFunction_) {
            Value* undefVal = builder_.getUndef(currentFunction_->getReturnType());
            builder_.createRet(undefVal);
        }
    }
}

void IRGenerator::generateWhileStmt(const ast::WhileStatement* stmt) {
    // 1. Create loop structure
    auto blocks = builder_.createLoop();

    // 2. Branch to header
    builder_.createBr(blocks.headerBlock);

    // 3. Set loop targets for break/continue
    setLoopTargets(blocks.exitBlock, blocks.headerBlock);

    // 4. Generate condition in header
    builder_.setInsertionPoint(blocks.headerBlock);
    Value* cond = generateExpression(stmt->condition.get());
    if (cond) {
        builder_.createCondBr(cond, blocks.bodyBlock, blocks.exitBlock);
    } else {
        // If condition generation failed, still add a terminator to avoid invalid IR
        reportError("While statement: failed to generate condition");
        builder_.createBr(blocks.exitBlock);  // Just exit the loop
    }

    // 5. Generate body
    builder_.setInsertionPoint(blocks.bodyBlock);
    if (stmt->thenBlock) {
        generateBlock(stmt->thenBlock.get());
    }
    // Loop back to header
    if (!isBlockTerminated()) {
        builder_.createBr(blocks.headerBlock);
    }

    // 6. Clear loop targets
    clearLoopTargets();

    // 7. Continue after loop
    builder_.setInsertionPoint(blocks.exitBlock);
}

void IRGenerator::generateForStmt(const ast::ForStatement* stmt) {
    (void)stmt;  // TODO: Implement
    reportError("For loops not yet implemented in IR generation");
    // Desugar to while loop:
    // let iterator = expr.iter()
    // while iterator.hasNext():
    //   let identifier = iterator.next()
    //   ... body ...
}

void IRGenerator::generateReturnStmt(const ast::ReturnStatement* stmt) {
    if (stmt->expression) {
        // Return with value
        Value* retVal = generateExpression(stmt->expression.get());
        if (retVal) {
            builder_.createRet(retVal);
        }
    } else {
        // Void return
        builder_.createRet();
    }
}

void IRGenerator::generateExprStmt(const ast::ExpressionStatement* stmt) {
    // Just generate the expression, discard the result
    // (Side effects like function calls will still happen)
    if (stmt->expr) {
        generateExpression(stmt->expr.get());
    }
}

void IRGenerator::generateBlock(const ast::Block* block) {
    if (!block) return;

    // Enter new scope for the block
    enterScope();

    // Generate each statement in the block
    for (const auto& stmt : block->statements) {
        // Skip remaining statements if block is already terminated
        if (isBlockTerminated()) {
            break;
        }
        generateStatement(stmt.get());
    }

    // Exit scope
    exitScope();
}

// ============================================================================
// Expression Generation
// ============================================================================

Value* IRGenerator::generateExpression(const ast::Expression* expr) {
    if (!expr) {
        reportError("generateExpression: null expression");
        return nullptr;
    }

    if (auto* intLit = dynamic_cast<const ast::IntegerLiteral*>(expr)) {
        return generateIntLiteral(intLit);
    }
    if (auto* floatLit = dynamic_cast<const ast::FloatLiteral*>(expr)) {
        return generateFloatLiteral(floatLit);
    }
    if (auto* boolLit = dynamic_cast<const ast::BooleanLiteral*>(expr)) {
        return generateBoolLiteral(boolLit);
    }
    if (auto* strLit = dynamic_cast<const ast::StringLiteral*>(expr)) {
        return generateStringLiteral(strLit);
    }
    if (auto* noneLit = dynamic_cast<const ast::NoneLiteral*>(expr)) {
        return generateNoneLiteral(noneLit);
    }
    if (auto* ident = dynamic_cast<const ast::IdentifierExpression*>(expr)) {
        return generateIdentifier(ident);
    }
    if (auto* binExpr = dynamic_cast<const ast::BinaryExpression*>(expr)) {
        return generateBinaryExpr(binExpr);
    }
    if (auto* unExpr = dynamic_cast<const ast::UnaryExpression*>(expr)) {
        return generateUnaryExpr(unExpr);
    }
    if (auto* callExpr = dynamic_cast<const ast::CallExpression*>(expr)) {
        return generateCallExpr(callExpr);
    }
    if (auto* indexExpr = dynamic_cast<const ast::IndexExpression*>(expr)) {
        return generateIndexExpr(indexExpr);
    }
    if (auto* arrLit = dynamic_cast<const ast::ArrayLiteral*>(expr)) {
        return generateArrayLiteral(arrLit);
    }
    if (auto* castExpr = dynamic_cast<const ast::CastExpression*>(expr)) {
       return generateCastExpr(castExpr);
    }

    // Try to get type info for debugging
    std::string typeName = "unknown";
    if (expr) {
        typeName = std::string(typeid(*expr).name());
    }
    reportError("Unknown/unimplemented expression type: " + typeName);
    return nullptr;
}

Value* IRGenerator::generateIntLiteral(const ast::IntegerLiteral* lit) {
    return builder_.getInt(lit->value);
}

Value* IRGenerator::generateFloatLiteral(const ast::FloatLiteral* lit) {
    return builder_.getFloat(lit->value);
}

Value* IRGenerator::generateBoolLiteral(const ast::BooleanLiteral* lit) {
    return builder_.getBool(lit->value);
}

Value* IRGenerator::generateStringLiteral(const ast::StringLiteral* lit) {
    return builder_.getString(lit->value);
}

Value* IRGenerator::generateNoneLiteral(const ast::NoneLiteral* lit) {
    // Need to know the Option type - this might come from type annotations
    // For now, return None with a placeholder type
    auto optType = builder_.getOptionType(builder_.getIntType());  // Placeholder
    return builder_.getNone(optType);
}

Value* IRGenerator::generateSomeLiteral(const ast::SomeLiteral* lit) {
    (void)lit;  // TODO: Implement
    reportError("Some literals not yet implemented in IR generation");
    // 1. Generate inner value
    // 2. Wrap in Option using OptionWrap
    return nullptr;
}

Value* IRGenerator::generateArrayLiteral(const ast::ArrayLiteral* lit) {
    (void)lit;  // TODO: Implement
    reportError("Array literals not yet implemented in IR generation");
    // 1. Create array with ArrayNew
    // 2. Store each element using ArraySet
    // 3. Return array pointer
    return nullptr;
}

Value* IRGenerator::generateIdentifier(const ast::IdentifierExpression* expr) {
    Value* allocaPtr = lookupVariable(expr->name);

    if (!allocaPtr) {
        std::string scopeInfo = "symbol table has " + std::to_string(symbolTable_.size()) + " scopes";
        reportError("Undefined variable '" + expr->name + "' (" + scopeInfo + ")");
        return nullptr;
    }

    // Load value from alloca
    // IMPORTANT: Variables are stored as allocas, so we must load!
    return builder_.createLoad(allocaPtr, expr->name);
}

Value* IRGenerator::generateBinaryExpr(const ast::BinaryExpression* expr) {
    using Op = ast::BinaryExpression::Operator;

    // Handle ALL assignment operators
    if (expr->op == Op::Assign ||
        expr->op == Op::AddAssign ||
        expr->op == Op::SubtractAssign ||
        expr->op == Op::MultiplyAssign ||
        expr->op == Op::DivideAssign) {
        return generateAssignment(expr);
    }

    // Generate left and right operands
    Value* left = generateExpression(expr->left.get());
    if (!left) {
        reportError("Failed to generate left operand of binary expression");
        return nullptr;
    }

    Value* right = generateExpression(expr->right.get());
    if (!right) {
        reportError("Failed to generate right operand of binary expression");
        return nullptr;
    }

    // Map operator to IR opcode
    Instruction::Opcode opcode = mapBinaryOp(expr->op);

    // Create appropriate instruction based on opcode (not operator enum value)
    switch (opcode) {
        // Arithmetic
        case Instruction::Opcode::Add: return builder_.createAdd(left, right);
        case Instruction::Opcode::Sub: return builder_.createSub(left, right);
        case Instruction::Opcode::Mul: return builder_.createMul(left, right);
        case Instruction::Opcode::Div: return builder_.createDiv(left, right);
        case Instruction::Opcode::Rem: return builder_.createRem(left, right);
        case Instruction::Opcode::Pow: return builder_.createPow(left, right);

        // Comparison
        case Instruction::Opcode::Eq: return builder_.createEq(left, right);
        case Instruction::Opcode::Ne: return builder_.createNe(left, right);
        case Instruction::Opcode::Lt: return builder_.createLt(left, right);
        case Instruction::Opcode::Le: return builder_.createLe(left, right);
        case Instruction::Opcode::Gt: return builder_.createGt(left, right);
        case Instruction::Opcode::Ge: return builder_.createGe(left, right);

        // Logical
        case Instruction::Opcode::And: return builder_.createAnd(left, right);
        case Instruction::Opcode::Or: return builder_.createOr(left, right);

        default: break;
    }

    reportError("Unknown binary operator");
    return nullptr;
}

Value* IRGenerator::generateUnaryExpr(const ast::UnaryExpression* expr) {
    Value* operand = generateExpression(expr->operand.get());
    if (!operand) {
        reportError("Failed to generate unary expression operand");
        return nullptr;
    }

    // Map operator
    Instruction::Opcode opcode = mapUnaryOp(expr->op);

    // Create instruction
    if (opcode == Instruction::Opcode::Neg) {
        return builder_.createNeg(operand);
    }
    if (opcode == Instruction::Opcode::Not) {
        return builder_.createNot(operand);
    }

    reportError("Unknown unary operator");
    return nullptr;
}

Value* IRGenerator::generateCallExpr(const ast::CallExpression* expr) {
    // 1. Get function name (callee should be an IdentifierExpression for now)
    auto* identExpr = dynamic_cast<const ast::IdentifierExpression*>(expr->callee.get());
    if (!identExpr) {
        reportError("Call expression: callee must be an identifier");
        return nullptr;
    }

    // 2. Look up function in module
    Function* func = module_.getFunction(identExpr->name);
    if (!func) {
        reportError("Call expression: undefined function '" + identExpr->name + "'");
        return nullptr;
    }

    // 3. Generate arguments
    std::vector<Value*> args;
    for (const auto& argExpr : expr->arguments) {
        Value* arg = generateExpression(argExpr.get());
        if (!arg) {
            reportError("Call expression: failed to generate argument");
            return nullptr;
        }
        args.push_back(arg);
    }

    // 4. Create call instruction
    return builder_.createCall(func, args);
}

Value* IRGenerator::generateIndexExpr(const ast::IndexExpression* expr) {
    (void)expr;  // TODO: Implement
    reportError("Index expressions not yet implemented in IR generation");
    // 1. Generate array expression
    // 2. Generate index expression
    // 3. Create ArrayGet instruction
    return nullptr;
}

Value* IRGenerator::generateMemberExpr(const ast::MemberExpression* expr) {
    (void)expr;  // TODO: Implement
    reportError("Member expressions not yet implemented in IR generation");
    // For structs, use ExtractValue or GEP
    return nullptr;
}

Value* IRGenerator::generateCastExpr(const ast::CastExpression* expr) {
    if (!analyzer_) {
        reportError("Cast expression: semantic analyzer not available");
        return nullptr;
    }

    // 1. Generate the value being cast
    Value* sourceValue = generateExpression(expr->expression.get());
    if (!sourceValue) {
        reportError("Cast expression: failed to generate source value");
        return nullptr;
    }

    // 2. Get the target type from AST and convert to IR type
    auto semType = analyzer_->resolveTypeAnnotation(expr->targetType.get());
    if (!semType) {
        reportError("Cast expression: failed to resolve target type");
        return nullptr;
    }

    auto targetType = lowerType(semType);
    if (!targetType) {
        reportError("Cast expression: failed to lower target type");
        return nullptr;
    }

    // 3. Create the Cast instruction
    return builder_.createCast(sourceValue, targetType);
}

Value* IRGenerator::generateAssignment(const ast::BinaryExpression* expr) {
    if (auto* ident = dynamic_cast<const ast::IdentifierExpression*>(expr->left.get())) {
        // Look up variable
        Value* allocaPtr = lookupVariable(ident->name);
        if (!allocaPtr) {
            reportError("Undefined variable: " + ident->name);
            return nullptr;
        }

        Value* rhs;

        if (expr->op == ast::BinaryExpression::Operator::Assign) {
            // Simple assignment: x = rhs
            rhs = generateExpression(expr->right.get());
        } else {
            // Compound assignment: x += rhs -> x = x + rhs
            Value* oldValue = builder_.createLoad(allocaPtr, ident->name);
            Value* rhsValue = generateExpression(expr->right.get());
            if (!rhsValue) return nullptr;

            // Map compound assign to binary op
            using Op = ast::BinaryExpression::Operator;
            switch (expr->op) {
                case Op::AddAssign:
                    rhs = builder_.createAdd(oldValue, rhsValue);
                    break;
                case Op::SubtractAssign:
                    rhs = builder_.createSub(oldValue, rhsValue);
                    break;
                case Op::MultiplyAssign:
                    rhs = builder_.createMul(oldValue, rhsValue);
                    break;
                case Op::DivideAssign:
                    rhs = builder_.createDiv(oldValue, rhsValue);
                    break;
                default:
                    reportError("Unknown compound assignment operator");
                    return nullptr;
            }
        }

        if (!rhs) return nullptr;

        // Store to alloca
        builder_.createStore(rhs, allocaPtr);
        return rhs;  // Assignment returns the assigned value
    }

    // Case 2: Array element assignment (arr[i] = value)
    if (auto* indexExpr = dynamic_cast<const ast::IndexExpression*>(expr->left.get())) {
        Value* array = generateExpression(indexExpr->object.get());
        Value* index = generateExpression(indexExpr->index.get());
        Value* value = generateExpression(expr->right.get());

        if (!array || !index || !value) return nullptr;

        builder_.createArraySet(array, index, value);
        return value;
    }

    // Case 3: Field assignment (obj.field = value)
    // TODO: Handle struct field assignment

    reportError("Invalid assignment target");
    return nullptr;
}

// ============================================================================
// Helper Methods
// ============================================================================

Value* IRGenerator::lookupVariable(const std::string& name) {
    for (auto it = symbolTable_.rbegin(); it != symbolTable_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr; 
}

void IRGenerator::declareVariable(const std::string& name, Value* value) {
    if (!symbolTable_.empty()) symbolTable_.back()[name] = value;
}

void IRGenerator::enterScope() {
    symbolTable_.push_back({});
}

void IRGenerator::exitScope() {
    if (!symbolTable_.empty()) symbolTable_.pop_back();
}

std::shared_ptr<IRType> IRGenerator::lowerType(std::shared_ptr<semantic::Type> semType) {
    return TypeLowering::lower(semType);
}

Function* IRGenerator::getCurrentFunction() const {
    return currentFunction_;
}

Instruction::Opcode IRGenerator::mapBinaryOp(ast::BinaryExpression::Operator op) {
    switch (op) {
        case ast::BinaryExpression::Operator::Add: return Instruction::Opcode::Add;
        case ast::BinaryExpression::Operator::Subtract: return Instruction::Opcode::Sub;
        case ast::BinaryExpression::Operator::Multiply: return Instruction::Opcode::Mul;
        case ast::BinaryExpression::Operator::Divide: return Instruction::Opcode::Div;
        case ast::BinaryExpression::Operator::Modulo: return Instruction::Opcode::Rem;
        case ast::BinaryExpression::Operator::Power: return Instruction::Opcode::Pow;

        case ast::BinaryExpression::Operator::Equal: return Instruction::Opcode::Eq;
        case ast::BinaryExpression::Operator::NotEqual: return Instruction::Opcode::Ne;
        case ast::BinaryExpression::Operator::Less: return Instruction::Opcode::Lt;
        case ast::BinaryExpression::Operator::LessEqual: return Instruction::Opcode::Le;
        case ast::BinaryExpression::Operator::Greater: return Instruction::Opcode::Gt;
        case ast::BinaryExpression::Operator::GreaterEqual: return Instruction::Opcode::Ge;

        case ast::BinaryExpression::Operator::LogicalAnd: return Instruction::Opcode::And;
        case ast::BinaryExpression::Operator::LogicalOr: return Instruction::Opcode::Or;

        default:
            reportError("Unsupported binary operator");
            return Instruction::Opcode::Add;  // Fallback
    }
}

Instruction::Opcode IRGenerator::mapUnaryOp(ast::UnaryExpression::Operator op) {
    switch (op) {
        case ast::UnaryExpression::Operator::Negate: return Instruction::Opcode::Neg;
        case ast::UnaryExpression::Operator::Not: return Instruction::Opcode::Not;
        default:
            reportError("Unsupported unary operator");
            return Instruction::Opcode::Neg;  // Fallback
    }
}

void IRGenerator::reportError(const std::string& message) {
    hasErrors_ = true;
    errors_.push_back(message);
}

bool IRGenerator::isBlockTerminated() const {
    BasicBlock* block = builder_.getInsertionBlock();
    if (!block) return false;

    return block->hasTerminator();
}

BasicBlock* IRGenerator::getBreakTarget() const {
    return breakTarget_;
}

BasicBlock* IRGenerator::getContinueTarget() const {
    return continueTarget_;
}

void IRGenerator::setLoopTargets(BasicBlock* breakTarget, BasicBlock* continueTarget) {
    breakTarget_ = breakTarget;
    continueTarget_ = continueTarget;
}

void IRGenerator::clearLoopTargets() {
    breakTarget_ = nullptr;
    continueTarget_ = nullptr;
}

// ============================================================================
// Standalone API
// ============================================================================

std::unique_ptr<Module> generateIR(
    const volta::ast::Program& program,
    const volta::semantic::SemanticAnalyzer& analyzer,
    const std::string& moduleName
) {
    auto module = std::make_unique<Module>(moduleName);

    // Register builtin/native functions
    // print(int) -> void
    {
        std::vector<std::shared_ptr<IRType>> params = { module->getIntType() };
        auto returnType = module->getVoidType();
        module->createFunction("print", returnType, params);
    }

    IRGenerator generator(*module, &analyzer);

    if (!generator.generate(&program)) {
        // Generation failed - print errors
        auto errors = generator.getErrors();
        std::cerr << "IR generation failed with " << errors.size() << " errors:\n";
        for (const auto& error : errors) {
            std::cerr << "  IR Gen Error: " << error << "\n";
        }
        return nullptr;
    }

    return module;
}

} // namespace volta::ir
