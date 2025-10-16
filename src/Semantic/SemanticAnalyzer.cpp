#include "../../include/Semantic/SemanticAnalyzer.hpp"
#include "../../include/Parser/AST.hpp"
#include <sstream>

namespace Semantic {

// ============================================================================
// Constructor
// ============================================================================

SemanticAnalyzer::SemanticAnalyzer(TypeRegistry& type_registry, DiagnosticManager& diag)
    : type_registry(type_registry),
      symbol_table(type_registry),
      diag(diag),
      current_return_type(nullptr),
      in_loop(false) {}

void SemanticAnalyzer::analyzeProgram(const HIR::Program& program) {
    for (const auto& stmt : program.statements) {
        if (auto* fnDecl = dynamic_cast<const FnDecl*>(stmt.get())) {
            collectFnDecl(fnDecl);
        }
    }

    for (const auto& stmt : program.statements) {
        analyzeStmt(stmt.get());
    }
}

void SemanticAnalyzer::collectFnDecl(const FnDecl* fn) {
    std::vector<FunctionParameter> parameters;

    for (const auto& param : fn->params) {
        const Type* interned_type = type_registry.internFromAST(param.type.get());

        parameters.emplace_back(
            param.name,
            interned_type,
            param.isRef,
            param.isMutRef
        );
    }

    const Type* return_type = type_registry.internFromAST(fn->returnType.get());

    FunctionSignature signature(parameters, return_type);

    if (!symbol_table.addFunction(fn->name, signature)) {
        std::ostringstream oss;
        oss << "Function '" << fn->name << "' is already defined";
        diag.error(oss.str(), 0, 0);
    }
}

void SemanticAnalyzer::analyzeStmt(const Stmt* stmt) {
    if (auto* fnDecl = dynamic_cast<const FnDecl*>(stmt)) {
        analyzeFnDecl(fnDecl);
    } else if (auto* varDecl = dynamic_cast<const VarDecl*>(stmt)) {
        analyzeVarDecl(varDecl);
    } else if (auto* returnStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        analyzeReturn(returnStmt);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        analyzeIf(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        analyzeWhile(whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        analyzeFor(forStmt);
    } else if (auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        analyzeBlock(blockStmt);
    } else if (auto* breakStmt = dynamic_cast<const BreakStmt*>(stmt)) {
        analyzeBreak(breakStmt);
    } else if (auto* continueStmt = dynamic_cast<const ContinueStmt*>(stmt)) {
        analyzeContinue(continueStmt);
    } else if (auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        analyzeExpr(exprStmt->expr.get());
    } else {
        diag.error("Unknown statement type in semantic analysis", 0, 0);
    }
}

void SemanticAnalyzer::analyzeFnDecl(const FnDecl* fn) {
    symbol_table.enterFunction();
    current_function = fn->name;

    const FunctionSignature* sig = symbol_table.lookupFunction(fn->name);
    current_return_type = sig->return_type;

    for (const auto& param : fn->params) {

        const Type* interned_type = type_registry.internFromAST(param.type.get());

        bool is_mutable;
        if (param.isMutRef) {
            is_mutable = true;
        } else if (param.isRef) {
            is_mutable = false;
        } else {

            is_mutable = true;
        }

        std::cerr << "[DEBUG] Defining parameter: " << param.name << " as " << (is_mutable ? "mutable" : "immutable") << std::endl;
        if (!symbol_table.define(param.name, interned_type, is_mutable)) {
            std::ostringstream oss;
            oss << "Parameter '" << param.name << "' is already defined";
            diag.error(oss.str(), 0, 0);
        }
    }

    for (const auto& stmt : fn->body) {
        analyzeStmt(stmt.get());
    }

    current_function = "";
    current_return_type = nullptr;
    symbol_table.exitFunction();
}

void SemanticAnalyzer::analyzeVarDecl(const VarDecl* varDecl) {
    std::string name = varDecl->name.lexeme;

    const Type* declared_type;

    if (varDecl->type_annotation) {
        declared_type = type_registry.internFromAST(varDecl->type_annotation.get());

        if (varDecl->init_value) {
            const Type* expr_type = analyzeExpr(varDecl->init_value.get());
            validateTypeCompatibility(
                declared_type,
                expr_type,
                "Initializer for variable '" + name + "'"
            );
        }
    } else {
        declared_type = analyzeExpr(varDecl->init_value.get());
        std::cerr << "[DEBUG] Inferred type for '" << name << "': " << (declared_type ? declared_type->toString() : "NULL") << std::endl;
    }

    if (!symbol_table.define(name, declared_type, varDecl->mutable_)) {
        std::ostringstream oss;
        oss << "Variable '" << name << "' is already defined in current scope";
        // DEBUG
        std::cerr << "[DEBUG] Failed to define variable: " << name << " (already exists)" << std::endl;
        diag.error(oss.str(), varDecl->name.line, varDecl->name.column);
    } else {
        // DEBUG
        std::cerr << "[DEBUG] Successfully defined variable: " << name << std::endl;
    }
}

void SemanticAnalyzer::analyzeReturn(const ReturnStmt* returnStmt) {
    if (!current_return_type) {
        diag.error("Return statement outside of function", 0, 0);
        return;
    }

    if (!returnStmt->value) {
        const Type* void_type = type_registry.getPrimitive(PrimitiveTypeKind::Void);
        if (current_return_type != void_type) {
            std::ostringstream oss;
            oss << "Function '" << current_function << "' expects return type "
                << current_return_type->toString() << ", but got void return";
            diag.error(oss.str(), 0, 0);
        }
    } else {
        const Type* void_type = type_registry.getPrimitive(PrimitiveTypeKind::Void);
        if (current_return_type == void_type) {
            std::ostringstream oss;
            oss << "Function '" << current_function << "' is void, cannot return a value";
            diag.error(oss.str(), 0, 0);
        }

        const Type* actual_type = analyzeExpr(returnStmt->value.get());
        validateTypeCompatibility(
            current_return_type,
            actual_type,
            "Return value for function '" + current_function + "'"
        );
    }
}

void SemanticAnalyzer::analyzeIf(const IfStmt* ifStmt) {
    const Type* condType = inferExprType(ifStmt->condition.get());

    const Type* bool_type = type_registry.getPrimitive(PrimitiveTypeKind::Bool);
    if (condType != bool_type) {
        std::ostringstream oss;
        oss << "Non-boolean condition for if statement. Got " << condType->toString();
        diag.error(oss.str(), 0, 0);
    }

    // Create new scope for then-body
    symbol_table.enterScope();
    for (const auto& stmt : ifStmt->thenBody) {
        analyzeStmt(stmt.get());
    }
    symbol_table.exitScope();

    if (!ifStmt->elseBody.empty()) {
        // Create new scope for else-body
        symbol_table.enterScope();
        for (const auto& stmt : ifStmt->elseBody) {
            analyzeStmt(stmt.get());
        }
        symbol_table.exitScope();
    }
}

void SemanticAnalyzer::analyzeWhile(const WhileStmt* whileStmt) {
    const Type* condType = inferExprType(whileStmt->condition.get());

    const Type* bool_type = type_registry.getPrimitive(PrimitiveTypeKind::Bool);
    if (condType != bool_type) {
        std::ostringstream oss;
        oss << "Non-boolean condition for while statement. Got " << condType->toString();
        diag.error(oss.str(), 0, 0);
    }

    bool was_in_loop = in_loop;
    in_loop = true;

    // Create new scope for while-body
    symbol_table.enterScope();
    for (const auto& stmt : whileStmt->thenBody) {
        analyzeStmt(stmt.get());
    }
    symbol_table.exitScope();

    in_loop = was_in_loop;
}

void SemanticAnalyzer::analyzeFor(const ForStmt* forStmt) {
    // For-loops should be fully desugared in HIR before semantic analysis
    // If we reach this point, something went wrong in the desugaring phase
    (void)forStmt;  // Unused
    diag.error("Internal error: ForStmt should be desugared before semantic analysis",
               forStmt->line, forStmt->column);
}

void SemanticAnalyzer::analyzeBreak(const BreakStmt* breakStmt) {
    (void)breakStmt;  // Unused
    if (!in_loop) {
        diag.error("'break' statement outside of loop", 0, 0);
    }
}

void SemanticAnalyzer::analyzeContinue(const ContinueStmt* continueStmt) {
    (void)continueStmt;  // Unused
    if (!in_loop) {
        diag.error("'continue' statement outside of loop", 0, 0);
    }
}

void SemanticAnalyzer::analyzeBlock(const BlockStmt* blockStmt) {
    // Create new scope for the block
    symbol_table.enterScope();

    // Analyze all statements in the block
    for (const auto& stmt : blockStmt->statements) {
        analyzeStmt(stmt.get());
    }

    // Exit scope
    symbol_table.exitScope();
}


const Type* SemanticAnalyzer::analyzeExpr(const Expr* expr) {
    if (auto* binaryExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        validateBinaryExpr(binaryExpr);
    } else if (auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        validateUnaryExpr(unaryExpr);
    } else if (auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        validateFnCall(fnCall);
    } else if (auto* assignment = dynamic_cast<const Assignment*>(expr)) {
        validateAssignment(assignment);
    } else if (auto* indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        validateIndexExpr(indexExpr);
    } else if (auto* arrayLiteral = dynamic_cast<const ArrayLiteral*>(expr)) {
        validateArrayLiteral(arrayLiteral);
    } else if (auto* increment = dynamic_cast<const Increment*>(expr)) {
        validateIncrement(increment);
    } else if (auto* decrement = dynamic_cast<const Decrement*>(expr)) {
        validateDecrement(decrement);
    } else if (auto* compoundAssign = dynamic_cast<const CompoundAssign*>(expr)) {
        validateCompoundAssign(compoundAssign);
    }

    return inferExprType(expr);
}

const Type* SemanticAnalyzer::inferExprType(const Expr* expr) {
    if (auto* literal = dynamic_cast<const Literal*>(expr)) {
        return type_registry.inferLiteral(literal);
    }

    if (auto* variable = dynamic_cast<const Variable*>(expr)) {
        const Symbol* symbol = symbol_table.lookup(variable->token.lexeme);
        if (!symbol) {
            std::ostringstream oss;
            oss << "Undefined variable '" << variable->token.lexeme << "'";
            diag.error(oss.str(), variable->token.line, variable->token.column);
            return type_registry.getPrimitive(PrimitiveTypeKind::Void);
        }
        return symbol->type;
    }

    if (auto* arrayLiteral = dynamic_cast<const ArrayLiteral*>(expr)) {
        return inferArrayLiteralType(arrayLiteral);
    }

    if (auto* binaryExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        const Type* left_type = inferExprType(binaryExpr->lhs.get());
        const Type* right_type = inferExprType(binaryExpr->rhs.get());
        const Type* result_type = type_registry.inferBinaryOp(left_type, binaryExpr->op, right_type);

        if (!result_type) {
            std::ostringstream oss;
            oss << "Invalid binary operation: " << left_type->toString()
                << " " << tokenTypeToString(binaryExpr->op) << " "
                << right_type->toString();
            diag.error(oss.str(), 0, 0);
            return type_registry.getPrimitive(PrimitiveTypeKind::Void);
        }
        return result_type;
    }

    if (auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        const Type* operand_type = inferExprType(unaryExpr->operand.get());
        const Type* result_type = type_registry.inferUnaryOp(unaryExpr->op, operand_type);

        if (!result_type) {
            std::ostringstream oss;
            oss << "Invalid unary operation: " << tokenTypeToString(unaryExpr->op)
                << " " << operand_type->toString();
            diag.error(oss.str(), 0, 0);
            return type_registry.getPrimitive(PrimitiveTypeKind::Void);
        }
        return result_type;
    }

    if (auto* grouping = dynamic_cast<const GroupingExpr*>(expr)) {
        return inferExprType(grouping->expr.get());
    }

    if (auto* fnCall = dynamic_cast<const FnCall*>(expr)) {
        const FunctionSignature* signature = symbol_table.lookupFunction(fnCall->name);
        if (!signature) {
            std::ostringstream oss;
            oss << "Undefined function '" << fnCall->name << "'";
            diag.error(oss.str(), 0, 0);
            return type_registry.getPrimitive(PrimitiveTypeKind::Void);
        }
        return signature->return_type;
    }

    if (auto* assignment = dynamic_cast<const Assignment*>(expr)) {
        return inferExprType(assignment->value.get());
    }

    if (auto* indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        const Type* array_type = inferExprType(indexExpr->array.get());
        const Type* element_type = type_registry.getArrayElementType(array_type);

        if (!element_type) {
            std::ostringstream oss;
            oss << "Cannot index into non-array type " << array_type->toString();
            diag.error(oss.str(), indexExpr->line, indexExpr->column);
            return type_registry.getPrimitive(PrimitiveTypeKind::Void);
        }

        return element_type;
    }

    if (auto* increment = dynamic_cast<const Increment*>(expr)) {
        return inferExprType(increment->var.get());
    }

    if (auto* decrement = dynamic_cast<const Decrement*>(expr)) {
        return inferExprType(decrement->var.get());
    }

    if (auto* compoundAssign = dynamic_cast<const CompoundAssign*>(expr)) {
        return inferExprType(compoundAssign->var.get());
    }

    if (auto* range = dynamic_cast<const Range*>(expr)) {
        // Range expressions themselves don't have a type, they're only used in for loops
        return type_registry.getPrimitive(PrimitiveTypeKind::Void);
    }

    diag.error("Unknown expression type in type inference", expr->line, expr->column);
    return type_registry.getPrimitive(PrimitiveTypeKind::Void);
}

void SemanticAnalyzer::validateBinaryExpr(const BinaryExpr* expr) {
    analyzeExpr(expr->lhs.get());
    analyzeExpr(expr->rhs.get());
}

void SemanticAnalyzer::validateUnaryExpr(const UnaryExpr* expr) {
    analyzeExpr(expr->operand.get());
}

void SemanticAnalyzer::validateFnCall(const FnCall* expr) {
    const FunctionSignature* signature = symbol_table.lookupFunction(expr->name);

    if (!signature) {
        std::ostringstream oss;
        oss << "Function '" << expr->name << "' is not defined";
        diag.error(oss.str(), 0, 0);
        return;
    }

    // Check argument count
    if (expr->args.size() != signature->parameters.size()) {
        std::ostringstream oss;
        oss << "Function '" << expr->name << "' expects "
            << signature->parameters.size() << " arguments, got " << expr->args.size();
        diag.error(oss.str(), 0, 0);
        return;
    }

    for (size_t i = 0; i < expr->args.size(); ++i) {
        const Expr* arg_expr = expr->args[i].get();
        const FunctionParameter& param = signature->parameters[i];

        const Type* arg_type = analyzeExpr(arg_expr);
        std::ostringstream context;
        context << "Argument " << (i + 1) << " (" << param.name
                << ") of function '" << expr->name << "'";
        validateTypeCompatibility(param.type, arg_type, context.str());

        // If parameter is mut ref, ensure argument is a mutable variable
        if (param.is_mut_ref) {
            auto* var = dynamic_cast<const Variable*>(arg_expr);
            if (!var) {
                std::ostringstream oss;
                oss << "Argument " << (i + 1) << " to 'mut ref' parameter '"
                    << param.name << "' must be a variable";
                diag.error(oss.str(), 0, 0);
            } else {
                const Symbol* symbol = symbol_table.lookup(var->token.lexeme);
                if (symbol && !symbol->is_mut) {
                    std::ostringstream oss;
                    oss << "Argument " << (i + 1) << ": cannot pass immutable variable '"
                        << var->token.lexeme << "' to 'mut ref' parameter '" << param.name << "'";
                    diag.error(oss.str(), var->token.line, var->token.column);
                }
            }
        }
    }
}

void SemanticAnalyzer::validateAssignment(const Assignment* expr) {
    if (auto* var = dynamic_cast<const Variable*>(expr->lhs.get())) {
        std::string var_name = var->token.lexeme;

        const Symbol* symbol = symbol_table.lookup(var_name);

        if (!symbol) {
            std::ostringstream oss;
            oss << "Undefined variable '" << var_name << "'";
            diag.error(oss.str(), var->token.line, var->token.column);
            return;
        }

        if (!symbol->is_mut) {
            std::ostringstream oss;
            oss << "Cannot assign to immutable variable '" << var_name << "'";
            diag.error(oss.str(), var->token.line, var->token.column);
        }

        const Type* value_type = analyzeExpr(expr->value.get());
        validateTypeCompatibility(
            symbol->type,
            value_type,
            "Assignment to variable '" + var_name + "'"
        );
    }

    else if (auto* idx = dynamic_cast<const IndexExpr*>(expr->lhs.get())) {

        const Type* array_type = analyzeExpr(idx->array.get());
        const Type* index_type = analyzeExpr(idx->index.get());

        // Get element type
        const Type* element_type = type_registry.getArrayElementType(array_type);
        if (!element_type) {
            diag.error("Cannot index into non-array type", 0, 0);
            return;
        }

        if (!type_registry.isInteger(index_type)) {
            diag.error("Array index must be an integer", 0, 0);
        }

        if (auto* arrVar = dynamic_cast<const Variable*>(idx->array.get())) {
            const Symbol* symbol = symbol_table.lookup(arrVar->token.lexeme);
            if (symbol && !symbol->is_mut) {
                std::ostringstream oss;
                oss << "Cannot assign to element of immutable array '" << arrVar->token.lexeme << "'";
                diag.error(oss.str(), arrVar->token.line, arrVar->token.column);
            }
        }

        const Type* value_type = analyzeExpr(expr->value.get());

        int line = 0, col = 0;
        if (auto* arrVar = dynamic_cast<const Variable*>(idx->array.get())) {
            line = arrVar->token.line;
            col = arrVar->token.column;
        }

        
        if (element_type != value_type && !type_registry.areTypesCompatible(element_type, value_type)) {
            std::ostringstream oss;
            oss << "Assignment to array element: expected " << element_type->toString()
                << ", got " << value_type->toString();
            diag.error(oss.str(), line, col);
        }
    }
}

void SemanticAnalyzer::validateIndexExpr(const IndexExpr* expr) {
    const Type* array_type = analyzeExpr(expr->array.get());
    const Type* index_type = analyzeExpr(expr->index.get());

    if (!type_registry.isArray(array_type)) {
        std::ostringstream oss;
        oss << "Cannot index into non-array type " << array_type->toString();
        diag.error(oss.str(), 0, 0);
    }

    if (!type_registry.isInteger(index_type)) {
        std::ostringstream oss;
        oss << "Array index must be an integer type, got " << index_type->toString();
        diag.error(oss.str(), 0, 0);
    }
}

void SemanticAnalyzer::validateArrayLiteral(const ArrayLiteral* expr) {
    if (expr->repeat_value) {
        analyzeExpr(expr->repeat_value.get());
    } else {
        for (const auto& elem : expr->elements) {
            analyzeExpr(elem.get());
        }
    }
}

void SemanticAnalyzer::validateIncrement(const Increment* expr) {
    const Symbol* symbol = symbol_table.lookup(expr->var->token.lexeme);

    if (!symbol) {
        std::ostringstream oss;
        oss << "Undefined variable '" << expr->var->token.lexeme << "'";
        diag.error(oss.str(), expr->var->token.line, expr->var->token.column);
        return;
    }

    if (!symbol->is_mut) {
        std::ostringstream oss;
        oss << "Cannot increment immutable variable '" << expr->var->token.lexeme << "'";
        diag.error(oss.str(), expr->line, expr->column);
    }

    if (!type_registry.isInteger(symbol->type)) {
        std::ostringstream oss;
        oss << "Cannot increment non-integer variable '" << expr->var->token.lexeme << "' of type " << symbol->type->toString();
        diag.error(oss.str(), expr->line, expr->column);
    }
}

void SemanticAnalyzer::validateDecrement(const Decrement* expr) {
    const Symbol* symbol = symbol_table.lookup(expr->var->token.lexeme);

    if (!symbol) {
        std::ostringstream oss;
        oss << "Undefined variable '" << expr->var->token.lexeme << "'";
        diag.error(oss.str(), expr->var->token.line, expr->var->token.column);
        return;
    }

    if (!symbol->is_mut) {
        std::ostringstream oss;
        oss << "Cannot decrement immutable variable '" << expr->var->token.lexeme << "'";
        diag.error(oss.str(), expr->line, expr->column);
    }

    if (!type_registry.isInteger(symbol->type)) {
        std::ostringstream oss;
        oss << "Cannot decrement non-integer variable '" << expr->var->token.lexeme << "' of type " << symbol->type->toString();
        diag.error(oss.str(), expr->line, expr->column);
    }
}

void SemanticAnalyzer::validateCompoundAssign(const CompoundAssign* expr) {
    const Symbol* symbol = symbol_table.lookup(expr->var->token.lexeme);

    if (!symbol) {
        std::ostringstream oss;
        oss << "Undefined variable '" << expr->var->token.lexeme << "'";
        diag.error(oss.str(), expr->var->token.line, expr->var->token.column);
        return;
    }

    if (!symbol->is_mut) {
        std::ostringstream oss;
        oss << "Cannot compound-assign to immutable variable '" << expr->var->token.lexeme << "'";
        diag.error(oss.str(), expr->line, expr->column);
    }

    const Type* value_type = analyzeExpr(expr->value.get());
    validateTypeCompatibility(
        symbol->type,
        value_type,
        "Compound assignment to variable '" + expr->var->token.lexeme + "'"
    );
}

const Type* SemanticAnalyzer::inferArrayLiteralType(const ArrayLiteral* arr) {
    if (arr->repeat_value) {
        const Type* elem_type = inferExprType(arr->repeat_value.get());
        return type_registry.internArray(elem_type, *arr->repeat_count);
    }

    if (arr->elements.empty()) {
        diag.error("Cannot infer type of empty array literal", 0, 0);
        return type_registry.internArray(
            type_registry.getPrimitive(PrimitiveTypeKind::Void), 0
        );
    }

    const Type* elem_type = inferExprType(arr->elements[0].get());

    for (size_t i = 1; i < arr->elements.size(); ++i) {
        const Type* elem_i_type = inferExprType(arr->elements[i].get());
        if (elem_type != elem_i_type) {  // Pointer equality for interned types!
            std::ostringstream oss;
            oss << "Array literal element " << i << " has type "
                << elem_i_type->toString() << ", expected " << elem_type->toString();
            diag.error(oss.str(), 0, 0);
        }
    }

    return type_registry.internArray(elem_type, arr->elements.size());
}

void SemanticAnalyzer::validateTypeCompatibility(const Type* expected_type,
                                                 const Type* actual_type,
                                                 const std::string& context) {
    if (expected_type == actual_type) {
        return;
    }

    if (type_registry.areTypesCompatible(expected_type, actual_type)) {
        return;
    }

    std::ostringstream oss;
    oss << context << ": expected " << expected_type->toString()
        << ", got " << actual_type->toString();
    diag.error(oss.str(), 0, 0);
}

}
