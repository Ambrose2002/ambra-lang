#include "ast/expr.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>

bool isEqualExpression(std::unique_ptr<Expr>& e1, std::unique_ptr<Expr>& e2)
{
    if (e1 == e2) return true;
    if (!e1 || !e2) return false;
    return *e1 == *e2;
}

TEST(Trial, trial)
{
    std::cout << "First test";
}