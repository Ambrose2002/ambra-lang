#include "lowering.h"

#include "ast/expr.h"
#include "ast/stmt.h"
#include "sema/analyzer.h"
void LoweringContext::lowerExpression(const Expr* expr)
{
    switch (expr->kind)
    {
    case IntLiteral:
    {
        const auto* e = static_cast<const IntLiteralExpr*>(expr);
        lowerIntExpr(e);
        return;
    }
    case BoolLiteral:
    {
        const auto* e = static_cast<const BoolLiteralExpr*>(expr);
        lowerBoolExpr(e);
        return;
    }
    case InterpolatedString:
    {
        const auto* e = static_cast<const StringExpr*>(expr);

        lowerStringExpr(e);

        return;
    }
    case Identifier:
    {
        const auto* e = static_cast<const IdentifierExpr*>(expr);

        lowerIdentifierExpr(e);
        return;
    }
    case Grouping:
    {
        const auto* e = static_cast<const GroupingExpr*>(expr);
        lowerGroupingExpr(e);
        return;
    }
    case Unary:
    {
        const auto* e = static_cast<const UnaryExpr*>(expr);
        lowerUnaryExpr(e);
        return;
    }
    case Binary:
    {
        const auto* e = static_cast<const BinaryExpr*>(expr);
        lowerBinaryExpr(e);
        return;
    }
    default:
        break;
    }
}

void LoweringContext::lowerIntExpr(const IntLiteralExpr* e)
{
    // 1. Allocate constant ID
    ConstId cid = program->nextConstId;
    program->nextConstId.value++;

    program->constants.emplace_back(Constant{I32, cid, e->getValue()});
    currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
    return;
}
void LoweringContext::lowerStringExpr(const StringExpr* e)
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
        lowerExpression(first.expr.get());
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
            lowerExpression(part.expr.get());
            currentFunction->instructions.emplace_back(Instruction{ToString, Operand{}});
        }

        currentFunction->instructions.emplace_back(Instruction{ConcatString, Operand{}});
    }
    return;
}
void LoweringContext::lowerBoolExpr(const BoolLiteralExpr* e)
{
    // 1. Allocate constant ID
    ConstId cid = program->nextConstId;
    program->nextConstId.value++;

    program->constants.emplace_back(Constant{Bool32, cid, e->getValue()});
    currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
    return;
}
void LoweringContext::lowerIdentifierExpr(const IdentifierExpr* e)
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
    return;
}
void LoweringContext::lowerUnaryExpr(const UnaryExpr* e)
{
    const auto& operand = e->getOperand();

    lowerExpression(&operand);
    auto opCode = e->getOperator() == LogicalNot ? NotBool : NegI32;
    currentFunction->instructions.emplace_back(Instruction{opCode, Operand{}});
    return;
}
void LoweringContext::lowerBinaryExpr(const BinaryExpr* e)
{
    const auto& left = e->getLeft();
    const auto& right = e->getRight();

    const auto op = e->getOperator();

    lowerExpression(&left);
    lowerExpression(&right);

    switch (op)
    {
    case EqualEqual:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpEqI32, Operand{}});
        return;
    }
    case NotEqual:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpNEqI32, Operand{}});
        return;
    }
    case Greater:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpGtI32, Operand{}});
        return;
    }
    case GreaterEqual:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpGtEqI32, Operand{}});
        return;
    }
    case Less:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpLtI32, Operand{}});
        return;
    }
    case LessEqual:
    {
        currentFunction->instructions.emplace_back(Instruction{CmpLtEqI32, Operand{}});
        return;
    }
    case Add:
    {
        currentFunction->instructions.emplace_back(Instruction{AddI32, Operand{}});
        return;
    }
    case Subtract:
    {
        currentFunction->instructions.emplace_back(Instruction{SubI32, Operand{}});
        return;
    }
    case Multiply:
    {
        currentFunction->instructions.emplace_back(Instruction{MulI32, Operand{}});
        return;
    }
    case Divide:
    {
        currentFunction->instructions.emplace_back(Instruction{DivI32, Operand{}});
        return;
    }
    default:
        hadError = true;
        return;
    }
}
void LoweringContext::lowerGroupingExpr(const GroupingExpr* e)
{
    lowerExpression(e);
    return;
}

void LoweringContext::lowerStatement(const Stmt* stmt)
{
    switch (stmt->kind)
    {
    case Summon:
    {
        const auto* s = static_cast<const SummonStmt*>(stmt);
        LocalId     lId = currentFunction->nextLocalId;
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

        lowerExpression(&s->getInitializer());
        currentFunction->instructions.emplace_back(Instruction{StoreLocal, Operand{lId}});
        return;
    }
    default:
        hadError = true;
        return;
    }
}