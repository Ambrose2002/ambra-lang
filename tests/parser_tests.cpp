#include "ast/expr.h"
#include "parser/parser.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <variant>

bool isEqualExpression(std::unique_ptr<Expr>& e1, std::unique_ptr<Expr>& e2)
{
    if (e1 == e2) return true;
    if (!e1 || !e2) return false;
    return *e1 == *e2;
}

TEST(SingleToken, IntLiteral)
{
    std::vector<Token> tokens = {
        Token("40", INTEGER, 42, 1, 1),
        Token("", EOF_TOKEN, std::monostate{}, 1, 3),
    };

    std::unique_ptr<Expr> expected = std::make_unique<IntLiteralExpr>(42, 1, 1);

    Parser parser(tokens);

    auto results = parser.parseExpression();

    ASSERT_TRUE(isEqualExpression(results, expected));
}