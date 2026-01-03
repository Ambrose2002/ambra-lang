#include "lowering.h"

#include "ast/expr.h"
#include "ast/stmt.h"
#include "sema/analyzer.h"

#include <unordered_map>

void LoweringContext::defineLabel(LabelId id)
{
    // record label -> current instruction index (the position of the JLabel instruction)
    size_t ip = currentFunction->instructions.size();

    // prevent duplicate label definitions (programming error)
    if (currentFunction->labelTable.position.find(id) != currentFunction->labelTable.position.end())
    {
        hadError = true;
        return;
    }

    currentFunction->labelTable.position.emplace(id, ip);
}

void LoweringContext::emitLabel(LabelId id)
{
    defineLabel(id);
    currentFunction->instructions.emplace_back(Instruction{JLabel, Operand{id}});
}

void LoweringContext::lowerExpression(const Expr* expr, Type expectedType)
{
    switch (expr->kind)
    {
    case IntLiteral:
    {
        const auto* e = static_cast<const IntLiteralExpr*>(expr);
        lowerIntExpr(e, expectedType);
        break;
    }
    case BoolLiteral:
    {
        const auto* e = static_cast<const BoolLiteralExpr*>(expr);
        lowerBoolExpr(e, expectedType);
        break;
    }
    case InterpolatedString:
    {
        const auto* e = static_cast<const StringExpr*>(expr);

        lowerStringExpr(e, expectedType);

        break;
    }
    case Identifier:
    {
        const auto* e = static_cast<const IdentifierExpr*>(expr);

        lowerIdentifierExpr(e, expectedType);
        return;
    }
    case Grouping:
    {
        const auto* e = static_cast<const GroupingExpr*>(expr);
        lowerGroupingExpr(e, expectedType);
        break;
    }
    case Unary:
    {
        const auto* e = static_cast<const UnaryExpr*>(expr);
        lowerUnaryExpr(e, expectedType);
        break;
    }
    case Binary:
    {
        const auto* e = static_cast<const BinaryExpr*>(expr);
        lowerBinaryExpr(e, expectedType);
        break;
    }
    default:
        hadError = true;
        break;
    }
}

void LoweringContext::lowerIntExpr(const IntLiteralExpr* e, Type expectedType)
{
    // 1. Allocate constant ID
    ConstId cid = program->nextConstId;
    program->nextConstId.value++;

    program->constants.emplace_back(Constant{I32, cid, e->getValue()});
    currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
    if (expectedType == String)
    {
        currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
    }
    return;
}
void LoweringContext::lowerStringExpr(const StringExpr* e, Type expectedType)
{
    const StringPart& first = e->getParts()[0];
    if (first.kind == StringPart::TEXT)
    {
        // 1. Allocate constant ID
        ConstId cid = program->nextConstId;
        program->nextConstId.value++;
        program->constants.emplace_back(Constant{String32, cid, first.text});
        currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
    }
    else
    {
        lowerExpression(first.expr.get(), Void);
        currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
    }

    for (size_t i = 1; i < e->getParts().size(); i++)
    {
        const StringPart& part = e->getParts()[i];

        if (part.kind == StringPart::TEXT)
        {
            // 1. Allocate constant ID
            ConstId cid = program->nextConstId;
            program->nextConstId.value++;
            program->constants.emplace_back(Constant{String32, cid, part.text});
            currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
        }
        else
        {
            lowerExpression(part.expr.get(), Void);
            currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
        }

        currentFunction->instructions.emplace_back(Instruction{ConcatString, Operand{}});
    }
    return;
}

void LoweringContext::lowerBoolExpr(const BoolLiteralExpr* e, Type expectedType)
{
    // 1. Allocate constant ID
    ConstId cid = program->nextConstId;
    program->nextConstId.value++;

    program->constants.emplace_back(Constant{Bool32, cid, e->getValue()});
    currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});

    if (expectedType == String)
    {
        currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
    }
    return;
}

