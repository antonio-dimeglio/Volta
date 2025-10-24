#include "HIR/Lowering.hpp"
#include "HIR/HIR.hpp"
#include "Parser/AST.hpp"
#include <iostream>

static std::unique_ptr<Expr> cloneExpr(Expr* expr);

HIR::HIRProgram HIRLowering::lower(Program& ast) {
    std::vector<std::unique_ptr<HIR::HIRStmt>> hirStmts;

    for (auto& stmt : ast.statements) {
        auto lowered = lowerStmt(*stmt);
        if (lowered) {
            hirStmts.push_back(std::move(lowered));
        }
    }

    return HIR::HIRProgram(std::move(hirStmts));
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerStmt(Stmt& stmt) {
    if (auto* varDecl = dynamic_cast<VarDecl*>(&stmt)) {
        return lowerVarDecl(*varDecl);
    } else if (auto* fnDecl = dynamic_cast<FnDecl*>(&stmt)) {
        return lowerFnDecl(*fnDecl);
    } else if (auto* retStmt = dynamic_cast<ReturnStmt*>(&stmt)) {
        return lowerReturnStmt(*retStmt);
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(&stmt)) {
        return lowerIfStmt(*ifStmt);
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(&stmt)) {
        return lowerWhileStmt(*whileStmt);
    } else if (auto* forStmt = dynamic_cast<ForStmt*>(&stmt)) {
        return lowerForStmt(*forStmt);
    } else if (auto* blockStmt = dynamic_cast<BlockStmt*>(&stmt)) {
        return lowerBlockStmt(*blockStmt);
    } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(&stmt)) {
        return lowerExprStmt(*exprStmt);
    } else if (auto* breakStmt = dynamic_cast<BreakStmt*>(&stmt)) {
        return lowerBreakStmt(*breakStmt);
    } else if (auto* continueStmt = dynamic_cast<ContinueStmt*>(&stmt)) {
        return lowerContinueStmt(*continueStmt);
    } else if (auto* externBlock = dynamic_cast<ExternBlock*>(&stmt)) {
        return lowerExternBlock(*externBlock);
    } else if (auto* importStmt = dynamic_cast<ImportStmt*>(&stmt)) {
        return lowerImportStmt(*importStmt);
    } else if (auto* structDecl = dynamic_cast<StructDecl*>(&stmt)) {
        return lowerStructDecl(*structDecl);
    }

    std::cerr << "Warning: Unknown statement type in HIR lowering\n";
    return nullptr;
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerVarDecl(VarDecl& decl) {
    auto desugaredInit = desugarExpr(*decl.initValue);

    return std::make_unique<HIR::HIRVarDecl>(
        decl.name,
        decl.typeAnnotation,
        std::move(desugaredInit),
        decl.mutable_,
        decl.line,
        decl.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerFnDecl(FnDecl& decl) {
    // Convert parameters
    std::vector<HIR::Param> hirParams;
    hirParams.reserve(decl.params.size());
for (auto& param : decl.params) {
        hirParams.emplace_back(param.name, param.type, param.isRef, param.isMutRef, param.isMutable);
    }

    // Lower function body
    std::vector<std::unique_ptr<HIR::HIRStmt>> hirBody;
    for (auto& stmt : decl.body) {
        auto lowered = lowerStmt(*stmt);
        if (lowered) {
            hirBody.push_back(std::move(lowered));
        }
    }

    return std::make_unique<HIR::HIRFnDecl>(
        decl.name,
        std::move(hirParams),
        decl.returnType,
        std::move(hirBody),
        decl.isExtern,
        decl.isPublic,
        decl.line,
        decl.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerIfStmt(IfStmt& stmt) {
    auto desugaredCond = desugarExpr(*stmt.condition);

    // Lower then body
    std::vector<std::unique_ptr<HIR::HIRStmt>> hirThen;
    for (auto& s : stmt.thenBody) {
        auto lowered = lowerStmt(*s);
        if (lowered) {
            hirThen.push_back(std::move(lowered));
        }
    }

    // Lower else body
    std::vector<std::unique_ptr<HIR::HIRStmt>> hirElse;
    for (auto& s : stmt.elseBody) {
        auto lowered = lowerStmt(*s);
        if (lowered) {
            hirElse.push_back(std::move(lowered));
        }
    }

    return std::make_unique<HIR::HIRIfStmt>(
        std::move(desugaredCond),
        std::move(hirThen),
        std::move(hirElse),
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerWhileStmt(WhileStmt& stmt) {
    auto desugaredCond = desugarExpr(*stmt.condition);

    std::vector<std::unique_ptr<HIR::HIRStmt>> hirBody;
    for (auto& s : stmt.thenBody) {
        auto lowered = lowerStmt(*s);
        if (lowered) {
            hirBody.push_back(std::move(lowered));
        }
    }

    return std::make_unique<HIR::HIRWhileStmt>(
        std::move(desugaredCond),
        std::move(hirBody),
        nullptr,  // No increment for regular while
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerForStmt(ForStmt& stmt) {
    // Desugar: for (var in range) { body }
    // Into: { let var = range.from; while (var < range.to) { body; var = var + 1; } }

    auto *range = dynamic_cast<Range*>(stmt.range.get());
    if (range == nullptr) {
        std::cerr << "Error: For loop range is not a Range expression\n";
        return nullptr;
    }

    // Create variable declaration: let var = range.from
    auto varDecl = std::make_unique<HIR::HIRVarDecl>(
        stmt.var->token,
        typeRegistry.getPrimitive(Type::PrimitiveKind::I32),  // For now, assume i32
        cloneExpr(range->from.get()),
        true,  // mutable
        stmt.line,
        stmt.column
    );

    // Create condition: var < range.to
    auto cond = std::make_unique<BinaryExpr>(
        cloneExpr(stmt.var.get()),
        cloneExpr(range->to.get()),
        TokenType::LessThan,
        stmt.line,
        stmt.column
    );

    // Create increment: var = var + 1
    auto one = std::make_unique<Literal>(
        Token(TokenType::Integer, stmt.line, stmt.column, "1")
    );
    auto increment = std::make_unique<BinaryExpr>(
        cloneExpr(stmt.var.get()),
        std::move(one),
        TokenType::Plus,
        stmt.line,
        stmt.column
    );

    // Lower body statements
    std::vector<std::unique_ptr<HIR::HIRStmt>> hirBody;
    for (auto& s : stmt.body) {
        auto lowered = lowerStmt(*s);
        if (lowered) {
            hirBody.push_back(std::move(lowered));
        }
    }

    // Create while loop with increment
    auto whileLoop = std::make_unique<HIR::HIRWhileStmt>(
        std::move(cond),
        std::move(hirBody),
        std::move(increment),
        stmt.line,
        stmt.column
    );

    // Wrap in a block: { var decl; while loop }
    std::vector<std::unique_ptr<HIR::HIRStmt>> blockStmts;
    blockStmts.push_back(std::move(varDecl));
    blockStmts.push_back(std::move(whileLoop));

    return std::make_unique<HIR::HIRBlockStmt>(
        std::move(blockStmts),
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerBlockStmt(BlockStmt& stmt) {
    std::vector<std::unique_ptr<HIR::HIRStmt>> hirStmts;
    for (auto& s : stmt.statements) {
        auto lowered = lowerStmt(*s);
        if (lowered) {
            hirStmts.push_back(std::move(lowered));
        }
    }

    return std::make_unique<HIR::HIRBlockStmt>(
        std::move(hirStmts),
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerReturnStmt(ReturnStmt& stmt) {
    std::unique_ptr<Expr> desugaredValue;
    if (stmt.value) {
        desugaredValue = desugarExpr(*stmt.value);
    }

    return std::make_unique<HIR::HIRReturnStmt>(
        std::move(desugaredValue),
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerExprStmt(ExprStmt& stmt) {
    auto desugared = desugarExpr(*stmt.expr);

    return std::make_unique<HIR::HIRExprStmt>(
        std::move(desugared),
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerBreakStmt(BreakStmt& stmt) {
    return std::make_unique<HIR::HIRBreakStmt>(stmt.line, stmt.column);
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerContinueStmt(ContinueStmt& stmt) {
    return std::make_unique<HIR::HIRContinueStmt>(stmt.line, stmt.column);
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerExternBlock(ExternBlock& block) {
    std::vector<std::unique_ptr<HIR::HIRFnDecl>> hirDecls;

    for (auto& fn : block.declarations) {
        std::vector<HIR::Param> params;
        for (auto& p : fn->params) {
            params.emplace_back(p.name, p.type, p.isRef, p.isMutRef);
        }

        hirDecls.push_back(std::make_unique<HIR::HIRFnDecl>(
            fn->name,
            std::move(params),
            fn->returnType,
            std::vector<std::unique_ptr<HIR::HIRStmt>>(),  // Empty body for extern
            true,  // isExtern
            false, // isPublic (extern declarations are file-scoped)
            fn->line,
            fn->column
        ));
    }

    return std::make_unique<HIR::HIRExternBlock>(
        std::move(hirDecls),
        block.line,
        block.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerImportStmt(ImportStmt& stmt) {
    std::vector<std::string> symbols;
    symbols.reserve(stmt.importedItems.size());
for (auto& sym : stmt.importedItems) {
        symbols.push_back(sym);
    }

    return std::make_unique<HIR::HIRImportStmt>(
        stmt.modulePath,
        std::move(symbols),
        stmt.line,
        stmt.column
    );
}

std::unique_ptr<HIR::HIRStmt> HIRLowering::lowerStructDecl(StructDecl& decl) {
    // Lower methods to HIRFnDecl
    std::vector<std::unique_ptr<HIR::HIRFnDecl>> loweredMethods;
    for (auto& method : decl.methods) {
        auto loweredMethod = lowerFnDecl(*method);
        loweredMethods.push_back(std::unique_ptr<HIR::HIRFnDecl>(
            dynamic_cast<HIR::HIRFnDecl*>(loweredMethod.release())));
    }

    // Struct declarations are lowered with methods
    return std::make_unique<HIR::HIRStructDecl>(
        decl.isPublic,
        decl.name,
        decl.fields,
        std::move(loweredMethods),
        decl.name.line,
        decl.name.column
    );
}

std::unique_ptr<Expr> HIRLowering::desugarExpr(Expr& expr) {
    // Handle compound assignments: a += b  =>  a = a + b
    if (auto* compound = dynamic_cast<CompoundAssign*>(&expr)) {
        auto binOp = std::make_unique<BinaryExpr>(
            cloneExpr(compound->var.get()),
            cloneExpr(compound->value.get()),
            compound->op,
            compound->line,
            compound->column
        );

        return std::make_unique<Assignment>(
            cloneExpr(compound->var.get()),
            std::move(binOp),
            compound->line,
            compound->column
        );
    }

    // Handle increment: a++  =>  a = a + 1
    if (auto* inc = dynamic_cast<Increment*>(&expr)) {
        auto one = std::make_unique<Literal>(
            Token(TokenType::Integer, inc->line, inc->column, "1")
        );

        auto binOp = std::make_unique<BinaryExpr>(
            cloneExpr(inc->var.get()),
            std::move(one),
            TokenType::Plus,
            inc->line,
            inc->column
        );

        return std::make_unique<Assignment>(
            cloneExpr(inc->var.get()),
            std::move(binOp),
            inc->line,
            inc->column
        );
    }

    // Handle decrement: a--  =>  a = a - 1
    if (auto* dec = dynamic_cast<Decrement*>(&expr)) {
        auto one = std::make_unique<Literal>(
            Token(TokenType::Integer, dec->line, dec->column, "1")
        );

        auto binOp = std::make_unique<BinaryExpr>(
            cloneExpr(dec->var.get()),
            std::move(one),
            TokenType::Minus,
            dec->line,
            dec->column
        );

        return std::make_unique<Assignment>(
            cloneExpr(dec->var.get()),
            std::move(binOp),
            dec->line,
            dec->column
        );
    }

    // Handle Point.new() => StaticMethodCall
    // The parser creates InstanceMethodCall for "TypeName.method()" because it can't
    // distinguish type names from variables. Here we desugar it into StaticMethodCall.
    if (auto* instanceCall = dynamic_cast<InstanceMethodCall*>(&expr)) {
        // Check if the object is a Variable node
        if (auto* varNode = dynamic_cast<Variable*>(instanceCall->object.get())) {
            // Check if this variable name is actually a struct type
            const auto* structType = typeRegistry.getStruct(varNode->token.lexeme);

            if (structType != nullptr) {
                // DESUGAR: Transform Point.new() => StaticMethodCall("Point", "new", [...])
                std::vector<std::unique_ptr<Expr>> clonedArgs;
                clonedArgs.reserve(instanceCall->args.size());
for (auto& arg : instanceCall->args) {
                    clonedArgs.push_back(cloneExpr(arg.get()));
                }

                return std::make_unique<StaticMethodCall>(
                    varNode->token,           // type name token
                    instanceCall->methodName, // method name token
                    std::move(clonedArgs),
                    instanceCall->line,
                    instanceCall->column
                );
            }
        }
    }

    // For other expressions, just clone them as-is
    return cloneExpr(&expr);
}

static std::unique_ptr<Expr> cloneExpr(Expr* expr) {
    if (expr == nullptr) {
        std::cerr << "Warning: cloneExpr called with null expression\n";
        return std::make_unique<Literal>(Token(TokenType::Integer, 0, 0, "0"));
    }

    if (auto* lit = dynamic_cast<Literal*>(expr)) {
        return std::make_unique<Literal>(lit->token);
    } else if (auto* var = dynamic_cast<Variable*>(expr)) {
        return std::make_unique<Variable>(var->token);
    } else if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        return std::make_unique<BinaryExpr>(
            cloneExpr(bin->lhs.get()),
            cloneExpr(bin->rhs.get()),
            bin->op,
            bin->line,
            bin->column
        );
    } else if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        return std::make_unique<UnaryExpr>(
            cloneExpr(unary->operand.get()),
            unary->op,
            unary->line,
            unary->column
        );
    } else if (auto* call = dynamic_cast<FnCall*>(expr)) {
        std::vector<std::unique_ptr<Expr>> clonedArgs;
        clonedArgs.reserve(call->args.size());
for (auto& arg : call->args) {
            clonedArgs.push_back(cloneExpr(arg.get()));
        }
        return std::make_unique<FnCall>(
            call->name,
            std::move(clonedArgs),
            call->line,
            call->column
        );
    } else if (auto* assign = dynamic_cast<Assignment*>(expr)) {
        return std::make_unique<Assignment>(
            cloneExpr(assign->lhs.get()),
            cloneExpr(assign->value.get()),
            assign->line,
            assign->column
        );
    } else if (auto* group = dynamic_cast<GroupingExpr*>(expr)) {
        return std::make_unique<GroupingExpr>(
            cloneExpr(group->expr.get()),
            group->line,
            group->column
        );
    } else if (auto* arrLit = dynamic_cast<ArrayLiteral*>(expr)) {
        // Handle repeat syntax: [value; count]
        if (arrLit->repeat_value && arrLit->repeat_count) {
            return std::make_unique<ArrayLiteral>(
                cloneExpr(arrLit->repeat_value.get()),
                *arrLit->repeat_count,
                arrLit->line,
                arrLit->column
            );
        }
        // Handle regular array: [e1, e2, e3, ...]
        std::vector<std::unique_ptr<Expr>> clonedElems;
        clonedElems.reserve(arrLit->elements.size());
        for (auto& elem : arrLit->elements) {
            clonedElems.push_back(cloneExpr(elem.get()));
        }
        return std::make_unique<ArrayLiteral>(
            std::move(clonedElems),
            arrLit->line,
            arrLit->column
        );
    } else if (auto* idx = dynamic_cast<IndexExpr*>(expr)) {
        std::vector<std::unique_ptr<Expr>> clonedIndices;
        for (const auto& indexExpr : idx->index) {
            clonedIndices.push_back(cloneExpr(indexExpr.get()));
        }
        return std::make_unique<IndexExpr>(
            cloneExpr(idx->array.get()),
            std::move(clonedIndices),
            idx->line,
            idx->column
        );
    } else if (auto* addrOf = dynamic_cast<AddrOf*>(expr)) {
        return std::make_unique<AddrOf>(
            cloneExpr(addrOf->operand.get()),
            addrOf->line,
            addrOf->column
        );
    } else if (auto* structLit = dynamic_cast<StructLiteral*>(expr)) {
        std::vector<std::pair<Token, std::unique_ptr<Expr>>> clonedFields;
        clonedFields.reserve(structLit->fields.size());
for (auto& [fieldName, fieldValue] : structLit->fields) {
            clonedFields.emplace_back(fieldName, cloneExpr(fieldValue.get()));
        }
        return std::make_unique<StructLiteral>(
            structLit->structName,
            std::move(clonedFields),
            structLit->line,
            structLit->column
        );
    } else if (auto* fieldAccess = dynamic_cast<FieldAccess*>(expr)) {
        return std::make_unique<FieldAccess>(
            cloneExpr(fieldAccess->object.get()),
            fieldAccess->fieldName,
            fieldAccess->line,
            fieldAccess->column
        );
    } else if (auto* staticCall = dynamic_cast<StaticMethodCall*>(expr)) {
        std::vector<std::unique_ptr<Expr>> clonedArgs;
        clonedArgs.reserve(staticCall->args.size());
for (auto& arg : staticCall->args) {
            clonedArgs.push_back(cloneExpr(arg.get()));
        }
        return std::make_unique<StaticMethodCall>(
            staticCall->typeName,
            staticCall->methodName,
            std::move(clonedArgs),
            staticCall->line,
            staticCall->column
        );
    } else if (auto* instanceCall = dynamic_cast<InstanceMethodCall*>(expr)) {
        std::vector<std::unique_ptr<Expr>> clonedArgs;
        clonedArgs.reserve(instanceCall->args.size());
for (auto& arg : instanceCall->args) {
            clonedArgs.push_back(cloneExpr(arg.get()));
        }
        return std::make_unique<InstanceMethodCall>(
            cloneExpr(instanceCall->object.get()),
            instanceCall->methodName,
            std::move(clonedArgs),
            instanceCall->line,
            instanceCall->column
        );
    }

    std::cerr << "Warning: cloneExpr doesn't handle this expression type: " << typeid(*expr).name() << "\n";
    // Return a dummy expression to avoid crashes
    return std::make_unique<Literal>(
        Token(TokenType::Integer, 0, 0, "0")
    );
}
