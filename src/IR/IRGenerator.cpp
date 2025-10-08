#include "IR/IRGenerator.hpp"
#include "IR/IRType.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include <functional>

namespace volta::ir {

// ============================================================================
// Helper Functions
// ============================================================================

std::string IRGenerator::mangleFunctionName(const std::string& name,
                                           const std::vector<std::shared_ptr<IRType>>& paramTypes) {
    // Simple name mangling: name + "_" + type suffixes
    // Example: print(i64) -> print_i64, print(str) -> print_str
    if (paramTypes.empty()) {
        return name;  // No mangling for no parameters
    }

    std::string mangled = name;
    for (const auto& type : paramTypes) {
        mangled += "_";
        switch (type->kind()) {
            case IRType::Kind::I64:
                mangled += "i64";
                break;
            case IRType::Kind::F64:
                mangled += "f64";
                break;
            case IRType::Kind::I1:
                mangled += "bool";
                break;
            case IRType::Kind::String:
                mangled += "str";
                break;
            case IRType::Kind::Void:
                mangled += "void";
                break;
            case IRType::Kind::Array:
                mangled += "array";
                break;
            default:
                mangled += "unknown";
                break;
        }
    }
    return mangled;
}

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
        // Check if this is a function declaration (only functions at module level)
        if (dynamic_cast<const ast::FnDeclaration*>(stmt.get())) {
            // Generate function declarations directly at module level
            generateStatement(stmt.get());
        } else {
            // All other statements (including variable declarations) go into __main
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
            if (!stmt) {
                reportError("Null statement in executable statements");
                continue;
            }
            generateStatement(stmt);
            if (hasErrors_) {
                // Stop generating if we hit errors
                break;
            }
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
    } else if (auto* breakStmt = dynamic_cast<const ast::BreakStatement*>(stmt)) {
        generateBreakStmt(breakStmt);
    } else if (auto* continueStmt = dynamic_cast<const ast::ContinueStatement*>(stmt)) {
        generateContinueStmt(continueStmt);
    } else if (auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(stmt)) {
        generateExprStmt(exprStmt);
    } else if (auto* block = dynamic_cast<const ast::Block*>(stmt)) {
        generateBlock(block);
    } else if (dynamic_cast<const ast::StructDeclaration*>(stmt)) {
        // Struct declarations are type definitions - no IR generation needed
        // They're handled during type lowering
    } else if (dynamic_cast<const ast::EnumDeclaration*>(stmt)) {
        // Enum declarations are type definitions - no IR generation needed
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

    // Check if this is a method on a generic struct/enum
    // If so, cache the AST and skip IR generation for now
    // We'll generate monomorphized versions at call sites instead
    if (decl->isMethod && !decl->receiverType.empty()) {
        auto* symTable = const_cast<volta::semantic::SymbolTable*>(analyzer_->getSymbolTable());
        auto symbol = symTable->lookup(decl->receiverType);
        if (symbol) {
            if (auto* structType = dynamic_cast<const volta::semantic::StructType*>(symbol->type.get())) {
                if (!structType->typeParams().empty()) {
                    // This is a method on a generic struct - cache AST and skip IR gen
                    std::string methodName = decl->receiverType + "." + decl->identifier;
                    genericMethodCache_[methodName] = decl;
                    return;
                }
            } else if (auto* enumType = dynamic_cast<const volta::semantic::EnumType*>(symbol->type.get())) {
                if (!enumType->typeParams().empty()) {
                    // This is a method on a generic enum - cache AST and skip IR gen
                    std::string methodName = decl->receiverType + "." + decl->identifier;
                    genericMethodCache_[methodName] = decl;
                    return;
                }
            }
        }
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

    // 3. Create function with qualified name for methods
    std::string functionName = decl->identifier;
    if (decl->isMethod) {
        functionName = decl->receiverType + "." + decl->identifier;
    }
    Function* func = builder_.createFunction(functionName, returnType, paramTypes);
    currentFunction_ = func;

    // 4. Enter function scope
    enterScope();

    // 5. Create allocas for parameters and add to symbol table
    for (size_t i = 0; i < decl->parameters.size(); i++) {
        const auto& param = decl->parameters[i];
        Argument* arg = func->getParam(i);

        if (param->isMutable) {
            // For mutable parameters, create an alloca and store the argument
            Value* alloca = builder_.createAlloca(arg->getType(), param->identifier);
            builder_.createStore(arg, alloca);
            declareVariable(param->identifier, alloca);
        } else {
            // Map immutable parameter name directly to the argument value
            declareVariable(param->identifier, arg);
        }
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
        } else {
            reportError("Failed to generate initializer for variable '" + decl->identifier + "'");
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
    // Desugar for-in loop to while loop
    // Two cases:
    // 1. for x in 0..10 => range iteration
    // 2. for x in arr => array iteration

    enterScope();

    // Check if expression is a range (BinaryExpression with Range operator)
    auto* rangeExpr = dynamic_cast<const ast::BinaryExpression*>(stmt->expression.get());
    bool isRange = rangeExpr && (rangeExpr->op == ast::BinaryExpression::Operator::Range ||
                                  rangeExpr->op == ast::BinaryExpression::Operator::RangeInclusive);

    if (isRange) {
        // Handle range: for i in start..end
        Value* startVal = generateExpression(rangeExpr->left.get());
        Value* endVal = generateExpression(rangeExpr->right.get());

        if (!startVal || !endVal) {
            reportError("For loop: failed to generate range bounds");
            exitScope();
            return;
        }

        // Create loop variable: i = start
        auto indexType = builder_.getIntType();
        Value* loopVarAlloca = builder_.createAlloca(indexType, stmt->identifier);
        builder_.createStore(startVal, loopVarAlloca);
        declareVariable(stmt->identifier, loopVarAlloca);

        // Create loop blocks
        auto* headerBlock = builder_.createBasicBlock("for.header", currentFunction_);
        auto* bodyBlock = builder_.createBasicBlock("for.body", currentFunction_);
        auto* incrementBlock = builder_.createBasicBlock("for.increment", currentFunction_);
        auto* exitBlock = builder_.createBasicBlock("for.exit", currentFunction_);

        setLoopTargets(exitBlock, incrementBlock);

        // Jump to header
        builder_.createBr(headerBlock);

        // Generate header: check i < end (or i <= end for RangeInclusive)
        builder_.setInsertionPoint(headerBlock);
        Value* currentVal = builder_.createLoad(loopVarAlloca, stmt->identifier);
        Value* cond;
        if (rangeExpr->op == ast::BinaryExpression::Operator::RangeInclusive) {
            cond = builder_.createLe(currentVal, endVal);
        } else {
            cond = builder_.createLt(currentVal, endVal);
        }
        builder_.createCondBr(cond, bodyBlock, exitBlock);

        // Generate body
        builder_.setInsertionPoint(bodyBlock);
        if (stmt->thenBlock) {
            generateBlock(stmt->thenBlock.get());
        }

        // Jump to increment if not already terminated
        if (!isBlockTerminated()) {
            builder_.createBr(incrementBlock);
        }

        // Generate increment block: i = i + 1
        builder_.setInsertionPoint(incrementBlock);
        Value* current = builder_.createLoad(loopVarAlloca, "__loop_var");
        Value* one = builder_.getInt(1);
        Value* next = builder_.createAdd(current, one);
        builder_.createStore(next, loopVarAlloca);
        builder_.createBr(headerBlock);

        clearLoopTargets();
        builder_.setInsertionPoint(exitBlock);

    } else {
        // Handle array iteration: for x in arr
        Value* iterable = generateExpression(stmt->expression.get());
        if (!iterable) {
            reportError("For loop: failed to generate iterable expression");
            exitScope();
            return;
        }

        // Store array pointer so it's accessible across blocks
        auto arrayPtrType = iterable->getType();
        Value* arrayAlloca = builder_.createAlloca(arrayPtrType, "__for_arr");
        builder_.createStore(iterable, arrayAlloca);

        // Create index variable: __idx = 0
        auto indexType = builder_.getIntType();
        Value* indexAlloca = builder_.createAlloca(indexType, "__for_idx");
        Value* zeroVal = builder_.getInt(0);
        builder_.createStore(zeroVal, indexAlloca);

        // Get array length
        Value* arrayPtr = builder_.createLoad(arrayAlloca, "__for_arr_ptr");
        Value* lengthVal = builder_.createArrayLen(arrayPtr, "__for_len");

        // Create loop blocks
        auto* headerBlock = builder_.createBasicBlock("for.header", currentFunction_);
        auto* bodyBlock = builder_.createBasicBlock("for.body", currentFunction_);
        auto* incrementBlock = builder_.createBasicBlock("for.increment", currentFunction_);
        auto* exitBlock = builder_.createBasicBlock("for.exit", currentFunction_);

        setLoopTargets(exitBlock, incrementBlock);

        // Jump to header
        builder_.createBr(headerBlock);

        // Generate header: check __idx < __len
        builder_.setInsertionPoint(headerBlock);
        Value* currentIdx = builder_.createLoad(indexAlloca, "__idx_val");
        Value* cond = builder_.createLt(currentIdx, lengthVal);
        builder_.createCondBr(cond, bodyBlock, exitBlock);

        // Generate body
        builder_.setInsertionPoint(bodyBlock);

        // Get current element: x = arr[__idx]
        Value* currentIdxBody = builder_.createLoad(indexAlloca, "__idx_body");
        Value* arrayPtrBody = builder_.createLoad(arrayAlloca, "__for_arr_body");
        Value* element = builder_.createArrayGet(arrayPtrBody, currentIdxBody, stmt->identifier);

        // Create alloca for loop variable and store element
        auto elementType = element->getType();
        Value* loopVarAlloca = builder_.createAlloca(elementType, stmt->identifier);
        builder_.createStore(element, loopVarAlloca);
        declareVariable(stmt->identifier, loopVarAlloca);

        // Generate loop body
        if (stmt->thenBlock) {
            generateBlock(stmt->thenBlock.get());
        }

        // Jump to increment if not already terminated
        if (!isBlockTerminated()) {
            builder_.createBr(incrementBlock);
        }

        // Generate increment block: __idx = __idx + 1
        builder_.setInsertionPoint(incrementBlock);
        Value* idx = builder_.createLoad(indexAlloca, "__idx_inc");
        Value* one = builder_.getInt(1);
        Value* nextIdx = builder_.createAdd(idx, one);
        builder_.createStore(nextIdx, indexAlloca);
        builder_.createBr(headerBlock);

        clearLoopTargets();
        builder_.setInsertionPoint(exitBlock);
    }

    exitScope();
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

void IRGenerator::generateBreakStmt(const ast::BreakStatement* stmt) {
    (void)stmt;  // Location not used yet
    auto* target = getBreakTarget();
    if (!target) {
        reportError("Break statement outside of loop");
        return;
    }
    builder_.createBr(target);
}

void IRGenerator::generateContinueStmt(const ast::ContinueStatement* stmt) {
    (void)stmt;  // Location not used yet
    auto* target = getContinueTarget();
    if (!target) {
        reportError("Continue statement outside of loop");
        return;
    }
    builder_.createBr(target);
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
    // Note: Some and None are now enum variants, handled as constructor calls
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
    if (auto* structLit = dynamic_cast<const ast::StructLiteral*>(expr)) {
        return generateStructLiteral(structLit);
    }
    if (auto* memberExpr = dynamic_cast<const ast::MemberExpression*>(expr)) {
        return generateMemberExpr(memberExpr);
    }
    if (auto* methodCall = dynamic_cast<const ast::MethodCallExpression*>(expr)) {
        return generateMethodCallExpr(methodCall);
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

// Note: generateNoneLiteral and generateSomeLiteral removed - these are now enum variants

Value* IRGenerator::generateArrayLiteral(const ast::ArrayLiteral* lit) {
    if (lit->elements.empty()) {
        // Create empty array
        Value* sizeVal = builder_.getInt(0);
        std::shared_ptr<ir::IRType> elementType = builder_.getIntType();  // Default to int64 for empty arrays
        return builder_.createArrayNew(elementType, sizeVal);
    }

    // 1. Create array with size = number of elements
    size_t numElements = lit->elements.size();
    Value* sizeVal = builder_.getInt(static_cast<int64_t>(numElements));

    // Determine element type from first element (assume all elements have same type)
    Value* firstElem = generateExpression(lit->elements[0].get());
    if (!firstElem) {
        return nullptr;
    }
    std::shared_ptr<ir::IRType> elementType = firstElem->getType();

    // 2. Create array
    Value* array = builder_.createArrayNew(elementType, sizeVal);

    // 3. Store each element using ArraySet
    for (size_t i = 0; i < numElements; i++) {
        Value* indexVal = builder_.getInt(static_cast<int64_t>(i));
        Value* elemVal;

        if (i == 0) {
            elemVal = firstElem;  // Reuse already generated first element
        } else {
            elemVal = generateExpression(lit->elements[i].get());
            if (!elemVal) {
                return nullptr;
            }
        }

        builder_.createArraySet(array, indexVal, elemVal);
    }

    return array;
}

Value* IRGenerator::generateIdentifier(const ast::IdentifierExpression* expr) {
    Value* val = lookupVariable(expr->name);

    if (!val) {
        std::string scopeInfo = "symbol table has " + std::to_string(symbolTable_.size()) + " scopes";
        reportError("Undefined variable '" + expr->name + "' (" + scopeInfo + ")");
        return nullptr;
    }

    if (dynamic_cast<Argument*>(val)) {
        return val;  
    }

    return builder_.createLoad(val, expr->name);
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

    // 2. Generate arguments
    std::vector<Value*> args;
    std::vector<std::shared_ptr<IRType>> argTypes;
    for (const auto& argExpr : expr->arguments) {
        Value* arg = generateExpression(argExpr.get());
        if (!arg) {
            reportError("Call expression: failed to generate argument");
            return nullptr;
        }
        args.push_back(arg);
        argTypes.push_back(arg->getType());
    }

    // 3. Look up function with mangled name (for overload resolution)
    std::string mangledName = mangleFunctionName(identExpr->name, argTypes);
    Function* func = module_.getFunction(mangledName);

    // If mangled name not found, try original name (for non-overloaded functions)
    if (!func) {
        func = module_.getFunction(identExpr->name);
    }

    if (!func) {
        reportError("Call expression: undefined function '" + identExpr->name + "'");
        return nullptr;
    }

    // 4. Create call instruction
    return builder_.createCall(func, args);
}

Value* IRGenerator::generateIndexExpr(const ast::IndexExpression* expr) {
    // 1. Generate array expression
    Value* arrayVal = generateExpression(expr->object.get());
    if (!arrayVal) {
        return nullptr;
    }

    // 2. Generate index expression
    Value* indexVal = generateExpression(expr->index.get());
    if (!indexVal) {
        return nullptr;
    }

    // 3. Verify array type is pointer
    auto arrayType = arrayVal->getType();
    if (!arrayType) {
        reportError("Index expression: array value has no type");
        return nullptr;
    }

    if (!arrayType->asPointer()) {
        reportError("Index expression: array value is not a pointer type (got " +
                   arrayType->toString() + ")");
        return nullptr;
    }

    // 4. Create ArrayGet instruction
    return builder_.createArrayGet(arrayVal, indexVal);
}

Value* IRGenerator::generateMemberExpr(const ast::MemberExpression* expr) {
    // Member expressions can be:
    // 1. Enum variant without data: Option.None
    // 2. Struct field access: point.x

    // Check if this is enum variant construction (no data)
    if (auto* identExpr = dynamic_cast<const ast::IdentifierExpression*>(expr->object.get())) {
        // Look up the type in semantic analyzer's symbol table
        auto* symTable = const_cast<volta::semantic::SymbolTable*>(analyzer_->getSymbolTable());
        auto symbol = symTable->lookup(identExpr->name);
        if (symbol && symbol->type->kind() == volta::semantic::Type::Kind::Enum) {
            // It's an enum variant! Generate enum construction with no fields
            const std::string& enumName = identExpr->name;
            const std::string& variantName = expr->member->name;

            // Get the semantic type of this expression
            auto exprType = analyzer_->getExpressionType(expr);
            if (!exprType) {
                reportError("No type information for enum variant");
                return nullptr;
            }

            auto* enumType = dynamic_cast<const volta::semantic::EnumType*>(exprType.get());
            if (!enumType) {
                reportError("Expression is not an enum type");
                return nullptr;
            }

            // Find variant index
            unsigned variantTag = 0;
            for (size_t i = 0; i < enumType->variants().size(); ++i) {
                if (enumType->variants()[i].name == variantName) {
                    variantTag = i;
                    break;
                }
            }

            // Lower the enum type to IR
            auto irEnumType = lowerType(exprType);
            if (!irEnumType) {
                reportError("Failed to lower enum type");
                return nullptr;
            }

            // Create enum with no field values
            std::vector<Value*> noFields;
            return builder_.createEnum(irEnumType, variantTag, noFields, variantName);
        }
    }

    // Otherwise it's struct field access
    Value* structVal = generateExpression(expr->object.get());
    if (!structVal) return nullptr;

    // Get the semantic struct type to find field index
    auto objectSemanticType = analyzer_->getExpressionType(expr->object.get());
    auto* semanticStructType = dynamic_cast<const volta::semantic::StructType*>(objectSemanticType.get());

    if (!semanticStructType) {
        reportError("Member access on non-struct/enum type");
        return nullptr;
    }

    // Look up which field this is (e.g., "x" -> 0, "y" -> 1)
    int fieldIndex = semanticStructType->getFieldIndex(expr->member->name);
    if (fieldIndex < 0) {
        reportError("Unknown field: " + expr->member->name);
        return nullptr;
    }

    // Generate the ExtractValue instruction
    return builder_.createExtractValue(structVal, static_cast<unsigned>(fieldIndex), expr->member->name);
}

Value* IRGenerator::generateMethodCallExpr(const ast::MethodCallExpression* expr) {
    // Method calls can be:
    // 1. Enum variant construction: Option.Some(42)
    // 2. Struct method calls (static or instance)

    // Check if object is a type name (for static methods or enum variants)
    if (auto* identExpr = dynamic_cast<const ast::IdentifierExpression*>(expr->object.get())) {
        // Look up the type in semantic analyzer's symbol table
        auto* symTable = const_cast<volta::semantic::SymbolTable*>(analyzer_->getSymbolTable());
        auto symbol = symTable->lookup(identExpr->name);

        if (symbol && symbol->type->kind() == volta::semantic::Type::Kind::Enum) {
            // It's an enum variant construction
            return generateEnumConstruction(expr);
        }

        if (symbol && symbol->type->kind() == volta::semantic::Type::Kind::Struct) {
            auto* structType = dynamic_cast<const volta::semantic::StructType*>(symbol->type.get());

            // Distinguish between type name and variable:
            // - Type name: identExpr->name == structType->name() (e.g., "Point" == "Point")
            // - Variable: identExpr->name != structType->name() (e.g., "p" != "Point")
            if (identExpr->name == structType->name()) {
                // It's a static method call on a struct (e.g., Point.new(1.0, 1.0))
                std::string baseName = structType->name();
                std::string qualifiedName = baseName + "." + expr->method->name;

                // Check if this is a generic struct - if so, we need monomorphization
                if (!structType->typeParams().empty()) {
                    // Get the semantic type of this expression to find concrete type args
                    auto exprType = analyzer_->getExpressionType(expr);
                    if (!exprType) {
                        reportError("No type information for generic method call '" + qualifiedName + "'");
                        return nullptr;
                    }

                    // Extract type arguments from the result type
                    std::vector<std::shared_ptr<volta::semantic::Type>> typeArgs;
                    if (auto* resultStructType = dynamic_cast<const volta::semantic::StructType*>(exprType.get())) {
                        if (resultStructType->isInstantiated()) {
                            typeArgs = resultStructType->typeArgs();
                        }
                    }

                    // If we couldn't infer type args from return type, try from arguments
                    if (typeArgs.empty()) {
                        // Analyze arguments to infer type parameters
                        typeArgs.resize(structType->typeParams().size(), nullptr);

                        // Look up the method's semantic type
                        auto methodSymbol = symTable->lookup(qualifiedName);
                        if (methodSymbol && methodSymbol->type->kind() == volta::semantic::Type::Kind::Function) {
                            auto* fnType = dynamic_cast<const volta::semantic::FunctionType*>(methodSymbol->type.get());

                            // Match argument types with parameter types to infer type variables
                            for (size_t i = 0; i < expr->arguments.size() && i < fnType->paramTypes().size(); ++i) {
                                auto argType = analyzer_->getExpressionType(expr->arguments[i].get());
                                auto paramType = fnType->paramTypes()[i];

                                // If param is a type variable, use arg type
                                if (paramType->kind() == volta::semantic::Type::Kind::TypeVariable) {
                                    auto* typeVar = dynamic_cast<const volta::semantic::TypeVariable*>(paramType.get());
                                    // Find which type parameter this is
                                    for (size_t j = 0; j < structType->typeParams().size(); ++j) {
                                        if (structType->typeParams()[j] == typeVar->name()) {
                                            typeArgs[j] = argType;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Check if we have type args
                    if (typeArgs.empty() || typeArgs[0] == nullptr) {
                        reportError("Failed to infer type arguments for generic method '" + qualifiedName + "'");
                        return nullptr;
                    }

                    // Generate monomorphized version
                    Function* fn = generateMonomorphizedMethod(qualifiedName, typeArgs);
                    if (!fn) {
                        reportError("Failed to generate monomorphized method '" + qualifiedName + "'");
                        return nullptr;
                    }

                    // Generate arguments
                    std::vector<Value*> args;
                    for (const auto& arg : expr->arguments) {
                        Value* argVal = generateExpression(arg.get());
                        if (!argVal) {
                            reportError("Failed to generate argument for monomorphized method call");
                            return nullptr;
                        }
                        args.push_back(argVal);
                    }

                    Value* result = builder_.createCall(fn, args);
                    if (!result) {
                        reportError("Failed to create call to monomorphized method");
                    }
                    return result;
                }

                // Non-generic static method - look up normally
                Function* fn = module_.getFunction(qualifiedName);
                if (!fn) {
                    reportError("Method '" + qualifiedName + "' not found");
                    return nullptr;
                }

                // Generate arguments
                std::vector<Value*> args;
                for (const auto& arg : expr->arguments) {
                    Value* argVal = generateExpression(arg.get());
                    if (!argVal) return nullptr;
                    args.push_back(argVal);
                }

                // Create the call
                return builder_.createCall(fn, args);
            }
            // If we reach here, it's a variable with struct type - fall through to instance method handling
        }
    }

    // Otherwise it's an instance method call on a struct (e.g., point.getX())
    // Generate the object
    Value* object = generateExpression(expr->object.get());
    if (!object) return nullptr;

    // Get the struct type from semantic analysis
    auto objectType = analyzer_->getExpressionType(expr->object.get());
    if (!objectType || objectType->kind() != volta::semantic::Type::Kind::Struct) {
        reportError("Method call on non-struct type");
        return nullptr;
    }

    auto* structType = dynamic_cast<const volta::semantic::StructType*>(objectType.get());

    // Get base name without type arguments for method lookup
    std::string baseName = structType->name();
    size_t bracketPos = baseName.find('[');
    if (bracketPos != std::string::npos) {
        baseName = baseName.substr(0, bracketPos);
    }
    std::string qualifiedName = baseName + "." + expr->method->name;

    Function* fn = nullptr;

    // Check if this is a generic struct instance - need monomorphization
    if (structType->isInstantiated() && !structType->typeArgs().empty()) {
        // Generate monomorphized version with concrete type args
        fn = generateMonomorphizedMethod(qualifiedName, structType->typeArgs());
        if (!fn) {
            reportError("Failed to generate monomorphized method '" + qualifiedName + "'");
            return nullptr;
        }
    } else {
        // Non-generic instance method - look up normally
        fn = module_.getFunction(qualifiedName);
        if (!fn) {
            reportError("Method '" + qualifiedName + "' not found");
            return nullptr;
        }
    }

    // Generate arguments (with object as first argument for instance methods)
    std::vector<Value*> args;
    args.push_back(object);  // 'self' parameter
    for (const auto& arg : expr->arguments) {
        Value* argVal = generateExpression(arg.get());
        if (!argVal) return nullptr;
        args.push_back(argVal);
    }

    // Create the call
    return builder_.createCall(fn, args);
}

Value* IRGenerator::generateEnumConstruction(const ast::MethodCallExpression* expr) {
    // Get the enum type and variant name
    auto* identExpr = dynamic_cast<const ast::IdentifierExpression*>(expr->object.get());
    if (!identExpr) {
        reportError("Invalid enum construction");
        return nullptr;
    }

    const std::string& enumName = identExpr->name;
    const std::string& variantName = expr->method->name;

    // Get the semantic type of this expression (the instantiated enum type)
    auto exprType = analyzer_->getExpressionType(expr);
    if (!exprType) {
        reportError("No type information for enum construction");
        return nullptr;
    }

    auto* enumType = dynamic_cast<const volta::semantic::EnumType*>(exprType.get());
    if (!enumType) {
        reportError("Expression is not an enum type");
        return nullptr;
    }

    // Get the variant
    const auto* variant = enumType->getVariant(variantName);
    if (!variant) {
        reportError("Enum '" + enumName + "' has no variant '" + variantName + "'");
        return nullptr;
    }

    // Find variant index
    unsigned variantTag = 0;
    for (size_t i = 0; i < enumType->variants().size(); ++i) {
        if (enumType->variants()[i].name == variantName) {
            variantTag = i;
            break;
        }
    }

    // Generate field values
    std::vector<Value*> fieldValues;
    for (const auto& arg : expr->arguments) {
        Value* val = generateExpression(arg.get());
        if (!val) {
            reportError("Failed to generate enum variant argument");
            return nullptr;
        }
        fieldValues.push_back(val);
    }

    // Lower the enum type to IR
    auto irEnumType = lowerType(exprType);
    if (!irEnumType) {
        reportError("Failed to lower enum type");
        return nullptr;
    }

    // Create the enum value using CreateEnumInst
    return builder_.createEnum(irEnumType, variantTag, fieldValues, variantName);
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
            Value* oldValue;
            if (dynamic_cast<Argument*>(allocaPtr)) {
                oldValue = allocaPtr;  // Arguments are values, use directly
            } else {
                oldValue = builder_.createLoad(allocaPtr, ident->name);  // Load from alloca
            }
            
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
    // HOW STRUCT FIELD ASSIGNMENT WORKS:
    // Since structs are values in SSA, we can't modify them in place.
    // Instead: Load old struct → InsertValue new field → Store back
    //
    // EXAMPLE IR for: point.x = 10
    //   %point_ptr = <alloca for point>
    //   %old_point = load %point_ptr
    //   %new_point = insertvalue %old_point, 10, 0  ; Update field 0
    //   store %new_point, %point_ptr
    //
    // WHY THIS WAY?
    // - SSA: Values are immutable, can't modify in place
    // - InsertValue: Creates NEW struct with one field changed
    // - Like functional programming: return new object instead of mutating

    if (auto* memberExpr = dynamic_cast<const ast::MemberExpression*>(expr->left.get())) {
        // The object must be a variable (identifier) that we can load/store
        auto* identExpr = dynamic_cast<const ast::IdentifierExpression*>(memberExpr->object.get());
        if (!identExpr) {
            reportError("Can only assign to fields of variables");
            return nullptr;
        }

        // Look up the variable (should be an alloca)
        Value* structPtr = lookupVariable(identExpr->name);
        if (!structPtr) {
            reportError("Undefined variable: " + identExpr->name);
            return nullptr;
        }

        // Get semantic type to find field index
        auto objectSemanticType = analyzer_->getExpressionType(memberExpr->object.get());
        auto* semanticStructType = dynamic_cast<const volta::semantic::StructType*>(objectSemanticType.get());

        if (!semanticStructType) {
            reportError("Field assignment on non-struct type");
            return nullptr;
        }

        int fieldIndex = semanticStructType->getFieldIndex(memberExpr->member->name);
        if (fieldIndex < 0) {
            reportError("Unknown field: " + memberExpr->member->name);
            return nullptr;
        }

        // Generate the new value for the field
        Value* newFieldValue = generateExpression(expr->right.get());
        if (!newFieldValue) return nullptr;

        // Load the old struct value
        Value* oldStruct = builder_.createLoad(structPtr, identExpr->name);

        // Create new struct with updated field
        Value* newStruct = builder_.createInsertValue(oldStruct, newFieldValue,
                                                       static_cast<unsigned>(fieldIndex), "");

        // Store the new struct back
        builder_.createStore(newStruct, structPtr);

        return newFieldValue;  // Assignment returns the assigned value
    }

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

    // Register builtin/native functions with mangled names for overload resolution
    // print(int) -> void
    {
        std::vector<std::shared_ptr<IRType>> params = { module->getIntType() };
        auto returnType = module->getVoidType();
        module->createFunction("print_i64", returnType, params);
    }
    // print(float) -> void
    {
        std::vector<std::shared_ptr<IRType>> params = { module->getFloatType() };
        auto returnType = module->getVoidType();
        module->createFunction("print_f64", returnType, params);
    }
    // print(bool) -> void
    {
        std::vector<std::shared_ptr<IRType>> params = { module->getBoolType() };
        auto returnType = module->getVoidType();
        module->createFunction("print_bool", returnType, params);
    }
    // print(str) -> void
    {
        std::vector<std::shared_ptr<IRType>> params = { module->getStringType() };
        auto returnType = module->getVoidType();
        module->createFunction("print_str", returnType, params);
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

Value* IRGenerator::generateStructLiteral(const ast::StructLiteral* lit) {
    // Generate all field values first
    std::vector<Value*> fieldValues;
    std::vector<std::shared_ptr<ir::IRType>> fieldTypes;

    for (const auto& field : lit->fields) {
        Value* fieldVal = generateExpression(field->value.get());
        if (!fieldVal) return nullptr;
        fieldValues.push_back(fieldVal);
        fieldTypes.push_back(fieldVal->getType());
    }

    // Create IRStructType
    auto structType = std::make_shared<ir::IRStructType>(fieldTypes, std::vector<std::string>{});

    // Allocate struct
    Value* structVal = builder_.createGCAlloc(structType, "struct");

    // Insert each field
    for (size_t i = 0; i < fieldValues.size(); i++) {
        structVal = builder_.createInsertValue(structVal, fieldValues[i], i, "");
    }

    return structVal;
}

Function* IRGenerator::generateMonomorphizedMethod(
    const std::string& baseName,
    const std::vector<std::shared_ptr<volta::semantic::Type>>& typeArgs) {

    // Create mangled name for this instantiation (e.g., "Box.new<int>")
    std::string mangledName = baseName + "<";
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        if (i > 0) mangledName += ",";
        mangledName += typeArgs[i]->toString();
    }
    mangledName += ">";

    // Check if we already generated this monomorphization
    Function* existing = module_.getFunction(mangledName);
    if (existing) {
        return existing;
    }

    // Look up the generic method AST
    auto it = genericMethodCache_.find(baseName);
    if (it == genericMethodCache_.end()) {
        reportError("Generic method '" + baseName + "' not found in cache (have " +
                   std::to_string(genericMethodCache_.size()) + " cached methods)");
        return nullptr;
    }

    const ast::FnDeclaration* decl = it->second;

    // Get the struct type to know the type parameters
    std::string structName = baseName.substr(0, baseName.find('.'));
    auto* symTable = const_cast<volta::semantic::SymbolTable*>(analyzer_->getSymbolTable());
    auto structSymbol = symTable->lookup(structName);
    if (!structSymbol) {
        reportError("Struct '" + structName + "' not found");
        return nullptr;
    }

    const volta::semantic::StructType* structType = nullptr;
    if (structSymbol->type->kind() == volta::semantic::Type::Kind::Struct) {
        structType = dynamic_cast<const volta::semantic::StructType*>(structSymbol->type.get());
    }

    if (!structType || structType->typeParams().empty()) {
        reportError("Not a generic struct: '" + structName + "'");
        return nullptr;
    }

    // Create a substitution map: type parameter name -> concrete type
    std::unordered_map<std::string, std::shared_ptr<volta::semantic::Type>> substitutionMap;
    for (size_t i = 0; i < structType->typeParams().size() && i < typeArgs.size(); ++i) {
        substitutionMap[structType->typeParams()[i]] = typeArgs[i];
    }

    // Helper to substitute types recursively
    std::function<std::shared_ptr<volta::semantic::Type>(const ast::Type*)> substituteType;
    substituteType = [&](const ast::Type* astType) -> std::shared_ptr<volta::semantic::Type> {
        // Simple named type
        if (auto* named = dynamic_cast<const ast::NamedType*>(astType)) {
            const std::string& typeName = named->identifier->name;

            // Check if it's a type parameter
            auto it = substitutionMap.find(typeName);
            if (it != substitutionMap.end()) {
                return it->second;
            }

            // Check if this is the struct name itself (e.g., "Box" in "self: Box")
            // If so, we need to instantiate it with the type arguments
            if (typeName == structName) {
                auto instantiated = structType->instantiate(typeArgs);
                if (instantiated) {
                    return instantiated;
                }
            }

            // Not a type parameter or struct name, resolve normally
            return analyzer_->resolveTypeAnnotation(astType);
        }

        // Generic type like Box[T] - substitute type arguments recursively
        if (auto* generic = dynamic_cast<const ast::GenericType*>(astType)) {
            const std::string& typeName = generic->identifier->name;

            // Recursively substitute type arguments
            std::vector<std::shared_ptr<volta::semantic::Type>> substitutedArgs;
            for (const auto& arg : generic->typeArgs) {
                substitutedArgs.push_back(substituteType(arg.get()));
            }

            // Look up the base type and instantiate with substituted args
            auto symbol = symTable->lookup(typeName);
            if (symbol && symbol->type->kind() == volta::semantic::Type::Kind::Struct) {
                auto* baseStructType = dynamic_cast<const volta::semantic::StructType*>(symbol->type.get());
                if (baseStructType) {
                    auto instantiated = baseStructType->instantiate(substitutedArgs);
                    if (instantiated) {
                        return instantiated;
                    }
                }
            }
        }

        // For other types, resolve normally
        return analyzer_->resolveTypeAnnotation(astType);
    };

    // Resolve and substitute parameter types
    std::vector<std::shared_ptr<IRType>> paramTypes;
    for (const auto& param : decl->parameters) {
        auto semType = substituteType(param->type.get());
        auto irType = lowerType(semType);
        if (!irType) {
            reportError("Failed to lower parameter type in monomorphized method");
            return nullptr;
        }
        paramTypes.push_back(irType);
    }

    // Resolve and substitute return type
    auto semReturnType = substituteType(decl->returnType.get());
    auto returnType = lowerType(semReturnType);
    if (!returnType) {
        reportError("Failed to lower return type in monomorphized method");
        return nullptr;
    }

    // Save current context
    Function* previousFunc = currentFunction_;
    BasicBlock* previousBlock = builder_.getInsertionBlock();

    // Create the monomorphized function
    Function* func = builder_.createFunction(mangledName, returnType, paramTypes);
    currentFunction_ = func;

    // Get the entry block (createFunction creates it automatically)
    // and set insertion point to it
    if (func->getEntryBlock()) {
        builder_.setInsertionPoint(func->getEntryBlock());
    } else {
        // If no entry block, create one
        auto* entryBlock = module_.createBasicBlock("entry", func);
        builder_.setInsertionPoint(entryBlock);
    }

    // Enter function scope
    enterScope();

    // Create allocas for parameters
    for (size_t i = 0; i < decl->parameters.size(); i++) {
        const auto& param = decl->parameters[i];
        Argument* arg = func->getParam(i);

        if (param->isMutable) {
            Value* alloca = builder_.createAlloca(arg->getType(), param->identifier);
            builder_.createStore(arg, alloca);
            declareVariable(param->identifier, alloca);
        } else {
            declareVariable(param->identifier, arg);
        }
    }

    // Generate function body
    if (decl->body) {
        generateBlock(decl->body.get());
    }

    // Add implicit return if missing
    if (returnType->kind() == IRType::Kind::Void) {
        bool hasReturn = func->getEntryBlock() && func->getEntryBlock()->hasTerminator();
        if (!hasReturn && func->getEntryBlock()) {
            builder_.setInsertionPoint(func->getEntryBlock());
            builder_.createRet();
        }
    }
    // Non-void functions should have been validated by semantic analyzer

    exitScope();
    currentFunction_ = previousFunc;
    if (previousBlock) {
        builder_.setInsertionPoint(previousBlock);  // Restore insertion point
    }

    return func;
}

} // namespace volta::ir