void LoweringContext::lowerIdentifierExpr(const IdentifierExpr* e, Type expectedType)
{
    auto it = resolutionTable.mapping.find(e);

    if (it == resolutionTable.mapping.end())
    {
        hadError = true;
        return;
    }

    const Symbol* symbol = it->second;

    LocalId lId;
    bool    found = false;

    for (auto scopeIt = localScopes.rbegin(); scopeIt != localScopes.rend(); scopeIt++)
    {
        auto foundIt = scopeIt->find(symbol);

        if (foundIt != scopeIt->end())
        {
            lId = foundIt->second;
            found = true;
            break;
        }
    }

    if (!found)
    {
        hadError = true;
        return;
    }
    currentFunction->instructions.emplace_back(Instruction{LoadLocal, Operand{lId}});
    auto typeIt = typeTable.mapping.find(e);
    if (typeIt == typeTable.mapping.end())
    {
        hadError = true;
        return;
    }
    if (typeIt->second != expectedType)
    {
        if (expectedType == String)
        {
            currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
        }
    }
    return;
}

void LoweringContext::lowerUnaryExpr(const UnaryExpr* e, Type expectedType)
{
    const auto& operand = e->getOperand();

    Type operandType = (e->getOperator() == LogicalNot) ? Bool : Int;
    lowerExpression(&operand, operandType);

    auto opCode = e->getOperator() == LogicalNot ? NotBool : NegI32;
    currentFunction->instructions.emplace_back(Instruction{opCode, Operand{}});

    if (expectedType == String)
        currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
    return;
}

void LoweringContext::lowerBinaryExpr(const BinaryExpr* e, Type expectedType)
{
    const auto& left = e->getLeft();
    const auto& right = e->getRight();

    const auto op = e->getOperator();

    Type operandType;
    switch (op)
    {
    case Add:
    case Subtract:
    case Multiply:
    case Divide:
    case Greater:
    case GreaterEqual:
    case Less:
    case LessEqual:
        operandType = Int;
        break;

    case EqualEqual:
    case NotEqual:
        operandType = typeTable.mapping.at(&left);
        break;

    default:
        hadError = true;
        return;
    }

    lowerExpression(&left, operandType);
    lowerExpression(&right, operandType);

    switch (op)
    {
    case EqualEqual:
        switch (operandType)
        {
        case Int:
            currentFunction->instructions.emplace_back(Instruction{CmpEqI32, Operand{}});
            break;
        case Bool:
            currentFunction->instructions.emplace_back(Instruction{CmpEqBool32, Operand{}});
            break;
        case String:
            currentFunction->instructions.emplace_back(Instruction{CmpEqString32, Operand{}});
            break;
        default:
            hadError = true;
            break;
        }
        break;
    case NotEqual:
        switch (operandType)
        {
        case Int:
            currentFunction->instructions.emplace_back(Instruction{CmpNEqI32, Operand{}});
            break;
        case Bool:
            currentFunction->instructions.emplace_back(Instruction{CmpNEqBool32, Operand{}});
            break;
        case String:
            currentFunction->instructions.emplace_back(Instruction{CmpNEqString32, Operand{}});
            break;
        default:
            hadError = true;
            break;
        }
        break;
    case Greater:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpGtI32, Operand{}});
        break;
    }
    case GreaterEqual:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpGtEqI32, Operand{}});
        break;
    }
    case Less:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpLtI32, Operand{}});
        break;
    }
    case LessEqual:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpLtEqI32, Operand{}});
        break;
    }
    case Add:
    {
        currentFunction->instructions.emplace_back(Instruction{AddI32, Operand{}});
        break;
    }
    case Subtract:
    {
        currentFunction->instructions.emplace_back(Instruction{SubI32, Operand{}});
        break;
    }
    case Multiply:
    {
        currentFunction->instructions.emplace_back(Instruction{MulI32, Operand{}});
        break;
    }
    case Divide:
    {
        currentFunction->instructions.emplace_back(Instruction{DivI32, Operand{}});
        break;
    }
    default:
        hadError = true;
        break;
    }

    if (expectedType == String)
        currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
    return;
}

void LoweringContext::lowerGroupingExpr(const GroupingExpr* e, Type expectedType)
{
    lowerExpression(&e->getExpression(), expectedType);
    return;
}

void LoweringContext::lowerStatement(const Stmt* stmt)
{
    switch (stmt->kind)
    {
    case Summon:
    {
        const auto* s = static_cast<const SummonStmt*>(stmt);
        lowerSummonStatement(s);
        return;
    }
    case Say:
    {
        const auto* s = static_cast<const SayStmt*>(stmt);
        lowerSayStatement(s);
        return;
    }
    case Block:
    {
        const auto* s = static_cast<const BlockStmt*>(stmt);
        lowerBlockStatement(s);
        return;
    }
    case IfChain:
    {
        const auto* s = static_cast<const IfChainStmt*>(stmt);
        lowerIfChainStatement(s);
        return;
    }
    case While:
    {
        const auto* s = static_cast<const WhileStmt*>(stmt);
        lowerWhileStatement(s);
        return;
    }
    default:
        hadError = true;
        return;
    }
}

