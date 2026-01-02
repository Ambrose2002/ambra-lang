#include "lowering.h"

#include "ast/expr.h"
#include "sema/analyzer.h"
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

    // ---- Step 1: lower first part (must leave 1 String on stack)
    const StringPart& first = e->getParts()[0];
    if (first.kind == StringPart::TEXT)
    {
        // 1. Allocate constant ID
        ConstId cid = program->nextConstId;
        program->nextConstId.value++;
        program->constants.emplace_back(Constant{String32, cid, first.text});
        currentFunction->instructions.emplace_back(
            Instruction{PushConst, Operand{cid}});
    }
    else
    {
        lowerExpression(first.expr.get());
        currentFunction->instructions.emplace_back(
            Instruction{ToString, Operand{}});
    }

    // ---- Step 2: process remaining parts
    for (size_t i = 1; i < e->getParts().size(); i++)
    {
        const StringPart& part = e->getParts()[i];

        if (part.kind == StringPart::TEXT)
        {
            // 1. Allocate constant ID
        ConstId cid = program->nextConstId;
        program->nextConstId.value++;
            program->constants.emplace_back(Constant{String32, cid, part.text});
            currentFunction->instructions.emplace_back(
                Instruction{PushConst, Operand{cid}});
        }
        else
        {
            lowerExpression(part.expr.get());
            currentFunction->instructions.emplace_back(
                Instruction{ToString, Operand{}});
        }

        // ---- Concatenate (stack: [..., str1, str2] → [..., str])
        currentFunction->instructions.emplace_back(
            Instruction{ConcatString, Operand{}});
    }

    return;
}
    default:
        break;
    }
}