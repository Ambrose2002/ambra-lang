#include "lowering.h"

#include "ast/expr.h"
void LoweringContext::lowerExpression(const Expr* expr)
{
    switch (expr->kind)
    {
    case IntLiteral:
    {
        const auto* e = static_cast<const IntLiteralExpr*>(expr);

        // 1. Allocate constant ID
        ConstId cid = program->nextConstId;
        program->nextConstId.value++;

        program->constants.emplace_back(Constant{I32, cid, e->getValue()});
        currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
        return;
    }
    case BoolLiteral:
    {
        const auto* e = static_cast<const BoolLiteralExpr*>(expr);

        // 1. Allocate constant ID
        ConstId cid = program->nextConstId;
        program->nextConstId.value++;

        program->constants.emplace_back(Constant{Bool32, cid, e->getValue()});
        currentFunction->instructions.emplace_back(Instruction{PushConst, Operand{cid}});
        return;
    }
    case InterpolatedString:
    {
        const auto* e = static_cast<const StringExpr*>(expr);

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
    case Identifier:
    {
        const auto* e = static_cast<const IdentifierExpr*>(expr);

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
    case Grouping:
    {
        const auto* e = static_cast<const GroupingExpr*>(expr);
        lowerExpression(e);
        return;
    }
    case Unary:
    {
        const auto* e = static_cast<const UnaryExpr*>(expr);
        const auto& operand = e->getOperand();

        lowerExpression(&operand);
        auto opCode = e->getOperator() == LogicalNot ? NotBool : NegI32;
        currentFunction->instructions.emplace_back(Instruction{opCode, Operand{}});
        return;
    }
    default:
        break;
    }
}