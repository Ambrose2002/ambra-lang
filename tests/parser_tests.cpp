#include "ast/expr.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>

bool isEqualExpression(std::unique_ptr<Expr>& e1, std::unique_ptr<Expr>& e2)
{
    if (!(e1->kind == e2->kind))
    {
        return false;
    }

    switch (e1->kind)
    {
    case IntLiteral:
    {
        auto* int1 = static_cast<IntLiteralExpr*>(e1.get());
        auto* int2 = static_cast<IntLiteralExpr*>(e2.get());
        return int1 == int2;
    }
    case BoolLiteral:
    {
        auto* bool1 = static_cast<BoolLiteralExpr*>(e1.get());
        auto* bool2 = static_cast<BoolLiteralExpr*>(e2.get());
        return bool1 == bool2;
    }
    case StringLiteral:
    {
        auto* string1 = static_cast<StringLiteralExpr*>(e1.get());
        auto* string2 = static_cast<StringLiteralExpr*>(e2.get());
        return string1 == string2;
    }
    case Variable:
    {
        auto* var1 = static_cast<VariableExpr*>(e1.get());
        auto* var2 = static_cast<VariableExpr*>(e2.get());
        return var1 == var2;
    }
    case Unary:
    {
        auto* u1 = static_cast<UnaryExpr*>(e1.get());
        auto* u2 = static_cast<UnaryExpr*>(e2.get());
        return u1 == u2;
    }
    case Binary:
    {
        auto* b1 = static_cast<BinaryExpr*>(e1.get());
        auto* b2 = static_cast<BinaryExpr*>(e2.get());
        return b1 == b2;
    }
    case Grouping:
    {
        auto* group1 = static_cast<GroupingExpr*>(e1.get());
        auto* group2 = static_cast<GroupingExpr*>(e2.get());
        return group1 == group2;
    }
    case InterpolatedString:
    {
        auto* string1 = static_cast<InterpolatedStringExpr*>(e1.get());
        auto* string2 = static_cast<InterpolatedStringExpr*>(e2.get());
        return string1 == string2;
    }
    default:
        return false;
    }
    return true;
}

TEST(Trial, trial)
{
    std::cout << "First test";
}