void LoweringContext::lowerSummonStatement(const SummonStmt* s)
{
    LocalId lId = currentFunction->nextLocalId;
    currentFunction->nextLocalId.value++;

    LocalInfo localInfo;
    localInfo.id = lId;
    localInfo.debugName = s->getIdentifier().getName();
    localInfo.declLoc = s->loc;

    IrType t;
    auto   typeIt = typeTable.mapping.find(&s->getInitializer());
    if (typeIt == typeTable.mapping.end())
    {
        hadError = true;
        return;
    }
    switch (typeIt->second)
    {
    case Int:
    {
        localInfo.type = I32;
        break;
    }
    case Bool:
    {
        localInfo.type = Bool32;
        break;
    }
    case String:
    {
        localInfo.type = String32;
        break;
    }
    case Void:
    {
        localInfo.type = Void32;
        break;
    }
    default:
        hadError = true;
        return;
    }
    currentFunction->localTable.locals.emplace_back(localInfo);

    auto symbol = s->getSymbol();
    localScopes.back()[symbol] = lId;

    lowerExpression(&s->getInitializer(), typeIt->second);
    currentFunction->instructions.emplace_back(Instruction{StoreLocal, Operand{lId}});
    return;
}

void LoweringContext::lowerSayStatement(const SayStmt* stmt)
{
    lowerExpression(&stmt->getExpression(), String);
    currentFunction->instructions.emplace_back(Instruction{PrintString, Operand{}});
    return;
}

void LoweringContext::lowerBlockStatement(const BlockStmt* stmt)
{
    localScopes.emplace_back();
    for (auto& stmt : *stmt)
    {
        lowerStatement(stmt.get());
    }
    localScopes.pop_back();
}

void LoweringContext::lowerIfChainStatement(const IfChainStmt* stmt)
{
    std::vector<LabelId> nextLabels;

    // Allocate labels for fallthrough between branches
    for (size_t i = 0; i < stmt->getBranches().size(); ++i)
    {
        nextLabels.push_back(currentFunction->nextLabelId);
        currentFunction->nextLabelId.value++;
    }

    // Final exit label
    LabelId endLabel = currentFunction->nextLabelId;
    currentFunction->nextLabelId.value++;

    const auto& branches = stmt->getBranches();

    for (size_t i = 0; i < branches.size(); ++i)
    {
        const auto& [cond, block] = branches[i];

        lowerExpression(cond.get(), Bool);

        currentFunction->instructions.emplace_back(
            Instruction{JumpIfFalse, Operand{nextLabels[i]}});

        lowerBlockStatement(block.get());

        // Jump to end after executing this branch
        currentFunction->instructions.emplace_back(Instruction{Jump, Operand{endLabel}});

        // Emit label for next branch
        emitLabel(nextLabels[i]);
    }

    if (stmt->getElseBranch())
    {
        lowerBlockStatement(stmt->getElseBranch().get());
    }

    emitLabel(endLabel);
}

void LoweringContext::lowerWhileStatement(const WhileStmt* stmt)
{

    LabelId loopLabel = currentFunction->nextLabelId;
    currentFunction->nextLabelId.value++;

    LabelId endLabel = currentFunction->nextLabelId;
    currentFunction->nextLabelId.value++;

    // push loop start label
    emitLabel(loopLabel);

    lowerExpression(&stmt->getCondition(), Bool);
    // jump to end if false
    currentFunction->instructions.emplace_back(Instruction{JumpIfFalse, Operand{endLabel}});

    lowerBlockStatement(&stmt->getBody());

    // jump back to loop start
    currentFunction->instructions.emplace_back(Instruction{Jump, Operand{loopLabel}});

    // push loop end label
    emitLabel(endLabel);
}

IrProgram LoweringContext::lowerProgram(const Program* prog)
{
    IrProgram result;

    program = &result;
    currentFunction = &result.main;

    localScopes.clear();
    localScopes.emplace_back();

    for (auto& stmt : *prog)
    {
        lowerStatement(stmt.get());
    }

    localScopes.pop_back();

    program = nullptr;
    currentFunction = nullptr;

    return result;
}